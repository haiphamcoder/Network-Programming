#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/select.h>

int main(int argc, char *argv[])
{
    // Kiểm tra đầu vào
    if (argc != 4)
    {
        printf("Usage: %s <server_ip> <server_port> <client_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Khởi tạo socket UDP cho sender
    int sender = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sender < 0)
    {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    // Thiết lập địa chỉ server cho sender
    struct sockaddr_in sender_addr;
    sender_addr.sin_family = AF_INET;
    sender_addr.sin_addr.s_addr = inet_addr(argv[1]);
    sender_addr.sin_port = htons(atoi(argv[2]));

    // Khởi tạo socket UDP cho receiver
    int receiver = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (receiver < 0)
    {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    // Thiết lập địa chỉ server cho receiver
    struct sockaddr_in receiver_addr;
    receiver_addr.sin_family = AF_INET;
    receiver_addr.sin_addr.s_addr = INADDR_ANY;
    receiver_addr.sin_port = htons(atoi(argv[3]));

    // Gán địa chỉ cho socket
    if (bind(receiver, (struct sockaddr *)&receiver_addr, sizeof(receiver_addr)) < 0)
    {
        perror("bind() failed");
        exit(EXIT_FAILURE);
    }

    fd_set fdread, fdtest;
    FD_ZERO(&fdread);
    FD_SET(STDIN_FILENO, &fdread);
    FD_SET(receiver, &fdread);

    char buf[256];

    while (1)
    {

        fdtest = fdread;

        if (select(receiver + 1, &fdtest, NULL, NULL, NULL))
        {
            perror("select() failed");
            break;
        }

        if (FD_ISSET(STDIN_FILENO, &fdtest))
        {
            fgets(buf, sizeof(buf), stdin);
            if (sendto(sender, buf, strlen(buf), 0, (struct sockaddr *)&sender_addr, sizeof(sender_addr)) < 0)
            {
                perror("send() failed");
                break;
            }

            if (FD_ISSET(receiver, &fdtest))
            {
                int len = recvfrom(receiver, buf, sizeof(buf), 0, NULL, NULL);
                if (len < 0)
                {
                    perror("recv() failed");
                    break;
                }
                else if (len == 0)
                {
                    printf("Connection closed\n");
                    break;
                }
                buf[len] = 0;
                printf("Received: %s\n", buf);
            }
        }
    }
    // Đóng kết nối
    close(sender);
    close(receiver);
}