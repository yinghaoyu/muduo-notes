set(net_SRCS
  Acceptor.cc
  Buffer.cc
  Connector.cc
  EventLoop.cc
  EventLoopThread.cc
  EventLoopThreadPool.cc
  Channel.cc
  Timer.cc
  TimerQueue.cc
  TcpClient.cc
  TcpConnection.cc
  TcpServer.cc
  InetAddress.cc
  Socket.cc
  SocketsOps.cc
  Poller.cc
  poller/DefaultPoller.cc
  poller/EPollPoller.cc
  poller/PollPoller.cc
  )

add_library(muduo_net ${net_SRCS})
target_link_libraries(muduo_net muduo_base) # muduo_net依赖于muduo_base

add_subdirectory(http)
add_subdirectory(inspect)
