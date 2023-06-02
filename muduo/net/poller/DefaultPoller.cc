#include "muduo/net/Poller.h"
#include "muduo/net/poller/EPollPoller.h"
#include "muduo/net/poller/PollPoller.h"

#include <stdlib.h>

using namespace muduo::net;

Poller *Poller::newDefaultPoller(EventLoop *loop)
{
  if (::getenv("MUDUO_USE_POLL"))
  {
    return new PollPoller(loop);
  }
  else
  {
    return new EPollPoller(loop);
  }
}
