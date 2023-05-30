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

        // Xóa file
        remove("out.txt");
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

void signalHandler()
{
    int stat;
    int pid = wait(&stat);
    if (pid > 0)
    {
        printf("Child %d terminated with exit status %d\n", pid, stat);
    }
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

    // Thiết lập signal handler
    signal(SIGCHLD, signalHandler);

    while (1)
    {
        printf("Waiting for new client on %s:%d\n",
               inet_ntoa(server_addr.sin_addr),
               ntohs(server_addr.sin_port));

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
        printf("New client connected from %s:%d\n",
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port));

        // Tạo process con để xử lý client
        if (fork() == 0)
        {
            // Đóng server socket
            close(server);

            // Khởi tạo thông tin client
            client_t client_info;
            memset(&client_info, 0, sizeof(client_info));
            client_info.sockfd = client;
            client_info.addr = client_addr;
            strcpy(client_info.username, "");
            strcpy(client_info.password, "");

            // Gửi thông báo yêu cầu nhập username và password
            char *msg = "Enter your \"username password\": ";
            if (send(client, msg, strlen(msg), 0) < 0)
            {
                perror("send() failed");
                continue;
            }

            // Tạo buffer để nhận dữ liệu từ client
            char buf[BUFFER_SIZE];

            // Xử lý yêu cầu của client
            while (1)
            {
                // Reset buffer
                memset(buf, 0, sizeof(buf));

                // Nhận dữ liệu từ client
                int bytes_received = recv(client, buf, sizeof(buf), 0);
                if (bytes_received < 0)
                {
                    perror("recv() failed");
                    break;
                }
                else if (bytes_received == 0)
                {
                    printf("Client from %s:%d disconnected\n",
                           inet_ntoa(client_addr.sin_addr),
                           ntohs(client_addr.sin_port));
                    break;
                }
                else
                {
                    // Xóa ký tự xuống dòng ở cuối chuỗi
                    buf[strcspn(buf, "\n")] = 0;

                    // Kiểm tra xem client đã đăng nhập hay chưa
                    if (strcmp(client_info.username, "") == 0 && strcmp(client_info.password, "") == 0)
                    {
                        char username[MAX_USERNAME_LEN];
                        char password[MAX_PASSWORD_LEN];
                        char temp[BUFFER_SIZE];
                        int ret = sscanf(buf, "%s %s %s", username, password, temp);
                        if (ret == 2)
                        {
                            // Xác thực thông tin đăng nhập
                            int status = authenticate_user(username, password);
                            if (status == 1)
                            {
                                // Lưu thông tin đăng nhập vào client
                                strcpy(client_info.username, username);
                                strcpy(client_info.password, password);

                                // Gửi thông báo đến client
                                char *msg = "Login successful\nEnter your command: ";
                                if (send(client, msg, strlen(msg), 0) < 0)
                                {
                                    perror("send() failed");
                                    continue;
                                }
                            }
                            else if (status == 0)
                            {
                                // Gửi thông báo đến client
                                char *msg = "Login failed! Username or Password is incorrect\nPlease enter again your \"username password\": ";
                                if (send(client, msg, strlen(msg), 0) < 0)
                                {
                                    perror("send() failed");
                                    continue;
                                }
                            }
                            else
                            {
                                // Gửi thông báo đến client
                                char *msg = "Server error\nDatabase file not found!\nPlease enter again your \"username password\": ";
                                if (send(client, msg, strlen(msg), 0) < 0)
                                {
                                    perror("send() failed");
                                    continue;
                                }
                            }
                        }
                        else
                        {
                            // Gửi thông báo đến client
                            char *msg = "Invalid format!\nPlease enter again your \"username password\": ";
                            if (send(client, msg, strlen(msg), 0) < 0)
                            {
                                perror("send() failed");
                                continue;
                            }
                        }
                    }
                    else
                    {
                        // Xử lý lệnh
                        if (strcmp(buf, "quit") == 0 || strcmp(buf, "exit") == 0)
                        {
                            // Gửi thông báo đến client
                            char *msg = "Goodbye!\n";
                            if (send(client, msg, strlen(msg), 0) < 0)
                            {
                                perror("send() failed");
                                continue;
                            }
                            break;
                        }
                        else
                        {
                            handle_command(client, buf);
                        }
                    }
                }
            }

            // Đóng client socket
            close(client);

            // Thoát process con
            exit(EXIT_SUCCESS);
        }

        // Đóng client socket
        close(client);
    }

    // Đóng server socket
    close(server);

    return 0;
}