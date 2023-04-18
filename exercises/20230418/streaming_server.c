#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>

#define MAX_CLIENT 10

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
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(atoi(argv[1]));

    // Tạo socket
    int server = socket(AF_INET, SOCK_STREAM, 0);
    if (server == -1)
    {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

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
    printf("Waiting for client on %s:%s\n", inet_ntoa(server_addr.sin_addr), argv[1]);

    // Chấp nhận kết nối từ client
    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(client_addr));
    socklen_t client_addr_len = sizeof(client_addr);
    int client = accept(server, (struct sockaddr *)&client_addr, &client_addr_len);
    if (client == -1)
    {
        perror("accept() failed");
        exit(EXIT_FAILURE);
    }
    printf("Connection from %s %d port [tcp/*] succeeded!\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    // Nhận dữ liệu từ client
    int count = 0;
    char buf[20], receive[21];
    memset(buf, 'x', 9);
    memset(buf + 9, 0, 11);
    while (1)
    {
        memset(receive, 0, 21);
        int bytes_received = recv(client, receive, 20, 0);
        receive[bytes_received] = '\0';
        if (bytes_received == -1)
        {
            perror("recv() failed");
            exit(EXIT_FAILURE);
        }
        else if (bytes_received == 0)
        {
            printf("Count: %d\n", count);
            break;
        }
        strncat(buf, receive, 10);
        if (strstr(buf, "0123456789") != NULL)
        {
            count++;
        }
        strcpy(buf, buf + 10);
        strcat(buf, receive + 10);
        if (strstr(buf, "0123456789") != NULL)
        {
            count++;
        }
        strcpy(buf, buf + 10);
    }
    close(client);
    close(server);
    return 0;
}