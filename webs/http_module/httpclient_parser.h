/**
 *
 * Copyright (c) 2010, Zed A. Shaw and Mongrel2 Project Contributors.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 * 
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 * 
 *     * Neither the name of the Mongrel2 Project, Zed A. Shaw, nor the names
 *       of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written
 *       permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef httpclient_parser_h
#define httpclient_parser_h

#include "http11_common.h"

typedef struct httpclient_parser {
    int cs;             // 当前解析器的状态
    size_t body_start;  // HTTP响应体开始的位置
    int content_len;    // HTTP响应体的长度
    int status;         // HTTP响应的状态码
    int chunked;        // 标记是否使用分块传输编码
    int chunks_done;    // 分块传输编码是否已经完成 -- 1表示完成，0表示未完成
    int close;          // 标记是否关闭连接
    size_t nread;       // 已读取的字节数
    size_t mark;        // 标记的位置，用于解析过程中的临时存储
    size_t field_start; // 当前正在解析的字段开始的位置
    size_t field_len;   // 当前正在解析的字段的长度

    void *data; // 用户自定义数据，通常用于回调函数

    field_cb http_field;      // 处理HTTP字段的回调函数
    element_cb reason_phrase; // 处理HTTP状态行中的原因短语的回调函数
    element_cb status_code;   // 处理HTTP状态行中的状态码的回调函数
    element_cb chunk_size;    // 处理分块大小的回调函数
    element_cb http_version;  // 处理HTTP版本的回调函数
    element_cb header_done;   // 处理HTTP头部完成的回调函数
    element_cb last_chunk;    // 处理最后一个分块的回调函数

} httpclient_parser;

int httpclient_parser_init(httpclient_parser *parser);
int httpclient_parser_finish(httpclient_parser *parser);
int httpclient_parser_execute(httpclient_parser *parser, const char *data, size_t len, size_t off);
int httpclient_parser_has_error(httpclient_parser *parser);
int httpclient_parser_is_finished(httpclient_parser *parser);

#define httpclient_parser_nread(parser) (parser)->nread

#endif