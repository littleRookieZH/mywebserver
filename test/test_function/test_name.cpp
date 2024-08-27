#include <iostream>
#include <string>

class Base {
public:
    Base(std::string name) :
        name(name) {
    }

    std::string getName() const {
        return name;
    }

protected:
    std::string name;
};

class Derived : public Base {
public:
    Derived(std::string name) :
        Base(name) {
    }
    std::string getName() const {
        return name;
    }

private:
    std::string name;
};

int main() {
    Derived d1("xxx");
    std::cout << "Derived name = " << d1.getName() << std::endl;
    std::cout << "Base name = " << d1.Base::getName() << std::endl;
    return 0;
}