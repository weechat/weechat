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

/* gui-gtk-infobar.c: infobar display functions for Gtk GUI */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "../../core/weechat.h"
#include "../../core/wee-config.h"
#include "../../plugins/plugin.h"
#include "../gui-infobar.h"
#include "../gui-window.h"
#include "gui-gtk.h"


/*
 * gui_infobar_draw_time: draw time in infobar window
 */

void
gui_infobar_draw_time (struct t_gui_buffer *buffer)
{
    /*struct t_gui_window *ptr_win;
    time_t time_seconds;
    struct tm *local_time;*/
    
    /* make C compiler happy */
    (void) buffer;
    
    if (!gui_ok)
        return;
    
    /* TODO: write this function for Gtk */
    (void) buffer;
}

/*
 * gui_infobar_draw: draw infobar window for a buffer
 */

void
gui_infobar_draw (struct t_gui_buffer *buffer, int erase)
{
    /*struct t_gui_window *ptr_win;
    time_t time_seconds;
    struct tm *local_time;
    char text_time[1024 + 1];*/
    
    /* make C compiler happy */
    (void) buffer;
    
    if (!gui_ok)
        return;
    
    /* TODO: write this function for Gtk */
    (void) buffer;
    (void) erase;
}

/*
 * gui_infobar_refresh_timer_cb: timer callback for refresh of infobar
 */

int
gui_infobar_refresh_timer_cb (void *data)
{
    /* make C compiler happy */
    (void) data;
    
    return WEECHAT_RC_OK;
}

/*
 * gui_infobar_highlight_timer_cb: timer callback for highlights in infobar
 */

int
gui_infobar_highlight_timer_cb (void *data)
{
    /* make C compiler happy */
    (void) data;
    
    return WEECHAT_RC_OK;
}
