#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAX_CLIENT 10
#define BUFFER_SIZE 1024

typedef enum
{
    OK = 100,
    NICKNAME_IN_USE = 200,
    INVALID_NICKNAME = 201,
    UNKNOWN_NICKNAME = 202,
    DENIED = 203,
    UNKNOWN_ERROR = 999
} ResponseCode;

typedef struct
{
    int socket;
    char nickname[20];
    bool is_logged_in;
} Client;

typedef struct
{
    char topic[50];
    Client *clients[MAX_CLIENT];
    int num_clients;
    Client *owner;
} ChatRoom;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
ChatRoom room = {topic : "Default", num_clients : 0, owner : NULL};

void send_response(int socket, ResponseCode code)
{
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);

    switch (code)
    {
    case OK:
        strcpy(buffer, "100 OK");
        break;
    case NICKNAME_IN_USE:
        strcpy(buffer, "200 NICKNAME IN USE");
        break;

    case INVALID_NICKNAME:
        strcpy(buffer, "201 INVALID NICKNAME");
        break;

    case UNKNOWN_NICKNAME:
        strcpy(buffer, "202 UNKNOWN NICKNAME");
        break;

    case DENIED:
        strcpy(buffer, "203 DENIED");
        break;

    case UNKNOWN_ERROR:
        strcpy(buffer, "999 UNKNOWN ERROR");
        break;

    default:
        break;
    }

    if (send(socket, buffer, BUFFER_SIZE, 0) < 0)
    {
        perror("send() failed");
        exit(EXIT_FAILURE);
    }
}

void *handle_client(void *arg){
    Client *client = (Client *)arg;
    
}

int main(int argc, char *argv[])
{
    
    return 0;
}