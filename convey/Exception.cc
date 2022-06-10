#include "Exception.h"

Exception::Exception(std::string msg) : m_message(std::move(msg)), m_stack() {}
