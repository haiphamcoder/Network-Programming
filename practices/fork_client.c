#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>

int main(int argc, char *argv[])
{
    // Kiểm tra đầu vào
    if (argc != 3)
    {
        printf("Usage: %s <ip> <port>\n", argv[0]);
        return 1;
    }

    // Thiết lập thông tin địa chỉ cho socket
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET; // IPv4
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    server_addr.sin_port = htons(atoi(argv[2]));

    // Khởi tạo socket
    int client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
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

    if (fork() == 0)
    {
        // Tiến trình con
        char buf[1024];
        while (1)
        {
            memset(buf, 0, sizeof(buf));
            fgets(buf, sizeof(buf), stdin);
            if(strcmp(buf, "exit\n") == 0)
            {
                kill(getppid(), SIGKILL);
                kill(getpid(), SIGKILL);
            }
            if (send(client, buf, strlen(buf), 0) == -1)
            {
                perror("send() failed");
                return 1;
            }
        }
    }
    else
    {
        // Tiến trình cha
        char buf[1024];
        while (1)
        {
            memset(buf, 0, sizeof(buf));
            int bytes_received = recv(client, buf, sizeof(buf), 0);
            if (bytes_received < 0)
            {
                perror("recv() failed");
                return 1;
            }
            else if (bytes_received == 0)
            {
                printf("Connection closed\n");
                break;
            }
            
            printf("Received: %s", buf);
        }
    }

    // Đóng socket
    close(client);

    // Dừng các tiến trình
    killpg(0, SIGKILL);

    return 0;
}