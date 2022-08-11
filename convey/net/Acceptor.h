#ifndef CONVEY_NET_ACCEPTOR_H
#define CONVEY_NET_ACCEPTOR_H

#include <functional>

#include "convey/net/Channel.h"
#include "convey/net/Socket.h"

namespace convey
{
namespace net
{
class EventLoop;
class InetAddress;

///
/// Acceptor of incoming TCP connections.
///
class Acceptor : noncopyable
{
 public:
  typedef std::function<void(int sockfd, const InetAddress &)> NewConnectionCallback;

  Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport);
  ~Acceptor();

  void setNewConnectionCallback(const NewConnectionCallback &cb) { newConnectionCallback_ = cb; }

  void listen();

  bool listening() const { return listening_; }

  // Deprecated, use the correct spelling one above.
  // Leave the wrong spelling here in case one needs to grep it for error messages.
  // bool listenning() const { return listening(); }

 private:
  void handleRead();

  EventLoop *loop_;        // acceptor归属的loop
  Socket acceptSocket_;    // accept的socket
  Channel acceptChannel_;  // accept的channel，这里拥有所有权
  NewConnectionCallback newConnectionCallback_;
  bool listening_;
  int idleFd_;
};

}  // namespace net
}  // namespace convey

#endif
