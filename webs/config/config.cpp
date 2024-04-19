#include "config.h"
#include <sys/stat.h>

namespace webs {

static webs::Logger::ptr g_logger = WEBS_LOG_NAME("system");
/* 查找配置参数，返回配置参数的基类 */
ConfigVarBase::ptr Config::LookupBase(const std::string &name) {
    RWMutexType::ReadLock lock(GetMutex());
    auto it = GetDatas().find(name);
    if (it != GetDatas().end()) {
        return it->second;
    }
    return nullptr;
}

/* 输出node中的map类型的键值对，key使用 . 进行拼接查找下一级的node */
static void ListAllMember(const std::string &prefix, const YAML::Node node, std::list<std::pair<std::string, const YAML::Node>> &output) {
    // 检查key是否符合命名规则
    if (prefix.find_first_not_of("0123456789._abcdefghijklmnopqrstuvwxyz") != std::string::npos) {
        WEBS_LOG_ERROR(g_logger) << " Config invaild name: " << prefix << " : " << node;
        return;
    }

    // 如果符合命名规则，根据node是否是map类型执行判断是否需要递归
    output.push_back(std::make_pair(prefix, node));
    if (node.IsMap()) {
        for (auto &it : node) {
            std::string tmp = prefix.empty() ? it.first.Scalar() : prefix + "." + it.first.Scalar();
            ListAllMember(tmp, it.second, output);
        }
    }
}

/* 使用YAML::Node 初始化配置模块；查找node不上锁没有关系，因为之后的查找、修改配置项的函数是上锁的 */
void Config::LoadFromYaml(const YAML::Node &node) {
    // ListAllMember 获取node中的信息
    std::list<std::pair<std::string, const YAML::Node>> output;
    ListAllMember("", node, output);
    // 如果 node_key 在配置项中，则将 value(string)保存到配置项中
    for (auto &i : output) {
        std::string key = i.first;
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        if (key.empty()) {
            continue;
        }
        ConfigVarBase::ptr val = LookupBase(key); // 查找key是否存在
        if (!val) {
            if (i.second.IsScalar()) { // 使用基类指针调用虚函数。使用 Scalar()将node数据转化为string
                val->fromString(i.second.Scalar());
            } else { // 非标量需要将 node 转为字符串
                std::stringstream ss;
                ss << i.second;
                val->fromString(ss.str());
            }
        }
    }
}

/* 保存配置文件的修改时间 */
static std::map<std::string, uint64_t> s_file2modifytime;
// 创建一把静态互斥锁：为什么需要创建静态锁
static webs::Mutex s_mutex;
/* 加载path文件夹里面的配置文件 */
void Config::LoadFromConfDir(const std::string &path, bool force) {
    // 获取配置文件的路径
    std::string path_tmp = webs::EnvMgr::GetInstance().getAbsolutePath(path);
    // 读取路径下的 .yml配置文件
    std::vector<std::string> files;
    FSUtil::ListAllFile(files, path_tmp, ".yml");

    // 更新文件修改时间，并加载配置项
    for (auto &i : files) {
        {
            struct stat st;
            lstat(i.c_str(), &st);
            // 必须使用一把新锁：如果和 LoadFromYaml 函数使用同一把锁，continue之后会导致死锁。
            webs::Mutex::Lock lock(s_mutex);
            // 非强制修改，需要进一步判断修改时间
            if (!force && s_file2modifytime[i.c_str()] == st.st_mtime) {
                continue;
            }
            s_file2modifytime[i.c_str()] = st.st_mtime;
        }
        // 从文件中读取并设置配置项
        try {
            YAML::Node node = YAML::LoadFile(i);
            LoadFromYaml(node);
            WEBS_LOG_INFO(g_logger) << "LoadConfFile file = " << i << " ok";
        } catch (...) // 捕获任何一种异常类型
        {
            WEBS_LOG_INFO(g_logger) << " LoadConfFile file = " << i << " failed";
        }
    }
}

/* 遍历配置模块里面所有的配置项:传入一个函数形式为：void(ConfigBase::ptr) 函数，将配置项作为参数传入该函数 */
void Config::Visit(std::function<void(ConfigVarBase::ptr)> cb) {
    RWMutexType::ReadLock lock(GetMutex());
    ConfigVarMap s_datas = GetDatas();
    for (auto &i : s_datas) {
        cb(i.second);
    }
}
} // namespace webs