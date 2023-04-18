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
        // Nhập tên máy tính
        char nameComputer[MAX_LENGTH];
        memset(nameComputer, 0, MAX_LENGTH);
        printf("Enter name computer: ");
        fgets(nameComputer, MAX_LENGTH, stdin);
        nameComputer[strcspn(nameComputer, "\n")] = 0;

        // Nhập số lượng ổ đĩa
        unsigned short numberDisk = 0;
        printf("Enter number disk: ");
        scanf("%hu", &numberDisk);
        getchar();

        // Nhập tên từng ổ đĩa và dung lượng đi kèm
        char nameDisk[MAX_LENGTH][MAX_LENGTH];
        unsigned short sizeDisk[MAX_LENGTH];
        for (int i = 0; i < numberDisk; i++)
        {
            memset(nameDisk[i], 0, MAX_LENGTH);
            printf("\t- Enter name disk %d: ", i + 1);
            fgets(nameDisk[i], MAX_LENGTH, stdin);
            nameDisk[i][strcspn(nameDisk[i], "\n")] = 0;

            printf("\t- Enter size disk %d: ", i + 1);
            scanf("%hu", &sizeDisk[i]);
            getchar();
        }

        // Đóng gói thông tin
        char buffer[MAX_LENGTH];
        memset(buffer, 0, MAX_LENGTH);
        sprintf(buffer, "%s;%hu", nameComputer, numberDisk);
        for (int i = 0; i < numberDisk; i++)
        {
            sprintf(buffer, "%s;%s;%hu", buffer, nameDisk[i], sizeDisk[i]);
        }

        // Gửi thông tin đến server
        if (send(client, buffer, strlen(buffer), 0) == -1)
        {
            perror("send() failed");
            return 1;
        }

        // Hỏi người dùng có muốn nhập tiếp không
        memset(buffer, 0, MAX_LENGTH);
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
    close(client);
    return 0;
}