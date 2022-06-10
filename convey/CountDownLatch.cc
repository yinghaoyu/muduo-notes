#include "CountDownLatch.h"

CountDownLatch::CountDownLatch(int count) : m_mutex(), m_condition(m_mutex), m_count(count) {}

void CountDownLatch::wait()
{
  MutexGuard lock(m_mutex);
  while (m_count > 0)
  {
    m_condition.wait();
  }
}

void CountDownLatch::countDown()
{
  MutexGuard lock(m_mutex);
  if (--m_count == 0)
  {
    m_condition.notifyAll();
  }
}

int CountDownLatch::getCount() const
{
  MutexGuard lock(m_mutex);
  return m_count;
}
