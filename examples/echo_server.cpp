#include "tcp_server.h"
#include "controllogger.h"
#include "iomanager.h"
#include "bytearray.h"
#include "socket.h"
#include "util.h"

class EchoServer : public version04::TcpServer
{
public:
    EchoServer(int type); // 选择文本协议还是二进制协议
    void handleClient(version04::Socket::ptr client);

private:
    int m_type = 0; // type=1 表示文本
};
EchoServer::EchoServer(int type) : m_type(type)
{
}
void EchoServer::handleClient(version04::Socket::ptr client)
{
    ULOG_INFO_SRC("main", "handle client: {}", client->getSocket());
    version04::ByteArray::ptr ba(new version04::ByteArray);
    while(true)
    {
        ba->clear();
        std::vector<iovec> iovs;
        ba->getWriteBuffers(iovs,1024);
        client->recv()
    }
}

int main(int argc, char *argv[])
{
    version04::initLogs();
    return 0;
}