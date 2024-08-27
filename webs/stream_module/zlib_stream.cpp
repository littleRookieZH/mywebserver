#include "zlib_stream.h"
#include "../util_module/macro.h"
#include <string.h>

namespace webs {

ZlibStream::ptr ZlibStream::CreateGzip(bool encode, uint32_t buff_size) {
    return Create(encode, buff_size, GZIP);
}

ZlibStream::ptr ZlibStream::CreateZlib(bool encode, uint32_t buff_size) {
    return Create(encode, buff_size, ZLIB);
}

ZlibStream::ptr ZlibStream::CreateDeflate(bool encode, uint32_t buff_size) {
    return Create(encode, buff_size, DEFLATE);
}

/* 创建ptr -- 初始化数据 */
ZlibStream::ptr ZlibStream::Create(bool encode, uint32_t buff_size, Type type,
                                   int level, int window_bits, int memlevel, Strategy strategy) {
    ZlibStream::ptr zlib = std::make_shared<ZlibStream>(encode, buff_size);
    if (zlib->init(type, level, window_bits, memlevel, strategy) == Z_OK) {
        return zlib;
    }
    return nullptr;
}

ZlibStream::ZlibStream(bool encode, uint32_t buff_size) :
    m_buffSize(buff_size),
    m_encode(encode),
    m_free(true) {
}

ZlibStream::~ZlibStream() {
    flush();
}

int ZlibStream::read(void *buf, size_t length) {
    throw std::logic_error("ZlibStream::read is invalid");
}

int ZlibStream::read(ByteArray::ptr ba, size_t length) {
    throw std::logic_error("ZlibStream::read is invalid");
}

int ZlibStream::write(const void *buf, size_t length) {
    iovec iov;
    iov.iov_base = (void *)buf;
    iov.iov_len = length;
    if (m_encode) {
        return encode(&iov, 1, false);
    } else {
        return decode(&iov, 1, false);
    }
}

int ZlibStream::write(ByteArray::ptr ba, size_t length) {
    std::vector<iovec> buffers;
    ba->getReadBuffer(buffers, length);
    if (m_encode) { //如何做到void* 和 vector共同使用同一个函数的？  通过次数控制
        return encode(&buffers[0], buffers.size(), false);
    } else {
        return decode(&buffers[0], buffers.size(), false);
    }
}

void ZlibStream::close() {
    flush();
}

/**
 * @brief 
 * 设置压缩流的输入参数  -- 是否刷新 -- 设置输出缓存区 -- 设置压缩流的输出参数 
 * -- 压缩 -- 根据是否还有数据需要压缩决定是否继续压缩 -- 依据是否刷新决定是否释放压缩资源
 * @param v 输入参数
 * @param size 有多少块iovec
 * @param finish 是否刷新
 * @return int Z_OK：操作成功。Z_STREAM_ERROR：流状态错误
 */
int ZlibStream::encode(const iovec *v, const uint64_t &size, bool finish) {
    int flush = 0;
    int rt = 0;
    for (uint64_t i = 0; i < size; ++i) {
        m_zstream.next_in = (Bytef *)v[i].iov_base;                            // 下一个 输入 字节的指针
        m_zstream.avail_in = v[i].iov_len;                                     // next_in 所指向的可用 输入 字节数
        flush = finish ? (i == size - 1 ? Z_FINISH : Z_NO_FLUSH) : Z_NO_FLUSH; // finish为true、最后一次压缩时刷新缓存区
        iovec *iov = nullptr;                                                  // 压缩输出缓存区
        do {
            if (!m_buffs.empty() && m_buffs.back().iov_len != m_buffSize) { // 说明当前数据块还可以存放数据
                iov = &m_buffs.back();
            } else { // 如果缓存区为空或者最后一块缓存区已满，那么将创建一个新的缓存区
                iovec tmp;
                tmp.iov_base = (iovec *)malloc(m_buffSize);
                tmp.iov_len = 0;
                m_buffs.push_back(tmp);
                iov = &m_buffs.back(); // 不能写成 iov = &tmp;
            }
            m_zstream.next_out = (Bytef *)iov->iov_base + iov->iov_len; // 指向下一个将要 输出 的字节的指针。
            m_zstream.avail_out = m_buffSize - iov->iov_len;            // next_out 所指向的剩余可用 输出 空间的字节数。
            rt = deflate(&m_zstream, flush);                            // 压缩数据;flush: 指定如何刷新压缩流。
            if (rt == Z_STREAM_ERROR) {
                return rt;
            }
            iov->iov_len = m_buffSize - m_zstream.avail_out; // 更新结构体大小
        } while (m_zstream.avail_out == 0);                  // 如果缓存区已满，说明可能还有数据要压缩；如果缓存区还有余量未使用，说明数据已经全部压缩完毕，可以压缩下一块数据
    }

    if (flush == Z_FINISH) {
        deflateEnd(&m_zstream); // 作用是结束压缩操作，释放 deflate 函数使用的所有内存和资源。
    }
    return Z_OK;
}

int ZlibStream::decode(const iovec *v, const uint64_t &size, bool finish) {
    int flush = 0;
    int rt = 0;
    for (uint64_t i = 0; i < size; ++i) {
        m_zstream.next_in = (Bytef *)v[i].iov_base;                            // 下一个 输入 字节的指针
        m_zstream.avail_in = v[i].iov_len;                                     // next_in 所指向的可用 输入 字节数
        flush = finish ? (i == size - 1 ? Z_FINISH : Z_NO_FLUSH) : Z_NO_FLUSH; // finish为true、最后一次解压缩时刷新缓存区
        iovec *iov = nullptr;                                                  // 解压缩输出缓存区
        do {
            if (!m_buffs.empty() && m_buffs.back().iov_len != m_buffSize) { // 说明当前数据块还可以存放数据
                iov = &m_buffs.back();
            } else { // 如果缓存区为空或者最后一块缓存区已满，那么将创建一个新的解压缩缓存区
                iovec tmp;
                tmp.iov_base = (iovec *)malloc(m_buffSize);
                tmp.iov_len = 0;
                m_buffs.push_back(tmp);
                iov = &m_buffs.back(); // 不能写成 iov = &tmp;
            }
            m_zstream.next_out = (Bytef *)iov->iov_base + iov->iov_len; // 指向下一个将要 输出 的字节的指针。
            m_zstream.avail_out = m_buffSize - iov->iov_len;            // next_out 所指向的剩余可用 输出 空间的字节数。
            rt = inflate(&m_zstream, flush);                            // inflate 函数用于解压缩数据
            if (rt == Z_STREAM_ERROR) {
                return rt;
            }
            iov->iov_len = m_buffSize - m_zstream.avail_out; // 更新结构体大小
        } while (m_zstream.avail_out == 0);                  // 如果缓存区已满，说明可能还有数据要压缩；如果缓存区还有余量未使用，说明数据已经全部压缩完毕，可以压缩下一块数据
    }
    if (flush == Z_FINISH) {
        inflateEnd(&m_zstream); // 用于结束解压缩操作并释放 inflate 函数使用的所有资源。
    }
    return Z_OK;
}

int ZlibStream::flush() {
    iovec iov;
    iov.iov_base = nullptr;
    iov.iov_len = 0;
    if (m_encode) {
        return encode(&iov, 1, true);
    } else {
        return decode(&iov, 1, true);
    }
}

int ZlibStream::init(Type type, int level, int window_bits,
                     int memlevel, Strategy strategy) {
    WEBS_ASSERT(((level >= 0 && level <= 9) || level == DEFAULT_COMPRESSION));
    WEBS_ASSERT((window_bits >= 8 && window_bits <= 15));
    WEBS_ASSERT((memlevel >= 1 && memlevel <= 9));
    memset(&m_zstream, 0, sizeof(m_zstream));
    m_zstream.zalloc = Z_NULL;
    m_zstream.zfree = Z_NULL;
    m_zstream.opaque = Z_NULL;

    switch ((int)type) {
    case DEFLATE:
        window_bits = -window_bits;
        break;
    case GZIP:
        window_bits += 16;
        break;
    }

    if (m_encode) {
        // 允许用户设置压缩级别、窗口大小、内存级别和压缩策略等参数。
        return deflateInit2(&m_zstream, level, Z_DEFLATED, window_bits, memlevel, (int)strategy);
    } else {
        return inflateInit2(&m_zstream, window_bits);
    }
}

std::string ZlibStream::getResult() const {
    std::string rt;
    for (auto &i : m_buffs) {
        rt.append((const char *)(i.iov_base), i.iov_len);
    }
    return rt;
}

webs::ByteArray::ptr ZlibStream::getByteArray() const {
    ByteArray::ptr ba = std::make_shared<ByteArray>();
    for (auto &i : m_buffs) {
        ba->write(i.iov_base, i.iov_len);
    }
    ba->setPosition(0); // 重置位置：为了读取数据
    return ba;
}

} // namespace webs
