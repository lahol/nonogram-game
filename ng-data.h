#pragma once

#include <glib.h>

typedef struct _NonogramPrivate NonogramPrivate;

typedef struct {
    guint16 width;
    guint16 height;
    guint32 *row_hints; /* lower 16 bit: hint, high 16 bit: state data (marks) */
    guint32 *col_hints;
    guint16 *row_offsets; /* height +1, offset indices of hints in row */
    guint16 *col_offsets;
    guchar *field;
    NonogramPrivate *priv;
} Nonogram;

#define NG_HINT_NUM(x) ((x) & 0xffff)
#define NG_HINT_STATE(x) (((x) >> 16) & 0xffff)
#define NG_HINT_SET_STATE(x,s) (x) = ((x) & 0xffff) | (((s) & 0xffff) << 16)

/* fills an area starting from point (x,y) cx units wide, cy units high
 * with value (for monochrom: 1: fill, 0: clear, 2: “lock” later more colors) */
void ng_fill_area(Nonogram *ng, guint16 x, guint16 y, guint16 cx, guint16 cy, guchar value);

Nonogram *ng_read_data_from_file(gchar *filename);

void ng_write_data_to_file(Nonogram *ng, gchar *filename);

void ng_data_ref(Nonogram *ng);
void ng_data_unref(Nonogram *ng);
