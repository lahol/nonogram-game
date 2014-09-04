#include <stdio.h>
#include <glib.h>
#include <gtk/gtk.h>
#include "ng-data.h"
#include "ng-view.h"
#include "ng-window.h"

int main(int argc, char **argv)
{
    gtk_init(&argc, &argv);
    
    Nonogram *ng = ng_read_data_from_file("example.ng");
    if (ng) {
        printf("successfully read nonogram\n");
        printf("size: %dx%d\n", ng->width, ng->height);
    }
    else {
        printf("an error occured\n");
    }

    NgView *view = ng_view_new(ng);

    NgWindow *window = ng_window_init();
    ng_window_set_view(window, view);

    gtk_main();

    ng_view_free(view);
    ng_free(ng);
    ng_window_destroy(window);

    return 0;
}
