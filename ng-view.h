#pragma once

#include <glib.h>
#include <cairo.h>
#include "ng-data.h"

typedef struct _NgView NgView;

NgView *ng_view_new(Nonogram *ng);
void ng_view_free(NgView *view);
Nonogram *ng_view_get_data(NgView *view);

void ng_view_update_map(NgView *view, guint x, guint y, guint cx, guint cy);

void ng_view_render(NgView *view, cairo_t *cr, guint width, guint height);

typedef enum {
    NG_VIEW_COORDINATE_FIELD,
    NG_VIEW_COORDINATE_ROW_HINT,
    NG_VIEW_COORDINATE_COLUMN_HINT,
    NG_VIEW_COORDINATE_UNKNOWN
} NgViewCoordinateType;

NgViewCoordinateType ng_view_translate_coordinate(NgView *view, gint vx, gint vy, guint *ox, guint *oy);
