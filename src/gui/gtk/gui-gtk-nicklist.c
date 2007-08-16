/*
 * Copyright (c) 2003-2007 by FlashCode <flashcode@flashtux.org>
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

/* gui-gtk-nicklist.c: nicklist display functions for Gtk GUI */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "../../common/weechat.h"
#include "../gui.h"
#include "../../common/weeconfig.h"
#include "../../irc/irc.h"
#include "gui-gtk.h"


/*
 * gui_nicklist_draw: draw nick window for a buffer
 */

void
gui_nicklist_draw (t_gui_buffer *buffer, int erase, int calculate_size)
{
    /*t_gui_window *ptr_win;
    int i, j, x, y, column, max_length, nicks_displayed;
    char format[32], format_empty[32];
    t_irc_nick *ptr_nick;*/
    
    if (!gui_ok || !GUI_BUFFER_HAS_NICKLIST(buffer))
        return;
    
    /* TODO: write this function for Gtk */
    (void) buffer;
    (void) erase;
    (void) calculate_size;
}
