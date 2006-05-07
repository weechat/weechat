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

/* gui-panel.c: panel functions, used by all GUI */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <ctype.h>

#include "../common/weechat.h"
#include "gui.h"


t_gui_panel *gui_panels = NULL;             /* pointer to first panel       */
t_gui_panel *last_gui_panel = NULL;         /* pointer to last panel        */


/*
 * gui_panel_new: create a new panel
 */

t_gui_panel *
gui_panel_new (char *name, int position, int type, int size, int separator)
{
    t_gui_panel *new_panel;
    t_gui_window *ptr_win;

    if (!name || !name[0])
        return NULL;
    
    if ((new_panel = (t_gui_panel *) malloc (sizeof (t_gui_panel))))
    {
        new_panel->position = position;
        new_panel->name = strdup (name);
        new_panel->separator = separator;
        new_panel->size = size;
        if (type == GUI_PANEL_WINDOWS)
        {
            /* create panel window for all opened windows */
            for (ptr_win = gui_windows; ptr_win;
                 ptr_win = ptr_win->next_window)
                gui_panel_window_new (new_panel, ptr_win);
        }
        else
            /* create only one panel window (global) */
            gui_panel_window_new (new_panel, NULL);
        
        /* add panel to panels queue */
        new_panel->prev_panel = last_gui_panel;
        if (gui_panels)
            last_gui_panel->next_panel = new_panel;
        else
            gui_panels = new_panel;
        last_gui_panel = new_panel;
        new_panel->next_panel = NULL;
        
        return new_panel;
    }
    else
        return NULL;
}

/*
 * gui_panel_free: delete a panel
 */

void
gui_panel_free (t_gui_panel *panel)
{
    /* remove panel from panels list */
    if (panel->prev_panel)
        panel->prev_panel->next_panel = panel->next_panel;
    if (panel->next_panel)
        panel->next_panel->prev_panel = panel->prev_panel;
    if (gui_panels == panel)
        gui_panels = panel->next_panel;
    if (last_gui_panel == panel)
        last_gui_panel = panel->prev_panel;

    /* free data */
    if (panel->name)
        free (panel->name);
    if (panel->panel_window)
        gui_panel_window_free (panel->panel_window);
    
    free (panel);
}
