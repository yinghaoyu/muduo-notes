#ifndef MUTEX_H
#define MUTEX_H

#include <assert.h>
#include <pthread.h>

#include "noncopyable.h"

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

  void lock() { MCHECK(pthread_mutex_lock(&mutex_)); }
  void unlock() { MCHECK(pthread_mutex_unlock(&mutex_)); }

  void detachHolder() { holder_ = 0; }
  void attachHolder()
  {
    holder_ = 0;  // TODO: 实现持有者
  }

  pthread_mutex_t *getMutexPtr() { return &mutex_; }

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
