#include "convey/net/http/HttpContext.h"
#include "convey/net/Buffer.h"

using namespace convey;
using namespace convey::net;

bool HttpContext::processRequestLine(const char *begin, const char *end)
{
  bool succeed = false;
  const char *start = begin;
  const char *space = std::find(start, end, ' ');
  if (space != end && request_.setMethod(start, space))
  {
    start = space + 1;
    space = std::find(start, end, ' ');
    if (space != end)
    {
      const char *question = std::find(start, space, '?');
      if (question != space)
      {
        request_.setPath(start, question);
        request_.setQuery(question, space);
      }
      else
      {
        request_.setPath(start, space);
      }
      start = space + 1;
      succeed = end - start == 8 && std::equal(start, end - 1, "HTTP/1.");
      if (succeed)
      {
        if (*(end - 1) == '1')
        {
          request_.setVersion(HttpRequest::kHttp11);
        }
        else if (*(end - 1) == '0')
        {
          request_.setVersion(HttpRequest::kHttp10);
        }
        else
        {
          succeed = false;
        }
      }
    }
  }
  return succeed;
}

// HTTP协议格式
//  ----------------------------------------
// |请求方法|space|URL|space|协议版本|\r|\n|    --> 请求行
// |头部字段名|:|          值        |\r|\n|
//              .   .   .
//              .   .   .                       --> 请求头部
//              .   .   .
// |头部字段名|:|          值        |\r|\n|
//                                   |\r|\n|
// |              数据                     |    ---> 请求数据

// return false if any error
bool HttpContext::parseRequest(Buffer *buf, Timestamp receiveTime)
{
  bool ok = true;
  bool hasMore = true;
  while (hasMore)  // 解析buf的状态机
  {
    if (state_ == kExpectRequestLine)
    {
      const char *crlf = buf->findCRLF();  // 找到'\r\n'
      if (crlf)
      {
        ok = processRequestLine(buf->peek(), crlf);  // 解析请求行
        if (ok)
        {
          request_.setReceiveTime(receiveTime);
          buf->retrieveUntil(crlf + 2);
          state_ = kExpectHeaders;
        }
        else
        {
          hasMore = false;
        }
      }
      else
      {
        hasMore = false;
      }
    }
    else if (state_ == kExpectHeaders)
    {
      const char *crlf = buf->findCRLF();
      if (crlf)
      {
        const char *colon = std::find(buf->peek(), crlf, ':');
        if (colon != crlf)
        {
          request_.addHeader(buf->peek(), colon, crlf);
        }
        else
        {
          // empty line, end of header
          // FIXME:
          state_ = kGotAll;
          hasMore = false;
        }
        buf->retrieveUntil(crlf + 2);
      }
      else
      {
        hasMore = false;
      }
    }
    else if (state_ == kExpectBody)
    {
      // FIXME:
    }
  }
  return ok;
}
