cmake_minimum_required(VERSION 2.6)

# LANGUAGES选项可指定构建工程需要的编程语言
project(convey C CXX)

# 为何注释后还能生成test?
# enable_testing()

# 如果没设置CMAKE_BUILD_TYPE，就默认设置为Release
if(CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

set(CXX_FLAGS
  -ggdb # gdb 调试选项
  -Wall # warn all意为显示所有警告
  -Wextra # 增加-Wall未包含的选项，具体见gcc手册
  -Werror # warn error 意为把警告当错误处理
  -Wconversion # 警告可能改变值的隐式转换
  -Wold-style-cast # 如果使用旧样式（C 样式）强制转换为非void类型，则发出警告
  -Woverloaded-virtual # 如果基类函数被子类函数隐藏，就引发编译错误，注意覆盖、隐藏、重载区别
  -Wshadow # 当一个局部变量遮盖了另一个局部变量，或者全局变量，则发出警告
  -Wpointer-arith # 对函数指针或者void *类型的指针进行算术操作时给出警告，如sizeof(void*)
  -Wwrite-strings # 把常量字符串的地址给一个可变指针时，则发出警告，如char* str = "abc"
  -std=c++11 # 指定语言标准为c++11
  -march=native # 编译代码的cpu和运行bin的cpu不匹配，使bin不能运行
  )

add_subdirectory(convey)
add_subdirectory(tests)
