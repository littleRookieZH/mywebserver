#include <iostream>

class Derived;
class Base {
 public:
  // 纯虚函数，使 Base 成为抽象类
  virtual void abstractFunction() = 0;

  // 非抽象函数
  void nonAbstractFunction();
  virtual ~Base() {}
};

class Derived : public Base {
 public:
  // 实现了抽象函数
      void abstractFunction() override {
           std::cout << "Derived::abstractFunction() called" << std::endl;
  }
};
class Derived2 : public Derived {
 public:
  // 实现了抽象函数
  void abstractFunction() override {
    std::cout << "Derived::abstractFunction() called" << std::endl;
  }
};

void Base::nonAbstractFunction() {
  Derived *d = new Derived;
  // abstractFunction();
  std::cout << "Base::nonAbstractFunction() called" << std::endl;
}

int main() {
  // Derived d;
  // d.abstractFunction();    // 输出: Derived::abstractFunction() called
  // d.nonAbstractFunction(); // 输出: Base::nonAbstractFunction() called

  return 0;
}
