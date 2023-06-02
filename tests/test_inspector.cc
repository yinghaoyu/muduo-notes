#include "muduo/net/EventLoop.h"
#include "muduo/net/EventLoopThread.h"
#include "muduo/net/inspect/Inspector.h"

using namespace muduo;
using namespace muduo::net;

int main()
{
  EventLoop loop;
  EventLoopThread t;
  Inspector ins(t.startLoop(), InetAddress(54321), "test");
  loop.loop();
}
