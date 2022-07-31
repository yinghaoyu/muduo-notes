#include "../../net/http/HttpResponse.h"
#include "../../net/Buffer.h"

#include <stdio.h>

using namespace convey;
using namespace convey::net;

void HttpResponse::appendToBuffer(Buffer *output) const
{
  char buf[32];
  snprintf(buf, sizeof buf, "HTTP/1.1 %d ", statusCode_);
  // 添加请求行
  output->append(buf);
  output->append(statusMessage_);
  output->append("\r\n");

  // 添加请求头部
  if (closeConnection_)
  {
    output->append("Connection: close\r\n");
  }
  else
  {
    snprintf(buf, sizeof buf, "Content-Length: %zd\r\n", body_.size());
    output->append(buf);
    output->append("Connection: Keep-Alive\r\n");
  }

  for (const auto &header : headers_)
  {
    output->append(header.first);
    output->append(": ");
    output->append(header.second);
    output->append("\r\n");
  }

  output->append("\r\n");
  // 添加请求体
  output->append(body_);
}
