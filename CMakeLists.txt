cmake_minimum_required(VERSION 3.21)
project(REACTORWEBSERVER)

include (cmake/utils.cmake)

# 1. 基本配置
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_ENABLE_EXPORTS ON)  # -rdynamic

# 2. 输出目录设置 只会控制可执行文件的输出路径
set(OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
set(EXECUTABLE_OUTPUT_PATH ${OUTPUT_DIRECTORY}/output)

## 控制动态链接库的输出路径
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)  ##这样后会输出在build下的lib文件夹下

# 3. C++标准设置
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 4. 按构建类型设置
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -ggdb -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# 5. 源文件收集（建议改为显式列表）
# file(GLOB SRC_LIST ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
# set(SRC_LIST src/func_file.cpp 
#     src/util.cpp
#     src/thread.cpp
#     src/main.cpp)

# 6. 添加可执行文件
# add_executable(${PROJECT_NAME} ${SRC_LIST})

set(SRC_LIST_LIBRARY 
    src/logsModule_init.cpp
    src/util.cpp
    src/thread.cpp
    src/controllogger.cpp
    src/fiber.cpp
    src/scheduler.cpp
    src/timer.cpp
    src/iomanager.cpp
    src/fd_manager.cpp
    src/address.cpp
    src/hook.cpp
    src/config.cpp
)

add_library(${PROJECT_NAME} SHARED ${SRC_LIST_LIBRARY})

# 强制覆盖 __FILE__ 宏
force_redefine_file_macro_for_sources(${PROJECT_NAME})


# 7. 目标级配置
target_include_directories(${PROJECT_NAME} PUBLIC include)

target_compile_options(${PROJECT_NAME} PRIVATE
    "$<$<CXX_COMPILER_ID:GNU,Clang>:-Wall>"
    "$<$<CXX_COMPILER_ID:GNU,Clang>:-Wextra>"
    "$<$<CXX_COMPILER_ID:GNU,Clang>:-Werror>"
    "$<$<CXX_COMPILER_ID:GNU,Clang>:-Wno-builtin-macro-redefined>"  # 禁用 -Werror 对 __FILE__ 重定义的警告​​ （其实是忽略了内置宏被重新定义的警告）
    "$<$<CXX_COMPILER_ID:GNU,Clang>:-Wno-deprecated-declarations>"
    "$<$<CXX_COMPILER_ID:GNU,Clang>:-Wno-unused-function>"
    "$<$<CXX_COMPILER_ID:GNU,Clang>:-Wno-unused-parameter>"      # 允许未使用的函数参数
    # "$<$<CXX_COMPILER_ID:GNU,Clang>:-Wno-implicit-fallthrough>"  # 允许 case 穿透 允许没有break
    # "$<$<CXX_COMPILER_ID:GNU,Clang>:-Wno-deprecated-enum-enum-conversion>"  # 允许不同枚举类型的位运算
)

# 8. 线程支持
find_package(Threads REQUIRED)
find_package(yaml-cpp REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)
if(TARGET yaml-cpp::yaml-cpp)
    target_link_libraries(${PROJECT_NAME} PRIVATE yaml-cpp::yaml-cpp)
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE yaml-cpp)
endif()


# 9. SPDLOG 配置
option(SPDLOG_LEVEL "Set SPDLOG_ACTIVE_LEVEL (default: INFO)" INFO)
set(SPDLOG_LEVEL_VALUES TRACE DEBUG INFO WARN ERROR CRITICAL OFF)
set_property(CACHE SPDLOG_LEVEL PROPERTY STRINGS ${SPDLOG_LEVEL_VALUES})

target_compile_definitions(${PROJECT_NAME} PRIVATE 
    SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_${SPDLOG_LEVEL}
)

# 10. 包含子目录
add_subdirectory(tests)