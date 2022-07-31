#ifndef CONVEY_NET_INSPECT_SYSTEMINSPECTOR_H
#define CONVEY_NET_INSPECT_SYSTEMINSPECTOR_H

#include "../inspect/Inspector.h"

namespace convey
{
namespace net
{
class SystemInspector : noncopyable
{
 public:
  void registerCommands(Inspector *ins);

  static string overview(HttpRequest::Method, const Inspector::ArgList &);
  static string loadavg(HttpRequest::Method, const Inspector::ArgList &);
  static string version(HttpRequest::Method, const Inspector::ArgList &);
  static string cpuinfo(HttpRequest::Method, const Inspector::ArgList &);
  static string meminfo(HttpRequest::Method, const Inspector::ArgList &);
  static string stat(HttpRequest::Method, const Inspector::ArgList &);
};

}  // namespace net
}  // namespace convey

#endif
