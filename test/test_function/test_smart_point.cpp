#include <memory>

class A {
};

int main() {
    std::shared_ptr<int> sharedPtr1(new int(42));

    std::weak_ptr<int> weakPtr(sharedPtr1);

    // 从 weakPtr 创建 sharedPtr2
    std::shared_ptr<int> sharedPtr2 = weakPtr.lock();

    // 使用 sharedPtr2 创建 sharedPtr3
    std::shared_ptr<int> sharedPtr3(sharedPtr2);

    const int *ptr = new int(42);
    delete ptr;
    const A *ptr1 = new A;
    // std::shared_ptr<A> sharedPtr1(ptr1);
    delete ptr1;
    // 使用 const 对象创建 shared_ptr
    // std::shared_ptr<const int> sharedPtr(ptr);
    return 0;
}
