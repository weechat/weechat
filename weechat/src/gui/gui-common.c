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


/* gui-common.c: display functions, used by any GUI */


#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <curses.h>

#include "../weechat.h"
#include "gui.h"
#include "../irc/irc.h"


int gui_ready;                              /* = 1 if GUI is initialized    */

t_gui_window *gui_windows = NULL;           /* pointer to first window      */
t_gui_window *last_gui_window = NULL;       /* pointer to last window       */
t_gui_window *gui_current_window = NULL;    /* pointer to current window    */


/*
 * gui_window_clear: clear window content
 */

void
gui_window_clear (t_gui_window *window)
{
    t_gui_line *ptr_line;
    t_gui_message *ptr_message;
    
    while (window->lines)
    {
        ptr_line = window->lines->next_line;
        while (window->lines->messages)
        {
            ptr_message = window->lines->messages->next_message;
            if (window->lines->messages->message)
                free (window->lines->messages->message);
            free (window->lines->messages);
            window->lines->messages = ptr_message;
        }
        free (window->lines);
        window->lines = ptr_line;
    }
    
    window->lines = NULL;
    window->last_line = NULL;
    window->first_line_displayed = 1;
    window->sub_lines = 0;
    window->line_complete = 1;
    window->unread_data = 0;
    
    if (window == gui_current_window)
        gui_redraw_window_chat (window);
}

/*
 * gui_window_clear_all: clear all windows content
 */

void
gui_window_clear_all ()
{
    t_gui_window *ptr_win;
    
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
        gui_window_clear (ptr_win);
}

/*
 * gui_window_new: create a new window
 *                 (TODO: add coordinates and size, for splited windows)
 */

t_gui_window *
gui_window_new (void *server, void *channel
                /*int x, int y, int width, int height*/)
{
    t_gui_window *new_window;
    
    if (gui_windows)
    {
        /* use first window if no server was assigned to this window */
        if (!SERVER(gui_windows))
        {
            if (server)
                ((t_irc_server *)(server))->window = gui_windows;
            if (channel)
                ((t_irc_channel *)(channel))->window = gui_windows;
            SERVER(gui_windows) = server;
            CHANNEL(gui_windows) = channel;
            return gui_windows;
        }
    }
    
    if ((new_window = (t_gui_window *)(malloc (sizeof (t_gui_window)))))
    {
        /* assign server and channel to window */
        SERVER(new_window) = server;
        CHANNEL(new_window) = channel;
        /* assign window to server and channel */
        if (server && !channel)
            SERVER(new_window)->window = new_window;
        if (channel)
            CHANNEL(new_window)->window = new_window;
        
        gui_calculate_pos_size (new_window);
        
        /* init windows */
        new_window->win_title = NULL;
        new_window->win_chat = NULL;
        new_window->win_nick = NULL;
        new_window->win_status = NULL;
        new_window->win_input = NULL;
        
        /* init lines */
        new_window->lines = NULL;
        new_window->last_line = NULL;
        new_window->first_line_displayed = 1;
        new_window->sub_lines = 0;
        new_window->line_complete = 1;
        new_window->unread_data = 0;
        
        /* init input buffer */
        new_window->input_buffer_alloc = INPUT_BUFFER_BLOCK_SIZE;
        new_window->input_buffer = (char *) malloc (INPUT_BUFFER_BLOCK_SIZE);
        new_window->input_buffer[0] = '\0';
        new_window->input_buffer_size = 0;
        new_window->input_buffer_pos = 0;
        new_window->input_buffer_1st_display = 0;
        
        /* init completion */
        completion_init (&(new_window->completion));
        
        /* init history */
        new_window->history = NULL;
        new_window->ptr_history = NULL;
        
        /* switch to new window */
        gui_switch_to_window (new_window);
        
        /* add window to windows queue */
        new_window->prev_window = last_gui_window;
        if (gui_windows)
            last_gui_window->next_window = new_window;
        else
            gui_windows = new_window;
        last_gui_window = new_window;
        new_window->next_window = NULL;
        
        /* redraw whole screen */
        gui_redraw_window (new_window);
    }
    else
        return NULL;
    
    return new_window;
}

/*
 * gui_window_free: delete a window
 */

void
gui_window_free (t_gui_window *window)
{
    t_gui_line *ptr_line;
    t_gui_message *ptr_message;
    int create_new;
    
    create_new = (window->server || window->channel);
    
    /* TODO: manage splitted windows! */
    if ((window == gui_current_window) &&
        ((window->next_window) || (window->prev_window)))
        gui_switch_to_previous_window ();
    
    /* free lines and messages */
    while (window->lines)
    {
        ptr_line = window->lines->next_line;
        while (window->lines->messages)
        {
            ptr_message = window->lines->messages->next_message;
            if (window->lines->messages->message)
                free (window->lines->messages->message);
            free (window->lines->messages);
            window->lines->messages = ptr_message;
        }
        free (window->lines);
        window->lines = ptr_line;
    }
    if (window->input_buffer)
        free (window->input_buffer);
    
    completion_free (&(window->completion));
    
    /* remove window from windows list */
    if (window->prev_window)
        window->prev_window->next_window = window->next_window;
    if (window->next_window)
        window->next_window->prev_window = window->prev_window;
    if (gui_windows == window)
        gui_windows = window->next_window;
    if (last_gui_window == window)
        last_gui_window = window->prev_window;
    
    free (window);
    
    /* always at least one window */
    if (!gui_windows && create_new)
        gui_window_new (NULL, NULL);
}

/*
 * gui_new_line: create new line for a window
 */

t_gui_line *
gui_new_line (t_gui_window *window)
{
    t_gui_line *new_line;
    
    if ((new_line = (t_gui_line *) malloc (sizeof (struct t_gui_line))))
    {
        new_line->length = 0;
        new_line->length_align = 0;
        new_line->line_with_message = 0;
        new_line->messages = NULL;
        new_line->last_message = NULL;
        if (!window->lines)
            window->lines = new_line;
        else
            window->last_line->next_line = new_line;
        new_line->prev_line = window->last_line;
        new_line->next_line = NULL;
        window->last_line = new_line;
    }
    else
    {
        log_printf (_("%s not enough memory for new line!\n"));
        return NULL;
    }
    return new_line;
}

/*
 * gui_new_message: create a new message for last line of window
 */

t_gui_message *
gui_new_message (t_gui_window *window)
{
    t_gui_message *new_message;
    
    if ((new_message = (t_gui_message *) malloc (sizeof (struct t_gui_message))))
    {
        if (!window->last_line->messages)
            window->last_line->messages = new_message;
        else
            window->last_line->last_message->next_message = new_message;
        new_message->prev_message = window->last_line->last_message;
        new_message->next_message = NULL;
        window->last_line->last_message = new_message;
    }
    else
    {
        log_printf (_("not enough memory!\n"));
        return NULL;
    }
    return new_message;
}

/*
 * gui_optimize_input_buffer_size: optimize input buffer size by adding
 *                                 or deleting data block (predefined size)
 */

void
gui_optimize_input_buffer_size (t_gui_window *window)
{
    int optimal_size;
    
    optimal_size = ((window->input_buffer_size / INPUT_BUFFER_BLOCK_SIZE) *
                   INPUT_BUFFER_BLOCK_SIZE) + INPUT_BUFFER_BLOCK_SIZE;
    if (window->input_buffer_alloc != optimal_size)
    {
        window->input_buffer_alloc = optimal_size;
        window->input_buffer = realloc (window->input_buffer, optimal_size);
    }
}

/*
 * gui_delete_previous_word: delete previous word 
 */

void
gui_delete_previous_word ()
{
    int i, j, num_char_deleted, num_char_end;
    
    if (gui_current_window->input_buffer_pos > 0)
    {
        i = gui_current_window->input_buffer_pos - 1;
        while ((i >= 0) &&
            (gui_current_window->input_buffer[i] == ' '))
            i--;
        if (i >= 0)
        {
            while ((i >= 0) &&
                (gui_current_window->input_buffer[i] != ' '))
                i--;
            if (i >= 0)
            {
                while ((i >= 0) &&
                    (gui_current_window->input_buffer[i] == ' '))
                    i--;
            }
        }
        
        if (i >= 0)
            i++;
        i++;
        num_char_deleted = gui_current_window->input_buffer_pos - i;
        num_char_end = gui_current_window->input_buffer_size -
            gui_current_window->input_buffer_pos;
        
        for (j = 0; j < num_char_end; j++)
            gui_current_window->input_buffer[i + j] =
                gui_current_window->input_buffer[gui_current_window->input_buffer_pos + j];
        
        gui_current_window->input_buffer_size -= num_char_deleted;
        gui_current_window->input_buffer[gui_current_window->input_buffer_size] = '\0';
        gui_current_window->input_buffer_pos = i;
        gui_draw_window_input (gui_current_window);
        gui_optimize_input_buffer_size (gui_current_window);
        gui_current_window->completion.position = -1;
    }
}

/*
 * gui_move_previous_word: move to beginning of previous word
 */

void
gui_move_previous_word ()
{
    int i;
    
    if (gui_current_window->input_buffer_pos > 0)
    {
        i = gui_current_window->input_buffer_pos - 1;
        while ((i >= 0) &&
            (gui_current_window->input_buffer[i] == ' '))
            i--;
        if (i < 0)
            gui_current_window->input_buffer_pos = 0;
        else
        {
            while ((i >= 0) &&
                (gui_current_window->input_buffer[i] != ' '))
                i--;
            gui_current_window->input_buffer_pos = i + 1;
        }
        gui_draw_window_input (gui_current_window);
    }
}

/*
 * gui_move_next_word: move to the end of next
 */

void
gui_move_next_word ()
{
    int i;
    
    if (gui_current_window->input_buffer_pos <
        gui_current_window->input_buffer_size + 1)
    {
        i = gui_current_window->input_buffer_pos;
        while ((i <= gui_current_window->input_buffer_size) &&
            (gui_current_window->input_buffer[i] == ' '))
            i++;
        if (i > gui_current_window->input_buffer_size)
            gui_current_window->input_buffer_pos = i - 1;
        else
        {
            while ((i <= gui_current_window->input_buffer_size) &&
                (gui_current_window->input_buffer[i] != ' '))
                i++;
            if (i > gui_current_window->input_buffer_size)
                gui_current_window->input_buffer_pos =
                    gui_current_window->input_buffer_size;
            else
                gui_current_window->input_buffer_pos = i;
            
        }
        gui_draw_window_input (gui_current_window);
    }
}

/*
 * gui_buffer_insert_string: insert a string into the input buffer
 */

void
gui_buffer_insert_string (char *string, int pos)
{
    int i, start, end, length;
    
    length = strlen (string);
    
    /* increase buffer size */
    gui_current_window->input_buffer_size += length;
    gui_optimize_input_buffer_size (gui_current_window);
    gui_current_window->input_buffer[gui_current_window->input_buffer_size] = '\0';
    
    /* move end of string to the right */
    start = pos + length;
    end = gui_current_window->input_buffer_size - 1;
    for (i = end; i >= start; i--)
         gui_current_window->input_buffer[i] =
         gui_current_window->input_buffer[i - length];
    
    /* insert new string */
    strncpy (gui_current_window->input_buffer + pos, string, length);
}
