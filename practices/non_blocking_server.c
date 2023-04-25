#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>

int main()
{
    // Create a socket
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0)
    {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    // Set socket to non-blocking
    int flags = fcntl(listener, F_GETFL, 0);
    fcntl(listener, F_SETFL, flags | O_NONBLOCK);

    // Bind socket to port
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(9000);
    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("bind() failed");
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(listener, 1) < 0)
    {
        perror("listen() failed");
        exit(EXIT_FAILURE);
    }

    // Accept connections
    while (1)
    {
        int client = accept(listener, NULL, NULL);
        if (client < 0)
        {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
            {
                // No connections available
                sleep(1);
            }
            else
            {
                perror("accept() failed");
                exit(EXIT_FAILURE);
            }
        }
        else
        {

        }
    }

    return 0;
}
