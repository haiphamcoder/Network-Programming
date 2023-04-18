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
    if (argc != 4)
    {
        printf("Usage: %s <server-IP-address> <port> <file-to-trans>\n", argv[0]);
        return 1;
    }

    // Thiết lập thông tin địa chỉ cho socket
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    server_addr.sin_port = htons(atoi(argv[2]));

    // Tạo socket
    int client = socket(AF_INET, SOCK_STREAM, 0);
    if (client == -1)
    {
        perror("socket() failed");
        return 1;
    }

    // Kết nối đến server
    if (connect(client, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("connect() failed");
        return 1;
    }
    printf("Connection to %s %s port [tcp/*] succeeded!\n", argv[1], argv[2]);

    // Mở file
    FILE *fp = fopen(argv[3], "r");
    if (fp == NULL)
    {
        perror("fopen() failed");
        return 1;
    }

    // Đọc file và gửi dữ liệu
    char buf[20];
    memset(buf, 0, 20);
    while (!feof(fp))
    {
        // Đọc dữ liệu từ file
        fgets(buf, 20, fp);

        // Gửi dữ liệu đến server
        if (send(client, buf, strlen(buf), 0) == -1)
        {
            perror("send() failed");
            return 1;
        }
    }
    fclose(fp);
    close(client);
    return 0;
}