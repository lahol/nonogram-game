#include "ng-view.h"
#include <stdio.h>

enum SURFACEID {
    SURF_FIELD = 0,
    SURF_ROWHINTS,
    SURF_COLHINTS,
    COUNT_SURF
};

enum ORIENTATION {
    ORIENTATION_HORIZONTAL,
    ORIENTATION_VERTICAL
};

struct _NgView {
    Nonogram *ng;
    guint gridsize;
    guint16 max_row_hints;
    guint16 max_col_hints;

    cairo_surface_t *surface[COUNT_SURF];
    guint surf_width[COUNT_SURF];
    guint surf_height[COUNT_SURF];

    gint offsetx;
    gint offsety;
};

void ng_view_render_grid(cairo_surface_t *surf, guint gridsize, guint cx, guint cy,
        gint highlight_x, gint highlight_y, gdouble shade)
{
    cairo_t *cr = cairo_create(surf);

    cairo_set_source_rgba(cr, shade, shade, shade, 1.0);
    cairo_rectangle(cr, 0, 0, gridsize * cx, gridsize * cy);
    cairo_fill(cr);

    guint i;
    for (i = 0; i < cx; ++i) {
        if (highlight_x > 0 && i % highlight_x == 0) {
            cairo_set_source_rgba(cr, 0.1, 0.1, 0.1, 1.0);
            cairo_set_line_width(cr, 0.6);
        }
        else {
            cairo_set_source_rgba(cr, 0.2, 0.2, 0.2, 1.0);
            cairo_set_line_width(cr, 0.2);
        }
        cairo_move_to(cr, i * gridsize, 0);
        cairo_line_to(cr, i * gridsize, cy * gridsize);
        cairo_stroke(cr);
    }

    for (i = 0; i < cy; ++i) {
        if (highlight_y > 0 && i % highlight_y == 0) {
            cairo_set_source_rgba(cr, 0.1, 0.1, 0.1, 1.0);
            cairo_set_line_width(cr, 0.6);
        }
        else {
            cairo_set_source_rgba(cr, 0.2, 0.2, 0.2, 1.0);
            cairo_set_line_width(cr, 0.2);
        }
        cairo_move_to(cr, 0, i * gridsize);
        cairo_line_to(cr, cx * gridsize, i * gridsize);
        cairo_stroke(cr);
    }

    cairo_destroy(cr);
}

void ng_view_render_hints(cairo_surface_t *surf, guint gridsize, guint32 *hints, guint16 *offsets,
                          guint16 count, enum ORIENTATION orientation)
{
    cairo_t *cr = cairo_create(surf);
    guint i, offset = 0, col=0, row=1;
    gchar buf[8];
    cairo_text_extents_t extents;

    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);
    cairo_set_font_size(cr, 10);
    for (i = 0; i < count; ++i) {
        while (offset < offsets[i+1]) {
            snprintf(buf, 8, "%d", hints[offset]);
            cairo_text_extents(cr, buf, &extents);
            cairo_move_to(cr, (col + 0.5) * gridsize - extents.width * 0.5,
                              (row - 0.5) * gridsize + extents.height * 0.5);
            cairo_show_text(cr, buf);
            if (orientation == ORIENTATION_HORIZONTAL)
                ++col;
            else
                ++row;
            ++offset;
        }
        if (orientation == ORIENTATION_HORIZONTAL) {
            col = 0;
            ++row;
        }
        else {
            row = 1;
            ++col;
        }
    }

    cairo_destroy(cr);
}

NgView *ng_view_new(Nonogram *ng)
{
    NgView *view = g_malloc0(sizeof(NgView));
    view->ng = ng;
    view->gridsize = 16;

    view->max_row_hints = 1;
    view->max_col_hints = 1;

    if (ng) {
        guint i;
        guint16 num;
        for (i = 0; i < ng->height; ++i) {
            num = ng->row_offsets[i+1] - ng->row_offsets[i];
            if (num > view->max_row_hints)
                view->max_row_hints = num;
        }
        for (i = 0; i < ng->width; ++i) {
            num = ng->col_offsets[i+1] - ng->col_offsets[i];
            if (num > view->max_col_hints)
                view->max_col_hints = num;
        }

        view->surf_width[SURF_FIELD] = view->gridsize * ng->width;
        view->surf_height[SURF_FIELD] = view->gridsize * ng->height;
        view->surf_width[SURF_ROWHINTS] = view->gridsize * view->max_row_hints;
        view->surf_height[SURF_ROWHINTS] = view->gridsize * ng->height;
        view->surf_width[SURF_COLHINTS] = view->gridsize * ng->width;
        view->surf_height[SURF_COLHINTS] = view->gridsize * view->max_col_hints;

        for (i = 0; i < COUNT_SURF; ++i)
            view->surface[i] = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
                    view->surf_width[i], view->surf_height[i]);

        ng_view_render_grid(view->surface[SURF_FIELD], view->gridsize, ng->width, ng->height, 5, 5, 1.0);
        ng_view_render_grid(view->surface[SURF_ROWHINTS], view->gridsize, view->max_row_hints, ng->height, -1, 5, 0.8);
        ng_view_render_hints(view->surface[SURF_ROWHINTS], view->gridsize, ng->row_hints, ng->row_offsets,
                ng->height, ORIENTATION_HORIZONTAL);
        ng_view_render_grid(view->surface[SURF_COLHINTS], view->gridsize, ng->width, view->max_col_hints, 5, -1, 0.8);
        ng_view_render_hints(view->surface[SURF_COLHINTS], view->gridsize, ng->col_hints, ng->col_offsets,
                ng->width, ORIENTATION_VERTICAL);
    }
    return view;
}

void ng_view_free(NgView *view)
{
    if (!view)
        return;
    guint i;
    for (i = 0; i < COUNT_SURF; ++i) {
        if (view->surface[i])
            cairo_surface_destroy(view->surface[i]);
    }
    g_free(view);
}

Nonogram *ng_view_get_data(NgView *view)
{
    if (view)
        return view->ng;
    return NULL;
}

void ng_view_update_map(NgView *view, guint x, guint y, guint cx, guint cy)
{
    if (view == NULL || view->ng == NULL)
        return;
    if (cx == 0 || cy == 0 || x >= view->ng->width || y >= view->ng->height)
        return;
    if (view->surface[SURF_FIELD] == NULL)
        return;
    guint i, j;

    cairo_t *cr = cairo_create(view->surface[SURF_FIELD]);

    for (j = y; j < y + cy && j < view->ng->width; ++j) {
        for (i = x; i < x + cx && i < view->ng->height; ++i) {
            switch (view->ng->field[j * view->ng->width + i]) {
                case 1:
                    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);
                    break;
                case 2:
                    cairo_set_source_rgba(cr, 0.8, 0.0, 0.0, 1.0);
                    break;
                default:
                    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
                    break;
            }
            cairo_rectangle(cr, i * view->gridsize + 1.0, j * view->gridsize + 1.0,
                    view->gridsize - 2.0, view->gridsize - 2.0);
            cairo_fill(cr);
        }
    }

    cairo_destroy(cr);
}

/* todo: set height, width on configure -> needed for translate coordinates */
void ng_view_render(NgView *view, cairo_t *cr, guint width, guint height)
{
    g_return_if_fail(view != NULL);

    cairo_rectangle(cr, 0, 0, width, height);
    cairo_set_source_rgba(cr, 0.5, 0.5, 0.5, 1.0);
    cairo_fill(cr);

    /* todo: allow scrolling */
    if (view->surface[SURF_FIELD]) {
        cairo_identity_matrix(cr);
        cairo_translate(cr, 0, 0);
        cairo_set_source_surface(cr, view->surface[SURF_FIELD], 0, 0);
        cairo_rectangle(cr, 0, 0, view->surf_width[SURF_FIELD], view->surf_height[SURF_FIELD]);
        cairo_fill(cr);
    }

    if (view->surface[SURF_ROWHINTS]) {
        cairo_identity_matrix(cr);
        cairo_translate(cr, view->surf_width[SURF_FIELD], 0);
        cairo_set_source_surface(cr, view->surface[SURF_ROWHINTS], 0, 0);
        cairo_rectangle(cr, 0, 0, view->surf_width[SURF_ROWHINTS], view->surf_height[SURF_ROWHINTS]);
        cairo_fill(cr);
    }

    if (view->surface[SURF_COLHINTS]) {
        cairo_identity_matrix(cr);
        cairo_translate(cr, 0, view->surf_height[SURF_FIELD]);
        cairo_set_source_surface(cr, view->surface[SURF_COLHINTS], 0, 0);
        cairo_rectangle(cr, 0, 0, view->surf_width[SURF_COLHINTS], view->surf_height[SURF_COLHINTS]);
        cairo_fill(cr);
    }
}

NgViewCoordinateType ng_view_translate_coordinate(NgView *view, gint vx, gint vy, guint *ox, guint *oy)
{
    if (view == NULL || view->ng == NULL)
        return NG_VIEW_COORDINATE_UNKNOWN;

    if (vx + view->offsetx < 0 ||
            vy + view->offsety < 0)
        return NG_VIEW_COORDINATE_UNKNOWN;

    guint tx = (vx + view->offsetx) / view->gridsize;
    guint ty = (vy + view->offsety) / view->gridsize;

    if (tx < view->ng->width && ty < view->ng->height) {
        if (ox) *ox = tx;
        if (oy) *oy = ty;
        return NG_VIEW_COORDINATE_FIELD;
    }

    if (tx < view->ng->width && ty - view->ng->height < view->max_col_hints) {
        if (ox) *ox = tx;
        if (oy) *oy = ty - view->ng->height;
        return NG_VIEW_COORDINATE_COLUMN_HINT;
    }

    if (tx - view->ng->width < view->max_row_hints && ty < view->ng->height) {
        if (ox) *ox = tx - view->ng->width;
        if (oy) *oy = ty;
        return NG_VIEW_COORDINATE_ROW_HINT;
    }

    return NG_VIEW_COORDINATE_UNKNOWN;
}

