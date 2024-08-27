#include "fd_manager.h"
#include "hook.h"

#include <sys/stat.h>
#include <sys/types.h>

namespace webs {

/* 构造函数 */
FdCtx::FdCtx(int fd) :
    m_isInit(false),
    m_socket(false),
    m_sysNonblock(false),
    m_userNonblock(false),
    m_isClosed(false),
    m_fd(fd),
    m_recvTimeout(-1),
    m_sendTimeout(-1) {
    init();
}

FdCtx::~FdCtx() {
}

bool FdCtx::init() {
    if (m_isInit) {
        return true;
    }
    m_recvTimeout = -1;
    m_sendTimeout = -1;

    // 确定文件描述是否为socket
    struct stat st;
    if (fstat(m_fd, &st) == -1) {
        m_isInit = false;
        m_socket = false;
    } else {
        m_isInit = true;
        m_socket = S_ISSOCK(st.st_mode); // 用于检查给定的文件类型是否为套接字
    }

    // 默认socket使用hook
    if (m_socket) {
        int flags = fcntl_f(m_fd, F_GETFL, 0);
        if (!(flags & O_NONBLOCK)) {
            fcntl_f(m_fd, F_SETFL, flags | O_NONBLOCK); // 是socket则设置为非堵塞
        }
        m_sysNonblock = true;
    } else {
        m_sysNonblock = false;
    }

    m_userNonblock = false;
    m_isClosed = false;
    return m_isInit;
}

void FdCtx::setTimeout(int type, uint64_t v) {
    if (type == SO_RCVTIMEO) {
        m_recvTimeout = v;
    } else {
        m_sendTimeout = v;
    }
}

/* type: SO_SNDTIMEO 或 SO_RCVTIMEO */
uint64_t FdCtx::getTimeout(int type) {
    if (type == SO_RCVTIMEO) {
        return m_recvTimeout;
    } else {
        return m_sendTimeout;
    }
}

FdManager::FdManager() {
    m_datas.resize(64);
}

FdCtx::ptr FdManager::get(int fd, bool auto_create) {
    if (fd < 0) {
        return nullptr;
    }

    // 读锁读数据
    RWMutexType::ReadLock lock(m_mutex);
    if ((int)m_datas.size() <= fd) { // 强制转换的优先级 低于 .
        if (!auto_create) {
            return nullptr;
        }
    } else {
        if (m_datas[fd] || !auto_create) { // 已经存在 或者 不存在并不用创建
            return m_datas[fd];
        }
    }
    lock.unlock();

    // 写锁写数据
    RWMutexType::WriteLock lock1(m_mutex);
    FdCtx::ptr ctx(new FdCtx(fd));
    if (fd >= (int)m_datas.size()) {
        m_datas.resize(fd * 1.5);
    }
    m_datas[fd] = ctx;
    return ctx;
}

/* 删除文件描述符 */
void FdManager::del(int fd) {
    RWMutexType::WriteLock lock(m_mutex);
    if ((int)m_datas.size() <= fd) {
        return;
    }
    m_datas[fd].reset();
}

} // namespace webs