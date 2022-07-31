set(http_SRCS
  HttpServer.cc
  HttpResponse.cc
  HttpContext.cc
  )

add_library(convey_http ${http_SRCS})
target_link_libraries(convey_http convey_net)
