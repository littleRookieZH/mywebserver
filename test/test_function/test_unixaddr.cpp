#include <iostream>
#include <sys/un.h>
#include <cstring>
#include <cstddef>
#include <arpa/inet.h>

class UnixAddress {
public:
    UnixAddress(const std::string &path) {
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sun_family = AF_UNIX;
        m_length = path.size() + 1;

        if (!path.empty() && path[0] == '\0') {
            --m_length;
        }

        if (m_length > sizeof(m_addr.sun_path)) {
            throw std::logic_error("path too long");
        }
        memcpy(m_addr.sun_path, path.c_str(), m_length);
        m_length += offsetof(sockaddr_un, sun_path);
    }

    void printPath() const {
        std::cout << offsetof(sockaddr_un, sun_path) << std::endl;
        size_t len = m_length - offsetof(sockaddr_un, sun_path);
        for (size_t i = 0; i < len; ++i) {
            if (m_addr.sun_path[i] == '\0') {
                std::cout << "\\0";
            } else {
                std::cout << m_addr.sun_path[i];
            }
        }
        std::cout << std::endl;
    }

private:
    sockaddr_un m_addr;
    socklen_t m_length;
};

int main() {
    // const std::string s1 = "\0";
    std::string ss = "@socket";
    ss[0] = '\0';
    UnixAddress addr(ss);
    addr.printPath();

    std::string ss1 = "dffff";
    UnixAddress addr1(ss1);
    addr1.printPath();
    return 0;
}
