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

/* history.c: memorize commands or text */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "weechat.h"
#include "history.h"
#include "util.h"
#include "weeconfig.h"
#include "../irc/irc.h"
#include "../gui/gui.h"


t_history *history_global = NULL;
t_history *history_global_last = NULL;
t_history *history_global_ptr = NULL;
int num_history_global = 0;


/*
 * history_buffer_add: add a text/command to buffer's history
 */

void
history_buffer_add (void *buffer, char *string)
{
    t_history *new_history, *ptr_history;

    if (!string)
        return;
    
    if ( !((t_gui_buffer *)(buffer))->history
         || ( ((t_gui_buffer *)(buffer))->history
              && ascii_strcasecmp (((t_gui_buffer *)(buffer))->history->text, string) != 0))
    {	
	new_history = (t_history *)malloc (sizeof (t_history));
	if (new_history)
	{
	    new_history->text = strdup (string);
	    if (cfg_log_hide_nickserv_pwd)
                irc_hide_password (new_history->text, 1);
	    
	    if (((t_gui_buffer *)(buffer))->history)
		((t_gui_buffer *)(buffer))->history->prev_history = new_history;
	    else
		((t_gui_buffer *)(buffer))->last_history = new_history;
	    new_history->next_history = ((t_gui_buffer *)(buffer))->history;
	    new_history->prev_history = NULL;
	    ((t_gui_buffer *)buffer)->history = new_history;
	    ((t_gui_buffer *)buffer)->num_history++;
	    
	    /* remove one command if necessary */
	    if ((cfg_history_max_commands > 0)
		&& (((t_gui_buffer *)(buffer))->num_history > cfg_history_max_commands))
	    {
		ptr_history = ((t_gui_buffer *)buffer)->last_history->prev_history;
                if (((t_gui_buffer *)buffer)->ptr_history == ((t_gui_buffer *)buffer)->last_history)
                    ((t_gui_buffer *)buffer)->ptr_history = ptr_history;
		((t_gui_buffer *)buffer)->last_history->prev_history->next_history = NULL;
		if (((t_gui_buffer *)buffer)->last_history->text)
		    free (((t_gui_buffer *)buffer)->last_history->text);
		free (((t_gui_buffer *)buffer)->last_history);
		((t_gui_buffer *)buffer)->last_history = ptr_history;
		((t_gui_buffer *)(buffer))->num_history++;
	    }
	}
    }
}

/*
 * history_global_add: add a text/command to buffer's history
 */

void
history_global_add (char *string)
{
    t_history *new_history, *ptr_history;

    if (!string)
        return;
    
    if (!history_global
        || (history_global
            && ascii_strcasecmp (history_global->text, string) != 0))
    {    
	new_history = (t_history *)malloc (sizeof (t_history));
	if (new_history)
	{
	    new_history->text = strdup (string);
	    if (cfg_log_hide_nickserv_pwd)
		irc_hide_password (new_history->text, 1);
	    
	    if (history_global)
		history_global->prev_history = new_history;
	    else
		history_global_last = new_history;
	    new_history->next_history = history_global;
	    new_history->prev_history = NULL;
	    history_global = new_history;
	    num_history_global++;
	    
	    /* remove one command if necessary */
	    if ((cfg_history_max_commands > 0)
		&& (num_history_global > cfg_history_max_commands))
	    {
		ptr_history = history_global_last->prev_history;
                if (history_global_ptr == history_global_last)
                    history_global_ptr = ptr_history;
		history_global_last->prev_history->next_history = NULL;
		if (history_global_last->text)
		    free (history_global_last->text);
		free (history_global_last);
		history_global_last = ptr_history;
		num_history_global--;
	    }
	}
    }
}

/*
 * history_global_free: free global history
 */

void
history_global_free ()
{
    t_history *ptr_history;
    
    while (history_global)
    {
        ptr_history = history_global->next_history;
        if (history_global->text)
            free (history_global->text);
        free (history_global);
        history_global = ptr_history;
    }
    history_global = NULL;
    history_global_last = NULL;
    history_global_ptr = NULL;
    num_history_global = 0;
}


/*
 * history_buffer_free: free history for a buffer
 */

void
history_buffer_free (void *buffer)
{
    t_history *ptr_history;
    
    while (((t_gui_buffer *)(buffer))->history)
    {
        ptr_history = ((t_gui_buffer *)(buffer))->history->next_history;
        if (((t_gui_buffer *)(buffer))->history->text)
            free (((t_gui_buffer *)(buffer))->history->text);
        free (((t_gui_buffer *)(buffer))->history);
        ((t_gui_buffer *)(buffer))->history = ptr_history;
    }
    ((t_gui_buffer *)(buffer))->history = NULL;
    ((t_gui_buffer *)(buffer))->last_history = NULL;
    ((t_gui_buffer *)(buffer))->ptr_history = NULL;
    ((t_gui_buffer *)(buffer))->num_history = 0;
}
