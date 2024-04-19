#ifndef __WEBS_ENV_H__
#define __WEBS_ENV_H__
#include <map>
#include <vector>
#include "../util/mutex.h"
#include "../util/singleton.h"

namespace webs {
/* 这个类提供了一系列方法管理应用程序的环境配置和参数信息 */
class Env {
public:
    typedef RWMutex RWMutexType;
    bool init(int argc, char **argv);
    /* 增 */
    void add(const std::string &key, const std::string &val);
    /* 查 */
    bool has(const std::string &key);
    /* 删 */
    void del(const std::string &key);
    /* 改 */
    std::string get(const std::string &key, const std::string &default_value = "");

    void addHelp(const std::string &key, const std::string &desc);
    void removeHelp(const std::string &key);
    void printHelp();

    /* 返回可行性文件的路径 */
    const std::string &getExe() const {
        return m_exe;
    }
    /* 返回当前工作目录的路径 */
    const std::string &getCwd() const {
        return m_cwd;
    }

    bool setEnv(const std::string &key, const std::string &val);
    std::string getEnv(const std::string &key, const std::string &default_value = "");

    std::string getAbsolutePath(const std::string &path) const;
    std::string getAbsoluteWorkPath(const std::string &path) const;
    /* 返回配置文件的路径 */
    std::string getConfigPath();

private:
    RWMutexType m_mutex;
    std::map<std::string, std::string> m_args;
    std::vector<std::pair<std::string, std::string>> m_helps;

    /* 这三个参数只在初始化的时候进行了设置 */
    std::string m_program;
    // 可执行文件的路径
    std::string m_exe;
    // 当前工作目录的路径
    std::string m_cwd;
};
typedef webs::Singleton<Env> EnvMgr;
}; // namespace webs

#endif