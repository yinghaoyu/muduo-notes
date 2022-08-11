#include "convey/base/AsyncLogging.h"
#include "convey/base/LogFile.h"
#include "convey/base/Timestamp.h"

#include <stdio.h>

using namespace convey;

AsyncLogging::AsyncLogging(const string &basename, off_t rollSize, int flushInterval)
    : flushInterval_(flushInterval),
      running_(false),
      basename_(basename),
      rollSize_(rollSize),
      thread_(std::bind(&AsyncLogging::threadFunc, this), "Logging"),
      latch_(1),
      mutex_(),
      cond_(mutex_),
      currentBuffer_(new Buffer),
      nextBuffer_(new Buffer),
      buffers_()
{
  currentBuffer_->bzero();
  nextBuffer_->bzero();
  buffers_.reserve(16);
}

void AsyncLogging::append(const char *logline, int len)
{
  // 前端线程调用append，需要加锁
  convey::MutexGuard lock(mutex_);
  if (currentBuffer_->avail() > len)
  {
    // 当前缓冲区容量够用
    currentBuffer_->append(logline, len);
  }
  else
  {
    // 当前缓冲器容量不够用
    // 把当前缓冲区加入缓冲块
    buffers_.push_back(std::move(currentBuffer_));

    if (nextBuffer_)
    {
      // 预分配缓冲区没使用，给当前缓冲区
      currentBuffer_ = std::move(nextBuffer_);
    }
    else
    {
      // 预分配缓冲区都用完了，当前缓冲区重新分配
      // 这种情况出现概率很小，写日志太频繁才会出现
      currentBuffer_.reset(new Buffer);  // Rarely happens
    }
    currentBuffer_->append(logline, len);
    cond_.notify();  // 通知日志线程可以写文件
  }
}

void AsyncLogging::threadFunc()
{
  assert(running_ == true);
  latch_.countDown();
  LogFile output(basename_, rollSize_, false);
  BufferPtr newBuffer1(new Buffer);
  BufferPtr newBuffer2(new Buffer);
  newBuffer1->bzero();
  newBuffer2->bzero();
  BufferVector buffersToWrite;
  buffersToWrite.reserve(16);
  while (running_)
  {
    // 后端线程写文件
    assert(newBuffer1 && newBuffer1->length() == 0);
    assert(newBuffer2 && newBuffer2->length() == 0);
    assert(buffersToWrite.empty());

    {
      convey::MutexGuard lock(mutex_);
      if (buffers_.empty())  // unusual usage!
      {
        // spurious weakup 并不会影响
        cond_.waitForSeconds(flushInterval_);
      }
      // 把当前缓冲区加入缓冲块
      buffers_.push_back(std::move(currentBuffer_));
      // 把预分配的newBuffer1给当前缓冲区
      currentBuffer_ = std::move(newBuffer1);
      // 缓冲块取出，准备写文件
      buffersToWrite.swap(buffers_);
      if (!nextBuffer_)
      {
        // 预分配的缓冲区被使用了，那就把newBuffer2给预分配的缓冲区
        nextBuffer_ = std::move(newBuffer2);
      }
    }

    assert(!buffersToWrite.empty());

    // 当缓冲块过大，就drop一些日志，避免堆溢出
    if (buffersToWrite.size() > 25)
    {
      char buf[256];
      snprintf(buf, sizeof buf, "Dropped log messages at %s, %zd larger buffers\n", Timestamp::now().toFormattedString().c_str(), buffersToWrite.size() - 2);
      fputs(buf, stderr);
      output.append(buf, static_cast<int>(strlen(buf)));
      buffersToWrite.erase(buffersToWrite.begin() + 2, buffersToWrite.end());
    }

    // 写缓冲块
    for (const auto &buffer : buffersToWrite)
    {
      // FIXME: use unbuffered stdio FILE ? or use ::writev ?
      output.append(buffer->data(), buffer->length());
    }
    if (buffersToWrite.size() > 2)
    {
      // 写完缓冲块后，丢弃无效数据的缓冲区，只保留两块
      // drop non-bzero-ed buffers, avoid trashing
      buffersToWrite.resize(2);
    }

    // 把两块无效数据的缓冲区，给预分配的newBuffer1，newBuffer2
    // 用std::move避免了重新分配和构造
    if (!newBuffer1)
    {
      assert(!buffersToWrite.empty());
      newBuffer1 = std::move(buffersToWrite.back());
      buffersToWrite.pop_back();
      newBuffer1->reset();
    }

    if (!newBuffer2)
    {
      assert(!buffersToWrite.empty());
      newBuffer2 = std::move(buffersToWrite.back());
      buffersToWrite.pop_back();
      newBuffer2->reset();
    }

    buffersToWrite.clear();
    output.flush();
  }
  output.flush();
}
