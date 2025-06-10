cmake_minimum_required(VERSION 3.21)
project(REACTORWEBSERVER)

set(CMAKE_CXX_STANDARD 20) #设置c++标准

#配置 CMake 生成可调试的二进制文件
set(CMAKE_BUILD_TYPE Debug)

#指定编译器标志
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_C_FLAGS_DEBUG "-g")

# SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
# SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

set(OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
set(EXECUTABLE_OUTPUT_PATH ${OUTPUT_DIRECTORY}/output)

set(MySQL_INCLUDE_DIR "/usr/include/mysql")
set(MySQL_LIBRARY "/usr/lib/x86_64-linux-gnu/libmysqlclient.so")




# find_package(MySQL REQUIRED COMPONENTS Client)
# find_package(MySQL REQUIRED)
# find_package(PkgConfig REQUIRED)
# pkg_search_module(MYSQL REQUIRED mysqlclient)

#头文件

file(GLOB SRC_lIST ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
file(GLOB SRC_LIST_C ${CMAKE_CURRENT_BINARY_DIR}/src/*.c)

# 新增：格式化库选择选项 ----------------------------------------
option(USE_STD_FORMAT "Use C++20 std::format instead of fmt (requires C++20)" OFF)
if(USE_STD_FORMAT)
    # 如果启用 std::format，需要 C++20
    # set(CMAKE_CXX_STANDARD 20)
    # target_compile_definitions(${PROJECT_NAME} PRIVATE SPDLOG_USE_STD_FORMAT)
    add_definitions(-DSPDLOG_USE_STD_FORMAT)  #gcc 12以上才支持，我现在安装的是11 推荐直接装13以上
    message(STATUS "Using C++20 std::format")
else()
    message(STATUS "Using bundled fmt library")
endif()


option(ENABLE_DEBUG_PRINT "Enable debug printing in code" OFF)
if(ENABLE_DEBUG_PRINT)
# add_definitions(-DDEBUG_PRINT)
add_definitions(-DDEBUG) 
endif()
#PROJECT_NAME 项目名称的cmake全局变量  #add_executable是编译生成可执行文件,add_library是编译生成库文件（包括静态和动态）
add_executable(${PROJECT_NAME} ${SRC_lIST} ${SRC_LIST_C})

# include_directories(${PROJECT_SOURCE_DIR}/include)
#包含MysQl的头文件
# target_include_directories(${PROJECT_NAME} PRIVATE include ${MYSQL_INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} PRIVATE include ${MySQL_INCLUDE_DIR})
# target_include_directories(${PROJECT_NAME} PRIVATE external/spdlog)

# 线程支持（如果使用 spdlog 异步日志）  gcc编译器版本高应该是不用显示写的
find_package(Threads REQUIRED)

# target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads MySQL::Client)
# target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads MySQL::MySQL)
# target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads ${MYSQL_LIBRARIES})
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads ${MySQL_LIBRARY})



# 新增：SPDLOG 日志级别控制选项
option(SPDLOG_LEVEL "Set SPDLOG_ACTIVE_LEVEL (default: INFO)" INFO)
set(SPDLOG_LEVEL_VALUES TRACE DEBUG INFO WARN ERROR CRITICAL OFF)
set_property(CACHE SPDLOG_LEVEL PROPERTY STRINGS ${SPDLOG_LEVEL_VALUES})

# 设置 SPDLOG 的编译时日志级别
#target_compile_definitions需要添加在 add_executable() 或 add_library()之后 
target_compile_definitions(${PROJECT_NAME} PRIVATE 
    SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_${SPDLOG_LEVEL}
)
