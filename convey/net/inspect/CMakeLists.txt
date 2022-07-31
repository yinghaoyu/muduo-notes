set(inspect_SRCS
  Inspector.cc
  PerformanceInspector.cc
  ProcessInspector.cc
  SystemInspector.cc
  )

add_library(convey_inspect ${inspect_SRCS})
target_link_libraries(convey_inspect convey_http)
