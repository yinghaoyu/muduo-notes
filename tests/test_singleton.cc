#include "../convey/CurrentThread.h"
#include "../convey/Singleton.h"
#include "../convey/Thread.h"

#include <stdio.h>

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

class TestNoDestroy : convey::noncopyable
{
 public:
  // Tag member for Singleton<T>
  void no_destroy();  // 有这个成员函数的类指针不会delete

  TestNoDestroy() { printf("tid=%d, constructing TestNoDestroy %p\n", convey::CurrentThread::tid(), this); }

  ~TestNoDestroy() { printf("tid=%d, destructing TestNoDestroy %p\n", convey::CurrentThread::tid(), this); }
};

void threadFunc()
{
  printf("tid=%d, %p name=%s\n", convey::CurrentThread::tid(), &convey::Singleton<Test>::instance(), convey::Singleton<Test>::instance().name().c_str());
  convey::Singleton<Test>::instance().setName("only one, changed");
}

int main()
{
  convey::Singleton<Test>::instance().setName("only one");
  convey::Thread t1(threadFunc);
  t1.start();
  t1.join();
  printf("tid=%d, %p name=%s\n", convey::CurrentThread::tid(), &convey::Singleton<Test>::instance(), convey::Singleton<Test>::instance().name().c_str());
  convey::Singleton<TestNoDestroy>::instance();
  // valgrind --leak-check=yes ./release-cpp11/bin/test_singleton
  printf("with valgrind, you should see %zd-byte memory leak.\n", sizeof(TestNoDestroy));
}
