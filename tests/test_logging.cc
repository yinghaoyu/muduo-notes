#include "../convey/base/LogFile.h"
#include "../convey/base/Logging.h"
#include "../convey/base/ThreadPool.h"
#include "../convey/base/TimeZone.h"

#include <stdio.h>
#include <unistd.h>
#include <memory>

int g_total;
FILE *g_file;
std::unique_ptr<convey::LogFile> g_logFile;

void dummyOutput(const char *msg, int len)
{
  g_total += len;
  if (g_file)
  {
    fwrite(msg, 1, len, g_file);
  }
  else if (g_logFile)
  {
    g_logFile->append(msg, len);
  }
}

void bench(const char *type)
{
  convey::Logger::setOutput(dummyOutput);
  convey::Timestamp start(convey::Timestamp::now());
  g_total = 0;

  int n = 1000 * 1000;
  const bool kLongLog = false;
  convey::string empty = " ";
  convey::string longStr(3000, 'X');
  longStr += " ";
  for (int i = 0; i < n; ++i)
  {
    LOG_INFO << "Hello 0123456789"
             << " abcdefghijklmnopqrstuvwxyz" << (kLongLog ? longStr : empty) << i;
  }
  convey::Timestamp end(convey::Timestamp::now());
  double seconds = timeDifference(end, start);
  printf("%12s: %f seconds, %d bytes, %10.2f msg/s, %.2f MiB/s\n", type, seconds, g_total, n / seconds, g_total / seconds / (1024 * 1024));
}

void logInThread()
{
  LOG_INFO << "logInThread";
  usleep(1000);
}

int main()
{
  getppid();  // for ltrace and strace

  convey::ThreadPool pool("pool");
  pool.start(5);
  pool.run(logInThread);
  pool.run(logInThread);
  pool.run(logInThread);
  pool.run(logInThread);
  pool.run(logInThread);

  LOG_TRACE << "trace";
  LOG_DEBUG << "debug";
  LOG_INFO << "Hello";  // 默认是INFO等级，低于INFO的上述两条不打印
  LOG_WARN << "World";
  LOG_ERROR << "Error";
  LOG_INFO << sizeof(convey::Logger);
  LOG_INFO << sizeof(convey::LogStream);
  LOG_INFO << sizeof(convey::Fmt);
  LOG_INFO << sizeof(convey::LogStream::Buffer);

  sleep(1);
  // 测试写入不同文件的时间

  bench("nop");  // 格式化日志，但不写文件的速度

  char buffer[64 * 1024];

  g_file = fopen("/dev/null", "w");  // 写入/dev/null的速度
  setbuffer(g_file, buffer, sizeof buffer);
  bench("/dev/null");
  fclose(g_file);

  g_file = fopen("/tmp/log", "w");  // 写入/tmp/log的速度
  setbuffer(g_file, buffer, sizeof buffer);
  bench("/tmp/log");
  fclose(g_file);

  g_file = NULL;
  // 写入LogFile的时间
  g_logFile.reset(new convey::LogFile("test_log_st", 500 * 1000 * 1000, false));
  bench("test_log_st");

  g_logFile.reset(new convey::LogFile("test_log_mt", 500 * 1000 * 1000, true));
  bench("test_log_mt");
  g_logFile.reset();

  {
    g_file = stdout;
    sleep(1);
    convey::TimeZone beijing(8 * 3600, "CST");
    convey::Logger::setTimeZone(beijing);
    LOG_TRACE << "trace CST";
    LOG_DEBUG << "debug CST";
    LOG_INFO << "Hello CST";
    LOG_WARN << "World CST";
    LOG_ERROR << "Error CST";

    sleep(1);
    convey::TimeZone newyork("/usr/share/zoneinfo/America/New_York");
    convey::Logger::setTimeZone(newyork);  // 测试时区切换
    LOG_TRACE << "trace NYT";
    LOG_DEBUG << "debug NYT";
    LOG_INFO << "Hello NYT";
    LOG_WARN << "World NYT";
    LOG_ERROR << "Error NYT";
    g_file = NULL;
  }
  bench("timezone nop");
}
