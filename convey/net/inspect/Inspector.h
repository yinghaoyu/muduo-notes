#ifndef CONVEY_NET_INSPECT_INSPECTOR_H
#define CONVEY_NET_INSPECT_INSPECTOR_H

#include "../../base/Mutex.h"
#include "../http/HttpRequest.h"
#include "../http/HttpServer.h"

#include <map>

namespace convey
{
namespace net
{
class ProcessInspector;
class PerformanceInspector;
class SystemInspector;

// An internal inspector of the running process, usually a singleton.
// Better to run in a seperated thread, as some method may block for seconds
class Inspector : noncopyable
{
 public:
  typedef std::vector<string> ArgList;
  typedef std::function<string(HttpRequest::Method, const ArgList &args)> Callback;
  Inspector(EventLoop *loop, const InetAddress &httpAddr, const string &name);
  ~Inspector();

  /// Add a Callback for handling the special uri : /mudule/command
  void add(const string &module, const string &command, const Callback &cb, const string &help);
  void remove(const string &module, const string &command);

 private:
  typedef std::map<string, Callback> CommandList;
  typedef std::map<string, string> HelpList;

  void start();
  void onRequest(const HttpRequest &req, HttpResponse *resp);

  HttpServer server_;
  std::unique_ptr<ProcessInspector> processInspector_;
  std::unique_ptr<PerformanceInspector> performanceInspector_;
  std::unique_ptr<SystemInspector> systemInspector_;
  Mutex mutex_;
  std::map<string, CommandList> modules_;
  std::map<string, HelpList> helps_;
};

}  // namespace net
}  // namespace convey

#endif  // convey_NET_INSPECT_INSPECTOR_H
