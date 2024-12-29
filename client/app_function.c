#include "app_function.h"
#include "utlis.h"

#define PORT 8080
#define BUFFER_SIZE 8192

int sock;
struct sockaddr_in serv_addr;
char buffer[BUFFER_SIZE] = {0};

GtkWidget *window;
GtkWidget *main_box;

void render_login_page(GtkWidget *widget);
void render_view_room_page(GtkWidget *widget);
void render_create_room_dialog(GtkWidget* widget);
void on_create_room_response(GtkDialog *dialog, gint response_id, gpointer *room_data);

void register_request(GtkWidget *widget, gpointer *data) {
    GtkWidget *username_entry = data[0];
    GtkWidget *password_entry = data[1];
    const gchar *username;
    const gchar *password;
    char buffer[BUFFER_SIZE] = {0};
    char response[BUFFER_SIZE] = {0};

    username = gtk_entry_get_text(GTK_ENTRY(username_entry));
    password = gtk_entry_get_text(GTK_ENTRY(password_entry));

    memset(buffer, 0, BUFFER_SIZE);
    sprintf(buffer, "REGISTER %s %s", username, password);
    send(sock, buffer, strlen(buffer), 0);
    memset(buffer, 0, BUFFER_SIZE);
    read(sock, buffer, BUFFER_SIZE);

    
    g_print("Response: %s\n", buffer);
}

void login_request(GtkWidget *widget, gpointer *data) {

    GtkWidget *username_entry = data[0];
    GtkWidget *password_entry = data[1];
    const gchar *username;
    const gchar *password;
    char buffer[BUFFER_SIZE] = {0};
    char response[BUFFER_SIZE] = {0};


    username = gtk_entry_get_text(GTK_ENTRY(username_entry));
    password = gtk_entry_get_text(GTK_ENTRY(password_entry));

printf("username: %s\n", username);
printf("password: %s\n", password);
    memset(buffer, 0, BUFFER_SIZE);
    sprintf(buffer, "LOGIN %s %s", username, password);
    send(sock, buffer, strlen(buffer), 0);
    memset(buffer, 0, BUFFER_SIZE);
    read(sock, buffer, BUFFER_SIZE);


    g_print("Response: %s\n", buffer);
    if (strcmp(buffer, "Login successful") == 0) {
        render_view_room_page(NULL);
    } else {
        GtkWidget *dialog;
        dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Login failed");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }
}

void render_register_page(GtkWidget *widget){
    GtkWidget *username_label;
    GtkWidget *password_label;
    GtkWidget *username_entry;
    GtkWidget *password_entry;
    GtkWidget *register_button;
    GtkWidget *back_button;
    GtkWidget *grid;

    // Remove all children from the main box
    remove_all_children(GTK_CONTAINER(main_box));

    // Create a grid container
    grid = gtk_grid_new();
    
    username_label = gtk_label_new("Username");
    password_label = gtk_label_new("Password");

    username_entry = gtk_entry_new();
    password_entry = gtk_entry_new();
     gtk_entry_set_visibility(GTK_ENTRY(password_entry), FALSE);

    register_button = gtk_button_new_with_label("Register");
    back_button = gtk_button_new_with_label("Back");
    gpointer *data = g_new(gpointer, 2);
    data[0] = username_entry;
    data[1] = password_entry;
    g_signal_connect(register_button, "clicked", G_CALLBACK(register_request), data);

    gtk_grid_attach(GTK_GRID(grid), username_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), password_label, 0, 1, 1, 1);

    // Add entries to the grid
    gtk_grid_attach(GTK_GRID(grid), username_entry, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), password_entry, 1, 1, 1, 1);

    // Add buttons to the grid
    gtk_grid_attach(GTK_GRID(grid), register_button, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), back_button, 1, 2, 1, 1);

    gtk_box_pack_start(GTK_BOX(main_box), grid, TRUE, TRUE, 0);


    // Show all widgets in the window
    gtk_widget_show_all(window);

}

void render_login_page(GtkWidget *widget) {
    GtkWidget *username_label;
    GtkWidget *password_label;
    GtkWidget *username_entry;
    GtkWidget *password_entry;
    GtkWidget *login_button;
    GtkWidget *back_button;
    GtkWidget *grid;

    // Remove all children from the main box
    remove_all_children(GTK_CONTAINER(main_box));

    // Create a grid container
    grid = gtk_grid_new();


    // Create labels
    username_label = gtk_label_new("Username");
    password_label = gtk_label_new("Password");

    // Create entries
    username_entry = gtk_entry_new();
    password_entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(password_entry), FALSE);

    // Create buttons
    login_button = gtk_button_new_with_label("Login");
    back_button = gtk_button_new_with_label("Back");
    gpointer *data = g_new(gpointer, 2);
    data[0] = username_entry;
    data[1] = password_entry;
    g_signal_connect(login_button, "clicked", G_CALLBACK(login_request), data);
  

    // Add labels to the grid
    gtk_grid_attach(GTK_GRID(grid), username_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), password_label, 0, 1, 1, 1);

    // Add entries to the grid
    gtk_grid_attach(GTK_GRID(grid), username_entry, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), password_entry, 1, 1, 1, 1);

    // Add buttons to the grid
    gtk_grid_attach(GTK_GRID(grid), login_button, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), back_button, 1, 2, 1, 1);

    // Add the grid container to the box container
    gtk_box_pack_start(GTK_BOX(main_box), grid, TRUE, TRUE, 0);


    // Show all widgets in the window
    gtk_widget_show_all(window);
}

void render_view_room_page (GtkWidget *widget) {
    GtkWidget *room_view_label;
    GtkWidget *grid;
    GtkWidget *create_room_button;

    remove_all_children(GTK_CONTAINER(main_box));
     room_view_label = gtk_label_new("Room List");

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    create_room_button = gtk_button_new_with_label("Create Room");
    g_signal_connect(create_room_button, "clicked", G_CALLBACK(render_create_room_dialog), NULL);
    gtk_widget_set_name(create_room_button, "create-room-button");
GtkWidget *title_name = gtk_label_new("Name");
    GtkWidget *title_time = gtk_label_new("Time");
    GtkWidget *title_topic = gtk_label_new("Topic");
    // GtkWidget *title_num_questions = gtk_label_new("Number of Questions");
    GtkWidget *title_easy = gtk_label_new("Easy");
    GtkWidget *title_medium = gtk_label_new("Medium");
    GtkWidget *title_hard = gtk_label_new("Hard");
    GtkWidget *title_privacy = gtk_label_new("Privacy");
    GtkWidget *title_num_candidates = gtk_label_new("Number of Candidates");
    GtkWidget *title_join = gtk_label_new("Join");

    gtk_grid_attach(GTK_GRID(grid), title_name, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), title_time, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), title_topic, 2, 0, 1, 1);
    // gtk_grid_attach(GTK_GRID(grid), title_num_questions, 3, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), title_easy, 4, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), title_medium, 5, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), title_hard, 6, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), title_privacy, 7, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), title_num_candidates, 8, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), title_join, 9, 0, 1, 1);

    gtk_box_pack_start(GTK_BOX(main_box), room_view_label, TRUE, TRUE, 0);

    // Add the grid container to the box container
    gtk_box_pack_start(GTK_BOX(main_box), grid, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(main_box), create_room_button, TRUE, FALSE, 0);


    // Show all widgets in the window
    gtk_widget_show_all(window);
  
}

void render_create_room_dialog(GtkWidget* widget) {
    GtkWidget *dialog;
    GtkWidget *grid;
    GtkWidget *name_label;
    GtkWidget *name_entry;
    GtkWidget *time_label;
    GtkWidget *time_entry;
    GtkWidget *topic_label;
    GtkWidget *topic_entry;
    // GtkWidget *num_questions_label;
    // GtkWidget *num_questions_entry;
    GtkWidget *easy_label;
    GtkWidget *easy_entry;
    GtkWidget *medium_label;
    GtkWidget *medium_entry;
    GtkWidget *hard_label;
    GtkWidget *hard_entry;
    GtkWidget *privacy_label;
    GtkWidget *privacy_entry;
    GtkWidget *num_candidates_label;
    GtkWidget *num_candidates_entry;

    dialog = gtk_dialog_new_with_buttons("Create Room", GTK_WINDOW(window), GTK_DIALOG_MODAL, "Create", GTK_RESPONSE_ACCEPT, "Cancel", GTK_RESPONSE_CANCEL, NULL);
     GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    grid = gtk_grid_new();
    name_label = gtk_label_new("Name");
    name_entry = gtk_entry_new();
    time_label = gtk_label_new("Time");
    time_entry = gtk_entry_new();
    topic_label = gtk_label_new("Topic");
    topic_entry = gtk_entry_new();
    // num_questions_label = gtk_label_new("Number of Questions");
    // num_questions_entry = gtk_entry_new();
    easy_label = gtk_label_new("Easy");
    easy_entry = gtk_entry_new();
    medium_label = gtk_label_new("Medium");
    medium_entry = gtk_entry_new();
    hard_label = gtk_label_new("Hard");
    hard_entry = gtk_entry_new();
    privacy_label = gtk_label_new("Privacy");
    privacy_entry = gtk_entry_new();
    num_candidates_label = gtk_label_new("Number of Candidates");
    num_candidates_entry = gtk_entry_new();

    gtk_grid_attach(GTK_GRID(grid), name_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), name_entry, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), time_label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), time_entry, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), topic_label, 0, 2, 1,1);
    gtk_grid_attach(GTK_GRID(grid), topic_entry, 1, 2, 1, 1);
    // gtk_grid_attach(GTK_GRID(grid), num_questions_label, 0, 3, 1, 1);
    // gtk_grid_attach(GTK_GRID(grid), num_questions_entry, 1, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), easy_label, 0, 4, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), easy_entry, 1, 4, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), medium_label, 0, 5, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), medium_entry, 1, 5, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), hard_label, 0, 6, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), hard_entry, 1, 6, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), privacy_label, 0, 7, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), privacy_entry, 1, 7, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), num_candidates_label, 0, 8, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), num_candidates_entry, 1, 8, 1, 1);

    gtk_container_add(GTK_CONTAINER(content_area), grid);
    gtk_widget_show_all(content_area);
    gtk_widget_show_all(dialog);
    gpointer *data = g_new(gpointer, 8);
    data[0] = name_entry;
    data[2] = easy_entry;
    data[3] = medium_entry;
    data[4] = hard_entry;
    data[5] = time_entry;
    data[1] = topic_entry;
    data[6] = privacy_entry;
    data[7] = num_candidates_entry;

    g_signal_connect(dialog, "response", G_CALLBACK(on_create_room_response), data);
}

void on_create_room_response(GtkDialog *dialog, gint response_id, gpointer *room_data) {
    if (response_id == GTK_RESPONSE_ACCEPT) {
       const char * room_name =  gtk_entry_get_text(GTK_ENTRY(room_data[0]));
            const char * easy =  gtk_entry_get_text(GTK_ENTRY(room_data[2]));
            const char * medium =  gtk_entry_get_text(GTK_ENTRY(room_data[3]));
            const char * hard =  gtk_entry_get_text(GTK_ENTRY(room_data[4]));
         const char * time =  gtk_entry_get_text(GTK_ENTRY(room_data[5]));
            const char * topic =  gtk_entry_get_text(GTK_ENTRY(room_data[1]));
            const char * privacy =  gtk_entry_get_text(GTK_ENTRY(room_data[6]));
            // const char * num_questions =  gtk_entry_get_text(GTK_ENTRY(room_data[3]));
            const char * num_candidates =  gtk_entry_get_text(GTK_ENTRY(room_data[7]));
            memset(buffer, 0, BUFFER_SIZE);
            buffer[0] = 0x01;
            sprintf(buffer + 1, "%s %s %s %s %s %s %s %s ", room_name,easy, medium, hard, time, topic, privacy, num_candidates);
            send(sock, buffer, BUFFER_SIZE, 0);
        // g_print("Room name: %s\n %s\n %s\n %s\n %s\n %s\n %s\n %s\n  ", room_name, easy, medium, hard, time, topic, privacy, num_candidates);
       g_print("Room name: %s\n %s\n %s\n %s\n %s\n %s\n %s\n %s\n  ", room_name, easy, medium, hard, time, topic, privacy, num_candidates);
       gtk_widget_destroy(GTK_WIDGET(dialog));
    } else {
        gtk_widget_destroy(GTK_WIDGET(dialog));
    }
    g_free(room_data);
    
}
//nho e

void activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *header_bar;
    GtkWidget *title_label;

    GtkWidget *loginBtn;
    GtkWidget *registerBtn;
    GtkWidget *grid;

    


    // Create a new window with the application
    window = gtk_application_window_new(app);
    gtk_window_set_default_size(GTK_WINDOW(window), 900, 600);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_window_close), NULL);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
    header_bar = gtk_header_bar_new();
    gtk_header_bar_set_title(GTK_HEADER_BAR(header_bar), "Custom Window Title");
    gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(header_bar), TRUE);
    title_label = gtk_label_new("Vòng xoay trắc nghiệm");
    gtk_widget_set_name(title_label, "title");
    gtk_header_bar_set_title(GTK_HEADER_BAR(header_bar), NULL);
    gtk_container_add(GTK_CONTAINER(header_bar), title_label);
    gtk_window_set_titlebar(GTK_WINDOW(window), header_bar);
    load_css(window);


    // Create a grid container
    grid = gtk_grid_new();

    loginBtn = gtk_button_new_with_label("Login");
    registerBtn = gtk_button_new_with_label("Register");

    g_signal_connect(loginBtn, "clicked", G_CALLBACK(render_login_page), NULL);
    g_signal_connect(registerBtn, "clicked", G_CALLBACK(render_register_page), NULL);


    // Create a box container (vertical or horizontal)
    main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_name(main_box, "content");



    // Add buttons to the grid
    gtk_grid_attach(GTK_GRID(grid), loginBtn, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), registerBtn, 1, 0, 1, 1);

    // Add the grid container to the box container
    gtk_box_pack_start(GTK_BOX(main_box), grid, TRUE, TRUE, 0);

    // Add the box container to the window
    gtk_container_add(GTK_CONTAINER(window), main_box);
    // Show all widgets in the window
    gtk_widget_show_all(window);
    create_app_socket(&sock, &serv_addr);

    g_print("%d\n", sock);

}