#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#define SERVER_DOMAIN "vnexpress.net"
#define SERVER_PORT 443

int main()
{
    // Tạo socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Lấy IPv4 từ domain
    struct hostent* host = gethostbyname(SERVER_DOMAIN);
    if (host == NULL)
    {
        perror("gethostbyname");
        exit(EXIT_FAILURE);
    }

    // Tạo địa chỉ server
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = *(long*)(host->h_addr);
    
    // Kết nối đến server
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    SSL_library_init();

    const SSL_METHOD* method = TLS_client_method();

    // Tạo SSL context
    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
    if (ctx == NULL)
    {
        perror("SSL_CTX_new");
        exit(EXIT_FAILURE);
    }

    // Thiết lập kết nối SSL
    SSL* ssl = SSL_new(ctx);
    if (ssl == NULL)
    {
        perror("SSL_new");
        exit(EXIT_FAILURE);
    }

    // Gán kết nối SSL với socket
    if (!SSL_set_fd(ssl, sockfd))
    {
        perror("SSL_set_fd");
        exit(EXIT_FAILURE);
    }

    // Thực hiện SSL handshake
    if (SSL_connect(ssl) != 1)
    {
        perror("SSL_connect");
        exit(EXIT_FAILURE);
    }

    // Gửi request
    char request[1024];
    sprintf(request, "GET / HTTP/1.1\r\nHost: %s\r\n\r\n", SERVER_DOMAIN);
    SSL_write(ssl, request, strlen(request));

    // Nhận response
    char response[1024];
    SSL_read(ssl, response, sizeof(response));
    printf("%s\n", response);

    // Đóng kết nối
    SSL_shutdown(ssl);
    SSL_free(ssl);
    SSL_CTX_free(ctx);
    close(sockfd);

    return 0;
}