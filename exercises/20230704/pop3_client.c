#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define MAX_BUFFER_SIZE 1024

// Hàm kết nối tới server POP3
int connectToServer(const char* serverIP, int serverPort) {
    int sockfd;
    struct sockaddr_in serverAddr;

    // Tạo socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Không thể tạo socket");
        exit(1);
    }

    // Thiết lập địa chỉ server
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);
    if (inet_pton(AF_INET, serverIP, &(serverAddr.sin_addr)) <= 0) {
        perror("Địa chỉ server không hợp lệ");
        exit(1);
    }

    // Kết nối tới server
    if (connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Không thể kết nối tới server");
        exit(1);
    }

    return sockfd;
}

// Hàm đăng nhập vào tài khoản POP3
void login(int sockfd, const char* username, const char* password) {
    char buffer[MAX_BUFFER_SIZE];

    // Nhận phản hồi từ server
    memset(buffer, 0, sizeof(buffer));
    recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    printf("%s\n", buffer);

    // Gửi lệnh USER
    snprintf(buffer, sizeof(buffer), "USER %s\r\n", username);
    send(sockfd, buffer, strlen(buffer), 0);

    // Nhận phản hồi từ server
    memset(buffer, 0, sizeof(buffer));
    recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    printf("%s\n", buffer);

    // Gửi lệnh PASS
    snprintf(buffer, sizeof(buffer), "PASS %s\r\n", password);
    send(sockfd, buffer, strlen(buffer), 0);

    // Nhận phản hồi từ server
    memset(buffer, 0, sizeof(buffer));
    recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    printf("%s\n", buffer);
}

// Hàm hiển thị danh sách email
void listEmails(int sockfd) {
    char buffer[MAX_BUFFER_SIZE];

    // Gửi lệnh LIST
    snprintf(buffer, sizeof(buffer), "LIST\r\n");
    send(sockfd, buffer, strlen(buffer), 0);

    // Nhận phản hồi từ server
    memset(buffer, 0, sizeof(buffer));
    recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    printf("%s\n", buffer);
}

// Hàm hiển thị nội dung email
void displayEmail(int sockfd, int emailNumber) {
    char buffer[MAX_BUFFER_SIZE];

    // Gửi lệnh RETR
    snprintf(buffer, sizeof(buffer), "RETR %d\r\n", emailNumber);
    send(sockfd, buffer, strlen(buffer), 0);

    // Nhận phản hồi từ server
    memset(buffer, 0, sizeof(buffer));
    recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    printf("%s\n", buffer);
}

int main() {
    const char* serverIP = "pop.example.com";
    int serverPort = 110;
    const char* username = "your_username";
    const char* password = "your_password";

    int sockfd = connectToServer(serverIP, serverPort);
    login(sockfd, username, password);
    listEmails(sockfd);
    displayEmail(sockfd, 1);

    close(sockfd);
    return 0;
}
