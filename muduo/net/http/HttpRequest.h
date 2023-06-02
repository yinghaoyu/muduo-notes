#ifndef MUDUO_NET_HTTP_HTTPREQUEST_H
#define MUDUO_NET_HTTP_HTTPREQUEST_H

#include "muduo/base/Timestamp.h"
#include "muduo/base/Types.h"
#include "muduo/base/copyable.h"

#include <assert.h>
#include <stdio.h>
#include <map>

namespace muduo
{
namespace net
{
class HttpRequest : public muduo::copyable
{
 public:
  enum Method
  {
    kInvalid,
    kGet,
    kPost,
    kHead,
    kPut,
    kDelete
  };
  enum Version
  {
    kUnknown,
    kHttp10,
    kHttp11
  };

  HttpRequest() : method_(kInvalid), version_(kUnknown) {}

  void setVersion(Version v) { version_ = v; }

  Version getVersion() const { return version_; }

  bool setMethod(const char *start, const char *end)
  {
    assert(method_ == kInvalid);
    string m(start, end);
    if (m == "GET")
    {
      method_ = kGet;
    }
    else if (m == "POST")
    {
      method_ = kPost;
    }
    else if (m == "HEAD")
    {
      method_ = kHead;
    }
    else if (m == "PUT")
    {
      method_ = kPut;
    }
    else if (m == "DELETE")
    {
      method_ = kDelete;
    }
    else
    {
      method_ = kInvalid;
    }
    return method_ != kInvalid;
  }

  Method method() const { return method_; }

  const char *methodString() const
  {
    const char *result = "UNKNOWN";
    switch (method_)
    {
    case kGet:
      result = "GET";
      break;
    case kPost:
      result = "POST";
      break;
    case kHead:
      result = "HEAD";
      break;
    case kPut:
      result = "PUT";
      break;
    case kDelete:
      result = "DELETE";
      break;
    default:
      break;
    }
    return result;
  }

  void setPath(const char *start, const char *end) { path_.assign(start, end); }

  const string &path() const { return path_; }

  void setQuery(const char *start, const char *end) { query_.assign(start, end); }

  const string &query() const { return query_; }

  void setReceiveTime(Timestamp t) { receiveTime_ = t; }

  Timestamp receiveTime() const { return receiveTime_; }

  void addHeader(const char *start, const char *colon, const char *end)
  {
    string field(start, colon);
    ++colon;
    // 去除开头的空格
    while (colon < end && isspace(*colon))
    {
      ++colon;
    }
    // 去除结尾的空格
    string value(colon, end);
    while (!value.empty() && isspace(value[value.size() - 1]))
    {
      value.resize(value.size() - 1);
    }
    headers_[field] = value;
  }

  string getHeader(const string &field) const
  {
    string result;
    std::map<string, string>::const_iterator it = headers_.find(field);
    if (it != headers_.end())
    {
      result = it->second;
    }
    return result;
  }

  const std::map<string, string> &headers() const { return headers_; }

  void swap(HttpRequest &that)
  {
    std::swap(method_, that.method_);
    std::swap(version_, that.version_);
    path_.swap(that.path_);
    query_.swap(that.query_);
    receiveTime_.swap(that.receiveTime_);
    headers_.swap(that.headers_);
  }

 private:
  Method method_;                     // http方法
  Version version_;                   // http版本号
  string path_;                       // 请求路径
  string query_;                      // 请求
  Timestamp receiveTime_;             // 请求收到的时间戳
  std::map<string, string> headers_;  // http头
};

}  // namespace net
}  // namespace muduo

#endif
