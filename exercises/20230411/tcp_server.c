#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

#define MAX_CLIENT 5
#define MAX_BUF_SIZE 1024

int main(int argc, char *argv[])
{
    // Kiểm tra đầu vào
    if (argc != 4)
    {
        printf("Usage: %s <Port> <The-file-contains-the-greeting> <The-file-that-stores-the-content-the-client-sends-to>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Tạo socket
    int server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
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
    if (bind(server, (struct sockaddr *)&server_addr,
             sizeof(server_addr)) == -1)
    {
        perror("bind() failed");
        exit(EXIT_FAILURE);
    }

    // Lắng nghe kết nối từ client
    if (listen(server, MAX_CLIENT) == -1)
    {
        perror("listen() failed");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        printf("Waiting for client on %s %s\n",
               inet_ntoa(server_addr.sin_addr), argv[1]);

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

        // Đọc nội dung tệp tin chứa câu chào
        FILE *fp = fopen(argv[2], "r");
        if (fp == NULL)
        {
            perror("fopen() failed");
            exit(EXIT_FAILURE);
        }
        fseek(fp, 0, SEEK_END);
        long fsize = ftell(fp);
        rewind(fp);
        char *greeting = (char *)malloc(fsize + 1);
        if (greeting == NULL)
        {
            perror("malloc() failed");
            exit(EXIT_FAILURE);
        }
        memset(greeting, 0, fsize + 1);
        fread(greeting, fsize, 1, fp);
        fclose(fp);

        // Gửi câu chào đến client
        if (send(client, greeting, strlen(greeting), 0) == -1)
        {
            perror("send() failed");
            exit(EXIT_FAILURE);
        }
        free(greeting);

        // Nhận dữ liệu từ client
        char buf[MAX_BUF_SIZE];
        memset(buf, 0, MAX_BUF_SIZE);
        while (1)
        {
            // Nhận dữ liệu từ client và ghi vào tệp tin
            int bytes_received = recv(client, buf, MAX_BUF_SIZE, 0);
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

            // In dữ liệu nhận được
            printf("Received %ld bytes from client: %s", strlen(buf), buf);

            // Ghi dữ liệu vào tệp tin
            fp = fopen(argv[3], "a");
            if (fp == NULL)
            {
                perror("fopen() failed");
                exit(EXIT_FAILURE);
            }
            fprintf(fp, "%s", buf);
            fclose(fp);
        }
        // Đóng kết nối với client
        close(client);
    }

    // Đóng socket
    close(server);

    return 0;
}