#include "../../webs/http_module/http_server.h"
#include "../../webs/log_module/log.h"

static webs::Logger::ptr g_logger = WEBS_LOG_ROOT();

// ... 接收一个可变参数  __VA_ARGS__ 将可变参数转为字符串
#define XX(...) #__VA_ARGS__

webs::IOManager::ptr worker;
void run() {
    g_logger->setLevel(webs::LogLevel::INFO);
    webs::http::HttpServer::ptr server(new webs::http::HttpServer());
    webs::Address::ptr addr = webs::Address::LookupAnyIPAddress("0.0.0.0:8020");
    while (!server->bind(addr)) {
        sleep(2);
    }
    auto sd = server->getServletDispatch();
    sd->addServlet("/var/www/htm/xx", [](webs::http::HttpRequest::ptr req, webs::http::HttpResponse::ptr rsp, webs::http::HttpSession::ptr session) { rsp->setBody(req->toString()); return 0; });

    sd->addGlobServlet("/var/www/htm/*", [](webs::http::HttpRequest::ptr req, webs::http::HttpResponse::ptr rsp, webs::http::HttpSession::ptr session) {rsp->setBody("Glob:\r\n" + req->toString());
    return 0; });

    sd->addGlobServlet("/webssource/*", [](webs::http::HttpRequest::ptr req, webs::http::HttpResponse::ptr rsp, webs::http::HttpSession::ptr session) {
        rsp->setBody(XX(<html>
                                <head><title> 404 Not Found</ title></ head>
                                <body>
                                <center><h1> 404 Not Found</ h1></ center>
                                <hr><center>
                                    nginx
                                / 1.16.0
                            < / center > </ body>
                            </ html> < !--a padding to disable MSIE
                        and Chrome friendly error page-- >
                                < !--a padding to disable MSIE
                        and Chrome friendly error page-- >
                                < !--a padding to disable MSIE
                        and Chrome friendly error page-- >
                                < !--a padding to disable MSIE
                        and Chrome friendly error page-- >
                                < !--a padding to disable MSIE
                        and Chrome friendly error page-- >
                                < !--a padding to disable MSIE
                        and Chrome friendly error page-- >));
        return 0; });
    server->start();
}

int main(int argc, char **argv) {
    webs::IOManager iom(1, true, "main");
    worker.reset(new webs::IOManager(3, false, "worker"));
    // webs::IOManager iom(2);
    iom.schedule(run);
    return 0;
}
