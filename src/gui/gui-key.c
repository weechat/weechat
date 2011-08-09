/*
 * Copyright (C) 2003-2011 Sebastien Helleu <flashcode@flashtux.org>
 *
 * This file is part of WeeChat, the extensible chat client.
 *
 * WeeChat is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * WeeChat is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * gui-key.c: keyboard functions (used by all GUI)
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "../core/weechat.h"
#include "../core/wee-hashtable.h"
#include "../core/wee-hdata.h"
#include "../core/wee-hook.h"
#include "../core/wee-infolist.h"
#include "../core/wee-input.h"
#include "../core/wee-log.h"
#include "../core/wee-string.h"
#include "../core/wee-utf8.h"
#include "../plugins/plugin.h"
#include "gui-key.h"
#include "gui-bar.h"
#include "gui-bar-window.h"
#include "gui-buffer.h"
#include "gui-chat.h"
#include "gui-color.h"
#include "gui-completion.h"
#include "gui-cursor.h"
#include "gui-focus.h"
#include "gui-input.h"
#include "gui-mouse.h"
#include "gui-window.h"


struct t_gui_key *gui_keys[GUI_KEY_NUM_CONTEXTS];     /* keys by context    */
struct t_gui_key *last_gui_key[GUI_KEY_NUM_CONTEXTS]; /* last key           */
struct t_gui_key *gui_default_keys[GUI_KEY_NUM_CONTEXTS]; /* default keys   */
struct t_gui_key *last_gui_default_key[GUI_KEY_NUM_CONTEXTS];
int gui_keys_count[GUI_KEY_NUM_CONTEXTS];            /* keys number         */
int gui_default_keys_count[GUI_KEY_NUM_CONTEXTS];    /* default keys number */

char *gui_key_context_string[GUI_KEY_NUM_CONTEXTS] =
{ "default", "search", "cursor", "mouse" };

char *gui_key_focus_string[GUI_KEY_NUM_FOCUS] =
{ "*", "chat", "bar", "item" };

int gui_key_verbose = 0;            /* 1 to see some messages               */

char gui_key_combo_buffer[1024];    /* buffer used for combos               */
int gui_key_grab = 0;               /* 1 if grab mode enabled (alt-k)       */
int gui_key_grab_count = 0;         /* number of keys pressed in grab mode  */
int gui_key_grab_command = 0;       /* grab command bound to key?           */
int gui_key_grab_delay = 0;         /* delay for grab (default is 500)      */

int *gui_key_buffer = NULL;         /* input buffer (for paste detection)   */
int gui_key_buffer_alloc = 0;       /* input buffer allocated size          */
int gui_key_buffer_size = 0;        /* input buffer size in bytes           */

int gui_key_paste_pending = 0;      /* 1 is big paste was detected and      */
                                    /* WeeChat is asking user what to do    */
int gui_key_paste_lines = 0;        /* number of lines for pending paste    */

time_t gui_key_last_activity_time = 0; /* last activity time (key)          */


/*
 * gui_key_init: init keyboard
 */

void
gui_key_init ()
{
    int i;
    
    gui_key_combo_buffer[0] = '\0';
    gui_key_grab = 0;
    gui_key_grab_count = 0;
    gui_key_last_activity_time = time (NULL);
    
    /* create default keys and save them in a separate list */
    for (i = 0; i < GUI_KEY_NUM_CONTEXTS; i++)
    {
        gui_keys[i] = NULL;
        last_gui_key[i] = NULL;
        gui_default_keys[i] = NULL;
        last_gui_default_key[i] = NULL;
        gui_keys_count[i] = 0;
        gui_default_keys_count[i] = 0;
        gui_key_default_bindings (i);
        gui_default_keys[i] = gui_keys[i];
        last_gui_default_key[i] = last_gui_key[i];
        gui_default_keys_count[i] = gui_keys_count[i];
        gui_keys[i] = NULL;
        last_gui_key[i] = NULL;
        gui_keys_count[i] = 0;
    }
}

/*
 * gui_key_search_context: search context by name
 */

int
gui_key_search_context (const char *context)
{
    int i;
    
    for (i = 0; i < GUI_KEY_NUM_CONTEXTS; i++)
    {
        if (string_strcasecmp (gui_key_context_string[i], context) == 0)
            return i;
    }
    
    /* context not found */
    return -1;
}

/*
 * gui_key_get_current_context: get current context
 */

int
gui_key_get_current_context ()
{
    if (gui_cursor_mode)
        return GUI_KEY_CONTEXT_CURSOR;
    
    if (gui_current_window
        && (gui_current_window->buffer->text_search != GUI_TEXT_SEARCH_DISABLED))
        return GUI_KEY_CONTEXT_SEARCH;
    
    return GUI_KEY_CONTEXT_DEFAULT;
}

/*
 * gui_key_grab_init: init "grab" mode
 */

void
gui_key_grab_init (int grab_command, const char *delay)
{
    long milliseconds;
    char *error;
    
    gui_key_grab = 1;
    gui_key_grab_count = 0;
    gui_key_grab_command = grab_command;
    
    gui_key_grab_delay = GUI_KEY_GRAB_DELAY_DEFAULT;
    if (delay != NULL)
    {
        error = NULL;
        milliseconds = strtol (delay, &error, 10);
        if (error && !error[0] && (milliseconds >= 0))
        {
            gui_key_grab_delay = milliseconds;
            if (gui_key_grab_delay == 0)
                gui_key_grab_delay = 1;
        }
    }
}

/*
 * gui_key_grab_end_timer_cb: insert grabbed key in input buffer
 */

int
gui_key_grab_end_timer_cb (void *data, int remaining_calls)
{
    char *expanded_key, *expanded_key2;
    struct t_gui_key *ptr_key;
    
    /* make C compiler happy */
    (void) data;
    (void) remaining_calls;
    
    /* get expanded name (for example: \x01+U => ctrl-u) */
    expanded_key = gui_key_get_expanded_name (gui_key_combo_buffer);
    if (expanded_key)
    {
        /*
         * the expanded_key should be valid UTF-8 at this point,
         * but some mouse codes can return ISO chars (for coordinates),
         * then we will convert them to UTF-8 string
         */
        if (!utf8_is_valid (expanded_key, NULL))
        {
            expanded_key2 = string_iconv_to_internal ("iso-8859-1",
                                                      expanded_key);
            if (expanded_key2)
            {
                free (expanded_key);
                expanded_key = expanded_key2;
            }
            else
            {
                /* conversion failed, then just replace invalid chars by '?' */
                utf8_normalize (expanded_key, '?');
            }
        }
        /* add expanded key to input buffer */
        if (gui_current_window->buffer->input)
        {
            gui_input_insert_string (gui_current_window->buffer, expanded_key, -1);
            if (gui_key_grab_command)
            {
                /* add command bound to key (if found) */
                ptr_key = gui_key_search (gui_keys[GUI_KEY_CONTEXT_DEFAULT],
                                          gui_key_combo_buffer);
                if (ptr_key)
                {
                    gui_input_insert_string (gui_current_window->buffer, " ", -1);
                    gui_input_insert_string (gui_current_window->buffer, ptr_key->command, -1);
                }
            }
            if (gui_current_window->buffer->completion)
                gui_completion_stop (gui_current_window->buffer->completion, 1);
            gui_input_text_changed_modifier_and_signal (gui_current_window->buffer, 1);
        }
        free (expanded_key);
    }
    
    /* end grab mode */
    gui_key_grab = 0;
    gui_key_grab_count = 0;
    gui_key_grab_command = 0;
    gui_key_combo_buffer[0] = '\0';
    
    return WEECHAT_RC_OK;
}

/*
 * gui_key_get_internal_code: get internal code from user key name
 *                            for example: return "\x01+R" for "ctrl-R"
 */

char *
gui_key_get_internal_code (const char *key)
{
    char *result;
    
    if ((result = malloc (strlen (key) + 1)))
    {
        result[0] = '\0';
        while (key[0])
        {
            if (strncmp (key, "meta2-", 6) == 0)
            {
                strcat (result, "\x01[[");
                key += 6;
            }
            if (strncmp (key, "meta-", 5) == 0)
            {
                strcat (result, "\x01[");
                key += 5;
            }
            else if (strncmp (key, "ctrl-", 5) == 0)
            {
                strcat (result, "\x01");
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
 * gui_key_get_expanded_name: get expanded name from internal key code
 *                            for example: return "ctrl-R" for "\x01+R"
 */

char *
gui_key_get_expanded_name (const char *key)
{
    char *result;
    
    if (!key)
        return NULL;
    
    result = malloc ((strlen (key) * 5) + 1);
    if (result)
    {
        result[0] = '\0';
        while (key[0])
        {
            if (strncmp (key, "\x01[[", 3) == 0)
            {
                strcat (result, "meta2-");
                key += 3;
            }
            if (strncmp (key, "\x01[", 2) == 0)
            {
                strcat (result, "meta-");
                key += 2;
            }
            else if ((key[0] == '\x01') && (key[1]))
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
    
    return result;
}

/*
 * gui_key_find_pos: find position for a key (for sorting keys list)
 */

struct t_gui_key *
gui_key_find_pos (struct t_gui_key *keys, struct t_gui_key *key)
{
    struct t_gui_key *ptr_key;
    
    for (ptr_key = keys; ptr_key; ptr_key = ptr_key->next_key)
    {
        if (strcmp (key->key, ptr_key->key) < 0)
            return ptr_key;
    }
    return NULL;
}

/*
 * gui_key_insert_sorted: insert key into sorted list
 */

void
gui_key_insert_sorted (struct t_gui_key **keys,
                       struct t_gui_key **last_key,
                       int *keys_count,
                       struct t_gui_key *key)
{
    struct t_gui_key *pos_key;
    
    if (*keys)
    {
        pos_key = gui_key_find_pos (*keys, key);
        
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
        /* first key in list */
        key->prev_key = NULL;
        key->next_key = NULL;
        *keys = key;
        *last_key = key;
    }
    
    (*keys_count)++;
}

/*
 * gui_key_new: add a new key in keys list
 *              if buffer is not null, then key is specific to buffer
 *              otherwise it's general key (for most keys)
 */

struct t_gui_key *
gui_key_new (struct t_gui_buffer *buffer, int context, const char *key,
             const char *command)
{
    struct t_gui_key *new_key;
    char *expanded_name;
    
    if (!key || !command)
        return NULL;
    
    if ((new_key = malloc (sizeof (*new_key))))
    {
        new_key->key = gui_key_get_internal_code (key);
        if (!new_key->key)
            new_key->key = strdup (key);
        if (!new_key->key)
        {
            free (new_key);
            return NULL;
        }
        new_key->command = strdup (command);
        if (!new_key->command)
        {
            free (new_key->key);
            free (new_key);
            return NULL;
        }
        
        if (buffer)
        {
            gui_key_insert_sorted (&buffer->keys, &buffer->last_key,
                                   &buffer->keys_count, new_key);
        }
        else
        {
            gui_key_insert_sorted (&gui_keys[context],
                                   &last_gui_key[context],
                                   &gui_keys_count[context], new_key);
        }
        
        expanded_name = gui_key_get_expanded_name (new_key->key);
        
        hook_signal_send ("key_bind",
                          WEECHAT_HOOK_SIGNAL_STRING, expanded_name);
        
        if (gui_key_verbose)
        {
            gui_chat_printf (NULL,
                             _("New key binding (context \"%s\"): "
                               "%s%s => %s%s"),
                             gui_key_context_string[context],
                             (expanded_name) ? expanded_name : new_key->key,
                             GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                             GUI_COLOR(GUI_COLOR_CHAT),
                             new_key->command);
        }
        if (expanded_name)
                free (expanded_name);
    }
    else
        return NULL;
    
    return new_key;
}

/*
 * gui_key_search: search a key
 */

struct t_gui_key *
gui_key_search (struct t_gui_key *keys, const char *key)
{
    struct t_gui_key *ptr_key;
    
    for (ptr_key = keys; ptr_key; ptr_key = ptr_key->next_key)
    {
        if (strcmp (ptr_key->key, key) == 0)
            return ptr_key;
    }
    
    /* key not found */
    return NULL;
}

/*
 * gui_key_cmp: compares 2 keys
 */

int
gui_key_cmp (const char *key, const char *search, int context)
{
    int diff;
    
    if (context == GUI_KEY_CONTEXT_MOUSE)
        return (string_match (key, search, 1)) ? 0 : 1;
    
    while (search[0])
    {
        diff = utf8_charcmp (key, search);
        if (diff != 0)
            return diff;
        key = utf8_next_char (key);
        search = utf8_next_char (search);
    }
    
    return 0;
}

/*
 * gui_key_search_part: search a key (maybe part of string)
 */

struct t_gui_key *
gui_key_search_part (struct t_gui_buffer *buffer, int context,
                     const char *key)
{
    struct t_gui_key *ptr_key;
    
    for (ptr_key = (buffer) ? buffer->keys : gui_keys[context]; ptr_key;
         ptr_key = ptr_key->next_key)
    {
        if (ptr_key->key
            && (((context != GUI_KEY_CONTEXT_CURSOR)
                 && (context != GUI_KEY_CONTEXT_MOUSE))
                || (ptr_key->key[0] != '@')))
        {
            if (gui_key_cmp (ptr_key->key, key, context) == 0)
                return ptr_key;
        }
    }
    
    /* key not found */
    return NULL;
}

/*
 * gui_key_bind: bind a key to a function (command or special function)
 *               if buffer is not null, then key is specific to buffer
 *               otherwise it's general key (for most keys)
 */

struct t_gui_key *
gui_key_bind (struct t_gui_buffer *buffer, int context, const char *key,
              const char *command)
{
    struct t_gui_key *new_key;
    
    if (!key || !command)
    {
        log_printf (_("Error: unable to bind key \"%s\""), key);
        return NULL;
    }
    
    gui_key_unbind (buffer, context, key, 0);
    
    new_key = gui_key_new (buffer, context, key, command);
    if (!new_key)
    {
        log_printf (_("Error: not enough memory for key binding"));
        return NULL;
    }
    
    return new_key;
}

/*
 * gui_key_unbind: remove a key binding
 */

int
gui_key_unbind (struct t_gui_buffer *buffer, int context, const char *key,
                int send_signal)
{
    struct t_gui_key *ptr_key;
    char *internal_code;
    
    internal_code = gui_key_get_internal_code (key);
    
    ptr_key = gui_key_search ((buffer) ? buffer->keys : gui_keys[context],
                              (internal_code) ? internal_code : key);
    if (ptr_key)
    {
        if (buffer)
        {
            gui_key_free (&buffer->keys, &buffer->last_key,
                          &buffer->keys_count, ptr_key);
        }
        else
        {
            gui_key_free (&gui_keys[context], &last_gui_key[context],
                          &gui_keys_count[context], ptr_key);
        }
    }
    
    if (internal_code)
        free (internal_code);
    
    if (send_signal)
    {
        hook_signal_send ("key_unbind",
                          WEECHAT_HOOK_SIGNAL_STRING, (char *)key);
    }
    
    return (ptr_key != NULL);
}

/*
 * gui_key_focus_matching: return 1 if area in key is matching focus area on
 *                         screen (cursor/mouse)
 */

int
gui_key_focus_matching (int area_type, const char *area_name,
                        int focus_specific, int focus_any,
                        struct t_gui_focus_info *focus_info)
{
    int match, match_any;
    char buffer_full_name[512];
    
    match_any = (strcmp (area_name, "*") == 0) ? 1 : 0;
    
    if (!focus_specific && !match_any)
        return 0;
    
    if (!focus_any && match_any)
        return 0;
    
    if (area_type == GUI_KEY_FOCUS_ANY)
        return 1;
    
    match = 0;

    switch (area_type)
    {
        case GUI_KEY_FOCUS_ANY:
            match = 1;
            break;
        case GUI_KEY_FOCUS_CHAT:
            if (focus_info->chat && focus_info->window)
            {
                snprintf (buffer_full_name, sizeof (buffer_full_name), "%s.%s",
                          gui_buffer_get_plugin_name ((focus_info->window)->buffer),
                          ((focus_info->window)->buffer)->name);
                if (string_match (buffer_full_name, area_name, 0))
                {
                    match = 1;
                }
            }
            break;
        case GUI_KEY_FOCUS_BAR:
            if (focus_info->bar_window
                && (string_match ((focus_info->bar_window)->bar->name, area_name, 0)))
            {
                match = 1;
            }
            break;
        case GUI_KEY_FOCUS_ITEM:
            if (focus_info->bar_item
                && (string_match (focus_info->bar_item, area_name, 0)))
            {
                match = 1;
            }
            break;
        case GUI_KEY_NUM_FOCUS:
            break;
    }
    
    return match;
}

/*
 * gui_key_focus_get_area: get area focus (any, chat, bar or item) and name
 */

void
gui_key_focus_get_area (const char *key, int *area_type, char **area_name)
{
    int i, length;
    char *pos_end;
    
    *area_type = GUI_KEY_FOCUS_ANY;
    *area_name = NULL;
    
    for (i = 0; i < GUI_KEY_NUM_FOCUS; i++)
    {
        length = strlen (gui_key_focus_string[i]);
        if (strncmp (key + 1, gui_key_focus_string[i], length) == 0)
        {
            if (i == GUI_KEY_FOCUS_ANY)
            {
                *area_type = i;
                *area_name = strdup ("*");
                return;
            }
            if (key[1 + length] == ':')
            {
                *area_type = i;
                *area_name = strdup ("*");
                return;
            }
            if ((key[1 + length] == '(') && key[1 + length + 1])
            {
                pos_end = strchr (key + 1 + length, ')');
                if (pos_end)
                {
                    *area_type = i;
                    *area_name = string_strndup (key + 1 + length + 1,
                                                 pos_end - key - length - 2);
                    return;
                }
            }
        }
    }
}

/*
 * gui_key_focus_command: run command according to focus
 *                        return 1 if a command was executed, otherwise 0
 */

int
gui_key_focus_command (const char *key, int context,
                       int focus_specific, int focus_any,
                       struct t_gui_focus_info *focus_info1,
                       struct t_gui_focus_info *focus_info2)
{
    struct t_gui_key *ptr_key;
    int i, errors, matching, area_type, debug;
    char *pos, *command, **commands, *area_name;
    struct t_hashtable *hashtable;
    
    debug = ((gui_cursor_debug && (context == GUI_KEY_CONTEXT_CURSOR))
             || (gui_mouse_debug && (context == GUI_KEY_CONTEXT_MOUSE)));
    
    for (ptr_key = gui_keys[context]; ptr_key;
         ptr_key = ptr_key->next_key)
    {
        if (ptr_key->key && (ptr_key->key[0] != '@'))
            continue;
        
        pos = strchr (ptr_key->key, ':');
        if (!pos)
            continue;
        
        if (gui_key_cmp (key, pos + 1, context) != 0)
            continue;
        
        gui_key_focus_get_area (ptr_key->key, &area_type, &area_name);
        if (!area_name)
            continue;
        
        matching = gui_key_focus_matching (area_type, area_name,
                                           focus_specific, focus_any,
                                           focus_info1);
        free (area_name);
        
        if (!matching)
            continue;
        
        hashtable = hook_focus_get_data (focus_info1,
                                         focus_info2,
                                         key);
        if (debug)
        {
            gui_chat_printf (NULL, "Hashtable focus: %s",
                             hashtable_get_string (hashtable,
                                                   "keys_values_sorted"));
        }
        if (string_strncasecmp (ptr_key->command, "hsignal:", 8) == 0)
        {
            if (ptr_key->command[8])
            {
                if (debug)
                {
                    gui_chat_printf (NULL, "Sending hsignal \"%s\"",
                                     ptr_key->command + 8);
                }
                hook_hsignal_send (ptr_key->command + 8, hashtable);
            }
        }
        else
        {
            command = string_replace_with_hashtable (ptr_key->command,
                                                     hashtable,
                                                     &errors);
            if (command)
            {
                if (errors == 0)
                {
                    if (debug)
                    {
                        gui_chat_printf (NULL,
                                         "Executing command: %s  (%s)",
                                         command, ptr_key->command);
                    }
                    if ((context == GUI_KEY_CONTEXT_CURSOR) && gui_cursor_debug)
                    {
                        gui_input_delete_line (gui_current_window->buffer);
                    }
                    commands = string_split_command (command, ';');
                    if (commands)
                    {
                        for (i = 0; commands[i]; i++)
                        {
                            input_data (gui_current_window->buffer, commands[i]);
                        }
                        string_free_split_command (commands);
                    }
                }
                else
                {
                    if (debug)
                    {
                        gui_chat_printf (NULL,
                                         "Command NOT executed (%s)",
                                         ptr_key->command);
                    }
                }
                free (command);
            }
        }
        if (hashtable)
            hashtable_free (hashtable);
        return 1;
    }
    
    return 0;
}

/*
 * gui_key_focus: process key pressed in cursor or mouse mode,
 *                looking for keys: "{area}key" in context "cursor" or "mouse"
 *                return 1 if a command was executed, otherwise 0
 */

int
gui_key_focus (const char *key, int context)
{
    struct t_gui_focus_info focus_info1, focus_info2, *ptr_focus_info2;
    
    ptr_focus_info2 = NULL;
    
    if (context == GUI_KEY_CONTEXT_MOUSE)
    {
        if (gui_mouse_debug)
        {
            gui_chat_printf (NULL, "Mouse: %s, (%d,%d) -> (%d,%d)",
                             key,
                             gui_mouse_event_x[0], gui_mouse_event_y[0],
                             gui_mouse_event_x[1], gui_mouse_event_y[1]);
        }
        gui_focus_get_info (gui_mouse_event_x[0], gui_mouse_event_y[0],
                            &focus_info1);
        if ((gui_mouse_event_x[0] != gui_mouse_event_x[1])
            || (gui_mouse_event_y[0] != gui_mouse_event_y[1]))
        {
            gui_focus_get_info (gui_mouse_event_x[1], gui_mouse_event_y[1],
                                &focus_info2);
            ptr_focus_info2 = &focus_info2;
        }
    }
    else
    {
        gui_focus_get_info (gui_cursor_x, gui_cursor_y, &focus_info1);
    }
    
    if (gui_key_focus_command (key, context, 1, 0,
                               &focus_info1, ptr_focus_info2))
    {
        return 1;
    }
    
    return gui_key_focus_command (key, context, 0, 1, &focus_info1,
                                  ptr_focus_info2);
}

/*
 * gui_key_pressed: process new key pressed
 *                  return: 1 if key should be added to input buffer
 *                          0 otherwise
 */

int
gui_key_pressed (const char *key_str)
{
    int i, first_key, context, length, length_key;
    struct t_gui_key *ptr_key;
    char **commands, *pos;
    
    /* add key to buffer */
    first_key = (gui_key_combo_buffer[0] == '\0');
    length = strlen (gui_key_combo_buffer);
    length_key = strlen (key_str);
    if (length + length_key + 1 <= (int)sizeof (gui_key_combo_buffer))
        strcat (gui_key_combo_buffer, key_str);
    
    /* if we are in "show mode", increase counter and return */
    if (gui_key_grab)
    {
        if (gui_key_grab_count == 0)
        {
            hook_timer (NULL, gui_key_grab_delay, 0, 1,
                        &gui_key_grab_end_timer_cb, NULL);
        }
        gui_key_grab_count++;
        return 0;
    }
    
    /* mouse event pending */
    if (gui_mouse_event_pending)
    {
        pos = strstr (gui_key_combo_buffer, "\x1B[M");
        if (pos)
        {
            pos[0] = '\0';
            gui_mouse_event_end ();
            gui_mouse_event_init ();
        }
        return 0;
    }
    
    if (strcmp (gui_key_combo_buffer, "\x01[[M") == 0)
    {
        gui_key_combo_buffer[0] = '\0';
        gui_mouse_event_init ();
        return 0;
    }
    
    ptr_key = NULL;
    
    context = gui_key_get_current_context ();
    switch (context)
    {
        case GUI_KEY_CONTEXT_DEFAULT:
            /* look for key combo in key table for current buffer */
            ptr_key = gui_key_search_part (gui_current_window->buffer,
                                           GUI_KEY_CONTEXT_DEFAULT,
                                           gui_key_combo_buffer);
            /* if key is not found for buffer, then look in general table */
            if (!ptr_key)
                ptr_key = gui_key_search_part (NULL,
                                               GUI_KEY_CONTEXT_DEFAULT,
                                               gui_key_combo_buffer);
            break;
        case GUI_KEY_CONTEXT_SEARCH:
            ptr_key = gui_key_search_part (NULL,
                                           GUI_KEY_CONTEXT_SEARCH,
                                           gui_key_combo_buffer);
            if (!ptr_key)
            {
                ptr_key = gui_key_search_part (NULL,
                                               GUI_KEY_CONTEXT_DEFAULT,
                                               gui_key_combo_buffer);
            }
            break;
        case GUI_KEY_CONTEXT_CURSOR:
            ptr_key = gui_key_search_part (NULL,
                                           GUI_KEY_CONTEXT_CURSOR,
                                           gui_key_combo_buffer);
            break;
    }
    
    /* if key is found, then execute action */
    if (ptr_key)
    {
        if (strcmp (ptr_key->key, gui_key_combo_buffer) == 0)
        {
            /* exact combo found => execute function or command */
            gui_key_combo_buffer[0] = '\0';
            if (ptr_key->command)
            {
                commands = string_split_command (ptr_key->command, ';');
                if (commands)
                {
                    for (i = 0; commands[i]; i++)
                    {
                        input_data (gui_current_window->buffer, commands[i]);
                    }
                    string_free_split_command (commands);
                }
            }
        }
        return 0;
    }
    else if (context == GUI_KEY_CONTEXT_CURSOR)
    {
        if (gui_key_focus (gui_key_combo_buffer, GUI_KEY_CONTEXT_CURSOR))
        {
            gui_key_combo_buffer[0] = '\0';
            return 0;
        }
    }
    
    gui_key_combo_buffer[0] = '\0';

    /*
     * if this is first key and not found (even partial) => return 1
     * else return 0 (= silently discard sequence of bad keys)
     */
    return first_key;
}

/*
 * gui_key_free: delete a key binding
 */

void
gui_key_free (struct t_gui_key **keys, struct t_gui_key **last_key,
              int *keys_count, struct t_gui_key *key)
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
    
    (*keys_count)--;
}

/*
 * gui_key_free_all: delete all key bindings
 */

void
gui_key_free_all (struct t_gui_key **keys, struct t_gui_key **last_key,
                       int *keys_count)
{
    while (*keys)
    {
        gui_key_free (keys, last_key, keys_count, *keys);
    }
}

/*
 * gui_key_buffer_optimize: optimize keyboard buffer size
 */

void
gui_key_buffer_optimize ()
{
    int optimal_size;
    
    optimal_size = (((gui_key_buffer_size * sizeof (int)) /
                     GUI_KEY_BUFFER_BLOCK_SIZE) *
                    GUI_KEY_BUFFER_BLOCK_SIZE) +
        GUI_KEY_BUFFER_BLOCK_SIZE;
    
    if (gui_key_buffer_alloc != optimal_size)
    {
        gui_key_buffer_alloc = optimal_size;
        gui_key_buffer = realloc (gui_key_buffer, optimal_size);
    }
}

/*
 * gui_key_buffer_reset: reset keyboard buffer
 *                       (create empty if never created before)
 */

void
gui_key_buffer_reset ()
{
    if (!gui_key_buffer)
    {
        gui_key_buffer_alloc = GUI_KEY_BUFFER_BLOCK_SIZE;
        gui_key_buffer_size = 0;
        gui_key_buffer = malloc (gui_key_buffer_alloc);
    }
    else
    {
        gui_key_buffer_size = 0;
        gui_key_buffer_optimize ();
    }
    gui_key_paste_lines = 0;
}

/*
 * gui_key_buffer_add: add a key to keyboard buffer
 */

void
gui_key_buffer_add (unsigned char key)
{
    if (!gui_key_buffer)
        gui_key_buffer_reset ();
    
    gui_key_buffer_size++;
    
    gui_key_buffer_optimize ();
    
    if (gui_key_buffer)
    {
        gui_key_buffer[gui_key_buffer_size - 1] = key;
        if ((key == 13)
            && (gui_key_buffer_size > 1)
            && (gui_key_buffer[gui_key_buffer_size - 2] != 13))
            gui_key_paste_lines++;
    }
    else
    {
        gui_key_buffer_alloc = 0;
        gui_key_buffer_size = 0;
        gui_key_paste_lines = 0;
    }
}

/*
 * gui_key_get_paste_lines: return real number of lines in buffer
 *                          if last key is not Return, then this is lines + 1
 *                          else it's lines
 */

int
gui_key_get_paste_lines ()
{
    if ((gui_key_buffer_size > 0)
        && (gui_key_buffer[gui_key_buffer_size - 1] != 13))
        return gui_key_paste_lines + 1;
    
    return gui_key_paste_lines;
}

/*
 * gui_key_paste_accept: accept paste from user
 */

void
gui_key_paste_accept ()
{
    gui_key_paste_pending = 0;
    gui_input_paste_pending_signal ();
}

/*
 * gui_key_paste_cancel: cancel paste from user (reset buffer)
 */

void
gui_key_paste_cancel ()
{
    gui_key_buffer_reset ();
    gui_key_paste_pending = 0;
    gui_input_paste_pending_signal ();
}

/*
 * gui_key_end: end keyboard (free some data)
 */

void
gui_key_end ()
{
    int i;
    
    /* free key buffer */
    if (gui_key_buffer)
        free (gui_key_buffer);
    
    for (i = 0; i < GUI_KEY_NUM_CONTEXTS; i++)
    {
        /* free keys */
        gui_key_free_all (&gui_keys[i], &last_gui_key[i],
                          &gui_keys_count[i]);
        /* free default keys */
        gui_key_free_all (&gui_default_keys[i], &last_gui_default_key[i],
                          &gui_default_keys_count[i]);
    }
}

/*
 * gui_key_hdata_key_cb: return hdata for key
 */

struct t_hdata *
gui_key_hdata_key_cb (void *data, const char *hdata_name)
{
    struct t_hdata *hdata;
    int i;
    char str_list[128];
    
    /* make C compiler happy */
    (void) data;
    
    hdata = hdata_new (NULL, hdata_name, "prev_key", "next_key");
    if (hdata)
    {
        HDATA_VAR(struct t_gui_key, key, STRING, NULL);
        HDATA_VAR(struct t_gui_key, command, STRING, NULL);
        HDATA_VAR(struct t_gui_key, prev_key, POINTER, hdata_name);
        HDATA_VAR(struct t_gui_key, next_key, POINTER, hdata_name);
        for (i = 0; i < GUI_KEY_NUM_CONTEXTS; i++)
        {
            snprintf (str_list, sizeof (str_list),
                      "gui_keys%s%s",
                      (i == GUI_KEY_CONTEXT_DEFAULT) ? "" : "_",
                      (i == GUI_KEY_CONTEXT_DEFAULT) ? "" : gui_key_context_string[i]);
            hdata_new_list(hdata, str_list, &gui_keys[i]);
            snprintf (str_list, sizeof (str_list),
                      "last_gui_key%s%s",
                      (i == GUI_KEY_CONTEXT_DEFAULT) ? "" : "_",
                      (i == GUI_KEY_CONTEXT_DEFAULT) ? "" : gui_key_context_string[i]);
            hdata_new_list(hdata, str_list, &last_gui_key[i]);
            snprintf (str_list, sizeof (str_list),
                      "gui_default_keys%s%s",
                      (i == GUI_KEY_CONTEXT_DEFAULT) ? "" : "_",
                      (i == GUI_KEY_CONTEXT_DEFAULT) ? "" : gui_key_context_string[i]);
            hdata_new_list(hdata, str_list, &gui_default_keys[i]);
            snprintf (str_list, sizeof (str_list),
                      "last_gui_default_key%s%s",
                      (i == GUI_KEY_CONTEXT_DEFAULT) ? "" : "_",
                      (i == GUI_KEY_CONTEXT_DEFAULT) ? "" : gui_key_context_string[i]);
            hdata_new_list(hdata, str_list, &last_gui_default_key[i]);
        }
    }
    return hdata;
}

/*
 * gui_key_add_to_infolist: add a key in an infolist
 *                          return 1 if ok, 0 if error
 */

int
gui_key_add_to_infolist (struct t_infolist *infolist, struct t_gui_key *key)
{
    struct t_infolist_item *ptr_item;
    char *expanded_key;
    
    if (!infolist || !key)
        return 0;
    
    ptr_item = infolist_new_item (infolist);
    if (!ptr_item)
        return 0;
    
    if (!infolist_new_var_string (ptr_item, "key_internal", key->key))
        return 0;
    expanded_key = gui_key_get_expanded_name (key->key);
    if (expanded_key)
    {
        if (!infolist_new_var_string (ptr_item, "key", expanded_key))
            return 0;
        free (expanded_key);
    }
    if (!infolist_new_var_string (ptr_item, "command", key->command))
        return 0;
    
    return 1;
}

/*
 * gui_key_print_log: print key infos in log (usually for crash dump)
 */

void
gui_key_print_log (struct t_gui_buffer *buffer)
{
    struct t_gui_key *ptr_key;
    int i;
    
    if (buffer)
    {
        log_printf ("    keys . . . . . . . . : 0x%lx", buffer->keys);
        log_printf ("    last_key . . . . . . : 0x%lx", buffer->last_key);
        log_printf ("    keys_count . . . . . : %d",    buffer->keys_count);
        for (ptr_key = buffer->keys; ptr_key; ptr_key = ptr_key->next_key)
        {
            log_printf ("");
            log_printf ("    [key (addr:0x%lx)]", ptr_key);
            log_printf ("      key. . . . . . . . : '%s'",  ptr_key->key);
            log_printf ("      command. . . . . . : '%s'",  ptr_key->command);
            log_printf ("      prev_key . . . . . : 0x%lx", ptr_key->prev_key);
            log_printf ("      next_key . . . . . : 0x%lx", ptr_key->next_key);
        }
    }
    else
    {
        for (i = 0; i < GUI_KEY_NUM_CONTEXTS; i++)
        {
            log_printf ("[keys for context: %s]", gui_key_context_string[i]);
            log_printf ("");
            log_printf ("  keys . . . . . . . . : 0x%lx", gui_keys[i]);
            log_printf ("  last_key . . . . . . : 0x%lx", last_gui_key[i]);
            log_printf ("  keys_count . . . . . : %d",    gui_keys_count[i]);
            
            for (ptr_key = gui_keys[i]; ptr_key; ptr_key = ptr_key->next_key)
            {
                log_printf ("");
                log_printf ("[key (addr:0x%lx)]", ptr_key);
                log_printf ("  key. . . . . . . . : '%s'",  ptr_key->key);
                log_printf ("  command. . . . . . : '%s'",  ptr_key->command);
                log_printf ("  prev_key . . . . . : 0x%lx", ptr_key->prev_key);
                log_printf ("  next_key . . . . . : 0x%lx", ptr_key->next_key);
            }
        }
    }
}
