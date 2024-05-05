#include <iostream>

int main() {
    int i = 0;
retry:
    if (i < 3) {
        std::cout << "i = " << i << std::endl;
        i++;
        goto retry; // 跳转到标签 retry 处
    }
    return 0;
}
