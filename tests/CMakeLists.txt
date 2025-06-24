# 查找 GoogleTest
find_package(GTest REQUIRED)
include(GoogleTest)

# 添加测试可执行文件
add_executable(
    test_example
    example_test.cpp
)

force_redefine_file_macro_for_sources(test_example)

# 链接 GTest 和主项目库
target_link_libraries(
    test_example
    GTest::GTest GTest::Main
)

# 注册为测试用例
gtest_discover_tests(test_example)


# 另一个可执行文件
get_filename_component(ABS_SRC_DIR_INCLUDE ../include ABSOLUTE)

# get_filename_component(ABS_SRC_DIR ../src ABSOLUTE)
# message(ABS_SRC_DIR: ${ABS_SRC_DIR})
# file(GLOB SRC_LIST_test ${ABS_SRC_DIR}/*.cpp)
# list(REMOVE_ITEM SRC_LIST_test "${ABS_SRC_DIR}/main.cpp")
# message(ABS_SRC_LIST_test: ${SRC_LIST_test})
# # 使用绝对路径处理更可靠
# add_executable(mytest_example ${SRC_lIST_test} mytest.cpp)


# add_executable(mytest_example ../src/func_file.cpp mytest.cpp)
add_executable(mytest_example mytest.cpp)
force_redefine_file_macro_for_sources(mytest_example)
add_dependencies(mytest_example REACTORWEBSERVER)
target_link_libraries(mytest_example GTest::GTest GTest::Main)
# target_include_directories(mytest_example PRIVATE ../include)
target_include_directories(mytest_example PUBLIC ${ABS_SRC_DIR_INCLUDE})
gtest_discover_tests(mytest_example)



# find_package(REACTORWEBSERVER)  刚刚生成的库 不需要find_package 能够找到

add_executable(test_thread test_thread.cpp)
force_redefine_file_macro_for_sources(test_thread)
add_dependencies(test_thread REACTORWEBSERVER)
target_include_directories(test_thread PUBLIC ${ABS_SRC_DIR_INCLUDE})
target_link_libraries(test_thread PRIVATE REACTORWEBSERVER)



# test_loggers

add_executable(test_loggers test_logger.cpp)
force_redefine_file_macro_for_sources(test_loggers)
# message(messagetest_loggers: ${ABS_SRC_DIR_INCLUDE})
add_dependencies(test_loggers REACTORWEBSERVER)
target_include_directories(test_loggers PUBLIC ${ABS_SRC_DIR_INCLUDE})
target_link_libraries(test_loggers PRIVATE REACTORWEBSERVER)

#---
# test_utils

add_executable(test_utils test_util.cpp)
force_redefine_file_macro_for_sources(test_utils)
# message(messagetest_loggers: ${ABS_SRC_DIR_INCLUDE})
add_dependencies(test_utils REACTORWEBSERVER)
target_include_directories(test_utils PUBLIC ${ABS_SRC_DIR_INCLUDE})
target_link_libraries(test_utils PRIVATE REACTORWEBSERVER)


# test_fiber
add_executable(test_fiber test_fiber.cpp)
force_redefine_file_macro_for_sources(test_fiber)
# message(messagetest_loggers: ${ABS_SRC_DIR_INCLUDE})
add_dependencies(test_fiber REACTORWEBSERVER)
target_include_directories(test_fiber PUBLIC ${ABS_SRC_DIR_INCLUDE})
target_link_libraries(test_fiber PRIVATE REACTORWEBSERVER)