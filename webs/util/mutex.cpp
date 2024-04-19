#include "mutex.h"

namespace webs {
Semaphore::Semaphore(uint32_t count) {
    if (sem_init(&m_semaphore, 0, count)) {
        throw std::logic_error("sem_init error");
    }
};
Semaphore::~Semaphore() {
    sem_destroy(&m_semaphore);
};
/* 获取信号量；阻塞信号量，直到大于0，此时信号量值 - 1，允许线程继续运行*/
void Semaphore::wait() {
    if (sem_wait(&m_semaphore)) {
        throw std::logic_error("sem_wait error");
    }
};
/* 释放信号量；信号量值 + 1 */
void Semaphore::notify() {
    if (sem_post(&m_semaphore)) {
        throw std::logic_error("sem_post error");
    }
};
} // namespace webs

// int main(){
//     return 0;
// }
