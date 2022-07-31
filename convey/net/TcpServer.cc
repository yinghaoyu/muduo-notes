#include "../net/TcpServer.h"

#include "../base/Logging.h"
#include "../net/Acceptor.h"
#include "../net/EventLoop.h"
#include "../net/EventLoopThreadPool.h"
#include "../net/SocketsOps.h"

#include <stdio.h>  // snprintf

using namespace convey;
using namespace convey::net;

TcpServer::TcpServer(EventLoop *loop, const InetAddress &listenAddr, const string &nameArg, Option option)
    : loop_(CHECK_NOTNULL(loop)),
      ipPort_(listenAddr.toIpPort()),
      name_(nameArg),
      acceptor_(new Acceptor(loop, listenAddr, option == kReusePort)),
      threadPool_(new EventLoopThreadPool(loop, name_)),
      connectionCallback_(defaultConnectionCallback),
      messageCallback_(defaultMessageCallback),
      nextConnId_(1)
{
  acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this, _1, _2));
}

TcpServer::~TcpServer()
{
  loop_->assertInLoopThread();
  LOG_TRACE << "TcpServer::~TcpServer [" << name_ << "] destructing";

  for (auto &item : connections_)
  {
    TcpConnectionPtr conn(item.second);
    item.second.reset();  // 引用计数减1
    conn->getLoop()->runInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
  }
}

void TcpServer::setThreadNum(int numThreads)
{
  assert(0 <= numThreads);
  // 设置线程池的线程数量
  threadPool_->setThreadNum(numThreads);
}

void TcpServer::start()
{
  if (started_.getAndSet(1) == 0)
  {
    threadPool_->start(threadInitCallback_);

    assert(!acceptor_->listening());
    loop_->runInLoop(std::bind(&Acceptor::listen, get_pointer(acceptor_)));
  }
}

void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr)
{
  loop_->assertInLoopThread();
  // 线程池轮询取出不同线程的loop
  EventLoop *ioLoop = threadPool_->getNextLoop();
  char buf[64];
  snprintf(buf, sizeof buf, "-%s#%d", ipPort_.c_str(), nextConnId_);
  ++nextConnId_;
  string connName = name_ + buf;

  LOG_INFO << "TcpServer::newConnection [" << name_ << "] - new connection [" << connName << "] from " << peerAddr.toIpPort();
  InetAddress localAddr(sockets::getLocalAddr(sockfd));
  // FIXME poll with zero timeout to double confirm the new connection
  // FIXME use make_shared if necessary
  TcpConnectionPtr conn(new TcpConnection(ioLoop, connName, sockfd, localAddr, peerAddr));
  connections_[connName] = conn;
  conn->setConnectionCallback(connectionCallback_);
  conn->setMessageCallback(messageCallback_);
  conn->setWriteCompleteCallback(writeCompleteCallback_);
  // TcpConnection被关闭的时候会回调这个函数，用的是shared_from_this()的引用
  // TcpServer和TcpConnection是在不同的线程上，TcpServer如果先析构了，this指针就不安全了
  conn->setCloseCallback(std::bind(&TcpServer::removeConnection, this, _1));  // FIXME: unsafe
  // 让TcpConnection在分配的ioLoop里，添加channel并关注可读事件，防止race condition
  ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr &conn)
{
  // 因为响应TcpConnection可读、可写事件的是轮询分配的ioLoop
  // TcpConnection构造的时候，是在TcpServer所在的loop
  // 因此析构的时候，最好是在TcpServer所在的loop上，以防止race condition
  // FIXME: unsafe
  loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn)
{
  loop_->assertInLoopThread();
  LOG_INFO << "TcpServer::removeConnectionInLoop [" << name_ << "] - connection " << conn->name();
  size_t n = connections_.erase(conn->name());
  (void) n;
  assert(n == 1);
  EventLoop *ioLoop = conn->getLoop();
  ioLoop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}
