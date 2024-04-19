#include <iostream>
#include <cassert>
#if defined __GNUC__ || defined __llvm__
/// LIKCLY 宏的封装, 告诉编译器优化,条件大概率成立；两次取反确定bool值
#define SYLAR_LIKELY(x) __builtin_expect(!!(x), 1)
/// LIKCLY 宏的封装, 告诉编译器优化,条件大概率不成立
#define SYLAR_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define SYLAR_LIKELY(x) (x)
#define SYLAR_UNLIKELY(x) (x)
#endif

/// 断言宏封装
#define SYLAR_ASSERT(x)                \
    if (SYLAR_UNLIKELY(!(x))) {        \
        std::cout << "ASSERTION: " #x  \
                  << "\nbacktrace:\n"; \
        assert(x);                     \
    }

int main() {
    int x = 10;
    SYLAR_ASSERT(x == 20); // 正确的断言，不会触发输出

    int y = 20;
    SYLAR_ASSERT(y == 10); // 错误的断言，会触发输出

    return 0;
}