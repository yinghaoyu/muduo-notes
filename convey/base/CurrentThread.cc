#include "convey/base/CurrentThread.h"

#include <cxxabi.h>
#include <execinfo.h>  // backtrace

namespace convey
{
namespace CurrentThread
{
__thread int t_cachedTid = 0;
__thread char t_tidString[32];
__thread int t_tidStringLength = 6;
__thread const char *t_threadName = "unknow";

// 对pid_t进行静态类型检查
static_assert(std::is_same<int, pid_t>::value, "pid_t should be int");

string stackTrace(bool demangle)  // demangle意为拆解
{
  string stack;
  const int max_frames = 200;
  void *frame[max_frames];
  int nptrs = ::backtrace(frame, max_frames);          // 获取最大200层栈帧
  char **strings = ::backtrace_symbols(frame, nptrs);  // 将栈帧转换成字符数组
  if (strings)
  {
    size_t len = 256;  // 可读的函数名长度
    char *demangled = demangle ? static_cast<char *>(::malloc(len)) : nullptr;
    for (int i = 1; i < nptrs; ++i)  // skipping the 0-th, which is this function
    {
      if (demangle)
      {
        // https://panthema.net/2008/0901-stacktrace-demangled/
        // bin/exception_test(_ZN3Bar4testEv+0x79) [0x401909]
        char *left_par = nullptr;
        char *plus = nullptr;
        for (char *p = strings[i]; *p; ++p)
        {
          if (*p == '(')
          {
            left_par = p;
          }
          else if (*p == '+')
          {
            plus = p;
          }
        }

        // left_par和plus之间的是编译出来的函数名
        // _ZN3Bar4testEv
        if (left_par && plus)
        {
          *plus = '\0';  // 先截断
          int status = 0;
          // 把编译出来的函数名转化成可读的函数名
          char *ret = abi::__cxa_demangle(left_par + 1, demangled, &len, &status);
          *plus = '+';  // 恢复'+'
          if (status == 0)
          {
            demangled = ret;  // ret could be realloc()
            stack.append(strings[i], left_par + 1);
            stack.append(demangled);  // 这里替换成可读的函数名
            stack.append(plus);
            stack.push_back('\n');
            continue;
          }
        }
      }
      // Fallback to mangled names
      stack.append(strings[i]);
      stack.push_back('\n');
    }
    free(demangled);
    free(strings);
  }
  return stack;
}
}  // namespace CurrentThread
}  // namespace convey
