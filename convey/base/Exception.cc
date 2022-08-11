#include "convey/base/Exception.h"
#include "convey/base/CurrentThread.h"

namespace convey
{
Exception::Exception(string msg) : message_(std::move(msg)), stack_(CurrentThread::stackTrace(/*demangle=*/false)) {}
}  // namespace convey
