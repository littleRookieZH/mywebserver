#include "http_parser.h"
#include "../log_module/log.h"
#include "../config_module/config.h"

namespace webs {
namespace http {

static webs::Logger::ptr g_logger = WEBS_LOG_NAME("system");

static webs::ConfigVar<uint64_t>::ptr g_http_request_buffer_size =
    webs::Config::Lookup("http.request.buffer_size", (uint64_t)(4 * 1024), "http request buffer size");

static webs::ConfigVar<uint64_t>::ptr g_http_request_max_body_size =
    webs::Config::Lookup("http.request.max_body_size", (uint64_t)(64 * 1024 * 1024), "http request max body size");

static webs::ConfigVar<uint64_t>::ptr g_http_response_buffer_size =
    webs::Config::Lookup("http.response.buffer_size", (uint64_t)(4 * 1024), "http response buffer size");

static webs::ConfigVar<uint64_t>::ptr g_http_response_max_body_size =
    webs::Config::Lookup("http.response.max_body_size", (uint64_t)(64 * 1024 * 1024), "http response max body size");

static uint64_t s_http_request_buffer_size = 0;
static uint64_t s_http_request_max_body_size = 0;
static uint64_t s_http_response_buffer_size = 0;
static uint64_t s_http_response_max_body_size = 0;

uint64_t HttpRequestParser::GetHttpRequestBufferSize() {
    return s_http_request_buffer_size;
}

uint64_t HttpRequestParser::GetHttpRequestMaxBodySize() {
    return s_http_request_max_body_size;
}

uint64_t HttpResponseParser::GetHttpResponseBufferSize() {
    return s_http_response_buffer_size;
}

uint64_t HttpResponseParser::GetHttpResponseMaxBodySize() {
    return s_http_response_max_body_size;
}

/* 使用匿名命名空间初始化参数；设置回调函数 */
namespace {
struct _RequestSizeIniter {
    _RequestSizeIniter() {
        // 初始化值
        s_http_request_buffer_size = g_http_request_buffer_size->getValue();
        s_http_request_max_body_size = g_http_request_max_body_size->getValue();
        s_http_response_buffer_size = g_http_response_buffer_size->getValue();
        s_http_response_max_body_size = g_http_response_max_body_size->getValue();

        // 添加回调函数。更新值：调用 setValue() --> 调用回调函数更新值
        g_http_request_buffer_size->addListener([](const uint64_t &oldValue, const uint64_t &newValue) { s_http_request_buffer_size = newValue; });
        g_http_request_max_body_size->addListener([](const uint64_t &oldValue, const uint64_t &newValue) { s_http_request_max_body_size = newValue; });
        g_http_response_buffer_size->addListener([](const uint64_t &oldValue, const uint64_t &newValue) { s_http_response_buffer_size = newValue; });
        g_http_response_max_body_size->addListener([](const uint64_t &oldValue, const uint64_t &newValue) { s_http_response_max_body_size = newValue; });
    }
};
static _RequestSizeIniter _init;
} // namespace

/* 类型转换 -- 解析方法名 -- 输出日志 -- 设置方法名(修改data信息) */
void on_request_method(void *data, const char *at, size_t length) {
    HttpRequestParser *parse = static_cast<HttpRequestParser *>(data);
    HttpMethod method = CharsToHttpMethod(at); // 使用chars的原因是：at指向起始位置，如果使用string，还需要复制一份
    if (method == HttpMethod::INVALID_METHOD) {
        WEBS_LOG_WARN(g_logger) << "invalid request method: " << std::string(at, length);
        parse->setError(1000);
        return;
    }
    parse->getData()->setMethod(method);
}

void on_request_uri(void *data, const char *at, size_t length) {
    // 由 on_request_fragment、on_request_path、on_request_query 解析
}

void on_request_fragment(void *data, const char *at, size_t length) {
    HttpRequestParser *parser = static_cast<HttpRequestParser *>(data);
    parser->getData()->setFragment(std::string(at, length));
}

void on_request_path(void *data, const char *at, size_t length) {
    HttpRequestParser *parser = static_cast<HttpRequestParser *>(data);
    parser->getData()->setPath(std::string(at, length));
}

void on_request_query(void *data, const char *at, size_t length) {
    HttpRequestParser *parser = static_cast<HttpRequestParser *>(data);
    parser->getData()->setQuery(std::string(at, length));
}

void on_request_version(void *data, const char *at, size_t length) {
    HttpRequestParser *parser = static_cast<HttpRequestParser *>(data);
    uint8_t version = 0;
    if (strncmp(at, "HTTP/1.1", length) == 0) {
        version = 0x11;
    } else if (strncmp(at, "HTTP/1.0", length) == 0) {
        version = 0x10;
    } else {
        WEBS_LOG_WARN(g_logger) << "invalid http request version: " << std::string(at, length);
        parser->setError(1001);
        return;
    }
    parser->getData()->setVersin(version);
}

void on_request_header_done(void *data, const char *at, size_t length) {
    // 使用on_request_http_field解析请求头信息
}

/* field: header - key; vlen: header - value */
void on_request_http_field(void *data, const char *field, size_t flen, const char *value, size_t vlen) {
    HttpRequestParser *parser = static_cast<HttpRequestParser *>(data);
    if (flen == 0) {
        WEBS_LOG_WARN(g_logger) << "invalid http request field length == 0";
        return;
    }
    parser->getData()->setHeader(std::string(field, flen), std::string(value, vlen));
}

HttpRequestParser::HttpRequestParser() :
    m_error(0) {
    m_data.reset(new webs::http::HttpRequest);
    http_parser_init(&m_parser);
    m_parser.request_method = on_request_method;
    m_parser.request_uri = on_request_uri;
    m_parser.fragment = on_request_fragment;
    m_parser.request_path = on_request_path;
    m_parser.query_string = on_request_query;
    m_parser.http_version = on_request_version;
    m_parser.header_done = on_request_header_done;
    m_parser.http_field = on_request_http_field;
    m_parser.data = this;
}

uint64_t HttpRequestParser::getContentLength() {
    return m_data->getHeaderAs<uint64_t>("content-length", 0);
}

/* http_parser_execute 的返回值为 >= 0 表示解析的数据长度；memmove：将数据向前移动，避免数据被缓存区切分 */
size_t HttpRequestParser::execute(char *data, size_t len) {
    size_t offset = http_parser_execute(&m_parser, data, len, 0);
    memmove(data, data + offset, (len - offset));
    return offset;
}

int HttpRequestParser::isFinished() {
    return http_parser_finish(&m_parser);
}

int HttpRequestParser::hasError() {
    return m_error || http_parser_has_error(&m_parser);
}

void on_response_reason(void *data, const char *at, size_t length) {
    HttpResponseParser *parser = static_cast<HttpResponseParser *>(data);
    parser->getData()->setReason(std::string(at, length));
}

void on_response_status(void *data, const char *at, size_t length) {
    HttpResponseParser *parser = static_cast<HttpResponseParser *>(data);
    HttpStatus status = (HttpStatus)atoi(at);
    parser->getData()->setStatus(status);
}

void on_response_chunk(void *data, const char *at, size_t length) {
}

void on_response_version(void *data, const char *at, size_t length) {
    HttpResponseParser *parser = static_cast<HttpResponseParser *>(data);
    uint8_t version = 0;
    if (strncmp(at, "HTTP/1.1", length) == 0) {
        version = 0x11;
    } else if (strncmp(at, "HTTP/1.0", length) == 0) {
        version = 0x10;
    } else {
        parser->setErrro(1001);
        WEBS_LOG_WARN(g_logger) << "invaild http response version: " << std::string(at, length);
        return;
    }
    parser->getData()->setVersion(version);
}

void on_response_header_done(void *data, const char *at, size_t length) {
}

void on_response_last_chunk(void *data, const char *at, size_t length) {
}

void on_response_http_field(void *data, const char *field, size_t flen, const char *value, size_t vlen) {
    HttpResponseParser *parser = static_cast<HttpResponseParser *>(data);
    if (flen == 0) {
        WEBS_LOG_WARN(g_logger) << "invalid http response field length == 0";
        return;
    }
    parser->getData()->setHeader(std::string(field, flen), std::string(value, vlen));
}

HttpResponseParser::HttpResponseParser() :
    m_error(0) {
    m_data.reset(new webs::http::HttpResponse);
    httpclient_parser_init(&m_parser);
    m_parser.reason_phrase = on_response_reason;
    m_parser.status_code = on_response_status;
    m_parser.chunk_size = on_response_chunk;
    m_parser.http_version = on_response_version;
    m_parser.header_done = on_response_header_done;
    m_parser.last_chunk = on_response_last_chunk;
    m_parser.http_field = on_response_http_field;
    m_parser.data = this;
}

int HttpResponseParser::execute(char *data, size_t len, bool chunck) {
    if (chunck) { //如果分段，则重新初始化数据再解析数据
        httpclient_parser_init(&m_parser);
    }
    int offset = httpclient_parser_execute(&m_parser, data, len, 0);
    if (offset == -1) {
        return offset;
    }
    memmove(data, data + offset, len - offset); // 把解析过的内存移走
    return offset;
}

int HttpResponseParser::isFinished() {
    return httpclient_parser_finish(&m_parser);
}

int HttpResponseParser::hasError() {
    return m_error || httpclient_parser_has_error(&m_parser);
}

/* 获取内容长度：从响应头中获取属性为 content-length 的值 */
uint64_t HttpResponseParser::getContentLength() {
    return m_data->getHeaderAs<uint64_t>("content-length", 0);
}

}
} // namespace webs::http
