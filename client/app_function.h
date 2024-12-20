#ifndef APP_FUNCTION_H
#define APP_FUNCTION_H

#include <gtk/gtk.h>

typedef struct {
    GtkLabel *label;
    time_t end_time;
} CountdownData;

// Declare the activate function
void activate(GtkApplication *app, gpointer user_data);

#endif // APP_FUNCTION_H