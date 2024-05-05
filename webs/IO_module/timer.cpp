#include "timer.h"
#include "../util_module/util.h"
#include "../log_module/log.h"

namespace webs {

static webs::Logger::ptr g_logger = WEBS_LOG_NAME("system");

/* 定时器的比较仿函数 */

/* 比较两个定时器智能指针的大小 */
bool Timer::Comparator::operator()(const Timer::ptr &lhs, const Timer::ptr &rhs) const {
    if (!lhs && !rhs) {
        return false;
    }
    if (!lhs) {
        return true;
    }
    if (!rhs) {
        return false;
    }
    if (lhs->m_next < rhs->m_next) {
        return true;
    }
    if (rhs->m_next < lhs->m_next) {
        return false;
    }
    return lhs.get() < rhs.get();
}

/* 构造函数；定时器执行时间间隔、回调函数、是否循环、定时器管理 */
Timer::Timer(uint64_t ms, std::function<void()> cb, bool recurring, TimerManager *manage) :
    m_recurring(recurring),
    m_ms(ms),
    m_cb(cb),
    m_manager(manage) {
    m_next = webs::GetCurrentMS() + ms;
}

/* 构造函数；执行的时间戳 */
Timer::Timer(uint64_t next) :
    m_next(next) {
}

/* 取消定时器; cb置空、IOManager删除对应定时器 */
bool Timer::cancel() {
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if (m_cb) {
        m_cb = nullptr;
        auto it = m_manager->m_timers.find(shared_from_this());
        m_manager->m_timers.erase(it);
        return true;
    }
    return false;
}

/* 刷新定时器；要先删除再添加 */
bool Timer::refresh() {
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if (!m_cb) {
        return false;
    }
    auto it = m_manager->m_timers.find(shared_from_this());
    if (it == m_manager->m_timers.end()) {
        return false;
    }
    m_manager->m_timers.erase(it);
    m_next = webs::GetCurrentMS() + m_ms;
    m_manager->m_timers.insert(shared_from_this());
    return true;
}

/* 重置定时器；要先删除再添加;ms:执行的周期；from_now:是否从现在开始 */
bool Timer::reset(uint64_t ms, bool from_now) {
    if (ms == m_ms && !from_now) {
        return true;
    }
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if (!m_cb) {
        return false;
    }
    auto it = m_manager->m_timers.find(shared_from_this());
    if (it == m_manager->m_timers.end()) {
        return false;
    }
    m_manager->m_timers.erase(it); // 先移除再添加
    uint64_t starttime;
    if (from_now) {
        starttime = webs::GetCurrentMS();
    } else {
        starttime = m_next - m_ms;
    }
    m_next = starttime + ms;
    m_ms = ms;
    m_manager->addTimer(shared_from_this(), lock);
    return true;
}

/* 构造函数 */
TimerManager::TimerManager() :
    m_previousTime(webs::GetCurrentMS()) {
}

TimerManager::~TimerManager() {
}

// 添加定时器
Timer::ptr TimerManager::addTimer(uint64_t ms, std::function<void()> cb, bool recurring) {
    Timer::ptr newTimer(new Timer(ms, cb, recurring, this));
    RWMutexType::WriteLock lock(m_mutex);
    addTimer(newTimer, lock);
    return newTimer;
}

static void OnTime(std::function<void()> cb, std::weak_ptr<void> weak_cond) {
    std::shared_ptr<void> p = weak_cond.lock();
    if (p) {
        cb();
    }
}

// 插入定时器 --> 如果是队首，判断是否需要执行对应的函数
void TimerManager::addTimer(Timer::ptr val, RWMutexType::WriteLock &lock) {
    auto it = m_timers.insert(val).first;
    bool at_front = it == m_timers.begin() && !m_tickled;
    if (at_front) {
        m_tickled = true;
    }
    lock.unlock();
    if (at_front) {
        onTimerInsertedAtFront();
    }
}

// 添加带条件的定时器
Timer::ptr TimerManager::addConditionTimer(uint64_t ms, std::function<void()> cb, std::weak_ptr<void> weak_cond, bool recurring) {
    return addTimer(ms, std::bind(&OnTime, cb, weak_cond), recurring);
}

/* 到最近一个定时器执行的时间间隔 */
uint64_t TimerManager::getNextTimer() {
    RWMutexType::ReadLock lock(m_mutex);
    m_tickled = false;
    if (m_timers.empty()) {
        return ~0ull; // 返回特定值
    }
    const Timer::ptr &next = *m_timers.begin();
    uint64_t nowtime = webs::GetCurrentMS();
    if (nowtime >= next->m_next) {
        return 0; // 本来应该执行的定时器不知道什么原因没有执行
    } else {
        return next->m_next - nowtime; // 还剩多少时间
    }
}

/* 获取需要执行的定时器的回调函数列表 */
/**
 * 双重检查 --> 回拨检查 --> 确定需要执行的函数对象 --> 删除原对象 --> 传出cbs所需参数，并检查是否存在循环
*/
void TimerManager::listExpiredCb(std::vector<std::function<void()>> &cbs) {
    std::vector<Timer::ptr> expired;
    // 双重检查：读锁的开销比写锁小。因此先使用读锁检查，避免不必要的写锁
    {
        RWMutexType::ReadLock lock(m_mutex);
        if (m_timers.empty()) {
            return;
        }
    }
    RWMutexType::WriteLock lock(m_mutex);
    if (m_timers.empty()) {
        return;
    }

    uint64_t nowtime = webs::GetCurrentMS();
    bool rollback = detectClockRollover(nowtime);
    if (!rollback && (*m_timers.begin())->m_next > nowtime) {
        return;
    }

    Timer::ptr now_timer(new Timer(nowtime));
    auto it = rollback ? m_timers.end() : m_timers.lower_bound(now_timer);
    while (it != m_timers.end() && (*it)->m_next == nowtime) {
        ++it;
    }

    expired.insert(expired.begin(), m_timers.begin(), it);
    m_timers.erase(m_timers.begin(), it);
    cbs.reserve(expired.size());
    for (auto &i : expired) {
        cbs.push_back(i->m_cb);
        if (i->m_recurring) {
            i->m_next = nowtime + i->m_ms;
            m_timers.insert(i);
        } else {
            i->m_cb = nullptr;
        }
    }
}

/* 检测服务器时间是否发生了回拨；并更新上次执行时间 */
bool TimerManager::detectClockRollover(uint64_t now_ms) {
    bool rollover = false;
    if (now_ms < (m_previousTime - 60 * 1000 * 1000)) {
        rollover = true;
    }
    m_previousTime = now_ms;
    return rollover;
}

/* 是否有定时器 */
bool TimerManager::hasTimer() {
    RWMutexType::ReadLock lock(m_mutex);
    return !m_timers.empty();
}

} // namespace webs