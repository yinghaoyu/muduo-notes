set(base_SRCS
  Condition.cc
  CountDownLatch.cc
  CurrentThread.cc
  Date.cc
  Exception.cc
  FileUtil.cc
  LogFile.cc
  Logging.cc
  LogStream.cc
  AsyncLogging.cc
  ProcessInfo.cc
  Timestamp.cc
  Thread.cc
  ThreadPool.cc
  TimeZone.cc
  )

add_library(muduo_base ${base_SRCS})
target_link_libraries(muduo_base pthread rt)
