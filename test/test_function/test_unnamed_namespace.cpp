#include <iostream>

namespace {

struct _SSLInit {
    _SSLInit() {
        std::cout << "SSL initialization..." << std::endl;
    }
    int func() {
        return 20;
    }
};

static _SSLInit s_init;

} // namespace

// static int s_init = 10;// 同时存在会出现歧义

// 访问匿名命名空间中的变量的函数
int getSSLInitValue() {
    return s_init.func();
}

int main() {
    std::cout << "Value of s_init: " << s_init.func() << std::endl;
    std::cout << "Value of _SSLInit in anonymous namespace: " << getSSLInitValue() << std::endl;
    return 0;
}
