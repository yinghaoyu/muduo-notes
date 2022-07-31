#ifndef CONVEY_SINGLETON_H
#define CONVEY_SINGLETON_H

#include "noncopyable.h"

#include <assert.h>
#include <pthread.h>
#include <stdlib.h>  // atexit

namespace convey
{
namespace detail
{
template <typename T>
struct has_no_destroy
{
  // This doesn't detect inherited member functions!
  // http://stackoverflow.com/questions/1966362/sfinae-to-check-for-inherited-member-functions
  // decltype是declare type的缩写，这里的作用是检测是否有成员函数
  template <typename C>
  static char test(decltype(&C::no_destroy));

  template <typename C>
  static int32_t test(...);

  const static bool value = sizeof(test<T>(0)) == 1;
};
}  // namespace detail

template <typename T>
class Singleton : public noncopyable
{
 public:
  Singleton() = delete;
  ~Singleton() = delete;

  static T &instance()
  {
    pthread_once(&ponce_, &Singleton::init);
    assert(value_ != NULL);
    return *value_;
  }

 private:
  static void init()
  {
    value_ = new T();
    if (!detail::has_no_destroy<T>::value)
    {
      ::atexit(destory);
    }
  }

  static void destory()
  {
    typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1];
    T_must_be_complete_type dummy;  // dummpy意为假人，模型
    (void) dummy;

    delete value_;
    value_ = NULL;
  }

 private:
  static pthread_once_t ponce_;
  static T *value_;
};

// 初始化静态变量
template <typename T>
pthread_once_t Singleton<T>::ponce_ = PTHREAD_ONCE_INIT;

template <typename T>
T *Singleton<T>::value_ = NULL;
}  // namespace convey
#endif
