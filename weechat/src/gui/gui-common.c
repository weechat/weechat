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
#include "../common/weeconfig.h"
#include "../common/hotlist.h"
#include "../common/log.h"
#include "../irc/irc.h"


int gui_init_ok = 0;                        /* = 1 if GUI is initialized    */
int gui_ok = 0;                             /* = 1 if GUI is ok             */
                                            /* (0 when term size too small) */

t_gui_window *gui_windows = NULL;           /* pointer to first window      */
t_gui_window *last_gui_window = NULL;       /* pointer to last window       */
t_gui_window *gui_current_window = NULL;    /* pointer to current window    */

t_gui_buffer *gui_buffers = NULL;           /* pointer to first buffer      */
t_gui_buffer *last_gui_buffer = NULL;       /* pointer to last buffer       */
t_gui_buffer *buffer_before_dcc = NULL;     /* buffer before dcc switch     */
t_gui_infobar *gui_infobar;                 /* pointer to infobar content   */


/*
 * gui_window_new: create a new window
 */

t_gui_window *
gui_window_new (int x, int y, int width, int height)
{
    t_gui_window *new_window;
    
    #ifdef DEBUG
    wee_log_printf ("creating new window (x:%d, y:%d, width:%d, height:%d)\n",
                    x, y, width, height);
    #endif
    if ((new_window = (t_gui_window *)(malloc (sizeof (t_gui_window)))))
    {
        new_window->win_x = x;
        new_window->win_y = y;
        new_window->win_width = width;
        new_window->win_height = height;
        
        new_window->win_chat_x = 0;
        new_window->win_chat_y = 0;
        new_window->win_chat_width = 0;
        new_window->win_chat_height = 0;
        new_window->win_chat_cursor_x = 0;
        new_window->win_chat_cursor_y = 0;
        
        new_window->win_nick_x = 0;
        new_window->win_nick_y = 0;
        new_window->win_nick_width = 0;
        new_window->win_nick_height = 0;
        
        new_window->win_title = NULL;
        new_window->win_chat = NULL;
        new_window->win_nick = NULL;
        new_window->win_status = NULL;
        new_window->win_infobar = NULL;
        new_window->win_input = NULL;
        new_window->win_separator = NULL;
        
        new_window->textview_chat = NULL;
        new_window->textbuffer_chat = NULL;
        new_window->texttag_chat = NULL;
        new_window->textview_nicklist = NULL;
        new_window->textbuffer_nicklist = NULL;
        
        new_window->dcc_first = NULL;
        new_window->dcc_selected = NULL;
        new_window->dcc_last_displayed = NULL;
        
        new_window->buffer = NULL;
        
        new_window->first_line_displayed = 0;
        new_window->sub_lines = 0;
        
        /* add window to windows queue */
        new_window->prev_window = last_gui_window;
        if (gui_windows)
            last_gui_window->next_window = new_window;
        else
            gui_windows = new_window;
        last_gui_window = new_window;
        new_window->next_window = NULL;
    }
    else
        return NULL;
    
    return new_window;
}

/*
 * gui_buffer_new: create a new buffer in current window
 */

t_gui_buffer *
gui_buffer_new (t_gui_window *window, void *server, void *channel, int dcc,
                int switch_to_buffer)
{
    t_gui_buffer *new_buffer;
    
    #ifdef DEBUG
    wee_log_printf ("creating new buffer\n");
    #endif
    
    /* use first buffer if no server was assigned to this buffer */
    if (!dcc && gui_buffers && (!SERVER(gui_buffers)))
    {
        if (server)
            ((t_irc_server *)(server))->buffer = gui_buffers;
        if (channel)
            ((t_irc_channel *)(channel))->buffer = gui_buffers;
        SERVER(gui_buffers) = server;
        CHANNEL(gui_buffers) = channel;
        if (cfg_log_auto_server)
            log_start (gui_buffers);
        return gui_buffers;
    }
    
    if ((new_buffer = (t_gui_buffer *)(malloc (sizeof (t_gui_buffer)))))
    {
        new_buffer->num_displayed = 0;
        new_buffer->number = (last_gui_buffer) ? last_gui_buffer->number + 1 : 1;
        
        /* assign server and channel to buffer */
        SERVER(new_buffer) = server;
        CHANNEL(new_buffer) = channel;
        new_buffer->dcc = dcc;
        /* assign buffer to server and channel */
        if (server && !channel)
            SERVER(new_buffer)->buffer = new_buffer;
        if (channel)
            CHANNEL(new_buffer)->buffer = new_buffer;
        
        if (!window->buffer)
        {
            window->buffer = new_buffer;
            window->first_line_displayed = 1;
            window->sub_lines = 0;
            gui_calculate_pos_size (window);
            gui_window_init_subwindows (window);
        }
        
        /* init lines */
        new_buffer->lines = NULL;
        new_buffer->last_line = NULL;
        new_buffer->num_lines = 0;
        new_buffer->line_complete = 1;
        
        /* create/append to log file */
        new_buffer->log_filename = NULL;
        new_buffer->log_file = NULL;
        if ((cfg_log_auto_server && BUFFER_IS_SERVER(new_buffer))
            || (cfg_log_auto_channel && BUFFER_IS_CHANNEL(new_buffer))
            || (cfg_log_auto_private && BUFFER_IS_PRIVATE(new_buffer)))
            log_start (new_buffer);
        
        /* init input buffer */
        new_buffer->input_buffer_alloc = INPUT_BUFFER_BLOCK_SIZE;
        new_buffer->input_buffer = (char *) malloc (INPUT_BUFFER_BLOCK_SIZE);
        new_buffer->input_buffer[0] = '\0';
        new_buffer->input_buffer_size = 0;
        new_buffer->input_buffer_pos = 0;
        new_buffer->input_buffer_1st_display = 0;
        
        /* init completion */
        completion_init (&(new_buffer->completion));
        
        /* init history */
        new_buffer->history = NULL;
        new_buffer->last_history = NULL;
        new_buffer->ptr_history = NULL;
        new_buffer->num_history = 0;
        
        /* add buffer to buffers queue */
        new_buffer->prev_buffer = last_gui_buffer;
        if (gui_buffers)
            last_gui_buffer->next_buffer = new_buffer;
        else
            gui_buffers = new_buffer;
        last_gui_buffer = new_buffer;
        new_buffer->next_buffer = NULL;
        
        /* switch to new buffer */
        if (switch_to_buffer)
            gui_switch_to_buffer (window, new_buffer);
        
        /* redraw buffer */
        gui_redraw_buffer (new_buffer);
    }
    else
        return NULL;
    
    return new_buffer;
}

/*
 * gui_buffer_clear: clear buffer content
 */

void
gui_buffer_clear (t_gui_buffer *buffer)
{
    t_gui_window *ptr_win;
    t_gui_line *ptr_line;
    t_gui_message *ptr_message;
    
    while (buffer->lines)
    {
        ptr_line = buffer->lines->next_line;
        while (buffer->lines->messages)
        {
            ptr_message = buffer->lines->messages->next_message;
            if (buffer->lines->messages->message)
                free (buffer->lines->messages->message);
            free (buffer->lines->messages);
            buffer->lines->messages = ptr_message;
        }
        free (buffer->lines);
        buffer->lines = ptr_line;
    }
    
    buffer->lines = NULL;
    buffer->last_line = NULL;
    buffer->num_lines = 0;
    buffer->line_complete = 1;
    
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        if (ptr_win->buffer == buffer)
        {
            ptr_win->first_line_displayed = 1;
            ptr_win->sub_lines = 0;
        }
    }
    
    if (buffer == gui_current_window->buffer)
        gui_draw_buffer_chat (buffer, 1);
}

/*
 * gui_buffer_clear_all: clear all buffers content
 */

void
gui_buffer_clear_all ()
{
    t_gui_buffer *ptr_buffer;
    
    for (ptr_buffer = gui_buffers; ptr_buffer; ptr_buffer = ptr_buffer->next_buffer)
        gui_buffer_clear (ptr_buffer);
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
        gui_draw_buffer_infobar (gui_current_window->buffer, 1);
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
        gui_draw_buffer_infobar (gui_current_window->buffer, 1);
    }
}

/*
 * gui_line_free: delete a line from a buffer
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
 * gui_buffer_free: delete a buffer
 */

void
gui_buffer_free (t_gui_buffer *buffer, int switch_to_another)
{
    t_gui_window *ptr_win;
    t_gui_buffer *ptr_buffer;
    t_gui_line *ptr_line;
    int create_new;
    
    create_new = (buffer->server || buffer->channel);
    
    hotlist_remove_buffer (buffer);
    if (hotlist_initial_buffer == buffer)
        hotlist_initial_buffer = NULL;
    
    if (switch_to_another)
    {
        for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
        {
            if ((buffer == ptr_win->buffer) &&
                ((buffer->next_buffer) || (buffer->prev_buffer)))
                gui_switch_to_previous_buffer (ptr_win);
        }
    }
    
    /* decrease buffer number for all next buffers */
    for (ptr_buffer = buffer->next_buffer; ptr_buffer; ptr_buffer = ptr_buffer->next_buffer)
    {
        ptr_buffer->number--;
    }
    
    /* free lines and messages */
    while (buffer->lines)
    {
        ptr_line = buffer->lines->next_line;
        gui_line_free (buffer->lines);
        buffer->lines = ptr_line;
    }
    
    /* close log if opened */
    if (buffer->log_file)
        log_end (buffer);
    
    if (buffer->input_buffer)
        free (buffer->input_buffer);
    
    completion_free (&(buffer->completion));
    
    /* remove buffer from buffers list */
    if (buffer->prev_buffer)
        buffer->prev_buffer->next_buffer = buffer->next_buffer;
    if (buffer->next_buffer)
        buffer->next_buffer->prev_buffer = buffer->prev_buffer;
    if (gui_buffers == buffer)
        gui_buffers = buffer->next_buffer;
    if (last_gui_buffer == buffer)
        last_gui_buffer = buffer->prev_buffer;
    
    free (buffer);
    
    /* always at least one buffer */
    if (!gui_buffers && create_new && switch_to_another)
        (void) gui_buffer_new (gui_windows, NULL, NULL, 0, 1);
}

/*
 * gui_new_line: create new line for a buffer
 */

t_gui_line *
gui_new_line (t_gui_buffer *buffer)
{
    t_gui_line *new_line, *ptr_line;
    
    if ((new_line = (t_gui_line *) malloc (sizeof (struct t_gui_line))))
    {
        new_line->length = 0;
        new_line->length_align = 0;
        new_line->line_with_message = 0;
        new_line->line_with_highlight = 0;
        new_line->messages = NULL;
        new_line->last_message = NULL;
        if (!buffer->lines)
            buffer->lines = new_line;
        else
            buffer->last_line->next_line = new_line;
        new_line->prev_line = buffer->last_line;
        new_line->next_line = NULL;
        buffer->last_line = new_line;
        buffer->num_lines++;
    }
    else
    {
        wee_log_printf (_("%s not enough memory for new line!\n"));
        return NULL;
    }
    
    /* remove one line if necessary */
    if ((cfg_history_max_lines > 0)
        && (buffer->num_lines > cfg_history_max_lines))
    {
        if (buffer->last_line == buffer->lines)
            buffer->last_line = NULL;
        ptr_line = buffer->lines->next_line;
        gui_line_free (buffer->lines);
        buffer->lines = ptr_line;
        ptr_line->prev_line = NULL;
        buffer->num_lines--;
        //if (buffer->first_line_displayed)
        gui_draw_buffer_chat (buffer, 1);
    }
    
    return new_line;
}

/*
 * gui_new_message: create a new message for last line of a buffer
 */

t_gui_message *
gui_new_message (t_gui_buffer *buffer)
{
    t_gui_message *new_message;
    
    if ((new_message = (t_gui_message *) malloc (sizeof (struct t_gui_message))))
    {
        if (!buffer->last_line->messages)
            buffer->last_line->messages = new_message;
        else
            buffer->last_line->last_message->next_message = new_message;
        new_message->prev_message = buffer->last_line->last_message;
        new_message->next_message = NULL;
        buffer->last_line->last_message = new_message;
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
gui_optimize_input_buffer_size (t_gui_buffer *buffer)
{
    int optimal_size;
    
    optimal_size = ((buffer->input_buffer_size / INPUT_BUFFER_BLOCK_SIZE) *
                   INPUT_BUFFER_BLOCK_SIZE) + INPUT_BUFFER_BLOCK_SIZE;
    if (buffer->input_buffer_alloc != optimal_size)
    {
        buffer->input_buffer_alloc = optimal_size;
        buffer->input_buffer = realloc (buffer->input_buffer, optimal_size);
    }
}

/*
 * gui_delete_previous_word: delete previous word 
 */

void
gui_delete_previous_word (t_gui_buffer *buffer)
{
    int i, j, num_char_deleted, num_char_end;
    
    if (buffer->input_buffer_pos > 0)
    {
        i = buffer->input_buffer_pos - 1;
        while ((i >= 0) &&
            (buffer->input_buffer[i] == ' '))
            i--;
        if (i >= 0)
        {
            while ((i >= 0) &&
                (buffer->input_buffer[i] != ' '))
                i--;
            if (i >= 0)
            {
                while ((i >= 0) &&
                    (buffer->input_buffer[i] == ' '))
                    i--;
            }
        }
        
        if (i >= 0)
            i++;
        i++;
        num_char_deleted = buffer->input_buffer_pos - i;
        num_char_end = buffer->input_buffer_size -
            buffer->input_buffer_pos;
        
        for (j = 0; j < num_char_end; j++)
            buffer->input_buffer[i + j] =
                buffer->input_buffer[buffer->input_buffer_pos + j];
        
        buffer->input_buffer_size -= num_char_deleted;
        buffer->input_buffer[buffer->input_buffer_size] = '\0';
        buffer->input_buffer_pos = i;
        gui_draw_buffer_input (buffer, 0);
        gui_optimize_input_buffer_size (buffer);
        buffer->completion.position = -1;
    }
}

/*
 * gui_move_previous_word: move to beginning of previous word
 */

void
gui_move_previous_word (t_gui_buffer *buffer)
{
    int i;
    
    if (buffer->input_buffer_pos > 0)
    {
        i = buffer->input_buffer_pos - 1;
        while ((i >= 0) &&
            (buffer->input_buffer[i] == ' '))
            i--;
        if (i < 0)
            buffer->input_buffer_pos = 0;
        else
        {
            while ((i >= 0) &&
                (buffer->input_buffer[i] != ' '))
                i--;
            buffer->input_buffer_pos = i + 1;
        }
        gui_draw_buffer_input (buffer, 0);
    }
}

/*
 * gui_move_next_word: move to the end of next
 */

void
gui_move_next_word (t_gui_buffer *buffer)
{
    int i;
    
    if (buffer->input_buffer_pos <
        buffer->input_buffer_size + 1)
    {
        i = buffer->input_buffer_pos;
        while ((i <= buffer->input_buffer_size) &&
            (buffer->input_buffer[i] == ' '))
            i++;
        if (i > buffer->input_buffer_size)
            buffer->input_buffer_pos = i - 1;
        else
        {
            while ((i <= buffer->input_buffer_size) &&
                (buffer->input_buffer[i] != ' '))
                i++;
            if (i > buffer->input_buffer_size)
                buffer->input_buffer_pos =
                    buffer->input_buffer_size;
            else
                buffer->input_buffer_pos = i;
            
        }
        gui_draw_buffer_input (buffer, 0);
    }
}

/*
 * gui_buffer_insert_string: insert a string into the input buffer
 */

void
gui_buffer_insert_string (t_gui_buffer *buffer, char *string, int pos)
{
    int i, start, end, length;
    
    length = strlen (string);
    
    /* increase buffer size */
    buffer->input_buffer_size += length;
    gui_optimize_input_buffer_size (buffer);
    buffer->input_buffer[buffer->input_buffer_size] = '\0';
    
    /* move end of string to the right */
    start = pos + length;
    end = buffer->input_buffer_size - 1;
    for (i = end; i >= start; i--)
         buffer->input_buffer[i] =
         buffer->input_buffer[i - length];
    
    /* insert new string */
    strncpy (buffer->input_buffer + pos, string, length);
}

/*
 * gui_switch_to_buffer_by_number: switch to another buffer with number
 */

t_gui_buffer *
gui_switch_to_buffer_by_number (t_gui_window *window, int number)
{
    t_gui_buffer *ptr_buffer;
    
    /* buffer is currently displayed ? */
    if (number == window->buffer->number)
        return window->buffer;
    
    /* search for buffer in the list */
    for (ptr_buffer = gui_buffers; ptr_buffer; ptr_buffer = ptr_buffer->next_buffer)
    {
        if ((ptr_buffer != window->buffer) && (number == ptr_buffer->number))
        {
            gui_switch_to_buffer (window, ptr_buffer);
            gui_redraw_buffer (window->buffer);
            return ptr_buffer;
        }
    }
    
    /* buffer not found */
    return NULL;
}

/*
 * gui_switch_to_buffer_by_number: switch to another buffer with number
 */

void
gui_move_buffer_to_number (t_gui_window *window, int number)
{
    t_gui_buffer *ptr_buffer;
    int i;
    
    /* buffer number is already ok ? */
    if (number == window->buffer->number)
        return;
    
    if (number < 1)
        number = 1;
    
    /* remove buffer from list */
    if (window->buffer == gui_buffers)
    {
        gui_buffers = window->buffer->next_buffer;
        gui_buffers->prev_buffer = NULL;
    }
    if (window->buffer == last_gui_buffer)
    {
        last_gui_buffer = window->buffer->prev_buffer;
        last_gui_buffer->next_buffer = NULL;
    }
    if (window->buffer->prev_buffer)
        (window->buffer->prev_buffer)->next_buffer = window->buffer->next_buffer;
    if (window->buffer->next_buffer)
        (window->buffer->next_buffer)->prev_buffer = window->buffer->prev_buffer;
    
    if (number == 1)
    {
        gui_buffers->prev_buffer = window->buffer;
        window->buffer->prev_buffer = NULL;
        window->buffer->next_buffer = gui_buffers;
        gui_buffers = window->buffer;
    }
    else
    {
        /* assign new number to all buffers */
        i = 1;
        for (ptr_buffer = gui_buffers; ptr_buffer; ptr_buffer = ptr_buffer->next_buffer)
        {
            ptr_buffer->number = i++;
        }
        
        /* search for new position in the list */
        for (ptr_buffer = gui_buffers; ptr_buffer; ptr_buffer = ptr_buffer->next_buffer)
        {
            if (ptr_buffer->number == number)
                break;
        }
        if (ptr_buffer)
        {
            /* insert before buffer found */
            window->buffer->prev_buffer = ptr_buffer->prev_buffer;
            window->buffer->next_buffer = ptr_buffer;
            if (ptr_buffer->prev_buffer)
                (ptr_buffer->prev_buffer)->next_buffer = window->buffer;
            ptr_buffer->prev_buffer = window->buffer;
        }
        else
        {
            /* number not found (too big)? => add to end */
            window->buffer->prev_buffer = last_gui_buffer;
            window->buffer->next_buffer = NULL;
            last_gui_buffer->next_buffer = window->buffer;
            last_gui_buffer = window->buffer;
        }
        
    }
    
    /* assign new number to all buffers */
    i = 1;
    for (ptr_buffer = gui_buffers; ptr_buffer; ptr_buffer = ptr_buffer->next_buffer)
    {
        ptr_buffer->number = i++;
    }
    
    gui_redraw_buffer (window->buffer);
}
