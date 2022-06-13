#include "Exception.h"
#include "CurrentThread.h"

namespace convey
{
Exception::Exception(std::string msg) : message_(std::move(msg)), stack_(CurrentThread::stackTrace(/*demangle=*/false)) {}
}  // namespace convey