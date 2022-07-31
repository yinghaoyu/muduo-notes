#include "../../net/http/HttpServer.h"

#include "../../base/Logging.h"
#include "../../net/http/HttpContext.h"
#include "../../net/http/HttpRequest.h"
#include "../../net/http/HttpResponse.h"

using namespace convey;
using namespace convey::net;

namespace convey
{
namespace net
{
namespace detail
{
void defaultHttpCallback(const HttpRequest &, HttpResponse *resp)
{
  resp->setStatusCode(HttpResponse::k404NotFound);
  resp->setStatusMessage("Not Found");
  resp->setCloseConnection(true);
}

}  // namespace detail
}  // namespace net
}  // namespace convey

HttpServer::HttpServer(EventLoop *loop, const InetAddress &listenAddr, const string &name, TcpServer::Option option)
    : server_(loop, listenAddr, name, option), httpCallback_(detail::defaultHttpCallback)
{
  server_.setConnectionCallback(std::bind(&HttpServer::onConnection, this, _1));
  server_.setMessageCallback(std::bind(&HttpServer::onMessage, this, _1, _2, _3));
}

void HttpServer::start()
{
  LOG_WARN << "HttpServer[" << server_.name() << "] starts listening on " << server_.ipPort();
  server_.start();
}

void HttpServer::onConnection(const TcpConnectionPtr &conn)
{
  if (conn->connected())
  {
    // 保存这个连接的上下文
    conn->setContext(HttpContext());
  }
}

void HttpServer::onMessage(const TcpConnectionPtr &conn, Buffer *buf, Timestamp receiveTime)
{
  // 获取这个连接的上下文
  HttpContext *context = boost::any_cast<HttpContext>(conn->getMutableContext());

  // 解析buffer，并保存到context的HttpRequest
  if (!context->parseRequest(buf, receiveTime))
  {
    // 解析出错
    conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
    conn->shutdown();
  }

  if (context->gotAll())
  {
    // 解析成功
    // 根据request发送相应的response
    onRequest(conn, context->request());
    // 处理本次request，重置connection的上下文
    context->reset();
  }
}

void HttpServer::onRequest(const TcpConnectionPtr &conn, const HttpRequest &req)
{
  const string &connection = req.getHeader("Connection");
  // http 1.0 如果是长连接，必须指定Keep-Alive选项
  bool close = connection == "close" || (req.getVersion() == HttpRequest::kHttp10 && connection != "Keep-Alive");
  HttpResponse response(close);
  httpCallback_(req, &response);
  Buffer buf;
  response.appendToBuffer(&buf);
  conn->send(&buf);
  if (response.closeConnection())
  {
    conn->shutdown();
  }
}
