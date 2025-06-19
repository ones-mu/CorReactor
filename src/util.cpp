#include "util.h"

namespace version04 {
pid_t GetThreadId() {
    return syscall(SYS_gettid);  //获取的是子线程的Id而不是进程的Id 完全等价于gettid()
}

uint32_t GetFiberId() {
    return 0;
}

}
