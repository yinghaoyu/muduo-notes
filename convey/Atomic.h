#ifndef CONVEY_ATOMIC_H
#define CONVEY_ATOMIC_H

#include "noncopyable.h"

#include <stdint.h>

namespace convey
{
namespace detail
{
template <typename T>
class AtomicInt : public noncopyable
{
 public:
  AtomicInt() : value_(0) {}

  AtomicInt(const AtomicInt &other) : value_(other.get()) {}
  AtomicInt &operator=(const AtomicInt &other) {}

  T get()
  {
    // in gcc >= 4.7: __atomic_load_n(&value_, __ATOMIC_SEQ_CST)
    return __sync_val_compare_and_swap(&value_, 0, 0);
  }

  T getAndSet(T x)
  {
    // in gcc >= 4.7: __atomic_exchange_n(&value_, newValue, __ATOMIC_SEQ_CST)
    return __sync_lock_test_and_set(&value_, x);
  }

  T getAndAdd(T x)
  {
    // in gcc >= 4.7: __atomic_fetch_add(&value_, x, __ATOMIC_SEQ_CST)
    return __sync_val_fetch_and_add(&value_, x);
  }
  T addAndGet(T x) { return getAndAdd(x) + x; }
  T incrementAndGet() { return addAndGet(1); }
  T decrementAndGet() { return addAndGet(-1); }

  void add(T x) { getAndAdd(x); }
  void increment() { incrementAndGet(); }
  void decrement() { decrementAndGet(); }

 private:
  volatile T value_;
};
}  // namespace detail

typedef detail::AtomicInt<int32_t> AtomicInt32;
typedef detail::AtomicInt<int64_t> AtomicInt64;

}  // namespace convey
#endif
