#include <functional>
#include <memory>
#include <iostream>

// 假设有一个 Socket 类
class Socket {
public:
    using ptr = std::shared_ptr<Socket>;
    // 假设有一个友好的重载 << 运算符
    friend std::ostream &operator<<(std::ostream &os, const Socket &sock) {
        os << "Socket";
        return os;
    }
};

// 假设有一个 Logger 类
class Logger {
public:
    void info(const std::string &message) {
        std::cout << message << std::endl;
    }
};
Logger g_logger;

// #define SYLAR_LOG_INFO(logger) logger.info

// Noncopyable 基类
class Noncopyable {
public:
    Noncopyable() = default;
    Noncopyable(const Noncopyable &) = delete;
    Noncopyable &operator=(const Noncopyable &) = delete;
};

// TcpServer 类
class TcpServer : public std::enable_shared_from_this<TcpServer>, Noncopyable {
public:
    using ptr = std::shared_ptr<TcpServer>;

    void handleClient(Socket::ptr client) {
        std::cout << "handleClient: " << *client << std::endl;
    }

    void schedule(std::function<void()> f) {
        f(); // 直接调用，实际情况下应该是将其添加到某个任务队列中
    }

    // shared_from_this获取的是调用run函数的对象
    void run(Socket::ptr client) {
        schedule(std::bind(&TcpServer::handleClient, shared_from_this(), client));
    }
};

int main() {
    auto server = std::make_shared<TcpServer>();
    auto client = std::make_shared<Socket>();
    server->run(client);
    client.reset();
    // server->run(server);
    server->run(client);
    return 0;
}
