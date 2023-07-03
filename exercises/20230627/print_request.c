#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define NUM_THREADS 4

void *client_proc(void *);

int main(int argc, char *argv[])
{
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1)
    {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;                // IPv4
    server_address.sin_addr.s_addr = htonl(INADDR_ANY); //
    server_address.sin_port = htons(PORT);
    socklen_t server_address_len = sizeof(server_address);

    if (bind(server_socket, (struct sockaddr *)&server_address, server_address_len) == -1)
    {
        perror("Failed to bind socket");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 1) == -1)
    {
        perror("Failed to listen for connections");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    pthread_t thread_id;
    for (int i = 0; i < NUM_THREADS; i++)
    {
        if (pthread_create(&thread_id, NULL, client_proc, (void *)&server_socket) != 0)
        {
            perror("Failed to create thread");
            continue;
        }
        sched_yield();
    }

    pthread_join(thread_id, NULL);

    return 0;
}

void *client_proc(void *param)
{
    int server_socket = *(int *)param;
    int client_socket;
    struct sockaddr_in client_address;
    socklen_t client_address_len = sizeof(client_address);

    while (1)
    {
        client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_address_len);
        if (client_socket == -1)
        {
            perror("Failed to accept connection");
            continue;
        }

        char buffer[BUFFER_SIZE];
        int bytes_read = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_read == -1)
        {
            perror("Failed to read from socket");
            continue;
        }
        buffer[bytes_read] = '\0';
        printf("Received request:\n%s\n", buffer);

        char *response = "HTTP/1.1 200 OK\r\n"
                         "Content-Type: text/html\r\n"
                         "\r\n"
                         "<html><body><h1>Hello, world!</h1></body></html>\r\n";
        int bytes_written = send(client_socket, response, strlen(response), 0);
        if (bytes_written == -1)
        {
            perror("Failed to write to socket");
            continue;
        }

        close(client_socket);
    }
    return NULL;
}