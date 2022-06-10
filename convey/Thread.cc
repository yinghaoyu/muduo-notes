#include "Thread.h"

#include <sys/syscall.h>
#include <unistd.h>

pid_t getTid()
{
  return static_cast<pid_t>(::syscall(SYS_gettid));
}

Thread::Thread(ThreadFunc func, const std::string name)
    : m_started(false), m_joined(false), m_pthread(0), m_tid(0), m_func(std::move(func)), m_name(name), m_latch(1)
{
  setDefaultName();
}

void Thread::setDefaultName()
{
  int num = s_numCreated.incrementAndGet();
  if (m_name.empty())
  {
    char buf[32];
    snprintf(buf, sizeof(buf), "Thread%d", num);
    m_name = buf;
  }
}

void Thread::start()
{
  assert(!m_started);
  m_started = true;
  if (pthread_create(&m_pthread, NULL, Thread::startThread, this))
  {
  }
  else
  {
    m_latch.wait();
    assert(m_tid > 0);
  }
}

void *Thread::startThread(void *obj)
{
  Thread *thread = static_cast<Thread *>(obj);
  thread->runInThread();
  return NULL;
}

void Thread::runInThread()
{
  m_tid = getTid();
  m_latch.countDown();
  try
  {
    m_func();
  }
  catch (...)
  {
  }
}
