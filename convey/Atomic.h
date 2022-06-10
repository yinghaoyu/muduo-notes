#ifndef CONVEY_ATOMIC_H
#define CONVEY_ATOMIC_H

#include "noncopyable.h"

#include <stdint.h>

template <typename T>
class AtomicInt : public noncopyable
{
 public:
  AtomicInt() : m_value(0) {}

  AtomicInt(const AtomicInt &other) : m_value(other.get()) {}
  AtomicInt &operator=(const AtomicInt &other) {}

  T get() { return __sync_val_compare_and_swap(&m_value, 0, 0); }
  T getAndSet(T x) { return __sync_lock_test_and_set(&m_value, x); }
  T getAndAdd(T x) { return __sync_val_fetch_and_add(&m_value, x); }
  T addAndGet(T x) { return getAndAdd(x) + x; }
  T incrementAndGet() { return addAndGet(1); }
  T decrementAndGet() { return addAndGet(-1); }

  void add(T x) { getAndAdd(x); }
  void increment() { incrementAndGet(); }
  void decrement() { decrementAndGet(); }

 private:
  volatile T m_value;
};

typedef AtomicInt<int32_t> AtomicInt32;
typedef AtomicInt<int64_t> AtomicInt64;

#endif
