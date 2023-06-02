#ifndef MUDUO_BASE_ASYNCLOGGING_H
#define MUDUO_BASE_ASYNCLOGGING_H

#include "muduo/base/BlockingQueue.h"
#include "muduo/base/BoundedBlockingQueue.h"
#include "muduo/base/CountDownLatch.h"
#include "muduo/base/LogStream.h"
#include "muduo/base/Mutex.h"
#include "muduo/base/Thread.h"

#include <atomic>
#include <vector>

namespace muduo
{
class AsyncLogging : noncopyable
{
 public:
  AsyncLogging(const string &basename, off_t rollSize, int flushInterval = 3);

  ~AsyncLogging()
  {
    if (running_)
    {
      stop();
    }
  }

  void append(const char *logline, int len);

  void start()
  {
    running_ = true;
    thread_.start();
    // 等日志线程函数执行，
    latch_.wait();
  }

  void stop()
  {
    running_ = false;
    // 日志线程可能在wait，虽然有timeout，但是notify会更快唤醒
    cond_.notify();
    thread_.join();
  }

 private:
  void threadFunc();

  typedef muduo::detail::FixedBuffer<muduo::detail::kLargeBuffer> Buffer;
  typedef std::vector<std::unique_ptr<Buffer>> BufferVector;
  typedef BufferVector::value_type BufferPtr;

  const int flushInterval_;       // 刷新缓冲区的间隔
  std::atomic<bool> running_;     // 是否写日志
  const string basename_;         // 基础文件名
  const off_t rollSize_;          // 达到峰值，滚动日志
  muduo::Thread thread_;         // 写日志的线程
  muduo::CountDownLatch latch_;  // 倒计时
  muduo::Mutex mutex_;
  muduo::Condition cond_;
  BufferPtr currentBuffer_;  //
  BufferPtr nextBuffer_;     // 预分配的buffer
  BufferVector buffers_;     // 缓冲buffer
};

}  // namespace muduo

#endif
