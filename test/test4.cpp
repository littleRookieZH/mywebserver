#include <memory>
#include <iostream>
class student;
static thread_local student *st1 = nullptr;

class student : public std::enable_shared_from_this<student> {
public:
    typedef std::shared_ptr<student> ptr;
    student(int a) :
        a(a) {
    }
    student() {
        std::cout << "fdsfsf" << std::endl;
        a = 20;
    }
    void setStudent(int b) {
        a = b;
    }
    void getStudent() {
        std::cout << "aaaaa" << std::endl;
    }
    ~student() {
        std::cout << "dsfsf" << std::endl;
    }
    static void func();

private:
    static int b;
    int a;
    enum { C = 4,
           D = 5 };
};
int student::b = 10;
void student::func() {
    // if (a > 10) { // 不行，非静态成员
    //     std::cout << "hhh";
    // }
    if (D > 10) { // 可以，编译器就确定的常量
        std::cout << "hhh";
    }
}

class tea {
public:
    tea() {
        std::cout << "3333" << std::endl;
    }

private:
    std::string s;
    int c;
    student::ptr st3;
};

int main() {
    // student *p = new student(12);
    // // student::ptr cur(new student);
    // student::ptr cur(p);
    student::ptr s2;
    // student::ptr cur1 = p->shared_from_this();
    // student *arwcur = cur.get();
    // cur.reset(new student(40));
    tea t1;
    std::cout
        << &t1 << std::endl;
    // arwcur->setStudent(30);
    // st1->getStudent();
    // delete p;
    // delete p;
    return 0;
}