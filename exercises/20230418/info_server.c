#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>

#define MAX_LENGTH 1024
#define MAX_CLIENT 10

int main(int argc, char *argv[])
{
    // Kiểm tra đầu vào
    if (argc != 2)
    {
        printf("Usage: %s <port>\n", argv[0]);
        return 1;
    }

    // Thiết lập thông tin địa chỉ cho socket
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(atoi(argv[1]));

    // Tạo socket
    int server = socket(AF_INET, SOCK_STREAM, 0);
    if (server == -1)
    {
        perror("socket() failed");
        return 1;
    }

    // Gán địa chỉ cho socket
    if (bind(server, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("bind() failed");
        return 1;
    }

    // Lắng nghe kết nối
    if (listen(server, MAX_CLIENT) == -1)
    {
        perror("listen() failed");
        return 1;
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
        int client = accept(server, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client == -1)
        {
            perror("accept() failed");
            return 1;
        }
        printf("Accepted connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        while (1)
        {
            // Nhận dữ liệu từ client
            int bytes_received = recv(client, buf, MAX_LENGTH, 0);
            buf[bytes_received] = '\0';
            if (bytes_received == -1)
            {
                perror("recv() failed");
                return 1;
            }
            else if (bytes_received == 0 || strcmp(buf, "exit\n") == 0)
            {
                printf("Client from %s:%d disconnected\n\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                break;
            }

            // Phân tách dữ liệu thông tin máy tính
            char *token = strtok(buf, ";");
            printf("Hostname: %s\n", token);
            token = strtok(NULL, ";");
            printf("Number of disks: %s\n", token);
            int n = atoi(token);
            while (token = strtok(NULL, ";"))
            {
                printf("\t- Disk %s: ", token);
                token = strtok(NULL, ";");
                printf("%sGB\n", token);
            }
            printf("\n");
        }
        close(client);
    }

    return 0;
}