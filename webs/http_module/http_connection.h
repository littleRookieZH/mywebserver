/**
 * @file http_connection.h
 * @author your name (you@domain.com)
 * @brief HTTP客户端类
 * @version 0.1
 * @date 2024-05-23
 * 
 * 
 */
#ifndef __WEBS_HTTP_CONNECTION_H__
#define __WEBS_HTTP_CONNECTION_H__

#include <memory>
#include "http.h"
#include "../stream_module/socket_stream.h"
#include "../net_module/uri.h"
#include "../util_module/mutex.h"
#include <list>

namespace webs {
namespace http {

/**
 * @brief Http响应结果
 * 
 */
struct HttpResult {
    typedef std::shared_ptr<HttpResult> ptr;
    enum class Error {
        // 正常
        OK = 0,
        // 非法URL
        INVALID_URL = 1,
        // 无法解析HOST
        INVALID_HOST = 2,
        // 连接失败
        CONNECT_FAIL = 3,
        // 连接对端被关闭
        SEND_CLOSE_BY_PEER = 4,
        // 发送请求发生Socket错误
        SEND_SOCKET_ERROR = 5,
        // 超时连接
        TIMEOUT = 6,
        // 创建Socket错误
        CREATE_SOCKET_ERROR = 7,
        // 从连接池获取连接失败
        POOL_GET_CONNECTION = 8,
        // 无效的连接
        POOL_INVALID_CONNECTION = 9
    };

    /**
     * @brief Construct a new Http Result object
     * 
     * @param _result 错误码
     * @param _response HTTP响应结构体
     * @param _error 错误描述
     */
    HttpResult(int _result, HttpResponse::ptr _response, const std::string &_error) :
        result(_result), response(_response), error(_error) {
    }

    std::string toString() const;

    // 错误码
    int result;
    // Http响应
    HttpResponse::ptr response;
    // 错误描述
    std::string error;
};

class HttpConnection : public SocketStream {
    friend class HttpConnectionPool;

public:
    typedef std::shared_ptr<HttpConnection> ptr;

    /**
     * @brief Construct a new Http Connection object
     * 
     * @param sock Socket类
     * @param owner 是否掌握所有权
     */
    HttpConnection(Socket::ptr sock, bool owner = true);

    ~HttpConnection();

    /**
     * @brief 发送HTTP的GET请求
     * 
     * @param uri 请求的url
     * @param timeout_ms 超时时间(毫秒)
     * @param headers HTTP请求头部参数
     * @param body 请求消息体
     * @return HttpResult::ptr 
     */
    static HttpResult::ptr DoGet(const std::string &uri, uint64_t timeout_ms,
                                 const std::map<std::string, std::string> &headers = {},
                                 const std::string &body = "");

    /**
     * @brief 发送HTTP的GET请求
     * 
     * @param uri URI结构体
     * @param timeout_ms 超时时间(毫秒)
     * @param headers HTTP请求头部参数
     * @param body 请求消息体
     * @return HttpResult::ptr 
     */
    static HttpResult::ptr DoGet(Uri::ptr uri, uint64_t timeout_ms,
                                 const std::map<std::string, std::string> &headers = {},
                                 const std::string &body = "");

    /**
     * @brief 发送HTTP的POST请求
     * 
     * @param uri 请求的url
     * @param timeout_ms 超时时间(毫秒)
     * @param headers HTTP请求头部参数
     * @param body 请求消息体
     * @return HttpResult::ptr 返回HTTP结果结构体
     */
    static HttpResult::ptr DoPost(const std::string &uri, uint64_t timeout_ms,
                                  const std::map<std::string, std::string> &headers = {},
                                  const std::string &body = "");

    /**
     * @brief 发送HTTP的POST请求
     * 
     * @param uri URI结构体
     * @param timeout_ms 超时时间(毫秒)
     * @param headers HTTP请求头部参数
     * @param body 请求消息体
     * @return HttpResult::ptr 返回HTTP结果结构体
     */
    static HttpResult::ptr DoPost(Uri::ptr uri, uint64_t timeout_ms,
                                  const std::map<std::string, std::string> &headers = {},
                                  const std::string &body = "");

    /**
     * @brief 发送HTTP请求
     * 
     * @param method 请求类型
     * @param uri 请求的url
     * @param timeout_ms 超时时间(毫秒)
     * @param headers HTTP请求头部参数
     * @param body 请求消息体
     * @return HttpResult::ptr 返回HTTP结果结构体
     */
    static HttpResult::ptr DoRequest(HttpMethod method, const std::string &uri, uint64_t timeout_ms,
                                     const std::map<std::string, std::string> &headers = {},
                                     const std::string &body = "");

    /**
     * @brief 发送HTTP请求
     * 
     * @param method 请求类型
     * @param uri URI结构体
     * @param timeout_ms 超时时间(毫秒)
     * @param headers HTTP请求头部参数
     * @param body 请求消息体
     * @return HttpResult::ptr 返回HTTP结果结构体
     */
    static HttpResult::ptr DoRequest(HttpMethod method, Uri::ptr uri, uint64_t timeout_ms,
                                     const std::map<std::string, std::string> &headers = {},
                                     const std::string &body = "");

    /**
     * @brief 发送HTTP请求
     * 
     * @param request 请求结构体
     * @param uri URI结构体
     * @param timeout_ms 超时时间(毫秒)
     * @return HttpResult::ptr 返回HTTP结果结构体
     */
    static HttpResult::ptr DoRequest(HttpRequest::ptr request, Uri::ptr uri, uint64_t timeout_ms);

    /**
     * @brief 接收HTTP响应
     * 
     * @return HttpResponse::ptr 
     */
    HttpResponse::ptr recvResponse();

    /**
     * @brief 发送HTTP请求
     * 
     * @param request HTTP请求结构
     * @return int 
     */
    int sendRequest(HttpRequest::ptr request);

private:
    uint64_t m_createTime = 0;
    uint64_t m_request = 0;
};

/**
 * @brief 用于维护长连接
 * 当连接数量超过 连接池中最大连接数量时 还是会创建连接，但是执行完再放回连接池，如果连接池满了该连接就会释放掉
 */
class HttpConnectionPool {
public:
    typedef std::shared_ptr<HttpConnectionPool> ptr;

    typedef Mutex MutexType;

    HttpConnectionPool(const std::string &host, const std::string &vhost,
                       uint32_t port, bool is_https, uint32_t maxSize, uint32_t max_alive_time,
                       uint32_t max_request);

    static HttpConnectionPool::ptr Create(const std::string &uri, const std::string &vhost, uint32_t max_size, uint32_t max_alive_time, uint32_t max_request);

    /**
     * @brief 获取连接
     * 
     * @return HttpConnection::ptr 
     */
    HttpConnection::ptr getConnection();

    /**
     * @brief 发送HTTP的GET请求
     * 
     * @param uri 请求的url
     * @param timeout_ms 超时时间(毫秒)
     * @param headers HTTP请求头部参数
     * @param body 请求消息体
     * @return HttpResult::ptr 
     */
    HttpResult::ptr doGet(const std::string &uri, uint64_t timeout_ms,
                          const std::map<std::string, std::string> &headers = {},
                          const std::string &body = "");

    /**
     * @brief 发送HTTP的GET请求
     * 
     * @param uri URI结构体
     * @param timeout_ms 超时时间(毫秒)
     * @param headers HTTP请求头部参数
     * @param body 请求消息体
     * @return HttpResult::ptr 
     */
    HttpResult::ptr doGet(Uri::ptr uri, uint64_t timeout_ms,
                          const std::map<std::string, std::string> &headers = {},
                          const std::string &body = "");

    /**
     * @brief 发送HTTP的POST请求
     * 
     * @param uri 请求的url
     * @param timeout_ms 超时时间(毫秒)
     * @param headers HTTP请求头部参数
     * @param body 请求消息体
     * @return HttpResult::ptr 返回HTTP结果结构体
     */
    HttpResult::ptr doPost(const std::string &uri, uint64_t timeout_ms,
                           const std::map<std::string, std::string> &headers = {},
                           const std::string &body = "");

    /**
     * @brief 发送HTTP的POST请求
     * 
     * @param uri URI结构体
     * @param timeout_ms 超时时间(毫秒)
     * @param headers HTTP请求头部参数
     * @param body 请求消息体
     * @return HttpResult::ptr 返回HTTP结果结构体
     */
    HttpResult::ptr doPost(Uri::ptr uri, uint64_t timeout_ms,
                           const std::map<std::string, std::string> &headers = {},
                           const std::string &body = "");

    /**
     * @brief 发送HTTP请求
     * 
     * @param method 请求类型
     * @param uri 请求的url
     * @param timeout_ms 超时时间(毫秒)
     * @param headers HTTP请求头部参数
     * @param body 请求消息体
     * @return HttpResult::ptr 返回HTTP结果结构体
     */
    HttpResult::ptr doRequest(HttpMethod method, const std::string &uri, uint64_t timeout_ms,
                              const std::map<std::string, std::string> &headers = {},
                              const std::string &body = "");

    /**
     * @brief 发送HTTP请求
     * 
     * @param method 请求类型
     * @param uri URI结构体
     * @param timeout_ms 超时时间(毫秒)
     * @param headers HTTP请求头部参数
     * @param body 请求消息体
     * @return HttpResult::ptr 返回HTTP结果结构体
     */
    HttpResult::ptr doRequest(HttpMethod method, Uri::ptr uri, uint64_t timeout_ms,
                              const std::map<std::string, std::string> &headers = {},
                              const std::string &body = "");

    /**
     * @brief 发送HTTP请求
     * 
     * @param request 请求结构体
     * @param timeout_ms 超时时间(毫秒)
     * @return HttpResult::ptr 返回HTTP结果结构体
     */
    HttpResult::ptr doRequest(HttpRequest::ptr request, uint64_t timeout_ms);

private:
    /**
     * @brief 释放连接
     * 
     * @param ptr 
     * @param pool 
     */
    static void ReleasePtr(HttpConnection *ptr, HttpConnectionPool *pool);

private:
    // 服务器主机名
    std::string m_host;
    // 虚拟主机名
    std::string m_vhost;
    // 服务器端口
    uint32_t m_port;
    // 连接池中最大连接数
    uint32_t m_maxSize;
    // 连接最大存在时间
    uint32_t m_maxAliveTime;
    // 最大请求次数
    uint32_t m_maxRequest;
    // 是否使用Http协议
    bool m_isHttp;
    //  互斥锁
    MutexType m_mutex;
    // 存放可用连接的列表 -- 为什么是指针而不是智能指针对象
    std::list<HttpConnection *> m_conns;
    // 当前连接的总数
    std::atomic<uint32_t> m_total = {0};
};
}
} // namespace webs::http

#endif
