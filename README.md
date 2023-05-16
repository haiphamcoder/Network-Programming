# Network Programming

## [20230411: Tuần 1](https://github.com/haiphamcoder/Network-Programming/tree/main/exercises/20230411)

**Bài 1:** Viết chương trình tcp_client, kết nối đến một máy chủ xác định bởi địa chỉ IP và cổng. Sau đó nhận dữ liệu từ bàn phím và gửi đến server. Tham số được truyền vào từ dòng lệnh có dạng:

> tcp_client <địa chỉ IP> <cổng>

***Source code: tcp_client.c***

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>

#define MAX_BUF_SIZE 1024

int main(int argc, char *argv[])
{
    // Kiểm tra đầu vào
    if (argc != 3)
    {
        printf("Usage: %s <server-IP-address> <Port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Thiết lập thông tin địa chỉ cho socket
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    server_addr.sin_port = htons(atoi(argv[2]));

    // Tạo socket
    int client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client == -1)
    {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    // Kết nối đến server
    if (connect(client, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("connect() failed");
        exit(EXIT_FAILURE);
    }
    printf("Connection to %s %s port [tcp/*] succeeded!\n", argv[1], argv[2]);

    // Nhận dữ liệu từ bàn phím và gửi đến server
    char buf[MAX_BUF_SIZE];
    memset(buf, 0, MAX_BUF_SIZE);
    while (1)
    {
        // Nhận dữ liệu từ server
        printf("Enter a message: ");
        fgets(buf, MAX_BUF_SIZE, stdin);

        // Gửi tin nhắn đến server
        if (send(client, buf, strlen(buf), 0) == -1)
        {
            perror("send() failed");
            exit(EXIT_FAILURE);
        }

        // Kiểm tra xem có phải là lệnh thoát không
        if (strcmp(buf, "exit\n") == 0)
        {
            break;
        }
    }
    // Đóng socket
    close(client);

    return 0;
}
```

**Bài 2:** Viết chương trình tcp_server, đợi kết nối ở cổng xác định bởi tham số dòng lệnh. Mỗi khi có client kết nối đến, thì gửi xâu chào được chỉ ra trong một tệp tin xác định, sau đó ghi toàn bộ nội dung client gửi đến vào một tệp tin khác được chỉ ra trong tham số dòng lệnh:

> tcp_server <cổng> <tệp tin chứa câu chào> <tệp tin lưu nội dung client gửi đến>

***Source code: tcp_server.c***

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

#define MAX_CLIENT 5
#define MAX_BUF_SIZE 1024

int main(int argc, char *argv[])
{
    // Kiểm tra đầu vào
    if (argc != 4)
    {
        printf("Usage: %s <Port> <The-file-contains-the-greeting> <The-file-that-stores-the-content-the-client-sends-to>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Tạo socket
    int server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server == -1)
    {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    // Thiết lập thông tin địa chỉ cho socket
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(atoi(argv[1]));

    // Gán địa chỉ cho socket
    if (bind(server, (struct sockaddr *)&server_addr,
             sizeof(server_addr)) == -1)
    {
        perror("bind() failed");
        exit(EXIT_FAILURE);
    }

    // Lắng nghe kết nối từ client
    if (listen(server, MAX_CLIENT) == -1)
    {
        perror("listen() failed");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        printf("Waiting for client on %s %s\n",
               inet_ntoa(server_addr.sin_addr), argv[1]);

        // Chấp nhận kết nối từ client
        struct sockaddr_in client_addr;
        memset(&client_addr, 0, sizeof(client_addr));
        socklen_t client_addr_len = sizeof(client_addr);
        int client = accept(server,
                            (struct sockaddr *)&client_addr,
                            &client_addr_len);
        if (client == -1)
        {
            perror("accept() failed");
            exit(EXIT_FAILURE);
        }
        printf("Accepted socket %d from IP: %s:%d\n",
               client,
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port));

        // Đọc nội dung tệp tin chứa câu chào
        FILE *fp = fopen(argv[2], "r");
        if (fp == NULL)
        {
            perror("fopen() failed");
            exit(EXIT_FAILURE);
        }
        fseek(fp, 0, SEEK_END);
        long fsize = ftell(fp);
        rewind(fp);
        char *greeting = (char *)malloc(fsize + 1);
        if (greeting == NULL)
        {
            perror("malloc() failed");
            exit(EXIT_FAILURE);
        }
        memset(greeting, 0, fsize + 1);
        fread(greeting, fsize, 1, fp);
        fclose(fp);

        // Gửi câu chào đến client
        if (send(client, greeting, strlen(greeting), 0) == -1)
        {
            perror("send() failed");
            exit(EXIT_FAILURE);
        }
        free(greeting);

        // Nhận dữ liệu từ client
        char buf[MAX_BUF_SIZE];
        memset(buf, 0, MAX_BUF_SIZE);
        while (1)
        {
            // Nhận dữ liệu từ client và ghi vào tệp tin
            int bytes_received = recv(client, buf, MAX_BUF_SIZE, 0);
            buf[bytes_received] = '\0';
            if (bytes_received == -1)
            {
                perror("recv() failed");
                exit(EXIT_FAILURE);
            }
            else if (bytes_received == 0 || strcmp(buf, "exit\n") == 0)
            {
                printf("Client from %s:%d disconnected\n\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                break;
            }

            // In dữ liệu nhận được
            printf("Received %ld bytes from client: %s", strlen(buf), buf);

            // Ghi dữ liệu vào tệp tin
            fp = fopen(argv[3], "a");
            if (fp == NULL)
            {
                perror("fopen() failed");
                exit(EXIT_FAILURE);
            }
            fprintf(fp, "%s", buf);
            fclose(fp);
        }
        // Đóng kết nối với client
        close(client);
    }

    // Đóng socket
    close(server);

    return 0;
}
```

**Bài 3:** Viết chương trình sv_client, cho phép người dùng nhập dữ liệu là thông tin của sinh viên bao gồm MSSV, họ tên, ngày sinh, và điểm trung bình các môn học. Các thông tin trên được đóng gói và gửi sang sv_server. Địa chỉ và cổng của server được nhập từ tham số dòng lệnh.

***Source code: sv_client.c***

```c
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
```

**Bài 4:** Viết chương trình sv_server, nhận dữ liệu từ sv_client, in ra màn hình và đồng thời ghi vào file sv_log.txt. Dữ liệu được ghi trên một dòng với mỗi client, kèm theo địa chỉ IP và thời gian client đã gửi. Tham số cổng và tên file log được nhập từ tham số dòng lệnh.Ví dụ dữ liệu trong file log:

> 127.0.0.1 2023-04-10 09:00:00 20201234 Nguyen Van A 2002-04-10 3.99
> 127.0.0.1 2023-04-10 09:00:10 20205678 Tran Van B 2002-08-18 3.50

***Source code: sv_server.c***

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>

#define MAX_LENGTH 1024
#define MAX_CLIENT 10

int main(int argc, char *argv[])
{

    // Kiểm tra đầu vào
    if (argc != 3)
    {
        printf("Usage: %s <port> <log-file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Tạo socket
    int server = socket(AF_INET, SOCK_STREAM, 0);
    if (server == -1)
    {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    // Thiết lập thông tin địa chỉ cho socket
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(atoi(argv[1]));

    // Gán địa chỉ cho socket
    if (bind(server, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("bind() failed");
        exit(EXIT_FAILURE);
    }

    // Lắng nghe kết nối
    if (listen(server, MAX_CLIENT) == -1)
    {
        perror("listen() failed");
        exit(EXIT_FAILURE);
    }

    // Nhận dữ liệu từ client
    char buf[MAX_LENGTH];
    memset(buf, 0, MAX_LENGTH);
    while (1)
    {
        printf("Waiting for client on %s %s\n", inet_ntoa(server_addr.sin_addr), argv[1]);

        // Chấp nhận kết nối từ client
        struct sockaddr_in client_addr;
        memset(&client_addr, 0, sizeof(client_addr));
        socklen_t client_addr_len = sizeof(client_addr);
        int client = accept(server,
                            (struct sockaddr *)&client_addr,
                            &client_addr_len);
        if (client == -1)
        {
            perror("accept() failed");
            exit(EXIT_FAILURE);
        }
        printf("Accepted socket %d from IP: %s:%d\n",
               client,
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port));

        while (1)
        {
            // Nhận dữ liệu từ client
            int bytes_received = recv(client, buf, MAX_LENGTH, 0);
            buf[bytes_received] = '\0';
            if (bytes_received == -1)
            {
                perror("recv() failed");
                exit(EXIT_FAILURE);
            }
            else if (bytes_received == 0 || strcmp(buf, "exit\n") == 0)
            {
                printf("Client from %s:%d disconnected\n\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                break;
            }

            // Lấy thời gian hiện tại
            time_t current_time = time(NULL);
            struct tm *time_info = localtime(&current_time);

            // Ghi vào file log
            FILE *log_file = fopen(argv[2], "a");
            if (log_file == NULL)
            {
                perror("fopen() failed");
                exit(EXIT_FAILURE);
            }
            printf("%s %d-%d-%d %d:%d:%d %s\n",
                   inet_ntoa(client_addr.sin_addr),
                   time_info->tm_year + 1900,
                   time_info->tm_mon, time_info->tm_mday,
                   time_info->tm_hour, time_info->tm_min, time_info->tm_sec, buf);
            fprintf(log_file, "%s %d-%d-%d %d:%d:%d %s\n",
                    inet_ntoa(client_addr.sin_addr),
                    time_info->tm_year + 1900,
                    time_info->tm_mon, time_info->tm_mday,
                    time_info->tm_hour, time_info->tm_min, time_info->tm_sec, buf);
            fclose(log_file);
        }

        // Đóng kết nối
        close(client);
    }
    return 0;
}
```

## [20230418: Tuần 2](https://github.com/haiphamcoder/Network-Programming/tree/main/exercises/20230418)

**Bài 1:** Ứng dụng info_client cho phép người dùng nhập tên máy tính (là chuỗi ký tự),danh sách các ổ đĩa (gồm ký tự và kích thước ổ đĩa)từ bàn phím.Các dữ liệu này sau đó được đóng gói và chuyển sang info_server.

Ứng dụng info_server nhận dữ liệu từ info_client, tách các dữ liệu và in ra màn hình. Ví dụ:

> + Tên máy tính MY_LAPTOP_DELL
>
> + Số ổ đĩa 3
>
>   C–500GB
>
>   D–250GB
>
>   E–250GB

***Source code: info_client.c***


***Source code: info_server.c***


**Bài 2:** Ứng dụng client đọc nội dung file văn bản và gửi sang server.

Ứng dụng server nhận dữ liệu từ client,in ra màn hình số lần xuất hiện xâu ký tự (server không cần tạo file để chứa nội dung nhận được): “0123456789”

Chú ý cần xử lý trường hợp khi xâu“0123456789” nằm giữa 2 lần truyền. Ví dụ nội dung file văn bản:

> SOICTSOICT0123456789012345678901234567890123456789SOICTSOICTSOICT01234567890123456789012345678901234567890123456789012345678901234567890123456789SOICTSOICT



**Bài 3:** Ứng dụng udp_file_sender cho phép người dùng nhập tên file từ dòng lệnh, sau đó truyền tên và nội dung file sang udp_file_receiver. Địa chỉ IP và cổng của receiver cũng được nhập từ dòng lệnh.

Ứng dụng udp_file_receiver nhận dữ liệu từ udp_file_sender và ghi vào file.Cổng chờ được nhập từ dòng lệnh.

**Bài 4:** Tại một thời điểm có thể có nhiều udp_file_sender cùng chạy.Đểm minh họa tình huống này, có thể dùng lệnh usleep() để tạm dừng chương trình trong một khoảng thời gian nhằm giảm tốc độ gửi file.

Ứng dụng udp_file_receiver có thể nhận nội dung các file từ nhiều udp_file_sender khác nhau. Ứng dụng cần phân biệt nội dung file được gửi từ sender nào để thực hiện việc ghép nội dung file được chính xác.

## [20230425: Tuần 3](https://github.com/haiphamcoder/Network-Programming/tree/main/exercises/20230425)

**Bài 1:** Sử dụng hàm select()/poll(), viết chương  trình chat_server thực hiện các chức năng sau:

Nhận kết nối từ các client, và vào hỏi tên client cho đến khi client gửi đúng cú pháp:“client_id: client_name” trong đó client_name là tên của client, xâu ký tự viết liền.

Sau đó nhận dữ liệu từ một client và gửi dữ liệu đó đến các client còn lại, ví dụ:

client có id “abc” gửi “xin chao” thì các client khác sẽ nhận được: “abc: xin chao” hoặc có thể thêm  thời gian vào trước ví dụ:

    2023/05/06 11:00:00PM abc: xin chao

## [20230509: Tuần 4](https://github.com/haiphamcoder/Network-Programming/tree/main/exercises/20230509)

**Bài 1:** Sử dụng hàm select()/poll(), viết chương  trình chat_server thực hiện các chức năng sau:

Nhận kết nối từ các client, và vào hỏi tên client chođến khi client gửi đúng cú pháp:“client_id: client_name”, trong đó client_name là tên của client, xâu ký tự viết liền.

Sau đó nhận dữ liệu từ một client và gửi dữ liệu đóđến các client còn lại, ví dụ: client có id “abc” gửi “xin chao” thì các client khác sẽ nhận được: “abc: xin chao”hoặc có thể thêm thời gian vào trước, ví  dụ: “2023/05/06 11:00:00PM abc: xin chao”.

**Bài 2:** Sử dụng hàm select()/poll(), viết chương trình telnet_server thực hiện các chức năng sau:

Khi đã kết nối với 1 client nào đó, yêu cầu client gửi user và pass, so sánh với file cơ sở dữ liệu là một file text, mỗi dòng chứa một cặp user + pass ví dụ:

    admin admin
    guest nopass
    ...

Nếu so sánh sai, không tìm thấy tài khoản thì báo lỗi đăng nhập. Nếu đúng thì đợi lệnh từ client, thực hiện lệnh và trả kết quả cho client. Dùng hàm system(“dir > out.txt”) để thực hiện lệnh.

dir là ví dụ lệnh dir mà client gửi. > out.txt để định hướng lại dữ liệu ra từ lệnh dir, khi đó kết quả lệnh dir sẽ được ghi vào file văn bản.
