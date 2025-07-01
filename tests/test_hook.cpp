#include "util.h"
#include "iomanager.h"
#include "hook.h"
#include "controllogger.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>


void test_sleep() {
    version04::IOManager iom(1);
    iom.schedule([](){
        sleep(2);
        ULOG_INFO_SRC("main", "sleep 2");
    });

    iom.schedule([](){
        sleep(3);
        ULOG_INFO_SRC("main", "sleep 3");
    });
    ULOG_INFO_SRC("main", "test_sleep");
}


void test_sock()
{
    int sockfd=socket(AF_INET,SOCK_STREAM,0);
    sockaddr_in addr;
    memset(&addr,0,sizeof(addr));
    addr.sin_family=AF_INET;
    addr.sin_port=htons(80);
    inet_pton(AF_INET,"127.0.0.1",&addr.sin_addr);
    ULOG_INFO_SRC("main","begin connect");
    int rt = connect(sockfd, (const sockaddr*)&addr, sizeof(addr));
    ULOG_INFO_SRC("main","connect rt={} errno={}",rt,errno);

    if(rt) {
        return;
    }

    const char data[] = "GET / HTTP/1.0\r\n\r\n";
    rt = send(sockfd, data, sizeof(data), 0);
    ULOG_INFO_SRC("main","send rt={} errno={}",rt,errno);

    if(rt <= 0) {
        return;
    }

    std::string buff;
    buff.resize(4096);

    rt = recv(sockfd, &buff[0], buff.size(), 0);
    ULOG_INFO_SRC("main","recv rt={} errno={}",rt,errno);

    if(rt <= 0) {
        return;
    }

    buff.resize(rt);
    ULOG_INFO_SRC("main","recv data={}",buff);
}


int main(void)
{
    version04::initLogs();
    test_sleep();
    // version04::IOManager iom;
    // iom.schedule(test_sock);
    return 0;
}