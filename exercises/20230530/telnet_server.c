#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define MAX_CLIENT 10
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

void handle_command(int sockfd, char *command)
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
            return;
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
                return;
            }
        }

        // Yêu cầu client nhập lệnh tiếp theo
        char *msg = "Enter command: ";
        if (send(sockfd, msg, strlen(msg), 0) == -1)
        {
            perror("send() failed");
            return;
        }

        // Đóng file
        fclose(fp);
    }
    else
    {
        // Gửi thông báo lỗi về client
        char *msg = "Invalid command!\nEnter another command: ";
        if (send(sockfd, msg, strlen(msg), 0) == -1)
        {
            perror("send() failed");
            return;
        }
    }
}

void signalHandler(int signo)
{
    int stat;
    printf("signo = %d\n", signo);
    int pid = wait(&stat);
    printf("Child %d terminated with status: %d\n", pid, stat);
    return;
}

int main(int argc, char *argv[])
{
    // Kiểm tra đầu vào
    if (argc != 2)
    {
        printf("Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Thiết lập thông tin địa chỉ cho socket
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET; // IPv4
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
    if (listen(server, MAX_CLIENT) < 0)
    {
        perror("listen() failed");
        exit(EXIT_FAILURE);
    }

    // // Thiết lập signal handler
    // signal(SIGCHLD, signalHandler);

    // Khai báo chứa các client
    client_t clients[MAX_CLIENT];
    int num_clients = 0;

    // Khai báo mảng file descriptor
    fd_set readfds;

    while (1)
    {
        // Xóa tập hợp readfds
        FD_ZERO(&readfds);

        // Thêm server vào tập hợp readfds
        FD_SET(server, &readfds);

        // Thêm các client vào tập hợp readfds
        if (num_clients == 0)
        {
            printf("Waiting for clients on %s:%d\n",
                   inet_ntoa(server_addr.sin_addr),
                   ntohs(server_addr.sin_port));
        }
        else
        {
            for (int i = 0; i < num_clients; i++)
            {
                FD_SET(clients[i].sockfd, &readfds);
            }
        }

        // Chờ sự kiện trên các file descriptor
        if (select(FD_SETSIZE, &readfds, NULL, NULL, NULL) < 0)
        {
            perror("select() failed");
            exit(EXIT_FAILURE);
        }

        // Kiểm tra xem server có sẵn sàng nhân kết nối từ client hay không
        if (FD_ISSET(server, &readfds))
        {
            // Chấp nhận kết nối từ client
            struct sockaddr_in client_addr;
            memset(&client_addr, 0, sizeof(client_addr));
            socklen_t client_addr_len = sizeof(client_addr);
            int client_sockfd = accept(server, (struct sockaddr *)&client_addr, &client_addr_len);
            if (client_sockfd < 0)
            {
                perror("accept() failed");
                continue;
            }

            // Thêm client vào mảng client
            if (num_clients < MAX_CLIENT)
            {
                clients[num_clients].sockfd = client_sockfd;
                clients[num_clients].addr = client_addr;
                strcpy(clients[num_clients].username, "");
                strcpy(clients[num_clients].password, "");
                num_clients++;
                printf("Client connected from %s:%d\n",
                       inet_ntoa(client_addr.sin_addr),
                       ntohs(client_addr.sin_port));

                // Gửi thông báo đến client yêu cầu nhập username và password
                char *msg = "Enter your \"username password\": ";
                if (send(client_sockfd, msg, strlen(msg), 0) < 0)
                {
                    perror("send() failed");
                    continue;
                }
            }
            else
            {
                char *msg = "Too many clients\n";
                if (send(client_sockfd, msg, strlen(msg), 0) < 0)
                {
                    perror("send() failed");
                    continue;
                }
                close(client_sockfd);
            }
        }

        // Kiểm tra xem có client nào gửi dữ liệu đến hay không
        for (int i = 0; i < num_clients; i++)
        {
            if (FD_ISSET(clients[i].sockfd, &readfds))
            {
                // Tạo tiến trình con để xử lý dữ liệu từ client
                if (fork() == 0)
                {
                    // Đóng server socket
                    close(server);

                    // Tạo buffer để nhận dữ liệu từ client
                    char buf[BUFFER_SIZE];
                    memset(buf, 0, BUFFER_SIZE);

                    while (1)
                    {
                        // Nhận dữ liệu từ client
                        int len = recv(clients[i].sockfd, buf, BUFFER_SIZE, 0);
                        if (len < 0)
                        {
                            perror("recv() failed");
                            break;
                        }
                        else if (len == 0)
                        {
                            printf("Client disconnected from %s:%d\n",
                                   inet_ntoa(clients[i].addr.sin_addr),
                                   ntohs(clients[i].addr.sin_port));
                            close(clients[i].sockfd);
                            clients[i].sockfd = -1;
                            break;
                        }
                        else
                        {
                            // Xoá ký tự xuống dòng
                            buf[strcspn(buf, "\n")] = 0;

                            // Kiểm tra xem client đã nhập username và password hay chưa
                            if (strcmp(clients[i].username, "") == 0 && strcmp(clients[i].password, "") == 0)
                            {
                                // Tách username và password từ chuỗi nhận được
                                char username[MAX_USERNAME_LEN];
                                char password[MAX_PASSWORD_LEN];
                                char temp[BUFFER_SIZE];
                                memset(username, 0, MAX_USERNAME_LEN);
                                memset(password, 0, MAX_PASSWORD_LEN);
                                memset(temp, 0, BUFFER_SIZE);
                                int ret = sscanf(buf, "%s %s %s", username, password, temp);
                                if (ret == 2)
                                {
                                    // Xác thực thông tin đăng nhập
                                    int status = authenticate_user(username, password);
                                    if (status == 1)
                                    {
                                        // Lưu thông tin đăng nhập của client
                                        strcpy(clients[i].username, username);
                                        strcpy(clients[i].password, password);

                                        // Gửi thông báo đến client
                                        char *msg = "Login successful!\nEnter command: ";
                                        if (send(clients[i].sockfd, msg, strlen(msg), 0) < 0)
                                        {
                                            perror("send() failed");
                                            break;
                                        }
                                    }
                                    else if (status == 0)
                                    {
                                        // Gửi thông báo đến client
                                        char *msg = "Login failed!\nEnter your \"username password\": ";
                                        if (send(clients[i].sockfd, msg, strlen(msg), 0) < 0)
                                        {
                                            perror("send() failed");
                                            break;
                                        }
                                    }
                                    else
                                    {
                                        // Gửi thông báo đến client
                                        char *msg = "Server error\nDatabase file not found!\nPlease enter again your \"username password\": ";
                                        if (send(clients[i].sockfd, msg, strlen(msg), 0) < 0)
                                        {
                                            perror("send() failed");
                                            break;
                                        }
                                    }
                                }
                                else
                                {
                                    // Gửi thông báo đến client
                                    char *msg = "Invalid format!\nEnter your \"username password\": ";
                                    if (send(clients[i].sockfd, msg, strlen(msg), 0) < 0)
                                    {
                                        perror("send() failed");
                                        break;
                                    }
                                    continue;
                                }
                            }
                            else
                            {
                                if (strcmp(buf, "exit") == 0)
                                {
                                    printf("Client disconnected from %s:%d\n",
                                           inet_ntoa(clients[i].addr.sin_addr),
                                           ntohs(clients[i].addr.sin_port));

                                    // Gửi thông báo đến client
                                    char *msg = "Goodbye\n";
                                    if (send(clients[i].sockfd, msg, strlen(msg), 0) < 0)
                                    {
                                        perror("send() failed");
                                        break;
                                    }
                                    break;
                                }
                                else
                                {
                                    handle_command(clients[i].sockfd, buf);
                                    continue;
                                }
                            }
                        }
                    }

                    // Thoát tiến trình con
                    exit(EXIT_SUCCESS);
                }

                wait(NULL);

                // Đóng client socket
                close(clients[i].sockfd);

                // Xóa client khỏi mảng client
                clients[i] = clients[num_clients - 1];
                num_clients--;
                i--;

                // Xóa client khỏi tập hợp readfds
                FD_CLR(clients[i].sockfd, &readfds);

                // Tiến trình cha tiếp tục vòng lặp
                continue;
            }
        }
    }

    // Đóng server socket
    close(server);

    return 0;
}