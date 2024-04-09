/**
 * 配置模块
 */
#ifndef _WEBS_CONFIG_H_
#define _WEBS_CONFIG_H_
#include <yaml-cpp/yaml.h>
#include <boost/lexical_cast.hpp>
#include <algorithm>
#include <unordered_set>
namespace webs
{

    class ConfigVarBase
    {
    public:
        ConfigVarBase(const std::string &name, const std::string &description = "") : m_name(name), m_description(description)
        {
            // 将名字全部转为小写
            std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        }

        virtual ~ConfigVarBase()
        {
        }

        /* 返回配置参数的名称 */
        const std::string &getname() const
        {
            return m_name;
        }

        /* 返回配置参数的描述*/
        const std::string &getdescription() const
        {
            return m_description;
        }

        /* 转成字符串 */
        virtual std::string toString() = 0;

        /* 从字符串初始化值 */
        virtual bool fromString(const std::string &) = 0;

        /* 返回配置参数的类型名称 */
        virtual std::string getTypeName() const = 0;

    private:
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
    class LexicalCast
    {
        T operator()(const F &v)
        {
            //  可以将一个数据类型转换为另一个数据类型
            return boost::lexical_cast<T>(v);
        }
    };

    /* 偏特化： string 转为 vector<T> */
    template <class T>
    class LexicalCast<std::string, std::vector<T>>
    {
        std::vector<T> operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            typename std::vector<T> vc;
            std::stringstream ss;
            for (size_t i = 0; i < node.size(); ++i)
            {
                ss.str(""); // 将流的内容清空，不可以使用ss.clear -- 清空的是错误状态
                ss << node[i];
                vc.push_back(LexicalCast<std::string, T>()(ss.str()));
            }
            return vc;
        }
    };

    /* 偏特化： vector<T> 转为 string*/
    template <class T>
    class LexicalCast<std::vector<T>, std::string>
    {
        std::string operator()(const std::vector<T> &v)
        {
            YAML::Node node(YAML::NodeType::Sequence);
            for (auto &i : v)
            {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    /* 偏特化： string 转为 list<T> */
    template <class T>
    class LexicalCast<std::string, std::list<T>>
    {
        std::list<T> operator()(const std::string &v)
        {
            YAML::Node node(YAML::Load(v));
            typename std::list<T> lt;
            // 将 node 中的数据转为 string
            std::stringstream ss;
            for (size_t i = 0; i < node.size(); ++i)
            {
                ss.str("");
                ss << node[i];
                lt.push_back(LexicalCast<std::string, T>()(ss.str()));
            }
            return lt;
        }
    };

    /* 偏特化： list<T> 转为 string */
    template <class T>
    class LexicalCast<std::list<T>, std::string>
    {
        std::string operator()(const std::list<T> &v)
        {
            YAML::Node node(YAML::NodeType::Sequence);

            for (auto &i : v)
            {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    /* 偏特化： string 转为 set<T> */
    template <class T>
    class LexicalCast
    {
        std::set<T> operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            typename std::set<T> st;
            std::stringstream ss;
            for(size_t i = 0;i < node.size();++i){
                ss.str("");
                ss << node[i];
                st.insert(LexicalCast<std::string, T>()(ss.str()));
                return st;
            }
        }
    };

    
    /* 偏特化： set<T> 转为 string */
    template <class T>
    class LexicalCast<std::string, std::set<T>>
    {
        std::string operator()(const std::set<T> &v)
        {
            YAML::Node node(YAML::NodeType::Sequence);
            for(auto &i: v){
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    /* 偏特化： string 转为 unordered_set<T> */
    template <class T>
    class LexicalCast<std::string, std::unordered_set<T>>
    {
        std::unordered_set<T> operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            typename std::unordered_set<t> ust;
            std::stringstream ss;
            for(size_t i = 0;i < node.size();++i){
                ss.str("");
                ss << node[i];
                ust.insert(LexicalCast<std::string, T>()(ss.str()));
            }
            return ust;
        }
    };

    /* 偏特化： unordered_set<T> 转为 string */
    template <class T>
    class LexicalCast
    {
        T operator()(const F &v)
        {
        
        }
    };

    /* 偏特化： string 转为 map<std::string, T> */
    template <class T>
    class LexicalCast<std::string, std::map<std::string, T>>
    {
        T operator()(const F &v)
        {
            
        }
    };

        /* 偏特化： map<T> 转为  string*/
    template <class T>
    class LexicalCast
    {
        T operator()(const F &v)
        {
            
        }
    };

    /* 偏特化： string 转为 unordered_map<T> */
    template <class T>
    class LexicalCast
    {
        T operator()(const F &v)
        {
            
        }
    };

    /* 偏特化： unordered_map<T> 转为 string */
    template <class T>
    class LexicalCast
    {
        T operator()(const F &v)
        {
        
        }
    };

}
#endif