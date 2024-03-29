#ifndef MUDUO_BASE_LOGFILE_H
#define MUDUO_BASE_LOGFILE_H

#include "muduo/base/Mutex.h"
#include "muduo/base/Types.h"

#include <memory>

namespace muduo
{
namespace FileUtil
{
class AppendFile;  // 前置声明，向外部隐藏实现
}

class LogFile : public noncopyable
{
 public:
  LogFile(const string &basename, off_t rollSize, bool threadSafe = true, int flushInterval = 3, int checkEveryN = 1024);
  ~LogFile();

  void append(const char *logline, int len);
  void flush();
  bool rollFile();

 private:
  void append_unlocked(const char *logline, int len);

  static string getLogFileName(const string &basename, time_t *now);

  const string basename_;
  const off_t rollSize_;
  const int flushInterval_;
  const int checkEveryN_;

  int count_;

  std::unique_ptr<Mutex> mutex_;
  time_t startOfPeriod_;
  time_t lastRoll_;
  time_t lastFlush_;
  std::unique_ptr<FileUtil::AppendFile> file_;

  const static int kRollPerSeconds_ = 60 * 60 * 24;
};

}  // namespace muduo
#endif
