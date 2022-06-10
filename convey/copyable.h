#ifndef COPYABLE_H
#define COPYABLE_H

// reference: https://github.com/chenshuo/muduo/blob/master/muduo/base/copyable.h
class copyable
{
  // 构造函数设置为protected类型，类外不可构造，派生类可以
 protected:
  copyable() = default;
  ~copyable() = default;
};

#endif
