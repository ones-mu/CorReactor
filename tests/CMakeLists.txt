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

target_link_libraries(mytest_example GTest::GTest GTest::Main)
# target_include_directories(mytest_example PRIVATE ../include)
target_include_directories(mytest_example PRIVATE ${ABS_SRC_DIR_INCLUDE})
gtest_discover_tests(mytest_example)



# find_package(REACTORWEBSERVER)
