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

/* gui-infobar.c: infobar functions, used by all GUI */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "../core/weechat.h"
#include "../core/wee-hook.h"
#include "../core/wee-log.h"
#include "gui-infobar.h"
#include "gui-color.h"
#include "gui-window.h"


struct t_gui_infobar *gui_infobar = NULL;          /* infobar content       */
struct t_hook *gui_infobar_refresh_timer = NULL;   /* refresh timer         */
struct t_hook *gui_infobar_highlight_timer = NULL; /* highlight timer       */


/* 
 * gui_infobar_printf: display message in infobar
 */

void
gui_infobar_printf (int delay, int color, char *message, ...)
{
    static char buf[1024];
    va_list argptr;
    struct t_gui_infobar *ptr_infobar;
    char *buf2, *ptr_buf, *pos;
    
    if (!message)
        return;
    
    va_start (argptr, message);
    vsnprintf (buf, sizeof (buf) - 1, message, argptr);
    va_end (argptr);
    
    ptr_infobar = (struct t_gui_infobar *)malloc (sizeof (struct t_gui_infobar));
    if (ptr_infobar)
    {
        buf2 = (char *)gui_color_decode ((unsigned char *)buf);
        ptr_buf = (buf2) ? buf2 : buf;
        
        ptr_infobar->color = color;
        ptr_infobar->text = strdup (ptr_buf);
        pos = strchr (ptr_infobar->text, '\n');
        if (pos)
            pos[0] = '\0';
        ptr_infobar->remaining_time = (delay <= 0) ? -1 : delay;
        ptr_infobar->next_infobar = gui_infobar;
        gui_infobar = ptr_infobar;
        gui_infobar_draw (gui_current_window->buffer, 1);
        if (buf2)
            free (buf2);
        
        if (!gui_infobar_highlight_timer)
            gui_infobar_highlight_timer = hook_timer (NULL, 1 * 1000, 0, 0,
                                                      &gui_infobar_highlight_timer_cb,
                                                      NULL);
    }
    else
        log_printf (_("Error: not enough memory for infobar message"));
}

/*
 * gui_infobar_remove: remove last displayed message in infobar
 */

void
gui_infobar_remove ()
{
    struct t_gui_infobar *new_infobar;
    
    if (gui_infobar)
    {
        new_infobar = gui_infobar->next_infobar;
        if (gui_infobar->text)
            free (gui_infobar->text);
        free (gui_infobar);
        gui_infobar = new_infobar;
    }
}

/*
 * gui_infobar_remove_all: remove last displayed message in infobar
 */

void
gui_infobar_remove_all ()
{
    while (gui_infobar)
    {
        gui_infobar_remove ();
    }
}
