#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>

int main(int argc, char *argv[])
{
    struct addrinfo *res, *p;
    int ret = getaddrinfo(argv[1], "http", NULL, &res);
    if (ret != 0)
    {
        printf("Failed to get IP\n");
        return 1;
    }
    p = res;
    while (p != NULL)
    {
        if (p->ai_family == AF_INET)
        {
            printf("IPv4\n");
            struct sockaddr_in addr;
            memcpy(&addr, p->ai_addr, p->ai_addrlen);
            printf("IP: %s\n", inet_ntoa(addr.sin_addr));
        }
        else
        {
            printf("IPv6\n");
            char buf[64];
            struct sockaddr_in6 addr6;
            memcpy(&addr6, p->ai_addr, p->ai_addrlen);
            printf("IP: %s\n", inet_ntop(p->ai_family, &addr6.sin6_addr, buf, sizeof(buf)));
        }
        p = p->ai_next;
    }
    freeaddrinfo(res);
    return 0;
}
