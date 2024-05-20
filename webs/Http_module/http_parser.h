#ifndef __WEBS_HTTP_PARSER_H__
#define __WEBS_HTTP_PARSER_H__

#include <memory>

#include "http.h"
#include "http11_parser.h"
#include "httpclient_parser.h"

namespace webs {
namespace http {
class HttpRequestParser {
public:
    // HTTP请求解析类的智能指针
    typedef std::shared_ptr<HttpRequestParser> ptr;

    HttpRequestParser();

    /**
     * @brief 解析协议
     * 
     * @param data 协议文本
     * @param len 协议文本内存长度
     * @return size_t 返回实际解析的长度，并且将已解析的数据移除
     */
    size_t execute(char *data, size_t len);

    /**
     * @brief 是否解析完成
     * 
     * @return int 
     */
    int isFinished();

    /**
     * @brief 是否有错误
     * 
     * @return int 
     */
    int hasError();

    /**
     * @brief 设置错误
     * 
     * @param value 
     */
    void setError(int value) {
        m_error = value;
    }

    /**
     * @brief 返回 HttpRequest 结构体
     * 
     * @return HttpRequest::ptr 
     */
    HttpRequest::ptr getData() const {
        return m_data;
    }

    /**
     * @brief 获取消息体长度
     * 
     * @return uint64_t 
     */
    uint64_t getContentLength();

    /**
     * @brief 获取http_parser结构体
     * 
     * @return const http_parser& 
     */
    const http_parser &getParser() const {
        return m_parser;
    }

public:
    /**
     * @brief 返回HttpRequest协议 解析的缓存大小
     * 
     * @return uint64_t 
     */
    static uint64_t GetHttpRequestBufferSize();

    /**
     * @brief 返回 HttpRequest协议 的最大消息体大小
     * 
     * @return uint64_t 
     */
    static uint64_t GetHttpRequestMaxBodySize();

private:
    // 协议参数结构体
    http_parser m_parser;
    // 解析出来的协议数据
    HttpRequest::ptr m_data;
    // 错误码
    // 1000: invalid method
    // 1001: invalid version
    // 1002: invalid field
    int m_error;
};

class HttpResponseParser {
public:
    typedef std::shared_ptr<HttpResponseParser> ptr;
    HttpResponseParser();

    /**
     * @brief 解析HTTP 响应请求
     * 
     * @param data 协议数据内存
     * @param len 协议数据内存大小
     * @param chunck 是否解析chunck
     * @return int 返回实际解析的长度，并且移除已解析的数据
     */
    int execute(char *data, size_t len, bool chunck);

    /**
     * @brief 是否解析完成
     * 
     * @return int 
     */
    int isFinished();

    /**
     * @brief 是否有错误
     * 
     * @return int 
     */
    int hasError();

    /**
     * @brief 设置错误码
     * 
     * @param value 
     */
    void setErrro(int value) {
        m_error = value;
    }

    /**
     * @brief 返回httpclient_parser
     * 
     * @return const httpclient_parser& 
     */
    const httpclient_parser &getParser() const {
        return m_parser;
    }

    /**
     * @brief 返回HttpResponse
     * 
     * @return HttpResponse::ptr 
     */
    HttpResponse::ptr getData() const {
        return m_data;
    }

    /**
     * @brief 获取消息体的长度
     * 
     * @return uint64_t 
     */
    uint64_t getContentLength();

public:
    /**
     * @brief 返回HTTP响应 解析缓存大小
     * 
     * @return uint64_t 
     */
    static uint64_t GetHttpResponseBufferSize();

    /**
     * @brief 返回HTTP响应 最大消息体大小
     * 
     * @return uint64_t 
     */
    static uint64_t GetHttpResponseMaxBodySize();

private:
    // httpclient_parser
    httpclient_parser m_parser;
    // HttpResponse
    HttpResponse::ptr m_data;
    // 错误码
    // 1001: invalid version
    // 1002：invalid field
    int m_error;
};
}
} // namespace webs::http

#endif