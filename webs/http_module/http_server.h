/**
 * @file http_server.h
 * @author your name (you@domain.com)
 * @brief HTTP服务器封装
 * @version 0.1
 * @date 2024-05-23
 * 
 * 
 */
#ifndef __HTTP_SERVER_H__
#define __HTTP_SERVER_H__

#include "../net_module/tcp_server.h"
#include "servlet.h"

namespace webs {
namespace http {
class HttpServer : public TcpServer {
public:
    typedef std::shared_ptr<HttpServer> ptr;

    /**
     * @brief Construct a new Http Server object
     * 
     * @param keepalive 
     * @param worker 
     * @param io_worker 
     * @param accept_worker 
     */
    HttpServer(bool keepalive = false, webs::IOManager *worker = webs::IOManager::GetThis(),
               webs::IOManager *io_worker = webs::IOManager::GetThis(), webs::IOManager *accept_worker = webs::IOManager::GetThis());

    /**
     * @brief 设置ServletDispatch
     * 
     * @param value 
     */
    void setServletDispatch(ServletDispatch::ptr value) {
        m_dispatch = value;
    }

    /**
     * @brief 获取ServletDispatch
     * 
     * @return ServletDispatch::ptr 
     */
    ServletDispatch::ptr getServletDispatch() const {
        return m_dispatch;
    }

    /**
     * @brief 设置服务器名称、设置分发器的 默认servlet
     * 
     * @param nam 
     */
    virtual void setName(const std::string &name) override;

protected:
    virtual void handleClient(Socket::ptr client) override;

private:
    // 是否支持长链接
    bool m_isKeepalive;
    // Servlet分发器
    ServletDispatch::ptr m_dispatch;
};
}
} // namespace webs::http

#endif