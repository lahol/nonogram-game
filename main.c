#include <stdio.h>
#include <glib.h>
#include <gtk/gtk.h>
#include "ng-data.h"
#include "ng-view.h"
#include "ng-window.h"

int main(int argc, char **argv)
{
    gtk_init(&argc, &argv);
    
    NgWindow *window = ng_window_init();

    gtk_main();

    ng_window_destroy(window);

    return 0;
}
