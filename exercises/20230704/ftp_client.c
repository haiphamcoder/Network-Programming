#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 1024

void error_handling(const char *);
void send_command(int, const char *);
void receive_response(int);
void login(int, const char *, const char *);
void list_directory(int);
void create_directory(int, const char *);
void rename_directory(int, const char *, const char *);
void delete_directory(int, const char *);
void rename_file(int, const char *, const char *);
void delete_file(int, const char *);
void download_file(int, const char *);
void upload_file(int, const char *);

int main(int argc, char *argv[])
{
    // Kiểm tra đầu vào
    if (argc != 3)
    {
        printf("Usage: %s <IP> <port>\n", argv[0]);
        exit(1);
    }

    // Khởi tạo socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        error_handling("socket() failed");
    }

    // Thiết lập địa chỉ server
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    server_addr.sin_port = htons(atoi(argv[2]));
    socklen_t server_addr_size = sizeof(server_addr);

    // Kết nối đến server
    if (connect(sockfd, (struct sockaddr *)&server_addr, server_addr_size) < 0)
    {
        error_handling("connect() failed");
    }

    // Nhận phản hồi đầu tiên từ server
    receive_response(sockfd);

    // Đăng nhập vào máy chủ FTP
    char username[BUF_SIZE];
    char password[BUF_SIZE];
    printf("Nhập tên đăng nhập: ");
    scanf("%s", username);
    printf("Nhập mật khẩu: ");
    scanf("%s", password);
    login(sockfd, username, password);

    int choice = 0;

    do
    {
        printf("\n-------- FTP Client --------\n");
        printf("1. Hiển thị nội dung thư mục\n");
        printf("2. Tạo thư mục\n");
        printf("3. Đổi tên thư mục\n");
        printf("4. Xóa thư mục\n");
        printf("5. Đổi tên file\n");
        printf("6. Xóa file\n");
        printf("7. Tải file lên server\n");
        printf("8. Tải file từ server\n");
        printf("0. Thoát\n");
        printf("Nhập lựa chọn của bạn: ");
        scanf("%d", &choice);

        switch (choice)
        {
        case 1:
            list_directory(sockfd);
            break;
        case 2:
        {
            char dir_name[BUF_SIZE];
            printf("Nhập tên thư mục: ");
            scanf("%s", dir_name);
            create_directory(sockfd, dir_name);
            break;
        }
        case 3:
        {
            char old_name[BUF_SIZE];
            char new_name[BUF_SIZE];
            printf("Nhập tên thư mục cũ: ");
            scanf("%s", old_name);
            printf("Nhập tên thư mục mới: ");
            scanf("%s", new_name);
            rename_directory(sockfd, old_name, new_name);
            break;
        }
        case 4:
        {
            char dir_name[BUF_SIZE];
            printf("Nhập tên thư mục: ");
            scanf("%s", dir_name);
            delete_directory(sockfd, dir_name);
            break;
        }
        case 5:
        {
            char old_name[BUF_SIZE];
            char new_name[BUF_SIZE];
            printf("Nhập tên file cũ: ");
            scanf("%s", old_name);
            printf("Nhập tên file mới: ");
            scanf("%s", new_name);
            rename_file(sockfd, old_name, new_name);
            break;
        }
        case 6:
        {
            char file_name[BUF_SIZE];
            printf("Nhập tên file: ");
            scanf("%s", file_name);
            delete_file(sockfd, file_name);
            break;
        }
        case 7:
        {
            char file_name[BUF_SIZE];
            printf("Nhập tên file: ");
            scanf("%s", file_name);
            upload_file(sockfd, file_name);
            break;
        }
        case 8:
        {
            char file_name[BUF_SIZE];
            printf("Nhập tên file: ");
            scanf("%s", file_name);
            download_file(sockfd, file_name);
            break;
        }
        case 0:
            break;
        default:
            printf("Lựa chọn không hợp lệ\n");
            break;
        }
    } while (choice != 0);

    // Đóng kết nối
    send_command(sockfd, "QUIT\r\n");
    close(sockfd);

    return 0;
}

void error_handling(const char *message)
{
    perror(message);
    exit(EXIT_FAILURE);
}

void send_command(int sockfd, const char *command)
{
    if(send(sockfd, command, strlen(command), 0) < 0)
    {
        error_handling("send() failed");
    }
}

void receive_response(int sockfd)
{
    char response[BUF_SIZE];
    memset(response, 0, BUF_SIZE);
    if(recv(sockfd, response, BUF_SIZE, 0) < 0)
    {
        error_handling("recv() failed");
    }
    printf("%s", response);
}

void login(int sockfd, const char *username, const char *password)
{
    char command[BUF_SIZE];
    memset(command, 0, BUF_SIZE);
    sprintf(command, "USER %s\r\n", username);
    send_command(sockfd, command);
    receive_response(sockfd);

    memset(command, 0, BUF_SIZE);
    sprintf(command, "PASS %s\r\n", password);
    send_command(sockfd, command);
    receive_response(sockfd);
}

void list_directory(int sockfd)
{
    send_command(sockfd, "LIST\r\n");
    receive_response(sockfd);
}

void create_directory(int sockfd, const char *dir_name)
{
    char command[BUF_SIZE];
    memset(command, 0, BUF_SIZE);
    sprintf(command, "MKD %s\r\n", dir_name);
    send_command(sockfd, command);
    receive_response(sockfd);
}

void rename_directory(int sockfd, const char *old_name, const char *new_name)
{
    char command[BUF_SIZE];
    memset(command, 0, BUF_SIZE);
    sprintf(command, "RNFR %s\r\n", old_name);
    send_command(sockfd, command);
    receive_response(sockfd);

    memset(command, 0, BUF_SIZE);
    sprintf(command, "RNTO %s\r\n", new_name);
    send_command(sockfd, command);
    receive_response(sockfd);
}

void delete_directory(int sockfd, const char *dir_name)
{
    char command[BUF_SIZE];
    memset(command, 0, BUF_SIZE);
    sprintf(command, "RMD %s\r\n", dir_name);
    send_command(sockfd, command);
    receive_response(sockfd);
}

void rename_file(int sockfd, const char *old_name, const char *new_name)
{
    char command[BUF_SIZE];
    memset(command, 0, BUF_SIZE);
    sprintf(command, "RNFR %s\r\n", old_name);
    send_command(sockfd, command);
    receive_response(sockfd);

    memset(command, 0, BUF_SIZE);
    sprintf(command, "RNTO %s\r\n", new_name);
    send_command(sockfd, command);
    receive_response(sockfd);
}

void delete_file(int sockfd, const char *file_name)
{
    char command[BUF_SIZE];
    memset(command, 0, BUF_SIZE);
    sprintf(command, "DELE %s\r\n", file_name);
    send_command(sockfd, command);
    receive_response(sockfd);
}

void upload_file(int sockfd, const char *file_name)
{
    char command[BUF_SIZE];
    memset(command, 0, BUF_SIZE);
    sprintf(command, "STOR %s\r\n", file_name);
    send_command(sockfd, command);
    receive_response(sockfd);
}

void download_file(int sockfd, const char *file_name)
{
    char command[BUF_SIZE];
    memset(command, 0, BUF_SIZE);
    sprintf(command, "RETR %s\r\n", file_name);
    send_command(sockfd, command);
    receive_response(sockfd);
}