#ifndef CONVEY_COUNTDOWNLATCH_H
#define CONVEY_COUNTDOWNLATCH_H

#include "Condition.h"
#include "Mutex.h"

namespace convey
{
class CountDownLatch : public noncopyable
{
 public:
  explicit CountDownLatch(int count);

  void wait();

  void countDown();

  int getCount() const;

 private:
  // 被mutable修饰的变量，将永远处于可变的状态，即使在一个const函数中
  mutable Mutex m_mutex;
  Condition m_condition;
  int m_count;
};
}  // namespace convey

#endif
