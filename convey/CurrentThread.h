#ifndef CONVEY_CUEERNTTHREAD_H
#define CONVEY_CUEERNTTHREAD_H

#include "Types.h"

namespace convey
{
namespace CurrentThread
{
// 线程局部变量
extern __thread int t_cachedTid;
extern __thread char t_tidString[32];
extern __thread int t_tidStringLength;
extern __thread const char *t_threadName;

void cachedTid();

inline int tid()
{
  // 指令预测
  // 这里的含义是t_cachedTid == 0为小概率事件
  // 编译优化的时候会把if分支的指令往后调整
  // https://zhuanlan.zhihu.com/p/55771717
  if (__builtin_expect(t_cachedTid == 0, 0))
  {
    cachedTid();
  }
  return t_cachedTid;
}

inline const char *tidString()
{
  return t_tidString;
}

inline int tidStringLength()
{
  return t_tidStringLength;
}

inline const char *name()
{
  return t_threadName;
}

bool isMainThread();

void sleepUsec(int64_t usec);

string stackTrace(bool demangle);

}  // namespace CurrentThread
}  // namespace convey

#endif
