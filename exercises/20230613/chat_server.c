#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define MAX_CLIENTS 10

typedef struct
{
    int client_socket;
    int partner_socket;
} ClientPair;

ClientPair *clients[MAX_CLIENTS];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *client_handler(void *arg)
{
    ClientPair *client_pair = (ClientPair *)arg;
    int client_socket = client_pair->client_socket;
    int partner_socket = client_pair->partner_socket;

    char buffer[1024];
    int read_size;

    while ((read_size = recv(client_socket, buffer, sizeof(buffer), 0)) > 0)
    {
        buffer[read_size] = '\0';
        send(partner_socket, buffer, strlen(buffer), 0);
        memset(buffer, 0, sizeof(buffer));
    }

    if (read_size == 0)
    {
        // Khi một client ngắt kết nối, đóng cả hai socket
        close(client_socket);
        close(partner_socket);

        pthread_mutex_lock(&mutex);
        // Tìm vị trí của client trong danh sách và giải phóng nó
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            if (clients[i] == client_pair)
            {
                free(clients[i]);
                clients[i] = NULL;
                break;
            }
        }
        pthread_mutex_unlock(&mutex);
    }

    return NULL;
}

int main(int argc, char *argv[])
{
    // Kiểm tra đầu vào
    if (argc != 2)
    {
        printf("Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Khởi tạo socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0)
    {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    // Khởi tạo mảng chứa danh sách các cặp client
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        clients[i] = NULL;
    }

    // Thiết lập thông tin của server
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[1])); // Chuyển đổi cổng từ kiểu chuỗi sang kiểu số nguyên
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // Gắn socket với địa chỉ và cổng
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("bind() failed");
        exit(EXIT_FAILURE);
    }

    // Lắng nghe kết nối từ client
    if (listen(server_socket, MAX_CLIENTS) < 0)
    {
        perror("listen() failed");
        exit(EXIT_FAILURE);
    }

    printf("Waiting for incoming connections on %s:%d...\n", inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));

    while (1)
    {
        // Chấp nhận kết nối từ client
        struct sockaddr_in client_addr;
        memset(&client_addr, 0, sizeof(client_addr));
        socklen_t client_addr_size = sizeof(client_addr);
        int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_size);
        if (client_socket == -1)
        {
            perror("accept() failed");
            exit(EXIT_FAILURE);
        }

        printf("Client from %s:%d connected\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        pthread_mutex_lock(&mutex);

        int available_index = -1;

        // Tìm vị trí trống trong danh sách các cặp client
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            if (clients[i] == NULL)
            {
                available_index = i;
                break;
            }
        }

        if (available_index == -1)
        {
            // Nếu không có vị trí trống, đóng kết nối với client mới
            printf("No available slot. Closing connection...\n");
            close(client_socket);
        }
        else
        {
            // Tạo một cặp client và lưu nó vào danh sách
            ClientPair *new_pair = (ClientPair *)malloc(sizeof(ClientPair));
            new_pair->client_socket = client_socket;
            new_pair->partner_socket = -1;
            clients[available_index] = new_pair;

            printf("Client added to queue. Number of clients in queue: %d\n", available_index % 2 + 1);

            if (available_index % 2 != 0)
            {
                // Nếu đủ 2 client trong hàng đợi, ghép cặp và tạo luồng xử lý riêng
                int partner_index = available_index - 1;
                clients[available_index]->partner_socket = clients[partner_index]->client_socket;
                clients[partner_index]->partner_socket = clients[available_index]->client_socket;

                pthread_t thread;
                pthread_create(&thread, NULL, client_handler, (void *)clients[available_index]);
                pthread_detach(thread);

                pthread_create(&thread, NULL, client_handler, (void *)clients[partner_index]);
                pthread_detach(thread);

                printf("Client pair created. Number of clients in queue: %d\n", available_index % 2 - 1);
            }
        }

        pthread_mutex_unlock(&mutex);
    }

    return 0;
}
