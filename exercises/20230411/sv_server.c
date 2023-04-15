#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>

#define MAX_LENGTH 1024
#define MAX_CLIENT 10

int main(int argc, char *argv[])
{

    // Kiểm tra đầu vào
    if (argc != 3)
    {
        printf("Usage: %s <port> <log-file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Tạo socket
    int server = socket(AF_INET, SOCK_STREAM, 0);
    if (server == -1)
    {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    // Thiết lập thông tin địa chỉ cho socket
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(atoi(argv[1]));

    // Gán địa chỉ cho socket
    if (bind(server, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("bind() failed");
        exit(EXIT_FAILURE);
    }

    // Lắng nghe kết nối
    if (listen(server, MAX_CLIENT) == -1)
    {
        perror("listen() failed");
        exit(EXIT_FAILURE);
    }

    // Nhận dữ liệu từ client
    char buf[MAX_LENGTH];
    memset(buf, 0, MAX_LENGTH);
    while (1)
    {
        printf("Waiting for client on %s %s\n", inet_ntoa(server_addr.sin_addr), argv[1]);

        // Chấp nhận kết nối từ client
        struct sockaddr_in client_addr;
        memset(&client_addr, 0, sizeof(client_addr));
        socklen_t client_addr_len = sizeof(client_addr);
        int client = accept(server,
                            (struct sockaddr *)&client_addr,
                            &client_addr_len);
        if (client == -1)
        {
            perror("accept() failed");
            exit(EXIT_FAILURE);
        }
        printf("Accepted socket %d from IP: %s:%d\n",
               client,
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port));

        while (1)
        {
            // Nhận dữ liệu từ client
            int bytes_received = recv(client, buf, MAX_LENGTH, 0);
            buf[bytes_received] = '\0';
            if (bytes_received == -1)
            {
                perror("recv() failed");
                exit(EXIT_FAILURE);
            }
            else if (bytes_received == 0 || strcmp(buf, "exit\n") == 0)
            {
                printf("Client from %s:%d disconnected\n\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                break;
            }

            // Lấy thời gian hiện tại
            time_t current_time = time(NULL);
            char *formatted_time = ctime(&current_time);
            formatted_time[strlen(formatted_time) - 1] = '\0';

            // Ghi vào file log
            FILE *log_file = fopen(argv[2], "a");
            if (log_file == NULL)
            {
                perror("fopen() failed");
                exit(EXIT_FAILURE);
            }
            printf("%s %s %s\n", inet_ntoa(client_addr.sin_addr), formatted_time, buf);
            fprintf(log_file, "%s %s %s\n", inet_ntoa(client_addr.sin_addr), formatted_time, buf);
            fclose(log_file);
        }

        // Đóng kết nối
        close(client);
    }
    return 0;
}