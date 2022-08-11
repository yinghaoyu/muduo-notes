#include "convey/net/EventLoop.h"
#include "convey/net/EventLoopThread.h"
#include "convey/net/inspect/Inspector.h"

using namespace convey;
using namespace convey::net;

int main()
{
  EventLoop loop;
  EventLoopThread t;
  Inspector ins(t.startLoop(), InetAddress(54321), "test");
  loop.loop();
}
