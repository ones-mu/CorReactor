#pragma once

#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <stdint.h>
#include <vector>
#include <string>
#include <sstream>
#include <cstdlib> //用于atoi
#include <sstream>
namespace version04
{

    // 全局的方法用大写开头

    pid_t GetThreadId();
    uint64_t GetFiberId();

    void initLogs();
    /**
     * @brief 获取当前的调用栈
     * @param[out] bt 保存调用栈
     * @param[in] size 最多返回层数
     * @param[in] skip 跳过栈顶的层数
     */
    void Backtrace(std::vector<std::string> &bt, int size, int skip = 1);
    /**
     * @brief 获取当前栈信息的字符串
     * @param[in] size 栈的最大层数
     * @param[in] skip 跳过栈顶的层数
     * @param[in] prefix 栈信息前输出的内容
     */
    std::string BacktraceToString(int size, int skip, const std::string &prefix);

    int evaluate_expression(const std::string &expr);

    /**
     * @brief 获取当前时间的毫秒
     */
    uint64_t GetCurrentMS();

    /**
     * @brief 获取当前时间的微秒
     */
    uint64_t GetCurrentUS();

    std::string Time2Str(time_t ts = time(0), const std::string& format = "%Y-%m-%d %H:%M:%S");
}
