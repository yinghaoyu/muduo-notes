#include "../net/EventLoopThread.h"

#include "../net/EventLoop.h"

using namespace convey;
using namespace convey::net;

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb, const string &name)
    : loop_(NULL), exiting_(false), thread_(std::bind(&EventLoopThread::threadFunc, this), name), mutex_(), cond_(mutex_), callback_(cb)
{
}

EventLoopThread::~EventLoopThread()
{
  exiting_ = true;
  if (loop_ != NULL)  // not 100% race-free, eg. threadFunc could be running callback_.
  {
    // still a tiny chance to call destructed object, if threadFunc exits just now.
    // but when EventLoopThread destructs, usually programming is exiting anyway.
    loop_->quit();
    thread_.join();
  }
}

EventLoop *EventLoopThread::startLoop()
{
  assert(!thread_.isStarted());
  thread_.start();

  EventLoop *loop = NULL;
  {
    MutexGuard lock(mutex_);
    while (loop_ == NULL)
    {
      cond_.wait();
    }
    loop = loop_;
  }

  return loop;
}

void EventLoopThread::threadFunc()
{
  EventLoop loop;

  if (callback_)
  {
    callback_(&loop);
  }

  {
    MutexGuard lock(mutex_);
    loop_ = &loop;
    cond_.notify();
  }

  loop.loop();
  // assert(exiting_);
  MutexGuard lock(mutex_);
  loop_ = NULL;
}
