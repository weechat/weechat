/*
 * Copyright (c) 2003-2008 by FlashCode <flashcode@flashtux.org>
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
#include "../core/wee-config.h"
#include "../core/wee-hook.h"
#include "../core/wee-log.h"
#include "../core/wee-string.h"
#include "../core/wee-utf8.h"
#include "../plugins/plugin.h"
#include "gui-buffer.h"
#include "gui-chat.h"
#include "gui-color.h"
#include "gui-completion.h"
#include "gui-history.h"
#include "gui-hotlist.h"
#include "gui-input.h"
#include "gui-main.h"
#include "gui-nicklist.h"
#include "gui-status.h"
#include "gui-window.h"


struct t_gui_buffer *gui_buffers = NULL;           /* first buffer          */
struct t_gui_buffer *last_gui_buffer = NULL;       /* last buffer           */
struct t_gui_buffer *gui_previous_buffer = NULL;   /* previous buffer       */


/*
 * gui_buffer_new: create a new buffer in current window
 */

struct t_gui_buffer *
gui_buffer_new (struct t_weechat_plugin *plugin, char *category, char *name,
                int (*input_callback)(void *data,
                                      struct t_gui_buffer *buffer,
                                      char *input_data),
                void *input_callback_data,
                int (*close_callback)(void *data,
                                      struct t_gui_buffer *buffer),
                void *close_callback_data)
{
    struct t_gui_buffer *new_buffer;
    struct t_gui_completion *new_completion;
    
    if (!category || !name)
        return NULL;
    
    if (gui_buffer_search_by_category_name (category, name))
    {
        gui_chat_printf (NULL,
                         _("%sError: a buffer with same name already exists "
                           "(%s / %s)"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         category, name);
        return NULL;
    }
    
    /* create new buffer */
    new_buffer = malloc (sizeof (*new_buffer));
    if (new_buffer)
    {
        /* init buffer */
        new_buffer->plugin = plugin;
        new_buffer->number = (last_gui_buffer) ? last_gui_buffer->number + 1 : 1;
        new_buffer->category = (category) ? strdup (category) : NULL;
        new_buffer->name = strdup (name);
        new_buffer->type = GUI_BUFFER_TYPE_FORMATED;
        new_buffer->notify_level = GUI_BUFFER_NOTIFY_LEVEL_DEFAULT;
        new_buffer->num_displayed = 0;
        
        /* close callback */
        new_buffer->close_callback = close_callback;
        new_buffer->close_callback_data = close_callback_data;
        
        /* title */
        new_buffer->title = NULL;
        new_buffer->title_refresh_needed = 1;
        
        /* chat lines (formated) */
        new_buffer->lines = NULL;
        new_buffer->last_line = NULL;
        new_buffer->last_read_line = NULL;
        new_buffer->lines_count = 0;
        new_buffer->lines_hidden = 0;
        new_buffer->prefix_max_length = 0;
        new_buffer->chat_refresh_needed = 2;
        
        /* nicklist */
        new_buffer->nicklist = 0;
        new_buffer->nicklist_case_sensitive = 0;
        new_buffer->nicklist_root = NULL;
        new_buffer->nicklist_max_length = -1;
        new_buffer->nicklist_display_groups = 1;
        new_buffer->nicklist_visible_count = 0;
        new_buffer->nicklist_refresh_needed = 1;
        gui_nicklist_add_group (new_buffer, NULL, "root", NULL, 0);
        
        /* input */
        new_buffer->input = 1;
        new_buffer->input_callback = input_callback;
        new_buffer->input_callback_data = input_callback_data;
        new_buffer->input_nick = NULL;
        new_buffer->input_buffer_alloc = GUI_BUFFER_INPUT_BLOCK_SIZE;
        new_buffer->input_buffer = malloc (GUI_BUFFER_INPUT_BLOCK_SIZE);
        new_buffer->input_buffer_color_mask = malloc (GUI_BUFFER_INPUT_BLOCK_SIZE);
        new_buffer->input_buffer[0] = '\0';
        new_buffer->input_buffer_color_mask[0] = '\0';
        new_buffer->input_buffer_size = 0;
        new_buffer->input_buffer_length = 0;
        new_buffer->input_buffer_pos = 0;
        new_buffer->input_buffer_1st_display = 0;
        new_buffer->input_refresh_needed = 1;
        
        /* init completion */
        new_completion = malloc (sizeof (*new_completion));
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
        
        hook_signal_send ("buffer_open",
                          WEECHAT_HOOK_SIGNAL_POINTER, new_buffer);
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
 * gui_buffer_get_string: get a buffer property as string
 */

char *
gui_buffer_get_string (struct t_gui_buffer *buffer, char *property)
{
    static char value[32];

    if (string_strcasecmp (property, "plugin") == 0)
        return (buffer->plugin) ? buffer->plugin->name : NULL;
    else if (string_strcasecmp (property, "category") == 0)
        return buffer->category;
    else if (string_strcasecmp (property, "name") == 0)
        return buffer->name;
    else if (string_strcasecmp (property, "title") == 0)
        return buffer->title;
    else if (string_strcasecmp (property, "nick") == 0)
        return buffer->input_nick;
    else if (string_strcasecmp (property, "lines_hidden") == 0)
    {
        snprintf (value, sizeof (value), "%d", buffer->lines_hidden);
        return value;
    }
    
    return NULL;
}

/*
 * gui_buffer_get_pointer: get a buffer property as pointer
 */

void *
gui_buffer_get_pointer (struct t_gui_buffer *buffer, char *property)
{
    if (string_strcasecmp (property, "plugin") == 0)
        return buffer->plugin;
    
    return NULL;
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
    gui_status_refresh_needed = 1;
    
    hook_signal_send ("buffer_renamed",
                      WEECHAT_HOOK_SIGNAL_POINTER, buffer);
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
        
        gui_status_refresh_needed = 1;
        
        hook_signal_send ("buffer_renamed",
                          WEECHAT_HOOK_SIGNAL_POINTER, buffer);
    }
}

/*
 * gui_buffer_set_type: set buffer type
 */

void
gui_buffer_set_type (struct t_gui_buffer *buffer, enum t_gui_buffer_type type)
{
    if (buffer->type == type)
        return;
    
    gui_chat_line_free_all (buffer);
    
    switch (type)
    {
        case GUI_BUFFER_TYPE_FORMATED:
            break;
        case GUI_BUFFER_TYPE_FREE:
            break;
    }
    buffer->type = type;
    buffer->chat_refresh_needed = 2;
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
    buffer->title_refresh_needed = 1;
}

/*
 * gui_buffer_set_nicklist: set nicklist for a buffer
 */

void
gui_buffer_set_nicklist (struct t_gui_buffer *buffer, int nicklist)
{
    buffer->nicklist = (nicklist) ? 1 : 0;
    gui_window_refresh_windows ();
}

/*
 * gui_buffer_set_nicklist_case_sensitive: set case_sensitive flag for a buffer
 */

void
gui_buffer_set_nicklist_case_sensitive (struct t_gui_buffer *buffer,
                                        int case_sensitive)
{
    buffer->nicklist_case_sensitive = (case_sensitive) ? 1 : 0;
}

/*
 * gui_buffer_set_nicklist_display_groups: set display_groups flag for a buffer
 */

void
gui_buffer_set_nicklist_display_groups (struct t_gui_buffer *buffer,
                                        int display_groups)
{
    buffer->nicklist_display_groups = (display_groups) ? 1 : 0;
    buffer->nicklist_visible_count = 0;
    gui_nicklist_compute_visible_count (buffer, buffer->nicklist_root);
    buffer->nicklist_refresh_needed = 1;
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
    buffer->input_refresh_needed = 1;
}

/*
 * gui_buffer_set: set a buffer property
 */

void
gui_buffer_set (struct t_gui_buffer *buffer, char *property, char *value)
{
    long number;
    char *error;

    if (!buffer || !property || !value)
        return;
    
    if (string_strcasecmp (property, "display") == 0)
    {
        gui_window_switch_to_buffer (gui_current_window, buffer);
        gui_window_redraw_buffer (buffer);
    }
    else if (string_strcasecmp (property, "category") == 0)
    {
        gui_buffer_set_category (buffer, value);
    }
    else if (string_strcasecmp (property, "name") == 0)
    {
        gui_buffer_set_name (buffer, value);
    }
    else if (string_strcasecmp (property, "type") == 0)
    {
        if (string_strcasecmp (value, "formated") == 0)
            gui_buffer_set_type (buffer, GUI_BUFFER_TYPE_FORMATED);
        else if (string_strcasecmp (value, "free") == 0)
            gui_buffer_set_type (buffer, GUI_BUFFER_TYPE_FREE);
    }
    else if (string_strcasecmp (property, "title") == 0)
    {
        gui_buffer_set_title (buffer, value);
    }
    else if (string_strcasecmp (property, "nicklist") == 0)
    {
        error = NULL;
        number = strtol (value, &error, 10);
        if (error && !error[0])
            gui_buffer_set_nicklist (buffer, number);
    }
    else if (string_strcasecmp (property, "nicklist_case_sensitive") == 0)
    {
        error = NULL;
        number = strtol (value, &error, 10);
        if (error && !error[0])
            gui_buffer_set_nicklist_case_sensitive (buffer, number);
    }
    else if (string_strcasecmp (property, "nicklist_display_groups") == 0)
    {
        error = NULL;
        number = strtol (value, &error, 10);
        if (error && !error[0])
            gui_buffer_set_nicklist_display_groups (buffer, number);
    }
    else if (string_strcasecmp (property, "nick") == 0)
    {
        gui_buffer_set_nick (buffer, value);
    }
    else if (string_strcasecmp (property, "hotlist") == 0)
    {
        if (strcmp (value, "-") == 0)
            gui_add_hotlist = 0;
        else if (strcmp (value, "+") == 0)
            gui_add_hotlist = 1;
        else
        {
            error = NULL;
            number = strtol (value, &error, 10);
            if (error && !error[0])
                gui_hotlist_add (buffer, number, NULL, 1);
        }
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
        if ((!category || (ptr_buffer->category
                           && (strcmp (ptr_buffer->category, category) == 0)))
            && (!name || (ptr_buffer->name
                          && (strcmp (ptr_buffer->name, name) == 0))))
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
 * gui_buffer_match_category_name: return 1 if buffer matches category.name
 *                                 otherwise 0
 *                                 category or name may begin or end with "*"
 *                                 examples:
 *                                     *.#weechat
 *                                     freenode.*
 *                                     freenode.#weechat*
 *                                     freenode.*chat*
 */

int
gui_buffer_match_category_name (struct t_gui_buffer *buffer, char *mask,
                                int case_sensitive)
{
    char *pos_point, *category, *pos_name;
    int rc;
    
    if (!mask || !mask[0])
        return 0;
    
    pos_point = strchr (mask, '.');
    if (pos_point)
    {
        category = string_strndup (mask, pos_point - mask);
        pos_name = pos_point + 1;
    }
    else
    {
        category = NULL;
        pos_name = mask;
    }
    
    rc = 1;
    
    if (category && buffer->category
        && !string_match (buffer->category, category, case_sensitive))
        rc = 0;
    
    if (rc && !string_match (buffer->name, pos_name, case_sensitive))
        rc = 0;
    
    if (category)
        free (category);
    
    return rc;
}

/*
 * gui_buffer_clear: clear buffer content
 */

void
gui_buffer_clear (struct t_gui_buffer *buffer)
{
    struct t_gui_window *ptr_win;
    
    if (!buffer)
        return;
    
    /* remove buffer from hotlist */
    gui_hotlist_remove_buffer (buffer);
    
    /* remove all lines */
    gui_chat_line_free_all (buffer);
    
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
    
    buffer->chat_refresh_needed = 2;
    gui_status_refresh_needed = 1;
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
 * gui_buffer_close: close a buffer
 */

void
gui_buffer_close (struct t_gui_buffer *buffer, int switch_to_another)
{
    struct t_gui_window *ptr_window;
    struct t_gui_buffer *ptr_buffer;
    
    hook_signal_send ("buffer_closing",
                      WEECHAT_HOOK_SIGNAL_POINTER, buffer);
    
    if (buffer->close_callback)
    {
        (void)(buffer->close_callback) (buffer->close_callback_data, buffer);
    }
    
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
    
    if (buffer->type == GUI_BUFFER_TYPE_FORMATED)
    {
        /* decrease buffer number for all next buffers */
        for (ptr_buffer = buffer->next_buffer; ptr_buffer;
             ptr_buffer = ptr_buffer->next_buffer)
        {
            ptr_buffer->number--;
        }
        
        /* free all lines */
        gui_chat_line_free_all (buffer);
    }
    
    /* free some data */
    if (buffer->title)
        free (buffer->title);
    if (buffer->category)
        free (buffer->category);
    if (buffer->name)
        free (buffer->name);
    if (buffer->input_buffer)
        free (buffer->input_buffer);
    if (buffer->input_buffer_color_mask)
        free (buffer->input_buffer_color_mask);
    if (buffer->completion)
        gui_completion_free (buffer->completion);
    gui_history_buffer_free (buffer);
    if (buffer->text_search_input)
        free (buffer->text_search_input);
    gui_nicklist_remove_all (buffer);
    
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
        gui_status_refresh_needed = 1;
    
    hook_signal_send ("buffer_closed", WEECHAT_HOOK_SIGNAL_STRING, NULL);
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
 * gui_buffer_switch_by_number: switch to another buffer with number
 */

void
gui_buffer_switch_by_number (struct t_gui_window *window, int number)
{
    struct t_gui_buffer *ptr_buffer;
    
    /* invalid buffer */
    if (number < 0)
        return;
    
    /* buffer is currently displayed ? */
    if (number == window->buffer->number)
        return;
    
    /* search for buffer in the list */
    for (ptr_buffer = gui_buffers; ptr_buffer; ptr_buffer = ptr_buffer->next_buffer)
    {
        if ((ptr_buffer != window->buffer) && (number == ptr_buffer->number))
        {
            gui_window_switch_to_buffer (window, ptr_buffer);
            gui_window_redraw_buffer (window->buffer);
            return;
        }
    }
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
    
    if (number < 1)
        number = 1;
    
    /* buffer number is already ok ? */
    if (number == buffer->number)
        return;
    
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
    
    hook_signal_send ("buffer_moved",
                      WEECHAT_HOOK_SIGNAL_POINTER, buffer);
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
    
    log_printf ("[buffer dump hexa (addr:0x%x)]", buffer);
    num_line = 1;
    for (ptr_line = buffer->lines; ptr_line; ptr_line = ptr_line->next_line)
    {
        /* display line without colors */
        message_without_colors = (ptr_line->message) ?
            (char *)gui_color_decode ((unsigned char *)ptr_line->message) : NULL;
        log_printf ("");
        log_printf ("  line %d: %s",
                    num_line,
                    (message_without_colors) ?
                    message_without_colors : "(null)");
        if (message_without_colors)
            free (message_without_colors);

        /* display raw message for line */
        if (ptr_line->message)
        {
            log_printf ("");
            log_printf ("  raw data for line %d (with color codes):",
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
                    log_printf ("    %-48s  %s", hexa, ascii);
                    hexa_pos = 0;
                    ascii_pos = 0;
                }
                msg_pos++;
            }
            if (ascii_pos > 0)
                log_printf ("    %-48s  %s", hexa, ascii);
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
    struct t_gui_line *ptr_line;
    char *tags;
    int num;
    
    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        log_printf ("");
        log_printf ("[buffer (addr:0x%x)]", ptr_buffer);
        log_printf ("  plugin . . . . . . . . : 0x%x", ptr_buffer->plugin);
        log_printf ("  number . . . . . . . . : %d",   ptr_buffer->number);
        log_printf ("  category . . . . . . . : '%s'", ptr_buffer->category);
        log_printf ("  name . . . . . . . . . : '%s'", ptr_buffer->name);
        log_printf ("  type . . . . . . . . . : %d",   ptr_buffer->type);
        log_printf ("  notify_level . . . . . : %d",   ptr_buffer->notify_level);
        log_printf ("  num_displayed. . . . . : %d",   ptr_buffer->num_displayed);
        log_printf ("  title. . . . . . . . . : '%s'", ptr_buffer->title);
        log_printf ("  lines. . . . . . . . . : 0x%x", ptr_buffer->lines);
        log_printf ("  last_line. . . . . . . : 0x%x", ptr_buffer->last_line);
        log_printf ("  last_read_line . . . . : 0x%x", ptr_buffer->last_read_line);
        log_printf ("  lines_count. . . . . . : %d",   ptr_buffer->lines_count);
        log_printf ("  lines_hidden . . . . . : %d",   ptr_buffer->lines_hidden);
        log_printf ("  prefix_max_length. . . : %d",   ptr_buffer->prefix_max_length);
        log_printf ("  chat_refresh_needed. . : %d",   ptr_buffer->chat_refresh_needed);
        log_printf ("  nicklist . . . . . . . : %d",   ptr_buffer->nicklist);
        log_printf ("  nicklist_case_sensitive: %d",   ptr_buffer->nicklist_case_sensitive);
        log_printf ("  nicklist_root. . . . . : 0x%x", ptr_buffer->nicklist_root);
        log_printf ("  nicklist_max_length. . : %d",   ptr_buffer->nicklist_max_length);
        log_printf ("  nicklist_display_groups: %d",   ptr_buffer->nicklist_display_groups);
        log_printf ("  nicklist_visible_count.: %d",   ptr_buffer->nicklist_visible_count);
        log_printf ("  nicklist_refresh_needed: %d",   ptr_buffer->nicklist_refresh_needed);
        log_printf ("  input. . . . . . . . . : %d",   ptr_buffer->input);
        log_printf ("  input_callback . . . . : 0x%x", ptr_buffer->input_callback);
        log_printf ("  input_callback_data. . : 0x%x", ptr_buffer->input_callback_data);
        log_printf ("  input_nick . . . . . . : '%s'", ptr_buffer->input_nick);
        log_printf ("  input_buffer . . . . . : '%s'", ptr_buffer->input_buffer);
        log_printf ("  input_buffer_color_mask: '%s'", ptr_buffer->input_buffer_color_mask);
        log_printf ("  input_buffer_alloc . . : %d",   ptr_buffer->input_buffer_alloc);
        log_printf ("  input_buffer_size. . . : %d",   ptr_buffer->input_buffer_size);
        log_printf ("  input_buffer_length. . : %d",   ptr_buffer->input_buffer_length);
        log_printf ("  input_buffer_pos . . . : %d",   ptr_buffer->input_buffer_pos);
        log_printf ("  input_buffer_1st_disp. : %d",   ptr_buffer->input_buffer_1st_display);
        log_printf ("  completion . . . . . . : 0x%x", ptr_buffer->completion);
        log_printf ("  history. . . . . . . . : 0x%x", ptr_buffer->history);
        log_printf ("  last_history . . . . . : 0x%x", ptr_buffer->last_history);
        log_printf ("  ptr_history. . . . . . : 0x%x", ptr_buffer->ptr_history);
        log_printf ("  num_history. . . . . . : %d",   ptr_buffer->num_history);
        log_printf ("  text_search. . . . . . : %d",   ptr_buffer->text_search);
        log_printf ("  text_search_exact. . . : %d",   ptr_buffer->text_search_exact);
        log_printf ("  text_search_found. . . : %d",   ptr_buffer->text_search_found);
        log_printf ("  text_search_input. . . : '%s'", ptr_buffer->text_search_input);
        log_printf ("  prev_buffer. . . . . . : 0x%x", ptr_buffer->prev_buffer);
        log_printf ("  next_buffer. . . . . . : 0x%x", ptr_buffer->next_buffer);

        log_printf ("");
        log_printf ("  => nicklist_root (addr:0x%x):", ptr_buffer->nicklist_root);
        gui_nicklist_print_log (ptr_buffer->nicklist_root, 0);
        
        log_printf ("");
        log_printf ("  => last 100 lines:");
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
            tags = string_build_with_exploded (ptr_line->tags_array, ",");
            log_printf ("       line N-%05d: y:%d, str_time:'%s', tags:'%s', "
                        "displayed:%d, refresh_needed:%d, prefix:'%s'",
                        num, ptr_line->y, ptr_line->str_time,
                        (tags) ? tags  : "",
                        (int)(ptr_line->displayed),
                        (int)(ptr_line->refresh_needed),
                        ptr_line->prefix);
            log_printf ("                     data: '%s'",
                        ptr_line->message);
            if (tags)
                free (tags);
            
            ptr_line = ptr_line->next_line;
        }
        
        if (ptr_buffer->completion)
        {
            log_printf ("");
            gui_completion_print_log (ptr_buffer->completion);
        }
    }
}
