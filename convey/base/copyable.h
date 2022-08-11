#ifndef CONVEY_BASE_COPYABLE_H
#define CONVEY_BASE_COPYABLE_H

namespace convey
{
// reference: https://github.com/chenshuo/muduo/blob/master/muduo/base/copyable.h
class copyable
{
  // 构造函数设置为protected类型，类外不可构造，派生类可以
 protected:
  copyable() = default;
  ~copyable() = default;
};
}  // namespace convey

#endif
