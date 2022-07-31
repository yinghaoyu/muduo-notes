#include "../convey/base/LogStream.h"
#include "../convey/base/Timestamp.h"

#include <stdio.h>
#include <sstream>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

using namespace convey;

const size_t N = 1000000;

#pragma GCC diagnostic ignored "-Wold-style-cast"

template <typename T>
void benchPrintf(const char *fmt)
{
  char buf[32];
  Timestamp start(Timestamp::now());
  for (size_t i = 0; i < N; ++i)
    snprintf(buf, sizeof buf, fmt, (T)(i));
  Timestamp end(Timestamp::now());

  printf("benchPrintf %f\n", timeDifference(end, start));
}

template <typename T>
void benchStringStream()
{
  Timestamp start(Timestamp::now());
  std::ostringstream os;

  for (size_t i = 0; i < N; ++i)
  {
    os << (T)(i);
    // 将写入位置设置为从流头开始偏移量为0
    // 即头插法
    os.seekp(0, std::ios_base::beg);
  }
  Timestamp end(Timestamp::now());

  printf("benchStringStream %f\n", timeDifference(end, start));
}

template <typename T>
void benchLogStream()
{
  Timestamp start(Timestamp::now());
  LogStream os;
  for (size_t i = 0; i < N; ++i)
  {
    os << (T)(i);
    os.resetBuffer();
  }
  Timestamp end(Timestamp::now());

  printf("benchLogStream %f\n", timeDifference(end, start));
}

int main()
{
  benchPrintf<int>("%d");

  // 分别统计和比较printf、std::stream、LogStream格式化消耗的时间
  puts("int");
  benchPrintf<int>("%d");
  benchStringStream<int>();
  benchLogStream<int>();

  puts("double");
  benchPrintf<double>("%.12g");
  benchStringStream<double>();
  benchLogStream<double>();

  puts("int64_t");
  benchPrintf<int64_t>("%" PRId64);
  benchStringStream<int64_t>();
  benchLogStream<int64_t>();

  puts("void*");
  benchPrintf<void *>("%p");
  benchStringStream<void *>();
  benchLogStream<void *>();
}
