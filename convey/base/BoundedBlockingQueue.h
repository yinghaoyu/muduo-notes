#ifndef CONVEY_BOUNDEDBLOCKINGQUEUE_H
#define CONVEY_BOUNDEDBLOCKINGQUEUE_H

#include "Condition.h"
#include "Mutex.h"

#include <assert.h>
#include <boost/circular_buffer.hpp>

namespace convey
{
template <typename T>
class BoundedBlockingQueue : public noncopyable
{
 public:
  using queue_type = boost::circular_buffer<T>;

  explicit BoundedBlockingQueue(int maxSize) : mutex_(), notEmpty_(mutex_), notFull_(mutex_), queue_(maxSize) {}

  void put(const T &x)
  {
    MutexGuard lock(mutex_);
    while (queue_.full())
    {
      notFull_.wait();
    }
    assert(!queue_.full());
    queue_.push_back(x);
    notEmpty_.notify();
  }

  void put(T &&x)
  {
    MutexGuard lock(mutex_);
    while (queue_.full())
    {
      notFull_.wait();
    }
    assert(!queue_.full());
    queue_.push_back(std::move(x));
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
    notFull_.notify();
    return front;
  }

  bool empty() const
  {
    MutexGuard lock(mutex_);
    return queue_.empty();
  }

  bool full() const
  {
    MutexGuard lock(mutex_);
    return queue_.full();
  }

  size_t capacity() const
  {
    MutexGuard lock(mutex_);
    return queue_.capacity();
  }

  size_t size() const
  {
    MutexGuard lock(mutex_);
    return queue_.size();
  }

 private:
  mutable Mutex mutex_;
  Condition notEmpty_;
  Condition notFull_;
  queue_type queue_;
};
}  // namespace convey

#endif
