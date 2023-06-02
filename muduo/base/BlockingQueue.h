#ifndef MUDUO_BASE_BLOCKINGQUEUE_H
#define MUDUO_BASE_BLOCKINGQUEUE_H

#include "muduo/base/Condition.h"
#include "muduo/base/Mutex.h"

#include <assert.h>
#include <deque>

namespace muduo
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
    // 为什么会存在虚假唤醒呢？
    // pthread 的条件变量等待 pthread_cond_wait 是使用阻塞的系统调用实现的（比如 Linux 上的
    // futex），这些阻塞的系统调用在进程被信号中断后，通常会中止阻塞、直接返回 EINTR 错误。 同样是阻塞系统调用，你从 read 拿到 EINTR
    // 错误后可以直接决定重试，因为这通常不影响它本身的语义。 而条件变量等待则不能，因为本线程拿到 EINTR 错误和重新调用 futex 等待之间，可能别的线程已经通过
    // pthread_cond_signal 或者 pthread_cond_broadcast发过通知了。
    // 所以，虚假唤醒的一个可能性是条件变量的等待被信号中断。
    // 不过，把等待放到循环里的另一个原因是还可能有这样的情况（有人觉得它是虚假唤醒的一种，有人觉得不是）：明明有对应的唤醒，但条件不成立。
    // 这是因为可能由于线程调度的原因，被条件变量唤醒的线程在本线程内真正执行「加锁并返回」前，另一个线程插了进来，完整地进行了一套「拿锁、改条件、还锁」的操作。
    // https://www.zhihu.com/question/271521213
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
}  // namespace muduo

#endif
