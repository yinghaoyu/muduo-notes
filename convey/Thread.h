#ifndef THREAD_H
#define THREAD_H

#include "Atomic.h"
#include "CountDownLatch.h"
#include "noncopyable.h"

#include <pthread.h>
#include <functional>

class Thread : public noncopyable
{
 public:
  typedef std::function<void()> ThreadFunc;
  explicit Thread(ThreadFunc, const std::string name);
  ~Thread();

  void start();
  int join();
  bool isStarted() const { return m_started; }

 private:
  static void *startThread(void *);
  void runInThread();
  void setDefaultName();
  bool m_started;          // 是否开始运行
  bool m_joined;           // 是否停止
  pthread_t m_pthread;     // 系统分配的线程结构
  pid_t m_tid;             // 线程号
  ThreadFunc m_func;       // 线程执行的函数
  std::string m_name;      // 线程名
  CountDownLatch m_latch;  // 保证 new thread first，从而得到有效的m_threadId
  static AtomicInt32 s_numCreated;
};

#endif
