#include "ng-view.h"
#include <stdio.h>
#include <math.h>

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

enum VIEWFLAGS {
    FLAG_TMP_LINE = 1 << 0
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
    guint width;
    guint height;

    gint cursor_x;
    gint cursor_y;

    guint visible_width;
    guint visible_height;

    guint32 flags;

    guint tmp_line_start_x;
    guint tmp_line_start_y;
    guint tmp_line_end_x;
    guint tmp_line_end_y;

    /* private */
    guint reference_count;
};

void ng_view_free(NgView *view)
{
    if (!view)
        return;
    guint i;
    for (i = 0; i < COUNT_SURF; ++i) {
        if (view->surface[i])
            cairo_surface_destroy(view->surface[i]);
    }
    if (view->ng)
        ng_data_unref(view->ng);
    g_free(view);
}

void ng_view_ref(NgView *view)
{
    g_return_if_fail(view != NULL);

    ++view->reference_count;
}

void ng_view_unref(NgView *view)
{
    g_return_if_fail(view != NULL);

    if (view->reference_count <= 1)
        ng_view_free(view);
    else
        --view->reference_count;
}

void ng_view_update_visible_area(NgView *view)
{
    if (view == NULL)
        return;
    
    view->visible_width = view->width;
    if (view->surface[SURF_ROWHINTS]) {
        if (view->visible_width >= view->surf_width[SURF_ROWHINTS])
            view->visible_width -= view->surf_width[SURF_ROWHINTS];
        else
            view->visible_width = 0;
    }
    
    view->visible_height = view->height;
    if (view->surface[SURF_COLHINTS]) {
        if (view->visible_height >= view->surf_height[SURF_COLHINTS])
            view->visible_height -= view->surf_height[SURF_COLHINTS];
        else
            view->visible_height = 0;
    }

    if (view->surface[SURF_FIELD]) {
        if (view->visible_width > view->surf_width[SURF_FIELD])
            view->visible_width = view->surf_width[SURF_FIELD];
        if (view->visible_height > view->surf_height[SURF_FIELD])
            view->visible_height = view->surf_height[SURF_FIELD];
    }
}

void ng_view_straighten_line(guint isx, guint isy, guint iex, guint iey,
                             guint *osx, guint *osy, guint *oex, guint *oey)
{
    guint dx = isx > iex ? isx-iex : iex-isx;
    guint dy = isy > iey ? isy-iey : iey-isy;

    if (dy > dx)
        iex = isx;
    else
        iey = isy;
    if (osx) *osx = isx > iex ? iex : isx;
    if (osy) *osy = isy > iey ? iey : isy;
    if (oex) *oex = isx > iex ? isx : iex;
    if (oey) *oey = isy > iey ? isy : iey;
}

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
            snprintf(buf, 8, "%d", NG_HINT_NUM(hints[offset]));
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
    view->reference_count = 1;

    view->max_row_hints = 1;
    view->max_col_hints = 1;

    if (ng) {
        ng_data_ref(ng);
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

        printf("max hints: %d, %d\n", view->max_row_hints, view->max_col_hints);

        view->surf_width[SURF_FIELD] = view->gridsize * ng->width;
        view->surf_height[SURF_FIELD] = view->gridsize * ng->height;
        view->surf_width[SURF_ROWHINTS] = view->gridsize * view->max_row_hints;
        view->surf_height[SURF_ROWHINTS] = view->gridsize * ng->height;
        view->surf_width[SURF_COLHINTS] = view->gridsize * ng->width;
        view->surf_height[SURF_COLHINTS] = view->gridsize * view->max_col_hints;

        for (i = 0; i < COUNT_SURF; ++i) {
            printf("surface[%d]: %d x %d\n", i, view->surf_width[i], view->surf_height[i]);
            view->surface[i] = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
                    view->surf_width[i], view->surf_height[i]);
        }

        ng_view_render_grid(view->surface[SURF_FIELD], view->gridsize, ng->width, ng->height, 5, 5, 1.0);
        ng_view_render_grid(view->surface[SURF_ROWHINTS], view->gridsize, view->max_row_hints, ng->height, -1, 5, 0.8);
        ng_view_render_hints(view->surface[SURF_ROWHINTS], view->gridsize, ng->row_hints, ng->row_offsets,
                ng->height, ORIENTATION_HORIZONTAL);
        ng_view_render_grid(view->surface[SURF_COLHINTS], view->gridsize, ng->width, view->max_col_hints, 5, -1, 0.8);
        ng_view_render_hints(view->surface[SURF_COLHINTS], view->gridsize, ng->col_hints, ng->col_offsets,
                ng->width, ORIENTATION_VERTICAL);

        ng_view_update_map(view, 0, 0, view->ng->width, view->ng->height);
        ng_view_update_marks(view, NG_VIEW_COORDINATE_ROW_HINT, 0, 0, view->max_row_hints, ng->height);
        ng_view_update_marks(view, NG_VIEW_COORDINATE_COLUMN_HINT, 0, 0, ng->width, view->max_col_hints);
    }

    return view;
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

    for (j = y; j < y + cy && j < view->ng->height; ++j) {
        for (i = x; i < x + cx && i < view->ng->width; ++i) {
            switch (view->ng->field[j * view->ng->width + i]) {
                case 1:
                    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);
                    break;
                case 0xff:
                    cairo_set_source_rgba(cr, 0.8, 0.0, 0.0, 1.0);
                    break;
                default:
                    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
                    break;
            }
            if (view->ng->field[j * view->ng->width + i] != 0xff) {
                cairo_rectangle(cr, i * view->gridsize + 1.0, j * view->gridsize + 1.0,
                        view->gridsize - 2.0, view->gridsize - 2.0);
            }
            else {
                cairo_save(cr);
                cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
                cairo_rectangle(cr, i * view->gridsize + 1.0, j * view->gridsize + 1.0,
                        view->gridsize - 2.0, view->gridsize - 2.0);
                cairo_fill(cr);
                cairo_restore(cr);
                cairo_arc(cr, (i + 0.5) * view->gridsize, (j + 0.5) * view->gridsize,
                        0.1 * view->gridsize, 0, 2 * M_PI);
            }
            cairo_fill(cr);
        }
    }

    cairo_destroy(cr);
}

void ng_view_update_marks(NgView *view, NgViewCoordinateType type, guint x, guint y, guint cx, guint cy)
{
    if (view == NULL || view->ng == NULL)
        return;
    if (cx == 0 || cy == 0)
        return;
    enum SURFACEID id;
    guint max_x, max_y;
    guint32 *hints;

    if (type == NG_VIEW_COORDINATE_ROW_HINT) {
        if (x >= view->max_row_hints || y >= view->ng->height || view->surface[SURF_ROWHINTS] == NULL)
            return;
        id = SURF_ROWHINTS;
        max_x = view->max_row_hints;
        max_y = view->ng->height;
        hints = view->ng->row_hints;
    }
    else if (type == NG_VIEW_COORDINATE_COLUMN_HINT) {
        if (y >= view->max_col_hints || x >= view->ng->width || view->surface[SURF_COLHINTS] == NULL)
            return;
        id = SURF_COLHINTS;
        max_x = view->ng->width;
        max_y = view->max_col_hints;
        hints = view->ng->col_hints;
    }
    else {
        return;
    }

    guint i, j;
    cairo_t *cr = cairo_create(view->surface[id]);
    guint16 offset;
    gchar buf[8];
    cairo_text_extents_t extents;
    for (j = y; j < max_y && j < y + cy; ++j) {
        for (i = x; i < max_x && i < x + cx; ++i) {
            if (ng_view_translate_hint_position(view, type, i, j, &offset)) {
                cairo_save(cr);
                if (NG_HINT_STATE(hints[offset]) == 0) {
                    cairo_set_source_rgba(cr, 0.8, 0.8, 0.8, 1.0);
                    cairo_rectangle(cr, i * view->gridsize + 1.0, j * view->gridsize + 1.0,
                            view->gridsize - 2.0, view->gridsize - 2.0);
                    cairo_fill(cr);
                    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);
                    snprintf(buf, 8, "%d", NG_HINT_NUM(hints[offset]));
                    cairo_text_extents(cr, buf, &extents);
                    cairo_move_to(cr, (i + 0.5) * view->gridsize - extents.width * 0.5,
                                      (j + 0.5) * view->gridsize + extents.height * 0.5);
                    cairo_show_text(cr, buf);
                }
                else {
                    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);
                    cairo_set_line_width(cr, 1.0);
                    cairo_move_to(cr, i * view->gridsize + 2.0, j * view->gridsize + 2.0);
                    cairo_line_to(cr, (i + 1) * view->gridsize - 2.0, (j + 1) * view->gridsize - 2.0);
                    cairo_stroke(cr);
                    cairo_move_to(cr, i * view->gridsize + 2.0, (j + 1) * view->gridsize - 2.0);
                    cairo_line_to(cr, (i + 1) * view->gridsize - 2.0, j * view->gridsize + 2.0);
                    cairo_stroke(cr);
                }
                cairo_restore(cr);
            }
        }
    }
    cairo_destroy(cr);
}

void ng_view_get_required_size(NgView *view, guint *width, guint *height)
{
    g_return_if_fail(view != NULL);
    g_return_if_fail(view->ng != NULL);

    if (width)
        *width = (view->ng->width + view->max_row_hints) * view->gridsize;
    if (height)
        *height = (view->ng->height + view->max_col_hints) * view->gridsize;
}

void ng_view_set_size(NgView *view, guint width, guint height)
{
    g_return_if_fail(view != NULL);

    view->width = width;
    view->height = height;

    ng_view_update_visible_area(view);
}

void ng_view_set_cursor_pos(NgView *view, gint x, gint y)
{
    g_return_if_fail(view != NULL);

    view->cursor_x = x;
    view->cursor_y = y;
}

void ng_view_scroll(NgView *view, NgViewScrollDirection direction)
{
    g_return_if_fail(view != NULL);

    switch (direction) {
        case NG_VIEW_SCROLL_LEFT:
            if (view->offsetx > view->gridsize)
                view->offsetx -= view->gridsize;
            else
                view->offsetx = 0;
            break;
        case NG_VIEW_SCROLL_RIGHT:
            if (view->offsetx + view->visible_width + view->gridsize >
                    view->surf_width[SURF_FIELD]) {
                view->offsetx = view->visible_width < view->surf_width[SURF_FIELD] ?
                    view->surf_width[SURF_FIELD] - view->visible_width : 0;
            }
            else {
                view->offsetx += view->gridsize;
            }
            break;
        case NG_VIEW_SCROLL_UP:
            if (view->offsety > view->gridsize)
                view->offsety -= view->gridsize;
            else
                view->offsety = 0;
            break;
        case NG_VIEW_SCROLL_DOWN:
            if (view->offsety + view->visible_height + view->gridsize >
                    view->surf_height[SURF_FIELD]) {
                view->offsety = view->visible_height < view->surf_height[SURF_FIELD] ?
                    view->surf_height[SURF_FIELD] - view->visible_height : 0;
            }
            else {
                view->offsety += view->gridsize;
            }
            break;
    }
}

void ng_view_render(NgView *view, cairo_t *cr, gboolean render_overlays)
{
    g_return_if_fail(view != NULL);

    cairo_rectangle(cr, 0, 0, view->width, view->height);
    cairo_set_source_rgba(cr, 0.5, 0.5, 0.5, 1.0);
    cairo_fill(cr);

    /* todo: allow scrolling */
    if (view->surface[SURF_FIELD]) {
        cairo_save(cr);
        cairo_rectangle(cr, 0, 0, view->visible_width, view->visible_height);
        cairo_clip(cr);
        cairo_set_source_surface(cr, view->surface[SURF_FIELD], -view->offsetx, -view->offsety);
        cairo_rectangle(cr, 0, 0, view->visible_width, view->visible_height);
        cairo_fill(cr);
        cairo_restore(cr);
    }

    if (view->surface[SURF_ROWHINTS]) {
        cairo_save(cr);
        cairo_translate(cr, view->visible_width, 0);
        cairo_set_source_surface(cr, view->surface[SURF_ROWHINTS], 0, -view->offsety);
        cairo_rectangle(cr, 0, 0, view->surf_width[SURF_ROWHINTS], view->visible_height);
        cairo_fill(cr);
        cairo_restore(cr);
    }

    if (view->surface[SURF_COLHINTS]) {
        cairo_save(cr);
        cairo_translate(cr, 0, view->visible_height);
        cairo_set_source_surface(cr, view->surface[SURF_COLHINTS], -view->offsetx, 0);
        cairo_rectangle(cr, 0, 0, view->visible_width, view->surf_height[SURF_COLHINTS]);
        cairo_fill(cr);
        cairo_restore(cr);
    }

    if (!render_overlays)
        return;

    if (view->flags & FLAG_TMP_LINE) {
        cairo_save(cr);
        cairo_rectangle(cr, 0, 0, view->visible_width, view->visible_height);
        cairo_clip(cr);
        cairo_translate(cr, -view->offsetx, -view->offsety);
        cairo_set_source_rgba(cr, 0.7, 0.7, 0.7, 0.5);
        guint sx, sy, ex, ey;
        ng_view_straighten_line(view->tmp_line_start_x, view->tmp_line_start_y,
                                view->tmp_line_end_x, view->tmp_line_end_y,
                                &sx, &sy, &ex, &ey);
        guint i;
        guint ox = 1, oy = 1;
        guint len = 1;
        if (sx == ex) {
            ox = 0;
            len = ey-sy+1;
        }
        if (sy == ey) {
            oy = 0;
            len = ex-sx+1;
        }
        if (len > 1) {
            for (i = 0; i < len; ++i) {
                cairo_rectangle(cr, (sx + i * ox) * view->gridsize + 1.0,
                                    (sy + i * oy) * view->gridsize + 1.0,
                                    view->gridsize - 2.0, view->gridsize - 2.0);
                cairo_fill(cr);
            }
        }
        cairo_restore(cr);
    }
    
    guint vx, vy;
    NgViewCoordinateType type = ng_view_translate_coordinate(view, view->cursor_x, view->cursor_y, &vx, &vy);
    if (type == NG_VIEW_COORDINATE_FIELD) {
        if (view->surface[SURF_ROWHINTS]) {
            cairo_save(cr);
            cairo_translate(cr, view->visible_width, 0);
            cairo_set_source_rgba(cr, 1.0, 0.0, 0.0, 1.0);
            cairo_set_line_width(cr, 2.0);
            cairo_rectangle(cr, 0, vy * view->gridsize - view->offsety, view->surf_width[SURF_ROWHINTS], view->gridsize);
            cairo_stroke(cr);
            cairo_restore(cr);
        }
        if (view->surface[SURF_COLHINTS]) {
            cairo_save(cr);
            cairo_translate(cr, 0, view->visible_height);
            cairo_set_source_rgba(cr, 1.0, 0.0, 0.0, 1.0);
            cairo_set_line_width(cr, 2.0);
            cairo_rectangle(cr, vx * view->gridsize - view->offsetx, 0, view->gridsize, view->surf_height[SURF_COLHINTS]);
            cairo_stroke(cr);
            cairo_restore(cr);
        }
    }
}

void ng_view_export_to_file(NgView *view, const gchar *filename)
{
    g_return_if_fail(view != NULL);
    g_return_if_fail(view->ng != NULL);
    g_return_if_fail(filename != NULL);

    NgView *offscreen = ng_view_new(view->ng);

    guint width = 0, height = 0;
    ng_view_get_required_size(offscreen, &width, &height);
    if (width == 0 || height == 0)
        goto done;
    ng_view_set_size(offscreen, width, height);

    cairo_surface_t *surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    cairo_t *cr = cairo_create(surf);

    ng_view_render(offscreen, cr, FALSE);

    cairo_destroy(cr);

    cairo_status_t status = cairo_surface_write_to_png(surf, filename);
    cairo_surface_destroy(surf);

    if (status != CAIRO_STATUS_SUCCESS)
        printf("Error exporting to file `%s'\n", filename);

done:
    ng_view_unref(offscreen);
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

    if (vx < view->visible_width && vy < view->visible_height &&
            tx < view->ng->width && ty < view->ng->height) {
        if (ox) *ox = tx;
        if (oy) *oy = ty;
        return NG_VIEW_COORDINATE_FIELD;
    }

    if (vy >= view->visible_height) {
        tx = (vx + view->offsetx) / view->gridsize;
        ty = (vy - view->visible_height) / view->gridsize;

        if (tx < view->ng->width && ty < view->max_col_hints) {
            if (ox) *ox = tx;
            if (oy) *oy = ty;
            return NG_VIEW_COORDINATE_COLUMN_HINT;
        }
    }

    if (vx >= view->visible_width) {
        tx = (vx - view->visible_width) / view->gridsize;
        ty = (vy + view->offsety) / view->gridsize;

        if (tx < view->max_row_hints && ty < view->ng->height) {
            if (ox) *ox = tx;
            if (oy) *oy = ty;
            return NG_VIEW_COORDINATE_ROW_HINT;
        }
    }

    return NG_VIEW_COORDINATE_UNKNOWN;
}

gboolean ng_view_translate_hint_position(NgView *view, NgViewCoordinateType type, guint hx, guint hy, guint16 *offset)
{
    /* this function is only called when we already have valid coordinates for the displayed
     * grid, i.e, hx, hy do not exceed bounds and view and view->ng and offsets are valid */
    if (type == NG_VIEW_COORDINATE_ROW_HINT) {
        if (hx + view->ng->row_offsets[hy] >= view->ng->row_offsets[hy + 1])
            return FALSE;
        if (offset)
            *offset = view->ng->row_offsets[hy] + hx;
        return TRUE;
    }
    else if (type == NG_VIEW_COORDINATE_COLUMN_HINT) {
        if (hy + view->ng->col_offsets[hx] >= view->ng->col_offsets[hx + 1])
            return FALSE;
        if (offset)
            *offset = view->ng->col_offsets[hx] + hy;
        return TRUE;
    }

    return FALSE;
}

void ng_view_tmp_line_start(NgView *view, guint x, guint y)
{
    if (view == NULL || view->ng == NULL)
        return;
    if (x >= view->ng->width || y >= view->ng->height)
        return;

    view->tmp_line_start_x = view->tmp_line_end_x = x;
    view->tmp_line_start_y = view->tmp_line_end_y = y;
    view->flags |= FLAG_TMP_LINE;
}

void ng_view_tmp_line_end(NgView *view, guint x, guint y)
{
    if (view == NULL || view->ng == NULL)
        return;
    if (!(view->flags & FLAG_TMP_LINE))
        return;
    if (x >= view->ng->width || y >= view->ng->height)
        return;
    view->tmp_line_end_x = x;
    view->tmp_line_end_y = y;
}

void ng_view_tmp_line_clear(NgView *view)
{
    if (view == NULL)
        return;
    view->flags &= ~FLAG_TMP_LINE;
}

gboolean ng_view_tmp_line_finish(NgView *view, guint *sx, guint *sy, guint *ex, guint *ey)
{
    if (view == NULL || view->ng == NULL)
        return FALSE;
    if (!(view->flags & FLAG_TMP_LINE))
        return FALSE;

    ng_view_straighten_line(view->tmp_line_start_x, view->tmp_line_start_y,
                            view->tmp_line_end_x, view->tmp_line_end_y,
                            sx, sy, ex, ey);

    return TRUE;
}
