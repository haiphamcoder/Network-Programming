#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>

GtkWidget *login_window;
GtkWidget *chat_window;
GtkTextBuffer *chat_buffer;
GtkWidget *chat_view;

int client_socket = -1;

void show_message_dialog(const gchar *message)
{
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(login_window),
                                               GTK_DIALOG_MODAL,
                                               GTK_MESSAGE_INFO,
                                               GTK_BUTTONS_OK,
                                               "%s", message);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

void on_send_button_clicked(GtkWidget *button, gpointer user_data)
{
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));

    // Gửi tin nhắn
    GtkEntry *entry = GTK_ENTRY(user_data);
    const gchar *text = gtk_entry_get_text(entry);
    sprintf(buffer, "%s\n", text);

    if (g_utf8_validate(text, -1, NULL) && g_utf8_strlen(text, -1) > 0)
    {
        // Tin nhắn hợp lệ, hiển thị trong chat_view
        GtkTextIter iter;
        gtk_text_buffer_get_end_iter(chat_buffer, &iter);
        gtk_text_buffer_insert(chat_buffer, &iter, text, -1);
        gtk_text_buffer_insert(chat_buffer, &iter, "\n", -1);

        // Cuộn xuống cuối cùng
        GtkAdjustment *adjustment = gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(chat_view));
        gtk_adjustment_set_value(adjustment, gtk_adjustment_get_upper(adjustment) - gtk_adjustment_get_page_size(adjustment));
        gtk_scrollable_set_vadjustment(GTK_SCROLLABLE(chat_view), adjustment);

        // Xóa nội dung tin nhắn
        gtk_entry_set_text(entry, "");

        if (send(client_socket, buffer, strlen(buffer), 0) < 0)
        {
            if(!(errno == EAGAIN || errno == EWOULDBLOCK))
            {
                // Lỗi
                perror("send");
                return;
            }
        }
    }
}

void on_chat_window_destroy(GtkWidget *widget, gpointer user_data)
{
    // Đóng cửa sổ chat
    gtk_main_quit();
}

void *client_recv_handle(void *arg)
{
    gchar buffer[1024];

    while (1)
    {
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received > 0)
        {
            // Tin nhắn hợp lệ, hiển thị trong chat_view
            GtkTextIter iter;
            gtk_text_buffer_get_end_iter(chat_buffer, &iter);
            gtk_text_buffer_insert(chat_buffer, &iter, buffer, -1);

            // Cuộn xuống cuối cùng
            GtkAdjustment *adjustment = gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(chat_view));
            gtk_adjustment_set_value(adjustment, gtk_adjustment_get_upper(adjustment) - gtk_adjustment_get_page_size(adjustment));
            gtk_scrollable_set_vadjustment(GTK_SCROLLABLE(chat_view), adjustment);
        }
        else if (bytes_received == 0)
        {
            // Đóng kết nối
            close(client_socket);
            client_socket = -1;

            // Thông báo lỗi
            show_message_dialog("Server disconnected");

            // Đóng cửa sổ chat
            gtk_main_quit();
        }
        else if (bytes_received < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // Không có dữ liệu
                usleep(1000);
            }
            else
            {
                // Lỗi
                perror("recv");
                break;
            }
        }
    }

    return NULL;
}

int main(int argc, char *argv[])
{
    // Khởi tạo socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0)
    {
        perror("socket");
        return 1;
    }

    // Thiết lập địa chỉ server
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(9000);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Kết nối tới server
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("connect");
        return 1;
    }

    fcntl(client_socket, F_SETFL, O_NONBLOCK);

    // Khởi tạo GTK
    gtk_init(&argc, &argv);

    // Tạo cửa sổ chat
    chat_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(chat_window), "Chat");
    gtk_container_set_border_width(GTK_CONTAINER(chat_window), 10);
    gtk_widget_set_size_request(chat_window, 400, 300);
    g_signal_connect(G_OBJECT(chat_window), "destroy", G_CALLBACK(on_chat_window_destroy), NULL);

    GtkWidget *chat_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(chat_window), chat_box);

    // Tạo khu vực hiển thị tin nhắn
    GtkScrolledWindow *chat_scroll = GTK_SCROLLED_WINDOW(gtk_scrolled_window_new(NULL, NULL)); // Khởi tạo chat_scroll
    gtk_scrolled_window_set_policy(chat_scroll, GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
    gtk_box_pack_start(GTK_BOX(chat_box), GTK_WIDGET(chat_scroll), TRUE, TRUE, 0);

    chat_view = gtk_text_view_new();
    chat_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(chat_view));
    gtk_text_view_set_editable(GTK_TEXT_VIEW(chat_view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(chat_view), FALSE);
    gtk_container_add(GTK_CONTAINER(chat_scroll), chat_view);

    // Tạo hộp nhập tin nhắn
    GtkWidget *input_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(chat_box), input_box, FALSE, FALSE, 0);

    GtkWidget *input_entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(input_box), input_entry, TRUE, TRUE, 0);

    GtkWidget *send_button = gtk_button_new_with_label("Gửi");
    g_signal_connect(G_OBJECT(send_button), "clicked", G_CALLBACK(on_send_button_clicked), input_entry);
    gtk_box_pack_start(GTK_BOX(input_box), send_button, FALSE, FALSE, 0);

    // Xử lý sự kiện nhấn phím Enter từ input_entry
    g_signal_connect(G_OBJECT(input_entry), "activate", G_CALLBACK(on_send_button_clicked), input_entry);

    gtk_widget_show_all(chat_window);

    // Xử lý nhận tin nhắn
    pthread_t recv_thread;
    if (pthread_create(&recv_thread, NULL, client_recv_handle, NULL) < 0)
    {
        show_message_dialog("Error creating thread");
        close(client_socket);
        client_socket = -1;
        gtk_main_quit();
    }

    gtk_main();

    return 0;
}