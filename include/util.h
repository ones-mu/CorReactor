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
namespace version04 {

//全局的方法用大写开头

pid_t GetThreadId();
uint32_t GetFiberId();

void initLogs();

void Backtrace(std::vector<std::string>&bt,int size,int skip=1);

std::string BacktraceToString(int size,int skip,const std::string&prefix);



}


