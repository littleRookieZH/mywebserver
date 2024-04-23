#include "../webs/util_module/mutex.h"
#include "../webs/util_module/Noncopyable.h"
#include <memory>
#include <iostream>

class student : public std::enable_shared_from_this<student>, webs::Noncopyable {
public:
    typedef std::shared_ptr<student> ptr;

private:
    int b;
};
class tt {
public:
    typedef webs::Mutex MutexType;
    void start();
    tt() {
        a = 0;
    }
    bool isexit() {
        return s1 == nullptr;
    }

private:
    int a = -1;
    /// Mutex
    MutexType m_mutex;
    student::ptr s1;
};

void tt::start() {
    MutexType::Lock lock(m_mutex);
    ++a;
    lock.unlock();
}

int main() {
    tt t1;
    std::cout << t1.isexit() << std::endl;
    t1.start();
}