#include "tcp_server.h"
#include "iomanager.h"
#include "controllogger.h"
#include "util.h"


void run()
{
    // version04::Address::GetInterfaceAddresses();//获取网卡地址
    auto addr = version04::Address::LookupAny("0.0.0.0:8033");
    // auto addr2 = version04::UnixAddress::ptr(new version04::UnixAddress("/tmp/unix_addr"));
    std::vector<version04::Address::ptr> addrs;
    addrs.push_back(addr);
    // addrs.push_back(addr2);

    version04::TcpServer::ptr tcp_server(new version04::TcpServer);
    std::vector<version04::Address::ptr> fails;
    while(!tcp_server->bind(addrs, fails)) {
        sleep(2);
    }
    tcp_server->start(); 
}



int main(int argc , char* argv[])
{
    version04::initLogs();
    version04::IOManager io_manager(2);
    // version04::TcpServer::ptr tcp_server(new version04::TcpServer);
    // auto addr = version04::Address::LookupAny("0.0.0.0:8030");
    // tcp_server->bind(addr, false);
    // tcp_server->start();
    io_manager.schedule(run);
    return 0;
}