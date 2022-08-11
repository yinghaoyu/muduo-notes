#include "convey/net/EventLoop.h"

#include "convey/base/Logging.h"
#include "convey/base/Mutex.h"
#include "convey/net/Channel.h"
#include "convey/net/Poller.h"
#include "convey/net/SocketsOps.h"
#include "convey/net/TimerQueue.h"

#include <algorithm>

#include <signal.h>
#include <sys/eventfd.h>
#include <unistd.h>

using namespace convey;
using namespace convey::net;

namespace
{
__thread EventLoop *t_loopInThisThread = 0;  // 线程局部变量，用于指向该线程的loop指针

const int kPollTimeMs = 10000;  // 超时时间10s

int createEventfd()
{
  int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);  // eventfd为非阻塞，子进程不可继承
  if (evtfd < 0)
  {
    LOG_SYSERR << "Failed in eventfd";
    abort();
  }
  return evtfd;
}

// 这里的目的是忽略旧式指针转换的错误，这里加不加无所谓
#pragma GCC diagnostic ignored "-Wold-style-cast"
class IgnoreSigPipe
{
 public:
  IgnoreSigPipe()
  {
    // TCP是全双工信道，close的本意是关闭整个连接，shutdown可以关闭读或者写（即TCP半关闭状态）
    // close会等socket的引用计数为0才会正常关闭socket，发送FIN包
    // shutdown不等引用计数为0，会直接发送FIN包
    // 当对端调用了close时，表明对端关闭了发送数据，但任然可以接收数据(内核接收缓冲区数据还没处理)
    // 本端只是收到来自对端的FIN包，由于TCP协议的限制，本端不能获知对端调用的是close还是shutdown
    // 对于一个已经收到FIN包的socket，调用read方法，如果缓冲区为空，read返回为0，表明对端关闭了连接
    // 但第一次调用write方法时，如果发送缓冲区没问题，会返回正确写入，但发送的报文会导致对端发送RST报文，因为对端的socket已经调用了close，既不发送数据，也不接收数据
    // 所以在第二次发送数据时（在收到RST包之后），会生成SIGPIPE信号
    // 默认的SIGPIPE信号处理函数是退出程序，因此要忽略这个信号，或者重写signaction
    ::signal(SIGPIPE, SIG_IGN);
    // LOG_TRACE << "Ignore SIGPIPE";
  }
};
#pragma GCC diagnostic error "-Wold-style-cast"

IgnoreSigPipe initObj;
}  // namespace

EventLoop *EventLoop::getEventLoopOfCurrentThread()
{
  return t_loopInThisThread;
}

EventLoop::EventLoop()
    : looping_(false),
      quit_(false),
      eventHandling_(false),
      callingPendingFunctors_(false),
      iteration_(0),
      threadId_(CurrentThread::tid()),
      poller_(Poller::newDefaultPoller(this)),
      timerQueue_(new TimerQueue(this)),
      wakeupFd_(createEventfd()),
      wakeupChannel_(new Channel(this, wakeupFd_)),
      currentActiveChannel_(NULL)
{
  LOG_DEBUG << "EventLoop created " << this << " in thread " << threadId_;
  if (t_loopInThisThread)
  {
    LOG_FATAL << "Another EventLoop " << t_loopInThisThread << " exists in this thread " << threadId_;
  }
  else
  {
    // 线程局部变量，保存该线程的loop指针
    t_loopInThisThread = this;
  }
  // 用于唤醒当前线程的eventfd
  wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
  // we are always reading the wakeupfd
  wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
  LOG_DEBUG << "EventLoop " << this << " of thread " << threadId_ << " destructs in thread " << CurrentThread::tid();
  // 这里关闭用于唤醒当前线程的eventfd
  // 为何不关闭channel？
  wakeupChannel_->disableAll();
  wakeupChannel_->remove();
  ::close(wakeupFd_);
  t_loopInThisThread = NULL;
}

void EventLoop::loop()
{
  assert(!looping_);
  assertInLoopThread();
  looping_ = true;
  quit_ = false;  // FIXME: what if someone calls quit() before loop() ?
  LOG_TRACE << "EventLoop " << this << " start looping";

  while (!quit_)
  {
    activeChannels_.clear();
    pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
    ++iteration_;
    if (Logger::logLevel() <= Logger::TRACE)
    {
      printActiveChannels();
    }
    // TODO sort channel by priority
    eventHandling_ = true;
    for (Channel *channel : activeChannels_)
    {
      currentActiveChannel_ = channel;
      currentActiveChannel_->handleEvent(pollReturnTime_);
    }
    currentActiveChannel_ = NULL;
    eventHandling_ = false;
    doPendingFunctors();
  }

  LOG_TRACE << "EventLoop " << this << " stop looping";
  looping_ = false;
}

void EventLoop::quit()
{
  quit_ = true;
  // 跨线程调用quit，假如该线程在此处挂起，这时因为quit_ = true导致loop归属的线程退出
  // loop被析构，等到当前线程被唤醒，这时访问的loop对象已经是非法对象了
  // There is a chance that loop() just executes while(!quit_) and exits,
  // then EventLoop destructs, then we are accessing an invalid object.
  // Can be fixed using mutex_ in both places.
  if (!isInLoopThread())
  {
    wakeup();
  }
}

void EventLoop::runInLoop(Functor cb)
{
  if (isInLoopThread())
  {
    cb();
  }
  else
  {
    queueInLoop(std::move(cb));
  }
}

void EventLoop::queueInLoop(Functor cb)
{
  {
    MutexGuard lock(mutex_);
    pendingFunctors_.push_back(std::move(cb));
  }

  // 调用者不在当前loop归属的线程，需要唤醒loop归属的线程以便快速执行cb
  // loop归属的线程在执行callingPendingFunctors时，后续可能会被挂起
  // 或者阻塞在poll阶段直到timeout，为了快速执行cb，因此也要唤醒
  if (!isInLoopThread() || callingPendingFunctors_)
  {
    wakeup();
  }
}

size_t EventLoop::queueSize() const
{
  MutexGuard lock(mutex_);
  return pendingFunctors_.size();
}

TimerId EventLoop::runAt(Timestamp time, TimerCallback cb)
{
  return timerQueue_->addTimer(std::move(cb), time, 0.0);
}

TimerId EventLoop::runAfter(double delay, TimerCallback cb)
{
  Timestamp time(addTime(Timestamp::now(), delay));
  return runAt(time, std::move(cb));
}

TimerId EventLoop::runEvery(double interval, TimerCallback cb)
{
  Timestamp time(addTime(Timestamp::now(), interval));
  return timerQueue_->addTimer(std::move(cb), time, interval);
}

void EventLoop::cancel(TimerId timerId)
{
  return timerQueue_->cancel(timerId);
}

void EventLoop::updateChannel(Channel *channel)
{
  assert(channel->ownerLoop() == this);
  assertInLoopThread();
  // 更新poller里面需要关注的事件
  poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel)
{
  assert(channel->ownerLoop() == this);
  assertInLoopThread();
  if (eventHandling_)
  {
    assert(currentActiveChannel_ == channel || std::find(activeChannels_.begin(), activeChannels_.end(), channel) == activeChannels_.end());
  }
  poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel *channel)
{
  assert(channel->ownerLoop() == this);
  assertInLoopThread();
  return poller_->hasChannel(channel);
}

void EventLoop::abortNotInLoopThread()
{
  LOG_FATAL << "EventLoop::abortNotInLoopThread - EventLoop " << this << " was created in threadId_ = " << threadId_
            << ", current thread id = " << CurrentThread::tid();
}

void EventLoop::wakeup()
{
  // 往loop的wakeupFd_写入8字节，以便唤醒loop归属的线程
  uint64_t one = 1;
  ssize_t n = sockets::write(wakeupFd_, &one, sizeof one);
  if (n != sizeof one)
  {
    LOG_ERROR << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
  }
}

void EventLoop::handleRead()
{
  // 读出8字节，防止重复唤醒
  uint64_t one = 1;
  ssize_t n = sockets::read(wakeupFd_, &one, sizeof one);
  if (n != sizeof one)
  {
    LOG_ERROR << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
  }
}

void EventLoop::doPendingFunctors()
{
  std::vector<Functor> functors;
  callingPendingFunctors_ = true;

  {
    MutexGuard lock(mutex_);
    functors.swap(pendingFunctors_);
  }

  for (const Functor &functor : functors)
  {
    functor();
  }
  callingPendingFunctors_ = false;
}

void EventLoop::printActiveChannels() const
{
  for (const Channel *channel : activeChannels_)
  {
    LOG_TRACE << "{" << channel->reventsToString() << "} ";
  }
}
