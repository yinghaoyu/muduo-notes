#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#include "muduo/net/TimerQueue.h"
#include "muduo/base/Logging.h"
#include "muduo/net/EventLoop.h"
#include "muduo/net/Timer.h"
#include "muduo/net/TimerId.h"

#include <sys/timerfd.h>
#include <unistd.h>

namespace muduo
{
namespace net
{
namespace detail
{
int createTimerfd()
{
  int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
  if (timerfd < 0)
  {
    LOG_SYSFATAL << "Failed in timerfd_create";
  }
  return timerfd;
}

struct timespec howMuchTimeFromNow(Timestamp when)
{
  int64_t microseconds = when.microSecondsSinceEpoch() - Timestamp::now().microSecondsSinceEpoch();
  if (microseconds < 100)  // 估计一个模糊的精度，这里为100ms
  {
    microseconds = 100;
  }
  struct timespec ts;
  ts.tv_sec = static_cast<time_t>(microseconds / Timestamp::kMicroSecondsPerSecond);
  ts.tv_nsec = static_cast<long>((microseconds % Timestamp::kMicroSecondsPerSecond) * 1000);
  return ts;
}

// 读出timeFd触发的可读事件
void readTimerfd(int timerfd, Timestamp now)
{
  uint64_t howmany;
  ssize_t n = ::read(timerfd, &howmany, sizeof howmany);
  LOG_TRACE << "TimerQueue::handleRead() " << howmany << " at " << now.toString();
  if (n != sizeof howmany)
  {
    LOG_ERROR << "TimerQueue::handleRead() reads " << n << " bytes instead of 8";
  }
}

// 重新设置timeFd的超时间隔，expiration为所有timer里面最早超时的
void resetTimerfd(int timerfd, Timestamp expiration)
{
  // wake up loop by timerfd_settime()
  struct itimerspec newValue;
  struct itimerspec oldValue;
  memZero(&newValue, sizeof newValue);
  memZero(&oldValue, sizeof oldValue);
  newValue.it_value = howMuchTimeFromNow(expiration);
  int ret = ::timerfd_settime(timerfd, 0, &newValue, &oldValue);
  if (ret)
  {
    LOG_SYSERR << "timerfd_settime()";
  }
}

}  // namespace detail
}  // namespace net
}  // namespace muduo

using namespace muduo;
using namespace muduo::net;
using namespace muduo::net::detail;

TimerQueue::TimerQueue(EventLoop *loop) : loop_(loop), timerfd_(createTimerfd()), timerfdChannel_(loop, timerfd_), timers_(), callingExpiredTimers_(false)
{
  // time channel的可读回调函数设置为当前TimeQueue的成员函数
  // 由于time channel是TimeQueue的成员，一定是channel先析构，然后TimeQueue再析构
  timerfdChannel_.setReadCallback(std::bind(&TimerQueue::handleRead, this));
  // we are always reading the timerfd, we disarm it with timerfd_settime.
  timerfdChannel_.enableReading();
}

TimerQueue::~TimerQueue()
{
  timerfdChannel_.disableAll();
  timerfdChannel_.remove();
  ::close(timerfd_);
  // do not remove channel, since we're in EventLoop::dtor();
  for (const Entry &timer : timers_)
  {
    delete timer.second;
  }
}

TimerId TimerQueue::addTimer(TimerCallback cb, Timestamp when, double interval)
{
  Timer *timer = new Timer(std::move(cb), when, interval);
  // runInLoop是线程安全的，addTimer如果被其他线程调用，那么会queueInLoop
  // 后续运行addTimerLoop的线程一定是TimerQueue归属的EventLoop
  loop_->runInLoop(std::bind(&TimerQueue::addTimerInLoop, this, timer));
  return TimerId(timer, timer->sequence());
}

void TimerQueue::cancel(TimerId timerId)
{
  // 线程安全
  loop_->runInLoop(std::bind(&TimerQueue::cancelInLoop, this, timerId));
}

void TimerQueue::addTimerInLoop(Timer *timer)
{
  loop_->assertInLoopThread();
  // timer会插入到std::set里面
  // 如果timer比原来std::set的最小元素小，那么earliestChaned返回true
  bool earliestChanged = insert(timer);

  if (earliestChanged)
  {
    // 最小超时timer改变了，就重新设置timerFd的触发时间间隔
    resetTimerfd(timerfd_, timer->expiration());
  }
}

void TimerQueue::cancelInLoop(TimerId timerId)
{
  loop_->assertInLoopThread();
  assert(timers_.size() == activeTimers_.size());
  ActiveTimer timer(timerId.timer_, timerId.sequence_);
  ActiveTimerSet::iterator it = activeTimers_.find(timer);
  if (it != activeTimers_.end())  // 情况一：待删除的timer在激活队列里
  {
    // std::set的erase返回的是删除的元素个数
    size_t n = timers_.erase(Entry(it->first->expiration(), it->first));
    assert(n == 1);
    (void) n;
    delete it->first;  // FIXME: no delete please
    activeTimers_.erase(it);
  }
  else if (callingExpiredTimers_)  // 情况二：待删除的timer不在激活队列里，但是还没调用回调函数
  {
    // 因为有可能是用户的timer function取消当前的timer
    // 这两个回调function都是在handleEvent阶段处理的
    cancelingTimers_.insert(timer);
  }
  assert(timers_.size() == activeTimers_.size());
}

void TimerQueue::handleRead()
{
  loop_->assertInLoopThread();
  Timestamp now(Timestamp::now());
  readTimerfd(timerfd_, now);

  // 获取所有的超时Entry
  std::vector<Entry> expired = getExpired(now);

  callingExpiredTimers_ = true;
  cancelingTimers_.clear();
  // safe to callback outside critical section
  for (const Entry &it : expired)
  {
    // 调用超时的回调函数
    it.second->run();
  }
  callingExpiredTimers_ = false;

  reset(expired, now);
}

std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now)
{
  assert(timers_.size() == activeTimers_.size());
  std::vector<Entry> expired;
  Entry sentry(now, reinterpret_cast<Timer *>(UINTPTR_MAX));
  // 从set里面找到第一个 > 当前时间戳的timer的位置
  TimerList::iterator end = timers_.lower_bound(sentry);
  assert(end == timers_.end() || now < end->first);
  // 把超时的timer全部拷贝到expired容器内
  std::copy(timers_.begin(), end, back_inserter(expired));
  // 把set的超时时间全部删除
  timers_.erase(timers_.begin(), end);

  for (const Entry &it : expired)
  {
    ActiveTimer timer(it.second, it.second->sequence());
    size_t n = activeTimers_.erase(timer);
    assert(n == 1);
    (void) n;
  }

  assert(timers_.size() == activeTimers_.size());
  return expired;
}

void TimerQueue::reset(const std::vector<Entry> &expired, Timestamp now)
{
  Timestamp nextExpire;

  for (const Entry &it : expired)
  {
    ActiveTimer timer(it.second, it.second->sequence());
    // timer是重复触发的，且没有被取消
    if (it.second->repeat() && cancelingTimers_.find(timer) == cancelingTimers_.end())
    {
      it.second->restart(now);  // 重置超时时间
      insert(it.second);        // 加入激活序列
    }
    else  // 说明timer是一次性的，或者被取消了
    {
      // FIXME move to a free list
      delete it.second;  // FIXME: no delete please
    }
  }

  if (!timers_.empty())
  {
    nextExpire = timers_.begin()->second->expiration();
  }

  if (nextExpire.valid())  // 第一个timer超时间隔合法
  {
    // 重设timerFd的超时间隔
    resetTimerfd(timerfd_, nextExpire);
  }
}

bool TimerQueue::insert(Timer *timer)
{
  loop_->assertInLoopThread();
  assert(timers_.size() == activeTimers_.size());
  bool earliestChanged = false;
  Timestamp when = timer->expiration();
  TimerList::iterator it = timers_.begin();
  if (it == timers_.end() || when < it->first)
  {
    earliestChanged = true;
  }
  {
    // std::set是基于红黑树的平衡二叉树，元素是唯一的
    // std::set的insert函数，返回值为std::pair类型
    // 若插入成功，返回插入位置的迭代器及true
    // 若插入失败，返回与当前插入元素相同的元素的位置的迭代器及false
    std::pair<TimerList::iterator, bool> result = timers_.insert(Entry(when, timer));
    assert(result.second);
    (void) result;
  }
  {
    std::pair<ActiveTimerSet::iterator, bool> result = activeTimers_.insert(ActiveTimer(timer, timer->sequence()));
    assert(result.second);
    (void) result;
  }

  assert(timers_.size() == activeTimers_.size());
  return earliestChanged;
}
