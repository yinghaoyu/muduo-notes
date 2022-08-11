#ifndef CONVEY_BASE_MUTEX_H
#define CONVEY_BASE_MUTEX_H

#include <assert.h>
#include <pthread.h>

#include "convey/base/CurrentThread.h"
#include "convey/base/noncopyable.h"

#define MCHECK(ret)      \
  ({                     \
    auto errnum = (ret); \
    assert(errnum == 0); \
    (void) errnum;       \
  })

namespace convey
{
class Mutex : public noncopyable
{
 public:
  Mutex() : holder_(0) { MCHECK(pthread_mutex_init(&mutex_, NULL)); }
  ~Mutex()
  {
    assert(holder_ == 0);
    MCHECK(pthread_mutex_destroy(&mutex_));
  }

  bool isLockedByThisThread() const { return holder_ == CurrentThread::tid(); }

  void assertLocked() const { assert(isLockedByThisThread()); }

  void lock()
  {
    MCHECK(pthread_mutex_lock(&mutex_));
    // 这里需要先上锁再attach持有者
    attachHolder();
  }

  void unlock()
  {
    // 这里需要先detach持有者再解锁
    // 如果先解锁，有可能会调度到其他线程运行，该线程获取了锁，成了新的持有者
    // 后面再到本线程运行detachHolder明显出错
    detachHolder();
    MCHECK(pthread_mutex_unlock(&mutex_));
  }

  pthread_mutex_t *getMutexPtr() { return &mutex_; }

 private:
  // 把Condition声明成友元类，就可以访问DetchGuard类
  friend class Condition;

  class DetachGuard : public noncopyable
  {
   public:
    explicit DetachGuard(Mutex &owner) : owner_(owner) { owner_.detachHolder(); }

    ~DetachGuard() { owner_.attachHolder(); }

   private:
    Mutex &owner_;
  };

  void detachHolder() { holder_ = 0; }
  void attachHolder() { holder_ = CurrentThread::tid(); }

 private:
  pthread_mutex_t mutex_;  // 系统分配的锁资源
  pid_t holder_;           // 锁的持有者，线程id
};

class MutexGuard : public noncopyable
{
  // 利用RAII加锁解锁
 public:
  explicit MutexGuard(Mutex &mutex) : mutex_(mutex) { mutex.lock(); }
  ~MutexGuard() { mutex_.unlock(); }

 private:
  Mutex &mutex_;  // 这里只是一个引用，不管理生命周期
};
}  // namespace convey

// 避免匿名对象，这种锁无实际意义
#define MutexGuard(x) error "Anonymous mutex object"

#endif
