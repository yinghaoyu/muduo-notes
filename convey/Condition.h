#ifndef CONVEY_CONDITION_H
#define CONVEY_CONDITION_H

#include "Mutex.h"

namespace convey
{
class Condition : public noncopyable
{
 public:
  explicit Condition(Mutex &mutex) : mutex_(mutex) { MCHECK(pthread_cond_init(&pcond_, NULL)); }
  ~Condition() { MCHECK(pthread_cond_destroy(&pcond_)); }

  void wait()
  {
    mutex_.attachHolder();
    MCHECK(pthread_cond_wait(&pcond_, mutex_.getMutexPtr()));
  }

  void notify() { MCHECK(pthread_cond_signal(&pcond_)); }

  void notifyAll() { MCHECK(pthread_cond_broadcast(&pcond_)); }

 private:
  Mutex &mutex_;  // 引用，不管理生命周期
  pthread_cond_t pcond_;
};
}  // namespace convey

#endif
