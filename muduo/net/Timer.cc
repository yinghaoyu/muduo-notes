#include "muduo/net/Timer.h"

using namespace muduo;
using namespace muduo::net;

AtomicInt64 Timer::s_numCreated_;

void Timer::restart(Timestamp now)
{
  if (repeat_)
  {
    expiration_ = addTime(now, interval_);  // 每次重启，都更新超时时间戳
  }
  else
  {
    expiration_ = Timestamp::invalid();
  }
}
