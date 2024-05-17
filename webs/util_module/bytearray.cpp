#include "bytearray.h"
#include "../log_module/log.h"

#include <string.h>
#include <cmath>
#include <iomanip>

namespace webs {
static webs::Logger::ptr g_logger = WEBS_LOG_NAME("system");

ByteArray::Node::Node(size_t size) :
    ptr(new char[size]), next(nullptr), size(size) {
}

ByteArray::Node::Node() :
    ptr(nullptr), next(nullptr), size(0) {
}

/* 释放内存块 */
ByteArray::Node::~Node() {
    if (ptr) {
        delete[] ptr;
    }
}

ByteArray::ByteArray(size_t base_size) :
    m_baseSize(base_size),
    m_position(0),
    m_capacity(base_size),
    m_size(0),
    m_endian(WEBS_BIG_ENDIAN), // 默认是大端。如果是小端，使用setLittleEndian
    m_root(new Node(base_size)),
    m_cur(m_root) {
}

ByteArray::~ByteArray() {
    //删除Node节点中的数据
    Node *tmp = m_root;
    while (tmp) {
        m_cur = tmp;
        tmp = tmp->next;
        delete m_cur;
    }
}

/* 数据传输单位的固定长度为一个字节，因此不需要考虑字节序的问题 */
void ByteArray::writeFint8(int8_t value) {
    write(&value, sizeof(int8_t));
}

void ByteArray::writeFuint8(uint8_t value) {
    write(&value, sizeof(uint8_t));
}

/* 数据传输单位的固定长度为一个字节，因此不需要考虑字节序的问题 */
void ByteArray::writeFint16(int16_t value) {
    if (m_endian != WEBS_BYTE_ORDER) {
        byteswap(value);
    }
    write(&value, sizeof(int16_t));
}

void ByteArray::writeFuint16(uint16_t value) {
    if (m_endian != WEBS_BYTE_ORDER) {
        byteswap(value);
    }
    write(&value, sizeof(uint16_t));
}

void ByteArray::writeFint32(int32_t value) {
    if (m_endian != WEBS_BYTE_ORDER) {
        byteswap(value);
    }
    write(&value, sizeof(int32_t));
}

void ByteArray::writeFuint32(uint32_t value) {
    if (m_endian != WEBS_BYTE_ORDER) {
        byteswap(value);
    }
    write(&value, sizeof(uint32_t));
}

void ByteArray::writeFint64(int64_t value) {
    if (m_endian != WEBS_BYTE_ORDER) {
        byteswap(value);
    }
    write(&value, sizeof(int64_t));
}

void ByteArray::writeFuint64(uint64_t value) {
    if (m_endian != WEBS_BYTE_ORDER) {
        byteswap(value);
    }
    write(&value, sizeof(uint64_t));
}

/* 编码：将有符号数转化为无符号数；如果是正数，value << 1；如果是负数，结果相当于 (-value)*2 - 1 */
static uint32_t EncodeZigzag32(int32_t value) {
    return (value << 1) ^ (value >> 31);
}

static uint64_t EncodeZigzag64(const int64_t &value) {
    return (value << 1) ^ (value >> 63);
}

static int32_t DecodeZigzag32(uint32_t value) {
    return (value >> 1) ^ -(value & 1);
}

static int64_t DecodeZigzag64(const uint64_t &value) {
    return (value >> 1) ^ -(value & 1);
}

/* 编码 -- 转换字节序 -- 再写入数据；如果是先转换字节序，那么编码结果将会出错 */
void ByteArray::writeInt32(int32_t value) {
    writeUint32(EncodeZigzag32(value));
}

/* 使用 uint8_t数组 保存数据，根据数据的大小确定数组的长度 */
void ByteArray::writeUint32(uint32_t value) {
    uint8_t arr[5];
    uint8_t i = 0;
    while (value >= 0x80) {               // 如果数值比 0111 1111 大，说明还需要更多的字节来保存结果
        arr[i++] = (value & 0x7F) | 0x80; // & 0111 1111 计算有效数据的大小，| 1000 0000 指示还有数据
        value >> 7;
    }
    arr[i++] = value; // 最后一次需要单独加入
    write(arr, i);
}

void ByteArray::writeInt64(int64_t value) {
    writeUint64(EncodeZigzag64(value));
}

void ByteArray::writeUint64(uint64_t value) {
    uint8_t arr[10];
    uint8_t i = 0;
    while (value >= 0x80) {               // 如果数值比 0111 1111 大，说明还需要更多的字节来保存结果
        arr[i++] = (value & 0x7F) | 0x80; // & 0111 1111 计算有效数据的大小，| 1000 0000 指示还有数据
        value >> 7;
    }
    arr[i++] = value; // 最后一次需要单独加入
    write(arr, i);
}

void ByteArray::writeFloat(float value) {
    uint32_t tmp;
    memcpy(&tmp, &value, sizeof(value));
    writeFuint32(tmp);
}

void ByteArray::writeDouble(double value) {
    uint64_t tmp;
    memcpy(&tmp, &value, sizeof(value));
    writeFuint64(tmp);
}

/* 这几个string write版本的差异在于 以不同的方式写入string的长度 */
void ByteArray::writeStringF16(const std::string &value) {
    writeFuint16(value.size());
    write(value.c_str(), value.size());
}

void ByteArray::writeStringF32(const std::string &value) {
    writeFuint32(value.size());
    write(value.c_str(), value.size());
}

void ByteArray::writeStringF64(const std::string &value) {
    writeFuint64(value.size());
    write(value.c_str(), value.size());
}

void ByteArray::writeStringVint(const std::string &value) {
    writeFuint64(value.size());
    write(value.c_str(), value.size());
}

void ByteArray::writeStringWithoutLength(const std::string &value) {
    write(value.c_str(), value.size());
}

int8_t ByteArray::readFint8() {
    int8_t value;
    read(&value, sizeof(int8_t));
    return value;
}

uint8_t ByteArray::readFuint8() {
    uint8_t value;
    read(&value, sizeof(uint8_t));
    return value;
}

// 先读数据，再转换字节序
#define XX(type)                       \
    type value;                        \
    read(&value, sizeof(type));        \
    if (m_endian != WEBS_BYTE_ORDER) { \
        byteswap(value);               \
    }                                  \
    return value;

int16_t ByteArray::readFint16() {
    XX(int16_t);
}

uint16_t ByteArray::readFuint16() {
    XX(uint16_t);
}

int32_t ByteArray::readFint32() {
    XX(int32_t);
}

uint32_t ByteArray::readFuint32() {
    XX(uint32_t);
}

int64_t ByteArray::readFint64() {
    XX(int64_t);
}

uint64_t ByteArray::readFuint64() {
    XX(uint64_t);
}
#undef XX // 取消宏xx的定义

/* 先读数据 -- 转换字节序 -- 再解码 */
int32_t ByteArray::readInt32() {
    return DecodeZigzag32(readUint32());
}

uint32_t ByteArray::readUint32() { // 读了40位
    uint32_t value = 0;
    // 32 / 7 = 4; 也就是读了5次(0 -- 4)，每次读取8位
    for (size_t i = 0; i < 32; i += 7) { // 7位有效数据，第8位是标志位
        uint8_t tmp = readFuint8();      // 读取8位
        if (tmp < 0x80) {
            value |= (uint32_t)tmp << i;
            break;
        } else {
            value |= (uint32_t)(tmp & 0x7F) << i;
        }
    }
    return value;
}

int64_t ByteArray::readInt64() {
    return DecodeZigzag64(readUint64());
}

uint64_t ByteArray::readUint64() {
    uint64_t value = 0;
    for (size_t i = 0; i < 64; i += 7) {
        uint8_t tmp = readFuint8();
        if (tmp < 0x80) {
            value |= (uint64_t)tmp << i;
            break;
        } else {
            value |= (uint64_t)(tmp & 0x7F) << i;
        }
    }
    return value;
}

float ByteArray::readFloat() {
    float value;
    uint32_t tmp = readFuint32();
    memcpy(&value, &tmp, sizeof(tmp));
    return value;
}

double ByteArray::readDouble() {
    double value;
    uint64_t tmp = readFuint64();
    memcpy(&value, &tmp, sizeof(tmp));
    return value;
}

/* 这几个string版本的区别在于 获取string长度的方式不同 */
std::string ByteArray::readStringF16() {
    uint16_t len = readFuint16();
    std::string buff;
    buff.resize(len);
    read(&buff[0], (size_t)len); // 以一个字符大小作为处理单元，所以不能使用 &s
    return buff;
}

std::string ByteArray::readStringF32() {
    uint32_t len = readFuint32();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}

std::string ByteArray::readStringF64() {
    uint64_t len = readFuint64();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}

std::string ByteArray::readStringVint() {
    uint64_t len = readUint64();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}

/* 只保留 m_root 所指向的内存块，其他的内存块全部删除 */
void ByteArray::clear() {
    if (!m_root) {
        return;
    }
    m_position = m_size = 0;
    m_capacity = m_baseSize;
    Node *tmp = m_root->next;
    while (tmp) {
        m_cur = tmp;
        tmp = tmp->next;
        delete m_cur;
    }
    m_cur = m_root;
    m_root->next = nullptr;
}

/* 校验size -- 扩容 -- 将数据写入Node中的 */
void ByteArray::write(const void *buf, size_t size) {
    if (m_size == 0) {
        return;
    }
    addCapacity(size);
    size_t npos = m_position % m_baseSize; // 计算块内偏移量
    size_t ncap = m_cur->size - npos;      // 剩余容量
    size_t bpos = 0;                       //已经写入内存的字节数
    while (size > 0) {
        if (ncap >= size) {                                            // 当前余量充足
            memcpy(m_cur->ptr + npos, (const char *)buf + bpos, size); // (const char*)是有必要的，确保指针的加法不会出错（其结果与指针所指类型有关）
            m_position += size;
            if (ncap == size) {
                m_cur = m_cur->next;
            }
            bpos += size;
            size = 0;
        } else { // 容量不足
            memcpy(m_cur + npos, (const char *)buf + bpos, ncap);
            m_position += ncap;
            bpos += ncap;
            size -= ncap;
            m_cur = m_cur->next; // 更新内存块指向
            npos = 0;
            ncap = m_cur->size; // 更新剩余容量
        }
    }
    if (m_position > m_size) { // 当前位置大于当前数据的大小，说明需要修改数据的真实大小 --- m_size可以理解为vector的size
        m_size = m_position;
    }
}

void ByteArray::read(void *buf, size_t size) {
    if (size > getReadSize()) {
        throw std::out_of_range("not enough len");
    }

    size_t npos = m_position % m_baseSize; // 获取块内的偏移量
    size_t ncap = m_cur->size - npos;      // 余量
    size_t bpos = 0;                       // 当前读了多少个字节
    while (size > 0) {
        if (ncap >= size) { // 余量充足
            memcpy((char *)buf + bpos, m_cur->ptr + npos, size);
            m_position += size;
            if (ncap == size) { // 写满了
                m_cur = m_cur->next;
            }
            bpos += size;
            size = 0;
        } else { // 余量不足
            memcpy((char *)buf + bpos, m_cur->next + npos, ncap);
            m_position += ncap;
            bpos += ncap;
            size -= ncap;
            m_cur = m_cur->next;
            npos = 0;
            ncap = m_cur->size;
        }
    }
}

void ByteArray::read(void *buf, size_t size, size_t position) const {
    if (size > m_size - position) {
        throw std::out_of_range("not enough len");
    }
    size_t npos = position % m_baseSize; // 偏移量
    size_t ncap = m_baseSize - npos;     // 余量
    size_t bpos = 0;

    size_t count = position / m_baseSize;
    Node *tmp = m_root;
    while (count > 0) {
        tmp = tmp->next;
        --count;
    }
    while (size > 0) {
        if (ncap >= size) {
            memcpy((char *)buf + bpos, tmp->ptr + npos, size);
            position += size;
            bpos += size;
            size = 0;
        } else {
            memcpy((char *)buf, tmp->ptr + npos, ncap);
            position += ncap;
            size -= ncap;
            tmp = tmp->next;
            bpos += ncap;
            npos = 0;
            ncap = tmp->size;
        }
    }
}

void ByteArray::addCapacity(size_t size) {
    if (size == 0) {
        return;
    }
    uint64_t old_cap = getCapacity();
    if (old_cap >= size) { // 剩余容量大于需要扩容的容量
        return;
    }
    // ceil: 计算大于或等于给定数值的最小整数
    size_t count = ceil(1.0 * (size - old_cap) / m_baseSize); // 记录还需要多少个内存块
    Node *tmp = m_cur;
    while (tmp->next) {
        tmp = tmp->next;
    }

    Node *first = nullptr;
    for (size_t i = 0; i < count; ++i) {
        tmp->next = new Node(m_baseSize);
        tmp = tmp->next;
        if (!first) {
            first = tmp;
        }
        m_capacity += m_baseSize;
    }

    if (old_cap == 0) { //当前的内存块已经没有可以写入的容量 -- 更新m_cur
        m_cur = first;
    }
}

/* 根据位置修改当前操作的内存块 */
void ByteArray::setPosition(size_t position) {
    if (position > m_capacity) {
        throw std::out_of_range("set_position out of range");
    }
    m_position = position;
    if (m_position > m_size) {
        m_size = m_position;
    }
    // 确定m_cur的位置
    m_cur = m_root;
    while (position > m_baseSize) {
        m_cur = m_cur->next;
        position -= m_baseSize;
    }
    if (position == 0) {
        m_cur = m_cur->next;
    }
}

/* 以二进制的形式创建文件 -- 从当前操作位置开始将数据写入到文件中 */
bool ByteArray::writeToFile(const std::string &name) const {
    std::ofstream ofs;
    ofs.open(name, std::ios::trunc | std::ios::binary);
    if (!ofs) {
        WEBS_LOG_DEBUG(g_logger) << "writeToFile name = " << name << " error, errno = " << errno << "errstr = " << strerror(errno);
        return false;
    }
    Node *tmp = m_cur;
    size_t position = m_position;
    size_t size = getReadSize(); // 可读数据
    while (size > 0) {
        size_t diff = m_position % m_baseSize; // 偏移量
        size_t len = size > m_baseSize - diff ? m_baseSize - diff : size;
        ofs.write(tmp->ptr + diff, diff);
        size -= diff;
        tmp = tmp->next;
        position += len;
    }
    return true;
}

/* 以二进制格式打开文件 -- 创建读缓存区 -- 读取数据并写入 ByteArray对象管理的内存块中 */
bool ByteArray::readFromFile(const std::string &name) {
    std::ifstream ifs;
    ifs.open(name, std::ios::trunc);
    if (!ifs) {
        WEBS_LOG_DEBUG(g_logger);
        return false;
    }
    // char *ptr = new char[m_baseSize];
    // delete[] ptr;

    std::shared_ptr<char> buffer(new char[m_baseSize], [](char *ptr) -> void { delete[] ptr; });
    while (!ifs.eof()) {
        ifs.read(buffer.get(), m_baseSize);
        write(buffer.get(), ifs.gcount());
    }
}

std::string ByteArray::toString() const {
    size_t len = getReadSize();
    std::string buf;
    buf.resize(len);
    read(&buf[0], len, m_position);
    return buf;
}

/* 格式:FF FF FF 设置字符宽度，填充数字0 */
std::string ByteArray::toHexString() const {
    std::string info = toString();
    std::stringstream ss;
    for (size_t i = 0; i < info.size(); ++i) {
        if (i > 0 && i % 32 == 0) { // 每32个数换一行
            ss << std::endl;
        }
        ss << std::setw(2) << std::setfill('0') << std::hex
           << (int)(uint8_t)info[i];
        if (i != info.size() - 1) {
            ss << ' ';
        }
    }
    return ss.str();
}

/* 确定len的真实长度 -- 创建 iovec -- 修改iovec成员变量 */
uint64_t ByteArray::getReadBuffer(std::vector<iovec> &buffers, uint64_t len) const {
    len = len > getReadSize() ? getReadSize() : len;

    if (len == 0) {
        return 0;
    }

    uint64_t size = len; // 返回值
    struct iovec iov;
    size_t npos = m_position % m_baseSize; // 偏移量
    size_t ncap = m_cur->size - npos;      // 余量
    Node *tmp = m_cur;
    while (len > 0) { // 修改iovec中指向数据的指针即可
        if (ncap >= len) {
            iov.iov_base = m_cur->ptr + npos;
            iov.iov_len = len;
            len = 0;
        } else {
            iov.iov_base = m_cur->ptr + npos;
            iov.iov_len = ncap;
            tmp = tmp->next;
            ncap = tmp->size;
            len -= ncap;
            npos = 0;
        }
        buffers.push_back(iov);
    }
    return size;
}

/* 确定len的真实长度 -- 创建 iovec -- 修改iovec成员变量 */
uint64_t ByteArray::getReadBuffer(std::vector<iovec> &buffers, uint64_t position, uint64_t len) const {
    len = len > (m_size - position) ? len : (m_size - position);
    if (len == 0) {
        return 0;
    }
    uint64_t size = len;

    struct iovec iov;
    size_t npos = position % m_baseSize;
    size_t count = position / m_baseSize;
    Node *node = m_root;
    while (count > 0) {
        node = node->next;
        --count;
    }

    size_t ncap = node->size - npos;

    while (len > 0) {
        if (ncap >= len) {
            iov.iov_base = node->ptr + npos;
            iov.iov_len = len;
            len = 0;
        } else {
            iov.iov_base = node->ptr + npos;
            iov.iov_len = ncap;
            node = node->next;
            len -= ncap;
            npos = 0;
            ncap = node->size;
        }
        buffers.push_back(iov);
    }
    return size;
}

/* read和write的区别在于len的处理，write len会扩容 */
uint64_t ByteArray::getWriteBuffer(std::vector<iovec> &buffers, uint64_t len) {
    if (len == 0) {
        return 0;
    }
    addCapacity(len);
    uint64_t size = len;

    struct iovec iov;
    size_t npos = m_position % m_baseSize; // 偏移量
    size_t ncap = m_cur->size - npos;      // 余量
    Node *tmp = m_cur;
    while (len > 0) { // 修改iovec中指向数据的指针即可
        if (ncap >= len) {
            iov.iov_base = m_cur->ptr + npos;
            iov.iov_len = len;
            len = 0;
        } else {
            iov.iov_base = m_cur->ptr + npos;
            iov.iov_len = ncap;
            tmp = tmp->next;
            ncap = tmp->size;
            len -= ncap;
            npos = 0;
        }
        buffers.push_back(iov);
    }
    return size;
}
} // namespace webs