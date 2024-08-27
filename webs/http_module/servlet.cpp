#include "servlet.h"
#include <fnmatch.h>

namespace webs {
namespace http {
FunctionServlet::FunctionServlet(callback cb) :
    Servlet("FunctionServlet"),
    m_cb(cb) {
}

int32_t FunctionServlet::handle(http::HttpRequest::ptr request, http::HttpResponse::ptr response, http::HttpSession::ptr session) {
    return m_cb(request, response, session);
}

NotFoundServlet::NotFoundServlet(const std::string &name) :
    Servlet("NotFoundServlet"), m_name(name) {
    // 无效访问404页面
    m_content = "<html><head><title>404 Not Found"
                "</title></head><body><center><h1>404 Not Found</h1></center>"
                "<hr><center>"
                + name + "</center></body></html>";
}

int32_t NotFoundServlet::handle(http::HttpRequest::ptr request, http::HttpResponse::ptr response, http::HttpSession::ptr session) {
    response->setStatus(webs::http::HttpStatus::NOT_FOUND);
    response->setHeader("Server", "webs/1.0.0");
    response->setHeader("Content-type", "text/html");
    response->setBody(m_content);
    return 0;
}

ServletDispatch::ServletDispatch() :
    Servlet("ServletDispatch"),
    m_default(Servlet::ptr(new NotFoundServlet("webs/1.0"))) {
}

/* 获取请求路径 -- 查找是否存在对应的 servlet对象 -- 如果存在，调用handle函数；不存在返回0  */
int32_t ServletDispatch::handle(http::HttpRequest::ptr request, http::HttpResponse::ptr response, http::HttpSession::ptr session) {
    auto servlet = getMatchedServlet(request->getPath());
    if (servlet) {
        return servlet->handle(request, response, session);
    }
    return 0;
}

void ServletDispatch::addServlet(const std::string &uri, Servlet::ptr slt) {
    RWMutex::WriteLock lock(m_mutex);
    m_datas[uri] = std::make_shared<HoldServletCreator>(slt); // 存在函数为slt的构造函数
}

void ServletDispatch::addServlet(const std::string &uri, FunctionServlet::callback cb) {
    RWMutex::WriteLock lock(m_mutex);
    m_datas[uri] = std::make_shared<HoldServletCreator>(std::make_shared<FunctionServlet>(cb));
}

void ServletDispatch::addServletCreator(const std::string &uri, IServletCreator::ptr creator) {
    RWMutex::WriteLock lock(m_mutex);
    m_datas[uri] = creator;
}

void ServletDispatch::addGlobServlet(const std::string &uri, Servlet::ptr slt) {
    RWMutex::WriteLock lock(m_mutex);
    for (auto it = m_globs.begin(); it != m_globs.end(); ++it) {
        if (it->first == uri) {
            m_globs.erase(it);
            break;
        }
    }
    m_globs.push_back(std::make_pair(uri, std::make_shared<HoldServletCreator>(slt)));
}

void ServletDispatch::addGlobServlet(const std::string &uri, FunctionServlet::callback cb) {
    return addGlobServlet(uri, std::make_shared<FunctionServlet>(cb));
}

void ServletDispatch::addGlobServletCreator(const std::string &uri, IServletCreator::ptr creator) {
    RWMutex::WriteLock lock(m_mutex);
    for (auto it = m_globs.begin(); it != m_globs.end(); ++it) {
        if (it->first == uri) {
            m_globs.erase(it);
            break;
        }
    }
    m_globs.push_back(std::make_pair(uri, creator));
}

void ServletDispatch::delServlet(const std::string &uri) {
    RWMutexType::WriteLock lock(m_mutex);
    m_datas.erase(uri); // map支持直接删除key
}

void ServletDispatch::delGlobServlet(const std::string &uri) {
    RWMutex::WriteLock lock(m_mutex);
    for (auto it = m_globs.begin(); it != m_globs.end(); ++it) {
        if (it->first == uri) {
            m_globs.erase(it);
            break;
        }
    }
}

Servlet::ptr ServletDispatch::getServlet(const std::string &uri) {
    RWMutex::ReadLock lock(m_mutex);
    auto it = m_datas.find(uri);
    return it == m_datas.end() ? nullptr : it->second->get();
}

Servlet::ptr ServletDispatch::getGlobServlet(const std::string &uri) {
    RWMutex::ReadLock lock(m_mutex);
    for (auto it = m_globs.begin(); it != m_globs.end(); ++it) {
        if (it->first == uri) {
            return it->second->get();
        }
    }
    return nullptr;
}

/* 匹配失败函数默认值 */
Servlet::ptr ServletDispatch::getMatchedServlet(const std::string &uri) {
    RWMutex::ReadLock lock(m_mutex);
    auto it = m_datas.find(uri);
    if (it != m_datas.end()) {
        return it->second->get();
    }
    for (auto it = m_globs.begin(); it != m_globs.end(); ++it) {
        if (!fnmatch(it->first.c_str(), uri.c_str(), 0)) { // 返回0 表示匹配成功
            return it->second->get();
        }
    }
    return m_default;
}

void ServletDispatch::listAllServletCreator(std::map<std::string, IServletCreator::ptr> &infos) {
    RWMutex::ReadLock lock(m_mutex);
    for (auto &i : m_datas) {
        infos[i.first] = i.second;
    }
}

void ServletDispatch::listAllGlobServletCreator(std::map<std::string, IServletCreator::ptr> &infos) {
    RWMutex::ReadLock lock(m_mutex);
    for (auto &i : m_datas) {
        infos[i.first] = i.second;
    }
}
}
} // namespace webs::http