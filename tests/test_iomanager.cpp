#include "iomanager.h"
#include "util.h"
#include "controllogger.h"
#include <sys/epoll.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>

int sock = 0;
void test_fiber()
{
    ULOG_INFO_SRC("main", "test fiber sock = {} ", sock);
    sock = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(sock, F_SETFL, O_NONBLOCK);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (!connect(sock, (const sockaddr *)&addr, sizeof(addr)))
    {
    }
    else if (errno == EINPROGRESS)
    {
        ULOG_INFO_SRC("main", "add event errno={} {}", errno, strerror(errno));
        version04::IOManager::GetThis()->addEvent(sock, version04::IOManager::Event::READ, []()
                                              { ULOG_INFO_SRC("main","read callback"); });
        version04::IOManager::GetThis()->addEvent(sock, version04::IOManager::Event::WRITE, []()
                                              {
            ULOG_INFO_SRC("main","write callback");
            //close(sock);
            version04::IOManager::GetThis()->cancelEvent(sock, version04::IOManager::Event::READ);
            close(sock); });
    }
    else
    {
        ULOG_INFO_SRC("main", "else, errno={} {}", errno, strerror(errno));
    }
}

void test1()
{
    ULOG_INFO_SRC("main", "test1,EPOLLIN={} EPOLLOUT={}", static_cast<uint32_t>(EPOLLIN), static_cast<uint32_t>(EPOLLOUT));
    version04::IOManager iom(2);
    iom.schedule(&test_fiber);
}

version04::Timer::ptr s_timer;
void test_timer()
{
    version04::IOManager iom(2);
    s_timer = iom.addTimer(1000, []()
                           {
        static int i=0;
        ULOG_INFO_SRC("main","test_timer, i={}",i);
        if(++i==3)
        {
            s_timer->reset(2000,true);
        } }, true);
}

int main(void)
{
    version04::initLogs();
    // test_timer();
    test1();
    return 0;
}