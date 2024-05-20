/**
 * @file http.h
 * @author your name (you@domain.com)
 * @brief HTTP定义结构体封装
 * @version 0.1
 * @date 2024-05-18
 * 
 * 
 */
#ifndef __WEBS_HTTP_H__
#define __WEBS_HTTP_H__

#include <string>
#include <boost/lexical_cast.hpp>
#include <memory>
#include <map>

namespace webs {
namespace http {

// 定义一个HTTP方法的映射表
/* Request Methods */
#define HTTP_METHOD_MAP(XX)          \
    XX(0, DELETE, DELETE)            \
    XX(1, GET, GET)                  \
    XX(2, HEAD, HEAD)                \
    XX(3, POST, POST)                \
    XX(4, PUT, PUT)                  \
    /* pathological */               \
    XX(5, CONNECT, CONNECT)          \
    XX(6, OPTIONS, OPTIONS)          \
    XX(7, TRACE, TRACE)              \
    /* WebDAV */                     \
    XX(8, COPY, COPY)                \
    XX(9, LOCK, LOCK)                \
    XX(10, MKCOL, MKCOL)             \
    XX(11, MOVE, MOVE)               \
    XX(12, PROPFIND, PROPFIND)       \
    XX(13, PROPPATCH, PROPPATCH)     \
    XX(14, SEARCH, SEARCH)           \
    XX(15, UNLOCK, UNLOCK)           \
    XX(16, BIND, BIND)               \
    XX(17, REBIND, REBIND)           \
    XX(18, UNBIND, UNBIND)           \
    XX(19, ACL, ACL)                 \
    /* subversion */                 \
    XX(20, REPORT, REPORT)           \
    XX(21, MKACTIVITY, MKACTIVITY)   \
    XX(22, CHECKOUT, CHECKOUT)       \
    XX(23, MERGE, MERGE)             \
    /* upnp */                       \
    XX(24, MSEARCH, M - SEARCH)      \
    XX(25, NOTIFY, NOTIFY)           \
    XX(26, SUBSCRIBE, SUBSCRIBE)     \
    XX(27, UNSUBSCRIBE, UNSUBSCRIBE) \
    /* RFC-5789 */                   \
    XX(28, PATCH, PATCH)             \
    XX(29, PURGE, PURGE)             \
    /* CalDAV */                     \
    XX(30, MKCALENDAR, MKCALENDAR)   \
    /* RFC-2068, section 19.6.1.2 */ \
    XX(31, LINK, LINK)               \
    XX(32, UNLINK, UNLINK)           \
    /* icecast */                    \
    XX(33, SOURCE, SOURCE)

/* Status Codes */
#define HTTP_STATUS_MAP(XX)                                                   \
    XX(100, CONTINUE, Continue)                                               \
    XX(101, SWITCHING_PROTOCOLS, Switching Protocols)                         \
    XX(102, PROCESSING, Processing)                                           \
    XX(200, OK, OK)                                                           \
    XX(201, CREATED, Created)                                                 \
    XX(202, ACCEPTED, Accepted)                                               \
    XX(203, NON_AUTHORITATIVE_INFORMATION, Non - Authoritative Information)   \
    XX(204, NO_CONTENT, No Content)                                           \
    XX(205, RESET_CONTENT, Reset Content)                                     \
    XX(206, PARTIAL_CONTENT, Partial Content)                                 \
    XX(207, MULTI_STATUS, Multi - Status)                                     \
    XX(208, ALREADY_REPORTED, Already Reported)                               \
    XX(226, IM_USED, IM Used)                                                 \
    XX(300, MULTIPLE_CHOICES, Multiple Choices)                               \
    XX(301, MOVED_PERMANENTLY, Moved Permanently)                             \
    XX(302, FOUND, Found)                                                     \
    XX(303, SEE_OTHER, See Other)                                             \
    XX(304, NOT_MODIFIED, Not Modified)                                       \
    XX(305, USE_PROXY, Use Proxy)                                             \
    XX(307, TEMPORARY_REDIRECT, Temporary Redirect)                           \
    XX(308, PERMANENT_REDIRECT, Permanent Redirect)                           \
    XX(400, BAD_REQUEST, Bad Request)                                         \
    XX(401, UNAUTHORIZED, Unauthorized)                                       \
    XX(402, PAYMENT_REQUIRED, Payment Required)                               \
    XX(403, FORBIDDEN, Forbidden)                                             \
    XX(404, NOT_FOUND, Not Found)                                             \
    XX(405, METHOD_NOT_ALLOWED, Method Not Allowed)                           \
    XX(406, NOT_ACCEPTABLE, Not Acceptable)                                   \
    XX(407, PROXY_AUTHENTICATION_REQUIRED, Proxy Authentication Required)     \
    XX(408, REQUEST_TIMEOUT, Request Timeout)                                 \
    XX(409, CONFLICT, Conflict)                                               \
    XX(410, GONE, Gone)                                                       \
    XX(411, LENGTH_REQUIRED, Length Required)                                 \
    XX(412, PRECONDITION_FAILED, Precondition Failed)                         \
    XX(413, PAYLOAD_TOO_LARGE, Payload Too Large)                             \
    XX(414, URI_TOO_LONG, URI Too Long)                                       \
    XX(415, UNSUPPORTED_MEDIA_TYPE, Unsupported Media Type)                   \
    XX(416, RANGE_NOT_SATISFIABLE, Range Not Satisfiable)                     \
    XX(417, EXPECTATION_FAILED, Expectation Failed)                           \
    XX(421, MISDIRECTED_REQUEST, Misdirected Request)                         \
    XX(422, UNPROCESSABLE_ENTITY, Unprocessable Entity)                       \
    XX(423, LOCKED, Locked)                                                   \
    XX(424, FAILED_DEPENDENCY, Failed Dependency)                             \
    XX(426, UPGRADE_REQUIRED, Upgrade Required)                               \
    XX(428, PRECONDITION_REQUIRED, Precondition Required)                     \
    XX(429, TOO_MANY_REQUESTS, Too Many Requests)                             \
    XX(431, REQUEST_HEADER_FIELDS_TOO_LARGE, Request Header Fields Too Large) \
    XX(451, UNAVAILABLE_FOR_LEGAL_REASONS, Unavailable For Legal Reasons)     \
    XX(500, INTERNAL_SERVER_ERROR, Internal Server Error)                     \
    XX(501, NOT_IMPLEMENTED, Not Implemented)                                 \
    XX(502, BAD_GATEWAY, Bad Gateway)                                         \
    XX(503, SERVICE_UNAVAILABLE, Service Unavailable)                         \
    XX(504, GATEWAY_TIMEOUT, Gateway Timeout)                                 \
    XX(505, HTTP_VERSION_NOT_SUPPORTED, HTTP Version Not Supported)           \
    XX(506, VARIANT_ALSO_NEGOTIATES, Variant Also Negotiates)                 \
    XX(507, INSUFFICIENT_STORAGE, Insufficient Storage)                       \
    XX(508, LOOP_DETECTED, Loop Detected)                                     \
    XX(510, NOT_EXTENDED, Not Extended)                                       \
    XX(511, NETWORK_AUTHENTICATION_REQUIRED, Network Authentication Required)

/**
 * @brief HTTP方法枚举，使用枚举类 -- 避免枚举名字冲突
 * 
 */
enum class HttpMethod {
#define XX(num, name, string) name = num,
    HTTP_METHOD_MAP(XX)
#undef XX
        INVALID_METHOD
};

/**
 * @brief HTPP状态枚举
 * desc:描述
 */
enum class HttpStatus {
#define XX(code, name, desc) name = code,
    HTTP_STATUS_MAP(XX)
#undef XX
};

/**
 * @brief 将字符串对象转为 HTTP方法名
 * 
 * @param m 
 * @return HttpMethod 
 */
HttpMethod StringToHttpMethod(const std::string &m);

/**
 * @brief 将字符数组转为 Http方法名
 * 
 * @param m 
 * @return HttpMethod 
 */
HttpMethod CharsToHttpMethod(const char *m);

/**
 * @brief 将Http方法枚举转换为字符数组
 * 
 * @param m 
 * @return const char* 
 */
const char *HttpMethodToString(const HttpMethod &m);

/**
 * @brief 将HTTP状态转为字符数组
 * 
 * @param s 
 * @return const char* 
 */
const char *HttpStatusToString(const HttpStatus &s);

/**
 * @brief 忽略大小写比较仿函数
 * 
 */
struct CaseInsensitiveLess {
    bool operator()(const std::string &lhs, const std::string &rhs) const;
};

/**
 * @brief 获取Map中的key值，并转为对应的类型，返回是否成功
 * 
 * @tparam MapType 
 * @tparam T 
 * @param m map类型数据
 * @param key 关键字
 * @param val 保存转换后的值
 * @param def T类型的默认值
 * @return true 转换成功, val 为对应的值
 * @return false 不存在或者转换失败 val = def
 */
template <class MapType, class T>
bool checkGetAs(const MapType &m, const std::string &key, T &val, const T &def = T()) {
    auto it = m.find(key);
    if (it == m.end()) {
        val = def;
        return false;
    } else {
        try {
            val = boost::lexical_cast<T>(it->second);
            return true;
        } catch (...) {
            val = def;
        }
    }
    return false;
}

/**
 * @brief 获取map中键值为key的值。并将其转为T类型
 * 如果成功返回对应的值，如果失败返回默认值
 * @tparam MapType 
 * @tparam T 
 * @param m map类型的对象
 * @param key 键值
 * @param def 默认值
 * @return T 
 */
template <class MapType, class T>
T getAs(const MapType &m, const std::string &key, const T def = T()) {
    auto it = m.find(key);
    if (it == m.end()) {
        return def;
    }
    try {
        return boost::lexical_cast<T>(it->second);
    } catch (...) {
    }
    return def;
}

class HttpResponse;

class HttpRequest {
public:
    // HTTP请求的智能指针
    typedef std::shared_ptr<HttpRequest> ptr;
    // Map结构
    typedef std::map<std::string, std::string, CaseInsensitiveLess> MapType;

    /**
     * @brief Construct a new Http Request object
     * 
     * @param version 版本
     * @param close 是否 keepalive
     */
    HttpRequest(uint8_t version = 0x11, bool close = true);

    /**
     * @brief Create a Response object
     * 
     * @return std::shared_ptr<HttpResponse> 
     */
    std::shared_ptr<HttpResponse> createResponse();

    /**
     * @brief 返回HTTP方法
     * 
     * @return HttpMethod 
     */
    HttpMethod getMethod() const {
        return m_method;
    }

    uint8_t getVersion() const {
        return m_version;
    }

    /**
     * @brief 返回HTTP请求的路径
     * 
     * @return std::string 
     */
    const std::string &getPath() const {
        return m_path;
    }

    /**
     * @brief 返回HTTP请求的查询参数
     * 
     * @return std::string 
     */
    const std::string &getQuery() const {
        return m_query;
    }

    /**
     * @brief 返回HTTP请求的消息体
     * 
     * @return const std::string 
     */
    const std::string &getBody() const {
        return m_body;
    }

    /**
     * @brief 返回HTTP请求的消息头MAP
     * 返回 MapType类型
     * @return const MapType 
     */
    const MapType &getHeaders() const {
        return m_headers;
    }

    /**
     * @brief 返回HTTP请求的参数MAP
     * 
     * @return const MapType 
     */
    const MapType &getParams() const {
        return m_params;
    }

    /**
     * @brief 返回HTTP请求的Cookie的MAP
     * 
     * @return const MapType 
     */
    const MapType &getCookie() const {
        return m_cookies;
    }

    /**
     * @brief 是否自动关闭
     * 
     * @return true 
     * @return false 
     */
    bool isClose() const {
        return m_close;
    }

    /**
     * @brief 是否websocket
     * 
     * @return true 
     * @return false 
     */
    bool isWebsocket() const {
        return m_websocket;
    }

    /**
     * @brief 设置HTTP方法
     * 
     */
    void setMethod(HttpMethod method) {
        m_method = method;
    }

    void setVersin(uint8_t version) {
        m_version = version;
    }

    /**
     * @brief 设置HTTP请求的路径
     * 
     */
    void setPath(const std::string &path) {
        m_path = path;
    }

    /**
     * @brief 设置HTTP请求的查询参数
     * 
     */
    void setQuery(const std::string &query) {
        m_query = query;
    }

    /**
     * @brief 设置HTTP请求的消息体
     * 
     */
    void setBody(const std::string &body) {
        m_body = body;
    }

    /**
     * @brief 设置HTTP请求的消息头MAP
     * 
     */
    void setHeaders(const MapType &headers) {
        m_headers = headers;
    }

    /**
     * @brief 设置HTTP请求的参数MAP
     * 
     */
    void setParams(const MapType &params) {
        m_params = params;
    }

    /**
     * @brief 设置HTTP请求的Cookie的MAP
     * 
     */
    void setCookie(const MapType &cookies) {
        m_cookies = cookies;
    }

    /**
     * @brief 设置自动关闭
     * 
     */
    void setClose(bool close) {
        m_close = close;
    }

    /**
     * @brief 设置websocket
     * 
     */
    void setWebsocket(bool websocket) {
        m_websocket = websocket;
    }

    /**
     * @brief 设置fragment
     * 
     * @param fragment 
     */
    void setFragment(const std::string &fragment) {
        m_fragment = fragment;
    }

    /**
     * @brief 获取HTTP请求的头部参数
     * 
     * @param key 关键字
     * @param def 默认值
     * @return 如果存在则返回对应值,否则返回默认值
     */
    std::string getParam(const std::string &key, const std::string &def = "");

    /**
     * @brief 获取HTTP请求的请求参数
     * 返回 it->second 
     * @param key 关键字
     * @param def 默认值
     * @return 如果存在则返回对应值,否则返回默认值
     */
    std::string getHeader(const std::string &key, const std::string &def = "") const;

    /**
     * @brief 获取HTTP请求的Cookie参数
     * 
     * @param key 关键字
     * @param def 默认值
     * @return 如果存在则返回对应值,否则返回默认值
     */
    std::string getCookie(const std::string &key, const std::string &def = "");

    /**
     * @brief 设置HTTP请求的请求参数
     * 
     * @param key 关键字
     * @param val 默认值
     * @return std::string 
     */
    void setParam(const std::string &key, const std::string &val);

    /**
     * @brief 设置HTTP请求的头部参数
     * 
     * @param key 关键字
     * @param val 默认值
     * @return std::string 
     */
    void setHeader(const std::string &key, const std::string &val);

    /**
     * @brief 设置HTTP请求的Cookie参数
     * 
     * @param key 关键字
     * @param val 默认值
     */
    void setCookie(const std::string &key, const std::string &val);

    /**
     * @brief 删除HTTP请求的请求参数
     * 
     * @param key 
     */
    void delParam(const std::string &key);

    /**
     * @brief 删除HTTP请求的头部参数
     * 
     * @param key 
     */
    void delHeader(const std::string &key);

    /**
     * @brief 删除HTTP请求的Cookie参数
     * 
     * @param key 
     */
    void delCookie(const std::string &key);

    /**
     * @brief 判断HTTP请求的请求参数是否存在
     * 
     * @param key 
     * @param val 如果存在,val非空则赋值
     * @return true 
     * @return false 
     */
    bool hasParam(const std::string &key, std::string *val);

    /**
     * @brief 判断HTTP请求的头部参数是否存在
     * 
     * @param key 
     * @param val 
     * @return true 
     * @return false 
     */
    bool hasHeader(const std::string &key, std::string *val);

    /**
     * @brief 判断HTTP请求的Cookie参数是否存在
     * 
     * @param key 
     * @param val 
     * @return true 
     * @return false 
     */
    bool hasCookie(const std::string &key, std::string *val);

    /**
     * @brief 检查并返回key对应的值
     * 将it->second 转为 T类型值
     * @tparam T 
     * @param key 
     * @param val 
     * @param def 
     * @return true 
     * @return false 
     */
    template <class T>
    bool checkGetHeaderAs(const std::string &key, T &val, const T &def = T()) {
        return checkGetAs(m_headers, key, val, def);
    }

    /**
     * @brief 返回key对应的值
     * 将it->second 转为 T类型值
     * @tparam T 
     * @param key 
     * @param val 
     * @param def 
     * @return true 
     * @return false 
     */
    template <class T>
    T getHeaderAs(const std::string &key, const T &def = T()) {
        return getAs(m_headers, key, def);
    }

    /**
     * @brief 检查并返回key对应的值
     * val T类型
     * 为什么需要初始化？
     * @tparam T 
     * @param key 
     * @param val 传出参数
     * @param def 默认参数
     * @return true 
     * @return false 
     */
    template <class T>
    bool checkGetParamAs(const std::string &key, T &val, const T &def = T()) {
        initQueryParam();
        initBodyParam();
        return checkGetAs(m_params, key, val, def);
    }

    /**
     * @brief Get the Param As object
     * 
     * @tparam T 
     * @param key 
     * @param def 
     * @return T 
     */
    template <class T>
    T getParamAs(const std::string &key, const T &def = T()) {
        initQueryParam();
        initBodyParam();
        return getAs(m_params, key, def);
    }

    /**
     * @brief  检查并返回key对应的值
     * 
     * @tparam T 
     * @param key 
     * @param val 
     * @param def 
     * @return true 
     * @return false 
     */
    template <class T>
    bool checkGetCookie(const std::string &key, T val, const T &def = T()) {
        initCookies();
        return checkGetAs(m_cookies, key, val, def);
    }

    /**
     * @brief Get the Cookie As object
     * 
     * @tparam T 
     * @param key 
     * @param def 
     * @return T 
     */
    template <class T>
    T getCookieAs(const std::string &key, const T &def = T()) {
        initCookies();
        return getAs(m_cookies, key, def);
    }

    std::ostream &dump(std::ostream &os) const;

    std::string toString() const;

    void init();
    void initParam();
    void initQueryParam();
    void initBodyParam();
    void initCookies();

private:
    // HTTP方法
    HttpMethod m_method;
    // HTTP版本
    uint8_t m_version;
    // 是否关闭
    bool m_close;
    // 是否是websocket
    bool m_websocket;

    uint8_t m_parserParamFlag;
    // 请求路径 url
    std::string m_path;
    // 请求参数
    std::string m_query;
    // 请求fragment
    std::string m_fragment;
    // 请求消息体
    std::string m_body;
    // 请求头部map
    MapType m_headers;
    // 请求参数map
    MapType m_params;
    // 请求Cookie map
    MapType m_cookies;
};

class HttpResponse {
public:
    typedef std::shared_ptr<HttpResponse> ptr;
    typedef std::map<std::string, std::string, CaseInsensitiveLess> MapType;

    HttpResponse(uint8_t version = 0x11, bool close = true);
    /**
     * @brief 返回响应状态
     * 
     * @return HttpStatus 
     */
    HttpStatus getStatus() const {
        return m_status;
    }

    /**
     * @brief 返回响应版本
     * 
     * @return uint8_t 
     */
    uint8_t getVersion() const {
        return m_version;
    }

    /**
     * @brief 是否自动关闭
     * 
     * @return true 
     * @return false 
     */
    bool isClose() const {
        return m_close;
    }

    /**
     * @brief 是否websocket
     * 
     * @return true 
     * @return false 
     */
    bool isWebsocket() const {
        return m_websocket;
    }

    /**
     * @brief 返回响应消息体
     * 
     * @return std::string 
     */
    const std::string getBody() const {
        return m_body;
    }

    /**
     * @brief 返回响应原因
     * 
     * @return std::string 
     */
    const std::string &getReason() const {
        return m_reason;
    }

    /**
     * @brief 返回响应头部MAP
     * 
     * @return MapType 
     */
    const MapType &getHeaders() {
        return m_headers;
    }

    void setStatus(HttpStatus status) {
        m_status = status;
    }

    void setVersion(uint8_t version) {
        m_version = version;
    }

    void setClose(bool close) {
        m_close = close;
    }

    void setWebsocket(bool websocket) {
        m_websocket = websocket;
    }

    void setBody(const std::string &body) {
        m_body = body;
    }

    void setReason(const std::string &reason) {
        m_reason = reason;
    }

    void setHeaders(const MapType &header) {
        m_headers = header;
    }

    /**
     * @brief 获取响应头部参数
     * 返回的是 it->second  (string类型)
     * @param key 关键字
     * @param def 值
     * @return std::string 
     */
    std::string getHeader(const std::string &key, const std::string &def = "") const;

    /**
     * @brief 设置响应头部参数
     * 
     * @param key 关键字
     * @param val 值
     */
    void setHeader(const std::string &key, const std::string &val);

    /**
     * @brief  删除响应头部参数
     * 
     * @param key 
     */
    void delHeader(const std::string &key);

    /**
     * @brief 检查并获取响应头部参数
     * 
     * @tparam T 值类型
     * @param key 关键字
     * @param val 传出参数值
     * @param def 默认值
     * @return true 如果存在且转换成功返回true
     * @return false 失败，则val=def
     */
    template <class T>
    bool checkGetHeaderAs(const std::string &key, T val, const T &def = T()) {
        return checkGetAs(m_headers, key, val, def);
    }

    /**
     * @brief 获取响应的头部参数
     * 
     * @tparam T 转换类型
     * @param key 关键字
     * @param def 默认值
     * @return T 如果存在且转换成功返回对应的值,否则返回def
     */
    template <class T>
    T getHeaderAs(const std::string &key, const T &def = T()) {
        return getAs(m_headers, key, def);
    }

    std::ostream &dump(std::ostream &os) const;

    std::string toString() const;

    void setRedirect(const std::string &url);

    void setCookie(const std::string &key, const std::string &val, time_t expired = 0,
                   const std::string &path = "", const std::string &domain = "", bool secure = false);

private:
    // http状态
    HttpStatus m_status;
    // 版本
    uint8_t m_version;
    // 是否关闭
    bool m_close;
    // 是否是websocket
    bool m_websocket;
    // 响应消息体
    std::string m_body;
    // 响应原因
    std::string m_reason;
    // 响应头部MAP
    MapType m_headers;
    // cookie
    std::vector<std::string> m_cookie;
};

// 重载operator<< 全局
std::ostream &operator<<(std::ostream &os, const HttpResponse &httpResponse);

std::ostream &operator<<(std::ostream &os, const HttpRequest &httpRequest);

}
} // namespace webs::http

#endif

// int main() {
//     return 0;
// }