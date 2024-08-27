/**
 * @file servlet.h
 * @author your name (you@domain.com)
 * @brief Servlet封装
 * 处理一个请求、启动时需要注册某些链接、url... 设定url响应哪些请求，执行哪些动作
 * @version 0.1
 * @date 2024-05-22
 * 
 * 
 */
#ifndef __WEBS_SERVLET_H__
#define __WEBS_SERVLET_H__

#include "http.h"
#include "http_session.h"
#include "../util_module/util.h"
#include "../util_module/mutex.h"
#include <unordered_map>

namespace webs {
namespace http {
class Servlet {
public:
    typedef std::shared_ptr<Servlet> ptr;

    Servlet(const std::string &name) :
        m_name(name){};

    virtual ~Servlet(){};

    /**
     * @brief 处理请求
     * 所有的Servlet 都继承该方法
     * @param request HTTP请求
     * @param response HTTP响应
     * @param session HTTP连接 -- 这是我们封装的Http连接socket
     * @return uint32_t 是否处理成功
     */
    virtual int32_t handle(http::HttpRequest::ptr request, http::HttpResponse::ptr response, http::HttpSession::ptr session) = 0;

    /**
     * @brief 返回Servlet名称
     * 
     * @return const std::string& 
     */
    const std::string &getName() const {
        return m_name;
    };

protected:
    // 名称
    std::string m_name;
};

class FunctionServlet : public Servlet {
public:
    // 智能指针类型定义
    typedef std::shared_ptr<FunctionServlet> ptr;
    //  函数回调类型定义
    typedef std::function<int32_t(http::HttpRequest::ptr, http::HttpResponse::ptr, http::HttpSession::ptr)> callback;

    /**
     * @brief Construct a new Function Servlet object
     * 
     * @param cb 回调函数
     */
    FunctionServlet(callback cb);

    /**
     * @brief handle 使用回调函数处理业务
     * 
     * @param request HTTP请求
     * @param response HTTP响应
     * @param session HTTP连接 -- 这是我们封装的Http连接socket
     * @return int32_t 是否处理成功
     */
    virtual int32_t handle(http::HttpRequest::ptr request, http::HttpResponse::ptr response, http::HttpSession::ptr session) override;

private:
    // 回调函数
    callback m_cb;
};

/**
 * @brief NotFoundServlet(默认返回404)
 * 
 */
class NotFoundServlet : public Servlet {
public:
    typedef std::shared_ptr<NotFoundServlet> ptr;

    NotFoundServlet(const std::string &name);

    virtual int32_t handle(http::HttpRequest::ptr request, http::HttpResponse::ptr response, http::HttpSession::ptr session) override;

private:
    // 名称  -- 并不同于 Servlet的 m_name
    std::string m_name;
    // 设置响应消息体
    std::string m_content;
};

/**
 * @brief 定义了获取Servlet对象和获取Servlet名称的两个纯虚函数。
 * 
 */
class IServletCreator {
public:
    typedef std::shared_ptr<IServletCreator> ptr;

    virtual ~IServletCreator() {
    }

    virtual Servlet::ptr get() const = 0;

    /**
     * @brief 获取名称 -- 获取的名称的实现是不同的
     * 
     * @return std::string 
     */
    virtual std::string getName() const = 0;
};

/**
 * @brief 持有一个已经创建好的Servlet对象。 
 * 
 */
class HoldServletCreator : public IServletCreator {
public:
    std::shared_ptr<HoldServletCreator> ptr;

    HoldServletCreator(Servlet::ptr servlet) :
        m_servlet(servlet) {
    }

    /**
     * @brief 获取持有的Servlet对象。
     * 
     * @return Servlet::ptr 
     */
    virtual Servlet::ptr get() const override {
        return m_servlet;
    };

    /**
     * @brief 获取Servlet的名字。事实上所有Servlet派生类都必须给Servlet，m_name赋值
     * 
     * @return std::string 
     */
    virtual std::string getName() const override {
        return m_servlet->getName();
    }

private:
    Servlet::ptr m_servlet;
};

/**
 * @brief 动态创建新的Servlet派生类对象。
 * 
 * @tparam T Servlet的派生类
 */
template <class T>
class ServletCreator : public IServletCreator {
public:
    typedef std::shared_ptr<ServletCreator> ptr;

    ServletCreator() = default;

    Servlet::ptr
    get() const override {
        return Servlet::ptr(new T); // 会调用 T的 带参构造函数
    };

    std::string getName() const override {
        return TypeToName<T>(); // 输出的是T类型的名字
    };
};

/**
 * @brief Servlet分发器
 * 负责管理和维护所有Servlet之间的关系
 */
class ServletDispatch : public Servlet {
public:
    // 智能指针类型定义
    typedef std::shared_ptr<ServletDispatch> ptr;

    // 读写锁类型定义
    typedef RWMutex RWMutexType;

    ServletDispatch();

    /**
     * @brief 与具体的handle不同；该函数是 根据情况 调用 处理具体事务的handle函数
     * 
     * @param request 
     * @param response 
     * @param session 
     * @return int32_t 
     */
    virtual int32_t handle(http::HttpRequest::ptr request, http::HttpResponse::ptr response, http::HttpSession::ptr session) override;

    /**
     * @brief 添加精确匹配servlet
     * 
     * @param uri uri
     * @param slt serlvet
     */
    void addServlet(const std::string &uri, Servlet::ptr slt);

    /**
     * @brief 添加精确匹配servlet
     * 
     * @param uri 
     * @param cb FunctionServlet回调函数
     */
    void addServlet(const std::string &uri, FunctionServlet::callback cb);

    /**
     * @brief 添加模糊匹配servlet
     * 
     * @param uri 
     * @param slt 
     */
    void addGlobServlet(const std::string &uri, Servlet::ptr slt);

    /**
     * @brief 添加模糊匹配servlet
     * 
     * @param uri 
     * @param cb FunctionServlet回调函数
     */
    void addGlobServlet(const std::string &uri, FunctionServlet::callback cb);

    /**
     * @brief 添加 IServletCreator 对象
     * 
     * @param uri 
     * @param creator 
     */
    void addServletCreator(const std::string &uri, IServletCreator::ptr creator);

    void addGlobServletCreator(const std::string &uri, IServletCreator::ptr creator);

    /**
     * @brief 向精确servlet集合中 添加创建servlet对象的 智能指针对象
     * 
     * @tparam T servlet的派生类
     * @param uri 
     */
    template <class T>
    void addServletCreator(const std::string &uri) {
        addServletCreator(uri, std::make_shared<ServletCreator<T>>());
    }

    /**
     * @brief 向模糊servlet集合中 添加创建servlet对象的 智能指针对象
     * 
     * @tparam T 
     * @param uri 
     */
    template <class T>
    void addGlobServletCreator(const std::string &uri) {
        addGlobServlet(uri, std::make_shared<ServletCreator<T>>());
    }

    void delServlet(const std::string &uri);

    void delGlobServlet(const std::string &uri);

    Servlet::ptr getServlet(const std::string &uri);

    Servlet::ptr getGlobServlet(const std::string &uri);

    Servlet::ptr getMatchedServlet(const std::string &uri);

    /**
     * @brief 遍历所有精确的 servlet 
     * 
     * @param infos 传出参数
     */
    void listAllServletCreator(std::map<std::string, IServletCreator::ptr> &infos);

    /**
     * @brief 遍历所有模糊的 servlet 
     * 
     * @param infos 传出参数
     */
    void listAllGlobServletCreator(std::map<std::string, IServletCreator::ptr> &infos);

    Servlet::ptr getDefault() const {
        return m_default;
    };

    void setDefault(Servlet::ptr value) {
        m_default = value;
    }

private:
    // 互斥量
    RWMutexType m_mutex;
    // 精确匹配 servlet MAP uri(/webs/xxx)  --> servlet
    std::unordered_map<std::string, IServletCreator::ptr> m_datas;
    // 模糊匹配 servlet数组 uri(/webs/*)  --> servlet
    std::vector<std::pair<std::string, IServletCreator::ptr>> m_globs;
    // 默认servlet，所有路径都没匹配到时使用
    Servlet::ptr m_default;
};
}
} // namespace webs::http

#endif