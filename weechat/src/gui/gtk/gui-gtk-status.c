/*
 * Copyright (c) 2003-2007 by FlashCode <flashcode@flashtux.org>
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

/* gui-gtk-status.c: status display functions for Gtk GUI */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "../../common/weechat.h"
#include "../gui.h"
#include "../../common/hotlist.h"
#include "../../common/weeconfig.h"
#include "gui-gtk.h"


/*
 * gui_status_draw: draw status window for a buffer
 */

void
gui_status_draw (t_gui_buffer *buffer, int erase)
{
    /*t_gui_window *ptr_win;
    t_weechat_hotlist *ptr_hotlist;
    char format[32], str_nicks[32], *more;
    int i, first_mode, x, server_pos, server_total;
    int display_name, names_count;*/
    
    /* make C compiler happy */
    (void) buffer;
    
    if (!gui_ok)
        return;
    
    /* TODO: write this function for Gtk */
    (void) buffer;
    (void) erase;
}
