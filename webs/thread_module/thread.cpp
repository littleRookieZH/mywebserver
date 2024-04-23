// #include "thread.h"
#include "../log_module/log.h"

namespace webs {
// 使用 thread_local 关键字声明的变量，每个线程都会拥有一份独立的副本，而不同线程间的变量值互不影响。
static thread_local Thread *t_thread = nullptr; // 工作线程
static thread_local std::string t_thread_name = "UNKNOW";
static webs::Logger::ptr g_logger = WEBS_LOG_NAME("system");

/* 获取当前线程的指针 */

Thread *Thread::GetThis() {
    return t_thread;
}
/* 获取线程名字 */
const std::string &Thread::GetName() {
    return t_thread_name;
}

/* 设置线程的名字 */
void Thread::SetName(const std::string &name) {
    if (name.empty()) {
        return;
    }
    if (t_thread) {
        t_thread->m_name = name;
    }
    t_thread_name = name;
}

Thread::Thread(std::function<void()> cb, const std::string &name) :
    m_cb(cb), m_name(name) {
    WEBS_LOG_DEBUG(g_logger) << "pthread_nmae = " << name;
    // 确保name不为空
    if (m_name.empty()) {
        m_name = "UNKNOW";
    }
    // 创建线程
    int rt = pthread_create(&m_thread, NULL, Thread::run, this);
    if (rt) {
        WEBS_LOG_ERROR(g_logger) << "pthread_create fail, rt = " << rt << " nmae = " << name;
        throw std::logic_error(" pthread_create error");
    }
    // 阻塞线程，直到设置线程运行函数后，允许线程继续执行
    m_semaphore.wait();
}

Thread::~Thread() {
    if (m_thread) {
        pthread_detach(m_thread);
    }
}

void Thread::join() {
    if (m_thread) {
        int rt = pthread_join(m_thread, NULL);
        if (rt) {
            WEBS_LOG_ERROR(g_logger) << "pthread_join thread fail rt = " << rt << "name = " << m_name;
            throw std::logic_error("pthread_join error");
        }
        m_thread = 0;
    }
}

void *Thread::run(void *arg) {
    Thread *thread = (Thread *)arg;
    // 设置工作线程
    t_thread = thread;
    t_thread_name = thread->m_name;
    thread->m_id = webs::GetThreadId();
    pthread_setname_np(thread->m_thread, t_thread_name.substr(0, 15).c_str());
    std::function<void()> cb;
    cb.swap(thread->m_cb);
    // 释放信号量
    thread->m_semaphore.notify();
    cb();
    return 0;
}

} // namespace webs