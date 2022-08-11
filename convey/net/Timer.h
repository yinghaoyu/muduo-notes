#ifndef CONVEY_NET_TIMER_H
#define CONVEY_NET_TIMER_H

#include "convey/base/Atomic.h"
#include "convey/base/Timestamp.h"
#include "convey/net/Callbacks.h"

namespace convey
{
namespace net
{
///
/// Internal class for timer event.
///
class Timer : noncopyable
{
 public:
  Timer(TimerCallback cb, Timestamp when, double interval)
      : callback_(std::move(cb)), expiration_(when), interval_(interval), repeat_(interval > 0.0), sequence_(s_numCreated_.incrementAndGet())
  {
  }

  void run() const { callback_(); }

  Timestamp expiration() const { return expiration_; }
  bool repeat() const { return repeat_; }
  int64_t sequence() const { return sequence_; }

  void restart(Timestamp now);

  static int64_t numCreated() { return s_numCreated_.get(); }

 private:
  const TimerCallback callback_;  // 回调函数
  Timestamp expiration_;          // 超时时间戳
  const double interval_;         // 定时器间隔
  const bool repeat_;             // 是否重复
  const int64_t sequence_;        // 定时器的序列号

  static AtomicInt64 s_numCreated_;  // 用于生成定时器的序列号，自增
};

}  // namespace net
}  // namespace convey

#endif
