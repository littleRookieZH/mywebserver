
#include "../util/mutex.h"
#include <memory>
#include <functional>
namespace webs{
    class Thread : Noncopyable{
        public:
            typedef std::shared_ptr<Thread> ptr;
            static const std::string &GetName();

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
}