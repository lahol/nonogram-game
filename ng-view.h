#pragma once

#include <glib.h>
#include <cairo.h>
#include "ng-data.h"

typedef struct _NgView NgView;

NgView *ng_view_new(Nonogram *ng);
void ng_view_ref(NgView *view);
void ng_view_unref(NgView *view);
Nonogram *ng_view_get_data(NgView *view);

void ng_view_update_map(NgView *view, guint x, guint y, guint cx, guint cy);

void ng_view_render(NgView *view, cairo_t *cr, gboolean render_overlays);
void ng_view_export_to_file(NgView *view, const gchar *filename);

void ng_view_get_required_size(NgView *view, guint *width, guint *height);
void ng_view_set_size(NgView *view, guint width, guint height);
void ng_view_set_cursor_pos(NgView *view, gint x, gint y);

typedef enum {
    NG_VIEW_SCROLL_LEFT,
    NG_VIEW_SCROLL_RIGHT,
    NG_VIEW_SCROLL_UP,
    NG_VIEW_SCROLL_DOWN
} NgViewScrollDirection;

void ng_view_scroll(NgView *view, NgViewScrollDirection direction);

typedef enum {
    NG_VIEW_COORDINATE_FIELD,
    NG_VIEW_COORDINATE_ROW_HINT,
    NG_VIEW_COORDINATE_COLUMN_HINT,
    NG_VIEW_COORDINATE_UNKNOWN
} NgViewCoordinateType;

NgViewCoordinateType ng_view_translate_coordinate(NgView *view, gint vx, gint vy, guint *ox, guint *oy);
gboolean ng_view_translate_hint_position(NgView *view, NgViewCoordinateType type, guint hx, guint hy, guint16 *offset);
void ng_view_update_marks(NgView *view, NgViewCoordinateType type, guint x, guint y, guint cx, guint cy);

void ng_view_tmp_line_start(NgView *view, guint x, guint y);
void ng_view_tmp_line_end(NgView *view, guint x, guint y);
void ng_view_tmp_line_clear(NgView *view);
gboolean ng_view_tmp_line_finish(NgView *view, guint *sx, guint *sy, guint *ex, guint *ey);
