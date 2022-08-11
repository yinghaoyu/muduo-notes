#include <stdio.h>
#include <functional>
#include <vector>
#include "convey/base/CurrentThread.h"
#include "convey/base/Exception.h"

class Bar
{
 public:
  // 这里的names未使用，会报警告
  // 使用-Wno-unused-parameter忽略这个
  void test(std::vector<std::string> names = {})
  {
    // 打印栈，解析函数名
    printf("Stack:\n%s\n", convey::CurrentThread::stackTrace(true).c_str());

    // lambda表达式的函数
    [] { printf("Stack inside lambda:\n%s\n", convey::CurrentThread::stackTrace(true).c_str()); }();

    // function对象函数
    std::function<void()> func([] { printf("Stack inside std::function:\n%s\n", convey::CurrentThread::stackTrace(true).c_str()); });

    func();

    // std::bind函数
    func = std::bind(&Bar::callback, this);
    func();

    throw convey::Exception("oops");
  }

 private:
  void callback() { printf("Stack inside std::bind:\n%s\n", convey::CurrentThread::stackTrace(true).c_str()); }
};

void foo()
{
  Bar b;
  b.test();
}

int main()
{
  try
  {
    foo();
  }
  catch (const convey::Exception &ex)
  {
    printf("reason: %s\n", ex.what());
    printf("stack trace:\n%s\n", ex.stackTrace());
  }
}
