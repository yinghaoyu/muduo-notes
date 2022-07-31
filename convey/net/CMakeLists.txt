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

add_library(convey_net ${net_SRCS})
target_link_libraries(convey_net convey_base) # convey_net依赖于convey_base

add_subdirectory(http)
add_subdirectory(inspect)
