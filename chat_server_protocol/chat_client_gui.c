#include <gtk/gtk.h>

GtkWidget *login_window;
GtkWidget *chat_window;
GtkTextBuffer *chat_buffer;
GtkWidget *chat_view;

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

void on_join_button_clicked(GtkWidget *button, gpointer user_data)
{
    GtkWidget *username_entry = GTK_WIDGET(user_data);

    const gchar *username = gtk_entry_get_text(GTK_ENTRY(username_entry));

    if (g_utf8_strlen(username, -1) > 0)
    {
        if (g_utf8_validate(username, -1, NULL))
        {
            // Join thành công
            gtk_widget_hide(login_window);
            gtk_widget_show_all(chat_window);
        }
        else
        {
            // Thông báo lỗi
            show_message_dialog("Tên người dùng không hợp lệ");
        }
    }
    else
    {
        // Thông báo lỗi
        show_message_dialog("Vui lòng nhập tên người dùng");
    }
}

void on_send_button_clicked(GtkWidget *button, gpointer user_data)
{
    // Gửi tin nhắn
    GtkEntry *entry = GTK_ENTRY(user_data);
    const gchar *text = gtk_entry_get_text(entry);

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
    }
}

void on_chat_window_destroy(GtkWidget *widget, gpointer user_data)
{
    // Đóng cửa sổ chat
    gtk_main_quit();
}

int main(int argc, char *argv[])
{
    gtk_init(&argc, &argv);

    // Tạo cửa sổ đăng nhập
    login_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(login_window), "Đăng nhập");
    gtk_container_set_border_width(GTK_CONTAINER(login_window), 10);
    gtk_widget_set_size_request(login_window, 300, 200);
    g_signal_connect(G_OBJECT(login_window), "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *login_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(login_window), login_box);

    GtkWidget *username_label = gtk_label_new("Tên người dùng");
    GtkWidget *username_entry = gtk_entry_new();
    g_signal_connect(G_OBJECT(username_entry), "activate", G_CALLBACK(on_join_button_clicked), username_entry);
    GtkWidget *join_button = gtk_button_new_with_label("Tham gia");

    gtk_widget_set_hexpand(username_entry, TRUE);

    gtk_box_pack_start(GTK_BOX(login_box), username_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(login_box), username_entry, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(login_box), join_button, FALSE, FALSE, 0);

    g_signal_connect(G_OBJECT(join_button), "clicked", G_CALLBACK(on_join_button_clicked), username_entry);

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

    gtk_widget_hide(chat_window);
    gtk_widget_show_all(login_window);

    gtk_main();

    return 0;
}