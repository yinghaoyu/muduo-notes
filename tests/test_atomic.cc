#include <assert.h>
#include "convey/base/Atomic.h"

int main()
{
  {
    convey::AtomicInt32 i32;
    assert(i32.get() == 0);
    assert(i32.getAndAdd(1) == 0);
    assert(i32.get() == 1);
    assert(i32.addAndGet(2) == 3);
    assert(i32.get() == 3);
    assert(i32.incrementAndGet() == 4);
    assert(i32.get() == 4);
    i32.increment();
    assert(i32.get() == 5);
    assert(i32.addAndGet(-3) == 2);
    assert(i32.getAndSet(100) == 2);
    assert(i32.get() == 100);
  }
}
