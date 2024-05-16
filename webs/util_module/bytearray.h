/**
 * @brief 序列化与反序列化；
 * 1、用于网络中数据的传输 -- 消除主机字节序与网络字节序的差异
 * 2、提供统一的读写接口 -- 统一操作
 */
#ifndef __WEBS_BYTEARRAY_H__
#define __WEBS_BYTEARRAY_H__

#include <memory>
#include <stdint.h>

namespace webs {
class ByteArray {
    typedef std::shared_ptr<ByteArray> ptr;

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
    void writeFuin16(uint16_t value);

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
     * @brief 写入 string类型的数据，以uint16_t 类型保存数据
     * 
     * @param value 
     */
    void writeStringF16(const std::string &value);

    /**
     * @brief 写入 string类型的数据，以 uint32_t 类型保存数据
     * 
     * @param value 
     */
    void writeStringF32(const std::string &value);

    /**
     * @brief 写入 string类型的数据，以 uint64_t 类型保存数据
     * 
     * @param value 
     */
    void writeStringF64(const std::string &value);

    /**
     * @brief 写入 string类型的数据，以 uint64_t 类型保存数据；使用压缩技术
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
    void readFint8(int8_t value);

    /**
     * @brief 读取固定长度为 uint8_t 类型的数据
     * 
     * @param value 
     */
    void readFuint8(uint8_t value);

    /**
     * @brief 读取固定长度为 int16_t 类型的数据
     * 
     * @param value 
     */
    void readFint16(int16_t value);

    /**
     * @brief 读取固定长度为 uint16_t 类型的数据
     * 
     * @param value 
     */
    void readFuint16(uint16_t value);

    /**
     * @brief 读取固定长度为 int32_t 类型的数据
     * 
     * @param value 
     */
    void readFint32(int32_t value);

    /**
     * @brief 读取固定长度为 uint32_t 类型的数据
     * 
     * @param value 
     */
    void readFuint32(uint32_t value);

    /**
     * @brief 读取固定长度为 int64_t 类型的数据
     * 
     * @param value 
     */
    void readFint64(int64_t value);

    /**
     * @brief 读取固定长度为 uint64_t 类型的数据
     * 
     * @param value 
     */
    void readFuint64(uint64_t value);

    /**
     * @brief 读取 int32_t 类型的数据；采用压缩技术 解压缩数据
     * 
     * @param value 
     */
    void readInt32(int32_t value);

    /**
     * @brief 读取 uint32_t 类型的数据；采用压缩技术 解压缩数据
     * 
     * @param value 
     */
    void readUint32(uint32_t value);

    /**
     * @brief 读取 int64_t 类型的数据；采用压缩技术 解压缩数据
     * 
     * @param value 
     */
    void readInt64(int64_t value);

    /**
     * @brief 读取 uin64_t 类型的数据；采用压缩技术 解压缩数据
     * 
     * @param value 
     */
    void readUint64(uint64_t value);

    /**
     * @brief 读取 float类型的数据；以读取 uint32_t类型数据为基础
     * 
     * @param value 
     */
    void readFloat(float value);

    /**
     * @brief 读取 double类型的数据；以读取 uint64_t类型数据为基础
     * 
     * @param value 
     */
    void readDouble(double value);

    /**
     * @brief 读取 string类型的数据，使用 uint16_t保存数据
     * 
     * @param value 
     */
    void readStringF16(const std::string &value);

    /**
     * @brief 读取 string类型的数据，使用 uint32_t保存数据
     * 
     * @param value 
     */
    void readStringF32(const std::string &value);

    /**
     * @brief 读取 string类型的数据，使用 uint64_t保存数据
     * 
     * @param value 
     */
    void readStringF64(const std::string &value);

    /**
     * @brief 读取 string类型的数据，以读取 readUint64为基础，解压缩字符串的长度
     * 
     * @param value 
     */
    void readStringVint(const std::string &value);

    /**
     * @brief 清空ByteArray
     * 
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
     * @brief 从当前位置开始，将内存块中的数据读取 size长度
     * 
     * @param buf 
     * @param size 
     */
    void read(void *buf, size_t size);
};
} // namespace webs

#endif