/*
 * Copyright (C) 2003-2012 Sebastien Helleu <flashcode@flashtux.org>
 *
 * This file is part of WeeChat, the extensible chat client.
 *
 * WeeChat is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * WeeChat is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * gui-gtk-bar-window.c: bar window functions for Gtk GUI
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>

#include "../../core/weechat.h"
#include "../../core/wee-config.h"
#include "../../core/wee-log.h"
#include "../gui-bar.h"
#include "../gui-bar-window.h"
#include "../gui-color.h"
#include "../gui-window.h"
#include "gui-gtk.h"


/*
 * Initializes Gtk windows for bar window.
 */

int
gui_bar_window_objects_init (struct t_gui_bar_window *bar_window)
{
    struct t_gui_bar_window_gtk_objects *new_objects;

    new_objects = malloc (sizeof (*new_objects));
    if (new_objects)
    {
        bar_window->gui_objects = new_objects;
        /* TODO: init Gtk windows */
        return 1;
    }
    return 0;
}

/*
 * Frees Gtk windows for a bar window.
 */

void
gui_bar_window_objects_free (struct t_gui_bar_window *bar_window)
{
    /* TODO: free Gtk windows */
    (void) bar_window;
}

/*
 * Creates curses window for bar.
 */

void
gui_bar_window_create_win (struct t_gui_bar_window *bar_window)
{
    (void) bar_window;

    /* TODO: write this function for Gtk */
}

/*
 * Prints a string text on a bar window.
 *
 * Returns number of chars displayed on screen.
 */

int
gui_bar_window_print_string (struct t_gui_bar_window *bar_window,
                             const char *string, int max_chars)
{
    (void) bar_window;
    (void) string;
    (void) max_chars;

    /* TODO: write this function for Gtk */
    return 0;
}

/*
 * Draws a bar for a window.
 */

void
gui_bar_window_draw (struct t_gui_bar_window *bar_window,
                     struct t_gui_window *window)
{
    (void) bar_window;
    (void) window;

    /* TODO: write this function for Gtk */
}

/*
 * Prints bar window infos in WeeChat log file (usually for crash dump).
 */

void
gui_bar_window_objects_print_log (struct t_gui_bar_window *bar_window)
{
    log_printf ("    bar window specific objects for Gtk:");
    /* TODO: add specific objects */
    (void) bar_window;
}
