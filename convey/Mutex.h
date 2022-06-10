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

class Mutex : public noncopyable
{
 public:
  Mutex() : m_holder(0) { MCHECK(pthread_mutex_init(&m_mutex, NULL)); }
  ~Mutex()
  {
    assert(m_holder == 0);
    MCHECK(pthread_mutex_destroy(&m_mutex));
  }

  void lock() { MCHECK(pthread_mutex_lock(&m_mutex)); }
  void unlock() { MCHECK(pthread_mutex_unlock(&m_mutex)); }

  void detachHolder() { m_holder = 0; }
  void attachHolder()
  {
    m_holder = 0;  // TODO: 实现持有者
  }

  pthread_mutex_t *getMutexPtr() { return &m_mutex; }

 private:
  pthread_mutex_t m_mutex;  // 系统分配的锁资源
  pid_t m_holder;           // 锁的持有者，线程id
};

class MutexGuard : public noncopyable
{
  // 利用RAII加锁解锁
 public:
  explicit MutexGuard(Mutex &mutex) : m_mutex(mutex) { mutex.lock(); }
  ~MutexGuard() { m_mutex.unlock(); }

 private:
  Mutex &m_mutex;  // 这里只是一个引用，不管理生命周期
};

// 避免匿名对象，这种锁无实际意义
#define MutexGuard(x) error "Anonymous mutex object"

#endif
