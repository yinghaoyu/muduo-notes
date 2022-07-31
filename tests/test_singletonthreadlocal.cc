#include "../convey/base/CurrentThread.h"
#include "../convey/base/Singleton.h"
#include "../convey/base/Thread.h"
#include "../convey/base/ThreadLocal.h"

#include <stdio.h>
#include <unistd.h>

class Test : convey::noncopyable
{
 public:
  Test() { printf("tid=%d, constructing %p\n", convey::CurrentThread::tid(), this); }

  ~Test() { printf("tid=%d, destructing %p %s\n", convey::CurrentThread::tid(), this, name_.c_str()); }

  const convey::string &name() const { return name_; }
  void setName(const convey::string &n) { name_ = n; }

 private:
  convey::string name_;
};

#define STL convey::Singleton<convey::ThreadLocal<Test> >::instance().value()

void print()
{
  printf("tid=%d, %p name=%s\n", convey::CurrentThread::tid(), &STL, STL.name().c_str());
}

void threadFunc(const char *changeTo)
{
  print();
  STL.setName(changeTo);
  sleep(1);
  print();
}

int main()
{
  STL.setName("main one");
  convey::Thread t1(std::bind(threadFunc, "thread1"));
  convey::Thread t2(std::bind(threadFunc, "thread2"));
  t1.start();
  t2.start();
  t1.join();
  print();
  t2.join();
  pthread_exit(0);
}
