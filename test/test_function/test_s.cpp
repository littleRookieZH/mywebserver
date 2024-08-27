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
    struct sockaddr_un name;
    int ret;
    int connection_socket;
    int data_socket;
    int result;
    char buffer[BUFFER_SIZE];

    /* 在文件系统中删除套接字文件 */
    unlink(SOCKET_NAME);

    /* 创建套接字 */
    connection_socket = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (connection_socket == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    /* 绑定套接字到套接字名 */
    memset(&name, 0, sizeof(struct sockaddr_un));
    name.sun_family = AF_UNIX;
    strncpy(name.sun_path, SOCKET_NAME, sizeof(name.sun_path) - 1);

    ret = bind(connection_socket, (const struct sockaddr *)&name,
               sizeof(struct sockaddr_un));
    if (ret == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    /* 监听连接 */
    ret = listen(connection_socket, 20);
    if (ret == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    /* 循环处理连接 */
    for (;;) {
        /* 接受连接 */
        data_socket = accept(connection_socket, NULL, NULL);
        if (data_socket == -1) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        /* 读取数据，处理数据，发送回复 */
        for (;;) {
            memset(buffer, 0, BUFFER_SIZE);
            ret = read(data_socket, buffer, BUFFER_SIZE);
            if (ret == -1) {
                perror("read");
                // exit(EXIT_FAILURE);
            }

            /* 确保字符串以null结尾 */
            buffer[sizeof(buffer) - 1] = 0;

            printf("Received: %s\n", buffer);

            /* 将结果写回客户端 */
            ret = write(data_socket, buffer, ret);
            if (ret == -1) {
                perror("write");
                // exit(EXIT_FAILURE);
            }
        }
    }
    while (1)
        ;

    /* 关闭套接字 */
    close(connection_socket);
    close(data_socket);

    return 0;
}
