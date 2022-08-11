#ifndef CONVEY_NET_TCPSERVER_H
#define CONVEY_NET_TCPSERVER_H

#include "convey/base/Atomic.h"
#include "convey/base/Types.h"
#include "convey/net/TcpConnection.h"

#include <map>

namespace convey
{
namespace net
{
class Acceptor;
class EventLoop;
class EventLoopThreadPool;

///
/// TCP server, supports single-threaded and thread-pool models.
///
/// This is an interface class, so don't expose too much details.
class TcpServer : noncopyable
{
 public:
  typedef std::function<void(EventLoop *)> ThreadInitCallback;
  enum Option
  {
    kNoReusePort,
    kReusePort,
  };

  // TcpServer(EventLoop* loop, const InetAddress& listenAddr);
  TcpServer(EventLoop *loop, const InetAddress &listenAddr, const string &nameArg, Option option = kNoReusePort);
  ~TcpServer();  // force out-line dtor, for std::unique_ptr members.

  const string &ipPort() const { return ipPort_; }
  const string &name() const { return name_; }
  EventLoop *getLoop() const { return loop_; }

  /// Set the number of threads for handling input.
  ///
  /// Always accepts new connection in loop's thread.
  /// Must be called before @c start
  /// @param numThreads
  /// - 0 means all I/O in loop's thread, no thread will created.
  ///   this is the default value.
  /// - 1 means all I/O in another thread.
  /// - N means a thread pool with N threads, new connections
  ///   are assigned on a round-robin basis.
  void setThreadNum(int numThreads);
  void setThreadInitCallback(const ThreadInitCallback &cb) { threadInitCallback_ = cb; }
  /// valid after calling start()
  std::shared_ptr<EventLoopThreadPool> threadPool() { return threadPool_; }

  /// Starts the server if it's not listening.
  ///
  /// It's harmless to call it multiple times.
  /// Thread safe.
  void start();

  /// Set connection callback.
  /// Not thread safe.
  void setConnectionCallback(const ConnectionCallback &cb) { connectionCallback_ = cb; }

  /// Set message callback.
  /// Not thread safe.
  void setMessageCallback(const MessageCallback &cb) { messageCallback_ = cb; }

  /// Set write complete callback.
  /// Not thread safe.
  void setWriteCompleteCallback(const WriteCompleteCallback &cb) { writeCompleteCallback_ = cb; }

 private:
  /// Not thread safe, but in loop
  void newConnection(int sockfd, const InetAddress &peerAddr);
  /// Thread safe.
  void removeConnection(const TcpConnectionPtr &conn);
  /// Not thread safe, but in loop
  void removeConnectionInLoop(const TcpConnectionPtr &conn);

  typedef std::map<string, TcpConnectionPtr> ConnectionMap;

  EventLoop *loop_;  // the acceptor loop
  const string ipPort_;
  const string name_;
  std::unique_ptr<Acceptor> acceptor_;  // avoid revealing Acceptor
  std::shared_ptr<EventLoopThreadPool> threadPool_;
  ConnectionCallback connectionCallback_;        // 新连接回调
  MessageCallback messageCallback_;              // 消息到达回调
  WriteCompleteCallback writeCompleteCallback_;  // 数据可写回调
  ThreadInitCallback threadInitCallback_;
  AtomicInt32 started_;  // 启动了多少次
  // always in loop thread
  int nextConnId_;             // 连接数，自增
  ConnectionMap connections_;  // 拥有TCPConncetion的所有权
};

}  // namespace net
}  // namespace convey

#endif
