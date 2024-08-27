#include "http_session.h"
#include "http_parser.h"

namespace webs {
namespace http {
HttpSession::HttpSession(Socket::ptr sock, bool owner) :
    SocketStream(sock, owner) {
}

/* 创建解析请求的对象 -- 根据请求缓存区大小，创建动态对象
 -- 循环解析数据(read --> execute --> 更新解析长度) -- 读取消息体 */
HttpRequest::ptr HttpSession::recvRequest() {
    HttpRequestParser::ptr parser(new HttpRequestParser());
    uint64_t buff_size = HttpRequestParser::GetHttpRequestBufferSize();
    std::shared_ptr<char> buffer(new char[buff_size], [](char *ptr) { delete[] ptr; });
    int offset = 0;
    char *data = buffer.get();
    do {
        int len = read(data + offset, buff_size - offset);
        if (len <= 0) {
            close();
            return nullptr;
        }
        len += offset;
        size_t nparse = parser->execute(data, len);
        if (parser->hasError()) {
            close();
            return nullptr;
        }
        offset = len - nparse;
        if (offset == (int)buff_size) {
            close();
            return nullptr;
        }
        if (parser->isFinished()) {
            break;
        }
    } while (true);
    int64_t length = parser->getContentLength();
    if (length > 0) {
        std::string body;
        body.resize(length);

        int len = 0; // 记录从data从读取了多少个字符
        if (length >= offset) {
            memcpy(&body[0], data, offset);
            len = offset;
        } else {
            memcpy(&body[0], data, length);
            len = length;
        }
        length -= offset;
        if (length > 0) { // 说明还有数据没读
            if (readFixSize(&body[len], length) <= 0) {
                close();
                return nullptr;
            }
        }
        parser->getData()->setBody(body);
    }
    parser->getData()->init();
    return parser->getData();
}

int HttpSession::sendResponse(HttpResponse::ptr rsp) {
    std::stringstream ss;
    ss << *rsp;
    std::string data = ss.str();
    // SocketStream::writeFixSize  -->  SocketStream::write -->  将数据写入到m_socket中
    return writeFixSize(data.c_str(), data.size());
}

}
} // namespace webs::http