#ifndef CONVEY_THREAD_H
#define CONVEY_THREAD_H

#include "Atomic.h"
#include "CountDownLatch.h"
#include "noncopyable.h"

#include <pthread.h>
#include <functional>

namespace convey
{
class Thread : public noncopyable
{
 public:
  typedef std::function<void()> ThreadFunc;
  explicit Thread(ThreadFunc, const std::string name);
  ~Thread();

  void start();
  int join();
  bool isStarted() const { return started_; }
  pid_t tid() const { return tid_; }
  const std::string &name() const { return name_; }

  static int numCreated() { return numCreated_.get(); }

 private:
  static void *startThread(void *);
  void runInThread();
  void setDefaultName();
  bool started_;          // 是否开始运行
  bool joined_;           // 是否停止
  pthread_t pthread_;     // 系统分配的线程结构
  pid_t tid_;             // 线程号
  ThreadFunc func_;       // 线程执行的函数
  std::string name_;      // 线程名
  CountDownLatch latch_;  // 保证 new thread first，从而得到有效的m_threadId
  static AtomicInt32 numCreated_;
};

}  // namespace convey
#endif
