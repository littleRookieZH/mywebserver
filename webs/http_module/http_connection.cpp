#include "http_connection.h"
#include "../log_module/log.h"
#include "http_parser.h"
#include "../stream_module/zlib_stream.h"

namespace webs {
namespace http {

static webs::Logger::ptr g_logger = WEBS_LOG_NAME("system");

std::string HttpResult::toString() const {
    std::stringstream ss;
    ss << "[HttpResult result = " << result
       << " error = " << error
       << " response = " << (response ? response->toString() : "nullptr")
       << "]";
    return ss.str();
}

HttpConnection::HttpConnection(Socket::ptr sock, bool owner) :
    SocketStream(sock, owner) {
}

HttpConnection::~HttpConnection() {
    WEBS_LOG_DEBUG(g_logger) << "HttpConnection::~HttpConnection";
}

/* 创建解析http响应的对象 -- 创建动态接受缓存区 -- 读取数据并解析 -- 是否存在分块解析 
-- 存在：读取数据并解析 ---- 将解析后剩余的数据加入body中 ---- 循环解析数据块，直到没有分块需要解析
-- 不存在：将解析后剩余的数据加入body中
-- 数据解析完毕之后，将body中的数据压缩
-- 将压缩后的body加入 response -- 返回response
 */
HttpResponse::ptr HttpConnection::recvResponse() {
    HttpResponseParser::ptr parser = std::make_shared<HttpResponseParser>();
    uint64_t buff_size = HttpResponseParser::GetHttpResponseBufferSize();
    std::shared_ptr<char> buffer(new char[buff_size], [](char *ptr) { delete[] ptr; });
    char *data = buffer.get();
    int offset = 0; // 剩余数据长度
    do {
        int len = read(data + offset, buff_size - offset);
        if (len <= 0) {
            close();
            return nullptr;
        }
        len += offset;
        data[len] = '\0'; // 存在断言，所以末尾加 \0
        size_t nparse = parser->execute(data, len, false);
        if (parser->hasError()) {
            close();
            break;
        }
        offset = len - nparse;
        if (offset == (int)buff_size) {
            close();
            return nullptr;
        }
        if (parser->isFinished()) { //可能表示当前解析结束 -- 还有下一次解析
            break;
        }
    } while (true);

    auto &client_parser = parser->getParser();
    // 还需要解析消息体
    std::string body;
    if (client_parser.chunked) {
        int len = offset; // 剩余数据长度
        do {
            bool begin = true;
            do {
                if (!begin || len == 0) { // 带解析数据长度为0 或 还未从开始解析数据
                    int rt = read(data + len, buff_size - len);
                    if (rt <= 0) {
                        close();
                        return nullptr;
                    }
                    len += rt;
                }
                data[len] = '\0';
                size_t nparse = parser->execute(data, len, true); // 解析数据
                if (parser->hasError()) {
                    close();
                    return nullptr;
                }
                len -= nparse;
                if (len == (int)buff_size) { // 不允许解析长度超过缓存区长度的数据
                    close();
                    return nullptr;
                }
                begin = false;
            } while (!parser->isFinished());
            // 获取当前消息体的长度 -- len是剩余数据--有一部分是消息体的内容
            int client_content_len = client_parser.content_len;
            WEBS_LOG_DEBUG(g_logger)
                << "content-length = " << client_parser.content_len;
            if (client_content_len + 2 <= len) {
                body.append(data, client_content_len);
                memmove(data, data + client_content_len + 2, len - client_content_len - 2);
                len -= (client_content_len + 2);
            } else {
                body.append(data, len);
                int left = client_content_len - len + 2;
                while (left > 0) {
                    int rt = read(data, left > (int)buff_size ? (int)buff_size : left);
                    if (rt <= 0) {
                        close();
                        return nullptr;
                    }
                    left -= rt;
                    body.append(data, rt);
                }
                body.resize(body.size() - 2);
                len = 0;
            }
        } while (!client_parser.chunks_done);
    } else { // 不需要分块解析
        int length = parser->getContentLength();
        if (length >= 0) { // 如果消息体长度不为0
            int len = 0;
            if (length >= offset) {
                memcpy(&body[0], data, offset);
                len = offset;

            } else {
                memcpy(&body[0], data, length);
                len = length;
            }
            length -= len;
            if (length > 0) {
                if (readFixSize(&body[len], length) <= 0) {
                    close();
                    return nullptr;
                }
            }
        }
    }

    // body解码
    if (!body.empty()) {
        std::string content_encoding = parser->getData()->getHeader("content-encoding");
        WEBS_LOG_DEBUG(g_logger) << "content_encoding = " << content_encoding << " body size = " << body.size();
        if (strcasecmp(content_encoding.c_str(), "gzip") == 0) {
            webs::ZlibStream::ptr zlib = ZlibStream::CreateGzip(false);
            zlib->write(body.c_str(), body.size());
            zlib->flush();
            zlib->getResult().swap(body);
        } else if (strcasecmp(content_encoding.c_str(), "deflate") == 0) {
            webs::ZlibStream::ptr zlib = ZlibStream::CreateDeflate(false);
            zlib->write(body.c_str(), body.size());
            zlib->flush();
            zlib->getResult().swap(body);
        }
        parser->getData()->setBody(body);
    }
    return parser->getData();
}

int HttpConnection::sendRequest(HttpRequest::ptr request) {
    std::string data = request->toString();
    return writeFixSize(data.c_str(), data.size());
}

/**
 *   HttpResult(int _result, HttpResponse::ptr _response, const std::string &_error) :
        result(_result), response(_response), error(_error) {
    }
 * 
 */

/* DoGet -- DoPost -- 全部转化为 DoRequest； std::string uri --> 转为 Uri */
HttpResult::ptr HttpConnection::DoGet(const std::string &url, uint64_t timeout_ms,
                                      const std::map<std::string, std::string> &headers,
                                      const std::string &body) {
    Uri::ptr uri_ptr = Uri::Create(url);
    if (!uri_ptr) { // 创建失败 -- 返回一个错误码为 INVALID_URL 的HttpResult
        return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL, nullptr, "invalid url: " + url);
    }
    return DoGet(uri_ptr, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnection::DoGet(Uri::ptr uri, uint64_t timeout_ms,
                                      const std::map<std::string, std::string> &headers,
                                      const std::string &body) {
    return DoRequest(HttpMethod::GET, uri, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnection::DoPost(const std::string &url, uint64_t timeout_ms,
                                       const std::map<std::string, std::string> &headers,
                                       const std::string &body) {
    Uri::ptr uri_ptr = Uri::Create(url);
    if (!uri_ptr) { // 创建失败 -- 返回一个错误码为 INVALID_URL 的HttpResult
        return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL, nullptr, "invalid url: " + url);
    }
    return DoPost(uri_ptr, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnection::DoPost(Uri::ptr uri, uint64_t timeout_ms,
                                       const std::map<std::string, std::string> &headers,
                                       const std::string &body) {
    return DoRequest(HttpMethod::POST, uri, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnection::DoRequest(HttpMethod method, const std::string &url, uint64_t timeout_ms,
                                          const std::map<std::string, std::string> &headers,
                                          const std::string &body) {
    Uri::ptr uri_ptr = Uri::Create(url);
    if (!uri_ptr) { // 创建失败 -- 返回一个错误码为 INVALID_URL 的HttpResult
        return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL, nullptr, "invalid url: " + url);
    }
    return DoRequest(method, uri_ptr, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnection::DoRequest(HttpMethod method, Uri::ptr uri, uint64_t timeout_ms,
                                          const std::map<std::string, std::string> &headers,
                                          const std::string &body) {
    webs::http::HttpRequest::ptr request = std::make_shared<HttpRequest>();
    request->setMethod(method);
    request->setPath(uri->getPath());
    request->setQuery(uri->getQuery());
    request->setFragment(uri->getFragment());
    bool hashost = false;
    for (auto &i : headers) {
        if (strcasecmp(i.first.c_str(), "connection") == 0) {
            if (strcasecmp(i.second.c_str(), "keep-alive") == 0) {
                request->setClose(false); // 默认是true
            }
            continue;
        }
        if (!hashost && strcasecmp(i.first.c_str(), "host") == 0) {
            hashost = !i.second.empty();
        }
        request->setHeader(i.first, i.second);
    }
    if (!hashost) { // 如果没有host，从uri获取
        request->setHeader("Host", uri->getHost());
    }
    request->setBody(body);
    return DoRequest(request, uri, timeout_ms);
}

/* 默认使用Tcp连接发送数据  创建地址对象 -- 创建socketfd -- 建立连接 -- 设置超时时间 -- 发送数据 -- 接收数据 */
HttpResult::ptr HttpConnection::DoRequest(HttpRequest::ptr request, Uri::ptr uri, uint64_t timeout_ms) {
    bool is_ssl = uri->getScheme() == "https";
    Address::ptr address = uri->createAddress(); // 默认是IP4
    if (!address) {
        return std::make_shared<HttpResult>((int)http::HttpResult::Error::INVALID_HOST, nullptr, "invalid host: " + uri->getHost());
    }

    Socket::ptr socket = is_ssl ? webs::SSLSocket::CreateTCP(address) : webs::Socket::CreateTCP(address);
    if (!socket) {
        return std::make_shared<HttpResult>((int)http::HttpResult::Error::CREATE_SOCKET_ERROR, nullptr,
                                            "create socket fail: " + address->toString() + " errno " + std::to_string(errno) + " errstr = " + strerror(errno));
    }

    if (!socket->connect(address)) {
        return std::make_shared<HttpResult>((int)http::HttpResult::Error::CONNECT_FAIL, nullptr, "connect fail: " + address->toString());
    }

    socket->setSendTimeout(timeout_ms);
    HttpConnection::ptr connection = std::make_shared<HttpConnection>(socket);
    int rt = connection->sendRequest(request);
    if (rt == 0) {
        return std::make_shared<HttpResult>((int)http::HttpResult::Error::SEND_CLOSE_BY_PEER, nullptr, "send request closed by peer: " + address->toString());
    }
    if (rt < 0) {
        return std::make_shared<HttpResult>((int)http::HttpResult::Error::SEND_SOCKET_ERROR, nullptr, "send request socket error errno = " + std::to_string(errno) + " errstr = " + strerror(errno));
    }
    HttpResponse::ptr response = connection->recvResponse();
    if (!response) {
        return std::make_shared<HttpResult>((int)http::HttpResult::Error::TIMEOUT, nullptr, "recv response timeout: " + address->toString() + " timeout_ms = " + std::to_string(timeout_ms));
    }
    return std::make_shared<HttpResult>((int)http::HttpResult::Error::OK, response, "ok");
}

HttpConnectionPool::HttpConnectionPool(const std::string &host, const std::string &vhost,
                                       uint32_t port, bool is_https, uint32_t maxSize, uint32_t max_alive_time,
                                       uint32_t max_request) :
    m_host(host),
    m_vhost(vhost),
    m_port(port ? port : (is_https ? 443 : 80)),
    m_maxSize(maxSize),
    m_maxAliveTime(max_alive_time),
    m_maxRequest(max_request),
    m_isHttp(is_https) {
}

/* 解析uri -- 创建对象 */
HttpConnectionPool::ptr HttpConnectionPool::Create(const std::string &uri, const std::string &vhost, uint32_t max_size, uint32_t max_alive_time, uint32_t max_request) {
    Uri::ptr uri_ptr = Uri::Create(uri);
    if (!uri_ptr) {
        WEBS_LOG_ERROR(g_logger) << "invalid uri = " << uri;
        return nullptr;
    }
    //     HttpConnectionPool(const std::string &host, const std::string &vhost,
    //    uint32_t port, bool is_https, uint32_t maxSize, uint32_t max_alive_time,
    //    uint32_t max_request);

    return std::make_shared<HttpConnectionPool>(uri_ptr->getHost(), vhost, uri_ptr->getPort(), uri_ptr->getScheme() == "https", max_size, max_alive_time, max_request);
}

/* 检查连接池中是否有可重用的连接 -- 找到一个处于连接状态的h_connection -- 释放已经关闭连接的指针 -- ptr如果不存在 -- 重新建立连接 -- 返回连接对象 */
HttpConnection::ptr HttpConnectionPool::getConnection() {
    std::vector<HttpConnection *> invalid_connect;
    uint64_t ms = webs::GetCurrentMS();
    Mutex::Lock lock(m_mutex);
    HttpConnection *connect_tmp = nullptr;
    while (!m_conns.empty()) {
        auto connect = *(m_conns.begin());
        m_conns.pop_front();
        if (!connect->isConnected()) {
            invalid_connect.push_back(connect);
            continue;
        }
        if (connect->m_createTime + m_maxAliveTime < ms) {
            invalid_connect.push_back(connect);
            continue;
        }
        connect_tmp = connect;
        break;
    }
    lock.unlock();
    m_total -= invalid_connect.size();
    if (!invalid_connect.empty()) {
        for (auto &i : invalid_connect) {
            delete i;
        }
    }
    if (!connect_tmp) {
        IPAddress::ptr addr = Address::LookupAnyIPAddress(m_host);
        if (!addr) {
            WEBS_LOG_DEBUG(g_logger) << "get addr fail: host = " << m_host;
            return nullptr;
        }
        addr->setPort(m_port);

        Socket::ptr socket = m_isHttp ? SSLSocket::CreateTCP(addr) : Socket::CreateTCP(addr);
        if (!socket) {
            WEBS_LOG_DEBUG(g_logger) << "create sock fail: " << *addr;
            return nullptr;
        }

        if (!socket->connect(addr)) {
            WEBS_LOG_DEBUG(g_logger) << "sock connect fail: " << *addr;
            return nullptr;
        }
        connect_tmp = new HttpConnection(socket);
        ++m_total;
    }
    // 绑定一个接受 HttpConnection * 参数的 动态对象释放函数
    // 这里不能使用make_shared：因为make_shared不支持自定义删除器
    // return std::make_shared<HttpConnection>(connect_tmp, std::bind(&HttpConnectionPool::ReleasePtr, std::placeholders::_1, this));
    return HttpConnection::ptr(connect_tmp, std::bind(&HttpConnectionPool::ReleasePtr, std::placeholders::_1, this));
}

void HttpConnectionPool::ReleasePtr(HttpConnection *ptr, HttpConnectionPool *pool) {
    ++ptr->m_request;
    if (!ptr->isConnected() || (ptr->m_createTime + pool->m_maxAliveTime) < webs::GetCurrentMS() || ptr->m_request > pool->m_maxRequest) {
        delete ptr;
        --pool->m_total;
        return;
    }
    Mutex::Lock lock(pool->m_mutex);
    pool->m_conns.push_back(ptr);
}

/* 以下方法全部转为sting */
HttpResult::ptr HttpConnectionPool::doGet(const std::string &uri, uint64_t timeout_ms,
                                          const std::map<std::string, std::string> &headers,
                                          const std::string &body) {
    return doRequest(HttpMethod::GET, uri, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnectionPool::doGet(Uri::ptr uri, uint64_t timeout_ms,
                                          const std::map<std::string, std::string> &headers,
                                          const std::string &body) {
    std::stringstream ss;
    ss << uri->getPath();
    ss << (uri->getQuery().empty() ? "" : "?") << uri->getQuery();
    ss << (uri->getFragment().empty() ? "" : "#") << uri->getFragment();
    return doRequest(HttpMethod::GET, ss.str(), timeout_ms, headers, body);
}

HttpResult::ptr HttpConnectionPool::doPost(const std::string &uri, uint64_t timeout_ms,
                                           const std::map<std::string, std::string> &headers,
                                           const std::string &body) {
    return doRequest(HttpMethod::POST, uri, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnectionPool::doPost(Uri::ptr uri, uint64_t timeout_ms,
                                           const std::map<std::string, std::string> &headers,
                                           const std::string &body) {
    std::stringstream ss;
    ss << uri->getPath();
    ss << (uri->getQuery().empty() ? "" : "?") << uri->getQuery();
    ss << (uri->getFragment().empty() ? "" : "#") << uri->getFragment();
    return doRequest(HttpMethod::POST, ss.str(), timeout_ms, headers, body);
}

HttpResult::ptr HttpConnectionPool::doRequest(HttpMethod method, const std::string &uri, uint64_t timeout_ms,
                                              const std::map<std::string, std::string> &headers,
                                              const std::string &body) {
    HttpRequest::ptr request = std::make_shared<HttpRequest>();
    request->setPath(uri);
    request->setMethod(method);
    request->setClose(false);
    bool has_host = false;
    for (auto &i : headers) {
        if (strcasecmp(i.first.c_str(), "connection") == 0) {
            if (strcasecmp(i.second.c_str(), "keep-alive") == 0) {
                request->setClose(false);
            }
            continue;
        }

        if (!has_host && strcasecmp(i.first.c_str(), "host") == 0) {
            has_host = !i.second.empty();
        }

        request->setHeader(i.first, i.second);
    }
    if (!has_host) {
        if (m_vhost.empty()) {
            request->setHeader("Host", m_host);
        } else {
            request->setHeader("Host", m_vhost);
        }
    }
    request->setBody(body);
    return doRequest(request, timeout_ms);
}

HttpResult::ptr HttpConnectionPool::doRequest(HttpMethod method, Uri::ptr uri, uint64_t timeout_ms,
                                              const std::map<std::string, std::string> &headers,
                                              const std::string &body) {
    std::stringstream ss;
    ss << uri->getPath();
    ss << (uri->getQuery().empty() ? "" : "?") << uri->getQuery();
    ss << (uri->getFragment().empty() ? "" : "#") << uri->getFragment();
    return doRequest(method, ss.str(), timeout_ms, headers, body);
}

HttpResult::ptr HttpConnectionPool::doRequest(HttpRequest::ptr request, uint64_t timeout_ms) {
    HttpConnection::ptr connect = getConnection();
    if (!connect) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::POOL_GET_CONNECTION, nullptr, "pool host:" + m_host + " port:" + std::to_string(m_port));
    }
    Socket::ptr socket = connect->getSocket();
    if (!socket) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::POOL_INVALID_CONNECTION, nullptr, "pool host:" + m_host + " port:" + std::to_string(m_port));
    }
    socket->setSendTimeout(timeout_ms);
    int rt = connect->sendRequest(request);
    if (rt == 0) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_CLOSE_BY_PEER, nullptr, "send request closed by peer: " + socket->getRemoteAddress()->toString());
    }
    if (rt < 0) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_SOCKET_ERROR, nullptr, "send request socket error errno=" + std::to_string(errno) + " errstr=" + std::string(strerror(errno)));
    }
    HttpResponse::ptr response = connect->recvResponse();
    if (!response) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::TIMEOUT, nullptr, "recv response timeout: " + socket->getRemoteAddress()->toString() + " timeout_ms:" + std::to_string(timeout_ms));
    }
    return std::make_shared<HttpResult>((int)HttpResult::Error::OK, response, "OK");
}
}
} // namespace webs::http