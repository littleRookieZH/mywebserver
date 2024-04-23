#include "../../webs/test_cmake/hello.h"
#include <iostream>
#include <thread>

// 线程函数，参数是一个整数
void threadFunction(int id) {
    std::cout << "Thread " << id << " is running" << std::endl;
}

int main() {
    // 创建一个线程并启动，参数是线程函数和函数的参数
    std::thread t(threadFunction, 1);

    // 等待线程结束
    t.join();

    return 0;
}
