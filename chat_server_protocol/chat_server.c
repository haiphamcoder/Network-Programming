#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>

#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10

typedef struct
{
    int socket;
    struct sockaddr_in address;
    char nickname[50];
    bool is_logged_in;
    bool is_running;
} Client;

typedef struct
{
    Client *clients[MAX_CLIENTS];
    int num_clients;
    int owner;
    char topic[50];
} ChatRoom;

typedef enum
{
    OK = 100,
    NICKNAME_IN_USE = 200,
    INVALID_NICKNAME = 201,
    UNKNOWN_NICKNAME = 202,
    DENIED = 203,
    ALREADY_LOGGED_IN = 503,
    UNKNOWN_ERROR = 999
} ResponseCode;

ChatRoom room = {.num_clients = 0, .owner = -1, .topic = ""};
pthread_mutex_t room_mutex = PTHREAD_MUTEX_INITIALIZER;

void send_response(int socket, ResponseCode code)
{
    char response[BUFFER_SIZE];
    memset(response, 0, BUFFER_SIZE);

    switch (code)
    {
    case OK:
        strcpy(response, "100 OK.\n");
        break;

    case NICKNAME_IN_USE:
        strcpy(response, "200 NICKNAME IN USE.\n");
        break;

    case INVALID_NICKNAME:
        strcpy(response, "201 INVALID NICKNAME.\n");
        break;

    case UNKNOWN_NICKNAME:
        strcpy(response, "202 UNKNOWN NICKNAME.\n");
        break;

    case DENIED:
        strcpy(response, "203 DENIED.\n");
        break;

    case ALREADY_LOGGED_IN:
        strcpy(response, "503 ALREADY LOGGED IN. QUIT FIRST.\n");
        break;

    case UNKNOWN_ERROR:
        strcpy(response, "999 UNKNOWN ERROR.\n");
        break;

    default:
        break;
    }

    if (send(socket, response, strlen(response), 0) < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            return;
        }
        else
        {
            perror("send() error");
            exit(EXIT_FAILURE);
        }
    }
}

bool validate_nickname(char *nickname)
{
    for (int i = 0; i < strlen(nickname); i++)
    {
        if (!((nickname[i] >= 'a' && nickname[i] <= 'z') || (nickname[i] >= '0' && nickname[i] <= '9')))
        {
            return false;
        }
    }
    return true;
}

void *client_handler(void *arg)
{
    Client *client = (Client *)arg;
    int client_socket = client->socket;
    struct sockaddr_in client_address = client->address;
    client->is_running = true;

    char buffer[BUFFER_SIZE];
    while (client->is_running)
    {
        memset(buffer, 0, BUFFER_SIZE);
        int received_bytes = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (received_bytes < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                continue;
            }
            else
            {
                perror("recv() error");
                exit(EXIT_FAILURE);
            }
        }
        else if (received_bytes == 0)
        {
            client->is_running = false;
            printf("Client from %s:%d disconnected.\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
            pthread_mutex_lock(&room_mutex);
            if (client->is_logged_in)
            {
                for (int i = 0; i < room.num_clients; i++)
                {
                    if (room.clients[i] == client)
                    {
                        for (int j = i; j < room.num_clients - 1; j++)
                        {
                            room.clients[j] = room.clients[j + 1];
                        }
                        room.num_clients--;
                        break;
                    }
                }

                if (room.num_clients > 0)
                {
                    if (room.owner == client->socket)
                    {
                        room.owner = room.clients[0]->socket;
                        char msg[BUFFER_SIZE];
                        sprintf(msg, "OP %s\n", client->nickname);
                        if (send(room.owner, msg, strlen(msg), 0) < 0)
                        {
                            if (errno == EAGAIN || errno == EWOULDBLOCK)
                            {
                                continue;
                            }
                            else
                            {
                                perror("send() error");
                                exit(EXIT_FAILURE);
                            }
                        }
                    }
                }
                else
                {
                    room.owner = -1;
                }

                char msg[BUFFER_SIZE];
                sprintf(msg, "QUIT %s\n", client->nickname);
                for (int i = 0; i < room.num_clients; i++)
                {
                    if (send(room.clients[i]->socket, msg, strlen(msg), 0) < 0)
                    {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                        {
                            continue;
                        }
                        else
                        {
                            perror("send() error");
                            exit(EXIT_FAILURE);
                        }
                    }
                }
            }
            pthread_mutex_unlock(&room_mutex);
            break;
        }

        // Xử lý thông điệp
        char command[20];
        char message[BUFFER_SIZE];
        memset(command, 0, 20);
        memset(message, 0, BUFFER_SIZE);
        int ret = sscanf(buffer, "%s %[^\n]", command, message);
        pthread_mutex_lock(&room_mutex);
        if (strcmp(command, "JOIN") == 0 && ret == 2)
        {
            if (!client->is_logged_in)
            {
                if (validate_nickname(message))
                {
                    // Kiểm tra nickname đã tồn tại hay chưa
                    bool is_nickname_in_use = false;
                    for (int i = 0; i < room.num_clients; i++)
                    {
                        if (strcmp(room.clients[i]->nickname, message) == 0)
                        {
                            is_nickname_in_use = true;
                            break;
                        }
                    }

                    if (!is_nickname_in_use)
                    {
                        // Gán nickname cho client
                        strcpy(client->nickname, message);

                        // Thêm client vào room
                        for (int i = 0; i < room.num_clients; i++)
                        {
                            if (send(room.clients[i]->socket, buffer, strlen(buffer), 0) < 0)
                            {
                                if (errno == EAGAIN || errno == EWOULDBLOCK)
                                {
                                    continue;
                                }
                                else
                                {
                                    perror("send() error");
                                    exit(EXIT_FAILURE);
                                }
                            }
                        }
                        room.clients[room.num_clients] = client;
                        room.num_clients++;

                        // Gửi phản hồi cho client
                        client->is_logged_in = true;
                        send_response(client_socket, OK);
                        if (room.num_clients == 1)
                        {
                            room.owner = client->socket;
                            char response[BUFFER_SIZE];
                            memset(response, 0, BUFFER_SIZE);
                            strcpy(response, "You are the owner of this room.\n");
                            if (send(client_socket, response, strlen(response), 0) < 0)
                            {
                                if (errno == EAGAIN || errno == EWOULDBLOCK)
                                {
                                    continue;
                                }
                                else
                                {
                                    perror("send() error");
                                    exit(EXIT_FAILURE);
                                }
                            }
                        }
                    }
                    else
                    {
                        send_response(client_socket, NICKNAME_IN_USE);
                    }
                }
                else
                {
                    send_response(client_socket, INVALID_NICKNAME);
                }
            }
            else
            {
                send_response(client_socket, ALREADY_LOGGED_IN);
            }
        }
        else if (strcmp(command, "MSG") == 0 && ret == 2)
        {
            char response[BUFFER_SIZE + 56];
            memset(response, 0, BUFFER_SIZE + 56);
            sprintf(response, "MSG %s %s\n", client->nickname, message);
            for (int i = 0; i < room.num_clients; i++)
            {
                if (room.clients[i] != client)
                {
                    if (send(room.clients[i]->socket, response, strlen(response), 0) < 0)
                    {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                        {
                            continue;
                        }
                        else
                        {
                            perror("send() error");
                            exit(EXIT_FAILURE);
                        }
                    }
                }
            }
            send_response(client_socket, OK);
        }
        else if (strcmp(command, "PMSG") == 0 && ret == 2)
        {
            char nickname[50];
            memset(nickname, 0, 50);
            ret = sscanf(message, "%s %[^\n]", nickname, message);
            if (ret == 2)
            {
                char response[BUFFER_SIZE + 57];
                memset(response, 0, BUFFER_SIZE + 57);
                sprintf(response, "PMSG %s %s\n", client->nickname, message);
                bool isFound = false;
                for (int i = 0; i < room.num_clients; i++)
                {
                    if (strcmp(room.clients[i]->nickname, nickname) == 0)
                    {
                        isFound = true;
                        if (send(room.clients[i]->socket, response, strlen(response), 0) < 0)
                        {
                            if (errno == EAGAIN || errno == EWOULDBLOCK)
                            {
                                continue;
                            }
                            else
                            {
                                perror("send() error");
                                exit(EXIT_FAILURE);
                            }
                        }
                        break;
                    }
                }
                if (isFound)
                {
                    send_response(client_socket, OK);
                }
                else
                {
                    send_response(client_socket, UNKNOWN_NICKNAME);
                }
            }
            else
            {
                send_response(client_socket, UNKNOWN_ERROR);
            }
        }
        else if (strcmp(command, "OP") == 0 && ret == 2)
        {
            if (client->socket == room.owner)
            {
                char response[BUFFER_SIZE];
                memset(response, 0, BUFFER_SIZE);
                sprintf(response, "OP %s.\n", client->nickname);
                bool isFound = false;
                for (int i = 0; i < room.num_clients; i++)
                {
                    if (room.clients[i] != client && strcmp(room.clients[i]->nickname, message) == 0)
                    {
                        isFound = true;
                        room.owner = room.clients[i]->socket;
                        if (send(room.clients[i]->socket, response, strlen(response), 0) < 0)
                        {
                            if (errno == EAGAIN || errno == EWOULDBLOCK)
                            {
                                continue;
                            }
                            else
                            {
                                perror("send() error");
                                exit(EXIT_FAILURE);
                            }
                        }
                        break;
                    }
                }
                if (isFound)
                {
                    send_response(client_socket, OK);
                }
                else
                {
                    send_response(client_socket, UNKNOWN_NICKNAME);
                }
            }
            else
            {
                send_response(client_socket, DENIED);
            }
        }
        else if (strcmp(command, "KICK") == 0 && ret == 2)
        {
            if (client->socket == room.owner)
            {
                char response[BUFFER_SIZE + 56];
                memset(response, 0, BUFFER_SIZE + 56);
                sprintf(response, "KICK %s %s\n", message, client->nickname);
                bool is_kicked = false;
                for (int i = 0; i < room.num_clients; i++)
                {
                    if (strcmp(room.clients[i]->nickname, message) == 0)
                    {
                        send_response(room.owner, OK);
                        is_kicked = true;
                        for (int j = 0; j < room.num_clients; j++)
                        {
                            if (room.clients[j] != client)
                            {
                                if (send(room.clients[j]->socket, response, strlen(response), 0) < 0)
                                {
                                    if (errno == EAGAIN || errno == EWOULDBLOCK)
                                    {
                                        continue;
                                    }
                                    else
                                    {
                                        perror("send() error");
                                        exit(EXIT_FAILURE);
                                    }
                                }
                            }
                        }

                        printf("Client from %s:%d disconnected\n", inet_ntoa(room.clients[i]->address.sin_addr), ntohs(room.clients[i]->address.sin_port));

                        if (room.clients[i]->socket == client->socket)
                        {
                            client->is_running = false;
                            if (room.num_clients > 1)
                            {
                                for (int j = i; j < room.num_clients - 1; j++)
                                {
                                    room.clients[j] = room.clients[j + 1];
                                }
                                room.owner = room.clients[0]->socket;
                                char msg[BUFFER_SIZE + 55];
                                memset(msg, 0, BUFFER_SIZE + 55);
                                sprintf(msg, "OP %s.\n", client->nickname);
                                if (send(room.owner, msg, strlen(msg), 0) < 0)
                                {
                                    if (errno == EAGAIN || errno == EWOULDBLOCK)
                                    {
                                        continue;
                                    }
                                    else
                                    {
                                        perror("send() error");
                                        exit(EXIT_FAILURE);
                                    }
                                }
                            }
                            else
                            {
                                room.owner = -1;
                            }
                            room.num_clients--;
                        }
                        else
                        {
                            room.clients[i]->is_running = false;
                            for (int j = i; j < room.num_clients - 1; j++)
                            {
                                room.clients[j] = room.clients[j + 1];
                            }
                            room.num_clients--;
                        }
                        break;
                    }
                }
                if (!is_kicked)
                {
                    send_response(client_socket, UNKNOWN_NICKNAME);
                }
            }
            else
            {
                send_response(client_socket, DENIED);
            }
        }
        else if (strcmp(command, "TOPIC") == 0 && ret == 2)
        {
            if (client->socket == room.owner)
            {
                strcpy(room.topic, message);
                char response[BUFFER_SIZE + 57];
                memset(response, 0, BUFFER_SIZE + 57);
                sprintf(response, "TOPIC %s %s\n", client->nickname, message);
                for (int i = 0; i < room.num_clients; i++)
                {
                    if (room.clients[i] != client)
                    {
                        if (send(room.clients[i]->socket, response, strlen(response), 0) < 0)
                        {
                            if (errno == EAGAIN || errno == EWOULDBLOCK)
                            {
                                continue;
                            }
                            else
                            {
                                perror("send() error");
                                exit(EXIT_FAILURE);
                            }
                        }
                    }
                }
                send_response(client_socket, OK);
            }
            else
            {
                send_response(client_socket, DENIED);
            }
        }
        else if (strcmp(command, "QUIT") == 0 && ret == 1)
        {
            client->is_running = false;
            send_response(client_socket, OK);

            // Xóa client khỏi room
            char response[BUFFER_SIZE];
            for (int i = 0; i < room.num_clients; i++)
            {
                if (room.clients[i] == client)
                {
                    for (int j = i; j < room.num_clients - 1; j++)
                    {
                        room.clients[j] = room.clients[j + 1];
                    }
                    room.num_clients--;
                    break;
                }
            }

            if (room.num_clients > 0)
            {
                if (room.owner == client->socket)
                {
                    room.owner = room.clients[0]->socket;
                    char msg[BUFFER_SIZE + 55];
                    memset(msg, 0, BUFFER_SIZE + 55);
                    sprintf(msg, "OP %s.\n", client->nickname);
                    if (send(room.owner, msg, strlen(msg), 0) < 0)
                    {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                        {
                            continue;
                        }
                        else
                        {
                            perror("send() error");
                            exit(EXIT_FAILURE);
                        }
                    }
                }
            }
            else
            {
                room.owner = -1;
            }

            memset(response, 0, BUFFER_SIZE);
            sprintf(response, "QUIT %s\n", client->nickname);
            for (int i = 0; i < room.num_clients; i++)
            {
                if (send(room.clients[i]->socket, response, strlen(response), 0) < 0)
                {
                    if (errno == EAGAIN || errno == EWOULDBLOCK)
                    {
                        continue;
                    }
                    else
                    {
                        perror("send() error");
                        exit(EXIT_FAILURE);
                    }
                }
            }

            printf("Client from %s:%d disconnected.\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
            pthread_mutex_unlock(&room_mutex);
        }
        else
        {
            send_response(client_socket, UNKNOWN_ERROR);
        }
        pthread_mutex_unlock(&room_mutex);
    }

    // Đóng kết nối
    close(client_socket);

    // Giải phóng bộ nhớ
    free(client);

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
        perror("socket() error");
        exit(EXIT_FAILURE);
    }

    // Thiết lập địa chỉ server
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(atoi(argv[1]));
    socklen_t server_address_len = sizeof(server_address);

    // Gán socket với địa chỉ server
    if (bind(server_socket, (struct sockaddr *)&server_address, server_address_len) < 0)
    {
        perror("bind() error");
        exit(EXIT_FAILURE);
    }

    // Lắng nghe kết nối
    if (listen(server_socket, 10) < 0)
    {
        perror("listen() error");
        exit(EXIT_FAILURE);
    }

    printf("Server is running at %s:%d\n", inet_ntoa(server_address.sin_addr), ntohs(server_address.sin_port));

    pthread_t tid;
    pthread_mutex_init(&room_mutex, NULL);

    while (true)
    {
        struct sockaddr_in client_address;
        memset(&client_address, 0, sizeof(client_address));
        socklen_t client_address_len = sizeof(client_address);
        int client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_address_len);
        if (client_socket < 0)
        {
            if (errno == EWOULDBLOCK)
            {
                sleep(1);
                continue;
            }
            else
            {
                perror("accept() error");
                exit(EXIT_FAILURE);
            }
            perror("accept() error");
            exit(EXIT_FAILURE);
        }
        else
        {
            printf("New client connected from %s:%d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

            // Kiểm tra số lượng client
            if (room.num_clients >= MAX_CLIENTS)
            {
                printf("Room is full\n");
                close(client_socket);
                continue;
            }
            else
            {
                // Tạo một client mới và thêm vào room
                Client *client = (Client *)malloc(sizeof(Client));
                memset(client, 0, sizeof(Client));
                client->socket = client_socket;
                client->address = client_address;
                strcpy(client->nickname, "");
                client->is_logged_in = false;
                client->is_running = false;

                fcntl(client_socket, F_SETFL, O_NONBLOCK);
                pthread_create(&tid, NULL, client_handler, (void *)client);
            }
        }
    }

    // Đóng socket
    close(server_socket);

    // Hủy mutex
    pthread_mutex_destroy(&room_mutex);

    return 0;
}