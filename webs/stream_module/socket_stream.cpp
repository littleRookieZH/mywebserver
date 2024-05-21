#include "socket_stream.h"

#include <vector>

namespace webs {

SocketStream::SocketStream(Socket::ptr sock, bool owner) :
    m_socket(sock),
    m_owner(owner) {
}

SocketStream::~SocketStream() {
    if (m_owner && m_socket) {
        m_socket->close();
    }
}

bool SocketStream::isConnected() const {
    return m_socket && m_socket->isConnected();
}

/* 是否连接 -- recv接受数据 */
int SocketStream::read(void *buf, size_t length) {
    if (!isConnected()) {
        return -1;
    }
    return m_socket->recv(buf, length);
}

int SocketStream::read(ByteArray::ptr ba, size_t length) {
    if (!isConnected()) {
        return -1;
    }
    std::vector<iovec> iov;
    ba->getReadBuffer(iov, length);
    int rt = m_socket->recv(&iov[0], iov.size());
    if (rt > 0) {
        ba->setPosition(ba->getPosition() + rt);
    }
    return rt;
}

int SocketStream::write(const void *buf, size_t length) {
    if (!isConnected()) {
        return -1;
    }
    return m_socket->send(buf, length);
}

int SocketStream::write(ByteArray::ptr ba, size_t length) {
    if (!isConnected()) {
        return -1;
    }
    std::vector<iovec> iov;
    ba->getWriteBuffer(iov, length);
    int rt = m_socket->send(&iov[0], iov.size());
    if (rt > 0) {
        ba->setPosition(ba->getPosition() + rt);
    }
    return rt;
}

void SocketStream::close() {
    if (m_socket) {
        m_socket->close();
    }
}

Address::ptr SocketStream::getRemoteAddress() {
    if (m_socket) {
        return m_socket->getRemoteAddress();
    }
    return nullptr;
}

Address::ptr SocketStream::getLocalAddress() {
    if (m_socket) {
        return m_socket->getLocalAddress();
    }
    return nullptr;
}

std::string SocketStream::getRemoteAddressString() {
    Address::ptr addr = getRemoteAddress();
    if (addr) {
        return addr->toString();
    }
    return "";
}

std::string SocketStream::getLocalAddressString() {
    Address::ptr addr = getLocalAddress();
    if (addr) {
        return addr->toString();
    }
    return "";
}

} // namespace webs