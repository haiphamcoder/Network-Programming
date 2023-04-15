#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>

#define MAX_LENGTH 1024

int main(int argc, char *argv[])
{
    // Kiểm tra đầu vào
    if (argc != 3)
    {
        printf("Usage: %s <server-IP-address> <port>\n", argv[0]);
        return 1;
    }

    // Thiết lập thông tin địa chỉ cho socket
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    server_addr.sin_port = htons(atoi(argv[2]));

    // Tạo socket
    int client = socket(AF_INET, SOCK_STREAM, 0);
    if (client == -1)
    {
        perror("socket() failed");
        return 1;
    }

    // Kết nối đến server
    if (connect(client, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("connect() failed");
        return 1;
    }
    printf("Connection to %s %s port [tcp/*] succeeded!\n", argv[1], argv[2]);

    while (1)
    {
        // Nhập thông tin sinh viên từ bàn phím
        char mssv[MAX_LENGTH], hoten[MAX_LENGTH], ngaysinh[MAX_LENGTH], diem[MAX_LENGTH];
        memset(mssv, 0, MAX_LENGTH);
        memset(hoten, 0, MAX_LENGTH);
        memset(ngaysinh, 0, MAX_LENGTH);
        memset(diem, 0, MAX_LENGTH);

        printf("Enter student information:\n");
        printf("\t- MSSV: ");
        fgets(mssv, MAX_LENGTH, stdin);
        mssv[strcspn(mssv, "\n")] = 0;

        printf("\t- Họ tên: ");
        fgets(hoten, MAX_LENGTH, stdin);
        hoten[strcspn(hoten, "\n")] = 0;

        printf("\t- Ngày sinh: ");
        fgets(ngaysinh, MAX_LENGTH, stdin);
        ngaysinh[strcspn(ngaysinh, "\n")] = 0;

        printf("\t- Điểm trung bình: ");
        fgets(diem, MAX_LENGTH, stdin);
        diem[strcspn(diem, "\n")] = 0;

        // Đóng gói thông tin sinh viên vào buffer để gửi đến server
        char buffer[4 * MAX_LENGTH + 1];
        memset(buffer, 0, 4 * MAX_LENGTH + 1);
        sprintf(buffer, "%s %s %s %s", mssv, hoten, ngaysinh, diem);

        // Gửi buffer chứa thông tin sinh viên đến server
        int bytes_sent = send(client, buffer, strlen(buffer), 0);
        if (bytes_sent == -1)
        {
            perror("send() failed");
            return 1;
        }
        printf("Data sent successfully\n\n");

        // Hỏi người dùng có muốn nhập tiếp không
        printf("Do you want to continue? (y/n): ");
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strcspn(buffer, "\n")] = 0;
        if (strcmp(buffer, "n") == 0)
        {
            if (send(client, "exit\n", 5, 0) == -1)
            {
                perror("send() failed");
                return 1;
            }
            break;
        }
    }

    // Đóng kết nối socket
    close(client);

    return 0;
}