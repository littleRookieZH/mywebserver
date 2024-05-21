#ifndef __WEBS_TCP_SERVER_H__
#define __WEBS_TCP_SERVER_H__

#include <memory>
#include <vector>
#include <map>

#include "../config_module/config.h"
#include "socket.h"
#include "address.h"
#include "../IO_module/iomanager.h"

namespace webs {
struct TcpServerConf {
    typedef std::shared_ptr<TcpServerConf> ptr;

    std::vector<std::string> address;
    int keepalive = 0;
    int time_out = 60 * 1000 * 2;
    int ssl = 0;
    std::string id;

    // 服务器类型  http ws
    std::string type = "http";
    std::string name;
    std::string cert_file;
    std::string key_file;
    std::string accept_worker;
    std::string io_worker;
    std::string process_worker;
    std::map<std::string, std::string> args;

    bool isVaild() {
        return !address.empty();
    }

    bool operator==(const TcpServerConf &rhs) {
        return address == rhs.address && keepalive == rhs.keepalive && time_out == rhs.time_out
               && ssl == rhs.ssl && id == rhs.id && type == rhs.type && name == rhs.name && cert_file == rhs.cert_file
               && key_file == rhs.key_file && accept_worker == rhs.accept_worker && io_worker == rhs.io_worker
               && process_worker == rhs.process_worker && args == rhs.args;
    }
};

template <>
class LexicalCast<std::string, TcpServerConf> {
public:
    TcpServerConf operator()(const std::string &v) {
        YAML::Node node = YAML::Load(v);
        TcpServerConf conf;
        conf.keepalive = node["keepalive"].as<int>(conf.keepalive);
        conf.time_out = node["time_out"].as<int>(conf.time_out);
        conf.ssl = node["ssl"].as<int>(conf.ssl);
        conf.id = node["id"].as<std::string>(conf.id);
        conf.type = node["type"].as<std::string>(conf.type);
        conf.name = node["name"].as<std::string>(conf.name);
        conf.cert_file = node["cert_file"].as<std::string>(conf.cert_file);
        conf.key_file = node["key_file"].as<std::string>(conf.key_file);
        conf.accept_worker = node["accept_worker"].as<std::string>();
        conf.io_worker = node["io_worker"].as<std::string>();
        conf.process_worker = node["process_worker"].as<std::string>();
        conf.args = LexicalCast<std::string, std::map<std::string, std::string>>()(node["args"].as<std::string>("")); // 使用隐式转换将string转为map
        if (node["address"].IsDefined()) {
            for (size_t i = 0; i < node["address"].size(); ++i) {
                conf.address.push_back(node["address"].as<std::string>());
            }
        }
        return conf;
    }
};

template <>
class LexicalCast<TcpServerConf, std::string> {
public:
    std::string operator()(const TcpServerConf &conf) {
        YAML::Node node;
        node["keepalive"] = conf.keepalive;
        node["time_out"] = conf.time_out;
        node["ssl"] = conf.ssl;
        node["id"] = conf.id;
        node["type"] = conf.type;
        node["name"] = conf.name;
        node["cert_file"] = conf.cert_file;
        node["key_file"] = conf.key_file;
        node["accept_worker"] = conf.accept_worker;
        node["io_worker"] = conf.io_worker;
        node["process_worker"] = conf.process_worker;
        node["args"] = YAML::Load(LexicalCast<std::map<std::string, std::string>, std::string>()(conf.args));
        for (auto &i : conf.address) {
            node["address"].push_back(i);
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

class TcpServer : public std::enable_shared_from_this<TcpServer>, Noncopyable {
public:
    typedef std::shared_ptr<TcpServer> ptr;

    /**
     * @brief Construct a new Tcp Server object
     * 
     * @param worker 执行soket客户端工作的协程调度器
     * @param io_worker 
     * @param accept_worker 服务器socket执行接收accpet连接的协程调度器
     */
    TcpServer(webs::IOManager *worker = webs::IOManager::GetThis(), webs::IOManager *io_worker = webs::IOManager::GetThis(), webs::IOManager *accept_worker = webs::IOManager::GetThis());

    virtual ~TcpServer();

    /**
     * @brief 创建socketfd，并绑定地址(服务器的ip和端口)
     * 
     * @param address 
     * @param ssl 是否是ssl
     * @return true 
     * @return false 
     */
    bool bind(webs::Address::ptr address, bool ssl = false);

    /**
     * @brief 创建socketfd，并绑定地址(服务器的ip和端口)
     * 
     * @param addr 需要绑定的地址数组
     * @param fails 绑定失败的地址
     * @param ssl 
     * @return true 
     * @return false 
     */
    bool bind(const std::vector<webs::Address::ptr> &addr, std::vector<webs::Address::ptr> &fails, bool ssl = false);

    /**
     * @brief 启动服务
     *  需要bind成功之后执行
     * @return true 
     * @return false 
     */
    virtual bool start();

    /**
     * @brief 停止服务
     * 
     */
    virtual void stop();

    std::vector<Socket::ptr> getSocks() const {
        return m_socks;
    }

    /**
     * @brief 返回读取超时时间(毫秒)
     * 
     * @return uint64_t 
     */
    uint64_t getRecvTimeout() const {
        return m_recvTimeout;
    }
    /**
     * @brief 设置读取超时时间(毫秒)
     * 
     * @param value 
     */
    void setRecvTimeout(uint64_t value) {
        m_recvTimeout = value;
    }

    /**
     * @brief 获取服务器名称
     * 
     * @return std::string 
     */
    std::string getName() const {
        return m_name;
    }

    /**
     * @brief 设置服务器名称
     * 
     * @param name 
     */
    void setName(const std::string &name) {
        m_name = name;
    }

    /**
     * @brief 是否停止
     * 
     * @return true 
     * @return false 
     */
    bool isStop() const {
        return m_isStop;
    }

    TcpServerConf::ptr getConf() const {
        return m_conf;
    }

    /**
     * @brief Set the Conf object
     * 
     * @param val 需要智能指针对象
     */
    void setConf(TcpServerConf::ptr val) {
        m_conf = val;
    }

    /**
     * @brief Set the Conf object
     * 
     * @param conf 需要TcpServerConf对象
     */
    void setConf(const TcpServerConf &conf);

    bool loadCertificates(const std::string &cert_file, const std::string &key_file);

    virtual std::string toString(const std::string &prefix = "");

protected:
    /**
     * @brief 处理新连接的Socket类；每次 accept 就调用这个函数
     * 
     * @param client 
     */
    virtual void
    handleClient(Socket::ptr client);

    /**
     * @brief 开始接受连接
     * 
     * @param sock 
     */
    virtual void startAccept(Socket::ptr sock);

private:
    // 监听Socket数组；可以监听多地址
    std::vector<Socket::ptr> m_socks;
    // 新连接的Socket工作的调度器；
    IOManager *m_worker;
    IOManager *m_ioWorker;
    // 服务器Socket接收连接的调度器 -- 可能是监听
    IOManager *m_acceptWorker;
    // 接收超时时间(毫秒)
    uint64_t m_recvTimeout;
    // 服务器的名称；可以是TCP UDP
    std::string m_name;
    // 服务器的类型
    std::string m_type = "tcp";
    // 服务器是否停止
    bool m_isStop;
    bool m_ssl;
    // 配置信息
    TcpServerConf::ptr m_conf;
};

} // namespace webs

#endif

/**
 * @brief socket管理类中 stop和 析构函数的设计往往不一样，原因是什么？
 * stop()函数的主要目的是停止服务器的运行。当你调用stop()时，服务器将停止接受新的连接，并关闭所有现有的socket。
 * 然而，stop()函数并不会销毁TcpServer对象本身。这意味着，即使在调用stop()之后，
 * 你仍然可以访问TcpServer对象的其他方法和成员变量。例如，你可能想要查询服务器的状态，
 * 或者在稍后的某个时间点重新启动服务器。

 * 另一方面，析构函数~TcpServer()的主要目的是销毁TcpServer对象。当TcpServer对象的生命周期结束时，
 * 析构函数将被自动调用。在析构函数中，你需要释放对象所拥有的所有资源，以防止内存泄漏。在这个例子中，
 * 析构函数将关闭所有的socket，并清除m_socks列表。
 * 
 */