#pragma once

#include "iomanager.h"
#include "address.h"
#include "socket.h"
#include "noncopyable.h"

#include <memory>
#include <functional>
#include <vector>
#include <string>
#include <cstdint>

namespace version04
{

    /**
     * @brief TCP Server 封装
     */
    class TcpServer : public std::enable_shared_from_this<TcpServer>, Noncopyable
    {
    public:
        using ptr = std::shared_ptr<TcpServer>;
        /**
         * @brief 构造函数
         * @param[in] worker socket客户端工作的协程调度器
         * @param[in] accept_worker 服务器socket执行接收socket连接的协程调度器
         */
        TcpServer(version04::IOManager *worker = version04::IOManager::GetThis(), version04::IOManager *accept_worker = version04::IOManager::GetThis());
        /**
         * @brief 虚析构函数
         */
        virtual ~TcpServer();
        /**
         * @brief 绑定地址
         * @return 返回是否绑定成功
         */
        virtual bool bind(version04::Address::ptr addr, bool ssl = false);
        /**
         * @brief 绑定地址数组
         * @param[in] addrs 需要绑定的地址数组
         * @param[out] fails 绑定失败的地址
         * @return 返回是否绑定成功
         */
        virtual bool bind(const std::vector<version04::Address::ptr> &addrs, std::vector<version04::Address::ptr> &fails, bool ssl = false);
        /**
         * @brief 启动服务
         * @return 返回是否启动成功
         */
        virtual bool start();
        /**
         * @brief 停止服务
         */
        virtual void stop();
        /**
         * @brief 返回读取超时时间（ms）
         */
        uint64_t getRecvTimeout() const { return m_recvTimeout; }
        /**
         * @brief 返回服务器名称
         */
        std::string getName() const { return m_name; }
        /**
         * @brief 设置读取超时时间（ms）
         */
        void setRecvTimeout(uint64_t v) { m_recvTimeout = v; }
        /**
         * @brief 设置服务器名称
         */
        void setName(const std::string &v) { m_name = v; }
        /**
         * @brief 是否停止
         */
        bool isSTop() const { return m_isStop; }

    protected:
        /**
         * @brief 处理新的客户端连接
         */
        virtual void handleClient(version04::Socket::ptr client);
        /**
         * @brief 开始接收连接
         */
        virtual void startAccept(version04::Socket::ptr sock);

    private:
        //要保证成员变量的声明顺序和构造函数中保持一致
        /// @brief 新连接的Socket工作的调度器
        IOManager *m_worker;
        /// @brief 服务器Socket接收连接的调度器
        IOManager *m_acceptWorker;
        /// @brief 接收超时时间（ms）
        uint64_t m_recvTimeout;
        /// @brief 服务器名称
        std::string m_name;
        /// @brief 是否停止
        bool m_isStop;
        /// @brief 监听Socket数组
        std::vector<version04::Socket::ptr> m_socks;
    };
}
