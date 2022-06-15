add_executable(test_thread test_thread.cc)
target_link_libraries(test_thread convey_base)

# 增加测试项，需要开启enable_testing()
add_executable(test_atomic test_atomic.cc)
add_test(NAME test_atomic COMMAND test_atomic)

add_executable(test_exception test_exception.cc)
target_link_libraries(test_exception convey_base)
