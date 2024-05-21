/**
 * @file stream.h
 * @author your name (you@domain.com)
 * @brief 流接口
 * 派生类只需要重写纯虚函数即可
 * 作用：用于写解析？没听懂、
 * 可以使用文件来模拟socket;比如实现一个 socketstream 和一个文件stream，就业务而言，我们可以使用文件stream来模拟socketstream进行调试等
 * @version 0.1
 * @date 2024-05-21
 * 
 * 
 */
#ifndef __WEBS_STREAM_H__
#define __WEBS_STREAM_H__

#include <memory>
#include "../util_module/bytearray.h"

namespace webs {
class Stream {
public:
    typedef std::shared_ptr<Stream> ptr;
    virtual ~Stream();

    virtual int read(void *buf, size_t length) = 0;

    virtual int read(ByteArray::ptr ba, size_t length) = 0;

    virtual int write(const void *buf, size_t length) = 0;

    virtual int write(ByteArray::ptr ba, size_t length) = 0;

    virtual int readFixSize(void *buf, size_t length);

    virtual int readFixSize(ByteArray::ptr ba, size_t length);

    virtual int writeFixSize(const void *buf, size_t length);

    virtual int writeFixSize(ByteArray::ptr ba, size_t length);

    virtual void close() = 0;
};
} // namespace webs

#endif