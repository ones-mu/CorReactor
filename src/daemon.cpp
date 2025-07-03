#include "daemon.h"
#include "config.h"
#include "controllogger.h"
#include "util.h"

#include <time.h>
#include <error.h>
#include <sys/types.h>
#include <sys/wait.h>

namespace version04
{
    static version04::ConfigVar<uint32_t>::ptr g_daemon_restart_interval = version04::Config::Create<uint32_t>("daemon.restart_interval", static_cast<uint32_t>(5), "daemon restart interval");

    std::string ProcessInfo::toString() const
    {
        std::stringstream ss;
        ss << "[ProcessInfo parent_id=" << parent_id
           << " main_id=" << main_id
           << " parent_start_time=" << version04::Time2Str(parent_start_time)
           << " main_start_time=" << version04::Time2Str(main_start_time)
           << " restart_count=" << restart_count << "]";
        return ss.str();
    }

    static int real_start(int argc, char **argv, std::function<int(int argc, char **argv)> main_cb)
    {
        return main_cb(argc, argv);
    }

    static int real_deamon(int argc, char **argv,
                           std::function<int(int argc, char **argv)> main_cb)
    {
        ProcessInfoMgr::GetInstance()->parent_id = getpid();
        ProcessInfoMgr::GetInstance()->parent_start_time = time(0);

        while (true)
        {
            pid_t pid = fork();
            if (pid == 0)
            {
                // 子进程，子进程启动后返回
                ProcessInfoMgr::GetInstance()->main_id = getpid();
                ProcessInfoMgr::GetInstance()->main_start_time = time(0);
                ULOG_INFO_SRC("main", "process start pid={}", getpid());
                return real_start(argc, argv, main_cb);
            }
            else if (pid < 0)
            {
                // fork失败
                ULOG_ERROR_SRC("system", "fork failed return = {}, errno = {}, errstr = {}", pid, errno, strerror(errno));
                return -1;
            }
            else
            {
                // 父进程返回
                int status = 0;
                waitpid(pid, &status, 0);
                if (status)
                {
                    ULOG_ERROR_SRC("system", "child crash pid={}, status={}", pid, status);
                }
                else
                {
                    ULOG_INFO_SRC("main", "child finished pid={}", pid);
                    break;
                }
                ProcessInfoMgr::GetInstance()->restart_count += 1;
                sleep(g_daemon_restart_interval->getValue());
            }
        }
        return 0;
    }

    int start_daemon(int argc, char **argv, std::function<int(int argc, char **argv)> main_cb, bool is_daemon)
    {
        if (!is_daemon)
        {
            return real_start(argc, argv, main_cb);
        }
        return real_deamon(argc, argv, main_cb);
    }

}