#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main()
{
    // Create a socket
    int client = socket(AF_INET, SOCK_STREAM, 0);
    if (client < 0)
    {
        perror("socket() failed");
        return 1;
    }

    // Thong tin server
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("0.0.0.0");
    server_addr.sin_port = htons(9000);

    return 0;
}