#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define NUM_THREADS sysconf(_SC_NPROCESSORS_ONLN)
#define MAX_CLIENTS 10

void send_response(int, int, const char *);
void send_file_content(int, const char *);
char *get_mime_type(const char *);
void send_directory_listing(int, const char *);
void handle_request(int);
void *thread_func(void *);
int main(int argc, char *argv[])
{
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
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(PORT);
    socklen_t server_address_len = sizeof(server_address);

    // Gán địa chỉ cho socket
    if (bind(server_socket, (struct sockaddr *)&server_address, server_address_len) < 0)
    {
        perror("bind() failed");
        exit(EXIT_FAILURE);
    }

    // Lắng nghe kết nối
    if (listen(server_socket, MAX_CLIENTS) < 0)
    {
        perror("listen() failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    // Tạo thread pool
    pthread_t thread_pool[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++)
    {
        if (pthread_create(&thread_pool[i], NULL, thread_func, (void *)&server_socket) != 0)
        {
            perror("pthread_create() failed");
            continue;
        }
    }

    // Chờ các thread kết thúc
    for (int i = 0; i < NUM_THREADS; i++)
    {
        pthread_join(thread_pool[i], NULL);
    }

    // Đóng socket
    close(server_socket);

    return 0;
}

void *thread_func(void *param)
{
    int server_socket = *(int *)param;
    while (1)
    {
        // Chấp nhận kết nối
        struct sockaddr_in client_address;
        memset(&client_address, 0, sizeof(client_address));
        socklen_t client_address_len = sizeof(client_address);
        int client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_address_len);
        if (client_socket < 0)
        {
            perror("accept() failed");
            continue;
        }

        // Xử lý request
        handle_request(client_socket);

        // Đóng socket
        close(client_socket);
    }
    return NULL;
}

void handle_request(int client_socket)
{
    // Nhận request từ client
    char request[BUFFER_SIZE];
    memset(request, 0, BUFFER_SIZE);
    int bytes_received = recv(client_socket, request, BUFFER_SIZE, 0);
    if (bytes_received < 0)
    {
        perror("recv() failed");
        return;
    }
    request[bytes_received] = '\0';

    // Xử lý request
    char *request_path = strtok(request, " ");
    request_path = strtok(NULL, " ");

    // Kiểm tra request path
    if (request_path == NULL)
    {
        send_response(client_socket, 400, "<h1>400 Bad Request</h1>");
        return;
    }

    // Xử lý request path
    char *path = malloc(strlen(request_path) + 1);
    memset(path, 0, strlen(request_path) + 1);
    strcpy(path, request_path);

    // Kiểm tra path
    if (strcmp(path, "/") == 0)
    {
        send_directory_listing(client_socket, ".");
    }
    else
    {
        // Xóa ký tự '/' đầu path
        memmove(path, path + 1, strlen(path));

        // Kiểm tra path có tồn tại không
        struct stat path_info;
        if (stat(path, &path_info) < 0) // Path không tồn tại
        {
            // Gửi response 404 Not Found
            send_response(client_socket, 404, "<h1>404 Not Found</h1>");
            return;
        }
        else if (S_ISDIR(path_info.st_mode)) // Path là thư mục
        {
            send_directory_listing(client_socket, path);
        }
        else if (S_ISREG(path_info.st_mode)) // Path là file
        {
            send_file_content(client_socket, path);
        }
    }
    return;
}

char *get_mime_type(const char *file_name)
{
    char *file_ext = strrchr(file_name, '.');
    if (file_ext == NULL)
    {
        return "text/plain";
    }
    if (strcmp(file_ext, ".html") == 0 || strcmp(file_ext, ".htm") == 0)
    {
        return "text/html";
    }
    if (strcmp(file_ext, ".jpg") == 0 || strcmp(file_ext, ".jpeg") == 0)
    {
        return "image/jpeg";
    }
    if (strcmp(file_ext, ".gif") == 0)
    {
        return "image/gif";
    }
    if (strcmp(file_ext, ".png") == 0)
    {
        return "image/png";
    }
    if (strcmp(file_ext, ".css") == 0)
    {
        return "text/css";
    }
    if (strcmp(file_ext, ".au") == 0)
    {
        return "audio/basic";
    }
    if (strcmp(file_ext, ".wav") == 0)
    {
        return "audio/wav";
    }
    if (strcmp(file_ext, ".avi") == 0)
    {
        return "video/x-msvideo";
    }
    if (strcmp(file_ext, ".mpeg") == 0 || strcmp(file_ext, ".mpg") == 0)
    {
        return "video/mpeg";
    }
    if (strcmp(file_ext, ".mp3") == 0)
    {
        return "audio/mpeg";
    }
    return "text/plain";
}

void send_directory_listing(int client_socket, const char *path)
{
    // Mở thư mục
    DIR *dir = opendir(path);
    if (dir == NULL)
    {
        send_response(client_socket, 404, "<h1>404 Not Found</h1>");
        return;
    }

    // Đọc thư mục
    struct dirent *dir_entry;
    char *response = malloc(1);
    memset(response, 0, 1);

    while ((dir_entry = readdir(dir)) != NULL)
    {
        // Bỏ qua thư mục cha và thư mục hiện tại
        if (strcmp(dir_entry->d_name, ".") == 0 || strcmp(dir_entry->d_name, "..") == 0)
        {
            continue;
        }

        // Tạo đường dẫn tuyệt đối cho file và thư mục
        int len_file_path = strlen(path) + strlen(dir_entry->d_name) + 2;
        char file_path[len_file_path];
        memset(file_path, 0, len_file_path);
        sprintf(file_path, "%s/%s", path, dir_entry->d_name);

        // Định dạng lại đường dẫn
        char temp[BUFFER_SIZE];
        memset(temp, 0, BUFFER_SIZE);
        if (dir_entry->d_type == 4) // DT_DIR = 4
        {
            sprintf(temp, "<p><strong><a href=\"/%s\">%s/</a></strong></p>", file_path, dir_entry->d_name);
        }
        else if (dir_entry->d_type == 8) // DT_REG = 8
        {
            sprintf(temp, "<p><em><a href=\"/%s\">%s</a></em></p>", file_path, dir_entry->d_name);
        }

        // Thêm temp vào response
        response = realloc(response, strlen(response) + strlen(temp) + 1);
        strcat(response, temp);
    }

    // Đóng thư mục
    closedir(dir);

    // Gửi response
    char header[BUFFER_SIZE];
    memset(header, 0, BUFFER_SIZE);
    sprintf(header, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: %ld\r\n\r\n", strlen(response));
    send(client_socket, header, strlen(header), 0);
    send(client_socket, response, strlen(response), 0);

    free(response);
    return;
}

void send_file_content(int client_socket, const char *file_path)
{
    // Mở file
    FILE *file = fopen(file_path, "rb");
    if (file == NULL)
    {
        send_response(client_socket, 501, "<h1>501 Internal Server Error</h1>");
        return;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    const char *mime_type = get_mime_type(file_path);
    char header[BUFFER_SIZE];
    memset(header, 0, BUFFER_SIZE);
    sprintf(header, "HTTP/1.1 200 OK\r\nContent-Type: %s; charset=utf-8;\r\nContent-Length: %ld\r\n\r\n", mime_type, file_size);
    if (send(client_socket, header, strlen(header), 0) < 0)
    {
        perror("send() failed");
        return;
    }

    // Đọc file và gửi nội dung file
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    size_t bytes_read = 0;
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0)
    {
        if (send(client_socket, buffer, bytes_read, 0) < 0)
        {
            perror("send() failed");
            return;
        }
        memset(buffer, 0, BUFFER_SIZE);
    }

    // Đóng file
    fclose(file);

    return;
}

void send_response(int client_socket, int status_code, const char *message_body)
{
    char *status_phrase;
    switch (status_code)
    {
    case 200:
        status_phrase = "OK";
        break;
    case 404:
        status_phrase = "Not Found";
        break;
    case 400:
        status_phrase = "Bad Request";
        break;
    case 501:
        status_phrase = "Internal Server Error";
        break;
    default:
        status_phrase = "Not Implemented";
        break;
    }

    char response[BUFFER_SIZE];
    memset(response, 0, BUFFER_SIZE);
    sprintf(response, "HTTP/1.1 %d %s\r\nContent-Type: text/html\r\nContent-Length: %ld\r\n\r\n%s", status_code, status_phrase, strlen(message_body), message_body);
    if (send(client_socket, response, strlen(response), 0) < 0)
    {
        perror("send() failed");
        return;
    }
    return;
}