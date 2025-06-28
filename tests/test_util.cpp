#include "macro.h"
#include "util.h"
#include "controllogger.h"

//直到是调用test_assert()触发了断言，但不知道是谁调用了test_assert()，所以无法确定是哪个文件触发了断言。
//如果可以将栈上的一些信息输出，那就可以很快的找到这个问题
void test_assert() {
    // assert(0);
    ULOG_ERROR_SRC("system","test_assert");
    VERSION04_ASSERT2(0==1,"test_assert func string");
}

int main(int argc, char* argv[])
{
    version04::initLogs();
    test_assert();
    return  0;
}