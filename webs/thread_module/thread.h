#ifndef __WEBS_THREAD_H__
#define __WEBS_THREAD_H__

#include <functional>
#include <memory>

#include "../util/mutex.h"
namespace webs {
class Thread : Noncopyable {
public:
    typedef std::shared_ptr<Thread> ptr;
    /// @brief
    /// @param cb
    /// @param name
    Thread(std::function<void()> cb, const std::string &name);
    ~Thread();

    /* 获取线程ID */
    pid_t getId() const {
        return m_id;
    }
    /* 获取线程的名字 */
    const std::string &getName() const {
        return m_name;
    };
    /* 获取当前线程的指针 */
    static Thread *GetThis();
    /* 获取线程名字 */
    static const std::string &GetName();
    /* 等待线程完成 */
    void join();
    /* 设置线程的名字 */
    static void SetName(const std::string &name);

private:
    /* 线程执行函数 */
    static void *run(void *arg);

private:
    // 线程ID
    pid_t m_id = -1;
    // 线程结构
    pthread_t m_thread = 0;
    // 线程执行函数
    std::function<void()> m_cb;
    // 线程名字
    std::string m_name;
    // 信号量
    Semaphore m_semaphore;
};
} // namespace webs
#endif