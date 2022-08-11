#ifndef CONVEY_BASE_CONDITION_H
#define CONVEY_BASE_CONDITION_H

#include "convey/base/Mutex.h"

namespace convey
{
class Condition : public noncopyable
{
 public:
  explicit Condition(Mutex &mutex) : mutex_(mutex) { MCHECK(pthread_cond_init(&pcond_, NULL)); }
  ~Condition() { MCHECK(pthread_cond_destroy(&pcond_)); }

  void wait()
  {
    // wait会先解锁，等到条件满足再获取锁
    // 利用RAII，先detach持有者，等重新获取锁后再attach持有者
    Mutex::DetachGuard dg(mutex_);
    MCHECK(pthread_cond_wait(&pcond_, mutex_.getMutexPtr()));
  }

  // returns true if time out, false otherwise.
  bool waitForSeconds(double seconds);

  void notify() { MCHECK(pthread_cond_signal(&pcond_)); }

  void notifyAll() { MCHECK(pthread_cond_broadcast(&pcond_)); }

 private:
  Mutex &mutex_;  // 引用，不管理生命周期
  pthread_cond_t pcond_;
};
}  // namespace convey

#endif
