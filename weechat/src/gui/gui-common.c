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

/* gui-common.c: display functions, used by any GUI */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <curses.h>

#include "../common/weechat.h"
#include "gui.h"
#include "../../common/weeconfig.h"
#include "../irc/irc.h"


int gui_ready;                              /* = 1 if GUI is initialized    */

t_gui_window *gui_windows = NULL;           /* pointer to first window      */

t_gui_view *gui_views = NULL;               /* pointer to first view        */
t_gui_view *last_gui_view = NULL;           /* pointer to last view         */
t_gui_view *gui_current_view = NULL;        /* pointer to current view      */
t_gui_infobar *gui_infobar;                 /* pointer to infobar content   */


/*
 * gui_window_new: create a new window
 */

t_gui_window *
gui_window_new (int x, int y, int width, int height)
{
    t_gui_window *window;
    
    if ((window = (t_gui_window *)(malloc (sizeof (t_gui_window)))))
    {
        window->win_x = x;
        window->win_y = y;
        window->win_width = width;
        window->win_height = height;
        
        window->win_chat_x = 0;
        window->win_chat_y = 0;
        window->win_chat_width = 0;
        window->win_chat_height = 0;
        window->win_chat_cursor_x = 0;
        window->win_chat_cursor_y = 0;
        
        window->win_nick_x = 0;
        window->win_nick_y = 0;
        window->win_nick_width = 0;
        window->win_nick_height = 0;
        
        window->win_title = NULL;
        window->win_chat = NULL;
        window->win_nick = NULL;
        window->win_status = NULL;
        window->win_infobar = NULL;
        window->win_input = NULL;
        window->textview_chat = NULL;
        window->textbuffer_chat = NULL;
        window->texttag_chat = NULL;
        window->textview_nicklist = NULL;
        window->textbuffer_nicklist = NULL;
    }
    else
        return NULL;
    
    return window;
}

/*
 * gui_view_new: create a new view in current window
 */

t_gui_view *
gui_view_new (t_gui_window *window, void *server, void *channel, int switch_to_view)
{
    t_gui_view *new_view;
    
    if (gui_views)
    {
        /* use first view if no server was assigned to this view */
        if (!SERVER(gui_views))
        {
            if (server)
                ((t_irc_server *)(server))->view = gui_views;
            if (channel)
                ((t_irc_channel *)(channel))->view = gui_views;
            SERVER(gui_views) = server;
            CHANNEL(gui_views) = channel;
            return gui_views;
        }
    }
    
    if ((new_view = (t_gui_view *)(malloc (sizeof (t_gui_view)))))
    {
        new_view->is_displayed = 0;
        
        /* assign server and channel to view */
        SERVER(new_view) = server;
        CHANNEL(new_view) = channel;
        /* assign view to server and channel */
        if (server && !channel)
            SERVER(new_view)->view = new_view;
        if (channel)
            CHANNEL(new_view)->view = new_view;
        
        new_view->window = window;
        
        gui_calculate_pos_size (new_view);
        
        /* init views */
        gui_view_init_subviews(new_view);
        
        /* init lines */
        new_view->lines = NULL;
        new_view->last_line = NULL;
        new_view->num_lines = 0;
        new_view->first_line_displayed = 1;
        new_view->sub_lines = 0;
        new_view->line_complete = 1;
        new_view->unread_data = 0;
        
        /* init input buffer */
        new_view->input_buffer_alloc = INPUT_BUFFER_BLOCK_SIZE;
        new_view->input_buffer = (char *) malloc (INPUT_BUFFER_BLOCK_SIZE);
        new_view->input_buffer[0] = '\0';
        new_view->input_buffer_size = 0;
        new_view->input_buffer_pos = 0;
        new_view->input_buffer_1st_display = 0;
        
        /* init completion */
        completion_init (&(new_view->completion));
        
        /* init history */
        new_view->history = NULL;
        new_view->last_history = NULL;
        new_view->ptr_history = NULL;
        new_view->num_history = 0;
        
        /* switch to new view */
        if (switch_to_view)
            gui_switch_to_view (new_view);
        
        /* add view to views queue */
        new_view->prev_view = last_gui_view;
        if (gui_views)
            last_gui_view->next_view = new_view;
        else
            gui_views = new_view;
        last_gui_view = new_view;
        new_view->next_view = NULL;
        
        /* redraw whole screen */
        /* TODO: manage splited windows */
        gui_redraw_view (gui_current_view);
    }
    else
        return NULL;
    
    return new_view;
}

/*
 * gui_view_clear: clear view content
 */

void
gui_view_clear (t_gui_view *view)
{
    t_gui_line *ptr_line;
    t_gui_message *ptr_message;
    
    while (view->lines)
    {
        ptr_line = view->lines->next_line;
        while (view->lines->messages)
        {
            ptr_message = view->lines->messages->next_message;
            if (view->lines->messages->message)
                free (view->lines->messages->message);
            free (view->lines->messages);
            view->lines->messages = ptr_message;
        }
        free (view->lines);
        view->lines = ptr_line;
    }
    
    view->lines = NULL;
    view->last_line = NULL;
    view->num_lines = 0;
    view->first_line_displayed = 1;
    view->sub_lines = 0;
    view->line_complete = 1;
    view->unread_data = 0;
    
    if (view == gui_current_view)
        gui_redraw_view_chat (view);
}

/*
 * gui_view_clear_all: clear all views content
 */

void
gui_view_clear_all ()
{
    t_gui_view *ptr_view;
    
    for (ptr_view = gui_views; ptr_view; ptr_view = ptr_view->next_view)
        gui_view_clear (ptr_view);
}

/* 
 * gui_infobar_printf: display message in infobar
 */

void
gui_infobar_printf (int time_displayed, int color, char *message, ...)
{
    static char buffer[1024];
    va_list argptr;
    t_gui_infobar *ptr_infobar;
    char *pos;
    
    va_start (argptr, message);
    vsnprintf (buffer, sizeof (buffer) - 1, message, argptr);
    va_end (argptr);
    
    ptr_infobar = (t_gui_infobar *)malloc (sizeof (t_gui_infobar));
    if (ptr_infobar)
    {
        ptr_infobar->color = color;
        ptr_infobar->text = strdup (buffer);
        pos = strchr (ptr_infobar->text, '\n');
        if (pos)
            pos[0] = '\0';
        ptr_infobar->remaining_time = (time_displayed <= 0) ? -1 : time_displayed;
        ptr_infobar->next_infobar = gui_infobar;
        gui_infobar = ptr_infobar;
        /* TODO: manage splited windows! */
        gui_redraw_view_infobar (gui_current_view);
    }
    else
        wee_log_printf (_("%s not enough memory for infobar message\n"),
                        WEECHAT_ERROR);
}

/*
 * gui_infobar_remove: remove last displayed message in infobar
 */

void
gui_infobar_remove ()
{
    t_gui_infobar *new_infobar;
    
    if (gui_infobar)
    {
        new_infobar = gui_infobar->next_infobar;
        if (gui_infobar->text)
            free (gui_infobar->text);
        free (gui_infobar);
        gui_infobar = new_infobar;
        /* TODO: manage splited windows! */
        gui_redraw_view_infobar (gui_current_view);
    }
}

/*
 * gui_line_free: delete a line from a view
 */

void
gui_line_free (t_gui_line *line)
{
    t_gui_message *ptr_message;
    
    while (line->messages)
    {
        ptr_message = line->messages->next_message;
        if (line->messages->message)
            free (line->messages->message);
        free (line->messages);
        line->messages = ptr_message;
    }
    free (line);
}

/*
 * gui_view_free: delete a view
 */

void
gui_view_free (t_gui_view *view)
{
    t_gui_line *ptr_line;
    int create_new;
    
    create_new = (view->server || view->channel);
    
    /* TODO: manage splited windows! */
    if ((view == gui_current_view) &&
        ((view->next_view) || (view->prev_view)))
        gui_switch_to_previous_view ();
    
    /* free lines and messages */
    while (view->lines)
    {
        ptr_line = view->lines->next_line;
        gui_line_free (view->lines);
        view->lines = ptr_line;
    }
    if (view->input_buffer)
        free (view->input_buffer);
    
    completion_free (&(view->completion));
    
    /* remove view from views list */
    if (view->prev_view)
        view->prev_view->next_view = view->next_view;
    if (view->next_view)
        view->next_view->prev_view = view->prev_view;
    if (gui_views == view)
        gui_views = view->next_view;
    if (last_gui_view == view)
        last_gui_view = view->prev_view;
    
    free (view);
    
    /* always at least one view */
    if (!gui_views && create_new)
        (void) gui_view_new (gui_windows, NULL, NULL, 1);
}

/*
 * gui_new_line: create new line for a view
 */

t_gui_line *
gui_new_line (t_gui_view *view)
{
    t_gui_line *new_line, *ptr_line;
    
    if ((new_line = (t_gui_line *) malloc (sizeof (struct t_gui_line))))
    {
        new_line->length = 0;
        new_line->length_align = 0;
        new_line->line_with_message = 0;
        new_line->messages = NULL;
        new_line->last_message = NULL;
        if (!view->lines)
            view->lines = new_line;
        else
            view->last_line->next_line = new_line;
        new_line->prev_line = view->last_line;
        new_line->next_line = NULL;
        view->last_line = new_line;
        view->num_lines++;
    }
    else
    {
        wee_log_printf (_("%s not enough memory for new line!\n"));
        return NULL;
    }
    
    /* remove one line if necessary */
    if ((cfg_history_max_lines > 0)
        && (view->num_lines > cfg_history_max_lines))
    {
        if (view->last_line == view->lines)
            view->last_line = NULL;
        ptr_line = view->lines->next_line;
        gui_line_free (view->lines);
        view->lines = ptr_line;
        ptr_line->prev_line = NULL;
        view->num_lines--;
        if (view->first_line_displayed)
            gui_redraw_view_chat (view);
    }
    
    return new_line;
}

/*
 * gui_new_message: create a new message for last line of a view
 */

t_gui_message *
gui_new_message (t_gui_view *view)
{
    t_gui_message *new_message;
    
    if ((new_message = (t_gui_message *) malloc (sizeof (struct t_gui_message))))
    {
        if (!view->last_line->messages)
            view->last_line->messages = new_message;
        else
            view->last_line->last_message->next_message = new_message;
        new_message->prev_message = view->last_line->last_message;
        new_message->next_message = NULL;
        view->last_line->last_message = new_message;
    }
    else
    {
        wee_log_printf (_("not enough memory!\n"));
        return NULL;
    }
    return new_message;
}

/*
 * gui_optimize_input_buffer_size: optimize input buffer size by adding
 *                                 or deleting data block (predefined size)
 */

void
gui_optimize_input_buffer_size (t_gui_view *view)
{
    int optimal_size;
    
    optimal_size = ((view->input_buffer_size / INPUT_BUFFER_BLOCK_SIZE) *
                   INPUT_BUFFER_BLOCK_SIZE) + INPUT_BUFFER_BLOCK_SIZE;
    if (view->input_buffer_alloc != optimal_size)
    {
        view->input_buffer_alloc = optimal_size;
        view->input_buffer = realloc (view->input_buffer, optimal_size);
    }
}

/*
 * gui_delete_previous_word: delete previous word 
 */

void
gui_delete_previous_word ()
{
    int i, j, num_char_deleted, num_char_end;
    
    if (gui_current_view->input_buffer_pos > 0)
    {
        i = gui_current_view->input_buffer_pos - 1;
        while ((i >= 0) &&
            (gui_current_view->input_buffer[i] == ' '))
            i--;
        if (i >= 0)
        {
            while ((i >= 0) &&
                (gui_current_view->input_buffer[i] != ' '))
                i--;
            if (i >= 0)
            {
                while ((i >= 0) &&
                    (gui_current_view->input_buffer[i] == ' '))
                    i--;
            }
        }
        
        if (i >= 0)
            i++;
        i++;
        num_char_deleted = gui_current_view->input_buffer_pos - i;
        num_char_end = gui_current_view->input_buffer_size -
            gui_current_view->input_buffer_pos;
        
        for (j = 0; j < num_char_end; j++)
            gui_current_view->input_buffer[i + j] =
                gui_current_view->input_buffer[gui_current_view->input_buffer_pos + j];
        
        gui_current_view->input_buffer_size -= num_char_deleted;
        gui_current_view->input_buffer[gui_current_view->input_buffer_size] = '\0';
        gui_current_view->input_buffer_pos = i;
        gui_draw_view_input (gui_current_view);
        gui_optimize_input_buffer_size (gui_current_view);
        gui_current_view->completion.position = -1;
    }
}

/*
 * gui_move_previous_word: move to beginning of previous word
 */

void
gui_move_previous_word ()
{
    int i;
    
    if (gui_current_view->input_buffer_pos > 0)
    {
        i = gui_current_view->input_buffer_pos - 1;
        while ((i >= 0) &&
            (gui_current_view->input_buffer[i] == ' '))
            i--;
        if (i < 0)
            gui_current_view->input_buffer_pos = 0;
        else
        {
            while ((i >= 0) &&
                (gui_current_view->input_buffer[i] != ' '))
                i--;
            gui_current_view->input_buffer_pos = i + 1;
        }
        gui_draw_view_input (gui_current_view);
    }
}

/*
 * gui_move_next_word: move to the end of next
 */

void
gui_move_next_word ()
{
    int i;
    
    if (gui_current_view->input_buffer_pos <
        gui_current_view->input_buffer_size + 1)
    {
        i = gui_current_view->input_buffer_pos;
        while ((i <= gui_current_view->input_buffer_size) &&
            (gui_current_view->input_buffer[i] == ' '))
            i++;
        if (i > gui_current_view->input_buffer_size)
            gui_current_view->input_buffer_pos = i - 1;
        else
        {
            while ((i <= gui_current_view->input_buffer_size) &&
                (gui_current_view->input_buffer[i] != ' '))
                i++;
            if (i > gui_current_view->input_buffer_size)
                gui_current_view->input_buffer_pos =
                    gui_current_view->input_buffer_size;
            else
                gui_current_view->input_buffer_pos = i;
            
        }
        gui_draw_view_input (gui_current_view);
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
    gui_current_view->input_buffer_size += length;
    gui_optimize_input_buffer_size (gui_current_view);
    gui_current_view->input_buffer[gui_current_view->input_buffer_size] = '\0';
    
    /* move end of string to the right */
    start = pos + length;
    end = gui_current_view->input_buffer_size - 1;
    for (i = end; i >= start; i--)
         gui_current_view->input_buffer[i] =
         gui_current_view->input_buffer[i - length];
    
    /* insert new string */
    strncpy (gui_current_view->input_buffer + pos, string, length);
}
