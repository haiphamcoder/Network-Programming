#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

typedef struct client
{
    int sockfd;
    struct sockaddr_in addr;
    char name[32];
    char id[32];
} client_t;

client_t clients[MAX_CLIENTS];
int client_count = 0;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
struct sockaddr_in server_addr;

void *client_proc(void *);

int main(int argc, char *argv[])
{
    // Kiểm tra đầu vào
    if (argc != 2)
    {
        printf("Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Tạo socket
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0)
    {
        perror("socket() failded");
        exit(EXIT_FAILURE);
    }

    // Thiết lập địa chỉ server
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(atoi(argv[1]));

    // Gán địa chỉ server vào socket
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind() failed");
        exit(EXIT_FAILURE);
    }

    // Lắng nghe kết nối
    if (listen(server_sock, MAX_CLIENTS) < 0)
    {
        perror("listen() failed");
        exit(EXIT_FAILURE);
    }

    printf("Waiting for clients on %s:%d...\n",
           inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));

    while (1)
    {
        // Chấp nhận kết nối từ client
        struct sockaddr_in client_addr;
        memset(&client_addr, 0, sizeof(client_addr));
        socklen_t client_len = sizeof(client_addr);
        int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
        if (client_sock < 0)
        {
            perror("accept() failed");
            exit(EXIT_FAILURE);
        }

        if (client_count == MAX_CLIENTS)
        {
            char *msg = "Server is full!\nPlease try again later.\n";
            if (send(client_sock, msg, strlen(msg), 0) < 0)
            {
                perror("send() failed");
            }
            close(client_sock);
            continue;
        }

        // Thông báo kết nối thành công
        printf("Client from %s:%d connected\n",
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        // Thêm client vào danh sách
        pthread_mutex_lock(&clients_mutex);
        clients[client_count].sockfd = client_sock;
        clients[client_count].addr = client_addr;
        strcpy(clients[client_count].name, "");
        strcpy(clients[client_count].id, "");
        client_count++;
        pthread_mutex_unlock(&clients_mutex);

        // Tạo thread để xử lý client
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, client_proc, (void *)&clients[client_count - 1]) != 0)
        {
            perror("pthread_create() failed");
            exit(EXIT_FAILURE);
        }
        pthread_detach(thread_id);
    }

    close(server_sock);
    return 0;
}

void *client_proc(void *param)
{
    client_t *client = (client_t *)param;
    char buffer[BUFFER_SIZE];

    // Nhận id và name từ client
    while (1)
    {
        // Yêu cầu client nhập "client_id: client_name"
        char *msg = "Enter your \"client_id: client_name\": ";
        if (send(client->sockfd, msg, strlen(msg), 0) < 0)
        {
            perror("send() failed");
        }

        // Nhận "client_id: client_name" từ client
        memset(buffer, 0, BUFFER_SIZE);
        int len = recv(client->sockfd, buffer, BUFFER_SIZE, 0);
        if (len < 0)
        {
            perror("recv() failed");
        }
        else if (len == 0)
        {
            printf("Client from %s:%d disconnected\n",
                   inet_ntoa(client->addr.sin_addr), ntohs(client->addr.sin_port));
            for (int i = 0; i < client_count; i++)
            {
                if (client->sockfd == clients[i].sockfd)
                {
                    pthread_mutex_lock(&clients_mutex);
                    clients[i] = clients[client_count - 1];
                    client_count--;
                    if (client_count == 0)
                    {
                        printf("Waiting for clients on %s:%d...\n",
                               inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));
                    }
                    pthread_mutex_unlock(&clients_mutex);
                    break;
                }
            }
            return NULL;
        }
        else
        {
            // Xóa ký tự xuống dòng
            buffer[strcspn(buffer, "\n")] = 0;

            // Tách id và name
            char id[BUFFER_SIZE];
            char name[BUFFER_SIZE];
            char temp[BUFFER_SIZE];

            int ret = sscanf(buffer, "%s %s %s", id, name, temp);
            if (ret == 2)
            {
                int len = strlen(id);
                if (id[len - 1] != ':')
                {
                    char *msg = "Invalid format! Please try again!\n";
                    if (send(client->sockfd, msg, strlen(msg), 0) < 0)
                    {
                        perror("send() failed");
                    }
                    continue;
                }
                else
                {
                    id[len - 1] = 0;
                    strcpy(client->id, id);
                    strcpy(client->name, name);

                    // Thông báo cho client biết đã sẵn sàng gửi tin nhắn
                    char *msg = "OK! Ready to chat!\n";
                    if (send(client->sockfd, msg, strlen(msg), 0) < 0)
                    {
                        perror("send() failed");
                    }

                    break;
                }
            }
            else
            {
                char *msg = "Invalid format! Please try again!\n";
                if (send(client->sockfd, msg, strlen(msg), 0) < 0)
                {
                    perror("send() failed");
                }
                continue;
            }
        }
    }

    // Nhận tin nhắn từ client và gửi lại cho tất cả client khác
    while (1)
    {
        memset(buffer, 0, BUFFER_SIZE);
        int len = recv(client->sockfd, buffer, BUFFER_SIZE, 0);
        if (len < 0)
        {
            perror("recv() failed");
        }
        else if (len == 0)
        {
            printf("Client from %s:%d disconnected\n",
                   inet_ntoa(client->addr.sin_addr), ntohs(client->addr.sin_port));
            for (int i = 0; i < client_count; i++)
            {
                if (client->sockfd == clients[i].sockfd)
                {
                    pthread_mutex_lock(&clients_mutex);
                    clients[i] = clients[client_count - 1];
                    client_count--;
                    if (client_count == 0)
                    {
                        printf("Waiting for clients on %s:%d...\n",
                               inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));
                    }
                    pthread_mutex_unlock(&clients_mutex);
                    break;
                }
            }
            break;
        }
        else
        {
            // Xóa ký tự xuống dòng
            buffer[strcspn(buffer, "\n")] = 0;

            // Định dạng tin nhắn cần gửi
            time_t now = time(NULL);
            struct tm *t = localtime(&now);
            char time_str[22];
            memset(time_str, 0, 22);
            strftime(time_str, BUFFER_SIZE, "%Y/%m/%d %I:%M:%S%p", t);
            char msg_to_send[BUFFER_SIZE + 58];
            sprintf(msg_to_send, "\n%s %s: %s\n\n", time_str, client->id, buffer);

            // Gửi tin nhắn cho tất cả client khác
            for (int i = 0; i < client_count; i++)
            {
                if (client->sockfd != clients[i].sockfd)
                {
                    if (strcmp(clients[i].id, "") == 0)
                    {
                        continue;
                    }
                    if (send(clients[i].sockfd, msg_to_send, strlen(msg_to_send), 0) < 0)
                    {
                        perror("send() failed");
                    }
                }
            }
        }
    }

    return NULL;
}