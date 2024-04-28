#include <iostream>
#include <memory>
#include <set>

class Derivde;
class MyClass : public std::enable_shared_from_this<MyClass> {
    friend class Derivde;

public:
    typedef std::shared_ptr<MyClass> ptr;
    MyClass() {
        std::cout << "MyClass constructed." << std::endl;
    }

    ~MyClass() {
        std::cout << "MyClass destructed." << std::endl;
    }
    void func();
    Derivde *p;
};

class Derivde {
    friend class MyClass;

public:
    std::set<MyClass::ptr> d_set;
};

void MyClass::func() {
    auto it = p->d_set.find(shared_from_this());
    p->d_set.erase(it);
    p->d_set.insert(shared_from_this());
}
int main() {
    std::shared_ptr<MyClass> ptr1(new MyClass());
    Derivde de;
    ptr1->p = &de;
    de.d_set.insert(ptr1);
    ptr1->func();
    // ptr1.reset(); // Release one shared_ptr

    return 0;
}
