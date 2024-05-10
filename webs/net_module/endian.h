/* 字节序操作 */
#ifndef __WEBS_ENDIAN_H__
#define __WEBS_ENDIAN_H__

#include <type_traits>
#include <stdint.h>
#include <byteswap.h>

#define WEBS_LITTLE_ENDIAN 1
#define WEBS_BIG_ENDIAN 2

namespace webs {
// 64 -- 32 --16 位的字节转换
template <typename T>
typename std::enable_if<sizeof(uint64_t) == sizeof(T), T>::type
byteswap(T value) {
    return (T)bswap_64((uint64_t)value);
}

template <class T>
typename std::enable_if<sizeof(T) == sizeof(uint32_t), T>::type
byteswap(T value) {
    return (T)bswap_32((uint32_t)value);
}

template <typename T>
typename std::enable_if<sizeof(uint16_t) == sizeof(T), T>::type
byteswap(T value) {
    return (T)bswap_16((uint16_t)value);
}

#if BYTE_ORDER == BIG_ENDIAN
#define WEBS_BYTE_ORDER WEBS_BIG_ENDIAN
#else
#define WEBS_BYTE_ORDER WEBS_LITTLE_ENDIAN
#endif

#if WEBS_BYTE_ORDER == WEBS_BIG_ENDIAN // 主机是大端字节序

template <typename T>
T byteswapOnLittleEndian(T t) { // 网络字节序本身为大端：主机是大端，所以什么都不做
    return t;
}

template <typename T>
T byteswapOnBigEndian(T t) { // 不明确设计目的
    return byteswap(t);
}

#else
template <class T>
T byteswapOnLittleEndian(T t) { // 网络字节序本身为大端：主机是小端，所以需要转换字节序
    return byteswap(t);
}

template <typename T>
T byteswapOnBigEndian(T t) { // 不明确设计目的
    return t;
}
// sdfsfsfsdf
#endif

} // namespace webs

#endif