/*
 * Copyright (c) 2003 by FlashCode <flashcode@flashtux.org>
 *                       Bounga <bounga@altern.org>
 *                       Xahlexx <xahlexx@tuxisland.org>
 * See README for License detail.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


/* history.c: memorize and call again commands or text */


#include <stdlib.h>
#include <string.h>

#include "weechat.h"
#include "history.h"
#include "gui/gui.h"


t_history *history_general = NULL;
t_history *history_general_ptr = NULL;


/*
 * history_add: add a text/command to history
 */

void
history_add (void *window, char *string)
{
    t_history *new_history;
    
    new_history = (t_history *)malloc (sizeof (t_history));
    if (new_history)
    {
        new_history->text = strdup (string);
        
        /* add history to general history */
        if (history_general)
            history_general->prev_history = new_history;
        new_history->next_history = history_general;
        new_history->prev_history = NULL;
        history_general = new_history;
        
        /* add history to local history */
        if (((t_gui_window *)(window))->history)
            ((t_gui_window *)(window))->history->prev_history = new_history;
        new_history->next_history = ((t_gui_window *)(window))->history;
        new_history->prev_history = NULL;
        ((t_gui_window *)window)->history = new_history;
    }
}
