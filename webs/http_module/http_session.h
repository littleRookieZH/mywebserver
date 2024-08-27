#ifndef __WEBS_HTTP_SESSION_H__
#define __WEBS_HTTP_SESSION_H__

#include "http.h"
#include "../stream_module/socket_stream.h"

namespace webs {
namespace http {
class HttpSession : public SocketStream {
public:
    // 智能指针类型的定义
    typedef std::shared_ptr<HttpSession> ptr;

    /**
     * @brief 构造函数
     * 
     * @param sock Socket类型
     * @param owner 是否托管
     */
    HttpSession(Socket::ptr sock, bool owner = true);

    /**
     * @brief 接收HTTP请求
     * 从m_socket中读取数据
     * @return HttpSession::ptr 
     */
    HttpRequest::ptr recvRequest();

    /**
     * @brief 发送HTTP响应
     * 
     * @param rsp HTTP响应
     * @return int 
     *      @retval >0 发送成功
     *      @retval =0 对方关闭
     *      @retval <0 Socket异常
     */
    int sendResponse(HttpResponse::ptr rsp);
};

}
} // namespace webs::http

#endif