#ifndef MUDUO_BASE_TYPES_H
#define MUDUO_BASE_TYPES_H

#include <stdint.h>
#include <string.h>  // memset
#include <string>

#ifndef NDEBUG
#include <assert.h>
#endif

namespace muduo
{
using std::string;

inline void memZero(void *p, size_t n)
{
  memset(p, 0, n);
}

// Taken from google-protobuf stubs/common.h
//
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/

template <typename To, typename From>
inline To implicit_cast(From const &f)
{
  return f;
}

// When you upcast (that is, cast a pointer from type Foo to type
// SuperclassOfFoo), it's fine to use implicit_cast<>, since upcasts
// always succeed.  When you downcast (that is, cast a pointer from
// type Foo to type SubclassOfFoo), static_cast<> isn't safe, because
// how do you know the pointer is really of type SubclassOfFoo?  It
// could be a bare Foo, or of type DifferentSubclassOfFoo.  Thus,
// when you downcast, you should use this macro.  In debug mode, we
// use dynamic_cast<> to double-check the downcast is legal (we die
// if it's not).  In normal mode, we do the efficient static_cast<>
// instead.  Thus, it's important to test in debug mode to make sure
// the cast is legal!
//    This is the only place in the code we should use dynamic_cast<>.
// In particular, you SHOULDN'T be using dynamic_cast<> in order to
// do RTTI (eg code like this:
//    if (dynamic_cast<Subclass1>(foo)) HandleASubclass1Object(foo);
//    if (dynamic_cast<Subclass2>(foo)) HandleASubclass2Object(foo);
// You should design the code some other way not to need this.

template <typename To, typename From>  // use like this: down_cast<T*>(foo);
inline To down_cast(From *f)           // so we only accept pointers
{
  // Ensures that To is a sub-type of From *.  This test is here only
  // for compile-time type checking, and has no overhead in an
  // optimized build at run-time, as it will be optimized away
  // completely.
  if (false)
  {
    // 通过编译验证From是To的父类
    // 父类指针向基类指针转换
    // 开启了编译优化这一整个if会被完全优化掉
    implicit_cast<From *, To>(0);
  }

#if !defined(NDEBUG) && !defined(GOOGLE_PROTOBUF_NO_RTTI)
  // 在debug模式的时候用dynamic_cast及时补抓错误，有点类似assert的味道
  assert(f == NULL || dynamic_cast<To>(f) != NULL);  // RTTI: debug mode only!
#endif
  return static_cast<To>(f);  // 在实际运行时用static_cast效率更高
}

}  // namespace muduo

#endif

// 举例说明
// class Top
// {
// };
// class A : Top
// {
// };
// class B : Top
// {
// };
// class Bottom : A, B
// {
// };
// void fun(A &) {}
// void fun(B &) {}
// void test()
// {
//   Bottom btm;
//   fun(btm);  // 这里编译会出错 Call to 'fun' is ambiguous
// }
// 在菱形继承中，可以把上面改为static_cast<A&>(btm)或者static_cast<B&>(btm)
// void test()
// {
//   Top top;
//   // 这里编译会通过，但是运行会崩溃
//   // static_cast不进行真正的类型检查(在down_cast和up_cast的时候)
//   fun(static_cast<A>(btm));
// }
// implicit_cast只能用在up_cast
// 当需要down_cast(这种形式)时候我们就应该使用上面的down_cast(上面的代码)
// down_cast在debug模式下内部使用了dynamic_cast进行验证
// 在release下使用static_cast替换dynamic_cast
// 为什么使用down_cast而不直接使用dynamic_cast?
// 1.因为但凡程序设计正确，dynamic_cast就可以用static_cast来替换，而后者比前者更有效率。
// 2.dynamic_cast可能失败(在运行时crash)，运行时RTTI不是好的设计，不应该在运行时RTTI或者需要RTTI时一般都有更好的选择。
