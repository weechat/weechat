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
#include "gui-action.h"
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

struct t_gui_key_function gui_key_functions[] =
{ { "return",                    gui_action_return,
    N_("terminate line") },
  { "tab",                       gui_action_tab,
    N_("complete word") },
  { "tab_previous",              gui_action_tab_previous,
    N_("find previous completion for word") },
  { "backspace",                 gui_action_backspace,
    N_("delete previous char") },
  { "delete",                    gui_action_delete,
    N_("delete next char") },
  { "delete_end_line",           gui_action_delete_end_of_line,
    N_("delete until end of line") },
  { "delete_beginning_line",     gui_action_delete_begin_of_line,
    N_("delete until beginning of line") },
  { "delete_line",               gui_action_delete_line,
    N_("delete entire line") },
  { "delete_previous_word",      gui_action_delete_previous_word,
    N_("delete previous word") },
  { "delete_next_word",          gui_action_delete_next_word,
    N_("delete next word") },
  { "clipboard_paste",           gui_action_clipboard_paste,
    N_("paste current clipboard content") },
  { "transpose_chars",           gui_action_transpose_chars,
    N_("transpose chars") },
  { "home",                      gui_action_home,
    N_("go to beginning of line") },
  { "end",                       gui_action_end,
    N_("go to end of line") },
  { "left",                      gui_action_left,
    N_("move one char left") },
  { "previous_word",             gui_action_previous_word,
    N_("move to previous word") },
  { "right",                     gui_action_right,
    N_("move one char right") },
  { "next_word",                 gui_action_next_word,
    N_("move to next word") },
  { "up",                        gui_action_up,
    N_("call previous command in history") },
  { "up_global",                 gui_action_up_global,
    N_("call previous command in global history") },
  { "down",                      gui_action_down,
    N_("call next command in history") },
  { "down_global",               gui_action_down_global,
    N_("call next command in global history") },
  { "page_up",                   gui_action_page_up,
    N_("scroll one page up") },
  { "page_down",                 gui_action_page_down,
    N_("scroll one page down") },
  { "scroll_up",                 gui_action_scroll_up,
    N_("scroll a few lines up") },
  { "scroll_down",               gui_action_scroll_down,
    N_("scroll a few lines down") },
  { "scroll_top",                gui_action_scroll_top,
    N_("scroll to top of buffer") },
  { "scroll_bottom",             gui_action_scroll_bottom,
    N_("scroll to bottom of buffer") },
  { "scroll_topic_left",         gui_action_scroll_topic_left,
    N_("scroll left topic") },
  { "scroll_topic_right",        gui_action_scroll_topic_right,
    N_("scroll right topic") },
  { "nick_beginning",            gui_action_nick_beginning,
    N_("display beginning of nicklist") },
  { "nick_end",                  gui_action_nick_end,
    N_("display end of nicklist") },
  { "nick_page_up",              gui_action_nick_page_up,
    N_("scroll nicklist one page up") },
  { "nick_page_down",            gui_action_nick_page_down,
    N_("scroll nicklist one page down") },
  { "jump_smart",                gui_action_jump_smart,
    N_("jump to buffer with activity") },
  { "jump_dcc",                  gui_action_jump_dcc,
    N_("jump to DCC buffer") },
  { "jump_last_buffer",          gui_action_jump_last_buffer,
    N_("jump to last buffer") },
  { "jump_previous_buffer",      gui_action_jump_previous_buffer,
    N_("jump to previous buffer") },
  { "jump_server",               gui_action_jump_server,
    N_("jump to server buffer") },
  { "jump_next_server",          gui_action_jump_next_server,
    N_("jump to next server") },
  { "switch_server",             gui_action_switch_server,
    N_("switch active server on servers buffer") },
  { "scroll_previous_highlight", gui_action_scroll_previous_highlight,
    N_("scroll to previous highlight in buffer") },
  { "scroll_next_highlight",     gui_action_scroll_next_highlight,
    N_("scroll to next highlight in buffer") },
  { "scroll_unread",             gui_action_scroll_unread,
    N_("scroll to first unread line in buffer") },
  { "set_unread",                gui_action_set_unread,
    N_("set unread marker on all buffers") },
  { "hotlist_clear",             gui_action_hotlist_clear,
    N_("clear hotlist") },
  { "infobar_clear",             gui_action_infobar_clear,
    N_("clear infobar") },
  { "refresh",                   gui_action_refresh_screen,
    N_("refresh screen") },
  { "grab_key",                  gui_action_grab_key,
    N_("grab a key") },
  { "insert",                    gui_action_insert_string,
    N_("insert a string in command line") },
  { "search_text",               gui_action_search_text,
    N_("search text in buffer history") },
  { NULL, NULL, NULL }
};


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
                gui_current_window->buffer->completion->position = -1;
            gui_input_draw (gui_current_window->buffer, 0);
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
gui_keyboard_get_internal_code (char *key)
{
    char *result;
    
    if ((result = (char *)malloc ((strlen (key) + 1) * sizeof (char))))
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
gui_keyboard_get_expanded_name (char *key)
{
    char *result;
    
    if ((result = (char *)malloc (((strlen (key) * 5) + 1) * sizeof (char))))
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
gui_keyboard_find_pos (struct t_gui_key *key)
{
    struct t_gui_key *ptr_key;
    
    for (ptr_key = gui_keys; ptr_key; ptr_key = ptr_key->next_key)
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
gui_keyboard_insert_sorted (struct t_gui_key *key)
{
    struct t_gui_key *pos_key;
    
    if (gui_keys)
    {
        pos_key = gui_keyboard_find_pos (key);
        
        if (pos_key)
        {
            /* insert key into the list (before key found) */
            key->prev_key = pos_key->prev_key;
            key->next_key = pos_key;
            if (pos_key->prev_key)
                pos_key->prev_key->next_key = key;
            else
                gui_keys = key;
            pos_key->prev_key = key;
        }
        else
        {
            /* add key to the end */
            key->prev_key = last_gui_key;
            key->next_key = NULL;
            last_gui_key->next_key = key;
            last_gui_key = key;
        }
    }
    else
    {
        key->prev_key = NULL;
        key->next_key = NULL;
        gui_keys = key;
        last_gui_key = key;
    }
}

/*
 * gui_keyboard_new: add a new key in keys list
 */

struct t_gui_key *
gui_keyboard_new (char *key, char *command, t_gui_key_func *function,
                  char *args)
{
    struct t_gui_key *new_key;
    char *internal_code;
    int length;
    
    if ((new_key = (struct t_gui_key *)malloc (sizeof (struct t_gui_key))))
    {
        internal_code = gui_keyboard_get_internal_code (key);
        new_key->key = (internal_code) ? strdup (internal_code) : strdup (key);
        if (internal_code)
            free (internal_code);
        new_key->command = (command) ? strdup (command) : NULL;
        new_key->function = function;
        if (args)
        {
            if (args[0] == '"')
            {
                length = strlen (args);
                if ((length > 1) && (args[length - 1] == '"'))
                    new_key->args = string_strndup (args + 1, length - 2);
                else
                    new_key->args = strdup (args);
            }
            else
                new_key->args = strdup (args);
        }
        else
            new_key->args = NULL;
        gui_keyboard_insert_sorted (new_key);
    }
    else
        return NULL;
    
    return new_key;
}

/*
 * gui_keyboard_search: search a key
 */

struct t_gui_key *
gui_keyboard_search (char *key)
{
    struct t_gui_key *ptr_key;

    for (ptr_key = gui_keys; ptr_key; ptr_key = ptr_key->next_key)
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
gui_keyboard_cmp (char *key, char *search)
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
gui_keyboard_search_part (char *key)
{
    struct t_gui_key *ptr_key;
    
    for (ptr_key = gui_keys; ptr_key; ptr_key = ptr_key->next_key)
    {
        if (gui_keyboard_cmp (ptr_key->key, key) == 0)
            return ptr_key;
    }
    
    /* key not found */
    return NULL;
}

/*
 * gui_keyboard_function_search_by_name: search a function by name
 */

t_gui_key_func *
gui_keyboard_function_search_by_name (char *name)
{
    int i;
    
    i = 0;
    while (gui_key_functions[i].function_name)
    {
        if (string_strcasecmp (gui_key_functions[i].function_name, name) == 0)
            return gui_key_functions[i].function;
        i++;
    }

    /* function not found */
    return NULL;
}

/*
 * gui_keyboard_function_search_by_ptr: search a function by pointer
 */

char *
gui_keyboard_function_search_by_ptr (t_gui_key_func *function)
{
    int i;
    
    i = 0;
    while (gui_key_functions[i].function_name)
    {
        if (gui_key_functions[i].function == function)
            return gui_key_functions[i].function_name;
        i++;
    }

    /* function not found */
    return NULL;
}

/*
 * gui_keyboard_bind: bind a key to a function (command or special function)
 */

struct t_gui_key *
gui_keyboard_bind (char *key, char *command)
{
    t_gui_key_func *ptr_function;
    struct t_gui_key *new_key;
    char *command2, *ptr_args;
    
    if (!key || !command)
    {
        log_printf (_("Error: unable to bind key \"%s\""), key);
        return NULL;
    }
    
    ptr_function = NULL;
    ptr_args = NULL;
    
    if (command[0] != '/')
    {
        ptr_args = strchr (command, ' ');
        if (ptr_args)
            command2 = string_strndup (command, ptr_args - command);
        else
            command2 = strdup (command);
        if (command2)
        {
            ptr_function = gui_keyboard_function_search_by_name (command2);
            if (ptr_args)
            {
                ptr_args++;
                while (ptr_args[0] == ' ')
                    ptr_args++;
            }
            free (command2);
        }
        if (!ptr_function)
        {
            log_printf (_("Error: unable to bind key \"%s\" "
                          "(invalid function name: \"%s\")"),
                        key, command);
            return NULL;
        }
    }
    
    gui_keyboard_unbind (key);
    
    new_key = gui_keyboard_new (key,
                                (ptr_function) ? NULL : command,
                                ptr_function,
                                ptr_args);
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
gui_keyboard_unbind (char *key)
{
    struct t_gui_key *ptr_key;
    char *internal_code;
    
    internal_code = gui_keyboard_get_internal_code (key);
    
    ptr_key = gui_keyboard_search ((internal_code) ? internal_code : key);
    if (ptr_key)
        gui_keyboard_free (ptr_key);
    
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
gui_keyboard_pressed (char *key_str)
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
    
    /* look for key combo in key table */
    ptr_key = gui_keyboard_search_part (gui_key_combo_buffer);
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
                                    *ptr_cmd, 0);
                    }
                    string_free_splitted_command (commands);
                }
            }
            else
                (void)(ptr_key->function)(ptr_key->args);
            
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
gui_keyboard_free (struct t_gui_key *key)
{
    /* free memory */
    if (key->key)
        free (key->key);
    if (key->command)
        free (key->command);
    if (key->args)
        free (key->args);
    
    /* remove key from keys list */
    if (key->prev_key)
        key->prev_key->next_key = key->next_key;
    if (key->next_key)
        key->next_key->prev_key = key->prev_key;
    if (gui_keys == key)
        gui_keys = key->next_key;
    if (last_gui_key == key)
        last_gui_key = key->prev_key;
    
    free (key);
}

/*
 * gui_keyboard_free_all: delete all key bindings
 */

void
gui_keyboard_free_all ()
{
    while (gui_keys)
    {
        gui_keyboard_free (gui_keys);
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
        gui_keyboard_buffer = realloc (gui_keyboard_buffer, optimal_size * sizeof (char));
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
        gui_keyboard_buffer = (int *)malloc (gui_keyboard_buffer_alloc);
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
gui_keyboard_buffer_add (int key)
{
    if (!gui_keyboard_buffer)
        gui_keyboard_buffer_reset ();
    
    gui_keyboard_buffer_size++;
    
    gui_keyboard_buffer_optimize ();
    
    if (gui_keyboard_buffer)
    {
        gui_keyboard_buffer[gui_keyboard_buffer_size - 1] = key;
        if ((key == 10)
            && (gui_keyboard_buffer_size > 1)
            && (gui_keyboard_buffer[gui_keyboard_buffer_size - 2] != 10))
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
        && (gui_keyboard_buffer[gui_keyboard_buffer_size - 1] != 10))
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
}


/*
 * gui_keyboard_paste_cancel: cancel paste from user (reset buffer)
 */

void
gui_keyboard_paste_cancel ()
{
    gui_keyboard_buffer_reset ();
    gui_keyboard_paste_pending = 0;
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
    gui_keyboard_free_all ();
}
