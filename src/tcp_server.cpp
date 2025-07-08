#include "tcp_server.h"
#include "controllogger.h"
#include "config.h"

namespace version04
{
    static version04::ConfigVar<uint64_t>::ptr g_tcp_server_read_timeout = version04::Config::Create<uint64_t>("tcp_server.read_timeout", (uint64_t)(60 * 1000 * 2), "tcp server read timeout");

    TcpServer::TcpServer(version04::IOManager *worker, version04::IOManager *accept_worker)
        : m_worker(worker), m_acceptWorker(accept_worker), m_recvTimeout(g_tcp_server_read_timeout->getValue()), m_name("TcpServerVersion04/1.0.1"), m_isStop(true)
    {
    }

    TcpServer::~TcpServer()
    {
        for (auto &item : m_socks)
        {
            item->close();
        }
        m_socks.clear();
    }

    bool TcpServer::bind(version04::Address::ptr addr, bool ssl)
    {
        std::vector<Address::ptr> addrs;
        std::vector<Address::ptr> fails;
        addrs.push_back(addr);
        return bind(addrs, fails, ssl);
    }

    bool TcpServer::bind(const std::vector<version04::Address::ptr> &addrs, std::vector<version04::Address::ptr> &fails, bool ssl)
    {
        for (auto &addr : addrs)
        {
            Socket::ptr sock = Socket::CreateTCP(addr);
            if (!sock->bind(addr))
            {
                ULOG_ERROR_SRC("system", "bind fail errno = {} errstr = {},addr = [{}]", errno, strerror(errno), addr->toString());
                fails.push_back(addr);
                continue;
            }
            if (!sock->listen())
            {
                ULOG_ERROR_SRC("system", "listen fail errno = {} errstr = {},addr = [{}]", errno, strerror(errno), addr->toString());
                fails.push_back(addr);
                continue;
            }
            m_socks.push_back(sock);
        }
        if (!fails.empty())
        {
            // 如果fails不是空的，也就是说有错误的情况，这就要清空吗，
            m_socks.clear();
            return false;
        }
        for (auto &sock : m_socks)
        {
            ULOG_INFO_SRC("main", "server bind success: {}", sock->getSocket());
        }
        return true;
    }
    void TcpServer::handleClient(version04::Socket::ptr client)
    {
        ULOG_INFO_SRC("main", "handle client: {}", client->getSocket());
    }
    void TcpServer::startAccept(version04::Socket::ptr sock)
    {
        while (!m_isStop)
        {
            Socket::ptr client = sock->accept();
            if (client)
            {
                client->setRecvTimeout(m_recvTimeout);
                m_worker->schedule(std::bind(&TcpServer::handleClient, shared_from_this(), client));
            }
            else
            {
                ULOG_ERROR_SRC("main", "accept errno={} errstr={}", errno, strerror(errno));
            }
        }
    }
    bool TcpServer::start()
    {
        if (!m_isStop)
        {
            // 表示开始过了
            return true;
        }
        m_isStop = false;
        for (auto $sock : m_socks)
        {
            m_acceptWorker->schedule(std::bind(&TcpServer::startAccept, shared_from_this(), $sock));
        }
        return true;
    }

    void TcpServer::stop()
    {
        m_isStop = true;
        auto self = shared_from_this();
        m_acceptWorker->schedule([this, self]()
                                 {
            for(auto& sock:m_socks)
            {
                sock->cancelAll();
                sock->close();
            }
            m_socks.clear(); });
    }
}