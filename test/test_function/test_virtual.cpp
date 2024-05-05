#include <iostream>

// 定义类 C
class C {
public:
    C() {
        std::cout << "Constructor of class C" << std::endl;
    }

    ~C() {
        std::cout << "Destructor of class C" << std::endl;
    }
    virtual void fun(int a = 10) {
        std::cout << "a = " << a << std::endl;
    }
};

// 定义类 B，继承自类 C
class B : public C {
public:
    B() {
        std::cout << "Constructor of class B" << std::endl;
    }

    ~B() {
        std::cout << "Destructor of class B" << std::endl;
    }
    virtual void fun(int a) {
        std::cout << "a = " << a << std::endl;
    }
};

// 定义类 A，继承自类 B
class A : public B {
public:
    A() {
        std::cout << "Constructor of class A" << std::endl;
    }

    ~A() {
        std::cout << "Destructor of class A" << std::endl;
    }
    virtual void fun(int a) {
        std::cout << "a = " << a << std::endl;
    }
};

int main() {
    A a; // 创建类 A 的对象
    C *b = new B;
    b->fun();
    return 0;
}
