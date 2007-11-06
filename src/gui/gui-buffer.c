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

/* gui-buffer.c: buffer functions, used by all GUI */


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

#include "../core/weechat.h"
#include "gui-completion.h"
#include "gui-history.h"
#include "gui-hotlist.h"
#include "gui-input.h"
#include "gui-main.h"
#include "gui-nicklist.h"
#include "gui-log.h"
#include "gui-status.h"
#include "gui-window.h"
#include "../core/wee-command.h"
#include "../core/wee-config.h"
#include "../core/wee-log.h"
#include "../core/wee-string.h"
#include "../core/wee-utf8.h"
#include "../plugins/plugin.h"


struct t_gui_buffer *gui_buffers = NULL;           /* first buffer          */
struct t_gui_buffer *last_gui_buffer = NULL;       /* last buffer           */
struct t_gui_buffer *gui_previous_buffer = NULL;   /* previous buffer       */
struct t_gui_buffer *gui_buffer_before_dcc = NULL; /* buffer before dcc     */
struct t_gui_buffer *gui_buffer_raw_data = NULL;   /* buffer with raw data  */
struct t_gui_buffer *gui_buffer_before_raw_data = NULL; /* buf. before raw  */


/*
 * gui_buffer_new: create a new buffer in current window
 */

struct t_gui_buffer *
gui_buffer_new (void *plugin, char *category, char *name)
{
    struct t_gui_buffer *new_buffer;
    struct t_gui_completion *new_completion;
    char buffer_str[16], *argv[1] = { NULL };
    
#ifdef DEBUG
    weechat_log_printf ("Creating new buffer\n");
#endif
    
    if (!category || !name)
        return NULL;
    
    /* create new buffer */
    if ((new_buffer = (struct t_gui_buffer *)(malloc (sizeof (struct t_gui_buffer)))))
    {
        /* init buffer */
        new_buffer->plugin = (struct t_weechat_plugin *)plugin;
        new_buffer->number = (last_gui_buffer) ? last_gui_buffer->number + 1 : 1;
        new_buffer->category = (category) ? strdup (category) : NULL;
        new_buffer->name = strdup (name);
        new_buffer->type = GUI_BUFFER_TYPE_FORMATED;
        new_buffer->notify_level = GUI_BUFFER_NOTIFY_LEVEL_DEFAULT;
        new_buffer->num_displayed = 0;
        
        /* create/append to log file */
        new_buffer->log_filename = NULL;
        new_buffer->log_file = NULL;
        
        /* title */
        new_buffer->title = NULL;
        
        /* chat lines */
        new_buffer->lines = NULL;
        new_buffer->last_line = NULL;
        new_buffer->last_read_line = NULL;
        new_buffer->lines_count = 0;
        new_buffer->prefix_max_length = 0;
        new_buffer->chat_refresh_needed = 1;
        
        /* nicklist */
        new_buffer->nicklist = 0;
        new_buffer->nick_case_sensitive = 0;
        new_buffer->nicks = NULL;
        new_buffer->last_nick = NULL;
        new_buffer->nick_max_length = -1;
        new_buffer->nicks_count = 0;
        
        /* input */
        new_buffer->input = 1;
        new_buffer->input_data_cb = NULL;
        new_buffer->input_nick = NULL;
        new_buffer->input_buffer_alloc = GUI_BUFFER_INPUT_BLOCK_SIZE;
        new_buffer->input_buffer = (char *) malloc (GUI_BUFFER_INPUT_BLOCK_SIZE);
        new_buffer->input_buffer_color_mask = (char *) malloc (GUI_BUFFER_INPUT_BLOCK_SIZE);
        new_buffer->input_buffer[0] = '\0';
        new_buffer->input_buffer_color_mask[0] = '\0';
        new_buffer->input_buffer_size = 0;
        new_buffer->input_buffer_length = 0;
        new_buffer->input_buffer_pos = 0;
        new_buffer->input_buffer_1st_display = 0;
        
        /* init completion */
        new_completion = (struct t_gui_completion *)malloc (sizeof (struct t_gui_completion));
        if (new_completion)
        {
            new_buffer->completion = new_completion;
            gui_completion_init (new_completion, new_buffer);
        }
        
        /* init history */
        new_buffer->history = NULL;
        new_buffer->last_history = NULL;
        new_buffer->ptr_history = NULL;
        new_buffer->num_history = 0;
        
        /* text search */
        new_buffer->text_search = GUI_TEXT_SEARCH_DISABLED;
        new_buffer->text_search_exact = 0;
        new_buffer->text_search_found = 0;
        new_buffer->text_search_input = NULL;
        
        /* add buffer to buffers list */
        new_buffer->prev_buffer = last_gui_buffer;
        if (gui_buffers)
            last_gui_buffer->next_buffer = new_buffer;
        else
            gui_buffers = new_buffer;
        last_gui_buffer = new_buffer;
        new_buffer->next_buffer = NULL;
        
        /* first buffer creation ? */
        if (!gui_current_window->buffer)
        {
            gui_current_window->buffer = new_buffer;
            gui_current_window->first_line_displayed = 1;
            gui_current_window->start_line = NULL;
            gui_current_window->start_line_pos = 0;
            gui_window_calculate_pos_size (gui_current_window, 1);
            gui_window_switch_to_buffer (gui_current_window, new_buffer);
            gui_window_redraw_buffer (new_buffer);
        }
        
        snprintf (buffer_str, sizeof (buffer_str) - 1, "%d", new_buffer->number);
        argv[0] = buffer_str;
        /* TODO: send buffer_open event */
        /*(void) plugin_event_handler_exec ("buffer_open", 1, argv);*/
    }
    else
        return NULL;
    
    return new_buffer;
}

/*
 * buffer_valid: check if a buffer pointer exists
 *               return 1 if buffer exists
 *                      0 if buffer is not found
 */

int
gui_buffer_valid (struct t_gui_buffer *buffer)
{
    struct t_gui_buffer *ptr_buffer;
    
    /* NULL buffer is valid (it's for printing on first buffer) */
    if (!buffer)
        return 1;
    
    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        if (ptr_buffer == buffer)
            return 1;
    }
    
    /* buffer not found */
    return 0;
}

/*
 * gui_buffer_set_category: set category for a buffer
 */

void
gui_buffer_set_category (struct t_gui_buffer *buffer, char *category)
{
    if (category && category[0])
    {
        if (buffer->category)
            free (buffer->category);
        buffer->category = strdup (category);
    }
}

/*
 * gui_buffer_set_name: set name for a buffer
 */

void
gui_buffer_set_name (struct t_gui_buffer *buffer, char *name)
{
    if (name && name[0])
    {
        if (buffer->name)
            free (buffer->name);
        buffer->name = strdup (name);
    }
}

/*
 * gui_buffer_set_log: set log file for a buffer
 */

void
gui_buffer_set_log (struct t_gui_buffer *buffer, char *log_filename)
{
    if (buffer->log_file)
        gui_log_end (buffer);

    if (log_filename)
    {
        buffer->log_filename = strdup (log_filename);
        gui_log_start (buffer);
    }
}

/*
 * gui_buffer_set_title: set title for a buffer
 */

void
gui_buffer_set_title (struct t_gui_buffer *buffer, char *new_title)
{
    if (buffer->title)
        free (buffer->title);
    buffer->title = (new_title && new_title[0]) ? strdup (new_title) : NULL;
}

/*
 * gui_buffer_set_nicklist: set nicklist for a buffer
 */

void
gui_buffer_set_nicklist (struct t_gui_buffer *buffer, int nicklist)
{
    buffer->nicklist = (nicklist) ? 1 : 0;
}

/*
 * gui_buffer_set_nick_case_sensitive: set nick_case_sensitive flag for a buffer
 */

void
gui_buffer_set_nick_case_sensitive (struct t_gui_buffer *buffer,
                                    int nick_case_sensitive)
{
    buffer->nick_case_sensitive = (nick_case_sensitive) ? 1 : 0;
}

/*
 * gui_buffer_set_nick: set nick for a buffer
 */

void
gui_buffer_set_nick (struct t_gui_buffer *buffer, char *new_nick)
{
    if (buffer->input_nick)
        free (buffer->input_nick);
    buffer->input_nick = (new_nick && new_nick[0]) ? strdup (new_nick) : NULL;
}

/*
 * gui_buffer_set: set a property for a buffer
 */

void
gui_buffer_set (struct t_gui_buffer *buffer, char *property, char *value)
{
    long number;
    char *error;
    
    if (string_strcasecmp (property, "display") == 0)
    {
        gui_window_switch_to_buffer (gui_current_window, buffer);
        gui_window_redraw_buffer (buffer);
    }
    else if (string_strcasecmp (property, "category") == 0)
    {
        gui_buffer_set_category (buffer, value);
        gui_status_draw (buffer, 1);
    }
    else if (string_strcasecmp (property, "name") == 0)
    {
        gui_buffer_set_name (buffer, value);
        gui_status_draw (buffer, 1);
    }
    else if (string_strcasecmp (property, "log") == 0)
    {
        gui_buffer_set_log (buffer, value);
    }
    else if (string_strcasecmp (property, "title") == 0)
    {
        gui_buffer_set_title (buffer, value);
        gui_chat_draw_title (buffer, 1);
    }
    else if (string_strcasecmp (property, "nicklist") == 0)
    {
        error = NULL;
        number = strtol (value, &error, 10);
        if (error && (error[0] == '\0'))
        {
            gui_buffer_set_nicklist (buffer, number);
            gui_window_refresh_windows ();
        }
    }
    else if (string_strcasecmp (property, "nick_case_sensitive") == 0)
    {
        error = NULL;
        number = strtol (value, &error, 10);
        if (error && (error[0] == '\0'))
            gui_buffer_set_nick_case_sensitive (buffer, number);
    }
    else if (string_strcasecmp (property, "nick") == 0)
    {
        gui_buffer_set_nick (buffer, value);
        gui_input_draw (buffer, 1);
    }
}

/*
 * gui_buffer_search_main: get main buffer (weechat one, created at startup)
 *                         return first buffer if not found
 */

struct t_gui_buffer *
gui_buffer_search_main ()
{
    struct t_gui_buffer *ptr_buffer;
    
    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        if (!ptr_buffer->plugin)
            return ptr_buffer;
    }
    
    /* buffer not found, return first buffer by default */
    return gui_buffers;
}

/*
 * gui_buffer_search_by_category_name: search a buffer by category and/or name
 */

struct t_gui_buffer *
gui_buffer_search_by_category_name (char *category, char *name)
{
    struct t_gui_buffer *ptr_buffer;
    
    if (!category && !name)
        return gui_current_window->buffer;
    
    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        if ((category && ptr_buffer->category
             && strcmp (ptr_buffer->category, category) == 0)
            || (name && ptr_buffer->name
                && strcmp (ptr_buffer->name, name) == 0))
            return ptr_buffer;
    }
    
    /* buffer not found */
    return NULL;
}

/*
 * gui_buffer_search_by_number: search a buffer by number
 */

struct t_gui_buffer *
gui_buffer_search_by_number (int number)
{
    struct t_gui_buffer *ptr_buffer;
    
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

struct t_gui_window *
gui_buffer_find_window (struct t_gui_buffer *buffer)
{
    struct t_gui_window *ptr_win;
    
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
 * gui_buffer_is_scrolled: return 1 if all windows displaying buffer are scrolled
 *                         (user doesn't see end of buffer)
 *                         return 0 if at least one window is NOT scrolled
 */

int
gui_buffer_is_scrolled (struct t_gui_buffer *buffer)
{
    struct t_gui_window *ptr_win;
    int buffer_found;

    if (!buffer)
        return 0;
    
    buffer_found = 0;
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        if (ptr_win->buffer == buffer)
        {
            buffer_found = 1;
            /* buffer found and not scrolled, exit immediately */
            if (!ptr_win->scroll)
                return 0;
        }
    }
    
    /* buffer found, and all windows were scrolled */
    if (buffer_found)
        return 1;
    
    /* buffer not found */
    return 0;
}

/*
 * gui_buffer_get_dcc: get pointer to DCC buffer (DCC buffer created if not existing)
 */

struct t_gui_buffer *
gui_buffer_get_dcc (struct t_gui_window *window)
{
    //struct t_gui_buffer *ptr_buffer;

    (void) window;
    /* check if dcc buffer exists */
    /*for (ptr_buffer = gui_buffers; ptr_buffer; ptr_buffer = ptr_buffer->next_buffer)
    {
        if (ptr_buffer->type == GUI_BUFFER_TYPE_DCC)
            break;
    }
    if (ptr_buffer)
        return ptr_buffer;
    else
        return gui_buffer_new (window, weechat_protocols,
        NULL, NULL, GUI_BUFFER_TYPE_DCC, 0);*/
    return NULL;
}

/*
 * gui_buffer_clear: clear buffer content
 */

void
gui_buffer_clear (struct t_gui_buffer *buffer)
{
    struct t_gui_window *ptr_win;
    struct t_gui_line *ptr_line;

    if (!buffer)
        return;

    if (buffer->type == GUI_BUFFER_TYPE_FREE)
    {
        /* TODO: clear buffer with free content */
        return;
    }
    
    /* remove buffer from hotlist */
    gui_hotlist_remove_buffer (buffer);
    
    /* remove lines from buffer */
    while (buffer->lines)
    {
        ptr_line = buffer->lines->next_line;
        if (buffer->lines->message)
            free (buffer->lines->message);
        free (buffer->lines);
        buffer->lines = ptr_line;
    }
    
    buffer->lines = NULL;
    buffer->last_line = NULL;
    buffer->lines_count = 0;
    
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
    struct t_gui_buffer *ptr_buffer;
    
    for (ptr_buffer = gui_buffers; ptr_buffer; ptr_buffer = ptr_buffer->next_buffer)
        gui_buffer_clear (ptr_buffer);
}

/*
 * gui_buffer_free: delete a buffer
 */

void
gui_buffer_free (struct t_gui_buffer *buffer, int switch_to_another)
{
    struct t_gui_window *ptr_window;
    struct t_gui_buffer *ptr_buffer;
    struct t_gui_line *ptr_line;
    char buffer_str[16], *argv[1] = { NULL };
    
    snprintf (buffer_str, sizeof (buffer_str) - 1, "%d", buffer->number);
    argv[0] = buffer_str;
    /* TODO: send buffer_close event */
    /*(void) plugin_event_handler_exec ("buffer_close", 1, argv);*/
    
    if (switch_to_another)
    {
        for (ptr_window = gui_windows; ptr_window;
             ptr_window = ptr_window->next_window)
        {
            if ((buffer == ptr_window->buffer) &&
                ((buffer->next_buffer) || (buffer->prev_buffer)))
                gui_buffer_switch_previous (ptr_window);
        }
    }
    
    gui_hotlist_remove_buffer (buffer);
    if (gui_hotlist_initial_buffer == buffer)
        gui_hotlist_initial_buffer = NULL;
    
    if (gui_previous_buffer == buffer)
        gui_previous_buffer = NULL;
    
    if (gui_buffer_before_dcc == buffer)
        gui_buffer_before_dcc = NULL;
    
    if (gui_buffer_before_raw_data == buffer)
        gui_buffer_before_raw_data = NULL;
    
    if (buffer->type == GUI_BUFFER_TYPE_FREE)
    {
        // TODO: review this
        gui_buffer_raw_data = NULL;
    }
    
    if (buffer->type == GUI_BUFFER_TYPE_FORMATED)
    {
        /* decrease buffer number for all next buffers */
        for (ptr_buffer = buffer->next_buffer; ptr_buffer;
             ptr_buffer = ptr_buffer->next_buffer)
        {
            ptr_buffer->number--;
        }
        
        /* free lines and messages */
        while (buffer->lines)
        {
            ptr_line = buffer->lines->next_line;
            gui_chat_line_free (buffer->lines);
            buffer->lines = ptr_line;
        }
        
        /* close log if opened */
        if (buffer->log_file)
            gui_log_end (buffer);
    }

    if (buffer->input_buffer)
        free (buffer->input_buffer);
    if (buffer->input_buffer_color_mask)
        free (buffer->input_buffer_color_mask);
    if (buffer->completion)
        gui_completion_free (buffer->completion);
    gui_history_buffer_free (buffer);
    if (buffer->text_search_input)
        free (buffer->text_search_input);
    
    /* remove buffer from buffers list */
    if (buffer->prev_buffer)
        buffer->prev_buffer->next_buffer = buffer->next_buffer;
    if (buffer->next_buffer)
        buffer->next_buffer->prev_buffer = buffer->prev_buffer;
    if (gui_buffers == buffer)
        gui_buffers = buffer->next_buffer;
    if (last_gui_buffer == buffer)
        last_gui_buffer = buffer->prev_buffer;
    
    for (ptr_window = gui_windows; ptr_window;
         ptr_window = ptr_window->next_window)
    {
        if (ptr_window->buffer == buffer)
            ptr_window->buffer = NULL;
    }
    
    free (buffer);
    
    if (gui_windows && gui_current_window && gui_current_window->buffer)
        gui_status_draw (gui_current_window->buffer, 1);
}

/*
 * gui_buffer_switch_previous: switch to previous buffer
 */

void
gui_buffer_switch_previous (struct t_gui_window *window)
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
gui_buffer_switch_next (struct t_gui_window *window)
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
gui_buffer_switch_dcc (struct t_gui_window *window)
{
    //struct t_gui_buffer *ptr_buffer;

    (void) window;
    /* check if dcc buffer exists */
    /*for (ptr_buffer = gui_buffers; ptr_buffer; ptr_buffer = ptr_buffer->next_buffer)
    {
        if (ptr_buffer->type == GUI_BUFFER_TYPE_DCC)
            break;
    }
    if (ptr_buffer)
    {
        gui_window_switch_to_buffer (window, ptr_buffer);
        gui_window_redraw_buffer (ptr_buffer);
    }
    else
        gui_buffer_new (window, weechat_protocols,
        NULL, NULL, GUI_BUFFER_TYPE_DCC, 1);*/
}

/*
 * gui_buffer_switch_raw_data: switch to rax IRC data buffer (create it if it does not exist)
 */

void
gui_buffer_switch_raw_data (struct t_gui_window *window)
{
    //struct t_gui_buffer *ptr_buffer;

    (void) window;
    /* check if raw IRC data buffer exists */
    /*for (ptr_buffer = gui_buffers; ptr_buffer; ptr_buffer = ptr_buffer->next_buffer)
    {
        if (ptr_buffer->type == GUI_BUFFER_TYPE_RAW_DATA)
            break;
    }
    if (ptr_buffer)
    {
        gui_window_switch_to_buffer (window, ptr_buffer);
        gui_window_redraw_buffer (ptr_buffer);
    }
    else
        gui_buffer_new (window, weechat_protocols,
        NULL, NULL, GUI_BUFFER_TYPE_RAW_DATA, 1);*/
}

/*
 * gui_buffer_switch_by_number: switch to another buffer with number
 */

struct t_gui_buffer *
gui_buffer_switch_by_number (struct t_gui_window *window, int number)
{
    struct t_gui_buffer *ptr_buffer;
    
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
gui_buffer_move_to_number (struct t_gui_buffer *buffer, int number)
{
    struct t_gui_buffer *ptr_buffer;
    int i;
    char buf1_str[16], buf2_str[16], *argv[2] = { NULL, NULL };
    
    /* if only one buffer then return */
    if (gui_buffers == last_gui_buffer)
        return;
    
    /* buffer number is already ok ? */
    if (number == buffer->number)
        return;
    
    if (number < 1)
        number = 1;
    
    snprintf (buf2_str, sizeof (buf2_str) - 1, "%d", buffer->number);
    
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
    
    snprintf (buf1_str, sizeof (buf1_str) - 1, "%d", buffer->number);
    argv[0] = buf1_str;
    argv[1] = buf2_str;
    /* TODO: send buffer_move event */
    /*(void) plugin_event_handler_exec ("buffer_move", 2, argv);*/
}

/*
 * gui_buffer_dump_hexa: dump content of buffer as hexa data in log file
 */

void
gui_buffer_dump_hexa (struct t_gui_buffer *buffer)
{
    struct t_gui_line *ptr_line;
    int num_line, msg_pos;
    char *message_without_colors;
    char hexa[(16 * 3) + 1], ascii[(16 * 2) + 1];
    int hexa_pos, ascii_pos;
    
    weechat_log_printf ("[buffer dump hexa (addr:0x%X)]\n", buffer);
    num_line = 1;
    for (ptr_line = buffer->lines; ptr_line; ptr_line = ptr_line->next_line)
    {
        /* display line without colors */
        message_without_colors = (ptr_line->message) ?
            (char *)gui_color_decode ((unsigned char *)ptr_line->message) : NULL;
        weechat_log_printf ("\n");
        weechat_log_printf ("  line %d: %s\n",
                            num_line,
                            (message_without_colors) ?
                            message_without_colors : "(null)");
        if (message_without_colors)
            free (message_without_colors);

        /* display raw message for line */
        if (ptr_line->message)
        {
            weechat_log_printf ("\n");
            weechat_log_printf ("  raw data for line %d (with color codes):\n",
                                num_line);
            msg_pos = 0;
            hexa_pos = 0;
            ascii_pos = 0;
            while (ptr_line->message[msg_pos])
            {
                snprintf (hexa + hexa_pos, 4, "%02X ",
                          (unsigned char)(ptr_line->message[msg_pos]));
                hexa_pos += 3;
                snprintf (ascii + ascii_pos, 3, "%c ",
                          ((((unsigned char)ptr_line->message[msg_pos]) < 32)
                           || (((unsigned char)ptr_line->message[msg_pos]) > 127)) ?
                          '.' : (unsigned char)(ptr_line->message[msg_pos]));
                ascii_pos += 2;
                if (ascii_pos == 32)
                {
                    weechat_log_printf ("    %-48s  %s\n", hexa, ascii);
                    hexa_pos = 0;
                    ascii_pos = 0;
                }
                msg_pos++;
            }
            if (ascii_pos > 0)
                weechat_log_printf ("    %-48s  %s\n", hexa, ascii);
        }
        
        num_line++;
    }
}

/*
 * gui_buffer_print_log: print buffer infos in log (usually for crash dump)
 */

void
gui_buffer_print_log ()
{
    struct t_gui_buffer *ptr_buffer;
    struct t_gui_nick *ptr_nick;
    struct t_gui_line *ptr_line;
    int num;
    
    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        weechat_log_printf ("\n");
        weechat_log_printf ("[buffer (addr:0x%X)]\n", ptr_buffer);
        weechat_log_printf ("  plugin . . . . . . . . : 0x%X\n", ptr_buffer->plugin);
        weechat_log_printf ("  number . . . . . . . . : %d\n",   ptr_buffer->number);
        weechat_log_printf ("  category . . . . . . . : '%s'\n", ptr_buffer->category);
        weechat_log_printf ("  name . . . . . . . . . : '%s'\n", ptr_buffer->name);
        weechat_log_printf ("  type . . . . . . . . . : %d\n",   ptr_buffer->type);
        weechat_log_printf ("  notify_level . . . . . : %d\n",   ptr_buffer->notify_level);
        weechat_log_printf ("  num_displayed. . . . . : %d\n",   ptr_buffer->num_displayed);
        weechat_log_printf ("  log_filename . . . . . : '%s'\n", ptr_buffer->log_filename);
        weechat_log_printf ("  log_file . . . . . . . : 0x%X\n", ptr_buffer->log_file);
        weechat_log_printf ("  title. . . . . . . . . : '%s'\n", ptr_buffer->title);
        weechat_log_printf ("  lines. . . . . . . . . : 0x%X\n", ptr_buffer->lines);
        weechat_log_printf ("  last_line. . . . . . . : 0x%X\n", ptr_buffer->last_line);
        weechat_log_printf ("  last_read_line . . . . : 0x%X\n", ptr_buffer->last_read_line);
        weechat_log_printf ("  lines_count. . . . . . : %d\n",   ptr_buffer->lines_count);
        weechat_log_printf ("  prefix_max_length. . . : %d\n",   ptr_buffer->prefix_max_length);
        weechat_log_printf ("  chat_refresh_needed. . : %d\n",   ptr_buffer->chat_refresh_needed);
        weechat_log_printf ("  nicklist . . . . . . . : %d\n",   ptr_buffer->nicklist);
        weechat_log_printf ("  nick_case_sensitive. . : %d\n",   ptr_buffer->nick_case_sensitive);
        weechat_log_printf ("  nicks. . . . . . . . . : 0x%X\n", ptr_buffer->nicks);
        weechat_log_printf ("  last_nick. . . . . . . : 0x%X\n", ptr_buffer->last_nick);
        weechat_log_printf ("  nicks_count. . . . . . : %d\n",   ptr_buffer->nicks_count);
        weechat_log_printf ("  input. . . . . . . . . : %d\n",   ptr_buffer->input);
        weechat_log_printf ("  input_data_cb. . . . . : 0x%X\n", ptr_buffer->input_data_cb);
        weechat_log_printf ("  input_nick . . . . . . : '%s'\n", ptr_buffer->input_nick);
        weechat_log_printf ("  input_buffer . . . . . : '%s'\n", ptr_buffer->input_buffer);
        weechat_log_printf ("  input_buffer_color_mask: '%s'\n", ptr_buffer->input_buffer_color_mask);
        weechat_log_printf ("  input_buffer_alloc . . : %d\n",   ptr_buffer->input_buffer_alloc);
        weechat_log_printf ("  input_buffer_size. . . : %d\n",   ptr_buffer->input_buffer_size);
        weechat_log_printf ("  input_buffer_length. . : %d\n",   ptr_buffer->input_buffer_length);
        weechat_log_printf ("  input_buffer_pos . . . : %d\n",   ptr_buffer->input_buffer_pos);
        weechat_log_printf ("  input_buffer_1st_disp. : %d\n",   ptr_buffer->input_buffer_1st_display);
        weechat_log_printf ("  completion . . . . . . : 0x%X\n", ptr_buffer->completion);
        weechat_log_printf ("  history. . . . . . . . : 0x%X\n", ptr_buffer->history);
        weechat_log_printf ("  last_history . . . . . : 0x%X\n", ptr_buffer->last_history);
        weechat_log_printf ("  ptr_history. . . . . . : 0x%X\n", ptr_buffer->ptr_history);
        weechat_log_printf ("  num_history. . . . . . : %d\n",   ptr_buffer->num_history);
        weechat_log_printf ("  text_search. . . . . . : %d\n",   ptr_buffer->text_search);
        weechat_log_printf ("  text_search_exact. . . : %d\n",   ptr_buffer->text_search_exact);
        weechat_log_printf ("  text_search_found. . . : %d\n",   ptr_buffer->text_search_found);
        weechat_log_printf ("  text_search_input. . . : '%s'\n", ptr_buffer->text_search_input);
        weechat_log_printf ("  prev_buffer. . . . . . : 0x%X\n", ptr_buffer->prev_buffer);
        weechat_log_printf ("  next_buffer. . . . . . : 0x%X\n", ptr_buffer->next_buffer);
        
        for (ptr_nick = ptr_buffer->nicks; ptr_nick;
             ptr_nick = ptr_nick->next_nick)
        {
            weechat_log_printf ("\n");
            weechat_log_printf ("  => nick %s (addr:0x%X):\n", ptr_nick->nick, ptr_nick);
            weechat_log_printf ("       sort_index. . . . . . : %d\n",   ptr_nick->sort_index);
            weechat_log_printf ("       color_nick. . . . . . : %d\n",   ptr_nick->color_nick);
            weechat_log_printf ("       prefix. . . . . . . . : '%c'\n", ptr_nick->prefix);
            weechat_log_printf ("       color_prefix. . . . . : %d\n",   ptr_nick->color_prefix);
            weechat_log_printf ("       prev_nick . . . . . . : 0x%X\n", ptr_nick->prev_nick);
            weechat_log_printf ("       next_nick . . . . . . : 0x%X\n", ptr_nick->next_nick);
        }
        
        weechat_log_printf ("\n");
        weechat_log_printf ("  => last 100 lines:\n");
        num = 0;
        ptr_line = ptr_buffer->last_line;
        while (ptr_line && (num < 100))
        {
            num++;
            ptr_line = ptr_line->prev_line;
        }
        if (!ptr_line)
            ptr_line = ptr_buffer->lines;
        else
            ptr_line = ptr_line->next_line;
        
        while (ptr_line)
        {
            num--;
            weechat_log_printf ("       line N-%05d: str_time:'%s', prefix:'%s'\n",
                                num, ptr_line->str_time, ptr_line->prefix);
            weechat_log_printf ("                     data: '%s'\n",
                                ptr_line->message);
            
            ptr_line = ptr_line->next_line;
        }
        
        if (ptr_buffer->completion)
        {
            weechat_log_printf ("\n");
            gui_completion_print_log (ptr_buffer->completion);
        }
    }
}
