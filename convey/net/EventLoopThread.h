#ifndef CONVEY_NET_EVENTLOOPTHREAD_H
#define CONVEY_NET_EVENTLOOPTHREAD_H

#include "../base/Condition.h"
#include "../base/Mutex.h"
#include "../base/Thread.h"

namespace convey
{
namespace net
{
class EventLoop;

class EventLoopThread : noncopyable
{
 public:
  typedef std::function<void(EventLoop *)> ThreadInitCallback;

  EventLoopThread(const ThreadInitCallback &cb = ThreadInitCallback(), const string &name = string());
  ~EventLoopThread();
  EventLoop *startLoop();

 private:
  void threadFunc();

  EventLoop *loop_;
  bool exiting_;
  Thread thread_;
  Mutex mutex_;
  Condition cond_;
  ThreadInitCallback callback_;
};

}  // namespace net
}  // namespace convey

#endif
