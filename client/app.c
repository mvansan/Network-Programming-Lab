#include <gtk/gtk.h>
#include "app_function.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <uuid/uuid.h>
#include <ctype.h>

#define APP_ID_PREFIX "com.example."

void generate_application_id(char *app_id, size_t len) {
    uuid_t uuid;
    char uuid_str[37]; // UUID string length (36 characters + null terminator)

    // Generate a UUID
    uuid_generate(uuid);
    uuid_unparse(uuid, uuid_str);

    // Use only the first 8 characters of the UUID
    uuid_str[8] = '\0';

    // Ensure the first character is a letter
    if (!isalpha(uuid_str[0])) {
        uuid_str[0] = 'a' + (uuid_str[0] % 26); // Replace with a random letter
    }

    // Combine the prefix with the adjusted UUID to form the application ID
    snprintf(app_id, len, "%s%s", APP_ID_PREFIX, uuid_str);
}

int main(int argc, char **argv) {
    GtkApplication *app;
    int status;
    char app_id[64];

    generate_application_id(app_id, sizeof(app_id));
    g_print("Generated Application ID: %s\n", app_id);

    // Check if the UUID is valid
    // if (!g_application_id_is_valid(app_id)) {
    //     g_print("Invalid application ID: %s\n", app_id);
    //     return 1;
    // }

    // Create a new GtkApplication with the unique UUID as the application ID
    app = gtk_application_new(app_id, 0);

    // Check if the application was created successfully
    if (app == NULL) {
        g_print("Failed to create GtkApplication\n");
        return 1;
    }

    // Connect the "activate" signal to the activate function
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

    // Run the application
    status = g_application_run(G_APPLICATION(app), argc, argv);

    // Free the memory allocated for the GtkApplication
    g_object_unref(app);

    return status;
}