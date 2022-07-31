#include <stdio.h>
#include "../convey/base/ProcessInfo.h"
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

int main()
{
  printf("pid = %d\n", convey::ProcessInfo::pid());
  printf("uid = %d\n", convey::ProcessInfo::uid());
  printf("euid = %d\n", convey::ProcessInfo::euid());
  printf("start time = %s\n", convey::ProcessInfo::startTime().toFormattedString().c_str());
  printf("hostname = %s\n", convey::ProcessInfo::hostname().c_str());
  printf("opened files = %d\n", convey::ProcessInfo::openedFiles());
  printf("threads = %zd\n", convey::ProcessInfo::threads().size());
  printf("num threads = %d\n", convey::ProcessInfo::numThreads());
  printf("status = %s\n", convey::ProcessInfo::procStatus().c_str());
}
