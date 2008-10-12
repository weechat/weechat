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

/* gui-keyboard: keyboard functions, used by all GUI */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "../core/weechat.h"
#include "../core/wee-input.h"
#include "../core/wee-log.h"
#include "../core/wee-string.h"
#include "../plugins/plugin.h"
#include "gui-keyboard.h"
#include "gui-buffer.h"
#include "gui-completion.h"
#include "gui-input.h"
#include "gui-window.h"


struct t_gui_key *gui_keys = NULL;     /* key bindings                      */
struct t_gui_key *last_gui_key = NULL; /* last key binding                  */

char gui_key_combo_buffer[128];     /* buffer used for combos               */
int gui_key_grab = 0;               /* 1 if grab mode enabled (alt-k)       */
int gui_key_grab_count = 0;         /* number of keys pressed in grab mode  */

int *gui_keyboard_buffer = NULL;    /* input buffer (for paste detection)   */
int gui_keyboard_buffer_alloc = 0;  /* input buffer allocated size          */
int gui_keyboard_buffer_size = 0;   /* input buffer size in bytes           */

int gui_keyboard_paste_pending = 0; /* 1 is big paste was detected and      */
                                    /* WeeChat is asking user what to do    */
int gui_keyboard_paste_lines = 0;   /* number of lines for pending paste    */

time_t gui_keyboard_last_activity_time = 0; /* last activity time (key)     */


/*
 * gui_keyboard_init: init keyboard (create default key bindings)
 */

void
gui_keyboard_init ()
{
    gui_key_combo_buffer[0] = '\0';
    gui_key_grab = 0;
    gui_key_grab_count = 0;
    
    gui_keyboard_default_bindings ();
}

/*
 * gui_keyboard_init_last_activity_time: init last activity time with current
 *                                       time
 */

void
gui_keyboard_init_last_activity_time ()
{
    struct timeval tv_time;
    
    gettimeofday (&tv_time, NULL);
    gui_keyboard_last_activity_time = tv_time.tv_sec;
}

/*
 * gui_keyboard_grab_init: init "grab" mode
 */

void
gui_keyboard_grab_init ()
{
    gui_key_grab = 1;
    gui_key_grab_count = 0;
}

/*
 * gui_keyboard_grab_end: insert grabbed key in input buffer
 */

void
gui_keyboard_grab_end ()
{
    char *expanded_key;
    
    /* get expanded name (for example: ^U => ctrl-u) */
    expanded_key = gui_keyboard_get_expanded_name (gui_key_combo_buffer);
    
    if (expanded_key)
    {
        if (gui_current_window->buffer->input)
        {
            gui_input_insert_string (gui_current_window->buffer, expanded_key, -1);
            if (gui_current_window->buffer->completion)
                gui_completion_stop (gui_current_window->buffer->completion, 1);
            gui_input_text_changed_signal ();
        }
        free (expanded_key);
    }
    
    /* end grab mode */
    gui_key_grab = 0;
    gui_key_grab_count = 0;
    gui_key_combo_buffer[0] = '\0';
}

/*
 * gui_keyboard_get_internal_code: get internal code from user key name
 *                                 for example: return "^R" for "ctrl-R"
 */

char *
gui_keyboard_get_internal_code (const char *key)
{
    char *result;
    
    if ((result = malloc (strlen (key) + 1)))
    {
        result[0] = '\0';
        while (key[0])
        {
            if (string_strncasecmp (key, "meta2-", 6) == 0)
            {
                strcat (result, "^[[");
                key += 6;
            }
            if (string_strncasecmp (key, "meta-", 5) == 0)
            {
                strcat (result, "^[");
                key += 5;
            }
            else if (string_strncasecmp (key, "ctrl-", 5) == 0)
            {
                strcat (result, "^");
                key += 5;
            }
            else
            {
                strncat (result, key, 1);
                key++;
            }
        }
    }
    else
        return NULL;

    return result;
}

/*
 * gui_keyboard_get_expanded_name: get expanded name from internal key code
 *                                 for example: return "ctrl-R" for "^R"
 */

char *
gui_keyboard_get_expanded_name (const char *key)
{
    char *result;
    
    if ((result = malloc ((strlen (key) * 5) + 1)))
    {
        result[0] = '\0';
        while (key[0])
        {
            if (string_strncasecmp (key, "^[[", 3) == 0)
            {
                strcat (result, "meta2-");
                key += 3;
            }
            if (string_strncasecmp (key, "^[", 2) == 0)
            {
                strcat (result, "meta-");
                key += 2;
            }
            else if ((key[0] == '^') && (key[1]))
            {
                strcat (result, "ctrl-");
                key++;
            }
            else
            {
                strncat (result, key, 1);
                key++;
            }
        }
    }
    else
        return NULL;

    return result;
}

/*
 * gui_keyboard_find_pos: find position for a key (for sorting keys list)
 */

struct t_gui_key *
gui_keyboard_find_pos (struct t_gui_key *keys, struct t_gui_key *key)
{
    struct t_gui_key *ptr_key;
    
    for (ptr_key = keys; ptr_key; ptr_key = ptr_key->next_key)
    {
        if (string_strcasecmp (key->key, ptr_key->key) < 0)
            return ptr_key;
    }
    return NULL;
}

/*
 * gui_keyboard_insert_sorted: insert key into sorted list
 */

void
gui_keyboard_insert_sorted (struct t_gui_key **keys, struct t_gui_key **last_key,
                            struct t_gui_key *key)
{
    struct t_gui_key *pos_key;
    
    if (*keys)
    {
        pos_key = gui_keyboard_find_pos (*keys, key);
        
        if (pos_key)
        {
            /* insert key into the list (before key found) */
            key->prev_key = pos_key->prev_key;
            key->next_key = pos_key;
            if (pos_key->prev_key)
                pos_key->prev_key->next_key = key;
            else
                *keys = key;
            pos_key->prev_key = key;
        }
        else
        {
            /* add key to the end */
            key->prev_key = *last_key;
            key->next_key = NULL;
            (*last_key)->next_key = key;
            *last_key = key;
        }
    }
    else
    {
        key->prev_key = NULL;
        key->next_key = NULL;
        *keys = key;
        *last_key = key;
    }
}

/*
 * gui_keyboard_new: add a new key in keys list
 *                   if buffer is not null, then key is specific to buffer
 *                   otherwise it's general key (for most keys)
 */

struct t_gui_key *
gui_keyboard_new (struct t_gui_buffer *buffer, const char *key,
                  const char *command)
{
    struct t_gui_key *new_key;
    char *internal_code;
    
    if ((new_key = malloc (sizeof (*new_key))))
    {
        internal_code = gui_keyboard_get_internal_code (key);
        new_key->key = (internal_code) ? strdup (internal_code) : strdup (key);
        if (internal_code)
            free (internal_code);
        new_key->command = (command) ? strdup (command) : NULL;
        
        if (buffer)
            gui_keyboard_insert_sorted (&buffer->keys, &buffer->last_key, new_key);
        else
            gui_keyboard_insert_sorted (&gui_keys, &last_gui_key, new_key);
    }
    else
        return NULL;
    
    return new_key;
}

/*
 * gui_keyboard_search: search a key
 */

struct t_gui_key *
gui_keyboard_search (struct t_gui_buffer *buffer, const char *key)
{
    struct t_gui_key *ptr_key;

    for (ptr_key = (buffer) ? buffer->keys : gui_keys; ptr_key;
         ptr_key = ptr_key->next_key)
    {
        if (string_strcasecmp (ptr_key->key, key) == 0)
            return ptr_key;
    }
    
    /* key not found */
    return NULL;
}

/*
 * gui_keyboard_cmp: compares 2 keys
 */

int
gui_keyboard_cmp (const char *key, const char *search)
{
    while (search[0])
    {
        if (toupper(key[0]) != toupper(search[0]))
            return search[0] - key[0];
        key++;
        search++;
    }
    
    return 0;
}

/*
 * gui_keyboard_search_part: search a key (maybe part of string)
 */

struct t_gui_key *
gui_keyboard_search_part (struct t_gui_buffer *buffer, const char *key)
{
    struct t_gui_key *ptr_key;
    
    for (ptr_key = (buffer) ? buffer->keys : gui_keys; ptr_key;
         ptr_key = ptr_key->next_key)
    {
        if (gui_keyboard_cmp (ptr_key->key, key) == 0)
            return ptr_key;
    }
    
    /* key not found */
    return NULL;
}

/*
 * gui_keyboard_bind: bind a key to a function (command or special function)
 *                    if buffer is not null, then key is specific to buffer
 *                    otherwise it's general key (for most keys)
 */

struct t_gui_key *
gui_keyboard_bind (struct t_gui_buffer *buffer, const char *key, const char *command)
{
    struct t_gui_key *new_key;
    
    if (!key || !command)
    {
        log_printf (_("Error: unable to bind key \"%s\""), key);
        return NULL;
    }
    
    gui_keyboard_unbind (buffer, key);
    
    new_key = gui_keyboard_new (buffer, key, command);
    if (!new_key)
    {
        log_printf (_("Error: not enough memory for key binding"));
        return NULL;
    }
    
    return new_key;
}

/*
 * gui_keyboard_unbind: remove a key binding
 */

int
gui_keyboard_unbind (struct t_gui_buffer *buffer, const char *key)
{
    struct t_gui_key *ptr_key;
    char *internal_code;
    
    internal_code = gui_keyboard_get_internal_code (key);
    
    ptr_key = gui_keyboard_search (buffer, (internal_code) ? internal_code : key);
    if (ptr_key)
    {
        if (buffer)
            gui_keyboard_free (&buffer->keys, &buffer->last_key, ptr_key);
        else
            gui_keyboard_free (&gui_keys, &last_gui_key, ptr_key);
    }
    
    if (internal_code)
        free (internal_code);
    
    return (ptr_key != NULL);
}

/*
 * gui_keyboard_pressed: treat new key pressed
 *                       return: 1 if key should be added to input buffer
 *                               0 otherwise
 */

int
gui_keyboard_pressed (const char *key_str)
{
    int first_key;
    struct t_gui_key *ptr_key;
    char *buffer_before_key;
    char **commands, **ptr_cmd;
    
    /* add key to buffer */
    first_key = (gui_key_combo_buffer[0] == '\0');
    strcat (gui_key_combo_buffer, key_str);
    
    /* if we are in "show mode", increase counter and return */
    if (gui_key_grab)
    {
        gui_key_grab_count++;
        return 0;
    }
    
    /* look for key combo in key table for current buffer */
    ptr_key = gui_keyboard_search_part (gui_current_window->buffer,
                                        gui_key_combo_buffer);
    /* if key is not found for buffer, then look in general table */
    if (!ptr_key)
        ptr_key = gui_keyboard_search_part (NULL, gui_key_combo_buffer);
    
    /* if key is found, then execute action */
    if (ptr_key)
    {
        if (string_strcasecmp (ptr_key->key, gui_key_combo_buffer) == 0)
        {
            /* exact combo found => execute function or command */
            buffer_before_key =
                (gui_current_window->buffer->input_buffer) ?
                strdup (gui_current_window->buffer->input_buffer) : strdup ("");
            gui_key_combo_buffer[0] = '\0';
            if (ptr_key->command)
            {
                commands = string_split_command (ptr_key->command, ';');
                if (commands)
                {
                    for (ptr_cmd = commands; *ptr_cmd; ptr_cmd++)
                    {
                        input_data (gui_current_window->buffer,
                                    *ptr_cmd);
                    }
                    string_free_splitted_command (commands);
                }
            }
            
            if (gui_current_window->buffer->text_search == GUI_TEXT_SEARCH_DISABLED)
            {
                /* TODO: execute keyboard hooks */
                /*(void) plugin_keyboard_handler_exec (
                    (ptr_key->command) ?
                    ptr_key->command : gui_keyboard_function_search_by_ptr (ptr_key->function),
                    buffer_before_key,
                    gui_current_window->buffer->input_buffer);*/
            }
            
            if (buffer_before_key)
                free (buffer_before_key);
        }
        return 0;
    }
    
    gui_key_combo_buffer[0] = '\0';

    /* if this is first key and not found (even partial) => return 1
       else return 0 (= silently discard sequence of bad keys) */
    return first_key;
}

/*
 * gui_keyboard_free: delete a key binding
 */

void
gui_keyboard_free (struct t_gui_key **keys, struct t_gui_key **last_key,
                   struct t_gui_key *key)
{
    /* free memory */
    if (key->key)
        free (key->key);
    if (key->command)
        free (key->command);
    
    /* remove key from keys list */
    if (key->prev_key)
        (key->prev_key)->next_key = key->next_key;
    if (key->next_key)
        (key->next_key)->prev_key = key->prev_key;
    if (*keys == key)
        *keys = key->next_key;
    if (*last_key == key)
        *last_key = key->prev_key;
    
    free (key);
}

/*
 * gui_keyboard_free_all: delete all key bindings
 */

void
gui_keyboard_free_all (struct t_gui_key **keys, struct t_gui_key **last_key)
{
    while (*keys)
    {
        gui_keyboard_free (keys, last_key, *keys);
    }
}

/*
 * gui_keyboard_buffer_optimize: optimize keyboard buffer size
 */

void
gui_keyboard_buffer_optimize ()
{
    int optimal_size;
    
    optimal_size = (((gui_keyboard_buffer_size * sizeof (int)) /
                     GUI_KEYBOARD_BUFFER_BLOCK_SIZE) *
                    GUI_KEYBOARD_BUFFER_BLOCK_SIZE) +
        GUI_KEYBOARD_BUFFER_BLOCK_SIZE;
    
    if (gui_keyboard_buffer_alloc != optimal_size)
    {
        gui_keyboard_buffer_alloc = optimal_size;
        gui_keyboard_buffer = realloc (gui_keyboard_buffer, optimal_size);
    }
}

/*
 * gui_keyboard_buffer_reset: reset keyboard buffer
 *                            (create empty if never created before)
 */

void
gui_keyboard_buffer_reset ()
{
    if (!gui_keyboard_buffer)
    {
        gui_keyboard_buffer_alloc = GUI_KEYBOARD_BUFFER_BLOCK_SIZE;
        gui_keyboard_buffer_size = 0;
        gui_keyboard_buffer = malloc (gui_keyboard_buffer_alloc);
    }
    else
    {
        gui_keyboard_buffer_size = 0;
        gui_keyboard_buffer_optimize ();
    }
    gui_keyboard_paste_lines = 0;
}

/*
 * gui_keyboard_buffer_add: add a key to keyboard buffer
 */

void
gui_keyboard_buffer_add (unsigned char key)
{
    if (!gui_keyboard_buffer)
        gui_keyboard_buffer_reset ();
    
    gui_keyboard_buffer_size++;
    
    gui_keyboard_buffer_optimize ();
    
    if (gui_keyboard_buffer)
    {
        gui_keyboard_buffer[gui_keyboard_buffer_size - 1] = key;
        if ((key == 13)
            && (gui_keyboard_buffer_size > 1)
            && (gui_keyboard_buffer[gui_keyboard_buffer_size - 2] != 13))
            gui_keyboard_paste_lines++;
    }
    else
    {
        gui_keyboard_buffer_alloc = 0;
        gui_keyboard_buffer_size = 0;
        gui_keyboard_paste_lines = 0;
    }
}

/*
 * gui_keyboard_get_paste_lines: return real number of lines in buffer
 *                               if last key is not Return, then this is lines + 1
 *                               else it's lines
 */

int
gui_keyboard_get_paste_lines ()
{
    if ((gui_keyboard_buffer_size > 0)
        && (gui_keyboard_buffer[gui_keyboard_buffer_size - 1] != 13))
        return gui_keyboard_paste_lines + 1;
    
    return gui_keyboard_paste_lines;
}

/*
 * gui_keyboard_paste_accept: accept paste from user
 */

void
gui_keyboard_paste_accept ()
{
    gui_keyboard_paste_pending = 0;
    gui_input_paste_pending_signal ();
}


/*
 * gui_keyboard_paste_cancel: cancel paste from user (reset buffer)
 */

void
gui_keyboard_paste_cancel ()
{
    gui_keyboard_buffer_reset ();
    gui_keyboard_paste_pending = 0;
    gui_input_paste_pending_signal ();
}

/*
 * gui_keyboard_end: end keyboard (free some data) 
 */

void
gui_keyboard_end ()
{
    /* free keyboard buffer */
    if (gui_keyboard_buffer)
        free (gui_keyboard_buffer);
    
    /* free all keys */
    gui_keyboard_free_all (&gui_keys, &last_gui_key);
}

/*
 * gui_keyboard_print_log: print key infos in log (usually for crash dump)
 */

void
gui_keyboard_print_log (struct t_gui_buffer *buffer)
{
    struct t_gui_key *ptr_key;
    char prefix[32];
    
    snprintf (prefix, sizeof (prefix), "%s", (buffer) ? "    " : "");
    
    for (ptr_key = (buffer) ? buffer->keys : gui_keys; ptr_key;
         ptr_key = ptr_key->next_key)
    {
        log_printf ("");
        log_printf ("%s[key (addr:0x%x)]", prefix, ptr_key);
        log_printf ("%skey. . . . . . . . . : '%s'", prefix, ptr_key->key);
        log_printf ("%scommand. . . . . . . : '%s'", prefix, ptr_key->command);
        log_printf ("%sprev_key . . . . . . : 0x%x", prefix, ptr_key->prev_key);
        log_printf ("%snext_key . . . . . . : 0x%x", prefix, ptr_key->next_key);
    }
}
