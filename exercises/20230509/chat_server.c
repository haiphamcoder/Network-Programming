#include <stdio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <poll.h>

#define MAX_CLIENT 64
#define MAX_MSG_LEN 1024

typedef struct client
{
    int sockfd;
    struct sockaddr_in addr;
    char id[20];
    char name[50];
} client_t;

int main(int argc, char *argv[])
{
    // Kiểm tra đầu vào
    if (argc != 2)
    {
        printf("Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Khởi tạo socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
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
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind() failed");
        exit(EXIT_FAILURE);
    }

    // Lắng nghe kết nối
    if (listen(sockfd, MAX_CLIENT) < 0)
    {
        perror("listen() failed");
        exit(EXIT_FAILURE);
    }

    // Khởi tạo mảng client
    client_t clients[MAX_CLIENT];
    memset(clients, 0, sizeof(clients));
    int n_client = 0;

    while (1)
    {
        // Khởi tạo mảng file descriptor
        struct pollfd fds[MAX_CLIENT + 1];
        memset(fds, 0, sizeof(fds));
        fds[0].fd = sockfd;
        fds[0].events = POLLIN;
        int nfds = 1;

        // Thêm các socket client vào mảng file descriptor
        if (n_client == 0)
        {
            printf("Waiting for clients on %s:%s\n",
                   inet_ntoa(server_addr.sin_addr), argv[1]);
        }
        else
        {
            for (int i = 0; i < n_client; i++)
            {
                fds[nfds].fd = clients[i].sockfd;
                fds[nfds].events = POLLIN;
                nfds++;
            }
        }

        // Chờ sự kiện từ các socket
        int ret = poll(fds, nfds, -1);
        if (ret < 0)
        {
            perror("poll() failed");
            continue;
        }
        if (ret == 0)
        {
            printf("poll() timed out\n");
            continue;
        }

        // Kiểm tra xem server có sẵn sàng nhận kết nối mới không
        if (fds[0].revents & POLLIN)
        {
            // Chấp nhận kết nối
            struct sockaddr_in client_addr;
            memset(&client_addr, 0, sizeof(client_addr));
            socklen_t client_len = sizeof(client_addr);
            int client = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);
            if (client < 0)
            {
                perror("accept() failed");
                continue;
            }

            // Thêm client vào mảng
            if (n_client < MAX_CLIENT)
            {
                clients[n_client].sockfd = client;
                clients[n_client].addr = client_addr;
                strcpy(clients[n_client].id, "");
                strcpy(clients[n_client].name, "");
                n_client++;
                printf("Client connected from %s:%d\n",
                       inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

                // Gửi thông báo yêu cầu client nhập "client_id: client_name"
                char *msg = "Enter your \"client_id: client_name\": ";
                if (send(client, msg, strlen(msg), 0) < 0)
                {
                    perror("send() failed");
                    continue;
                }
            }
            else
            {
                printf("Maximum clients reached!\n");
                printf("Client from %s:%d disconnected: too many clients\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                close(client);
            }
        }

        // Kiểm tra xem có dữ liệu từ các client không
        for (int i = 0; i < n_client; i++)
        {
            if (fds[i + 1].revents & (POLLIN | POLLERR))
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
                    printf("Client from %s:%d disconnected\n", inet_ntoa(clients[i].addr.sin_addr), ntohs(clients[i].addr.sin_port));

                    // Gửi thông báo cho các client khác biết client này đã ngắt kết nối
                    char notification[MAX_MSG_LEN];
                    sprintf(notification, "Client %s disconnected\n", clients[i].id);
                    for (int j = 0; j < n_client; j++)
                    {
                        if (j != i && strcmp(clients[j].id, "") != 0)
                        {
                            if (send(clients[j].sockfd, notification, strlen(notification), 0) < 0)
                            {
                                perror("send() failed");
                                continue;
                            }
                        }
                    }

                    // Đóng socket client
                    close(clients[i].sockfd);

                    // Xóa client khỏi mảng client
                    clients[i] = clients[n_client - 1];
                    n_client--;

                    // Xóa socket client khỏi mảng file descriptor
                    fds[i + 1] = fds[nfds - 1];
                    nfds--;
                    i--;
                    continue;
                }
                else
                {
                    // Xóa ký tự xuống dòng
                    msg[strcspn(msg, "\n")] = 0;

                    // Kiểm tra xem client đã nhập "client_id: client_name" chưa
                    if (strcmp(clients[i].id, "") == 0 && strcmp(clients[i].name, "") == 0)
                    {
                        // Lấy id và name của client
                        char id[20];
                        char name[50];
                        if (sscanf(msg, "%[^:]: %s", id, name) == 2)
                        {
                            // Kiểm tra xem id có trùng với các client khác không
                            int is_valid = 1;
                            for (int j = 0; j < n_client; j++)
                            {
                                if (strcmp(clients[j].id, id) == 0)
                                {
                                    is_valid = 0;
                                    break;
                                }
                            }
                            if (is_valid)
                            {
                                strcpy(clients[i].id, id);
                                strcpy(clients[i].name, name);
                                printf("Client %s:%d registered as %s:%s\n",
                                       inet_ntoa(clients[i].addr.sin_addr), ntohs(clients[i].addr.sin_port),
                                       clients[i].id, clients[i].name);
                                char *msg = "Registered successfully!\n";
                                if (send(clients[i].sockfd, msg, strlen(msg), 0) < 0)
                                {
                                    perror("send() failed");
                                    continue;
                                }
                            }
                            else
                            {
                                char *msg = "Client ID already exists!\n";
                                if (send(clients[i].sockfd, msg, strlen(msg), 0) < 0)
                                {
                                    perror("send() failed");
                                    continue;
                                }
                                msg = "Enter again your \"client_id: client_name\": ";
                                if (send(clients[i].sockfd, msg, strlen(msg), 0) < 0)
                                {
                                    perror("send() failed");
                                    continue;
                                }
                            }
                        }
                        else
                        {
                            char *msg = "Invalid format!\n";
                            if (send(clients[i].sockfd, msg, strlen(msg), 0) < 0)
                            {
                                perror("send() failed");
                                continue;
                            }
                            msg = "Enter again your \"client_id: client_name\": ";
                            if (send(clients[i].sockfd, msg, strlen(msg), 0) < 0)
                            {
                                perror("send() failed");
                                continue;
                            }
                        }
                    }
                    else
                    {
                        // Kiểm tra xem client muốn gửi tin nhắn đến mọi người hay là một người khác
                        char message[MAX_MSG_LEN];
                        char receiver[20];
                        int ret = sscanf(msg, "%[^@]@%s", message, receiver);

                        // Định dạng tin nhắn cần gửi
                        time_t now = time(NULL);
                        struct tm *t = localtime(&now);
                        char time_str[22];
                        memset(time_str, 0, 22);
                        strftime(time_str, MAX_MSG_LEN, "%Y/%m/%d %I:%M:%S%p", t);
                        char msg_to_send[MAX_MSG_LEN + 44];
                        sprintf(msg_to_send, "%s %s: %s\n", time_str, clients[i].id, msg);

                        if (ret == 2)
                        {
                            // Gửi tin nhắn đến một người khác
                            int is_valid = 0;
                            for (int j = 0; j < n_client; j++)
                            {
                                if (strcmp(clients[j].id, receiver) == 0)
                                {
                                    is_valid = 1;
                                    if (send(clients[j].sockfd, msg_to_send, strlen(msg_to_send), 0) < 0)
                                    {
                                        perror("send() failed");
                                        continue;
                                    }
                                    break;
                                }
                            }
                            if (!is_valid)
                            {
                                char *msg = "Invalid receiver!\n";
                                if (send(clients[i].sockfd, msg, strlen(msg), 0) < 0)
                                {
                                    perror("send() failed");
                                    continue;
                                }
                            }
                        }
                        else
                        {
                            // Gửi tin nhắn đến mọi người
                            for (int j = 0; j < n_client; j++)
                            {
                                if (j != i && strcmp(clients[j].id, "") != 0)
                                {
                                    if (send(clients[j].sockfd, msg_to_send, strlen(msg_to_send), 0) < 0)
                                    {
                                        perror("send() failed");
                                        continue;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Đóng socket
    close(sockfd);

    return 0;
}
