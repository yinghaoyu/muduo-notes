#include "convey/net/TcpConnection.h"

#include "convey/base/Logging.h"
#include "convey/base/WeakCallback.h"
#include "convey/net/Channel.h"
#include "convey/net/EventLoop.h"
#include "convey/net/Socket.h"
#include "convey/net/SocketsOps.h"

#include <errno.h>

using namespace convey;
using namespace convey::net;

void convey::net::defaultConnectionCallback(const TcpConnectionPtr &conn)
{
  LOG_TRACE << conn->localAddress().toIpPort() << " -> " << conn->peerAddress().toIpPort() << " is " << (conn->connected() ? "UP" : "DOWN");
  // do not call conn->forceClose(), because some users want to register message callback only.
}

void convey::net::defaultMessageCallback(const TcpConnectionPtr &, Buffer *buf, Timestamp)
{
  buf->retrieveAll();
}

TcpConnection::TcpConnection(EventLoop *loop, const string &nameArg, int sockfd, const InetAddress &localAddr, const InetAddress &peerAddr)
    : loop_(CHECK_NOTNULL(loop)),
      name_(nameArg),
      state_(kConnecting),
      reading_(true),
      socket_(new Socket(sockfd)),
      channel_(new Channel(loop, sockfd)),
      localAddr_(localAddr),
      peerAddr_(peerAddr),
      highWaterMark_(64 * 1024 * 1024)
{
  // 设置当前连接的回调函数
  channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this, _1));
  channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
  channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
  channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));
  LOG_DEBUG << "TcpConnection::ctor[" << name_ << "] at " << this << " fd=" << sockfd;
  socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
  LOG_DEBUG << "TcpConnection::dtor[" << name_ << "] at " << this << " fd=" << channel_->fd() << " state=" << stateToString();
  assert(state_ == kDisconnected);
}

bool TcpConnection::getTcpInfo(struct tcp_info *tcpi) const
{
  return socket_->getTcpInfo(tcpi);
}

string TcpConnection::getTcpInfoString() const
{
  char buf[1024];
  buf[0] = '\0';
  socket_->getTcpInfoString(buf, sizeof buf);
  return buf;
}

void TcpConnection::send(const void *data, int len)
{
  send(StringPiece(static_cast<const char *>(data), len));
}

void TcpConnection::send(const StringPiece &message)
{
  if (state_ == kConnected)  // 在连接状态才发送数据
  {
    if (loop_->isInLoopThread())
    {
      // 当前线程和TcpConnection的loop是同一个线程，直接发送数据
      sendInLoop(message);
    }
    else
    {
      // 跨线程
      // 这里的message是TcpConnection应用层缓冲区的数据，TcpConnection具有Buffer的所有权
      // 所以直到TcpConnection析构，然后Buffer析构，message才会失效
      void (TcpConnection::*fp)(const StringPiece &message) = &TcpConnection::sendInLoop;
      loop_->runInLoop(std::bind(fp,
                                 this,  // FIXME
                                 message.as_string()));
      // std::forward<string>(message)));
    }
  }
}

// FIXME efficiency!!!
void TcpConnection::send(Buffer *buf)
{
  if (state_ == kConnected)
  {
    if (loop_->isInLoopThread())
    {
      sendInLoop(buf->peek(), buf->readableBytes());
      buf->retrieveAll();
    }
    else
    {
      void (TcpConnection::*fp)(const StringPiece &message) = &TcpConnection::sendInLoop;
      loop_->runInLoop(std::bind(fp,
                                 this,  // FIXME
                                 buf->retrieveAllAsString()));
      // std::forward<string>(message)));
    }
  }
}

void TcpConnection::sendInLoop(const StringPiece &message)
{
  sendInLoop(message.data(), message.size());
}

void TcpConnection::sendInLoop(const void *data, size_t len)
{
  loop_->assertInLoopThread();
  ssize_t nwrote = 0;
  size_t remaining = len;
  bool faultError = false;
  if (state_ == kDisconnected)
  {
    LOG_WARN << "disconnected, give up writing";
    return;
  }
  // if no thing in output queue, try writing directly
  // 应用层发送缓冲区如果为空，channel就不会关注可写事件
  if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
  {
    // 应用层发送缓冲区为空，就直接写入内核
    nwrote = sockets::write(channel_->fd(), data, len);
    if (nwrote >= 0)
    {
      remaining = len - nwrote;
      if (remaining == 0 && writeCompleteCallback_)
      {
        loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
      }
    }
    else  // nwrote < 0
    {
      nwrote = 0;
      if (errno != EWOULDBLOCK)
      {
        LOG_SYSERR << "TcpConnection::sendInLoop";
        if (errno == EPIPE || errno == ECONNRESET)  // FIXME: any others?
        {
          faultError = true;
        }
      }
    }
  }

  // 如果数据没发送完，那么必须先加入到应用层的发送缓冲区，目的是为了有序发送
  assert(remaining <= len);
  if (!faultError && remaining > 0)
  {
    size_t oldLen = outputBuffer_.readableBytes();
    if (oldLen + remaining >= highWaterMark_ && oldLen < highWaterMark_ && highWaterMarkCallback_)
    {
      // 应用层写缓冲区超过高水位
      loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
    }
    // 把未发送完的数据保存到应用层写缓冲区
    outputBuffer_.append(static_cast<const char *>(data) + nwrote, remaining);
    if (!channel_->isWriting())  // 关注内核socket可写事件，可写事件把应用层数据写到内核
    {
      channel_->enableWriting();
    }
  }
}

void TcpConnection::shutdown()
{
  // FIXME: use compare and swap
  if (state_ == kConnected)
  {
    setState(kDisconnecting);
    // FIXME: shared_from_this()?
    loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
  }
}

void TcpConnection::shutdownInLoop()
{
  loop_->assertInLoopThread();
  if (!channel_->isWriting())
  {
    // we are not writing
    socket_->shutdownWrite();
  }
}

// void TcpConnection::shutdownAndForceCloseAfter(double seconds)
// {
//   // FIXME: use compare and swap
//   if (state_ == kConnected)
//   {
//     setState(kDisconnecting);
//     loop_->runInLoop(std::bind(&TcpConnection::shutdownAndForceCloseInLoop, this, seconds));
//   }
// }

// void TcpConnection::shutdownAndForceCloseInLoop(double seconds)
// {
//   loop_->assertInLoopThread();
//   if (!channel_->isWriting())
//   {
//     // we are not writing
//     socket_->shutdownWrite();
//   }
//   loop_->runAfter(
//       seconds,
//       makeWeakCallback(shared_from_this(),
//                        &TcpConnection::forceCloseInLoop));
// }

void TcpConnection::forceClose()
{
  // FIXME: use compare and swap
  if (state_ == kConnected || state_ == kDisconnecting)
  {
    setState(kDisconnecting);
    loop_->queueInLoop(std::bind(&TcpConnection::forceCloseInLoop, shared_from_this()));
  }
}

void TcpConnection::forceCloseWithDelay(double seconds)
{
  if (state_ == kConnected || state_ == kDisconnecting)
  {
    setState(kDisconnecting);
    // 假如在seconds秒内，这个连接被关闭了，那么TcpConnection会被析构，因此用makeWeakCallback

    // 这里用forceClose是为了把关闭TcpConnection延迟到doPendingFunctors
    // 假如关闭TcpConnection在handleEvent阶段处理，同时刚好这个TcpConnection有数据可读
    // 那么可能存在这种情况，handleEvent先close掉TcpConnection，然后又处理TcpConnection的可读事件，这明显不符合预期
    loop_->runAfter(seconds, makeWeakCallback(shared_from_this(), &TcpConnection::forceClose));  // not forceCloseInLoop to avoid race condition
  }
}

void TcpConnection::forceCloseInLoop()
{
  loop_->assertInLoopThread();
  if (state_ == kConnected || state_ == kDisconnecting)
  {
    // as if we received 0 byte in handleRead();
    handleClose();
  }
}

const char *TcpConnection::stateToString() const
{
  switch (state_)
  {
  case kDisconnected:
    return "kDisconnected";
  case kConnecting:
    return "kConnecting";
  case kConnected:
    return "kConnected";
  case kDisconnecting:
    return "kDisconnecting";
  default:
    return "unknown state";
  }
}

void TcpConnection::setTcpNoDelay(bool on)
{
  socket_->setTcpNoDelay(on);
}

void TcpConnection::startRead()
{
  loop_->runInLoop(std::bind(&TcpConnection::startReadInLoop, this));
}

void TcpConnection::startReadInLoop()
{
  loop_->assertInLoopThread();
  if (!reading_ || !channel_->isReading())
  {
    channel_->enableReading();
    reading_ = true;
  }
}

void TcpConnection::stopRead()
{
  loop_->runInLoop(std::bind(&TcpConnection::stopReadInLoop, this));
}

void TcpConnection::stopReadInLoop()
{
  loop_->assertInLoopThread();
  if (reading_ || channel_->isReading())
  {
    channel_->disableReading();  // 不关注socket的可读事件
    reading_ = false;
  }
}

void TcpConnection::connectEstablished()
{
  loop_->assertInLoopThread();
  assert(state_ == kConnecting);
  setState(kConnected);
  channel_->tie(shared_from_this());  // 保留一份数据
  channel_->enableReading();          // 这个时候分配的ioloop会把channel加入，并关注可读事件

  connectionCallback_(shared_from_this());
}

void TcpConnection::connectDestroyed()
{
  loop_->assertInLoopThread();
  if (state_ == kConnected)
  {
    setState(kDisconnected);
    channel_->disableAll();  // 不关注任何事件

    connectionCallback_(shared_from_this());
  }
  channel_->remove();  // 从归属的loop中删除channel的裸指针，loop没有channel的所有权
}

void TcpConnection::handleRead(Timestamp receiveTime)
{
  loop_->assertInLoopThread();
  int savedErrno = 0;
  // 把数据读入到应用层输入缓冲区
  ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
  if (n > 0)
  {
    // 正常读出数据
    messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
  }
  else if (n == 0)
  {
    // read返回为0，说明对方关闭了连接
    handleClose();
  }
  else
  {
    errno = savedErrno;
    LOG_SYSERR << "TcpConnection::handleRead";
    handleError();
  }
}

void TcpConnection::handleWrite()
{
  loop_->assertInLoopThread();
  if (channel_->isWriting())  // channel可写
  {
    ssize_t n = sockets::write(channel_->fd(), outputBuffer_.peek(), outputBuffer_.readableBytes());
    if (n > 0)
    {
      outputBuffer_.retrieve(n);
      if (outputBuffer_.readableBytes() == 0)
      {
        channel_->disableWriting();  // 应用层无数据可写，不关注可写事件
        if (writeCompleteCallback_)
        {
          loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
        }
        if (state_ == kDisconnecting)  // 应用层数据写完了，并主动调用了shutdown的话
        {
          // 关闭写，TCP处于半关闭状态
          shutdownInLoop();
        }
      }
    }
    else
    {
      LOG_SYSERR << "TcpConnection::handleWrite";
      // if (state_ == kDisconnecting)
      // {
      //   shutdownInLoop();
      // }
    }
  }
  else
  {
    LOG_TRACE << "Connection fd = " << channel_->fd() << " is down, no more writing";
  }
}

void TcpConnection::handleClose()
{
  loop_->assertInLoopThread();
  LOG_TRACE << "fd = " << channel_->fd() << " state = " << stateToString();
  assert(state_ == kConnected || state_ == kDisconnecting);
  // we don't close fd, leave it to dtor, so we can find leaks easily.
  setState(kDisconnected);
  channel_->disableAll();

  TcpConnectionPtr guardThis(shared_from_this());
  connectionCallback_(guardThis);
  // must be the last line
  closeCallback_(guardThis);
}

void TcpConnection::handleError()
{
  int err = sockets::getSocketError(channel_->fd());
  LOG_ERROR << "TcpConnection::handleError [" << name_ << "] - SO_ERROR = " << err << " " << strerror_tl(err);
}
