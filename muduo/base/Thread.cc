#include "muduo/base/Thread.h"
#include "muduo/base/CurrentThread.h"
#include "muduo/base/Exception.h"
#include "muduo/base/Timestamp.h"

#include <sys/prctl.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace muduo
{
namespace detail
{
pid_t getTid()
{
  return static_cast<pid_t>(::syscall(SYS_gettid));
}

void afterFork()
{
  muduo::CurrentThread::t_cachedTid = 0;
  muduo::CurrentThread::t_threadName = "main";
  CurrentThread::tid();
}

class ThreadNameInitializer
{
 public:
  ThreadNameInitializer()
  {
    muduo::CurrentThread::t_threadName = "main";
    CurrentThread::tid();
    pthread_atfork(NULL, NULL, &afterFork);
  }
};

// 通过全局变量RAII方式初始化线程局部变量
ThreadNameInitializer init;

struct ThreadData
{
  typedef muduo::Thread::ThreadFunc ThreadFunc;
  ThreadFunc func_;
  string name_;
  pid_t *tid_;
  CountDownLatch *latch_;

  ThreadData(ThreadFunc func, const string &name, pid_t *tid, CountDownLatch *latch) : func_(std::move(func)), name_(name), tid_(tid), latch_(latch) {}

  void runInThread()
  {
    *tid_ = muduo::CurrentThread::tid();
    tid_ = NULL;
    latch_->countDown();
    latch_ = NULL;

    muduo::CurrentThread::t_threadName = name_.empty() ? "Thread" : name_.c_str();
    ::prctl(PR_SET_NAME, muduo::CurrentThread::t_threadName);
    try
    {
      func_();
      muduo::CurrentThread::t_threadName = "finished";
    }
    catch (const Exception &ex)
    {
      muduo::CurrentThread::t_threadName = "crashed";
      fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
      fprintf(stderr, "reason: %s\n", ex.what());
      fprintf(stderr, "stack trace: %s\n", ex.stackTrace());
      abort();
    }
    catch (const std::exception &ex)
    {
      muduo::CurrentThread::t_threadName = "crashed";
      fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
      fprintf(stderr, "reason: %s\n", ex.what());
      abort();
    }
    catch (...)
    {
      muduo::CurrentThread::t_threadName = "crashed";
      fprintf(stderr, "unknown exception caught in Thread %s\n", name_.c_str());
      throw;  // rethrow
    }
  }
};

void *startThread(void *obj)
{
  ThreadData *data = static_cast<ThreadData *>(obj);
  data->runInThread();
  delete data;
  return NULL;
}

}  // namespace detail

void CurrentThread::cachedTid()
{
  if (t_cachedTid == 0)
  {
    t_cachedTid = detail::getTid();
    t_tidStringLength = snprintf(t_tidString, sizeof(t_tidString), "%5d ", t_cachedTid);
  }
}

bool CurrentThread::isMainThread()
{
  // linux主线程的tid就是进程的pid
  return tid() == ::getpid();
}

void CurrentThread::sleepUsec(int64_t usec)
{
  struct timespec ts = {0, 0};
  ts.tv_sec = static_cast<time_t>(usec / Timestamp::kMicroSecondsPerSecond);
  ts.tv_nsec = static_cast<long>(usec % Timestamp::kMicroSecondsPerSecond * 1000);
  ::nanosleep(&ts, NULL);
}

// 初始化局部静态变量
AtomicInt32 Thread::numCreated_;

Thread::Thread(ThreadFunc func, const string &name) : started_(false), joined_(false), pthread_(0), tid_(0), func_(std::move(func)), name_(name), latch_(1)
{
  setDefaultName();
}

Thread::~Thread()
{
  if (started_ && !joined_)
  {
    // 如果线程没有正常joined就detach
    // 假如Thread类先析构，通过detach使得主线程与子线程分离
    // detach不会终结子线程，子线程结束后，资源自动回收
    pthread_detach(pthread_);
  }
}

void Thread::setDefaultName()
{
  int num = numCreated_.incrementAndGet();
  if (name_.empty())
  {
    char buf[32];
    snprintf(buf, sizeof(buf), "Thread%d", num);
    name_ = buf;
  }
}

void Thread::start()
{
  assert(!started_);
  started_ = true;
  detail::ThreadData *data = new detail::ThreadData(func_, name_, &tid_, &latch_);
  if (pthread_create(&pthread_, NULL, &detail::startThread, data))
  {
    started_ = false;
    delete data;
  }
  else
  {
    latch_.wait();
    assert(tid_ > 0);
  }
}

int Thread::join()
{
  assert(started_);
  assert(!joined_);
  joined_ = true;
  return pthread_join(pthread_, NULL);
}
}  // namespace muduo
