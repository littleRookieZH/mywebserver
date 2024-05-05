#ifndef __FD_MANAGER_H__
#define __FD_MANAGER_H__

#include <memory>
#include <vector>

#include "../util_module/mutex.h"
#include "../util_module/singleton.h"

/* 文件描述符管理类 */

namespace webs {
class FdCtx : public std::enable_shared_from_this<FdCtx> {
public:
    typedef std::shared_ptr<FdCtx> ptr;
    /* 构造函数 */
    FdCtx(int fd);

    /* 析构函数 */
    ~FdCtx();

    /* 是否初始化 */
    bool isInit() const {
        return m_isInit;
    }

    /* 是否为socket */
    bool isSocket() const {
        return m_socket;
    }

    /* 是否关闭 */
    bool isClose() const {
        return m_isClosed;
    }

    /* 是否用户主动设置非堵塞 */
    void setUserNonblock(bool v) {
        m_userNonblock = v;
    }

    /* 获取用户设置 */
    bool getUserNonblock() const {
        return m_userNonblock;
    }

    /* 设置系统非阻塞*/
    void setSysNonblock(bool v) {
        m_sysNonblock = v;
    }

    /* 获取系统非阻塞*/
    bool getSysNonblock() {
        return m_sysNonblock;
    }

    /* 设置超时时间 */
    void setTimeout(int type, uint64_t v);

    /* 获取超时时间 */
    uint64_t getTimeout(int type);

private:
    bool init();

private:
    // 是否初始化
    bool m_isInit : 1;
    // 是否为socket
    bool m_socket : 1;
    // 是否hook
    bool m_sysNonblock : 1;
    // 是否用户主动设置非堵塞
    bool m_userNonblock : 1;
    // 是否关闭
    bool m_isClosed : 1;
    // 文件描述符
    int m_fd;
    // 读超时时间
    uint64_t m_recvTimeout;
    // 写超时时间
    uint64_t m_sendTimeout;
};

class FdManager {
public:
    typedef RWMutex RWMutexType;
    FdManager();
    /* 获取 / 创建文件描述符；如果不存在，auto_create = true，则自动创建 */
    FdCtx::ptr get(int fd, bool auto_create = false);

    /* 删除文件描述符 */
    void del(int fd);

private:
    // 读写锁
    RWMutexType m_mutex;
    // 文件描述符集合
    std::vector<FdCtx::ptr> m_datas;
};

typedef Singleton<FdManager> FdMgr;

} // namespace webs

#endif