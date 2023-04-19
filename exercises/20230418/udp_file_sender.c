#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MAX_LENGTH 1024

int main(int argc, char *argv[])
{
    // Kiểm tra đầu vào
    if (argc != 4)
    {
        printf("Usage: %s <server-IP-address> <port> <file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Tạo socket
    int sender = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sender < 0)
    {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    // Thiết lập địa chỉ server receiver
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    server_addr.sin_port = htons(atoi(argv[2]));

    // Mở file
    FILE *fp = fopen(argv[3], "rb");
    if (fp == NULL)
    {
        perror("fopen() failed");
        exit(EXIT_FAILURE);
    }

    // Gửi tên file đến server receiver
    if (sendto(sender, argv[3], strlen(argv[3]), 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) != strlen(argv[3]))
    {
        perror("sendto() failed");
        exit(EXIT_FAILURE);
    }

    // Đọc nội dung file và gửi sang server receiver
    int len = 0;
    char buf[MAX_LENGTH];
    while ((len = fread(buf, 1, MAX_LENGTH, fp)) > 0)
    {
        if (sendto(sender, buf, len, 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) != len)
        {
            perror("sendto() failed");
            exit(EXIT_FAILURE);
        }
    }
    printf("File sent successfully.\n");

    // Đóng file
    fclose(fp);

    // Đóng socket
    close(sender);

    return 0;
}