#ifndef CONVEY_NET_INSPECT_PROCESSINSPECTOR_H
#define CONVEY_NET_INSPECT_PROCESSINSPECTOR_H

#include "convey/net/inspect/Inspector.h"

namespace convey
{
namespace net
{
class ProcessInspector : noncopyable
{
 public:
  void registerCommands(Inspector *ins);

  static string overview(HttpRequest::Method, const Inspector::ArgList &);
  static string pid(HttpRequest::Method, const Inspector::ArgList &);
  static string procStatus(HttpRequest::Method, const Inspector::ArgList &);
  static string openedFiles(HttpRequest::Method, const Inspector::ArgList &);
  static string threads(HttpRequest::Method, const Inspector::ArgList &);

  static string username_;
};

}  // namespace net
}  // namespace convey

#endif
