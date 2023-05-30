#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>

#define BUF_SIZE 1024
#define MAX_CLIENT 10

void format_time(char buf[], const char *format)
{
    time_t now = time(NULL);
    struct tm *timeinfo = localtime(&now);
    if (strcmp(format, "dd/mm/yyyy") == 0)
    {
        strftime(buf, BUF_SIZE, "%d/%m/%Y\n", timeinfo);
    }
    else if (strcmp(format, "dd/mm/yy") == 0)
    {
        strftime(buf, BUF_SIZE, "%d/%m/%y\n", timeinfo);
    }
    else if (strcmp(format, "mm/dd/yyyy") == 0)
    {
        strftime(buf, BUF_SIZE, "%m/%d/%Y\n", timeinfo);
    }
    else if (strcmp(format, "mm/dd/yy") == 0)
    {
        strftime(buf, BUF_SIZE, "%m/%d/%y\n", timeinfo);
    }
    else
    {
        strcpy(buf, "Invalid format\n");
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
    server_addr.sin_family = AF_INET; // IPv4
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

    // Thiết lập signal handler
    signal(SIGCHLD, signalHandler);

    while (1)
    {
        printf("Waiting for new client on %s:%d\n",
               inet_ntoa(server_addr.sin_addr),
               ntohs(server_addr.sin_port));

        // Chấp nhận kết nối
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

        // Tạo tiến trình con để xử lý client
        if (fork() == 0)
        {
            // Tiến trình con
            // Đóng socket server
            close(server);

            // Xử lý client
            while (1)
            {
                // Gửi yêu cầu đến client
                char *msg = "Enter command: ";
                if (send(client, msg, strlen(msg), 0) < 0)
                {
                    perror("send() failed");
                    break;
                }

                // Nhận yêu cầu từ client
                char buf[BUF_SIZE];
                memset(buf, 0, BUF_SIZE);
                int len = recv(client, buf, BUF_SIZE, 0);
                if (len < 0)
                {
                    perror("recv() failed");
                    break;
                }
                else if (len == 0)
                {
                    printf("Client from %s:%d disconnected\n",
                           inet_ntoa(client_addr.sin_addr),
                           ntohs(client_addr.sin_port));
                    break;
                }
                else
                {
                    // Xoá ký tự xuống dòng
                    buf[strcspn(buf, "\n")] = 0;

                    // Thoát nếu client gửi exit hoặc quit
                    if(strcmp(buf, "exit") == 0 || strcmp(buf, "quit") == 0) {
                        printf("Client from %s:%d disconnected\n",
                           inet_ntoa(client_addr.sin_addr),
                           ntohs(client_addr.sin_port));
                        break;
                    }

                    // Xử lý yêu cầu
                    char cmd[BUF_SIZE];
                    char format[BUF_SIZE];
                    char temp[BUF_SIZE];
                    int ret = sscanf(buf, "%s %s %s", cmd, format, temp);
                    if (ret == 2)
                    {
                        if (strcmp(cmd, "GET_TIME") == 0)
                        {
                            memset(buf, 0, BUF_SIZE);
                            format_time(buf, format);
                            if (send(client, buf, strlen(buf), 0) < 0)
                            {
                                perror("send() failed");
                                break;
                            }
                        } else {
                            char *msg = "Invalid command\n";
                            if (send(client, msg, strlen(msg), 0) < 0)
                            {
                                perror("send() failed");
                                break;
                            }
                        }
                    }
                    else
                    {
                        char *msg = "Invalid command\n";
                        if (send(client, msg, strlen(msg), 0) < 0)
                        {
                            perror("send() failed");
                            break;
                        }
                    }
                }
            }

            // Đóng socket client
            close(client);

            // Kết thúc tiến trình con
            exit(EXIT_SUCCESS);
        }

        close(client);
    }

    // Đóng socket
    close(server);

    return 0;
}