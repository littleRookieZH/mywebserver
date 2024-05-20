
#ifndef http11_parser_h
#define http11_parser_h

#include "http11_common.h"

/**
 * @brief 用于解析 HTTP 请求
 * 
 * field_start：这个字段标记了 HTTP 头部字段开始的位置。HTTP 头部字段是键值对的形式，
 * 例如 Content-Type: application/json，field_start 就是这些字段开始的位置。
 * 
 * query_start：这个字段标记了 URI 中查询参数开始的位置。查询参数通常出现在 URL 的 ? 之后，
 * 例如在 URL http://example.com/page?param1=value1&param2=value2 中，
 * query_start 就是 param1=value1&param2=value2 开始的位置。
 */
typedef struct http_parser {
    int cs;             // 当前解析器的状态。
    size_t body_start;  // HTTP 主体开始的位置。
    int content_len;    // HTTP 主体的长度。
    size_t nread;       // 表示已经读取的字节数
    size_t mark;        // 标记的位置，用于内部处理。
    size_t field_start; //字段开始的位置。
    size_t field_len;   // 字段的长度。
    size_t query_start; // 查询字符串开始的位置。
    int xml_sent;       // 是否已发送 XML。
    int json_sent;      // 是否已发送 JSON。

    void *data; // 指向用户数据的指针。

    int uri_relaxed;           // URI 解析是否宽松。
    field_cb http_field;       // 处理 HTTP 字段的回调函数。
    element_cb request_method; //处理请求方法的回调函数。
    element_cb request_uri;    // 处理请求 URI 的回调函数。
    element_cb fragment;       //处理 URI 片段的回调函数。
    element_cb request_path;   //处理请求路径的回调函数。
    element_cb query_string;   //处理查询字符串的回调函数。
    element_cb http_version;   //处理 HTTP 版本的回调函数。
    element_cb header_done;    //处理头部完成的回调函数。

} http_parser;

int http_parser_init(http_parser *parser);
int http_parser_finish(http_parser *parser);
size_t http_parser_execute(http_parser *parser, const char *data, size_t len, size_t off);
int http_parser_has_error(http_parser *parser);
int http_parser_is_finished(http_parser *parser);

#define http_parser_nread(parser) (parser)->nread

#endif