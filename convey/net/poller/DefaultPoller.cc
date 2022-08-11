#include "convey/net/Poller.h"
#include "convey/net/poller/EPollPoller.h"
#include "convey/net/poller/PollPoller.h"

#include <stdlib.h>

using namespace convey::net;

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
