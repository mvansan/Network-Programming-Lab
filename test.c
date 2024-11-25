#include <gtk/gtk.h>

static void activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window;
    GtkWidget *button;
    GtkWidget *background;

    // Tạo cửa sổ chính
    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Simple GTK Application");
    gtk_window_set_default_size(GTK_WINDOW(window), 900, 800);

    // Tạo lớp chứa để đặt ảnh nền và nút
    GtkWidget *overlay = gtk_overlay_new();
    gtk_container_add(GTK_CONTAINER(window), overlay);

    // Tải ảnh làm nền
    background = gtk_image_new_from_file("IMG_7056.jpg");
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), background);

    // Tạo nút
    button = gtk_button_new_with_label("Click Me");
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), button); 

    // Căn chỉnh nút
    gtk_widget_set_halign(button, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(button, GTK_ALIGN_CENTER);

    // Hiển thị tất cả widget
    gtk_widget_show_all(window);
}

int main(int argc, char **argv) {
    GtkApplication *app;
    int status;

    app = gtk_application_new("com.example.GtkApplication", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
