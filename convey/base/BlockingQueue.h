#ifndef CONVEY_BLOCKINGQUEUE_H
#define CONVEY_BLOCKINGQUEUE_H

#include "Condition.h"
#include "Mutex.h"

#include <assert.h>
#include <deque>

namespace convey
{
template <typename T>
class BlockingQueue : public noncopyable
{
 public:
  using queue_type = std::deque<T>;

  BlockingQueue() : mutex_(), notEmpty_(mutex_), queue_() {}

  void put(const T &x)
  {
    MutexGuard lock(mutex_);
    queue_.push_back(x);
    // http://www.domaigne.com/blog/computing/condvars-signal-with-mutex-locked-or-not/
    // signal/broadcast后操作系统会将上下文切换到被唤醒的线程
    // 在单核的系统上，可能导致不必要的上下文切换
    // 此处还没来得及解锁，被唤醒的线程无法获得锁只能继续阻塞，然后又切换到这个线程解锁
    notEmpty_.notify();
  }

  T take()
  {
    MutexGuard lock(mutex_);
    while (queue_.empty())  // spurious wakeup
    {
      notEmpty_.wait();
    }
    assert(!queue_.empty());
    T front(std::move(queue_.front()));
    queue_.pop_front();
    return front;
  }

  queue_type drain()  // drain意为排干
  {
    queue_type queue;
    {
      MutexGuard lock(mutex_);
      queue = std::move(queue_);
      assert(queue_.empty());
    }
    return queue;
  }

  size_t size() const
  {
    MutexGuard lock(mutex_);
    return queue_.size();
  }

 private:
  mutable Mutex mutex_;
  Condition notEmpty_;
  queue_type queue_;
};
}  // namespace convey

#endif
