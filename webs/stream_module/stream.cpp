#include "stream.h"

namespace webs {

int Stream::readFixSize(void *buf, size_t length) {
    size_t offset = 0;
    int64_t left = length;
    while (left > 0) {
        int64_t rt = read((char *)buf + offset, left);
        if (rt <= 0) {
            return rt;
        }
        left -= rt;
        offset += rt;
    }
    return length;
}

int Stream::readFixSize(ByteArray::ptr ba, size_t length) {
    uint64_t left = length;
    while (left > 0) {               // read函数是派生类提供的；比如sock_stream、zlib_stream
        int64_t rt = read(ba, left); // ByteArray对象自带m_position，用于标记读位置
        if (rt <= 0) {
            return rt;
        }
        left -= rt;
    }
    return length;
}

int Stream::writeFixSize(const void *buf, size_t length) {
    size_t offset = 0;
    int64_t left = length;
    while (left > 0) {
        int64_t rt = write((const char *)buf + offset, left);
        if (rt <= 0) {
            return rt;
        }
        left -= rt;
        offset += rt;
    }
    return length;
}

int Stream::writeFixSize(ByteArray::ptr ba, size_t length) {
    uint64_t left = length;
    while (left > 0) {
        int64_t rt = write(ba, left);
        if (rt <= 0) {
            return rt;
        }
        left -= rt;
    }
    return length;
}
} // namespace webs