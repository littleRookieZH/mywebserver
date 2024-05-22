/**
 * @brief 序列化与反序列化；
 * 1、用于网络中数据的传输 -- 消除主机字节序与网络字节序的差异
 * 2、提供统一的读写接口 -- 统一操作
 */
#ifndef __WEBS_BYTEARRAY_H__
#define __WEBS_BYTEARRAY_H__

#include <memory>
#include <stdint.h>
#include <vector>
// #include <sys/types.h>
#include <sys/socket.h>

#include "endian.h"

namespace webs {
class ByteArray {
public:
    typedef std::shared_ptr<ByteArray> ptr;

private:
    /**
     * @brief ByteArray的存储节点
     * 
     */
    struct Node {
        Node();
        Node(size_t size);
        ~Node();
        // 内存块指针
        char *ptr;
        // 下一块内存的指针
        Node *next;
        // 内存块大小
        size_t size;
    };

public:
    ByteArray(size_t base_size = 4096);

    ~ByteArray();

    /**
     * 读写数据使用了两种保存数据的方法；一：采用压缩技术；根据数据大小选择合适的数据结构保存。二：根据数据所声明类型对应的数据结构进行保存
     * 压缩技术采用的是 zigzag编码
     *  */

    /**
     * @brief 写入固定长度int8_t类型的数据；非压缩技术
     * 
     * @param value 
     */
    void writeFint8(int8_t value);

    /**
     * @brief 写入固定长度uint8_t类型的数据；非压缩技术
     * 
     * @param value 
     */
    void writeFuint8(uint8_t value);

    /**
     * @brief 写入固定长度int16_t类型的数据；非压缩技术
     * 
     * @param value 
     */
    void writeFint16(int16_t value);

    /**
     * @brief 写入固定长度uint16_t类型的数据；非压缩技术
     * 
     * @param value 
     */
    void writeFuint16(uint16_t value);

    /**
     * @brief 写入固定长度为int32_t类型的数据；非压缩技术
     * 
     * @param value 
     */
    void writeFint32(int32_t value);

    /**
     * @brief 写入固定长度为uint32_t类型的数据；非压缩技术
     * 
     * @param value 
     */
    void writeFuint32(uint32_t value);

    /**
     * @brief 写入固定长度为 int64_t 类型的数据；非压缩技术
     * 
     * @param value 
     */
    void writeFint64(int64_t value);

    /**
     * @brief 写入固定长度为 uint64_t 类型的数据；非压缩技术
     * 
     * @param value 
     */
    void writeFuint64(uint64_t value);

    /**
     * @brief 写入 int32_t 类型的数据；采用压缩技术保存
     * 
     * @param value 
     */
    void writeInt32(int32_t value);

    /**
     * @brief 写入 uint32_t 类型的数据；采用压缩技术保存
     * 
     * @param value 
     */
    void writeUint32(uint32_t value);

    /**
     * @brief 写入 int64 类型的数据；采用压缩技术保存
     * 
     * @param value 
     */
    void writeInt64(int64_t value);

    /**
     * @brief 写入 uint64 类型的数据；采用压缩技术保存
     * 
     * @param value 
     */
    void writeUint64(uint64_t value);

    /**
     * @brief 写入 float 类型的数据；以 uint32_t 类型保存数据；非压缩技术
     * 
     * @param value 
     */
    void writeFloat(float value);

    /**
     * @brief 写入 double 类型的数据；以 uint64_t 类型保存数据；非压缩技术
     * 
     * @param value 
     */
    void writeDouble(double value);

    /**
     * @brief 写入 string类型的数据，以uint16_t 类型保存 string的长度
     * 
     * @param value 
     */
    void writeStringF16(const std::string &value);

    /**
     * @brief 写入 string类型的数据，以 uint32_t 类型保存 string的长度
     * 
     * @param value 
     */
    void writeStringF32(const std::string &value);

    /**
     * @brief 写入 string类型的数据，以 uint64_t 类型保存 string的长度
     * 
     * @param value 
     */
    void writeStringF64(const std::string &value);

    /**
     * @brief 写入 string类型的数据，使用压缩技术以 uint64_t 类型保存 string的长度；
     * 
     * @param value 
     */
    void writeStringVint(const std::string &value);

    /**
     * @brief 写入 std::string类型的数据，无长度
     * 
     * @param value 
     */
    void writeStringWithoutLength(const std::string &value);

    /**
     * @brief 读取固定长度为 int8_t 类型的数据
     * 
     * @param value 
     */
    int8_t readFint8();

    /**
     * @brief 读取固定长度为 uint8_t 类型的数据
     * 
     * @param value 
     */
    uint8_t readFuint8();

    /**
     * @brief 读取固定长度为 int16_t 类型的数据
     * 
     * @param value 
     */
    int16_t readFint16();

    /**
     * @brief 读取固定长度为 uint16_t 类型的数据
     * 
     * @param value 
     */
    uint16_t readFuint16();

    /**
     * @brief 读取固定长度为 int32_t 类型的数据
     * 
     * @param value 
     */
    int32_t readFint32();

    /**
     * @brief 读取固定长度为 uint32_t 类型的数据
     * 
     * @param value 
     */
    uint32_t readFuint32();

    /**
     * @brief 读取固定长度为 int64_t 类型的数据
     * 
     * @param value 
     */
    int64_t readFint64();

    /**
     * @brief 读取固定长度为 uint64_t 类型的数据
     * 
     * @param value 
     */
    uint64_t readFuint64();

    /**
     * @brief 读取 int32_t 类型的数据；采用压缩技术 解压缩数据
     * 
     * @param value 
     */
    int32_t readInt32();

    /**
     * @brief 读取 uint32_t 类型的数据；采用压缩技术 解压缩数据
     * 
     * @param value 
     */
    uint32_t readUint32();

    /**
     * @brief 读取 int64_t 类型的数据；采用压缩技术 解压缩数据
     * 
     * @param value 
     */
    int64_t readInt64();

    /**
     * @brief 读取 uin64_t 类型的数据；采用压缩技术 解压缩数据
     * 
     * @param value 
     */
    uint64_t readUint64();

    /**
     * @brief 读取 float类型的数据；以读取 uint32_t类型数据为基础
     * 
     * @param value 
     */
    float readFloat();

    /**
     * @brief 读取 double类型的数据；以读取 uint64_t类型数据为基础
     * 
     * @param value 
     */
    double readDouble();

    /**
     * @brief 读取 string类型的数据，使用 uint16_t保存数据
     * 
     * @param value 
     */
    std::string readStringF16();

    /**
     * @brief 读取 string类型的数据，使用 uint32_t保存数据
     * 
     * @param value 
     */
    std::string readStringF32();

    /**
     * @brief 读取 string类型的数据，使用 uint64_t保存数据
     * 
     * @param value 
     */
    std::string readStringF64();

    /**
     * @brief 读取 string类型的数据，以读取 readUint64为基础，解压缩字符串的长度
     * 
     * @param value 
     */
    std::string readStringVint();

    /**
     * @brief 清空ByteArray；只保留 m_root 所指向的内存块，其他的内存块全部删除
     * m_position  m_size  m_capacity 重置
     */
    void clear();

    /**
     * @brief 将 size 长度的数据写入当前内存块中
     * 
     * @param buf 内存缓存指针； 指向待写入的数据
     * @param size 数据大小
     */
    void write(const void *buf, size_t size);

    /**
     * @brief 从当前位置开始，读取内存块中的数据 长度为size
     * m_position会更新位置
     * @param buf 指定数据缓存位置 -- 传出参数
     * @param size 指定读取的数据长度，如果超过可读数据长度，抛出异常
     */
    void read(void *buf, size_t size);

    /**
     * @brief 从指定位置开始，读取内存中的数据 长度为size
     * m_position不会更新位置
     * @param buf 指定数据缓存位置 -- 传出参数0
     * @param size 指定读取的数据长度，如果超过可读数据长度，抛出异常
     * @param position 可以是指定的任意位置
     */
    void read(void *buf, size_t size, size_t position) const;

    /**
     * @brief 返回ByteArray当前的位置
     * 
     * @return size_t 
     */
    size_t getPosition() const {
        return m_position;
    }

    /**
     * @brief 设置ByteArray当前的位置
     * 
     * @param size 
     */
    void setPosition(size_t position);

    /**
     * @brief 将 ByteArray中的数据写入到文件中
     * 
     * @param name 
     * @return true 
     * @return false 
     */
    bool writeToFile(const std::string &name) const;

    /**
     * @brief 将文件中的数据读入到 ByteArray中
     * 
     * @param name 
     * @return true 
     * @return false 
     */
    bool readFromFile(const std::string &name);

    /**
     * @brief 获取 ByteAarray的内存块大小
     * 
     * @return size_t 
     */
    size_t getBaseSize() const {
        return m_baseSize;
    }

    /**
     * @brief 获取可读数据的长度
     * 数据总长 - 当前操作的读位置
     * @return size_t 
     */
    size_t getReadSize() const {
        return m_size - m_position;
    }

    /**
     * @brief 设置的网络字节序是否是小端
     * 
     * @return true 是小端
     * @return false 
     */
    bool isLittleEndian() const {
        return m_endian == WEBS_LITTLE_ENDIAN;
    }

    /**
     * @brief 设置网络字节序为小端字节序
     * 
     * @param value 
     * @return true 设置网络字节序为小端字节序
     * @return false 
     */
    void setIsLittleEndian(bool value) {
        m_endian = value ? WEBS_LITTLE_ENDIAN : WEBS_BIG_ENDIAN;
    }

    /**
     * @brief 将 ByteArray里面的数据 [m_position, m_size) 转成std::string
     * 
     * @return std::string 
     */
    std::string toString() const;

    /**
     * @brief 将 ByteArray里面的数据 [m_position, m_size) 转为16进制的 std::string
     * 
     * @return std::string 
     */
    std::string toHexString() const;

    /**
     * @brief 获取可读数据的缓存，保存成iovec格式；从当前位置开始计算可读数据
     * 
     * @param buffers 传出参数
     * @param len 指定获取可读数据的长度；len = len < getReadSize() ? getReadSize() : len;
     */
    uint64_t getReadBuffer(std::vector<iovec> &buffers, uint64_t len = ~0ull) const;

    /**
     * @brief 获取可读数据的缓存，保存成iovec格式；从指定位置开始计算可读数据
     * 
     * @param buffers 传出参数
     * @param len 指定获取可读数据的长度；len = len < getReadSize() ? getReadSize() : len;
     * @param position 指定位置
     * @return uint64_t 
     */
    uint64_t getReadBuffer(std::vector<iovec> &buffers, uint64_t len, uint64_t position) const;

    /**
     * @brief 获取可写入的缓存，使用iovec格式保存
     * 
     * @param buffers 传出参数
     * @param len 指定获取多长的写入缓存；如果当前容量(m_capacity)不足，将会扩容 -- 所以函数不能是 const
     * @return uint64_t 返回实际的长度
     */
    uint64_t getWriteBuffer(std::vector<iovec> &buffers, uint64_t len);

    /**
     * @brief 返回当前保存的数据长度
     * 
     * @return size_t 
     */
    size_t getSize() const {
        return m_size;
    }

private:
    // 与内存分配有关的函数
    /**
     * @brief 扩容 ByteArray，使其可以容纳size个数据
     * m_capacity = m_capacity > size ? m_capacity : size
     * @param size 
     */
    void addCapacity(size_t size);

    /**
     * @brief 获取当前可写入的数据容量
     * 
     * @return uint64_t 
     */
    uint64_t getCapacity() const {
        return m_capacity - m_position;
    };

private:
    // 内存块的大小
    size_t m_baseSize;
    // 当前操作位置  -- 意味着从当前位置开始读写
    size_t m_position;
    // 当前的总容量
    size_t m_capacity;
    // 当前数据的大小
    size_t m_size;
    // 网络字节序；默认是大端
    int8_t m_endian;
    // 第一个内存块指针
    Node *m_root;
    // 当前操作的内存块指针
    Node *m_cur;
};
} // namespace webs

#endif