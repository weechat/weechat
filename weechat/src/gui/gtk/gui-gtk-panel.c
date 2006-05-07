/*
 * Copyright (c) 2003-2006 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/* gui-gtk-panel.c: panel functions for Gtk GUI */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>

#include "../../common/weechat.h"
#include "../gui.h"
#include "gui-gtk.h"


/*
 * gui_panel_window_new: create a new "window panel" for a panel, in screen or a window
 *                       if window is not NULL, panel window will be in this window
 */

int
gui_panel_window_new (t_gui_panel *panel, t_gui_window *window)
{
    /* TODO: write this function for Gtk */
    (void) panel;
    (void) window;
    
    return 1;
}

/*
 * gui_panel_window_free: delete a panel window
 */

void
gui_panel_window_free (void *panel_win)
{
    t_gui_panel_window *ptr_panel_win;

    ptr_panel_win = (t_gui_panel_window *)panel_win;
    
    free (ptr_panel_win);
}
