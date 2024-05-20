#include "http.h"
#include "../util_module/util.h"

namespace webs {
namespace http {

HttpMethod StringToHttpMethod(const std::string &m) {
#define XX(num, name, string)              \
    if (strcmp(#string, m.c_str()) == 0) { \
        return HttpMethod::name;           \
    }
    HTTP_METHOD_MAP(XX);
#undef XX
    return HttpMethod::INVALID_METHOD;
}

/* 使用strncmp的原因是： 在传入参数时，使用的时const char* 指向操作的起始位置，只能比较n个字符是否相同 */
HttpMethod CharsToHttpMethod(const char *m) {
#define XX(num, name, string)                        \
    if (strncmp(#string, m, strlen(#string)) == 0) { \
        return HttpMethod::name;                     \
    }
    HTTP_METHOD_MAP(XX);
#undef XX
    return HttpMethod::INVALID_METHOD;
}

static const char *s_method_string[] = {
#define XX(nun, name, string) #string,
    HTTP_METHOD_MAP(XX)
#undef XX
};

/* 方法数字是连续的，因此可以使用数组的长度判断 */
const char *HttpMethodToString(const HttpMethod &m) {
    uint32_t idx = (uint32_t)m;
    if (idx >= sizeof(s_method_string) / sizeof(s_method_string[0])) {
        return "<unknown>";
    }
    return s_method_string[idx];
}

/* 返回状态字对应的描述；状态数字是非连续的，只能使用条件判断 */
const char *HttpStatusToString(const HttpStatus &s) {
    switch (s) {
#define XX(code, name, desc) \
    case HttpStatus::name:   \
        return #desc;
        HTTP_STATUS_MAP(XX);
#undef XX
    default:
        return "<unknown>";
    }
}

bool CaseInsensitiveLess::operator()(const std::string &lhs, const std::string &rhs) const {
    // strcasecmp 比较两个字符串的字符，忽略大小写;lhs < rhs，则返回负数
    return strcasecmp(lhs.c_str(), rhs.c_str()) < 0;
}

/* 默认是根目录 */
HttpRequest::HttpRequest(uint8_t version, bool close) :
    m_method(HttpMethod::GET),
    m_version(version),
    m_close(close),
    m_websocket(false),
    m_parserParamFlag(0),
    m_path("/") {
}

std::shared_ptr<HttpResponse> HttpRequest::createResponse() {
    HttpResponse::ptr spt(new HttpResponse(getVersion(), isClose()));
    return spt;
}

std::string HttpRequest::getHeader(const std::string &key, const std::string &def) const {
    auto it = m_headers.find(key);
    return it == m_headers.end() ? def : it->second;
}

std::string HttpRequest::getParam(const std::string &key, const std::string &def) {
    initQueryParam();
    initBodyParam();
    auto it = m_headers.find(key);
    return it == m_headers.end() ? def : it->second;
}

std::string HttpRequest::getCookie(const std::string &key, const std::string &def) {
    initCookies();
    auto it = m_headers.find(key);
    return it == m_headers.end() ? def : it->second;
}

void HttpRequest::setParam(const std::string &key, const std::string &val) {
    m_params[key] = val;
}

void HttpRequest::setHeader(const std::string &key, const std::string &val) {
    m_headers[key] = val;
}

void HttpRequest::setCookie(const std::string &key, const std::string &val) {
    m_cookies[key] = val;
}

void HttpRequest::delParam(const std::string &key) {
    m_params.erase(key);
}

void HttpRequest::delHeader(const std::string &key) {
    m_headers.erase(key);
}

void HttpRequest::delCookie(const std::string &key) {
    m_cookies.erase(key);
}

bool HttpRequest::hasParam(const std::string &key, std::string *val) {
    initQueryParam();
    initBodyParam();
    auto it = m_params.find(key);
    if (it == m_params.end()) {
        return false;
    }
    if (val) {
        *val = it->second;
    }
    return true;
}

bool HttpRequest::hasHeader(const std::string &key, std::string *val) {
    auto it = m_headers.find(key);
    if (it == m_headers.end()) {
        return false;
    }
    if (val) {
        *val = it->second;
    }
    return true;
}

bool HttpRequest::hasCookie(const std::string &key, std::string *val) {
    initCookies();
    auto it = m_cookies.find(key);
    if (it == m_cookies.end()) {
        return false;
    }
    if (val) {
        *val = it->second;
    }
    return true;
}

/**
 * 检查 m_parserParamFlag 的最低位是否为 1 -- 依次读取 key=value& 先找=，再找& --- 找到之后再添加新的键值对
 * 
 */
void HttpRequest::initQueryParam() {
    if (m_parserParamFlag & 0x1) { // 最低位是否为 1，如果是，说明查询参数已经解析过，直接返回。
        return;
    }

// 该宏负责解析 str 字符串，并将解析后的键值对插入到 m 中。flag 表示键值对之间的连接符号。trim 表示对字符串的处理函数，比如去除首尾空格
#define PARSE_PARAM(str, m, flag, trim)                                                            \
    size_t pos = 0;                                                                                \
    do {                                                                                           \
        size_t last = pos;                                                                         \
        pos = str.find('=', pos);                                                                  \
        if (pos == std::string::npos) {                                                            \
            break;                                                                                 \
        }                                                                                          \
        size_t key = pos;                                                                          \
        pos = str.find(flag, pos);                                                                 \
        m.insert(std::make_pair(trim(str.substr(last, key - last)),                                \
                                webs::StringUtil::UrlDecode(str.substr(key + 1, pos - key - 1)))); \
        if (pos == std::string::npos) {                                                            \
            break;                                                                                 \
        }                                                                                          \
        ++pos;                                                                                     \
    } while (true);

    PARSE_PARAM(m_query, m_params, '&', );
    // 解析完之后置位
    m_parserParamFlag |= 0x1;
}

/* 是否已经检查 -- 获取 请求头中key 为 content-type 的值 -- 检查值中是否出现了 "application/x-www-form-urlencoded" 
如果出现，使用 宏PARSE_PARAM 解析 m_body，并将结果加入 m_params
*/
void HttpRequest::initBodyParam() {
    if (m_parserParamFlag & 0x2) {
        return;
    }
    std::string val = getHeader("content-type");
    if (strcasestr(val.c_str(), "application/x-www-form-urlencoded") == nullptr) {
        m_parserParamFlag |= 0x2;
        return;
    }
    PARSE_PARAM(m_body, m_params, '&', );
    m_parserParamFlag |= 0x2;
}

/* 检查是否已经初始化 -- 获取请求头中key为cookie的值val -- 提取 val 中的键值对，添加到m_params中 */
void HttpRequest::initCookies() {
    if (m_parserParamFlag & 0x4) {
        return;
    }
    std::string cookie = getHeader("cookie");
    if (cookie.empty()) {
        m_parserParamFlag |= 0x4;
        return;
    }
    PARSE_PARAM(cookie, m_cookies, ',', webs::StringUtil::Trim);
    m_parserParamFlag |= 0x4;
}

std::ostream &HttpRequest::dump(std::ostream &os) const {
    os << HttpMethodToString(m_method) << " " << m_path
       << (m_query.empty() ? "" : "?") << m_query
       << (m_fragment.empty() ? "" : "#") << m_fragment
       << "http" << ((uint32_t)(m_version >> 4)) << "." << ((uint32_t)(m_version & 0x0F))
       << "\r\n";
    if (!m_websocket) {
        os << "connection: " << (m_close ? "close" : "keep-alive") << "\r\n";
    }
    // 去除connection的key
    for (auto &it : m_headers) {
        if (!m_websocket && strcasecmp(it.first.c_str(), "connection") == 0) {
            continue;
        }
        os << it.first << ": " << it.second << "\r\n";
    }
    if (!m_body.empty()) {
        os << "content-length: " << m_body.size() << "\r\n\r\n"
           << m_body;
    } else {
        os << "\r\n";
    }
    return os;
}

std::string HttpRequest::toString() const {
    std::stringstream ss;
    dump(ss);
    return ss.str();
}

void HttpRequest::init() {
    std::string conn = getHeader("connection");
    if (!conn.empty()) {
        if (strcasecmp(conn.c_str(), "keep-alive") == 0) {
            m_close = false;
        } else {
            m_close = true;
        }
    }
}

void HttpRequest::initParam() {
    initQueryParam();
    initBodyParam();
    initCookies();
}
HttpResponse::HttpResponse(uint8_t version, bool close) :
    m_status(HttpStatus::OK),
    m_version(version),
    m_close(close),
    m_websocket(false) {
}

std::string HttpResponse::getHeader(const std::string &key, const std::string &def) const {
    auto it = m_headers.find(key);
    return it == m_headers.end() ? def : it->second;
}

void HttpResponse::setHeader(const std::string &key, const std::string &val) {
    m_headers[key] = val;
}

void HttpResponse::delHeader(const std::string &key) {
    m_headers.erase(key);
}

// 在 HTTP 协议中，302 Found 状态码表示请求的资源已被临时移动到由 Location 头部字段给出的 URL。这是执行 URL 重定向的常见方式是
void HttpResponse::setRedirect(const std::string &url) {
    m_status = HttpStatus::FOUND;
    setHeader("Location", url);
}

void HttpResponse::setCookie(const std::string &key, const std::string &val, time_t expired,
                             const std::string &path, const std::string &domain, bool secure) {
    std::stringstream ss;
    ss << key << "=" << val;
    if (expired > 0) {
        ss << ";expires=" << webs::Time2Str(expired, "%a, %d %b %Y %H:%M:%S") << " GMT";
    }
    if (!domain.empty()) {
        ss << ";domain=" << domain;
    }
    if (!path.empty()) {
        ss << ";path=" << path;
    }
    if (secure) {
        ss << ";secure";
    }
    m_cookie.push_back(ss.str());
}

std::string HttpResponse::toString() const {
    std::stringstream ss;
    dump(ss);
    return ss.str();
}

std::ostream &HttpResponse::dump(std::ostream &os) const {
    os << "HTTP/" << ((uint32_t)(m_version >> 4)) << "." << ((uint32_t)(m_version & 0x0F))
       << " " << (uint32_t)m_status << " " << (m_reason.empty() ? HttpStatusToString(m_status) : m_reason)
       << "\r\n";
    for (auto &it : m_headers) {
        if (!m_websocket && strcasecmp(it.first.c_str(), "connection") == 0) {
            continue;
        }
        os << it.first << ": " << it.second << "\r\n";
    }
    for (auto &it : m_cookie) {
        os << "Set-Cookie: " << it << "\r\n";
    }
    if (!m_websocket) {
        os << "connection: " << (m_close ? "close" : "keep-alive") << "\r\n";
    }
    if (!m_body.empty()) {
        os << "content-length: " << m_body.size() << "\r\n\r\n"
           << m_body;
    } else {
        os << "\r\n";
    }
    return os;
}

std::ostream &operator<<(std::ostream &os, const HttpResponse &httpResponse) {
    return httpResponse.dump(os);
}

std::ostream &operator<<(std::ostream &os, const HttpRequest &httpRequest) {
    return httpRequest.dump(os);
}

}
} // namespace webs::http