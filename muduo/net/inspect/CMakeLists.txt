set(inspect_SRCS
  Inspector.cc
  PerformanceInspector.cc
  ProcessInspector.cc
  SystemInspector.cc
  )

add_library(muduo_inspect ${inspect_SRCS})
target_link_libraries(muduo_inspect muduo_http)
