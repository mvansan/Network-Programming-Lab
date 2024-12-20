#include "utlis.h"
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
// #include <gst/gst.h>

#define PORT 8080
#define BUFFER_SIZE 1024

void create_app_socket(int *sock, struct sockaddr_in *serv_addr) {
    if ((*sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        g_print("Socket creation error\n");
        return;
    }

    serv_addr->sin_family = AF_INET;
    serv_addr->sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr->sin_addr) <= 0) {
        g_print("Invalid address/ Address not supported \n");
        return;
    }

    if (connect(*sock, (struct sockaddr *)serv_addr, sizeof(*serv_addr)) < 0) {
        g_print("Connection Failed \n");
        return;
    }
}

void load_css(GtkWidget *widget) {
    GtkCssProvider *provider = gtk_css_provider_new();
    GdkDisplay *display = gdk_display_get_default();
    GdkScreen *screen = gdk_display_get_default_screen(display);

    gtk_style_context_add_provider_for_screen(screen, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);

    GError *error = NULL;
    gtk_css_provider_load_from_path(provider, "./style.css", &error);
    if (error) {
        g_warning("Error loading CSS file: %s", error->message);
        g_clear_error(&error);
    }

    g_object_unref(provider);
}

void remove_all_children(GtkContainer *container) {
    GList *children = gtk_container_get_children(container);
    for (GList *iter = children; iter != NULL; iter = iter->next) {
        GtkWidget *child = GTK_WIDGET(iter->data);
        g_signal_handlers_disconnect_by_data(child, NULL);
        gtk_widget_destroy(child);
    }
    g_list_free(children);
}

void remove_child_by_name(GtkContainer *container, const gchar *name) {
    GList *children = gtk_container_get_children(container);
    for (GList *iter = children; iter != NULL; iter = iter->next) {
        GtkWidget *child = GTK_WIDGET(iter->data);
        if (g_strcmp0(gtk_widget_get_name(child), name) == 0) {
            gtk_widget_destroy(child);
        }
    }
    g_list_free(children);
}

void add_css_class_to_widget(GtkWidget *widget, const gchar *css_class) {
    GtkStyleContext *context = gtk_widget_get_style_context(widget);
    gtk_style_context_add_class(context, css_class);
}

// void play_sound_effect(const char *file_path) {
//     GError *error = NULL;
//     GstElement *pipeline;

//     // Initialize GStreamer
//     gst_init(NULL, NULL);

//     // Stop and unref the current pipeline if it exists
//     if (current_pipeline) {
//         gst_element_set_state(current_pipeline, GST_STATE_NULL);
//         gst_object_unref(current_pipeline);
//         current_pipeline = NULL;
//     }

//     // Create a new pipeline to play the sound
//     char pipeline_description[256];
//     snprintf(pipeline_description, sizeof(pipeline_description), "playbin uri=file://%s", file_path);

//     pipeline = gst_parse_launch(pipeline_description, &error);
//     if (!pipeline) {
//         g_printerr("Failed to create pipeline: %s\n", error->message);
//         g_error_free(error);
//         return;
//     }

//     // Set the new pipeline as the current pipeline
//     current_pipeline = pipeline;

//     // Start playing the audio
//     gst_element_set_state(pipeline, GST_STATE_PLAYING);

//     // Wait until playback finishes
//     GstBus *bus = gst_element_get_bus(pipeline);
//     gst_bus_poll(bus, GST_MESSAGE_EOS, -1);
//     gst_object_unref(bus);

//     // Clean up the pipeline
//     gst_element_set_state(pipeline, GST_STATE_NULL);
//     gst_object_unref(pipeline);
//     current_pipeline = NULL;
// }
