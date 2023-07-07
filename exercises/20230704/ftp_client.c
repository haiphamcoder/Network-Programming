#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdbool.h>

#define BUF_SIZE 1024

struct sockaddr_in *get_data_sock_addr(int, int);

void delete_file(int, char *);
void download_file(int, int, char *);
void upload_file(int, int, char *);
void rename_file(int, char *, char *);
void rename_directory(int, char *, char *);
void delete_directory(int, char *);
void create_directory(int, char *);
void listing_directory(int, int);
bool login(int);

int main(int argc, char *argv[])
{
    // Kiểm tra đầu vào
    if (argc != 3)
    {
        printf("Usage: %s <IP> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Khởi tạo control socket
    int control_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (control_sock < 0)
    {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    // Thiết lập địa cho control socket
    struct sockaddr_in control_addr;
    memset(&control_addr, 0, sizeof(control_addr));
    control_addr.sin_family = AF_INET;
    control_addr.sin_addr.s_addr = inet_addr(argv[1]);
    control_addr.sin_port = htons(atoi(argv[2]));
    socklen_t control_addr_len = sizeof(control_addr);

    // Kết nối tới server
    if (connect(control_sock, (struct sockaddr *)&control_addr, control_addr_len) < 0)
    {
        perror("connect() failed");
        exit(EXIT_FAILURE);
    }

    // Nhận thông báo đầu tiên từ server
    char buf[BUF_SIZE];
    memset(buf, 0, BUF_SIZE);
    if (recv(control_sock, buf, BUF_SIZE, 0) < 0)
    {
        perror("recv() failed");
        exit(EXIT_FAILURE);
    }
    int reply_code = atoi(buf);
    if (reply_code != 220)
    {
        printf("Error: %s\n", buf);
        exit(EXIT_FAILURE);
    }

    int choice = 0;
    bool is_continue = true;
    while (is_continue)
    {
        printf("\n");
        printf("-------- FTP Client --------\n");
        printf("1. Login\n");
        printf("2. Quit\n");
        printf("----------------------------\n");
        printf("Enter your choice (1-2): ");
        scanf("%d", &choice);
        getchar();

        switch (choice)
        {
        case 1:
            if (!login(control_sock))
            {
                printf("Login failed\n");
            }
            else
            {
                printf("Login successfully\n");
                int mode = 0;
                while (true)
                {
                    printf("Enter the mode (1-active, 2-passive): ");
                    scanf("%d", &mode);
                    getchar();

                    if (mode == 1 || mode == 2)
                    {
                        break;
                    }
                    else
                    {
                        printf("Invalid mode\n");
                    }
                }

                while (true)
                {
                    printf("\n");
                    printf("----------------------------\n");
                    printf("Mode: %s\n", mode == 1 ? "active" : "passive");
                    printf("-------- FTP Client --------\n");
                    printf("0. Change mode\n");
                    printf("1. Listing directory\n");
                    printf("2. Create directory\n");
                    printf("3. Delete directory\n");
                    printf("4. Rename directory\n");
                    printf("5. Rename file\n");
                    printf("6. Delete file\n");
                    printf("7. Upload file\n");
                    printf("8. Download file\n");
                    printf("9. Logout\n");
                    printf("----------------------------\n");
                    printf("Enter your choice (0-9): ");
                    scanf("%d", &choice);
                    getchar();

                    if (choice == 0)
                    {
                        while (true)
                        {
                            printf("Enter the mode (1-active, 2-passive): ");
                            scanf("%d", &mode);
                            getchar();

                            if (mode == 1 || mode == 2)
                            {
                                break;
                            }
                            else
                            {
                                printf("Invalid mode\n");
                            }
                        }
                    }
                    else if (choice == 1)
                    {
                        listing_directory(control_sock, mode);
                    }
                    else if (choice == 2)
                    {
                        // Nhập tên thư mục
                        char directory_name[BUF_SIZE];
                        memset(directory_name, 0, BUF_SIZE);
                        printf("Enter directory name: ");
                        fgets(directory_name, BUF_SIZE, stdin);
                        directory_name[strlen(directory_name) - 1] = '\0';

                        create_directory(control_sock, directory_name);
                    }
                    else if (choice == 3)
                    {
                        // Nhập tên thư mục
                        char directory_name[BUF_SIZE];
                        memset(directory_name, 0, BUF_SIZE);
                        printf("Enter directory name: ");
                        fgets(directory_name, BUF_SIZE, stdin);
                        directory_name[strlen(directory_name) - 1] = '\0';

                        delete_directory(control_sock, directory_name);
                    }
                    else if (choice == 4)
                    {
                        // Nhập tên thư mục cũ
                        char old_directory_name[BUF_SIZE];
                        memset(old_directory_name, 0, BUF_SIZE);
                        printf("Enter old directory name: ");
                        fgets(old_directory_name, BUF_SIZE, stdin);
                        old_directory_name[strlen(old_directory_name) - 1] = '\0';

                        // Nhập tên thư mục mới
                        char new_directory_name[BUF_SIZE];
                        memset(new_directory_name, 0, BUF_SIZE);
                        printf("Enter new directory name: ");
                        fgets(new_directory_name, BUF_SIZE, stdin);
                        new_directory_name[strlen(new_directory_name) - 1] = '\0';

                        rename_directory(control_sock, old_directory_name, new_directory_name);
                    }
                    else if (choice == 5)
                    {
                        // Nhập tên file cũ
                        char old_file_name[BUF_SIZE];
                        memset(old_file_name, 0, BUF_SIZE);
                        printf("Enter old file name: ");
                        fgets(old_file_name, BUF_SIZE, stdin);
                        old_file_name[strlen(old_file_name) - 1] = '\0';

                        // Nhập tên file mới
                        char new_file_name[BUF_SIZE];
                        memset(new_file_name, 0, BUF_SIZE);
                        printf("Enter new file name: ");
                        fgets(new_file_name, BUF_SIZE, stdin);
                        new_file_name[strlen(new_file_name) - 1] = '\0';

                        rename_file(control_sock, old_file_name, new_file_name);
                    }
                    else if (choice == 6)
                    {
                        // Nhập tên file
                        char file_name[BUF_SIZE];
                        memset(file_name, 0, BUF_SIZE);
                        printf("Enter file name: ");
                        fgets(file_name, BUF_SIZE, stdin);
                        file_name[strlen(file_name) - 1] = '\0';

                        delete_file(control_sock, file_name);
                    }
                    else if (choice == 7)
                    {
                        // Nhập tên file
                        char file_name[BUF_SIZE];
                        memset(file_name, 0, BUF_SIZE);
                        printf("Enter file name: ");
                        fgets(file_name, BUF_SIZE, stdin);
                        file_name[strlen(file_name) - 1] = '\0';

                        upload_file(control_sock, mode, file_name);
                    }
                    else if (choice == 8)
                    {
                        // Nhập tên file
                        char file_name[BUF_SIZE];
                        memset(file_name, 0, BUF_SIZE);
                        printf("Enter file name: ");
                        fgets(file_name, BUF_SIZE, stdin);
                        file_name[strlen(file_name) - 1] = '\0';

                        download_file(control_sock, mode, file_name);
                    }
                    else if (choice == 9)
                    {
                        break;
                    }
                    else
                    {
                        printf("Invalid choice\n");
                    }
                    // Enter để tiếp tục
                    printf("Press Enter to continue...");
                    getchar();
                }
            }
            break;

        case 2:
            printf("Goodbye!\n");
            is_continue = false;
            break;

        default:
            printf("Invalid choice\n");
            break;
        }
    }

    // Đóng control socket
    close(control_sock);

    return 0;
}

bool login(int sockfd)
{
    char command[BUF_SIZE + 7];
    memset(command, 0, BUF_SIZE + 7);

    char username[BUF_SIZE];
    memset(username, 0, BUF_SIZE);
    printf("Enter username: ");
    fgets(username, BUF_SIZE, stdin);
    username[strlen(username) - 1] = '\0';

    sprintf(command, "USER %s\r\n", username);
    if (send(sockfd, command, strlen(command), 0) < 0)
    {
        perror("send() failed");
        exit(EXIT_FAILURE);
    }
    char buf[BUF_SIZE];
    memset(buf, 0, BUF_SIZE);
    if (recv(sockfd, buf, BUF_SIZE, 0) < 0)
    {
        perror("recv() failed");
        exit(EXIT_FAILURE);
    }

    char password[BUF_SIZE];
    memset(password, 0, BUF_SIZE);
    printf("Enter password: ");
    fgets(password, BUF_SIZE, stdin);
    password[strlen(password) - 1] = '\0';

    memset(command, 0, BUF_SIZE + 7);
    sprintf(command, "PASS %s\r\n", password);
    if (send(sockfd, command, strlen(command), 0) < 0)
    {
        perror("send() failed");
        exit(EXIT_FAILURE);
    }
    memset(buf, 0, BUF_SIZE);
    if (recv(sockfd, buf, BUF_SIZE, 0) < 0)
    {
        perror("recv() failed");
        exit(EXIT_FAILURE);
    }

    return atoi(buf) == 230;
}

void listing_directory(int control_sockfd, int mode)
{
    char command[BUF_SIZE];
    char buf[BUF_SIZE];

    if (mode == 1)
    {
        // Tạo data socket
        int data_sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (data_sockfd < 0)
        {
            perror("socket() failed");
            exit(EXIT_FAILURE);
        }

        // Gán địa chỉ cho data socket
        struct sockaddr_in *data_server_addr = get_data_sock_addr(control_sockfd, mode);
        if (bind(data_sockfd, (struct sockaddr *)data_server_addr, sizeof(*data_server_addr)) < 0)
        {
            perror("bind() failed");
            exit(EXIT_FAILURE);
        }
        free(data_server_addr);

        // Lắng nghe kết nối
        if (listen(data_sockfd, 5) < 0)
        {
            perror("listen() failed");
            exit(EXIT_FAILURE);
        }

        // Gửi lệnh LIST
        char command[BUF_SIZE];
        memset(command, 0, BUF_SIZE);
        sprintf(command, "LIST\r\n");
        if (send(control_sockfd, command, strlen(command), 0) < 0)
        {
            perror("send() failed");
            exit(EXIT_FAILURE);
        }

        // Nhận phản hồi từ server
        char buf[BUF_SIZE];
        memset(buf, 0, BUF_SIZE);
        if (recv(control_sockfd, buf, BUF_SIZE, 0) < 0)
        {
            perror("recv() failed");
            exit(EXIT_FAILURE);
        }

        if (atoi(buf) != 150)
        {
            printf("Error: %s\n", buf);
            exit(EXIT_FAILURE);
        }
        else
        {
            printf("%s\n", buf);
        }

        // Chấp nhận kết nối từ kênh truyền data của ftp server
        int data_client_sockfd = accept(data_sockfd, NULL, NULL);
        if (data_client_sockfd < 0)
        {
            perror("accept() failed");
            exit(EXIT_FAILURE);
        }

        // Nhận dữ liệu từ server
        memset(buf, 0, BUF_SIZE);
        while (recv(data_client_sockfd, buf, BUF_SIZE, 0) > 0)
        {
            printf("%s", buf);
            memset(buf, 0, BUF_SIZE);
        }

        // Nhận phản hồi từ server
        memset(buf, 0, BUF_SIZE);
        if (recv(control_sockfd, buf, BUF_SIZE, 0) < 0)
        {
            perror("recv() failed");
            exit(EXIT_FAILURE);
        }

        if (atoi(buf) != 226)
        {
            printf("Error: %s\n", buf);
            exit(EXIT_FAILURE);
        }
        else
        {
            printf("\n%s\n", buf);
        }

        // Đóng kết nối
        close(data_client_sockfd);
        close(data_sockfd);
    }
    else if (mode == 2)
    {
        // Tạo data socket
        int data_sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (data_sockfd < 0)
        {
            perror("socket() failed");
            exit(EXIT_FAILURE);
        }

        // Kết nối đến server
        struct sockaddr_in *data_server_addr = get_data_sock_addr(control_sockfd, mode);
        if (connect(data_sockfd, (struct sockaddr *)data_server_addr, sizeof(*data_server_addr)) < 0)
        {
            perror("connect() failed");
            exit(EXIT_FAILURE);
        }
        free(data_server_addr);

        // Gửi lệnh LIST
        memset(command, 0, BUF_SIZE);
        sprintf(command, "LIST\r\n");
        if (send(control_sockfd, command, strlen(command), 0) < 0)
        {
            perror("send() failed");
            exit(EXIT_FAILURE);
        }

        // Nhận phản hồi từ server
        memset(buf, 0, BUF_SIZE);
        if (recv(control_sockfd, buf, BUF_SIZE, 0) < 0)
        {
            perror("recv() failed");
            exit(EXIT_FAILURE);
        }

        if (atoi(buf) != 150)
        {
            printf("Error: %s\n", buf);
            exit(EXIT_FAILURE);
        }
        else
        {
            printf("%s\n", buf);
        }

        // Nhận dữ liệu từ server
        memset(buf, 0, BUF_SIZE);
        while (recv(data_sockfd, buf, BUF_SIZE, 0) > 0)
        {
            printf("%s", buf);
            memset(buf, 0, BUF_SIZE);
        }

        // Nhận phản hồi từ server
        memset(buf, 0, BUF_SIZE);
        if (recv(control_sockfd, buf, BUF_SIZE, 0) < 0)
        {
            perror("recv() failed");
            exit(EXIT_FAILURE);
        }

        if (atoi(buf) != 226)
        {
            printf("Error: %s\n", buf);
            exit(EXIT_FAILURE);
        }
        else
        {
            printf("\n%s\n", buf);
        }

        // Đóng kết nối
        close(data_sockfd);
    }
}

void create_directory(int control_sockfd, char *dir_name)
{
    // Gửi lệnh MKD
    char command[BUF_SIZE];
    memset(command, 0, BUF_SIZE);
    sprintf(command, "MKD %s\r\n", dir_name);
    if (send(control_sockfd, command, strlen(command), 0) < 0)
    {
        perror("send() failed");
        exit(EXIT_FAILURE);
    }

    // Nhận phản hồi từ server
    char buf[BUF_SIZE];
    memset(buf, 0, BUF_SIZE);
    if (recv(control_sockfd, buf, BUF_SIZE, 0) < 0)
    {
        perror("recv() failed");
        exit(EXIT_FAILURE);
    }

    if (atoi(buf) != 257)
    {
        printf("Error: %s\n", buf);
        printf("Create directory failed\n");
    }
    else
    {
        printf("%s\n", buf);
    }
}

void delete_directory(int control_sockfd, char *dir_name)
{
    // Gửi lệnh RMD
    char command[BUF_SIZE];
    memset(command, 0, BUF_SIZE);
    sprintf(command, "RMD %s\r\n", dir_name);
    if (send(control_sockfd, command, strlen(command), 0) < 0)
    {
        perror("send() failed");
        exit(EXIT_FAILURE);
    }

    // Nhận phản hồi từ server
    char buf[BUF_SIZE];
    memset(buf, 0, BUF_SIZE);
    if (recv(control_sockfd, buf, BUF_SIZE, 0) < 0)
    {
        perror("recv() failed");
        exit(EXIT_FAILURE);
    }

    if (atoi(buf) != 250)
    {
        printf("Error: %s\n", buf);
        printf("Delete directory failed\n");
    }
    else
    {
        printf("%s\n", buf);
    }
}

void rename_directory(int control_sockfd, char *old_name, char *new_name)
{
    // Gửi lệnh RNFR
    char command[BUF_SIZE];
    memset(command, 0, BUF_SIZE);
    sprintf(command, "RNFR %s\r\n", old_name);
    if (send(control_sockfd, command, strlen(command), 0) < 0)
    {
        perror("send() failed");
        exit(EXIT_FAILURE);
    }

    // Nhận phản hồi từ server
    char buf[BUF_SIZE];
    memset(buf, 0, BUF_SIZE);
    if (recv(control_sockfd, buf, BUF_SIZE, 0) < 0)
    {
        perror("recv() failed");
        exit(EXIT_FAILURE);
    }

    if (atoi(buf) != 350)
    {
        printf("Error: %s\n", buf);
        printf("Rename directory failed\n");
    }
    else
    {
        printf("%s\n", buf);
    }

    // Gửi lệnh RNTO
    memset(command, 0, BUF_SIZE);
    sprintf(command, "RNTO %s\r\n", new_name);
    if (send(control_sockfd, command, strlen(command), 0) < 0)
    {
        perror("send() failed");
        exit(EXIT_FAILURE);
    }

    // Nhận phản hồi từ server
    memset(buf, 0, BUF_SIZE);
    if (recv(control_sockfd, buf, BUF_SIZE, 0) < 0)
    {
        perror("recv() failed");
        exit(EXIT_FAILURE);
    }

    if (atoi(buf) != 250)
    {
        printf("Error: %s\n", buf);
        printf("Rename directory failed\n");
    }
    else
    {
        printf("%s\n", buf);
    }
}

void rename_file(int control_sockfd, char *old_name, char *new_name)
{
    // Gửi lệnh RNFR
    char command[BUF_SIZE];
    memset(command, 0, BUF_SIZE);
    sprintf(command, "RNFR %s\r\n", old_name);
    if (send(control_sockfd, command, strlen(command), 0) < 0)
    {
        perror("send() failed");
        exit(EXIT_FAILURE);
    }

    // Nhận phản hồi từ server
    char buf[BUF_SIZE];
    memset(buf, 0, BUF_SIZE);
    if (recv(control_sockfd, buf, BUF_SIZE, 0) < 0)
    {
        perror("recv() failed");
        exit(EXIT_FAILURE);
    }

    if (atoi(buf) != 350)
    {
        printf("Error: %s\n", buf);
        printf("Rename file failed\n");
    }
    else
    {
        printf("%s\n", buf);
    }

    // Gửi lệnh RNTO
    memset(command, 0, BUF_SIZE);
    sprintf(command, "RNTO %s\r\n", new_name);
    if (send(control_sockfd, command, strlen(command), 0) < 0)
    {
        perror("send() failed");
        exit(EXIT_FAILURE);
    }

    // Nhận phản hồi từ server
    memset(buf, 0, BUF_SIZE);
    if (recv(control_sockfd, buf, BUF_SIZE, 0) < 0)
    {
        perror("recv() failed");
        exit(EXIT_FAILURE);
    }

    if (atoi(buf) != 250)
    {
        printf("Error: %s\n", buf);
        printf("Rename file failed\n");
    }
    else
    {
        printf("%s\n", buf);
    }
}

void delete_file(int control_sockfd, char *file_name)
{
    // Gửi lệnh DELE
    char command[BUF_SIZE];
    memset(command, 0, BUF_SIZE);
    sprintf(command, "DELE %s\r\n", file_name);
    if (send(control_sockfd, command, strlen(command), 0) < 0)
    {
        perror("send() failed");
        exit(EXIT_FAILURE);
    }

    // Nhận phản hồi từ server
    char buf[BUF_SIZE];
    memset(buf, 0, BUF_SIZE);
    if (recv(control_sockfd, buf, BUF_SIZE, 0) < 0)
    {
        perror("recv() failed");
        exit(EXIT_FAILURE);
    }

    if (atoi(buf) != 250)
    {
        printf("Error: %s\n", buf);
        printf("Delete file failed\n");
    }
    else
    {
        printf("%s\n", buf);
    }
}

void upload_file(int control_sockfd, int mode, char *file_name)
{
    // Gửi lệnh TYPE
    char command[BUF_SIZE];
    memset(command, 0, BUF_SIZE);
    sprintf(command, "TYPE I\r\n");
    if (send(control_sockfd, command, strlen(command), 0) < 0)
    {
        perror("send() failed");
        exit(EXIT_FAILURE);
    }

    // Nhận phản hồi từ server
    char buf[BUF_SIZE];
    memset(buf, 0, BUF_SIZE);
    if (recv(control_sockfd, buf, BUF_SIZE, 0) < 0)
    {
        perror("recv() failed");
        exit(EXIT_FAILURE);
    }

    if (atoi(buf) != 200)
    {
        printf("Error: %s\n", buf);
        printf("Change type failed\n");
    }
    else
    {
        printf("%s\n", buf);
    }

    if (mode == 1)
    {
        // Active mode
        // Tạo socket data
        int data_sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (data_sockfd < 0)
        {
            perror("socket() failed");
            exit(EXIT_FAILURE);
        }

        // Gán địa chỉ cho socket data
        struct sockaddr_in *data_addr = get_data_sock_addr(control_sockfd, mode);
        if (bind(data_sockfd, (struct sockaddr *)data_addr, sizeof(struct sockaddr)) < 0)
        {
            perror("bind() failed");
            exit(EXIT_FAILURE);
        }
        free(data_addr);

        // Lắng nghe kết nối từ client
        if (listen(data_sockfd, 5) < 0)
        {
            perror("listen() failed");
            exit(EXIT_FAILURE);
        }

        // Gửi lệnh STOR
        memset(command, 0, BUF_SIZE);
        sprintf(command, "STOR %s\r\n", file_name);
        if (send(control_sockfd, command, strlen(command), 0) < 0)
        {
            perror("send() failed");
            exit(EXIT_FAILURE);
        }

        // Nhận phản hồi từ server
        memset(buf, 0, BUF_SIZE);
        if (recv(control_sockfd, buf, BUF_SIZE, 0) < 0)
        {
            perror("recv() failed");
            exit(EXIT_FAILURE);
        }

        if (atoi(buf) != 150)
        {
            printf("Error: %s\n", buf);
            printf("Upload file failed\n");
        }
        else
        {
            printf("%s\n", buf);
        }

        // Chấp nhận kết nối từ client
        int data_conn_sockfd = accept(data_sockfd, NULL, NULL);
        if (data_conn_sockfd < 0)
        {
            perror("accept() failed");
            exit(EXIT_FAILURE);
        }

        // Mở file
        FILE *file = fopen(file_name, "rb");
        if (file == NULL)
        {
            perror("fopen() failed");
            exit(EXIT_FAILURE);
        }

        // Đọc file và gửi lên server
        memset(buf, 0, BUF_SIZE);
        int num_bytes_read;
        while ((num_bytes_read = fread(buf, 1, BUF_SIZE, file)) > 0)
        {
            if (send(data_conn_sockfd, buf, num_bytes_read, 0) < 0)
            {
                perror("send() failed");
                exit(EXIT_FAILURE);
            }
            memset(buf, 0, BUF_SIZE);
        }

        // Đóng file
        fclose(file);

        // Đóng kết nối
        close(data_conn_sockfd);

        // Nhận phản hồi từ server
        memset(buf, 0, BUF_SIZE);
        if (recv(control_sockfd, buf, BUF_SIZE, 0) < 0)
        {
            perror("recv() failed");
            exit(EXIT_FAILURE);
        }

        if (atoi(buf) != 226)
        {
            printf("Error: %s\n", buf);
            printf("Upload file failed\n");
        }
        else
        {
            printf("%s\n", buf);
        }

        // Đóng kết nối
        close(data_sockfd);
    }
    else if (mode == 2)
    {
        // Passive mode
        // Tạo socket data
        int data_sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (data_sockfd < 0)
        {
            perror("socket() failed");
            exit(EXIT_FAILURE);
        }

        // Kết nối đến server
        struct sockaddr_in *data_addr = get_data_sock_addr(control_sockfd, mode);
        if (connect(data_sockfd, (struct sockaddr *)data_addr, sizeof(struct sockaddr)) < 0)
        {
            perror("connect() failed");
            exit(EXIT_FAILURE);
        }
        free(data_addr);

        // Gửi lệnh STOR
        memset(command, 0, BUF_SIZE);
        sprintf(command, "STOR %s\r\n", file_name);
        if (send(control_sockfd, command, strlen(command), 0) < 0)
        {
            perror("send() failed");
            exit(EXIT_FAILURE);
        }

        // Nhận phản hồi từ server
        memset(buf, 0, BUF_SIZE);
        if (recv(control_sockfd, buf, BUF_SIZE, 0) < 0)
        {
            perror("recv() failed");
            exit(EXIT_FAILURE);
        }

        if (atoi(buf) != 150)
        {
            printf("Error: %s\n", buf);
            printf("Upload file failed\n");
        }
        else
        {
            printf("%s\n", buf);
        }

        // Mở file
        FILE *file = fopen(file_name, "rb");
        if (file == NULL)
        {
            perror("fopen() failed");
            exit(EXIT_FAILURE);
        }

        // Đọc file và gửi lên server
        memset(buf, 0, BUF_SIZE);
        int num_bytes_read;
        while ((num_bytes_read = fread(buf, 1, BUF_SIZE, file)) > 0)
        {
            if (send(data_sockfd, buf, num_bytes_read, 0) < 0)
            {
                perror("send() failed");
                exit(EXIT_FAILURE);
            }
            memset(buf, 0, BUF_SIZE);
        }

        // Đóng file
        fclose(file);

        // Đóng kết nối
        close(data_sockfd);

        // Nhận phản hồi từ server
        memset(buf, 0, BUF_SIZE);
        if (recv(control_sockfd, buf, BUF_SIZE, 0) < 0)
        {
            perror("recv() failed");
            exit(EXIT_FAILURE);
        }

        if (atoi(buf) != 226)
        {
            printf("Error: %s\n", buf);
            printf("Upload file failed\n");
        }
        else
        {
            printf("%s\n", buf);
        }
    }
}

void download_file(int control_sockfd, int mode, char *file_name)
{
    char command[BUF_SIZE];
    char buf[BUF_SIZE];

    if (mode == 1)
    {
        // Active mode
        // Tạo socket data
        int data_sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (data_sockfd < 0)
        {
            perror("socket() failed");
            exit(EXIT_FAILURE);
        }

        // Gán địa chỉ cho socket data
        struct sockaddr_in *data_addr = get_data_sock_addr(control_sockfd, mode);
        if (bind(data_sockfd, (struct sockaddr *)data_addr, sizeof(struct sockaddr)) < 0)
        {
            perror("bind() failed");
            exit(EXIT_FAILURE);
        }
        free(data_addr);

        // Lắng nghe kết nối
        if (listen(data_sockfd, 5) < 0)
        {
            perror("listen() failed");
            exit(EXIT_FAILURE);
        }

        // Gửi lệnh RETR
        memset(command, 0, BUF_SIZE);
        sprintf(command, "RETR %s\r\n", file_name);
        if (send(control_sockfd, command, strlen(command), 0) < 0)
        {
            perror("send() failed");
            exit(EXIT_FAILURE);
        }

        // Nhận phản hồi từ server
        memset(buf, 0, BUF_SIZE);
        if (recv(control_sockfd, buf, BUF_SIZE, 0) < 0)
        {
            perror("recv() failed");
            exit(EXIT_FAILURE);
        }

        if (atoi(buf) != 150)
        {
            printf("Error: %s\n", buf);
            printf("Download file failed\n");
        }
        else
        {
            printf("%s\n", buf);
        }

        // Chấp nhận kết nối từ client
        int client_sockfd;
        if ((client_sockfd = accept(data_sockfd, NULL, NULL)) < 0)
        {
            perror("accept() failed");
            exit(EXIT_FAILURE);
        }

        // Mở file
        FILE *file = fopen(file_name, "wb");
        if (file == NULL)
        {
            perror("fopen() failed");
            exit(EXIT_FAILURE);
        }

        // Nhận dữ liệu từ server và ghi vào file
        memset(buf, 0, BUF_SIZE);
        int num_bytes_read;
        while ((num_bytes_read = recv(client_sockfd, buf, BUF_SIZE, 0)) > 0)
        {
            if (fwrite(buf, 1, num_bytes_read, file) < 0)
            {
                perror("fwrite() failed");
                exit(EXIT_FAILURE);
            }
            memset(buf, 0, BUF_SIZE);
        }

        // Đóng file
        fclose(file);

        // Đóng kết nối
        close(client_sockfd);

        // Nhận phản hồi từ server
        memset(buf, 0, BUF_SIZE);
        if (recv(control_sockfd, buf, BUF_SIZE, 0) < 0)
        {
            perror("recv() failed");
            exit(EXIT_FAILURE);
        }

        if (atoi(buf) != 226)
        {
            printf("Error: %s\n", buf);
            printf("Download file failed\n");
        }
        else
        {
            printf("%s\n", buf);
        }

        // Đóng kết nối
        close(data_sockfd);
    }
    else if (mode == 2)
    {
        // Passive mode
        // Tạo socket data
        int data_sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (data_sockfd < 0)
        {
            perror("socket() failed");
            exit(EXIT_FAILURE);
        }

        // Kết nối tới server
        struct sockaddr_in *data_addr = get_data_sock_addr(control_sockfd, mode);
        if (connect(data_sockfd, (struct sockaddr *)data_addr, sizeof(struct sockaddr)) < 0)
        {
            perror("connect() failed");
            exit(EXIT_FAILURE);
        }
        free(data_addr);

        // Gửi lệnh RETR
        memset(command, 0, BUF_SIZE);
        sprintf(command, "RETR %s\r\n", file_name);
        if (send(control_sockfd, command, strlen(command), 0) < 0)
        {
            perror("send() failed");
            exit(EXIT_FAILURE);
        }

        // Nhận phản hồi từ server
        memset(buf, 0, BUF_SIZE);
        if (recv(control_sockfd, buf, BUF_SIZE, 0) < 0)
        {
            perror("recv() failed");
            exit(EXIT_FAILURE);
        }

        if (atoi(buf) != 150)
        {
            printf("Error: %s\n", buf);
            printf("Download file failed\n");
        }
        else
        {
            printf("%s\n", buf);
        }

        // Mở file
        FILE *file = fopen(file_name, "wb");
        if (file == NULL)
        {
            perror("fopen() failed");
            exit(EXIT_FAILURE);
        }

        // Nhận dữ liệu từ server và ghi vào file
        memset(buf, 0, BUF_SIZE);
        int num_bytes_read;
        while ((num_bytes_read = recv(data_sockfd, buf, BUF_SIZE, 0)) > 0)
        {
            if (fwrite(buf, 1, num_bytes_read, file) < 0)
            {
                perror("fwrite() failed");
                exit(EXIT_FAILURE);
            }
            memset(buf, 0, BUF_SIZE);
        }

        // Đóng file
        fclose(file);

        // Đóng kết nối
        close(data_sockfd);

        // Nhận phản hồi từ server
        memset(buf, 0, BUF_SIZE);
        if (recv(control_sockfd, buf, BUF_SIZE, 0) < 0)
        {
            perror("recv() failed");
            exit(EXIT_FAILURE);
        }

        if (atoi(buf) != 226)
        {
            printf("Error: %s\n", buf);
            printf("Download file failed\n");
        }
        else
        {
            printf("%s\n", buf);
        }
    }
}

struct sockaddr_in *get_data_sock_addr(int control_sockfd, int mode)
{
    struct sockaddr_in *addr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
    memset(addr, 0, sizeof(struct sockaddr_in));

    if (mode == 1)
    {
        // Active mode
        // Gửi lệnh EPSV
        char command[BUF_SIZE];
        memset(command, 0, BUF_SIZE);
        sprintf(command, "EPSV\r\n");
        if (send(control_sockfd, command, strlen(command), 0) < 0)
        {
            perror("send() failed");
            exit(EXIT_FAILURE);
        }

        // Nhận phản hồi từ server
        char buf[BUF_SIZE];
        memset(buf, 0, BUF_SIZE);
        if (recv(control_sockfd, buf, BUF_SIZE, 0) < 0)
        {
            perror("recv() failed");
            exit(EXIT_FAILURE);
        }

        if (atoi(buf) != 229)
        {
            printf("Error: %s\n", buf);
            exit(EXIT_FAILURE);
        }
        else
        {
            printf("\n%s\n", buf);
        }

        // Phân tích port
        char *start = strchr(buf, '(');
        char *end = strchr(buf, ')');
        *end = '\0';
        char *token = strtok(start, "|");
        int port = 0;
        while (token != NULL)
        {
            port = atoi(token);
            token = strtok(NULL, "|");
        }

        // Gửi lệnh PORT
        memset(command, 0, BUF_SIZE);
        sprintf(command, "PORT 127,0,0,1,%d,%d\r\n", port / 256, port % 256);
        if (send(control_sockfd, command, strlen(command), 0) < 0)
        {
            perror("send() failed");
            exit(EXIT_FAILURE);
        }

        // Nhận phản hồi từ server
        memset(buf, 0, BUF_SIZE);
        if (recv(control_sockfd, buf, BUF_SIZE, 0) < 0)
        {
            perror("recv() failed");
            exit(EXIT_FAILURE);
        }

        if (atoi(buf) != 200)
        {
            printf("Error: %s\n", buf);
            exit(EXIT_FAILURE);
        }
        else
        {
            printf("%s\n", buf);
        }

        addr->sin_family = AF_INET;
        addr->sin_port = htons(port);
        addr->sin_addr.s_addr = inet_addr("127.0.0.1");
    }
    else if (mode == 2)
    {
        // Passive mode
        // Gửi lệnh PASV
        char command[BUF_SIZE];
        memset(command, 0, BUF_SIZE);
        sprintf(command, "PASV\r\n");
        if (send(control_sockfd, command, strlen(command), 0) < 0)
        {
            perror("send() failed");
            exit(EXIT_FAILURE);
        }

        // Nhận phản hồi từ server
        char buf[BUF_SIZE];
        memset(buf, 0, BUF_SIZE);
        if (recv(control_sockfd, buf, BUF_SIZE, 0) < 0)
        {
            perror("recv() failed");
            exit(EXIT_FAILURE);
        }

        if (atoi(buf) != 227)
        {
            printf("Error: %s\n", buf);
            exit(EXIT_FAILURE);
        }
        else
        {
            printf("\n%s\n", buf);
        }

        // Phân tích ip_address và port
        int p[6];
        char *token = strtok(buf, "(,)");
        int i = 0;
        while (token != NULL)
        {
            p[i++] = atoi(token);
            token = strtok(NULL, "(,)");
        }

        char ip_address[BUF_SIZE];
        memset(ip_address, 0, BUF_SIZE);
        sprintf(ip_address, "%d.%d.%d.%d", p[1], p[2], p[3], p[4]);

        int port = p[5] * 256 + p[6];

        addr->sin_family = AF_INET;
        addr->sin_port = htons(port);
        addr->sin_addr.s_addr = inet_addr(ip_address);
    }

    return addr;
}