#include <gtk/gtk.h>

// Data structure for room information
typedef struct {
    gchar *room_name;
    gint num_questions;
    gint test_duration;
    gchar *status; // Room status (e.g., Pending)
} Room;

// List to store rooms
GList *room_list = NULL;

// Function to display a dialog for registration or login
static void on_auth_button_clicked(GtkWidget *widget, gpointer user_data) {
    const gchar *title = (const gchar *)user_data;
    GtkWidget *dialog, *content_area, *dialog_box;
    GtkWidget *username_label, *username_entry;
    GtkWidget *password_label, *password_entry;

    dialog = gtk_dialog_new_with_buttons(title,
                                         NULL,
                                         GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                         "OK", GTK_RESPONSE_OK,
                                         "Cancel", GTK_RESPONSE_CANCEL,
                                         NULL);

    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    dialog_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

    username_label = gtk_label_new("Username:");
    gtk_box_pack_start(GTK_BOX(dialog_box), username_label, FALSE, FALSE, 0);

    username_entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(dialog_box), username_entry, FALSE, FALSE, 0);

    password_label = gtk_label_new("Password:");
    gtk_box_pack_start(GTK_BOX(dialog_box), password_label, FALSE, FALSE, 0);

    password_entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(password_entry), FALSE);
    gtk_box_pack_start(GTK_BOX(dialog_box), password_entry, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(content_area), dialog_box, TRUE, TRUE, 0);
    gtk_widget_show_all(dialog);

    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    if (response == GTK_RESPONSE_OK) {
        const gchar *username = gtk_entry_get_text(GTK_ENTRY(username_entry));
        const gchar *password = gtk_entry_get_text(GTK_ENTRY(password_entry));
        g_print("%s successful for user: %s\n", title, username);
    }

    gtk_widget_destroy(dialog);
}

// Function to display the room list
static void show_room_list(GtkWidget *widget, gpointer user_data) {
    GtkWindow *parent_window = GTK_WINDOW(user_data);
    GtkWidget *dialog, *content_area, *list_box, *room_label;

    dialog = gtk_dialog_new_with_buttons("Room List",
                                         parent_window,
                                         GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                         "Close", GTK_RESPONSE_CLOSE,
                                         NULL);

    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    list_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

    for (GList *iter = room_list; iter != NULL; iter = iter->next) {
        Room *room = (Room *)iter->data;
        gchar *room_info = g_strdup_printf("Room: %s | Questions: %d | Duration: %d min | Status: %s",
                                           room->room_name,
                                           room->num_questions,
                                           room->test_duration,
                                           room->status);
        room_label = gtk_label_new(room_info);
        gtk_box_pack_start(GTK_BOX(list_box), room_label, FALSE, FALSE, 0);
        g_free(room_info);
    }

    gtk_box_pack_start(GTK_BOX(content_area), list_box, TRUE, TRUE, 0);
    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

// Function to create a room
static void on_create_room_clicked(GtkWidget *widget, gpointer user_data) {
    GtkWindow *parent_window = GTK_WINDOW(user_data);
    GtkWidget *dialog, *content_area, *dialog_box;
    GtkWidget *name_label, *name_entry;
    GtkWidget *questions_label, *questions_entry;
    GtkWidget *duration_label, *duration_entry;

    dialog = gtk_dialog_new_with_buttons("Create Room",
                                         parent_window,
                                         GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                         "OK", GTK_RESPONSE_OK,
                                         "Cancel", GTK_RESPONSE_CANCEL,
                                         NULL);

    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    dialog_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

    name_label = gtk_label_new("Type room's name:");
    gtk_box_pack_start(GTK_BOX(dialog_box), name_label, FALSE, FALSE, 0);

    name_entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(dialog_box), name_entry, FALSE, FALSE, 0);

    questions_label = gtk_label_new("Number of questions:");
    gtk_box_pack_start(GTK_BOX(dialog_box), questions_label, FALSE, FALSE, 0);

    questions_entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(dialog_box), questions_entry, FALSE, FALSE, 0);

    duration_label = gtk_label_new("Test duration (minutes):");
    gtk_box_pack_start(GTK_BOX(dialog_box), duration_label, FALSE, FALSE, 0);

    duration_entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(dialog_box), duration_entry, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(content_area), dialog_box, TRUE, TRUE, 0);
    gtk_widget_show_all(dialog);

    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    if (response == GTK_RESPONSE_OK) {
        const gchar *room_name = gtk_entry_get_text(GTK_ENTRY(name_entry));
        const gchar *num_questions = gtk_entry_get_text(GTK_ENTRY(questions_entry));
        const gchar *test_duration = gtk_entry_get_text(GTK_ENTRY(duration_entry));

        if (g_strcmp0(room_name, "") != 0 && g_strcmp0(num_questions, "") != 0 && g_strcmp0(test_duration, "") != 0) {
            Room *new_room = g_malloc(sizeof(Room));
            new_room->room_name = g_strdup(room_name);
            new_room->num_questions = atoi(num_questions);
            new_room->test_duration = atoi(test_duration);
            new_room->status = g_strdup("Pending");

            room_list = g_list_append(room_list, new_room);
            show_room_list(NULL, parent_window);
        } else {
            g_print("Invalid input. Please fill all fields.\n");
        }
    }

    gtk_widget_destroy(dialog);
}

static void activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window, *overlay, *background;
    GtkWidget *button_box, *create_room_button, *view_room_button;
    GtkWidget *register_button, *login_button;

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Room Management");
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);

    overlay = gtk_overlay_new();
    gtk_container_add(GTK_CONTAINER(window), overlay);

    background = gtk_image_new_from_file("bg.jpg");
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), background);

    button_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_halign(button_box, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(button_box, GTK_ALIGN_CENTER);
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), button_box);

    create_room_button = gtk_button_new_with_label("Create Room");
    g_signal_connect(create_room_button, "clicked", G_CALLBACK(on_create_room_clicked), window);
    gtk_box_pack_start(GTK_BOX(button_box), create_room_button, TRUE, FALSE, 0);

    view_room_button = gtk_button_new_with_label("Show Room");
    g_signal_connect(view_room_button, "clicked", G_CALLBACK(show_room_list), window);
    gtk_box_pack_start(GTK_BOX(button_box), view_room_button, TRUE, FALSE, 0);

    register_button = gtk_button_new_with_label("Register");
    gtk_widget_set_halign(register_button, GTK_ALIGN_END);
    gtk_widget_set_valign(register_button, GTK_ALIGN_START);
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), register_button);
    g_signal_connect(register_button, "clicked", G_CALLBACK(on_auth_button_clicked), "Register");

    login_button = gtk_button_new_with_label("Login");
    gtk_widget_set_halign(login_button, GTK_ALIGN_END);
    gtk_widget_set_valign(login_button, GTK_ALIGN_START);
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), login_button);
    g_signal_connect(login_button, "clicked", G_CALLBACK(on_auth_button_clicked), "Login");

    gtk_widget_show_all(window);
}

int main(int argc, char **argv) {
    GtkApplication *app = gtk_application_new("com.example.room_manager", G_APPLICATION_FLAGS_NONE);
    int status;

    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
