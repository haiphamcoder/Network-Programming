#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024
#define MAX_USERNAME_LEN 32
#define MAX_PASSWORD_LEN 32

typedef struct client
{
    int sockfd;
    struct sockaddr_in addr;
    char username[MAX_USERNAME_LEN];
    char password[MAX_PASSWORD_LEN];
} client_t;

client_t clients[MAX_CLIENTS];
int client_count = 0;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
struct sockaddr_in server_addr;

int authenticate_user(char *, char *);
int handle_command(int, char *);
void *client_proc(void *);

int main(int argc, char *argv[])
{
    // Kiểm tra đầu vào
    if (argc != 2)
    {
        printf("Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Thiết lập thông tin địa chỉ cho socket
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(atoi(argv[1]));

    // Khởi tạo socket
    int server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server < 0)
    {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    // Gán địa chỉ cho socket
    if (bind(server, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind() failed");
        exit(EXIT_FAILURE);
    }

    // Chuyển socket sang trạng thái lắng nghe
    if (listen(server, MAX_CLIENTS) < 0)
    {
        perror("listen() failed");
        exit(EXIT_FAILURE);
    }

    printf("Waiting for new client on %s:%d...\n",
               inet_ntoa(server_addr.sin_addr),
               ntohs(server_addr.sin_port));


    while (1)
    {
        // Chấp nhận kết nối từ client
        struct sockaddr_in client_addr;
        memset(&client_addr, 0, sizeof(client_addr));
        socklen_t client_addr_len = sizeof(client_addr);
        int client = accept(server, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client < 0)
        {
            perror("accept() failed");
            exit(EXIT_FAILURE);
        }
        if (client_count == MAX_CLIENTS)
        {
            char *msg = "Server is full!\nPlease try again later.\n";
            if (send(client, msg, strlen(msg), 0) < 0)
            {
                perror("send() failed");
            }
            close(client);
            continue;
        }
        printf("Client from %s:%d connected\n",
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port));

        pthread_mutex_lock(&clients_mutex);
        clients[client_count].sockfd = client;
        clients[client_count].addr = client_addr;
        strcpy(clients[client_count].username, "");
        strcpy(clients[client_count].password, "");
        client_count++;
        pthread_mutex_unlock(&clients_mutex);

        // Tạo thread để xử lý client
        pthread_t thread_id;
        pthread_create(&thread_id, NULL, client_proc, (void *)&clients[client_count - 1]);
        pthread_detach(thread_id);
    }

    // Đóng server socket
    close(server);

    return 0;
}

int authenticate_user(char *username, char *password)
{
    // Mở file database.txt
    FILE *fp = fopen("database.txt", "r");
    if (fp == NULL)
    {
        perror("fopen() failed");
        return -1; // Lỗi mở file
    }

    // Tạo buffer để đọc từ file
    char buf[MAX_USERNAME_LEN + MAX_PASSWORD_LEN + 2];
    memset(buf, 0, sizeof(buf));

    // Đọc từng dòng trong file
    while (fgets(buf, sizeof(buf), fp) != NULL)
    {
        // Xóa ký tự xuống dòng
        buf[strcspn(buf, "\n")] = 0;

        // Tách username và password
        char stored_username[MAX_USERNAME_LEN];
        char stored_password[MAX_PASSWORD_LEN];
        sscanf(buf, "%s %s", stored_username, stored_password);

        // So sánh username và password
        if (strcmp(username, stored_username) == 0 && strcmp(password, stored_password) == 0)
        {
            // Đóng file
            fclose(fp);

            return 1; // Xác thực thành công
        }
    }

    // Đóng file
    fclose(fp);

    return 0; // Xác thực thất bại
}

int handle_command(int sockfd, char *command)
{
    char cmd[BUFFER_SIZE];
    sprintf(cmd, "%s > out.txt", command);

    // Thực thi lệnh
    int status = system(cmd);
    if (status == 0)
    {
        // Mở file out.txt
        FILE *fp = fopen("out.txt", "r");
        if (fp == NULL)
        {
            perror("fopen() failed");
            return -1;
        }

        // Tạo buffer để đọc từ file
        char buf[BUFFER_SIZE];
        memset(buf, 0, sizeof(buf));

        // Đọc từng dòng trong file
        while (fgets(buf, sizeof(buf), fp) != NULL)
        {
            // Gửi dữ liệu về client
            if (send(sockfd, buf, strlen(buf), 0) == -1)
            {
                perror("send() failed");
                return -1;
            }
        }

        // Đóng file
        fclose(fp);

        // Xóa file
        remove("out.txt");
    }
    return status;
}

void *client_proc(void *param)
{
    client_t *client_info = (client_t *)param;
    char buffer[BUFFER_SIZE];

    // Xác thực client
    while (1)
    {
        // Gửi thông báo yêu cầu nhập username và password
        char *msg = "Enter your \"username password\": ";
        if (send(client_info->sockfd, msg, strlen(msg), 0) < 0)
        {
            perror("send() failed");
        }

        // Nhận username và password từ client
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(client_info->sockfd, buffer, sizeof(buffer), 0);
        if (bytes_received < 0)
        {
            perror("recv() failed");
            return NULL;
        }
        else if (bytes_received == 0)
        {
            printf("Client from %s:%d disconnected\n",
                   inet_ntoa(client_info->addr.sin_addr),
                   ntohs(client_info->addr.sin_port));
            for (int i = 0; i < client_count; i++)
            {
                if (client_info->sockfd == clients[i].sockfd)
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
            // Xóa ký tự xuống dòng ở cuối chuỗi
            buffer[strcspn(buffer, "\n")] = 0;

            // Tách username và password
            char username[MAX_USERNAME_LEN];
            char password[MAX_PASSWORD_LEN];
            char temp[BUFFER_SIZE];
            int ret = sscanf(buffer, "%s %s %s", username, password, temp);
            if (ret == 2)
            {
                // Xác thực thông tin đăng nhập
                int status = authenticate_user(username, password);
                if (status == 1)
                {
                    // Lưu thông tin đăng nhập vào client
                    strcpy(client_info->username, username);
                    strcpy(client_info->password, password);

                    // Gửi thông báo đến client
                    char *msg = "Login successful\nReady to execute command!\n";
                    if (send(client_info->sockfd, msg, strlen(msg), 0) < 0)
                    {
                        perror("send() failed");
                        continue;
                    }
                    break;
                }
                else if (status == 0)
                {
                    // Gửi thông báo đến client
                    char *msg = "Login failed! Username or Password is incorrect\nPlease try again!\n";
                    if (send(client_info->sockfd, msg, strlen(msg), 0) < 0)
                    {
                        perror("send() failed");
                        continue;
                    }
                    continue;
                }
                else
                {
                    // Gửi thông báo đến client
                    char *msg = "Server error\nDatabase file not found!\nPlease try again later!\n";
                    if (send(client_info->sockfd, msg, strlen(msg), 0) < 0)
                    {
                        perror("send() failed");
                        continue;
                    }
                    continue;
                }
            }
            else
            {
                // Gửi thông báo đến client
                char *msg = "Invalid format!\nPlease try again!\n";
                if (send(client_info->sockfd, msg, strlen(msg), 0) < 0)
                {
                    perror("send() failed");
                    continue;
                }
                continue;
            }
        }
    }

    // Xử lý lệnh
    while (1)
    {
        // Yêu cầu client nhập lệnh
        char *msg = "Enter your command: ";
        if (send(client_info->sockfd, msg, strlen(msg), 0) < 0)
        {
            perror("send() failed");
            continue;
        }

        // Nhận lệnh từ client
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(client_info->sockfd, buffer, sizeof(buffer), 0);
        if (bytes_received < 0)
        {
            perror("recv() failed");
            return NULL;
        }
        else if (bytes_received == 0)
        {
            printf("Client from %s:%d disconnected\n",
                   inet_ntoa(client_info->addr.sin_addr),
                   ntohs(client_info->addr.sin_port));
            for (int i = 0; i < client_count; i++)
            {
                if (client_info->sockfd == clients[i].sockfd)
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
            // Xóa ký tự xuống dòng ở cuối chuỗi
            buffer[strcspn(buffer, "\n")] = 0;

            if (strcmp(buffer, "quit") == 0 || strcmp(buffer, "exit") == 0)
            {
                // Gửi thông báo đến client
                char *msg = "Goodbye!\n";
                if (send(client_info->sockfd, msg, strlen(msg), 0) < 0)
                {
                    perror("send() failed");
                    continue;
                }
                printf("Client from %s:%d disconnected\n",
                       inet_ntoa(client_info->addr.sin_addr),
                       ntohs(client_info->addr.sin_port));
                for (int i = 0; i < client_count; i++)
                {
                    if (client_info->sockfd == clients[i].sockfd)
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
                // Xử lý lệnh
                if (handle_command(client_info->sockfd, buffer) != 0)
                {
                    char *msg = "Invalid command!\nPleas try again!\n";
                    if (send(client_info->sockfd, msg, strlen(msg), 0) < 0)
                    {
                        perror("send() failed");
                        continue;
                    }
                    continue;
                }
            }
        }
    }

    return NULL;
}