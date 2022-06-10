#ifndef CONDITION_H
#define CONDITION_H

#include "Mutex.h"

class Condition : public noncopyable
{
 public:
  explicit Condition(Mutex &mutex) : m_mutex(mutex) { MCHECK(pthread_cond_init(&m_pcond, NULL)); }
  ~Condition() { MCHECK(pthread_cond_destroy(&m_pcond)); }

  void wait()
  {
    m_mutex.attachHolder();
    MCHECK(pthread_cond_wait(&m_pcond, m_mutex.getMutexPtr()));
  }

  void notify() { MCHECK(pthread_cond_signal(&m_pcond)); }

  void notifyAll() { MCHECK(pthread_cond_broadcast(&m_pcond)); }

 private:
  Mutex &m_mutex;  // 引用，不管理生命周期
  pthread_cond_t m_pcond;
};

#endif
