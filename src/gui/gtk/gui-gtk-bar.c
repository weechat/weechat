/*
 * Copyright (c) 2003-2008 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* gui-gtk-bar.c: bar functions for Gtk GUI */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>

#include "../../core/weechat.h"
#include "../../core/wee-log.h"
#include "../gui-bar.h"
#include "../gui-chat.h"
#include "../gui-color.h"
#include "../gui-window.h"
#include "gui-gtk.h"


/*
 * gui_bar_windows_get_size: get total bar size (window bars) for a position
 *                           bar is optional, if not NULL, size is computed
 *                           from bar 1 to bar # - 1
 */

int
gui_bar_window_get_size (struct t_gui_bar *bar, struct t_gui_window *window,
                         int position)
{
    (void) bar;
    (void) window;
    (void) position;
    
    /* TODO: write this function for Gtk */
    return 0;
}

/*
 * gui_bar_window_new: create a new "window bar" for a bar, in screen or a window
 *                     if window is not NULL, bar window will be in this window
 */

int
gui_bar_window_new (struct t_gui_bar *bar, struct t_gui_window *window)
{
    (void) bar;
    (void) window;
    
    /* TODO: write this function for Gtk */
    return 0;
}

/*
 * gui_bar_draw: draw a bar
 */

void
gui_bar_draw (struct t_gui_bar *bar)
{
    (void) bar;

    /* TODO: write this function for Gtk */
}

/*
 * gui_bar_window_free: delete an bar window
 */

void
gui_bar_window_free (struct t_gui_bar_window *bar_window)
{
    /* TODO: complete this function for Gtk */
    
    /* free bar window */
    free (bar_window);
}

/*
 * gui_bar_window_print_log: print bar window infos in log (usually for crash dump)
 */

void
gui_bar_window_print_log (struct t_gui_bar_window *bar_window)
{
    log_printf ("");
    log_printf ("  [window bar (addr:0x%x)]",   bar_window);
    log_printf ("    bar . . . . . . . : 0x%x", bar_window->bar);
    log_printf ("    x . . . . . . . . : %d",   bar_window->x);
    log_printf ("    y . . . . . . . . : %d",   bar_window->y);
    log_printf ("    width . . . . . . : %d",   bar_window->width);
    log_printf ("    height. . . . . . : %d",   bar_window->height);
}
