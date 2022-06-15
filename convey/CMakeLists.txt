set(base_SRCS
  Condition.cc
  CountDownLatch.cc
  CurrentThread.cc
  Date.cc
  Exception.cc
  Logging.cc
  LogStream.cc
  Timestamp.cc
  Thread.cc
  ThreadPool.cc
  TimeZone.cc
  )

add_library(convey_base ${base_SRCS})
target_link_libraries(convey_base pthread rt)
