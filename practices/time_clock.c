#include <gtk/gtk.h>
#include <stdbool.h>

GtkWidget *window;
GtkWidget *label;
GtkWidget *startButton;
GtkWidget *pauseButton;
GtkWidget *resetButton;

gint64 startTime;
gint64 pauseTime;
gboolean isRunning;

gboolean updateTimer(gpointer data)
{
    if (isRunning)
    {
        gint64 currentTime = g_get_monotonic_time();
        gint64 elapsedTime = currentTime - startTime;
        elapsedTime /= 1000; // Convert to milliseconds

        gint hours = (gint)(elapsedTime / (3600 * 1000));
        gint minutes = (gint)((elapsedTime / (60 * 1000)) % 60);
        gint seconds = (gint)((elapsedTime / 1000) % 60);
        gint milliseconds = (gint)(elapsedTime % 1000);

        gchar *timeString = g_strdup_printf("%02d:%02d:%02d.%03d", hours, minutes, seconds, milliseconds);

        gtk_label_set_text(GTK_LABEL(label), timeString);

        g_free(timeString);
    }

    return G_SOURCE_CONTINUE;
}

void onStartButtonClicked(GtkWidget *widget, gpointer data)
{
    if (!isRunning)
    {
        isRunning = true;
        startTime = g_get_monotonic_time();
        gtk_widget_set_sensitive(startButton, FALSE);           // Disable the start button
        gtk_widget_set_sensitive(pauseButton, TRUE);            // Enable the pause button
        gtk_button_set_label(GTK_BUTTON(pauseButton), "Pause"); // Change the label of the pause button
    }
}

void onPauseButtonClicked(GtkWidget *widget, gpointer data)
{
    if (isRunning)
    {
        isRunning = false;
        pauseTime = g_get_monotonic_time();
        gtk_button_set_label(GTK_BUTTON(pauseButton), "Continue"); // Change the label of the pause button
    }
    else
    {
        isRunning = true;
        startTime += g_get_monotonic_time() - pauseTime;
        gtk_button_set_label(GTK_BUTTON(pauseButton), "Pause"); // Change the label of the pause button
    }
}

void onResetButtonClicked(GtkWidget *widget, gpointer data)
{
    isRunning = false;
    startTime = 0;
    pauseTime = 0;
    gtk_label_set_text(GTK_LABEL(label), "00:00:00.000");
    gtk_widget_set_sensitive(startButton, TRUE);            // Enable the start button
    gtk_widget_set_sensitive(pauseButton, FALSE);           // Disable the pause button
    gtk_button_set_label(GTK_BUTTON(pauseButton), "Pause"); // Reset the label of the pause button
}

void createUI()
{
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "StopWatch");
    gtk_window_set_default_size(GTK_WINDOW(window), 200, 100);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_column_homogeneous(GTK_GRID(grid), TRUE);
    gtk_grid_set_row_homogeneous(GTK_GRID(grid), TRUE);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_container_add(GTK_CONTAINER(window), grid);

    label = gtk_label_new("00:00:00.000");
    gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(label, GTK_ALIGN_CENTER);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 2, 1);

    startButton = gtk_button_new_with_label("Start");
    g_signal_connect(startButton, "clicked", G_CALLBACK(onStartButtonClicked), NULL);
    gtk_widget_set_halign(startButton, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(startButton, GTK_ALIGN_CENTER);
    gtk_widget_set_sensitive(pauseButton, FALSE); // Disable the pause button initially
    gtk_grid_attach(GTK_GRID(grid), startButton, 0, 1, 1, 1);

    pauseButton = gtk_button_new_with_label("Pause");
    g_signal_connect(pauseButton, "clicked", G_CALLBACK(onPauseButtonClicked), NULL);
    gtk_widget_set_halign(pauseButton, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(pauseButton, GTK_ALIGN_CENTER);
    gtk_widget_set_sensitive(pauseButton, FALSE); // Disable the pause button initially
    gtk_grid_attach(GTK_GRID(grid), pauseButton, 1, 1, 1, 1);

    resetButton = gtk_button_new_with_label("Reset");
    g_signal_connect(resetButton, "clicked", G_CALLBACK(onResetButtonClicked), NULL);
    gtk_widget_set_halign(resetButton, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(resetButton, GTK_ALIGN_CENTER);
    gtk_grid_attach(GTK_GRID(grid), resetButton, 0, 2, 2, 1);
    gtk_widget_show_all(window);
}

int main(int argc, char *argv[])
{
    gtk_init(&argc, &argv);

    isRunning = false;

    createUI();

    g_timeout_add(10, updateTimer, NULL);

    gtk_main();

    return 0;
}
