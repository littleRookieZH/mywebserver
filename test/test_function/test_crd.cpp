#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <iostream>

#define SOCKET_NAME "/tmp/DemoSocket"
#define BUFFER_SIZE 128

int main(void) {
    struct sockaddr_un addr;
    int i;
    int ret;
    int data_socket;
    char buffer[BUFFER_SIZE];

    /* 创建数据套接字 */
    data_socket = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (data_socket == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    /* 连接到服务器 */
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_NAME, sizeof(addr.sun_path) - 1);

    ret = connect(data_socket, (const struct sockaddr *)&addr,
                  sizeof(struct sockaddr_un));

    std::cout << " ret1 = " << ret << " errstr = " << strerror(errno) << std::endl;

    while (1)
        ;
    if (ret == -1) {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    /* 发送参数 */
    printf("Sending: %s\n", "Hello, world!");
    ret = write(data_socket, "Hello, world!", strlen("Hello, world!") + 1);
    while (1)
        ;
    // if (ret == -1) {
    //     perror("write");
    //     exit(EXIT_FAILURE);
    // }

    /* 关闭套接字 */
    close(data_socket);

    return 0;
}
