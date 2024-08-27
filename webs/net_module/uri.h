/**
 * @file uri.h
 * @author your name (you@domain.com)
 * @brief URI封装类
 * 通过解析uri，确定连接哪一个host、port  -->解析出来之后就可以连接对应的地址 --> 发送请求
 * Create、Uri、dump、toString、createAddress、getPort函数的实现在uri.rl文件中
 * @version 0.1
 * @date 2024-05-24
 *       foo://user@sylar.com:8042/over/there?name=ferret#nose
 *       \___/ \________________/\_________/ \_________/ \__/
 *        |            |             |            |        |
 *    scheme      authority        path        query   fragment
 *               user ip port
 */
#ifndef __WEBS_URI_H__
#define __WEBS_URI_H__

#include <memory>
#include <iostream>
#include <string>

#include "address.h"

namespace webs {
class Uri {
public:
    typedef std::shared_ptr<Uri> ptr;

    Uri();

    /**
     * @brief 创建Uri对象
     * 
     * @param uri uri字符串
     * @return Uri::ptr 解析成功返回Uri对象否则返回nullptr
     */
    static Uri::ptr Create(const std::string &uri);

    std::ostream &dump(std::ostream &os) const;

    std::string toString() const;

    /**
     * @brief 获取Address  -- 通过uri创建地址
     * 
     * @return Address::ptr 
     */
    Address::ptr createAddress() const;

    /**
     * @brief 返回scheme
     * 
     * @return const std::string& 
     */
    const std::string &getScheme() const {
        return m_scheme;
    }

    /**
     * @brief 返回用户信息
     * 
     * @return const std::string& 
     */
    const std::string &getUserInfo() const {
        return m_userinfo;
    }

    /**
     * @brief 返回host
     * 
     * @return const std::string& 
     */
    const std::string &getHost() {
        return m_host;
    }

    /**
     * @brief 返回路径
     * 
     * @return const std::string& 
     */
    const std::string &getPath() const;

    /**
     * @brief 返回查询条件
     * 
     * @return const std::string& 
     */
    const std::string &getQuery() const {
        return m_query;
    }

    /**
     * @brief 返回fragment
     * 
     * @return const std::string& 
     */
    const std::string &getFragment() const {
        return m_fragment;
    }

    /**
     * @brief 返回端口
     * 
     * @return int32_t 
     */
    int32_t getPort() const;

    /**
     * @brief设置scheme
     * 
     * @param value 
     */
    void setScheme(const std::string &value) {
        m_scheme = value;
    }

    /**
     * @brief 设置用户信息
     * 
     * @param value 
     */
    void setUserinfo(const std::string &value) {
        m_userinfo = value;
    }

    /**
     * @brief 设置host信息 主机信息
     * 
     * @param value 
     */
    void setHost(const std::string &value) {
        m_host = value;
    }

    /**
     * @brief 设置路径
     * 
     * @param value 
     */
    void setPath(const std::string &value) {
        m_path = value;
    }

    /**
     * @brief 设置查询条件
     * 
     * @param value 
     */
    void setQuery(const std::string &value) {
        m_query = value;
    }

    /**
     * @brief 设置fragment
     * 
     * @param value 
     */
    void setFragment(const std::string &value) {
        m_fragment = value;
    }

    /**
     * @brief 设置端口号
     * 
     * @param value 
     */
    void setPort(int32_t value) {
        m_port = value;
    }

private:
    /**
     * @brief 是否默认端口
     * http 80 ；https 443 ； port = 0 
     * @return true 
     * @return false 
     */
    bool isDefaultPort() const;

private:
    // scheme
    std::string m_scheme;
    // 用户信息
    std::string m_userinfo;
    // host 主机信息
    std::string m_host;
    // 路径
    std::string m_path;
    // 查询参数
    std::string m_query;
    // fragment
    std::string m_fragment;
    // 端口
    int32_t m_port;
};
} // namespace webs
#endif