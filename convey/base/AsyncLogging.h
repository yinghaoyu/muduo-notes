#ifndef CONVEY_BASE_ASYNCLOGGING_H
#define CONVEY_BASE_ASYNCLOGGING_H

#include "convey/base/BlockingQueue.h"
#include "convey/base/BoundedBlockingQueue.h"
#include "convey/base/CountDownLatch.h"
#include "convey/base/LogStream.h"
#include "convey/base/Mutex.h"
#include "convey/base/Thread.h"

#include <atomic>
#include <vector>

namespace convey
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

  typedef convey::detail::FixedBuffer<convey::detail::kLargeBuffer> Buffer;
  typedef std::vector<std::unique_ptr<Buffer>> BufferVector;
  typedef BufferVector::value_type BufferPtr;

  const int flushInterval_;       // 刷新缓冲区的间隔
  std::atomic<bool> running_;     // 是否写日志
  const string basename_;         // 基础文件名
  const off_t rollSize_;          // 达到峰值，滚动日志
  convey::Thread thread_;         // 写日志的线程
  convey::CountDownLatch latch_;  // 倒计时
  convey::Mutex mutex_;
  convey::Condition cond_;
  BufferPtr currentBuffer_;  //
  BufferPtr nextBuffer_;     // 预分配的buffer
  BufferVector buffers_;     // 缓冲buffer
};

}  // namespace convey

#endif
