/**
 * 配置模块
 */
#ifndef __WEBS_CONFIG_H__
#define __WEBS_CONFIG_H__
#include <yaml-cpp/yaml.h>
#include <boost/lexical_cast.hpp>
#include <algorithm>
#include <unordered_set>
#include "../util_module/mutex.h"
#include "../util_module/util.h"
#include "env.h"
#include "../log_module/log.h"

namespace webs {

class ConfigVarBase {
public:
    typedef std::shared_ptr<ConfigVarBase> ptr;
    ConfigVarBase(const std::string &name, const std::string &description = "") :
        m_name(name), m_description(description) {
        // 将名字全部转为小写
        std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);
    }

    virtual ~ConfigVarBase() {
    }

    /* 返回配置参数的名称 */
    const std::string &getName() const {
        return m_name;
    }

    /* 返回配置参数的描述*/
    const std::string &getDescription() const {
        return m_description;
    }

    /* 转成字符串 */
    virtual std::string toString() = 0;

    /* 从字符串初始化值 */
    virtual bool fromString(const std::string &) = 0;

    /* 返回配置参数的类型名称 */
    virtual std::string getTypeName() const = 0;

    // 子类可用
protected:
    // 配置参数的名称
    std::string m_name;
    // 配置参数的描述
    std::string m_description;
};

/**
     * 类型转换模板 (F是源， T是目标)
     *
     */
template <class F, class T>
class LexicalCast {
public:
    T operator()(const F &v) {
        //  可以将一个数据类型转换为另一个数据类型
        return boost::lexical_cast<T>(v);
    }
};

/* 偏特化： string 转为 vector<T> */
template <class T>
class LexicalCast<std::string, std::vector<T>> {
public:
    std::vector<T> operator()(const std::string &v) {
        YAML::Node node = YAML::Load(v);
        typename std::vector<T> vc;
        std::stringstream ss;
        for (size_t i = 0; i < node.size(); ++i) {
            ss.str(""); // 将流的内容清空，不可以使用ss.clear -- 清空的是错误状态
            ss << node[i];
            vc.push_back(LexicalCast<std::string, T>()(ss.str()));
        }
        return vc;
    }
};

/* 偏特化： vector<T> 转为 string*/
template <class T>
class LexicalCast<std::vector<T>, std::string> {
public:
    std::string operator()(const std::vector<T> &v) {
        YAML::Node node(YAML::NodeType::Sequence);
        for (auto &i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

/* 偏特化： string 转为 list<T> */
template <class T>
class LexicalCast<std::string, std::list<T>> {
public:
    std::list<T> operator()(const std::string &v) {
        YAML::Node node(YAML::Load(v));
        typename std::list<T> lt;
        // 将 node 中的数据转为 string
        std::stringstream ss;
        for (size_t i = 0; i < node.size(); ++i) {
            ss.str("");
            ss << node[i];
            lt.push_back(LexicalCast<std::string, T>()(ss.str()));
        }
        return lt;
    }
};

/* 偏特化： list<T> 转为 string */
template <class T>
class LexicalCast<std::list<T>, std::string> {
public:
    std::string operator()(const std::list<T> &v) {
        YAML::Node node(YAML::NodeType::Sequence);

        for (auto &i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

/* 偏特化： string 转为 set<T> */
template <class T>
class LexicalCast<std::string, std::set<T>> {
public:
    std::set<T> operator()(const std::string &v) {
        YAML::Node node = YAML::Load(v);
        typename std::set<T> st;
        std::stringstream ss;
        for (size_t i = 0; i < node.size(); ++i) {
            ss.str("");
            ss << node[i];
            st.insert(LexicalCast<std::string, T>()(ss.str()));
        }
        return st;
    }
};

/* 偏特化： set<T> 转为 string */
template <class T>
class LexicalCast<std::set<T>, std::string> {
public:
    std::string operator()(const std::set<T> &v) {
        YAML::Node node(YAML::NodeType::Sequence);
        for (auto &i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

/* 偏特化： string 转为 unordered_set<T> */
template <class T>
class LexicalCast<std::string, std::unordered_set<T>> {
public:
    std::unordered_set<T> operator()(const std::string &v) {
        YAML::Node node = YAML::Load(v);
        typename std::unordered_set<T> ust;
        std::stringstream ss;
        for (size_t i = 0; i < node.size(); ++i) {
            ss.str("");
            ss << node[i];
            ust.insert(LexicalCast<std::string, T>()(ss.str()));
        }
        return ust;
    }
};

/* 偏特化： unordered_set<T> 转为 string */
template <class T>
class LexicalCast<std::unordered_set<T>, std::string> {
public:
    std::string operator()(const std::unordered_set<T> &v) {
        YAML::Node node(YAML::NodeType::Sequence);
        for (auto &i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

/* 偏特化： string 转为 map<std::string, T> */
template <class T>
class LexicalCast<std::string, std::map<std::string, T>> {
public:
    std::map<std::string, T> operator()(const std::string &v) {
        YAML::Node node = YAML::Load(v);
        typename std::map<std::string, T> mp;
        std::stringstream ss;
        for (auto it = node.begin(); it != node.end(); ++it) {
            ss.str("");
            ss << it->second;
            // Scalar()获取标量值，还需要将 node[i] 的值转为string
            mp.insert(std::make_pair(it->first.Scalar(), LexicalCast<std::string, T>()(ss.str())));
        }
        return mp;
    }
};

/* 偏特化： map<std::string, T> 转为  string*/
template <class T>
class LexicalCast<std::map<std::string, T>, std::string> {
public:
    std::string operator()(const std::map<std::string, T> &v) {
        YAML::Node node(YAML::NodeType::Map);
        for (auto &i : v) {
            node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

/* 偏特化： string 转为 unordered_map<T> */
template <class T>
class LexicalCast<std::string, std::unordered_map<std::string, T>> {
public:
    std::unordered_map<std::string, T> operator()(const std::string &v) {
        YAML::Node node = YAML::Load(v);
        typename std::unordered_map<std::string, T> ump;
        std::stringstream ss;
        for (auto it = node.begin(); it != node.end(); ++it) {
            ss.str("");
            ss << it->second;
            ump.insert(std::make_pair(it->first.Scalar(), LexicalCast<std::string, T>()(ss.str())));
        }
        return ump;
    }
};

/* 偏特化： unordered_map<T> 转为 string */
template <class T>
class LexicalCast<std::unordered_map<std::string, T>, std::string> {
public:
    std::string operator()(const std::unordered_map<std::string, T> &v) {
        YAML::Node node(YAML::NodeType::Map);
        for (auto &i : v) {
            node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

/**
     * 通过参数模板子类，保存对应类型的参数值
     * FromStr 从std::string 转换成 T类型的仿函数
     * ToStr 从T转换成 std::string 的仿函数
     * std::string 为YAML格式的字符串
     *
     */
template <class T, class FromStr = LexicalCast<std::string, T>, class ToStr = LexicalCast<T, std::string>>
class ConfigVar : public ConfigVarBase {
public:
    typedef RWMutex RWMutexType;
    typedef std::shared_ptr<ConfigVar> ptr;
    typedef std::function<void(const T &old_value, const T &new_value)> on_change_cb;

    /**
         * 通过参数名，参数值，描述构造ConfigVar
         * 参数名称有效字符：[0-9a-z_.]
         *
         */
    ConfigVar(const std::string &name, const T &default_value, const std::string &description = "") :
        ConfigVarBase(name, description), m_val(default_value) {
    }
    ~ConfigVar() {
    }

    /* 将参数转换为YAML String */
    std::string toString() override {
        try {
            RWMutexType::ReadLock lock(m_mutex);
            return ToStr()(m_val);
        } catch (std::exception &e) {
            WEBS_LOG_ERROR(WEBS_LOG_ROOT()) << " ConfigVar::toString exception " << e.what() << " convert: " << TypeToName<T>() << " to string"
                                            << " name = " << m_name;
        }
        return "";
    }

    /* 将YAML String转成 参数的值 -- 意味着还需要使用setValue */
    bool fromString(const std::string &s) override {
        try {
            setValue(FromStr()(s));
            return true;
        } catch (std::exception &e) {
            // what() 获取异常的描述信息
            WEBS_LOG_ERROR(WEBS_LOG_ROOT()) << "ConfigVal::fromString exception " << e.what() << " convert: string to " << TypeToName<T>()
                                            << " name = " << m_name << " - " << s;
        }
        return false;
    }

    /* 获取当前参数的值 */
    const T getValue() {
        RWMutexType::ReadLock lock(m_mutex);
        return m_val;
    }

    /* 设置当前参数的值 */
    void setValue(const T &v) {
        // 读取时使用读锁
        {
            RWMutexType::WriteLock lock(m_mutex);
            if (m_val == v) {
                return;
            }
            for (auto &i : m_cbs) {
                i.second(m_val, v);
            }
        }
        // 修改时使用写锁
        RWMutexType::WriteLock lock(m_mutex);
        m_val = v;
    }

    /* 返回参数值的类型名称,这两个修饰没有顺序关系 */
    std::string getTypeName() const override {
        return TypeToName<T>();
    }

    /* 添加变化回调函数 */
    uint64_t addListener(on_change_cb cb) {
        static uint64_t s_fun_id = 0;
        RWMutexType::WriteLock lock(m_mutex);
        ++s_fun_id;
        m_cbs[s_fun_id] = cb;
        return s_fun_id;
    }

    /* 删除回调函数 */
    void delListener(uint64_t key) {
        RWMutexType::WriteLock lock(m_mutex);
        m_cbs.erase(key);
    }

    /* 获取回调函数 */
    on_change_cb getListener(uint64_t key) {
        RWMutexType::ReadLock lock(m_mutex);
        auto it = m_cbs.find(key);
        if (it != m_cbs.end()) {
            return it->second;
        }
        return nullptr;
    }

    /* 清理所有的回调函数 */
    void clearListener() {
        RWMutexType::WriteLock lock(m_mutex);
        m_cbs.clear();
    }

private:
    RWMutexType m_mutex;
    T m_val;
    // 变更回调函数组，uint64_t key，要求唯一，一般可以用hash
    std::map<uint64_t, on_change_cb> m_cbs;
};

/**
     * ConfigVal 的管理类
     * 提供便捷的方法创建/访问ConfigVal
     *
     */
class Config {
public:
    typedef std::unordered_map<std::string, ConfigVarBase::ptr> ConfigVarMap;
    typedef RWMutex RWMutexType;

    /**
         * 获取或者创建对应参数名的配置参数
         */
    template <class T>
    static typename ConfigVar<T>::ptr Lookup(const std::string &name, const T &default_value, const std::string &description = "") {
        // 加锁
        RWMutexType::WriteLock lock(GetMutex());
        auto it = GetDatas().find(name);
        // 如果name存在，检查type是否正确，不正确输出错误信息到日志中
        if (it != GetDatas().end()) {
            // 智能指针的安全向下转换
            auto tmp = std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
            if (tmp) {
                WEBS_LOG_INFO(WEBS_LOG_ROOT()) << " Lookup name = " << name << " exists";
                return tmp;
            } else {
                WEBS_LOG_ERROR(WEBS_LOG_ROOT()) << "Lookup name = " << name << " exists but type not " << TypeToName<T>() << " real_type = " << it->second->getTypeName()
                                                << " " << it->second->toString();
                return nullptr;
            }
        }
        // 如果name不存在，检查name是否合法，如果不合法，输出错误信息到日志中
        if (name.find_first_not_of("abcdefghikjlmnopqrstuvwxyz._0123456789") != std::string::npos) {
            WEBS_LOG_ERROR(WEBS_LOG_ROOT()) << " Lookup name invalid " << name;
            // 无效参数异常
            throw std::invalid_argument(name);
        }
        // name不存在且合法，创建配置参数
        typename ConfigVar<T>::ptr v(new ConfigVar<T>(name, default_value, description));
        GetDatas()[name] = v;
        return v;
    }

    /* 查找配置参数 */
    template <class T>
    static typename ConfigVar<T>::ptr Lookup(const std::string &name) {
        RWMutexType::ReadLock lock(GetMutex());
        auto it = GetDatas().find(name);
        if (it == GetDatas().end()) {
            return nullptr;
        }
        return std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
    }

    /* 使用YAML::Node 初始化配置模块 */
    static void LoadFromYaml(const YAML::Node &node);
    /* 加载path文件夹里面的配置文件 */
    static void LoadFromConfDir(const std::string &path, bool force = false);
    /* 查找配置参数，返回配置参数的基类 */
    static ConfigVarBase::ptr LookupBase(const std::string &name);
    /* 遍历配置模块里面所有的配置项:传入一个函数形式为：void(ConfigBase::ptr) 函数，将配置项作为参数传入该函数 */
    static void Visit(std::function<void(ConfigVarBase::ptr)> cb);

private:
    /* 返回所有配置项 */
    static ConfigVarMap &GetDatas() {
        static ConfigVarMap s_datas;
        return s_datas;
    }

    /* 配置项的锁 */
    static RWMutexType &GetMutex() {
        static RWMutexType s_mutex;
        return s_mutex;
    }
};
} // namespace webs
#endif