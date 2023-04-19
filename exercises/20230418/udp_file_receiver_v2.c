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
    if (argc != 2)
    {
        printf("Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Tạo socket
    int receiver = socket(AF_INET, SOCK_DGRAM, 0);
    if (receiver < 0)
    {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    // Thiết lập địa chỉ server receiver
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(atoi(argv[1]));

    // Gán địa chỉ cho socket
    if (bind(receiver, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind() failed");
        exit(EXIT_FAILURE);
    }

    // Nhận nội dung file từ client sender
    char buf[MAX_LENGTH];
    memset(buf, 0, MAX_LENGTH);
    int bytes_received = 0;
    while ((bytes_received = recvfrom(receiver, buf, MAX_LENGTH, 0, NULL, NULL)) > 0)
    {
        printf("Received %d bytes: %s\n", bytes_received, buf);
        char *pos = strchr(buf, '-');
        char filename[31];
        memset(filename, 0, 31);
        strncpy(filename, buf, pos - buf);
        FILE *fp = fopen(filename, "ab");
        if (fp == NULL)
        {
            perror("fopen() failed");
            exit(EXIT_FAILURE);
        }
        fwrite(pos + 1, 1, strlen(pos + 1), fp);
        fclose(fp);
        memset(buf, 0, MAX_LENGTH);
    }

    // Đóng socket
    close(receiver);

    return 0;
}