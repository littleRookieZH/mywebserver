#include "http_server.h"
#include "../log_module/log.h"

namespace webs {
namespace http {

static Logger::ptr g_logger = WEBS_LOG_NAME("system");

HttpServer::HttpServer(bool keepalive, webs::IOManager *worker,
                       webs::IOManager *io_worker, webs::IOManager *accept_worker) :
    TcpServer(worker, io_worker, accept_worker),
    m_isKeepalive(keepalive) {
    m_type = "http";
    m_dispatch.reset(new ServletDispatch);
    // m_dispatch->addServlet("/_/status", std::make_shared<StatusServlet>());
    // m_dispatch->addServlet("/_/config", std::make_shared<ConfigServlet>());
}

void HttpServer::setName(const std::string &name) {
    TcpServer::setName(name);
    m_dispatch->setDefault(std::make_shared<NotFoundServlet>(name));
}

/* 根据clientfd创建 HttpSession -- session接受请求消息，返回httprequest -- 创建 httpresponse -- 执行handle，处理消息 -- session设置httpresponse(会将消息写入socket) */
/* 获取请求消息 -- 处理请求 -- 将处理结果返回 */
void HttpServer::handleClient(Socket::ptr client) {
    WEBS_LOG_DEBUG(g_logger) << "handleClient = " << *client;
    HttpSession::ptr session = std::make_shared<HttpSession>(client);
    do {
        HttpRequest::ptr request = session->recvRequest();
        if (!request) {
            WEBS_LOG_DEBUG(g_logger) << "recv http request fail, errno = "
                                     << errno << "errstr = " << strerror(errno) << " client = " << *client << " keep_alive = " << m_isKeepalive;
            break;
        }

        HttpResponse::ptr response = std::make_shared<HttpResponse>(request->getVersion(), request->isClose() || !m_isKeepalive);
        response->setHeader("Server", getName()); // 服务器名称
        m_dispatch->handle(request, response, session);
        session->sendResponse(response); // 没有在handle中sendResponse，因为可能需要经过多种处理才能发送
        if (!m_isKeepalive || request->isClose()) {
            break;
        }
    } while (true);
    session->close();
}
}
} // namespace webs::http