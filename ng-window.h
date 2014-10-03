/*
 * nonogram-game -- play nonograms
 * Copyright (c) 2014 Holger Langenau (see also: LICENSE)
 *
 */
#pragma once

#include <gtk/gtk.h>
#include "ng-view.h"

typedef struct _NgWindow NgWindow;

NgWindow *ng_window_init(void);
void ng_window_set_view(NgWindow *win, NgView *view);
void ng_window_destroy(NgWindow *win);
void ng_window_update(NgWindow *win);
