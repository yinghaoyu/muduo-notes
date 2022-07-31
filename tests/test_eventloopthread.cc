#include "../convey/base/CountDownLatch.h"
#include "../convey/base/Thread.h"
#include "../convey/net/EventLoop.h"
#include "../convey/net/EventLoopThread.h"

#include <stdio.h>
#include <unistd.h>

using namespace convey;
using namespace convey::net;

void print(EventLoop *p = NULL)
{
  // %p输出指针本身，也就是指针指向的地址值
  printf("print: pid = %d, tid = %d, loop = %p\n", getpid(), CurrentThread::tid(), p);
}

void quit(EventLoop *p)
{
  print(p);
  p->quit();
}

int main()
{
  print();

  {
    EventLoopThread thr1;  // never start
  }

  {
    // dtor calls quit()
    EventLoopThread thr2;
    EventLoop *loop = thr2.startLoop();
    loop->runInLoop(std::bind(print, loop));
    CurrentThread::sleepUsec(500 * 1000);
  }

  {
    // quit() before dtor
    EventLoopThread thr3;
    EventLoop *loop = thr3.startLoop();
    loop->runInLoop(std::bind(quit, loop));
    CurrentThread::sleepUsec(500 * 1000);
  }
}
