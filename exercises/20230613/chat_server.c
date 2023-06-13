#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#define MAX_CLIENTS 2

pthread_mutex_t mutex; // Mutex để đồng bộ hóa truy cập vào hàng đợi và gửi tin nhắn
pthread_cond_t cond;   // Biến điều kiện để kiểm tra số lượng client trong hàng đợi

int client_queue[MAX_CLIENTS]; // Hàng đợi chứa các client
int connected_clients = 0;     // Số lượng client đã kết nối

void *client_thread(void *arg)
{
    int client_id = *(int *)arg;
    printf("Client %d connected\n", client_id);

    while (1)
    {
        pthread_mutex_lock(&mutex);

        // Kiểm tra xem có đủ 2 client trong hàng đợi hay không
        if (connected_clients < MAX_CLIENTS)
        {
            client_queue[connected_clients++] = client_id;

            // Nếu đã đủ 2 client, gửi tin nhắn cho client còn lại
            if (connected_clients == MAX_CLIENTS)
            {
                printf("Matched clients %d and %d\n", client_queue[0], client_queue[1]);
                // Code chuyển tiếp tin nhắn giữa hai client

                connected_clients = 0;
                pthread_cond_broadcast(&cond);
            }
            else
            {
                pthread_cond_wait(&cond, &mutex);
            }
        }
        else
        {
            // Kiểm tra xem client hiện tại có bị ngắt kết nối không
            if (client_id != client_queue[0] && client_id != client_queue[1])
            {
                printf("Client %d disconnected\n", client_id);
                break;
            }
        }

        pthread_mutex_unlock(&mutex);
    }

    // Thực hiện các thao tác giải phóng tài nguyên, đóng kết nối, v.v. tại đây
    printf("Client %d disconnected\n", client_id);

    pthread_exit(NULL);
}

int main()
{
    pthread_t tid[MAX_CLIENTS];
    int client_ids[MAX_CLIENTS];

    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);

    // Tạo và chạy luồng cho mỗi client kết nối
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        client_ids[i] = i + 1;
        pthread_create(&tid[i], NULL, client_thread, &client_ids[i]);
    }

    // Đợi cho tất cả các luồng kết thúc
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        pthread_join(tid[i], NULL);
    }

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);

    return 0;
}
