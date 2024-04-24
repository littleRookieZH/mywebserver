#ifndef __WEBS_TIMER_H__
#define __WEBS_TIMER_H__
#include <memory>
#include <vector>
#include <functional>
#include <set>
#include "../util_module/mutex.h"

namespace webs {
class TimerManager;
class Timer : public std::enable_shared_from_this<Timer> {
    friend class TimerManager;

public:
    typedef std::shared_ptr<Timer> ptr;

    /* 取消定时器 */
    bool cancel();

    /* 刷新定时器 */
    bool refresh();

    /* 重置定时器 */
    bool reset(uint64_t ms, bool from_now);

private:
    /* 构造函数；定时器执行时间间隔、回调函数、是否循环、定时器管理 */
    Timer(uint64_t ms, std::function<void()> cb, bool recurring, TimerManager *manage);

    /* 构造函数；执行的时间戳 */
    Timer(uint64_t next);

private:
    // 是否循环定时器
    bool m_recurring = false;
    // 执行周期
    uint64_t m_ms = 0;
    // 精确的执行时间
    uint64_t m_next = 0;
    // 回调函数
    std::function<void()> m_cb;
    // 定时器管理器
    TimerManager *m_manager = nullptr;

private:
    /* 定时器的比较仿函数 */
    struct Comparator {
        /* 比较两个定时器智能指针的大小 */
        bool operator()(const Timer::ptr &lhs, const Timer::ptr &rhs) const;
    };
};

class TimerManager {
    friend class Timer;

public:
    typedef RWMutex RWMutexType;

    /* 构造函数 */
    TimerManager();

    /* 虚析构函数 */
    virtual ~TimerManager();

    // 添加定时器
    Timer::ptr addTimer(uint64_t ms, std::function<void()> cb, bool recurring);

    // 添加带条件的定时器
    Timer::ptr addConditionTimer(uint64_t ms, std::function<void()> cb, std::weak_ptr<void> weak_cond, bool recurring = false);

    /* 到最近一个定时器执行的时间间隔 */
    uint64_t getNextTimer();

    /* 获取需要执行的定时器的回调函数列表 */
    void listExpiredCb(std::vector<std::function<void()>> &cbs);

    /* 是否有定时器 */
    bool hasTimer();

protected:
    /* 当有新的定时器插入到定时器的首部，执行该函数 */
    virtual void onTimerInsertedAtFront() = 0;

    /* 将定时器添加到管理器中 */
    void addTimer(Timer::ptr val, RWMutexType::WriteLock &lock);

private:
    /* 检测服务器时间是否被调后了 */
    bool detectClockRollover(uint64_t now_ms);

private:
    RWMutexType m_mutex;
    // 定时器集合
    std::set<Timer::ptr, Timer::Comparator> m_timers;
    // 是否触发onTimerInsertedAtFront
    bool m_tickled = false;
    // 上次执行时间
    uint64_t m_previousTime = 0;
};
} // namespace webs

#endif