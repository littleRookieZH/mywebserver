#include <cstdint>
#include <iostream>

static uint32_t EncodeZigzag32(const int32_t &v) { // 正负数转换
    if (v < 0) {
        return ((uint32_t)(-v)) * 2 - 1;
    } else {
        return v * 2;
    }
}

static uint32_t Encode(const int32_t &x) {
    std::cout << (x << 1) << std::endl;
    std::cout << (x >> 31) << std::endl;
    return (x << 1) ^ (x >> 31);
}

int main() {
    int a = -7;
    std::cout << EncodeZigzag32(a) << std::endl;
    std::cout << Encode(a) << std::endl;
}