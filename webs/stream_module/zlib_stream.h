#ifndef __WEBS_ZLIB_STREAM_H__
#define __WEBS_ZLIB_STREAM_H__

#include <memory>
#include <zlib.h>
#include <vector>
#include "stream.h"

namespace webs {
class ZlibStream : public Stream {
public:
    typedef std::shared_ptr<ZlibStream> ptr;

    /**
     * @brief 压缩类型枚举
     * 
     */
    enum Type {
        ZLIB,
        DEFLATE, // 使用了LZ77算法与哈夫曼编码
        GZIP
    };

    /**
     * @brief 压缩策略枚举
     * 
     */
    enum Strategy {
        DEFAULT = Z_DEFAULT_STRATEGY, // 默认策略
        FILTERED = Z_FILTERED,        // 过滤器策略
        HUFFMAN = Z_HUFFMAN_ONLY,     // 仅使用Huffman编码策略
        FIXED = Z_FIXED,              // 固定编码策略
        RLE = Z_RLE                   // RLE编码策略
    };

    /**
     * @brief 压缩级别枚举
     * 
     */
    enum CompressLevel {
        NO_COMPRESSION = Z_NO_COMPRESSION,          // 无压缩
        BEST_SPEED = Z_BEST_SPEED,                  // 最快速度压缩
        BEST_COMPRESSION = Z_BEST_COMPRESSION,      // 最佳压缩率
        DEFAULT_COMPRESSION = Z_DEFAULT_COMPRESSION // 默认压缩级别
    };

    /**
     * @brief 创建一个gzip格式的ZlibStream实例
     * 
     * @param encode 
     * @param buff_size 
     * @return ZlibStream::ptr 
     */
    static ZlibStream::ptr CreateGzip(bool encode, uint32_t buff_size = 4096);

    /**
     * @brief 创建一个zlib格式的ZlibStream实例
     * 
     * @param encode 
     * @param buff_size 
     * @return ZlibStream::ptr 
     */
    static ZlibStream::ptr CreateZlib(bool encode, uint32_t buff_size = 4096);

    /**
     * @brief 创建一个deflate格式的ZlibStream实例
     * 
     * @param encode 
     * @param buff_size 
     * @return ZlibStream::ptr 
     */
    static ZlibStream::ptr CreateDeflate(bool encode, uint32_t buff_size = 4096);

    /**
     * @brief 创建一个ZlibStream实例
     * 
     * @param encode 
     * @param buff_size 
     * @param type 
     * @param level 
     * @param windows_bits 
     * @param memlevel 
     * @param strategy 
     * @return ZlibStream::ptr 
     */
    static ZlibStream::ptr Create(bool encode, uint32_t buff_size = 4096, Type type = DEFLATE,
                                  int level = DEFAULT_COMPRESSION, int window_bits = 15, int memlevel = 8, Strategy strategy = DEFAULT);

    ZlibStream(bool encode, uint32_t buff_size = 4096);

    ~ZlibStream();

    /**
     * @brief 不需要此函数
     * 
     * @param buf 
     * @param length 
     * @return int 
     */
    virtual int read(void *buf, size_t length) override;

    /**
     * @brief 不需要此函数
     * 
     * @param ba 
     * @param length 
     * @return int 
     */
    virtual int read(ByteArray::ptr ba, size_t length) override;

    /**
     * @brief 将指定的数据写入流，返回实际写入的字节数
     * 
     * @param buf 
     * @param length 
     * @return int 
     */
    virtual int write(const void *buf, size_t length) override;

    /**
     * @brief 将指定的数据写入流，返回实际写入的字节数
     * 
     * @param ba 
     * @param length 
     * @return int 
     */
    virtual int write(ByteArray::ptr ba, size_t length) override;

    /**
     * @brief 关闭流
     * 
     */
    virtual void close() override;

    /**
     * @brief 刷新压缩或解压缩缓冲区
     * 
     * @return int 
     */
    int flush();

    bool isFree() const {
        return m_free;
    }

    void setFree(bool value) {
        m_free = value;
    }

    bool isEncode() const {
        return m_encode;
    }

    void setEncode(bool value) {
        m_encode = value;
    }

    std::vector<iovec> &getBuffers() {
        return m_buffs;
    }

    std::string getResult() const;

    webs::ByteArray::ptr getByteArray() const;

private:
    /**
     * @brief 初始化z_stream结构体的成员变量
     * 
     * @param type 
     * @param level 
     * @param window_bits 
     * @param memlevel 
     * @param strategy 
     * @return int 
     */
    int init(Type type = DEFLATE, int level = DEFAULT_COMPRESSION, int window_bits = 15,
             int memlevel = 8, Strategy strategy = DEFAULT);

    /**
     * @brief 对指定的数据进行编码
     * 
     * @param v 
     * @param size 
     * @param finish 是否结束
     * @return int 
     */
    int encode(const iovec *v, const uint64_t &size, bool finish);

    /**
     * @brief 对指定的数据进行解码
     * 
     * @param v 
     * @param size 
     * @param finish 
     * @return int 
     */
    int decode(const iovec *v, const uint64_t &size, bool finish);

private:
    z_stream m_zstream;         // zlib库中的z_stream结构体
    uint32_t m_buffSize;        // 缓冲区大小
    bool m_encode;              // 是否进行编码
    bool m_free;                // 流是否空闲
    std::vector<iovec> m_buffs; // 缓冲区列表
};
} // namespace webs

#endif