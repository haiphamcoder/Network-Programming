#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <sys/select.h>
#include <time.h>
#include <sys/ioctl.h>
#include <errno.h>

#define MAX_CLIENT 10
#define MAX_MSG_LEN 1024

typedef struct client
{
    int sockfd;
    struct sockaddr_in addr;
} client_t;

int main(int argc, char *argv[])
{
    // Kiểm tra đầu vào
    if (argc != 2)
    {
        printf("Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Khởi tạo socket TCP
    int server = socket(AF_INET, SOCK_STREAM, 0);
    if (server < 0)
    {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    // Thiết lập địa chỉ server
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(atoi(argv[1]));

    // Gán địa chỉ cho socket
    if (bind(server, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind() failed");
        exit(EXIT_FAILURE);
    }

    // Lắng nghe kết nối
    if (listen(server, MAX_CLIENT) < 0)
    {
        perror("listen() failed");
        exit(EXIT_FAILURE);
    }

    // Khởi tạo mảng client
    client_t clients[MAX_CLIENT];
    int client_count = 0;

    while (1)
    {
        // Khởi tạo mảng file descriptor
        fd_set readfds;
        FD_ZERO(&readfds);

        // Thêm server vào mảng file descriptor
        FD_SET(server, &readfds);

        // Thêm các client vào mảng file descriptor
        if (client_count == 0)
        {
            printf("\nWaiting for client on %s:%s\n",
                   inet_ntoa(server_addr.sin_addr), argv[1]);
        }
        else
        {
            for (int i = 0; i < client_count; i++)
            {
                FD_SET(clients[i].sockfd, &readfds);
            }
        }

        if (select(FD_SETSIZE, &readfds, NULL, NULL, NULL) < 0)
        {
            perror("select() failed");
            continue;
        }

        // Kiểm tra server có sẵn sàng nhận kết nối mới không
        if (FD_ISSET(server, &readfds))
        {
            // Chấp nhận kết nối mới
            struct sockaddr_in client_addr;
            socklen_t client_addr_len = sizeof(client_addr);
            int client = accept(server, (struct sockaddr *)&client_addr, &client_addr_len);
            if (client < 0)
            {
                perror("accept() failed");
                continue;
            }

            // Thêm client vào mảng client
            if (client_count < MAX_CLIENT)
            {
                clients[client_count].sockfd = client;
                clients[client_count].addr = client_addr;
                client_count++;
                printf("Client from %s:%d connected\n",
                       inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                // Gửi thông báo cho client biết về số lượng client hiện tại
                char msg[MAX_MSG_LEN];
                memset(msg, 0, MAX_MSG_LEN);
                sprintf(msg, "Hello. There are %d clients online\n", client_count);
                if (send(client, msg, strlen(msg), 0) < 0)
                {
                    perror("send() failed");
                    continue;
                }

                // Gửi thông báo yêu cầu client nhập xâu cần chuẩn hóa
                memset(msg, 0, MAX_MSG_LEN);
                strcpy(msg, "Enter a string to capitalize: ");
                if (send(client, msg, strlen(msg), 0) < 0)
                {
                    perror("send() failed");
                    continue;
                }
            }
            else
            {
                printf("Maximum number of clients reached\n");
                printf("Client %d disconnected\n", client);
            }
        }

        // Kiểm tra các client có sẵn sàng gửi dữ liệu không
        for (int i = 0; i < client_count; i++)
        {
            if (FD_ISSET(clients[i].sockfd, &readfds))
            {
                // Nhận dữ liệu từ client
                char msg[MAX_MSG_LEN];
                memset(msg, 0, MAX_MSG_LEN);
                int msg_len = recv(clients[i].sockfd, msg, MAX_MSG_LEN, 0);
                if (msg_len < 0)
                {
                    perror("recv() failed");
                    continue;
                }
                else if (msg_len == 0)
                {
                    // Xóa client khỏi mảng file descriptor
                    FD_CLR(clients[i].sockfd, &readfds);

                    // Xóa client khỏi mảng client
                    printf("Client from %s:%d disconnected\n",
                           inet_ntoa(clients[i].addr.sin_addr), ntohs(clients[i].addr.sin_port));
                    if (i < client_count - 1)
                    {
                        clients[i] = clients[client_count - 1];
                    }
                    client_count--;

                    continue;
                }
                else
                {
                    // Xử lý dữ liệu
                    if (strcmp(msg, "exit\n") == 0)
                    {
                        // Gửi thông báo chào tạm biệt cho client
                        memset(msg, 0, MAX_MSG_LEN);
                        strcpy(msg, "Goodbye!\n");
                        if (send(clients[i].sockfd, msg, strlen(msg), 0) < 0)
                        {
                            perror("send() failed");
                            continue;
                        }

                        // Xóa client khỏi mảng file descriptor
                        FD_CLR(clients[i].sockfd, &readfds);

                        // Xóa client khỏi mảng client
                        printf("Client from %s:%d disconnected\n",
                               inet_ntoa(clients[i].addr.sin_addr), ntohs(clients[i].addr.sin_port));
                        if (i < client_count - 1)
                        {
                            clients[i] = clients[client_count - 1];
                        }
                        client_count--;

                        continue;
                    }
                    else
                    {
                        msg[strcspn(msg, "\n")] = '\0';

                        // Chuẩn hóa xâu
                        // Xóa khoảng trắng dư thừa ở đầu xâu
                        while (msg[0] == ' ')
                        {
                            for(int i = 0; i < strlen(msg); i++)
                            {
                                msg[i] = msg[i + 1];
                            }
                        }

                        // Xóa khoảng trắng dư thừa ở cuối xâu
                        while (msg[strlen(msg) - 1] == ' ')
                        {
                            msg[strlen(msg) - 1] = '\0';
                        }

                        // Xóa khoảng trắng dư thừa ở giữa xâu
                        for (int i = 0; i < strlen(msg) - 1; i++)
                        {
                            if (msg[i] == ' ' && msg[i + 1] == ' ')
                            {
                                for(int j = i; j < strlen(msg); j++)
                                {
                                    msg[j] = msg[j + 1];
                                }
                                i--;
                            }
                        }

                        // Viết hoa chữ cái đầu tiên của mỗi từ
                        for (int i = 0; i < strlen(msg); i++)
                        {
                            if (i == 0 || msg[i - 1] == ' ')
                            {
                                if (msg[i] >= 'a' && msg[i] <= 'z')
                                {
                                    msg[i] -= 32;
                                }
                            }
                        }

                        // Gửi xâu đã chuẩn hóa cho client
                        strcat(msg, "\n");
                        char msg_to_sent[MAX_MSG_LEN];
                        strcpy(msg_to_sent, "Capitalized string: ");
                        strcat(msg_to_sent, msg);
                        if (send(clients[i].sockfd, msg_to_sent, strlen(msg_to_sent), 0) < 0)
                        {
                            perror("send() failed");
                            continue;
                        }

                        // Yêu cầu client nhập xâu mới
                        memset(msg, 0, MAX_MSG_LEN);
                        strcpy(msg, "Enter a string to capitalize: ");
                        if (send(clients[i].sockfd, msg, strlen(msg), 0) < 0)
                        {
                            perror("send() failed");
                            continue;
                        }
                    }
                }
            }
        }
    }

    // Đóng socket
    close(server);

    return 0;
}