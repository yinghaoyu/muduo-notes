#ifndef CONVEY_NONCOPYABLE_H
#define CONVEY_NONCOPYABLE_H

namespace convey
{
class noncopyable
{
 public:
  // 禁止了拷贝构造和拷贝赋值
  // 外部类、派生类调用引发编译出错
  noncopyable(const noncopyable &) = delete;
  void operator=(const noncopyable &) = delete;

 protected:
  // 构造函数和析构函数设置protected权限
  // 外面的类不能调用，派生类可以调用
  noncopyable() = default;
  ~noncopyable() = default;
};
}  // namespace convey
#endif
