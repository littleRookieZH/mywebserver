#include "fiber.h"
#include "../log_module/log.h"
#include "../config/config.h"
#include "../util/macro.h"
#include <atomic>
namespace webs {

// 创建日志信息
webs::Logger::ptr g_logger = WEBS_LOG_NAME("system");
// 创建原子变量
static std::atomic<uint64_t> s_fiber_id{0};
static std::atomic<uint64_t> s_fiber_count{0};
// 创建线程的协程的智能指针对象和协程的指针
static thread_local Fiber *t_fiber = nullptr;
static thread_local Fiber::ptr t_threadFiber = nullptr;
// 获取协程运行时栈大小的配置信息
webs::ConfigVar<uint32_t>::ptr g_fiber_stack_size = Config::Lookup<uint32_t>("fiber.stack_size", 128 * 1024, "fiber stack size");

/* 动态分配内存的类 */
class MallocStackAllocator {
public:
    static void *Alloc(size_t size) {
        return malloc(size);
    }
    static void Delloc(void *vp, size_t size) {
        return free(vp);
    }
};

// 重命名
using StackAllocator = MallocStackAllocator;

// 私有构造
Fiber::Fiber() {
    // 修改状态，设置当前的线程的协程指针(t_fiber)
    m_state = EXEC;
    Fiber::SetThis(this);
    // 保存当前协程的上下文
    if (getcontext(&m_ctx)) {
        WEBS_ASSERT2(false, "getcontext");
    }
    // 增加协程计数
    ++s_fiber_count;
    // 将 "Fiber::Fiber main" 输出到debug日志
    WEBS_LOG_DEBUG(g_logger) << "Fiber::Fiber main";
}
} // namespace webs