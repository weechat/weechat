/*
 * Copyright (c) 2004 by FlashCode <flashcode@flashtux.org>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* history.c: memorize and call again commands or text */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "weechat.h"
#include "history.h"
#include "weeconfig.h"
#include "../gui/gui.h"


t_history *history_general = NULL;
t_history *history_general_last = NULL;
t_history *history_general_ptr = NULL;
int num_history_general = 0;


/*
 * history_add: add a text/command to history
 */

void
history_add (void *buffer, char *string)
{
    t_history *new_history, *ptr_history;
    
    /* add history to general history */
    new_history = (t_history *)malloc (sizeof (t_history));
    if (new_history)
    {
        new_history->text = strdup (string);
        
        if (history_general)
            history_general->prev_history = new_history;
        else
            history_general_last = new_history;
        new_history->next_history = history_general;
        new_history->prev_history = NULL;
        history_general = new_history;
        num_history_general++;
        
        /* remove one command if necessary */
        if ((cfg_history_max_commands > 0)
            && (num_history_general > cfg_history_max_commands))
        {
            ptr_history = history_general_last->prev_history;
            history_general_last->prev_history->next_history = NULL;
            if (history_general_last->text)
                free (history_general_last->text);
            free (history_general_last);
            history_general_last = ptr_history;
            num_history_general--;
        }
    }
    
    /* add history to local history */
    new_history = (t_history *)malloc (sizeof (t_history));
    if (new_history)
    {
        new_history->text = strdup (string);
        
        if (((t_gui_buffer *)(buffer))->history)
            ((t_gui_buffer *)(buffer))->history->prev_history = new_history;
        else
            ((t_gui_buffer *)(buffer))->last_history = new_history;
        new_history->next_history = ((t_gui_buffer *)(buffer))->history;
        new_history->prev_history = NULL;
        ((t_gui_buffer *)buffer)->history = new_history;
        ((t_gui_buffer *)(buffer))->num_history++;
        
        /* remove one command if necessary */
        if ((cfg_history_max_commands > 0)
            && (((t_gui_buffer *)(buffer))->num_history > cfg_history_max_commands))
        {
            ptr_history = ((t_gui_buffer *)buffer)->last_history->prev_history;
            ((t_gui_buffer *)buffer)->last_history->prev_history->next_history = NULL;
            if (((t_gui_buffer *)buffer)->last_history->text)
                free (((t_gui_buffer *)buffer)->last_history->text);
            free (((t_gui_buffer *)buffer)->last_history);
            ((t_gui_buffer *)buffer)->last_history = ptr_history;
            ((t_gui_buffer *)(buffer))->num_history++;
        }
    }
}
