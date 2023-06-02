#include "muduo/base/CurrentThread.h"
#include "muduo/base/Thread.h"

#include <stdio.h>
#include <unistd.h>
#include <string>

void mysleep(int seconds)
{
  timespec t = {seconds, 0};
  nanosleep(&t, NULL);
}

void func1()
{
  printf("tid=%d\n", muduo::CurrentThread::tid());
}

void func2(int x)
{
  printf("tid=%d, x=%d\n", muduo::CurrentThread::tid(), x);
}

void func3()
{
  printf("tid=%d\n", muduo::CurrentThread::tid());
  mysleep(1);
}

class Foo
{
 public:
  explicit Foo(double x) : x_(x) {}

  void memberFunc1() { printf("tid=%d, Foo::x_=%f\n", muduo::CurrentThread::tid(), x_); }

  void memberFunc2(const std::string &text) { printf("tid=%d, Foo::x_=%f, text=%s\n", muduo::CurrentThread::tid(), x_, text.c_str()); }

 private:
  double x_;
};

int main()
{
  // 主线程pid和tid应该相等
  printf("pid=%d, tid=%d\n", ::getpid(), muduo::CurrentThread::tid());

  muduo::Thread t1(func1);
  t1.start();
  // 这里由于是new thread first，所以tid取到的值应该不为0
  assert(t1.tid() != 0);
  printf("t1.tid=%d\n", t1.tid());
  t1.join();

  // 普通函数传参
  muduo::Thread t2(std::bind(func2, 42), "thread for free function with argument");
  t2.start();
  printf("t2.tid=%d\n", t2.tid());
  t2.join();

  // 成员函数
  Foo foo(87.53);
  muduo::Thread t3(std::bind(&Foo::memberFunc1, &foo), "thread for member function without argument");
  t3.start();
  t3.join();

  // 成员函数传参
  muduo::Thread t4(std::bind(&Foo::memberFunc2, std::ref(foo), std::string("Who am I")));
  t4.start();
  t4.join();

  {
    muduo::Thread t5(func3);
    t5.start();
    // t5 may destruct eariler than thread creation.
  }
  mysleep(2);
  {
    muduo::Thread t6(func3);
    t6.start();
    mysleep(2);
    // t6 destruct later than thread creation.
  }
  sleep(2);
  printf("number of created threads %d\n", muduo::Thread::numCreated());
  return 0;
}
