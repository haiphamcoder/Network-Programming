#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024
#define DEFAULT_PORT 8080
#define MAX_FILES 100
#define MAX_CLIENTS 10
#define DT_DIR 4

void send_file_list(int client_socket);
void send_file_content(int client_socket, const char *filename);

int main(int argc, char *argv[])
{
    // Kiểm tra đầu vào
    if (argc != 2)
    {
        printf("Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Tạo socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0)
    {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    // Thiết lập địa chỉ server
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(atoi(argv[1]));
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);

    // Gán địa chỉ server vào socket
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        perror("bind() failed");
        exit(1);
    }

    // Listen for incoming connections
    if (listen(server_socket, MAX_CLIENTS) < 0)
    {
        perror("listen() failed");
        exit(1);
    }

    while (1)
    {
        // Thiết lập địa chỉ client
        struct sockaddr_in client_address;
        memset(&client_address, 0, sizeof(client_address));
        socklen_t client_address_length = sizeof(client_address);

        // Chấp nhận kết nối từ client
        int client_socket = accept(server_socket, (struct sockaddr *)&client_address, (socklen_t *)&client_address_length);
        if (client_socket < 0)
        {
            perror("accept() failed");
            exit(1);
        }

        // Tạo một tiến trình con để xử lý client
        pid_t pid = fork();
        if (pid < 0)
        {
            perror("fork() failed");
            exit(1);
        }
        else if (pid == 0)
        {
            // Child process
            close(server_socket);

            // Gửi danh sách file cho client
            send_file_list(client_socket);

            // Nhận tên file từ client
            char buffer[BUFFER_SIZE];
            memset(buffer, 0, BUFFER_SIZE);
            int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
            if (bytes_received < 0)
            {
                perror("recv() failed");
                exit(1);
            }

            // Xóa ký tự xuống dòng cuối chuỗi
            if (buffer[strlen(buffer) - 1] == '\n')
                buffer[strlen(buffer) - 1] = '\0';

            // Gửi nội dung file cho client
            send_file_content(client_socket, buffer);

            // Đóng kết nối
            close(client_socket);
            exit(0);
        }
        else
        {
            // Parent process
            close(client_socket);
        }
    }

    // Đóng socket
    close(server_socket);

    return 0;
}

void send_file_list(int client_socket)
{
    DIR *dir;
    struct dirent *ent;
    char file_list[BUFFER_SIZE];
    memset(file_list, 0, BUFFER_SIZE);

    // Mở thư mục hiện tại
    dir = opendir(".");
    if (dir == NULL)
    {
        perror("opendir() failed");
        exit(1);
    }
    int file_count = 0;
    while ((ent = readdir(dir)) != NULL && file_count < MAX_FILES)
    {
        // Bỏ qua thư mục và file ẩn
        if (ent->d_type == DT_DIR || ent->d_name[0] == '.')
            continue;

        // Thêm tên file vào danh sách
        strcat(file_list, ent->d_name);
        strcat(file_list, "\r\n");
        file_count++;
    }

    closedir(dir);

    if (file_count > 0)
    {
        // Gửi danh sách file cho client
        char response[BUFFER_SIZE];
        sprintf(response, "OK %d\r\n%s\r\n\r\n", file_count, file_list);
        if (send(client_socket, response, strlen(response), 0) < 0)
        {
            perror("send() failed");
            exit(1);
        }
    }
    else
    {
        // Gửi thông báo lỗi cho client
        char response[] = "ERROR No files to download \r\n";
        if (send(client_socket, response, strlen(response), 0) < 0)
        {
            perror("send() failed");
            exit(1);
        }
        close(client_socket);
        exit(0);
    }
}

void send_file_content(int client_socket, const char *filename)
{
    FILE *file = fopen(filename, "rb");
    if (file == NULL)
    {
        // Gửi thông báo lỗi cho client
        char response[] = "ERROR File not found \r\n";
        if (send(client_socket, response, strlen(response), 0) < 0)
        {
            perror("send() failed");
            exit(1);
        }
        return;
    }

    // Tính kích thước file
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Cấp phát bộ nhớ cho buffer
    char *file_content = (char *)malloc(file_size);
    if (file_content == NULL)
    {
        perror("malloc() failed");
        exit(1);
    }

    // Đọc nội dung file vào buffer
    size_t bytes_read = fread(file_content, 1, file_size, file);
    if (bytes_read != file_size)
    {
        perror("fread() failed");
        exit(1);
    }

    // Gửi OK response với kích thước file
    char response[BUFFER_SIZE];
    sprintf(response, "OK %ld\r\n", file_size);
    if (send(client_socket, response, strlen(response), 0) < 0)
    {
        perror("send() failed");
        exit(1);
    }

    // Gửi nội dung file cho client
    if (send(client_socket, file_content, file_size, 0) < 0)
    {
        perror("send() failed");
        exit(1);
    }

    // Đóng file và giải phóng bộ nhớ
    fclose(file);
    free(file_content);
}
