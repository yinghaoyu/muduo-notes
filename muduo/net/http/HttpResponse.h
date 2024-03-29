#ifndef MUDUO_NET_HTTP_HTTPRESPONSE_H
#define MUDUO_NET_HTTP_HTTPRESPONSE_H

#include "muduo/base/Types.h"
#include "muduo/base/copyable.h"

#include <map>

namespace muduo
{
namespace net
{
class Buffer;
class HttpResponse : public muduo::copyable
{
 public:
  enum HttpStatusCode
  {
    kUnknown,
    k200Ok = 200,  // 客户端请求成功
    k301MovedPermanently = 301,
    k400BadRequest = 400,  // 客户端请求语法有错
    k404NotFound = 404,    // 请求资源部存在，eg：输入了错误的URl
  };

  explicit HttpResponse(bool close) : statusCode_(kUnknown), closeConnection_(close) {}

  void setStatusCode(HttpStatusCode code) { statusCode_ = code; }

  void setStatusMessage(const string &message) { statusMessage_ = message; }

  void setCloseConnection(bool on) { closeConnection_ = on; }

  bool closeConnection() const { return closeConnection_; }

  void setContentType(const string &contentType) { addHeader("Content-Type", contentType); }

  // FIXME: replace string with StringPiece
  void addHeader(const string &key, const string &value) { headers_[key] = value; }

  void setBody(const string &body) { body_ = body; }

  void appendToBuffer(Buffer *output) const;

 private:
  std::map<string, string> headers_;
  HttpStatusCode statusCode_;
  // FIXME: add http version
  string statusMessage_;
  bool closeConnection_;
  string body_;
};

}  // namespace net
}  // namespace muduo

#endif
