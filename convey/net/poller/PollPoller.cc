#include "../../net/poller/PollPoller.h"

#include "../../base/Logging.h"
#include "../../base/Types.h"
#include "../../net/Channel.h"

#include <assert.h>
#include <errno.h>
#include <poll.h>

using namespace convey;
using namespace convey::net;

PollPoller::PollPoller(EventLoop *loop) : Poller(loop) {}

PollPoller::~PollPoller() = default;

Timestamp PollPoller::poll(int timeoutMs, ChannelList *activeChannels)
{
  // XXX pollfds_ shouldn't change
  int numEvents = ::poll(&*pollfds_.begin(), pollfds_.size(), timeoutMs);
  int savedErrno = errno;
  Timestamp now(Timestamp::now());
  if (numEvents > 0)
  {
    LOG_TRACE << numEvents << " events happened";
    fillActiveChannels(numEvents, activeChannels);
  }
  else if (numEvents == 0)
  {
    LOG_TRACE << " nothing happened";
  }
  else
  {
    if (savedErrno != EINTR)
    {
      errno = savedErrno;
      LOG_SYSERR << "PollPoller::poll()";
    }
  }
  return now;
}

void PollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const
{
  for (PollFdList::const_iterator pfd = pollfds_.begin(); pfd != pollfds_.end() && numEvents > 0; ++pfd)
  {
    if (pfd->revents > 0)
    {
      --numEvents;
      // 根据fd找到channel
      ChannelMap::const_iterator ch = channels_.find(pfd->fd);
      assert(ch != channels_.end());
      Channel *channel = ch->second;
      assert(channel->fd() == pfd->fd);
      // 把revents保存到channel里面
      channel->set_revents(pfd->revents);
      // pfd->revents = 0;
      activeChannels->push_back(channel);
    }
  }
}

void PollPoller::updateChannel(Channel *channel)
{
  Poller::assertInLoopThread();
  LOG_TRACE << "fd = " << channel->fd() << " events = " << channel->events();
  if (channel->index() < 0)
  {
    // a new one, add to pollfds_
    assert(channels_.find(channel->fd()) == channels_.end());
    // 保存一个和channel匹配的pollfd
    struct pollfd pfd;
    pfd.fd = channel->fd();
    pfd.events = static_cast<short>(channel->events());
    pfd.revents = 0;
    pollfds_.push_back(pfd);
    int idx = static_cast<int>(pollfds_.size()) - 1;
    // 为新加入的channel分配idx
    channel->set_index(idx);
    channels_[pfd.fd] = channel;
  }
  else
  {
    // update existing one
    assert(channels_.find(channel->fd()) != channels_.end());
    assert(channels_[channel->fd()] == channel);
    int idx = channel->index();
    assert(0 <= idx && idx < static_cast<int>(pollfds_.size()));
    struct pollfd &pfd = pollfds_[idx];
    assert(pfd.fd == channel->fd() || pfd.fd == -channel->fd() - 1);
    pfd.fd = channel->fd();
    pfd.events = static_cast<short>(channel->events());
    pfd.revents = 0;
    if (channel->isNoneEvent())
    {
      // ignore this pollfd
      pfd.fd = -channel->fd() - 1;
    }
  }
}

void PollPoller::removeChannel(Channel *channel)
{
  Poller::assertInLoopThread();
  LOG_TRACE << "fd = " << channel->fd();
  assert(channels_.find(channel->fd()) != channels_.end());
  assert(channels_[channel->fd()] == channel);
  assert(channel->isNoneEvent());
  int idx = channel->index();
  assert(0 <= idx && idx < static_cast<int>(pollfds_.size()));
  const struct pollfd &pfd = pollfds_[idx];
  (void) pfd;
  // removeChannel之前，channel一定是NodeEvent
  assert(pfd.fd == -channel->fd() - 1 && pfd.events == channel->events());
  size_t n = channels_.erase(channel->fd());
  assert(n == 1);
  (void) n;
  if (implicit_cast<size_t>(idx) == pollfds_.size() - 1)
  {
    // 在idx位置的pollfd刚好是最后一个元素，直接pop_back
    pollfds_.pop_back();
  }
  else
  {
    int channelAtEnd = pollfds_.back().fd;
    // 把idx位置的pollfd移动到最后
    iter_swap(pollfds_.begin() + idx, pollfds_.end() - 1);
    if (channelAtEnd < 0)
    {
      // 因为删除之前，ignore了，见97行
      // 为了得到正确的channel的fd，需要再操作一次
      channelAtEnd = -channelAtEnd - 1;
    }
    // 更新被交换的pollfd的index
    channels_[channelAtEnd]->set_index(idx);
    // 删除不需要的pollfd
    pollfds_.pop_back();
  }
}
