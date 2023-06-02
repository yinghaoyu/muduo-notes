#ifndef MUDUO_BASE_THREADLOCAL_H
#define MUDUO_BASE_THREADLOCAL_H

#include "muduo/base/Mutex.h"
#include "muduo/base/noncopyable.h"

namespace muduo
{
template <typename T>
class ThreadLocal : public noncopyable
{
 public:
  ThreadLocal() { MCHECK(pthread_key_create(&pkey_, &ThreadLocal::destructor)); }
  ~ThreadLocal() { MCHECK(pthread_key_delete(pkey_)); }

  // 返回的是T的引用
  T &value()
  {
    T *perThreadValue = static_cast<T *>(pthread_getspecific(pkey_));
    if (!perThreadValue)
    {
      T *newOBj = new T();
      // 线程局部变量存的是堆上的指针
      MCHECK(pthread_setspecific(pkey_, newOBj));
      perThreadValue = newOBj;
    }
    return *perThreadValue;
  }

 private:
  static void destructor(void *x)
  {
    T *obj = static_cast<T *>(x);
    // 静态检查是否是完整类型
    // 什么是不完整类型？
    // 比如只声明不定义的类、结构体、数组
    // class test；
    // struct test;
    // extern int test[];
    typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1];
    T_must_be_complete_type dummy;
    (void) dummy;
    delete obj;
  }

 private:
  pthread_key_t pkey_;
};
}  // namespace muduo

#endif
