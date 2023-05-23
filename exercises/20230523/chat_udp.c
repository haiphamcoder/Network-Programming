#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/select.h>
#include <string.h>
#include <time.h>

#define MSG_MAX_LEN 1024

int main(int argc, char *argv[])
{
    // Kiểm tra đầu vào
    if (argc != 4)
    {
        printf("Usage: %s <receiver_ip> <receiver_port> <listening_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Thiết lập địa chỉ cho receiver
    struct sockaddr_in receiver_addr;
    memset(&receiver_addr, 0, sizeof(receiver_addr));
    receiver_addr.sin_family = AF_INET;
    receiver_addr.sin_addr.s_addr = inet_addr(argv[1]);
    receiver_addr.sin_port = htons(atoi(argv[2]));

    // Thiết lập địa chỉ cho local
    struct sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = INADDR_ANY;
    local_addr.sin_port = htons(atoi(argv[3]));

    // Tạo socket cho local
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    // Gán địa chỉ cho socket của local
    if (bind(sockfd, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0)
    {
        perror("bind() failed");
        exit(EXIT_FAILURE);
    }

    // Khai bảo danh sách file descriptor
    fd_set readfds;

    // Khai báo buffer để nhận và gửi tin nhắn
    char buffer[MSG_MAX_LEN];

    while (1)
    {
        // Xóa danh sách file descriptor và thêm socket và stdin vào danh sách
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(sockfd, &readfds);

        // Chờ đến khi có sự kiện trên socket local hoặc stdin
        if (select(FD_SETSIZE, &readfds, NULL, NULL, NULL) < 0)
        {
            perror("select() failed");
            exit(EXIT_FAILURE);
        }

        // Nhận tin nhắn từ stdin
        if (FD_ISSET(STDIN_FILENO, &readfds))
        {
            memset(buffer, 0, MSG_MAX_LEN);
            fgets(buffer, MSG_MAX_LEN, stdin);
            buffer[strcspn(buffer, "\n")] = 0;

            // Gửi tin nhắn cho receiver
            if (sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&receiver_addr, sizeof(receiver_addr)) < 0)
            {
                perror("sendto() failed");
                exit(EXIT_FAILURE);
            }
        }

        // Kiểm tra xem local có sẵn sàng nhận tin nhắn không
        if (FD_ISSET(sockfd, &readfds))
        {
            struct sockaddr_in sender_addr;
            memset(&sender_addr, 0, sizeof(sender_addr));
            socklen_t sender_addr_len = sizeof(sender_addr);

            // Nhận tin nhắn từ sender
            memset(buffer, 0, MSG_MAX_LEN);
            int msg_len = recvfrom(sockfd, buffer, MSG_MAX_LEN, 0, (struct sockaddr *)&sender_addr, &sender_addr_len);
            if (msg_len < 0)
            {
                perror("recvfrom() failed");
                exit(EXIT_FAILURE);
            } 

            // In ra màn hình
            time_t now = time(NULL);
            struct tm *t = localtime(&now);
            char time_str[22];
            memset(time_str, 0, 22);
            strftime(time_str, 22, "%Y/%m/%d %I:%M:%S%p", t);
            printf("%s:%d %s: -> %s\n", inet_ntoa(sender_addr.sin_addr), ntohs(sender_addr.sin_port), time_str,buffer);
        }
    }

    // Đóng socket
    close(sockfd);
}