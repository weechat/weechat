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

/* gui-window.c: window functions, used by all GUI */


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
#include "../common/command.h"
#include "../common/weeconfig.h"
#include "../common/history.h"
#include "../common/hotlist.h"
#include "../common/log.h"
#include "../common/utf8.h"
#include "../irc/irc.h"


t_gui_buffer *gui_buffers = NULL;           /* pointer to first buffer      */
t_gui_buffer *last_gui_buffer = NULL;       /* pointer to last buffer       */
t_gui_buffer *gui_buffer_before_dcc = NULL; /* buffer before dcc switch     */
t_gui_buffer *gui_buffer_raw_data = NULL;   /* buffer with raw IRC data     */
t_gui_buffer *gui_buffer_before_raw_data = NULL; /* buffer before raw switch*/


/*
 * gui_buffer_servers_search: search servers buffer
 *                            (when same buffer is used for all servers)
 */

t_gui_buffer *
gui_buffer_servers_search ()
{
    t_gui_buffer *ptr_buffer;
    
    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        if (ptr_buffer->all_servers)
            return ptr_buffer;
    }
    
    /* buffer not found */
    return NULL;
}

/*
 * gui_buffer_new: create a new buffer in current window
 */

t_gui_buffer *
gui_buffer_new (t_gui_window *window, void *server, void *channel, int type,
                int switch_to_buffer)
{
    t_gui_buffer *new_buffer, *ptr_buffer;
    
#ifdef DEBUG
    weechat_log_printf ("Creating new buffer\n");
#endif
    
    /* use first buffer if no server was assigned to this buffer */
    if ((type == BUFFER_TYPE_STANDARD) && gui_buffers && (!SERVER(gui_buffers)))
    {
        if (server)
            ((t_irc_server *)(server))->buffer = gui_buffers;
        if (channel)
            ((t_irc_channel *)(channel))->buffer = gui_buffers;
        gui_buffers->server = server;
        gui_buffers->channel = channel;
        if (cfg_look_one_server_buffer && server && !channel)
            gui_buffers->all_servers = 1;
        if (cfg_log_auto_server)
            gui_log_start (gui_buffers);
        gui_buffers->completion.server = server;
        return gui_buffers;
    }
    
    if (cfg_look_one_server_buffer && (type == BUFFER_TYPE_STANDARD) &&
        gui_buffers && server && !channel)
    {
        ptr_buffer = gui_buffer_servers_search ();
        if (ptr_buffer)
        {
            ((t_irc_server *)(server))->buffer = gui_buffers;
            gui_buffers->server = server;
            if (switch_to_buffer)
                gui_window_switch_to_buffer (window, gui_buffers);
            gui_window_redraw_buffer (gui_buffers);
            return gui_buffers;
        }
    }
    
    if ((new_buffer = (t_gui_buffer *)(malloc (sizeof (t_gui_buffer)))))
    {
        new_buffer->num_displayed = 0;
        new_buffer->number = (last_gui_buffer) ? last_gui_buffer->number + 1 : 1;
        
        /* assign server and channel to buffer */
        new_buffer->server = server;
        new_buffer->all_servers = 0;
        new_buffer->channel = channel;
        new_buffer->type = type;
        if (new_buffer->type == BUFFER_TYPE_RAW_DATA)
            gui_buffer_raw_data = new_buffer;
        /* assign buffer to server and channel */
        if (server && !channel)
        {
            SERVER(new_buffer)->buffer = new_buffer;
            new_buffer->all_servers = (cfg_look_one_server_buffer) ? 1 : 0;
        }
        if (!gui_buffers && cfg_look_one_server_buffer)
            new_buffer->all_servers = 1;
        if (channel)
            CHANNEL(new_buffer)->buffer = new_buffer;
        
        if (!window->buffer)
        {
            window->buffer = new_buffer;
            window->first_line_displayed = 1;
            window->start_line = NULL;
            window->start_line_pos = 0;
            gui_window_calculate_pos_size (window, 1);
        }
        
        /* init lines */
        new_buffer->lines = NULL;
        new_buffer->last_line = NULL;
        new_buffer->last_read_line = NULL;
        new_buffer->num_lines = 0;
        new_buffer->line_complete = 1;
        
        /* notify level */
        new_buffer->notify_level = channel_get_notify_level (server, channel);
        
        /* create/append to log file */
        new_buffer->log_filename = NULL;
        new_buffer->log_file = NULL;
        if ((cfg_log_auto_server && BUFFER_IS_SERVER(new_buffer))
            || (cfg_log_auto_channel && BUFFER_IS_CHANNEL(new_buffer))
            || (cfg_log_auto_private && BUFFER_IS_PRIVATE(new_buffer)))
            gui_log_start (new_buffer);
        
        /* init input buffer */
        new_buffer->has_input = (new_buffer->type == BUFFER_TYPE_STANDARD) ? 1 : 0;
        if (new_buffer->has_input)
        {
            new_buffer->input_buffer_alloc = INPUT_BUFFER_BLOCK_SIZE;
            new_buffer->input_buffer = (char *) malloc (INPUT_BUFFER_BLOCK_SIZE);
            new_buffer->input_buffer_color_mask = (char *) malloc (INPUT_BUFFER_BLOCK_SIZE);
            new_buffer->input_buffer[0] = '\0';
            new_buffer->input_buffer_color_mask[0] = '\0';
        }
        else
        {
            new_buffer->input_buffer = NULL;
            new_buffer->input_buffer_color_mask = NULL;
        }
        new_buffer->input_buffer_size = 0;
        new_buffer->input_buffer_length = 0;
        new_buffer->input_buffer_pos = 0;
        new_buffer->input_buffer_1st_display = 0;
        
        /* init completion */
        completion_init (&(new_buffer->completion), server, channel);
        
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
        
        /* move buffer next to server */
        if (server && cfg_look_open_near_server && (!cfg_look_one_server_buffer))
        {
            ptr_buffer = SERVER(new_buffer)->buffer;
            while (ptr_buffer && (ptr_buffer->server == server))
            {
                ptr_buffer = ptr_buffer->next_buffer;
            }
            if (ptr_buffer)
                gui_buffer_move_to_number (new_buffer, ptr_buffer->number);
        }
        
        /* switch to new buffer */
        if (switch_to_buffer)
            gui_window_switch_to_buffer (window, new_buffer);
        
        /* redraw buffer */
        gui_window_redraw_buffer (new_buffer);
    }
    else
        return NULL;
    
    return new_buffer;
}

/*
 * gui_buffer_search: search a buffer by server and channel name
 */

t_gui_buffer *
gui_buffer_search (char *server, char *channel)
{
    t_irc_server *ptr_server, *ptr_srv;
    t_irc_channel *ptr_channel, *ptr_chan;
    t_gui_buffer *ptr_buffer;
    
    ptr_server = NULL;
    ptr_channel = NULL;
    ptr_buffer = NULL;
    
    /* nothing given => print on current buffer */
    if ((!server || !server[0]) && (!channel || !channel[0]))
        ptr_buffer = gui_current_window->buffer;
    else
    {
        if (server && server[0])
        {
            ptr_server = server_search (server);
            if (!ptr_server)
                return NULL;
        }
        
        if (channel && channel[0])
        {
            if (ptr_server)
            {
                ptr_channel = channel_search_any (ptr_server, channel);
                if (ptr_channel)
                    ptr_buffer = ptr_channel->buffer;
            }
            else
            {
                for (ptr_srv = irc_servers; ptr_srv;
                     ptr_srv = ptr_srv->next_server)
                {
                    for (ptr_chan = ptr_srv->channels; ptr_chan;
                         ptr_chan = ptr_chan->next_channel)
                    {
                        if (ascii_strcasecmp (ptr_chan->name, channel) == 0)
                        {
                            ptr_channel = ptr_chan;
                            break;
                        }
                    }
                    if (ptr_channel)
                        break;
                }
                if (ptr_channel)
                    ptr_buffer = ptr_channel->buffer;
            }
        }
        else
        {
            if (ptr_server)
                ptr_buffer = ptr_server->buffer;
            else
                ptr_buffer = gui_current_window->buffer;
        }
    }
    
    if (!ptr_buffer)
        return NULL;
    
    return (ptr_buffer->type != BUFFER_TYPE_STANDARD) ?
        gui_buffers : ptr_buffer;
}

/*
 * gui_buffer_search_by_number: search a buffer by number
 */

t_gui_buffer *
gui_buffer_search_by_number (int number)
{
    t_gui_buffer *ptr_buffer;
    
    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        if (ptr_buffer->number == number)
            return ptr_buffer;
    }
    
    /* buffer not found */
    return NULL;
}

/*
 * gui_buffer_find_window: find a window displaying buffer
 */

t_gui_window *
gui_buffer_find_window (t_gui_buffer *buffer)
{
    t_gui_window *ptr_win;
    
    if (gui_current_window->buffer == buffer)
        return gui_current_window;
    
    for (ptr_win = gui_windows; ptr_win;
         ptr_win = ptr_win->next_window)
    {
        if (ptr_win->buffer == buffer)
            return ptr_win;
    }
    
    /* no window found */
    return NULL;
}

/*
 * gui_buffer_get_dcc: get pointer to DCC buffer (DCC buffer created if not existing)
 */

t_gui_buffer *
gui_buffer_get_dcc (t_gui_window *window)
{
    t_gui_buffer *ptr_buffer;
    
    /* check if dcc buffer exists */
    for (ptr_buffer = gui_buffers; ptr_buffer; ptr_buffer = ptr_buffer->next_buffer)
    {
        if (ptr_buffer->type == BUFFER_TYPE_DCC)
            break;
    }
    if (ptr_buffer)
        return ptr_buffer;
    else
        return gui_buffer_new (window, NULL, NULL, BUFFER_TYPE_DCC, 0);
}

/*
 * gui_buffer_clear: clear buffer content
 */

void
gui_buffer_clear (t_gui_buffer *buffer)
{
    t_gui_window *ptr_win;
    t_gui_line *ptr_line;
    
    /* remove buffer from hotlist */
    hotlist_remove_buffer (buffer);
    
    /* remove lines from buffer */
    while (buffer->lines)
    {
        ptr_line = buffer->lines->next_line;
        if (buffer->lines->data)
            free (buffer->lines->data);
        free (buffer->lines);
        buffer->lines = ptr_line;
    }
    
    buffer->lines = NULL;
    buffer->last_line = NULL;
    buffer->num_lines = 0;
    buffer->line_complete = 1;

    /* remove any scroll for buffer */
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        if (ptr_win->buffer == buffer)
        {
            ptr_win->first_line_displayed = 1;
            ptr_win->start_line = NULL;
            ptr_win->start_line_pos = 0;
        }
    }
    
    gui_chat_draw (buffer, 1);
    gui_status_draw (buffer, 1);
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
 * gui_buffer_line_free: delete a line from a buffer
 */

void
gui_buffer_line_free (t_gui_line *line)
{
    t_gui_window *ptr_win;
    
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        if (ptr_win->start_line == line)
        {
            ptr_win->start_line = ptr_win->start_line->next_line;
            ptr_win->start_line_pos = 0;
            gui_chat_draw (ptr_win->buffer, 0);
            gui_status_draw (ptr_win->buffer, 0);
        }
    }
    if (line->nick)
        free (line->nick);
    if (line->data)
        free (line->data);
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
    t_irc_server *ptr_server;
    int create_new;
    
    create_new = (buffer->server || buffer->channel);
    
    hotlist_remove_buffer (buffer);
    if (hotlist_initial_buffer == buffer)
        hotlist_initial_buffer = NULL;
    
    if (gui_buffer_before_dcc == buffer)
        gui_buffer_before_dcc = NULL;
    
    if (gui_buffer_before_raw_data == buffer)
        gui_buffer_before_raw_data = NULL;
    
    if (buffer->type == BUFFER_TYPE_RAW_DATA)
        gui_buffer_raw_data = NULL;
    
    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        if (ptr_server->saved_buffer == buffer)
            ptr_server->saved_buffer = NULL;
    }
    
    if (switch_to_another)
    {
        for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
        {
            if ((buffer == ptr_win->buffer) &&
                ((buffer->next_buffer) || (buffer->prev_buffer)))
                gui_buffer_switch_previous (ptr_win);
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
        gui_buffer_line_free (buffer->lines);
        buffer->lines = ptr_line;
    }
    
    /* close log if opened */
    if (buffer->log_file)
        gui_log_end (buffer);
    
    if (buffer->input_buffer)
        free (buffer->input_buffer);
    if (buffer->input_buffer_color_mask)
        free (buffer->input_buffer_color_mask);
    
    completion_free (&(buffer->completion));
    
    history_buffer_free (buffer);
    
    /* remove buffer from buffers list */
    if (buffer->prev_buffer)
        buffer->prev_buffer->next_buffer = buffer->next_buffer;
    if (buffer->next_buffer)
        buffer->next_buffer->prev_buffer = buffer->prev_buffer;
    if (gui_buffers == buffer)
        gui_buffers = buffer->next_buffer;
    if (last_gui_buffer == buffer)
        last_gui_buffer = buffer->prev_buffer;
    
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        if (ptr_win->buffer == buffer)
            ptr_win->buffer = NULL;
    }
    
    free (buffer);
    
    /* always at least one buffer */
    if (!gui_buffers && create_new && switch_to_another)
        (void) gui_buffer_new (gui_windows, NULL, NULL,
                               BUFFER_TYPE_STANDARD, 1);
}

/*
 * gui_buffer_line_new: create new line for a buffer
 */

t_gui_line *
gui_buffer_line_new (t_gui_buffer *buffer)
{
    t_gui_line *new_line, *ptr_line;
    
    if ((new_line = (t_gui_line *) malloc (sizeof (struct t_gui_line))))
    {
        new_line->length = 0;
        new_line->length_align = 0;
        new_line->log_write = 1;
        new_line->line_with_message = 0;
        new_line->line_with_highlight = 0;
        new_line->nick = NULL;
        new_line->data = NULL;
        new_line->ofs_after_date = -1;
        new_line->ofs_start_message = -1;
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
        weechat_log_printf (_("Not enough memory for new line\n"));
        return NULL;
    }
    
    /* remove one line if necessary */
    if ((cfg_history_max_lines > 0)
        && (buffer->num_lines > cfg_history_max_lines))
    {
        if (buffer->last_line == buffer->lines)
            buffer->last_line = NULL;
        ptr_line = buffer->lines->next_line;
        gui_buffer_line_free (buffer->lines);
        buffer->lines = ptr_line;
        ptr_line->prev_line = NULL;
        buffer->num_lines--;
        gui_chat_draw (buffer, 1);
    }
    
    return new_line;
}

/*
 * gui_buffer_merge_servers: merge server buffers in one buffer
 */

void
gui_buffer_merge_servers (t_gui_window *window)
{
    t_gui_buffer *ptr_buffer_server, *ptr_buffer, *new_ptr_buffer;
    t_irc_server *ptr_server;
    
    /* new server buffer is the first server buffer found */
    for (ptr_buffer_server = gui_buffers; ptr_buffer_server;
         ptr_buffer_server = ptr_buffer_server->next_buffer)
    {
        if (BUFFER_IS_SERVER(ptr_buffer_server))
            break;
    }
    
    /* no server buffer found */
    if (!ptr_buffer_server)
        return;

    ptr_buffer = gui_buffers;
    while (ptr_buffer)
    {
        if ((ptr_buffer != ptr_buffer_server)
            && (BUFFER_IS_SERVER(ptr_buffer)))
        {
            ptr_server = SERVER(ptr_buffer);
            
            /* add (by pointer artefact) lines from buffer found to server buffer */
            if (ptr_buffer->lines)
            {
                if (ptr_buffer_server->lines)
                {
                    ptr_buffer->lines->prev_line =
                        ptr_buffer_server->last_line;
                    ptr_buffer_server->last_line->next_line =
                        ptr_buffer->lines;
                    ptr_buffer_server->last_line =
                        ptr_buffer->last_line;
                }
                else
                {
                    ptr_buffer_server->lines = ptr_buffer->lines;
                    ptr_buffer_server->last_line = ptr_buffer->last_line;
                }
            }
            
            /* free buffer but not lines, because they're now used by 
               our unique server buffer */
            new_ptr_buffer = ptr_buffer->next_buffer;
            ptr_buffer->lines = NULL;
            gui_buffer_free (ptr_buffer, 1);
            ptr_buffer = new_ptr_buffer;
            
            /* asociate server with new server buffer */
            ptr_server->buffer = ptr_buffer_server;
        }
        else
            ptr_buffer = ptr_buffer->next_buffer;
    }
    
    ptr_buffer_server->all_servers = 1;
    gui_window_redraw_buffer (window->buffer);
}

/*
 * gui_buffer_split_server: split the server buffer into many buffers (one by server)
 */

void
gui_buffer_split_server (t_gui_window *window)
{
    t_gui_buffer *ptr_buffer;
    t_irc_server *ptr_server;
    
    ptr_buffer = gui_buffer_servers_search ();
    
    if (ptr_buffer)
    {
        if (SERVER(ptr_buffer))
        {
            for (ptr_server = irc_servers; ptr_server;
                 ptr_server = ptr_server->next_server)
            {
                if (ptr_server->buffer
                    && (ptr_server != SERVER(ptr_buffer))
                    && (ptr_server->buffer == ptr_buffer))
                {
                    ptr_server->buffer = NULL;
                    gui_buffer_new (window, ptr_server, NULL,
                                    BUFFER_TYPE_STANDARD, 0);
                }
            }
        }
        ptr_buffer->all_servers = 0;
        gui_status_draw (window->buffer, 1);
        gui_input_draw (window->buffer, 1);
    }
}

/*
 * gui_buffer_switch_previous: switch to previous buffer
 */

void
gui_buffer_switch_previous (t_gui_window *window)
{
    if (!gui_ok)
        return;
    
    /* if only one buffer then return */
    if (gui_buffers == last_gui_buffer)
        return;
    
    if (window->buffer->prev_buffer)
        gui_window_switch_to_buffer (window, window->buffer->prev_buffer);
    else
        gui_window_switch_to_buffer (window, last_gui_buffer);
    
    gui_window_redraw_buffer (window->buffer);
}

/*
 * gui_buffer_switch_next: switch to next buffer
 */

void
gui_buffer_switch_next (t_gui_window *window)
{
    if (!gui_ok)
        return;
    
    /* if only one buffer then return */
    if (gui_buffers == last_gui_buffer)
        return;
    
    if (window->buffer->next_buffer)
        gui_window_switch_to_buffer (window, window->buffer->next_buffer);
    else
        gui_window_switch_to_buffer (window, gui_buffers);
    
    gui_window_redraw_buffer (window->buffer);
}

/*
 * gui_buffer_switch_dcc: switch to dcc buffer (create it if it does not exist)
 */

void
gui_buffer_switch_dcc (t_gui_window *window)
{
    t_gui_buffer *ptr_buffer;
    
    /* check if dcc buffer exists */
    for (ptr_buffer = gui_buffers; ptr_buffer; ptr_buffer = ptr_buffer->next_buffer)
    {
        if (ptr_buffer->type == BUFFER_TYPE_DCC)
            break;
    }
    if (ptr_buffer)
    {
        gui_window_switch_to_buffer (window, ptr_buffer);
        gui_window_redraw_buffer (ptr_buffer);
    }
    else
        gui_buffer_new (window, NULL, NULL, BUFFER_TYPE_DCC, 1);
}

/*
 * gui_buffer_switch_raw_data: switch to rax IRC data buffer (create it if it does not exist)
 */

void
gui_buffer_switch_raw_data (t_gui_window *window)
{
    t_gui_buffer *ptr_buffer;
    
    /* check if raw IRC data buffer exists */
    for (ptr_buffer = gui_buffers; ptr_buffer; ptr_buffer = ptr_buffer->next_buffer)
    {
        if (ptr_buffer->type == BUFFER_TYPE_RAW_DATA)
            break;
    }
    if (ptr_buffer)
    {
        gui_window_switch_to_buffer (window, ptr_buffer);
        gui_window_redraw_buffer (ptr_buffer);
    }
    else
        gui_buffer_new (window, NULL, NULL, BUFFER_TYPE_RAW_DATA, 1);
}

/*
 * gui_buffer_switch_by_number: switch to another buffer with number
 */

t_gui_buffer *
gui_buffer_switch_by_number (t_gui_window *window, int number)
{
    t_gui_buffer *ptr_buffer;
    
    /* invalid buffer */
    if (number < 0)
        return NULL;
    
    /* buffer is currently displayed ? */
    if (number == window->buffer->number)
        return window->buffer;
    
    /* search for buffer in the list */
    for (ptr_buffer = gui_buffers; ptr_buffer; ptr_buffer = ptr_buffer->next_buffer)
    {
        if ((ptr_buffer != window->buffer) && (number == ptr_buffer->number))
        {
            gui_window_switch_to_buffer (window, ptr_buffer);
            gui_window_redraw_buffer (window->buffer);
            return ptr_buffer;
        }
    }
    
    /* buffer not found */
    return NULL;
}

/*
 * gui_buffer_move_to_number: move a buffer to another number
 */

void
gui_buffer_move_to_number (t_gui_buffer *buffer, int number)
{
    t_gui_buffer *ptr_buffer;
    int i;
    
    /* if only one buffer then return */
    if (gui_buffers == last_gui_buffer)
        return;
    
    /* buffer number is already ok ? */
    if (number == buffer->number)
        return;
    
    if (number < 1)
        number = 1;
    
    /* remove buffer from list */
    if (buffer == gui_buffers)
    {
        gui_buffers = buffer->next_buffer;
        gui_buffers->prev_buffer = NULL;
    }
    if (buffer == last_gui_buffer)
    {
        last_gui_buffer = buffer->prev_buffer;
        last_gui_buffer->next_buffer = NULL;
    }
    if (buffer->prev_buffer)
        (buffer->prev_buffer)->next_buffer = buffer->next_buffer;
    if (buffer->next_buffer)
        (buffer->next_buffer)->prev_buffer = buffer->prev_buffer;
    
    if (number == 1)
    {
        gui_buffers->prev_buffer = buffer;
        buffer->prev_buffer = NULL;
        buffer->next_buffer = gui_buffers;
        gui_buffers = buffer;
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
            buffer->prev_buffer = ptr_buffer->prev_buffer;
            buffer->next_buffer = ptr_buffer;
            if (ptr_buffer->prev_buffer)
                (ptr_buffer->prev_buffer)->next_buffer = buffer;
            ptr_buffer->prev_buffer = buffer;
        }
        else
        {
            /* number not found (too big)? => add to end */
            buffer->prev_buffer = last_gui_buffer;
            buffer->next_buffer = NULL;
            last_gui_buffer->next_buffer = buffer;
            last_gui_buffer = buffer;
        }
        
    }
    
    /* assign new number to all buffers */
    i = 1;
    for (ptr_buffer = gui_buffers; ptr_buffer; ptr_buffer = ptr_buffer->next_buffer)
    {
        ptr_buffer->number = i++;
    }
    
    gui_window_redraw_buffer (buffer);
}

/*
 * gui_buffer_print_log: print buffer infos in log (usually for crash dump)
 */

void
gui_buffer_print_log (t_gui_buffer *buffer)
{
    t_gui_line *ptr_line;
    int num;
    
    weechat_log_printf ("[buffer (addr:0x%X)]\n", buffer);
    weechat_log_printf ("  num_displayed. . . . . : %d\n",   buffer->num_displayed);
    weechat_log_printf ("  number . . . . . . . . : %d\n",   buffer->number);
    weechat_log_printf ("  server . . . . . . . . : 0x%X\n", buffer->server);
    weechat_log_printf ("  all_servers. . . . . . : %d\n",   buffer->all_servers);
    weechat_log_printf ("  channel. . . . . . . . : 0x%X\n", buffer->channel);
    weechat_log_printf ("  type . . . . . . . . . : %d\n",   buffer->type);
    weechat_log_printf ("  lines. . . . . . . . . : 0x%X\n", buffer->lines);
    weechat_log_printf ("  last_line. . . . . . . : 0x%X\n", buffer->last_line);
    weechat_log_printf ("  last_read_line . . . . : 0x%X\n", buffer->last_read_line);
    weechat_log_printf ("  num_lines. . . . . . . : %d\n",   buffer->num_lines);
    weechat_log_printf ("  line_complete. . . . . : %d\n",   buffer->line_complete);
    weechat_log_printf ("  notify_level . . . . . : %d\n",   buffer->notify_level);
    weechat_log_printf ("  log_filename . . . . . : '%s'\n", buffer->log_filename);
    weechat_log_printf ("  log_file . . . . . . . : 0x%X\n", buffer->log_file);
    weechat_log_printf ("  has_input. . . . . . . : %d\n",   buffer->has_input);
    weechat_log_printf ("  input_buffer . . . . . : '%s'\n", buffer->input_buffer);
    weechat_log_printf ("  input_buffer_color_mask: '%s'\n", buffer->input_buffer_color_mask);
    weechat_log_printf ("  input_buffer_alloc . . : %d\n",   buffer->input_buffer_alloc);
    weechat_log_printf ("  input_buffer_size. . . : %d\n",   buffer->input_buffer_size);
    weechat_log_printf ("  input_buffer_length. . : %d\n",   buffer->input_buffer_length);
    weechat_log_printf ("  input_buffer_pos . . . : %d\n",   buffer->input_buffer_pos);
    weechat_log_printf ("  input_buffer_1st_disp. : %d\n",   buffer->input_buffer_1st_display);
    weechat_log_printf ("  completion . . . . . . : 0x%X\n", &(buffer->completion));
    weechat_log_printf ("  history. . . . . . . . : 0x%X\n", buffer->history);
    weechat_log_printf ("  last_history . . . . . : 0x%X\n", buffer->last_history);
    weechat_log_printf ("  ptr_history. . . . . . : 0x%X\n", buffer->ptr_history);
    weechat_log_printf ("  prev_buffer. . . . . . : 0x%X\n", buffer->prev_buffer);
    weechat_log_printf ("  next_buffer. . . . . . : 0x%X\n", buffer->next_buffer);
    weechat_log_printf ("\n");
    weechat_log_printf ("  => last 100 lines:\n");
    
    num = 0;
    ptr_line = buffer->last_line;
    while (ptr_line && (num < 100))
    {
        num++;
        ptr_line = ptr_line->prev_line;
    }
    if (!ptr_line)
        ptr_line = buffer->lines;
    else
        ptr_line = ptr_line->next_line;
    
    while (ptr_line)
    {
        num--;
        weechat_log_printf ("       line N-%05d: %s\n",
                            num,
                            (ptr_line->data) ? ptr_line->data : "(empty)");
        
        ptr_line = ptr_line->next_line;
    }
    
    weechat_log_printf ("\n");
    completion_print_log (&(buffer->completion));
}
