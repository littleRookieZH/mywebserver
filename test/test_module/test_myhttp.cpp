/**
 * @file test_myhttp.cpp
 * @author your name (you@domain.com)
 * @brief 测试http
 * @version 0.1
 * @date 2024-08-27
 * 
 * 
 */

#include "../../webs/http_module/http_server.h"
#include "../../webs/log_module/log.h"

webs::Logger::ptr g_logger = WEBS_LOG_ROOT();
webs::IOManager::ptr worker;
void run() {
    g_logger->setLevel(webs::LogLevel::INFO);
    webs::Address::ptr addr = webs::Address::LookupAnyIPAddress("0.0.0.0:8020");
    if (!addr) {
        WEBS_LOG_ERROR(g_logger) << "get address error";
        return;
    }

    webs::http::HttpServer::ptr http_server(new webs::http::HttpServer());
    while (!http_server->bind(addr)) {
        WEBS_LOG_ERROR(g_logger) << "bind" << *addr << "fail";
        sleep(1);
    }
    http_server->start();
}

int main() {
    webs::IOManager iom(1);
    // worker.reset(new webs::IOManager(1, false));
    iom.schedule(run);
    return 0;
}
