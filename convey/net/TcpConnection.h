#ifndef CONVEY_NET_TCPCONNECTION_H
#define CONVEY_NET_TCPCONNECTION_H

#include "../base/StringPiece.h"
#include "../base/Types.h"
#include "../base/noncopyable.h"
#include "../net/Buffer.h"
#include "../net/Callbacks.h"
#include "../net/InetAddress.h"

#include <memory>

#include <boost/any.hpp>

// struct tcp_info is in <netinet/tcp.h>
struct tcp_info;

namespace convey
{
namespace net
{
class Channel;
class EventLoop;
class Socket;

///
/// TCP connection, for both client and server usage.
///
/// This is an interface class, so don't expose too much details.
class TcpConnection : noncopyable, public std::enable_shared_from_this<TcpConnection>
{
 public:
  /// Constructs a TcpConnection with a connected sockfd
  ///
  /// User should not create this object.
  TcpConnection(EventLoop *loop, const string &name, int sockfd, const InetAddress &localAddr, const InetAddress &peerAddr);
  ~TcpConnection();

  EventLoop *getLoop() const { return loop_; }
  const string &name() const { return name_; }
  const InetAddress &localAddress() const { return localAddr_; }
  const InetAddress &peerAddress() const { return peerAddr_; }
  bool connected() const { return state_ == kConnected; }
  bool disconnected() const { return state_ == kDisconnected; }
  // return true if success.
  bool getTcpInfo(struct tcp_info *) const;
  string getTcpInfoString() const;

  // void send(string&& message); // C++11
  void send(const void *message, int len);
  void send(const StringPiece &message);
  // void send(Buffer&& message); // C++11
  void send(Buffer *message);  // this one will swap data
  void shutdown();             // NOT thread safe, no simultaneous calling
  // void shutdownAndForceCloseAfter(double seconds); // NOT thread safe, no simultaneous calling
  void forceClose();
  void forceCloseWithDelay(double seconds);
  void setTcpNoDelay(bool on);
  // reading or not
  void startRead();
  void stopRead();
  bool isReading() const { return reading_; };  // NOT thread safe, may race with start/stopReadInLoop

  void setContext(const boost::any &context) { context_ = context; }

  const boost::any &getContext() const { return context_; }

  boost::any *getMutableContext() { return &context_; }

  void setConnectionCallback(const ConnectionCallback &cb) { connectionCallback_ = cb; }

  void setMessageCallback(const MessageCallback &cb) { messageCallback_ = cb; }

  void setWriteCompleteCallback(const WriteCompleteCallback &cb) { writeCompleteCallback_ = cb; }

  void setHighWaterMarkCallback(const HighWaterMarkCallback &cb, size_t highWaterMark)
  {
    highWaterMarkCallback_ = cb;
    highWaterMark_ = highWaterMark;
  }

  /// Advanced interface
  Buffer *inputBuffer() { return &inputBuffer_; }

  Buffer *outputBuffer() { return &outputBuffer_; }

  /// Internal use only.
  void setCloseCallback(const CloseCallback &cb) { closeCallback_ = cb; }

  // called when TcpServer accepts a new connection
  void connectEstablished();  // should be called only once
  // called when TcpServer has removed me from its map
  void connectDestroyed();  // should be called only once

 private:
  enum StateE
  {
    kDisconnected,
    kConnecting,
    kConnected,
    kDisconnecting
  };
  void handleRead(Timestamp receiveTime);
  void handleWrite();
  void handleClose();
  void handleError();
  // void sendInLoop(string&& message);
  void sendInLoop(const StringPiece &message);
  void sendInLoop(const void *message, size_t len);
  void shutdownInLoop();
  // void shutdownAndForceCloseInLoop(double seconds);
  void forceCloseInLoop();
  void setState(StateE s) { state_ = s; }
  const char *stateToString() const;
  void startReadInLoop();
  void stopReadInLoop();

  EventLoop *loop_;    // 分配的loop
  const string name_;  // 连接名
  StateE state_;       // FIXME: use atomic variable
  bool reading_;
  // we don't expose those classes to client.
  std::unique_ptr<Socket> socket_;    // connection的socket
  std::unique_ptr<Channel> channel_;  // connection的channel
  const InetAddress localAddr_;       // 本端地址
  const InetAddress peerAddr_;        // 对端地址
  ConnectionCallback connectionCallback_;
  MessageCallback messageCallback_;              // 消息到达回调
  WriteCompleteCallback writeCompleteCallback_;  // 缓冲区可写回调
  HighWaterMarkCallback highWaterMarkCallback_;  // 缓冲区溢出回调
  CloseCallback closeCallback_;                  // 连接关闭回调
  size_t highWaterMark_;                         // 缓冲区高水位大小
  Buffer inputBuffer_;                           //应用层输入缓冲区
  Buffer outputBuffer_;                          // FIXME: use list<Buffer> as output buffer.
  boost::any context_;                           // 上下文
  // FIXME: creationTime_, lastReceiveTime_
  //        bytesReceived_, bytesSent_
};

typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;

}  // namespace net
}  // namespace convey

#endif
