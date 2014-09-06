#include "ng-window.h"
#include "ng-data.h"

struct _NgWindow {
    GtkWidget *window;
    GtkWidget *drawing_area;
    GtkWidget *menu_bar;
    GtkWidget *status_bar;
    NgView *view;
};

gboolean ng_window_event_draw(GtkWidget *widget, cairo_t *cr, NgWindow *win)
{
    if (win && win->view) {
        ng_view_render(win->view, cr);
    }
    else {
        guint width, height;

        width = gtk_widget_get_allocated_width(widget);
        height = gtk_widget_get_allocated_height(widget);

        cairo_rectangle(cr, 0, 0, width, height);
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
        cairo_fill(cr);
    }
    return FALSE;
}

gboolean ng_window_configure_event(GtkWidget *widget, GdkEventConfigure *event, NgWindow *win)
{
    /* event->width, event->height */
        guint width, height;

        width = gtk_widget_get_allocated_width(widget);
        height = gtk_widget_get_allocated_height(widget);

    if (win && win->view) {
        ng_view_set_size(win->view, event->width, event->height);
        ng_window_update(win);
    }
    return FALSE;
}

gboolean ng_window_button_press_event(GtkWidget *widget, GdkEventButton *event, NgWindow *win)
{
    if (win == NULL)
        return TRUE;

    guint vx, vy;
    NgViewCoordinateType type = ng_view_translate_coordinate(win->view, event->x,
            event->y, &vx, &vy);
   
    if (type == NG_VIEW_COORDINATE_FIELD) {
        ng_view_tmp_line_start(win->view, vx, vy);
    }
    ng_window_update(win);

    return TRUE;
}

gboolean ng_window_motion_notify_event(GtkWidget *widget, GdkEventMotion *event, NgWindow *win)
{
    if (win == NULL)
        return TRUE;

    guint vx, vy;
    NgViewCoordinateType type = ng_view_translate_coordinate(win->view, event->x,
            event->y, &vx, &vy);
   
    if (type == NG_VIEW_COORDINATE_FIELD) {
        ng_view_tmp_line_end(win->view, vx, vy);
    }
    ng_window_update(win);

    return TRUE;
}

gboolean ng_window_button_release_event(GtkWidget *widget, GdkEventButton *event, NgWindow *win)
{
    if (win == NULL)
        return TRUE;

    guchar value = 0xff;
    guint vx, vy;
    NgViewCoordinateType type = ng_view_translate_coordinate(win->view, event->x,
            event->y, &vx, &vy);
   
    if (type == NG_VIEW_COORDINATE_FIELD) {
        ng_view_tmp_line_end(win->view, vx, vy);

        if (event->button == 1) {
            if (event->state & GDK_SHIFT_MASK)
                value = 0;
            else
                value = 1;
        }
        else if (event->button == 3) {
            if (event->state & GDK_SHIFT_MASK)
                value = 0;
            else
                value = 2;
        }
        else
            goto done;
        guint sx, sy, ex, ey;
        if (ng_view_tmp_line_finish(win->view, &sx, &sy, &ex, &ey)) {
            ng_fill_area(ng_view_get_data(win->view), sx, sy, ex-sx+1, ey-sy+1, value);
            ng_view_update_map(win->view, sx, sy, ex-sx+1, ey-sy+1);
            ng_window_update(win);
        }
    }

done:
    ng_view_tmp_line_clear(win->view);

    return TRUE;
}

gboolean ng_window_scroll_event(GtkWidget *widget, GdkEventScroll *event, NgWindow *win)
{
    if (win && win->view) {
        NgViewScrollDirection dir;
        switch (event->direction) {
            case GDK_SCROLL_LEFT: dir = NG_VIEW_SCROLL_LEFT; break;
            case GDK_SCROLL_RIGHT: dir = NG_VIEW_SCROLL_RIGHT; break;
            case GDK_SCROLL_UP:
                if (event->state & GDK_SHIFT_MASK)
                    dir = NG_VIEW_SCROLL_LEFT;
                else
                    dir = NG_VIEW_SCROLL_UP;
                break;
            case GDK_SCROLL_DOWN:
                if (event->state & GDK_SHIFT_MASK)
                    dir = NG_VIEW_SCROLL_RIGHT;
                else
                    dir = NG_VIEW_SCROLL_DOWN;
                break;
            default:
                return TRUE;
        }
        ng_view_scroll(win->view, dir);
        ng_window_update(win);
    }
    return TRUE;
}

gboolean ng_window_key_release_event(GtkWidget *widget, GdkEventKey *event, NgWindow *win)
{
    if (event->keyval == GDK_KEY_Escape)
        gtk_main_quit();
    return TRUE;
}

static void ng_activate_menu_item_open(GtkMenuItem *item, NgWindow *win)
{
}

static void ng_activate_menu_item_save(GtkMenuItem *item, NgWindow *win)
{
}

static void ng_activate_menu_item_reset(GtkMenuItem *item, NgWindow *win)
{
}

static void ng_activate_menu_item_quit(GtkMenuItem *item, NgWindow *win)
{
    gtk_main_quit();
}

GtkWidget *ng_window_create_main_menu(NgWindow *win)
{
    GtkWidget *menu = gtk_menu_bar_new();
    GtkWidget *submenu = gtk_menu_new();
    GtkWidget *item;

    item = gtk_menu_item_new_with_label("Open");
    g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(ng_activate_menu_item_open), win);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    gtk_widget_show(item);

    item = gtk_menu_item_new_with_label("Save");
    g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(ng_activate_menu_item_save), win);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    gtk_widget_show(item);

    item = gtk_menu_item_new_with_label("Reset");
    g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(ng_activate_menu_item_reset), win);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    gtk_widget_show(item);

    item = gtk_menu_item_new_with_label("Quit");
    g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(ng_activate_menu_item_quit), win);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    gtk_widget_show(item);

    item = gtk_menu_item_new_with_label("Nonogram");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    return menu;
}

NgWindow *ng_window_init(void)
{
    NgWindow *win = g_malloc0(sizeof(NgWindow));
    win->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

    if (win->window == NULL)
        goto err;

    gtk_window_set_default_size(GTK_WINDOW(win->window), 600, 400);
    win->drawing_area = gtk_drawing_area_new();
    win->status_bar = gtk_statusbar_new();
    win->menu_bar = ng_window_create_main_menu(win);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    gtk_box_pack_start(GTK_BOX(vbox), win->menu_bar, FALSE, FALSE, 3);
    gtk_box_pack_start(GTK_BOX(vbox), win->drawing_area, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), win->status_bar, FALSE, FALSE, 0);

    gtk_widget_add_events(GTK_WIDGET(win->drawing_area),
            GDK_BUTTON_PRESS_MASK |
            GDK_BUTTON_RELEASE_MASK |
            GDK_POINTER_MOTION_MASK |
            GDK_KEY_PRESS_MASK |
            GDK_KEY_RELEASE_MASK |
            GDK_SCROLL_MASK);

    gtk_container_add(GTK_CONTAINER(win->window), vbox);
    gtk_widget_show_all(vbox);

    g_signal_connect(G_OBJECT(win->drawing_area), "draw", G_CALLBACK(ng_window_event_draw), win);
    g_signal_connect(G_OBJECT(win->drawing_area), "button-press-event", G_CALLBACK(ng_window_button_press_event), win);
    g_signal_connect(G_OBJECT(win->drawing_area), "motion-notify-event", G_CALLBACK(ng_window_motion_notify_event), win);
    g_signal_connect(G_OBJECT(win->drawing_area), "button-release-event", G_CALLBACK(ng_window_button_release_event), win);
    g_signal_connect(G_OBJECT(win->drawing_area), "configure-event", G_CALLBACK(ng_window_configure_event), win);
    g_signal_connect(G_OBJECT(win->drawing_area), "scroll-event", G_CALLBACK(ng_window_scroll_event), win);
    g_signal_connect(G_OBJECT(win->window), "key-release-event", G_CALLBACK(ng_window_key_release_event), win);

    gtk_window_present(GTK_WINDOW(win->window));

    return win;

err:
    g_free(win);
    return NULL;
}

void ng_window_destroy(NgWindow *win)
{
}

void ng_window_update(NgWindow *win)
{
    if (win && win->window) {
        gtk_widget_queue_draw(win->window);
    }
}

void ng_window_set_view(NgWindow *win, NgView *view)
{
    if (win) {
        win->view = view;

        if (view && win->drawing_area) {
            guint width, height;

            width = gtk_widget_get_allocated_width(win->drawing_area);
            height = gtk_widget_get_allocated_height(win->drawing_area);

            ng_view_set_size(view, width, height);
        }
    }

    ng_window_update(win);
}
