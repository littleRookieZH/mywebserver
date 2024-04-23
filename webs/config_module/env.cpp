#include "env.h"
#include <unistd.h>
#include <string.h>
#include <iomanip>
#include "../log_module/log.h"
#include "config.h"

namespace webs {
static webs::Logger::ptr g_logger = WEBS_LOG_NAME("system");
bool Env::init(int argc, char **argv) {
    // 读取链接文件的内容
    char link[1024] = {0};
    char path[1024] = {0};
    // "/proc/%d/exe"是linux系统自动生成的当前进程的可执行文件路径
    sprintf(link, "/proc/%d/exe", getpid());
    // 读取link符号链接所指向的路径
    assert((readlink(link, path, sizeof(link)) != -1));
    // 读取可执行文件、当前工作目录、程序名称
    m_exe = path;
    auto pos = m_exe.find_last_of("/");
    m_cwd = m_exe.substr(0, pos) + "/";
    m_program = argv[0];
    // 读取 m_args 信息
    // 这些配置关系是一一对应的，字符'-'后面是key，紧着的argv[i]是value
    // -config /path/to/config -file xxxx -d
    const char *new_pos = nullptr;
    for (int i = 1; i < argc; ++i) {
        if (argv[i][0] == '-') {
            // 如果argv[i]的长度大于1，说明 '-' 之后还有参数，否则输出错误信息
            if (strlen(argv[i]) > 1) {
                if (new_pos) {
                    add(new_pos, ""); // 连续两个 -XX -XXX
                }
                new_pos = argv[i] + 1;
            } else {
                WEBS_LOG_ERROR(g_logger) << "Invaild arg idx = " << i << " val = " << argv[i];
                return false;
            }
        } else {
            if (new_pos) {
                add(new_pos, argv[i]);
                new_pos = nullptr;
            } else {
                WEBS_LOG_ERROR(g_logger) << "Invaild arg idx = " << i << " val = " << argv[i];
                return false;
            }
        }
    }
    // 最后一个是 -XX 的情况
    if (new_pos) {
        add(new_pos, "");
    }
    return true;
}

/* 增 */
void Env::add(const std::string &key, const std::string &val) {
    RWMutexType::WriteLock lock(m_mutex);
    m_args[key] = val;
}

/* 查 */
bool Env::has(const std::string &key) {
    RWMutexType::ReadLock lock(m_mutex);
    return m_args.find(key) == m_args.end() ? false : true;
}

/* 删 */
void Env::del(const std::string &key) {
    RWMutexType::ReadLock lock(m_mutex);
    // erase 成功返回1，不存在返回0
    m_args.erase(key);
}

/* 获取value，如果不存在返回输入的默认值 */
std::string Env::get(const std::string &key, const std::string &default_value) {
    RWMutexType::WriteLock lock(m_mutex);
    auto it = m_args.find(key);
    return it != m_args.end() ? it->second : default_value;
}

/* 先删除原数据，在添加*/
void Env::addHelp(const std::string &key, const std::string &desc) {
    removeHelp(key);
    RWMutexType::WriteLock lock(m_mutex);
    m_helps.push_back(std::pair<std::string, std::string>(key, desc));
}

void Env::removeHelp(const std::string &key) {
    RWMutexType::WriteLock lock(m_mutex);

    for (auto it = m_helps.begin(); it != m_helps.end(); ++it) {
        if (it->first == key) {
            it = m_helps.erase(it);
        } else {
            ++it;
        }
    }
}

void Env::printHelp() {
    RWMutexType::ReadLock lock(m_mutex);
    std::cout << "Usage: " << m_program << " [options]" << std::endl;
    for (auto &i : m_helps) { // setw设置字段宽度
        std::cout << std::setw(5) << "-" << i.first << " : " << i.second << std::endl;
    }
}

bool Env::setEnv(const std::string &key, const std::string &val) {
    return !setenv(key.c_str(), val.c_str(), 1);
}
std::string Env::getEnv(const std::string &key, const std::string &default_value) {
    const char *p = getenv(key.c_str());
    if (p == nullptr) {
        return default_value;
    }
    return p;
}

/* 如果传入空路径，则返回根路径；
    如果传入的路径已经是绝对路径，则直接返回；
    如果传入的是相对路径，则将当前工作目录与相对路径拼接起来，得到完整的绝对路径。没有校验路径，在这里默认是有效的路径*/
std::string Env::getAbsolutePath(const std::string &path) const {
    if (path.empty()) {
        return "/";
    }
    if (path[0] == '/') {
        return path;
    }
    return m_cwd + path;
}

std::string Env::getAbsoluteWorkPath(const std::string &path) const {
    if (path.empty()) {
        return "/";
    }
    if (path[0] == '/') {
        return path;
    }
    // 这里默认work_path是有效的，不为空
    static webs::ConfigVar<std::string>::ptr g_server_work_path =
        webs::Config::Lookup<std::string>("server.work_path");
    return g_server_work_path->getValue() + "/" + path;
}
/* 返回配置文件的路径 */
std::string Env::getConfigPath() {
    return getAbsolutePath(get("c", "conf"));
}
} // namespace webs