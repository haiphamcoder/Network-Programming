#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>

#define MAX_LENGTH 1024

int main(int argc, char *argv[])
{
    // Kiểm tra đầu vào
    if (argc != 3)
    {
        printf("Usage: %s <server-IP-address> <port>\n", argv[0]);
        return 1;
    }

    // Thiết lập thông tin địa chỉ cho socket
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(atoi(argv[1]));

    // Tạo socket
    int server = socket(AF_INET, SOCK_STREAM, 0);
    if (server == -1)
    {
        perror("socket() failed");
        return 1;
    }

    // Gán địa chỉ cho socket
    if (bind(server, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("bind() failed");
        return 1;
    }

    // Lắng nghe kết nối
    if (listen(server, 10) == -1)
    {
        perror("listen() failed");
        return 1;
    }

    // Nhận dữ liệu từ client
    char buf[MAX_LENGTH];
    memset(buf, 0, MAX_LENGTH);
    close(server);
    return 0;
}