/*
 * gui-key.c - keyboard functions (used by all GUI)
 *
 * Copyright (C) 2003-2023 Sébastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <https://www.gnu.org/licenses/>.
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
#include "../core/wee-config.h"
#include "../core/wee-eval.h"
#include "../core/wee-hashtable.h"
#include "../core/wee-hdata.h"
#include "../core/wee-hook.h"
#include "../core/wee-infolist.h"
#include "../core/wee-input.h"
#include "../core/wee-list.h"
#include "../core/wee-log.h"
#include "../core/wee-string.h"
#include "../core/wee-utf8.h"
#include "../plugins/plugin.h"
#include "gui-key.h"
#include "gui-bar.h"
#include "gui-bar-item.h"
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
int gui_key_paste_bracketed = 0;    /* bracketed paste mode detected        */
struct t_hook *gui_key_paste_bracketed_timer = NULL;
                                    /* timer for bracketed paste            */
int gui_key_paste_lines = 0;        /* number of lines for pending paste    */

time_t gui_key_last_activity_time = 0; /* last activity time (key)          */


/*
 * Initializes keyboard.
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
 * Searches for a context by name.
 *
 * Returns index of context in enum t_gui_key_context, -1 if not found.
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
 * Gets current context.
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
 * Initializes "grab" mode.
 */

void
gui_key_grab_init (int grab_command, const char *delay)
{
    long milliseconds;
    char *error;

    gui_key_grab = 1;
    gui_key_grab_count = 0;
    gui_key_grab_command = grab_command;

    gui_key_grab_delay = CONFIG_INTEGER(config_look_key_grab_delay);
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
 * Inserts grabbed key in input buffer.
 */

int
gui_key_grab_end_timer_cb (const void *pointer, void *data,
                           int remaining_calls)
{
    char *expanded_key, *expanded_key2;
    struct t_gui_key *ptr_key;

    /* make C compiler happy */
    (void) pointer;
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
        if (!utf8_is_valid (expanded_key, -1, NULL))
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
            gui_input_insert_string (gui_current_window->buffer, expanded_key);
            if (gui_key_grab_command)
            {
                /* add command bound to key (if found) */
                ptr_key = gui_key_search (gui_keys[GUI_KEY_CONTEXT_DEFAULT],
                                          gui_key_combo_buffer);
                if (ptr_key)
                {
                    gui_input_insert_string (gui_current_window->buffer, " ");
                    gui_input_insert_string (gui_current_window->buffer, ptr_key->command);
                }
            }
            gui_input_text_changed_modifier_and_signal (gui_current_window->buffer,
                                                        1, /* save undo */
                                                        1); /* stop completion */
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
 * Gets internal code from user key name.
 *
 * For example: returns '\x01'+'R' for "ctrl-R"
 *
 * Note: result must be freed after use.
 */

char *
gui_key_get_internal_code (const char *key)
{
    char *result, *result2;

    if ((key[0] == '@') && strchr (key, ':'))
        return strdup (key);

    result = malloc (strlen (key) + 1);
    if (!result)
        return NULL;

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
        else if (strncmp (key, "space", 5) == 0)
        {
            strcat (result, " ");
            key += 5;
        }
        else
        {
            strncat (result, key, 1);
            key++;
        }
    }

    result2 = strdup (result);
    free (result);

    return result2;
}

/*
 * Gets expanded name from internal key code.
 *
 * For example: return "ctrl-R" for "\x01+R".
 *
 * Note: result must be freed after use.
 */

char *
gui_key_get_expanded_name (const char *key)
{
    char *result, *result2;

    if (!key)
        return NULL;

    result = malloc ((strlen (key) * 5) + 1);
    if (!result)
        return NULL;

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
        else if (key[0] == ' ')
        {
            strcat (result, "space");
            key++;
        }
        else
        {
            strncat (result, key, 1);
            key++;
        }
    }

    result2 = strdup (result);
    free (result);

    return result2;
}

/*
 * Searches for position of a key (to keep keys sorted).
 */

struct t_gui_key *
gui_key_find_pos (struct t_gui_key *keys, struct t_gui_key *key)
{
    struct t_gui_key *ptr_key;

    for (ptr_key = keys; ptr_key; ptr_key = ptr_key->next_key)
    {
        if ((key->score < ptr_key->score)
            || ((key->score == ptr_key->score)
                && (strcmp (key->key, ptr_key->key) < 0)))
        {
            return ptr_key;
        }
    }
    return NULL;
}

/*
 * Inserts key into sorted list.
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
 * Sets area type and name.
 *
 * For example: "bar(nicklist)" returns:
 *   type: 2 (bar)
 *   name: "nicklist"
 *
 * Warning: if no area is found, values are NOT set.
 */

void
gui_key_set_area_type_name (const char *area,
                            int *area_type, char **area_name)
{
    int focus, length;
    char *pos_end;

    for (focus = 0; focus < GUI_KEY_NUM_FOCUS; focus++)
    {
        length = strlen (gui_key_focus_string[focus]);
        if (strncmp (area, gui_key_focus_string[focus], length) == 0)
        {
            if (focus == GUI_KEY_FOCUS_ANY)
            {
                *area_type = focus;
                *area_name = strdup ("*");
                break;
            }
            if (!area[length])
            {
                *area_type = focus;
                *area_name = strdup ("*");
                break;
            }
            if ((area[length] == '(') && area[length + 1])
            {
                pos_end = strchr (area + length, ')');
                if (pos_end)
                {
                    *area_type = focus;
                    *area_name = string_strndup (area + length + 1,
                                                 pos_end - area - length - 1);
                    break;
                }
            }
        }
    }
}

/*
 * Sets areas types (any, chat, bar or item) and names for a key.
 */

void
gui_key_set_areas (struct t_gui_key *key)
{
    int area;
    char *pos_colon, *pos_area2, *areas[2];

    for (area = 0; area < 2; area++)
    {
        key->area_type[area] = GUI_KEY_FOCUS_ANY;
        key->area_name[area] = NULL;
    }
    key->area_key = NULL;

    if (key->key[0] != '@')
        return;

    areas[0] = NULL;
    areas[1] = NULL;

    pos_colon = strchr (key->key + 1, ':');
    if (!pos_colon)
        return;
    pos_area2 = strchr (key->key + 1, '>');

    key->area_key = strdup (pos_colon + 1);

    if (!pos_area2 || (pos_area2 > pos_colon))
        areas[0] = string_strndup (key->key + 1, pos_colon - key->key - 1);
    else
    {
        if (pos_area2 > key->key + 1)
            areas[0] = string_strndup (key->key + 1, pos_area2 - key->key - 1);
        areas[1] = string_strndup (pos_area2 + 1, pos_colon - pos_area2 - 1);
    }

    for (area = 0; area < 2; area++)
    {
        if (!areas[area])
        {
            key->area_name[area] = strdup ("*");
            continue;
        }
        gui_key_set_area_type_name (areas[area],
                                    &(key->area_type[area]),
                                    &(key->area_name[area]));
    }

    if (areas[0])
        free (areas[0]);
    if (areas[1])
        free (areas[1]);
}

/*
 * Computes a score key for sorting keys and set it in key (high score == at the
 * end of list).
 */

void
gui_key_set_score (struct t_gui_key *key)
{
    int score, bonus, area;

    score = 0;
    bonus = 8;

    key->score = score;

    if (key->key[0] != '@')
        return;

    /* basic score for key with area */
    score |= 1 << bonus;
    bonus--;

    /* add score for each area type */
    for (area = 0; area < 2; area++)
    {
        /* bonus if area type is "any" */
        if (key->area_name[area]
            && (key->area_type[area] == GUI_KEY_FOCUS_ANY))
        {
            score |= 1 << bonus;
        }
        bonus--;
    }

    /* add score for each area name */
    for (area = 0; area < 2; area++)
    {
        /* bonus if area name is "*" */
        if (key->area_name[area]
            && (strcmp (key->area_name[area], "*") == 0))
        {
            score |= 1 << bonus;
        }
        bonus--;
    }

    key->score = score;
}

/*
 * Checks if a key is safe or not: a safe key begins always with the "meta" or
 * "ctrl" code (except "@" allowed in cursor/mouse contexts).
 *
 * Returns:
 *   1: key is safe
 *   0: key is NOT safe
 */

int
gui_key_is_safe (int context, const char *key)
{
    char *internal_code;
    int rc;

    /* "@" is allowed at beginning for cursor/mouse contexts */
    if ((key[0] == '@')
        && ((context == GUI_KEY_CONTEXT_CURSOR)
            || (context == GUI_KEY_CONTEXT_MOUSE)))
    {
        return 1;
    }

    /* check that first char is a ctrl or meta code */
    internal_code = gui_key_get_internal_code (key);
    if (!internal_code)
        return 0;
    rc = (internal_code[0] == '\x01') ? 1 : 0;
    free (internal_code);

    return rc;
}

/*
 * Adds a new key in keys list.
 *
 * If buffer is not null, then key is specific to buffer, otherwise it's general
 * key (for most keys).
 *
 * Returns pointer to new key, NULL if error.
 */

struct t_gui_key *
gui_key_new (struct t_gui_buffer *buffer, int context, const char *key,
             const char *command)
{
    struct t_gui_key *new_key;
    char *expanded_name;

    if (!key || !command)
        return NULL;

    new_key = malloc (sizeof (*new_key));
    if (!new_key)
        return NULL;

    new_key->key = gui_key_get_internal_code (key);
    if (!new_key->key)
        new_key->key = strdup (key);
    new_key->command = strdup (command);
    gui_key_set_areas (new_key);
    gui_key_set_score (new_key);

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

    (void) hook_signal_send ("key_bind",
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

    return new_key;
}

/*
 * Searches for a key.
 *
 * Returns pointer to key found, NULL if not found.
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
 * Compares two keys.
 */

int
gui_key_cmp (const char *key, const char *search, int context)
{
    int diff;

    if (context == GUI_KEY_CONTEXT_MOUSE)
        return (string_match (key, search, 1)) ? 0 : 1;

    while (search[0])
    {
        diff = string_charcmp (key, search);
        if (diff != 0)
            return diff;
        key = utf8_next_char (key);
        search = utf8_next_char (search);
    }

    return 0;
}

/*
 * Searches for a key (maybe part of string).
 *
 * Returns pointer to key found, NULL if not found.
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
 * Binds a key to a command.
 *
 * If buffer is not null, then key is specific to buffer otherwise it's general
 * key (for most keys).
 *
 * If key already exists, it is removed then added again with new value.
 *
 * Returns pointer to new key, NULL if error.
 */

struct t_gui_key *
gui_key_bind (struct t_gui_buffer *buffer, int context, const char *key,
              const char *command)
{
    if (!key || !command || (string_strcasecmp (key, "meta-") == 0))
        return NULL;

    gui_key_unbind (buffer, context, key);

    return gui_key_new (buffer, context, key, command);
}

/*
 * Binds keys in hashtable.
 */

void
gui_key_bind_plugin_hashtable_map_cb (void *data,
                                      struct t_hashtable *hashtable,
                                      const void *key, const void *value)
{
    int *user_data;
    struct t_gui_key *ptr_key;
    char *internal_code;

    /* make C compiler happy */
    (void) hashtable;

    user_data = (int *)data;

    if (user_data && key && value)
    {
        /* ignore special key "__quiet" */
        if (strcmp (key, "__quiet") == 0)
            return;

        internal_code = gui_key_get_internal_code (key);
        if (internal_code)
        {
            ptr_key = gui_key_search (gui_keys[user_data[0]], internal_code);
            if (!ptr_key)
            {
                if (gui_key_new (NULL, user_data[0], key, value))
                    user_data[1]++;
            }
            free (internal_code);
        }
    }
}

/*
 * Creates many keys using a hashtable (used by plugins only).
 *
 * If key already exists, it is NOT changed (plugins should never overwrite user
 * keys).
 *
 * Returns number of keys added.
 */

int
gui_key_bind_plugin (const char *context, struct t_hashtable *keys)
{
    int data[2];
    const char *ptr_quiet;

    data[0] = gui_key_search_context (context);
    if (data[0] < 0)
        return 0;

    data[1] = 0;

    ptr_quiet = hashtable_get (keys, "__quiet");
    gui_key_verbose = (ptr_quiet) ? 0 : 1;

    hashtable_map (keys, &gui_key_bind_plugin_hashtable_map_cb, data);

    gui_key_verbose = 0;

    return data[1];
}

/*
 * Removes one key binding.
 *
 * Returns:
 *   1: key removed
 *   0: key not removed
 */

int
gui_key_unbind (struct t_gui_buffer *buffer, int context, const char *key)
{
    struct t_gui_key *ptr_key;
    char *internal_code;

    internal_code = gui_key_get_internal_code (key);
    if (!internal_code)
        return 0;

    ptr_key = gui_key_search ((buffer) ? buffer->keys : gui_keys[context],
                              (internal_code) ? internal_code : key);
    free (internal_code);
    if (ptr_key)
    {
        if (buffer)
        {
            gui_key_free (&buffer->keys, &buffer->last_key,
                          &buffer->keys_count, ptr_key);
        }
        else
        {
            if (gui_key_verbose)
            {
                gui_chat_printf (NULL,
                                 _("Key \"%s\" unbound (context: \"%s\")"),
                                 key,
                                 gui_key_context_string[context]);
            }
            gui_key_free (&gui_keys[context], &last_gui_key[context],
                          &gui_keys_count[context], ptr_key);
        }
        (void) hook_signal_send ("key_unbind",
                                 WEECHAT_HOOK_SIGNAL_STRING, (char *)key);
        return 1;
    }

    return 0;
}

/*
 * Removes one or more key binding(s) (used by plugins only).
 *
 * Returns number of keys removed.
 */

int
gui_key_unbind_plugin (const char *context, const char *key)
{
    int ctxt, num_keys, area_type;
    char *area_name;
    struct t_gui_key *ptr_key, *ptr_next_key;

    ctxt = gui_key_search_context (context);
    if (ctxt < 0)
        return 0;

    if (strncmp (key, "quiet:", 6) == 0)
    {
        key += 6;
    }
    else
    {
        gui_key_verbose = 1;
    }

    if (strncmp (key, "area:", 5) == 0)
    {
        num_keys = 0;
        area_type = -1;
        area_name = NULL;
        gui_key_set_area_type_name (key + 5, &area_type, &area_name);
        if (area_name)
        {
            ptr_key = gui_keys[ctxt];
            while (ptr_key)
            {
                ptr_next_key = ptr_key->next_key;

                if (((ptr_key->area_type[0] == area_type)
                     && ptr_key->area_name[0]
                     && (strcmp (ptr_key->area_name[0], area_name) == 0))
                    || ((ptr_key->area_type[1] == area_type)
                        && ptr_key->area_name[1]
                        && (strcmp (ptr_key->area_name[1], area_name) == 0)))
                {
                    num_keys += gui_key_unbind (NULL, ctxt, ptr_key->key);
                }

                ptr_key = ptr_next_key;
            }
            free (area_name);
        }
    }
    else
    {
        num_keys = gui_key_unbind (NULL, ctxt, key);
    }

    gui_key_verbose = 0;

    return num_keys;
}

/*
 * Checks if area in key is matching focus area on screen (cursor/mouse).
 *
 * Returns:
 *   1: area in key is matching focus area
 *   0: area in key is not matching focus area
 */

int
gui_key_focus_matching (struct t_gui_key *key,
                        struct t_hashtable **hashtable_focus)
{
    int match[2], area;
    const char *chat, *buffer_full_name, *bar_name, *bar_item_name;

    for (area = 0; area < 2; area++)
    {
        match[area] = 0;
        switch (key->area_type[area])
        {
            case GUI_KEY_FOCUS_ANY:
                match[area] = 1;
                break;
            case GUI_KEY_FOCUS_CHAT:
                chat = hashtable_get (hashtable_focus[area], "_chat");
                buffer_full_name = hashtable_get (hashtable_focus[area],
                                                  "_buffer_full_name");
                if (chat && (strcmp (chat, "1") == 0)
                    && buffer_full_name && buffer_full_name[0])
                {
                    if (string_match (buffer_full_name, key->area_name[area], 0))
                        match[area] = 1;
                }
                break;
            case GUI_KEY_FOCUS_BAR:
                bar_name = hashtable_get (hashtable_focus[area], "_bar_name");
                if (bar_name && bar_name[0]
                    && (string_match (bar_name, key->area_name[area], 0)))
                {
                    match[area] = 1;
                }
                break;
            case GUI_KEY_FOCUS_ITEM:
                bar_item_name = hashtable_get (hashtable_focus[area],
                                               "_bar_item_name");
                if (bar_item_name && bar_item_name[0]
                    && (string_match (bar_item_name, key->area_name[area], 0)))
                {
                    match[area] = 1;
                }
                break;
            case GUI_KEY_NUM_FOCUS:
                break;
        }
    }

    return match[0] && match[1];
}

/*
 * Runs command according to focus.
 *
 * Returns:
 *   1: command was executed
 *   0: command was not executed
 */

int
gui_key_focus_command (const char *key, int context,
                       struct t_hashtable **hashtable_focus)
{
    struct t_gui_key *ptr_key;
    int i, matching, debug, rc;
    unsigned long value;
    char *command, **commands;
    const char *str_buffer;
    struct t_hashtable *hashtable;
    struct t_weelist *list_keys;
    struct t_weelist_item *ptr_item;
    struct t_gui_buffer *ptr_buffer;

    debug = 0;
    if (gui_cursor_debug && (context == GUI_KEY_CONTEXT_CURSOR))
        debug = gui_cursor_debug;
    else if (gui_mouse_debug && (context == GUI_KEY_CONTEXT_MOUSE))
        debug = gui_mouse_debug;

    for (ptr_key = gui_keys[context]; ptr_key;
         ptr_key = ptr_key->next_key)
    {
        /* ignore key if it has not area name or key for area */
        if (!ptr_key->area_name[0] || !ptr_key->area_key)
            continue;

        /* the special command "-" is used to ignore key */
        if (strcmp (ptr_key->command, "-") == 0)
            continue;

        /* ignore key if key for area is not matching */
        if (gui_key_cmp (key, ptr_key->area_key, context) != 0)
            continue;

        /* ignore mouse event if not explicit requested */
        if ((context == GUI_KEY_CONTEXT_MOUSE) &&
            (string_match (key, "*-event-*", 1) != string_match (ptr_key->area_key, "*-event-*", 1)))
            continue;

        /* check if focus is matching with key */
        matching = gui_key_focus_matching (ptr_key, hashtable_focus);
        if (!matching)
            continue;

        hashtable = hook_focus_get_data (hashtable_focus[0],
                                         hashtable_focus[1]);
        if (!hashtable)
            continue;

        /* get buffer */
        ptr_buffer = gui_current_window->buffer;
        str_buffer = hashtable_get (hashtable, "_buffer");
        if (str_buffer && str_buffer[0])
        {
            rc = sscanf (str_buffer, "%lx", &value);
            if ((rc != EOF) && (rc != 0))
                ptr_buffer = (struct t_gui_buffer *)value;
        }
        if (!ptr_buffer)
            continue;

        if ((context == GUI_KEY_CONTEXT_CURSOR) && gui_cursor_debug)
        {
            gui_input_delete_line (gui_current_window->buffer);
        }

        if (debug > 1)
        {
            gui_chat_printf (NULL, _("Hashtable focus:"));
            list_keys = hashtable_get_list_keys (hashtable);
            if (list_keys)
            {
                for (ptr_item = list_keys->items; ptr_item;
                     ptr_item = ptr_item->next_item)
                {
                    gui_chat_printf (NULL, "  %s: \"%s\"",
                                     ptr_item->data,
                                     hashtable_get (hashtable, ptr_item->data));
                }
                weelist_free (list_keys);
            }
        }
        if (debug)
        {
            gui_chat_printf (NULL, _("Command for key: \"%s\""),
                             ptr_key->command);
        }
        if (ptr_key->command)
        {
            commands = string_split_command (ptr_key->command, ';');
            if (commands)
            {
                for (i = 0; commands[i]; i++)
                {
                    if (string_strncasecmp (commands[i], "hsignal:", 8) == 0)
                    {
                        if (commands[i][8])
                        {
                            if (debug)
                            {
                                gui_chat_printf (NULL,
                                                 _("Sending hsignal: \"%s\""),
                                                 commands[i] + 8);
                            }
                            (void) hook_hsignal_send (commands[i] + 8,
                                                      hashtable);
                        }
                    }
                    else
                    {
                        command = eval_expression (commands[i], NULL,
                                                   hashtable, NULL);
                        if (command)
                        {
                            if (debug)
                            {
                                gui_chat_printf (NULL,
                                                 _("Executing command: \"%s\" "
                                                   "on buffer \"%s\""),
                                                 command,
                                                 ptr_buffer->full_name);
                            }
                            (void) input_data (ptr_buffer, command, NULL);
                            free (command);
                        }
                    }
                }
                string_free_split (commands);
            }
        }
        hashtable_free (hashtable);
        return 1;
    }

    return 0;
}

/*
 * Processes key pressed in cursor or mouse mode, looking for keys: "{area}key"
 * in context "cursor" or "mouse".
 *
 * Returns:
 *   1: command was executed
 *   0: command was not executed
 */

int
gui_key_focus (const char *key, int context)
{
    struct t_gui_focus_info *focus_info1, *focus_info2;
    struct t_hashtable *hashtable_focus[2];
    int rc;

    rc = 0;
    focus_info1 = NULL;
    focus_info2 = NULL;
    hashtable_focus[0] = NULL;
    hashtable_focus[1] = NULL;

    if (context == GUI_KEY_CONTEXT_MOUSE)
    {
        focus_info1 = gui_focus_get_info (gui_mouse_event_x[0],
                                          gui_mouse_event_y[0]);
        if (!focus_info1)
            goto end;
        hashtable_focus[0] = gui_focus_to_hashtable (focus_info1, key);
        if (!hashtable_focus[0])
            goto end;
        if ((gui_mouse_event_x[0] != gui_mouse_event_x[1])
            || (gui_mouse_event_y[0] != gui_mouse_event_y[1]))
        {
            focus_info2 = gui_focus_get_info (gui_mouse_event_x[1],
                                              gui_mouse_event_y[1]);
            if (!focus_info2)
                goto end;
            hashtable_focus[1] = gui_focus_to_hashtable (focus_info2, key);
            if (!hashtable_focus[1])
                goto end;
        }
        if (gui_mouse_debug)
        {
            gui_chat_printf (NULL, "Mouse: %s, (%d,%d) -> (%d,%d)",
                             key,
                             gui_mouse_event_x[0], gui_mouse_event_y[0],
                             gui_mouse_event_x[1], gui_mouse_event_y[1]);
        }
    }
    else
    {
        focus_info1 = gui_focus_get_info (gui_cursor_x, gui_cursor_y);
        if (!focus_info1)
            goto end;
        hashtable_focus[0] = gui_focus_to_hashtable (focus_info1, key);
        if (!hashtable_focus[0])
            goto end;
    }

    rc = gui_key_focus_command (key, context, hashtable_focus);

end:
    if (focus_info1)
        gui_focus_free_info (focus_info1);
    if (focus_info2)
        gui_focus_free_info (focus_info2);
    if (hashtable_focus[0])
        hashtable_free (hashtable_focus[0]);
    if (hashtable_focus[1])
        hashtable_free (hashtable_focus[1]);

    return rc;
}

/*
 * Checks if the given combo of keys is complete or not.
 * It's not complete if the end of string is in the middle of a key
 * (for example meta- without the following char/key).
 *
 * Returns:
 *   0: key is incomplete (truncated)
 *   1: key is complete
 */

int
gui_key_is_complete (const char *key)
{
    int length;

    if (!key || !key[0])
        return 1;

    length = strlen (key);

    if (((length >= 1) && (strcmp (key + length - 1, "\x01") == 0))
        || ((length >= 2) && (strcmp (key + length - 2, "\x01[") == 0))
        || ((length >= 3) && (strcmp (key + length - 3, "\x01[[") == 0)))
        return 0;

    return 1;
}

/*
 * Processes a new key pressed.
 *
 * Returns:
 *   1: key must be added to input buffer
 *   0: key must not be added to input buffer
 */

int
gui_key_pressed (const char *key_str)
{
    int i, first_key, context, length, length_key, rc, signal_sent;
    struct t_gui_key *ptr_key;
    char *pos, signal_name[128], **commands;

    signal_sent = 0;

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
                        &gui_key_grab_end_timer_cb, NULL, NULL);
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
            if (!gui_window_bare_display)
                gui_mouse_event_end ();
            gui_mouse_event_init ();
        }
        return 0;
    }

    if (strstr (gui_key_combo_buffer, "\x01[[M"))
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
            /* exact combo found => execute command */
            snprintf (signal_name, sizeof (signal_name),
                      "key_combo_%s", gui_key_context_string[context]);
            rc = hook_signal_send (signal_name,
                                   WEECHAT_HOOK_SIGNAL_STRING,
                                   gui_key_combo_buffer);
            gui_key_combo_buffer[0] = '\0';
            if ((rc != WEECHAT_RC_OK_EAT) && ptr_key->command)
            {
                commands = string_split_command (ptr_key->command, ';');
                if (commands)
                {
                    for (i = 0; commands[i]; i++)
                    {
                        (void) input_data (gui_current_window->buffer,
                                           commands[i], NULL);
                    }
                    string_free_split (commands);
                }
            }
        }
        return 0;
    }
    else if (context == GUI_KEY_CONTEXT_CURSOR)
    {
        signal_sent = 1;
        snprintf (signal_name, sizeof (signal_name),
                  "key_combo_%s", gui_key_context_string[context]);
        if (hook_signal_send (signal_name,
                              WEECHAT_HOOK_SIGNAL_STRING,
                              gui_key_combo_buffer) == WEECHAT_RC_OK_EAT)
        {
            gui_key_combo_buffer[0] = '\0';
            return 0;
        }
        if (gui_key_focus (gui_key_combo_buffer, GUI_KEY_CONTEXT_CURSOR))
        {
            gui_key_combo_buffer[0] = '\0';
            return 0;
        }
    }

    if (!signal_sent)
    {
        snprintf (signal_name, sizeof (signal_name),
                  "key_combo_%s", gui_key_context_string[context]);
        if (hook_signal_send (signal_name,
                              WEECHAT_HOOK_SIGNAL_STRING,
                              gui_key_combo_buffer) == WEECHAT_RC_OK_EAT)
        {
            gui_key_combo_buffer[0] = '\0';
            return 0;
        }
    }

    if (gui_key_is_complete (gui_key_combo_buffer))
        gui_key_combo_buffer[0] = '\0';

    /*
     * if this is first key and not found (even partial) => return 1
     * else return 0 (= silently discard sequence of bad keys)
     */
    return first_key;
}

/*
 * Deletes a key binding.
 */

void
gui_key_free (struct t_gui_key **keys, struct t_gui_key **last_key,
              int *keys_count, struct t_gui_key *key)
{
    int i;

    if (!key)
        return;

    /* free memory */
    if (key->key)
        free (key->key);
    for (i = 0; i < 2; i++)
    {
        if (key->area_name[i])
            free (key->area_name[i]);
    }
    if (key->area_key)
        free (key->area_key);
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
 * Deletes all key bindings.
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
 * Optimizes keyboard buffer size.
 */

void
gui_key_buffer_optimize ()
{
    int optimal_size, *gui_key_buffer2;

    optimal_size = (((gui_key_buffer_size * sizeof (int)) /
                     GUI_KEY_BUFFER_BLOCK_SIZE) *
                    GUI_KEY_BUFFER_BLOCK_SIZE) +
        GUI_KEY_BUFFER_BLOCK_SIZE;

    if (gui_key_buffer_alloc != optimal_size)
    {
        gui_key_buffer_alloc = optimal_size;
        gui_key_buffer2 = realloc (gui_key_buffer, optimal_size);
        if (!gui_key_buffer2)
        {
            if (gui_key_buffer)
            {
                free (gui_key_buffer);
                gui_key_buffer = NULL;
            }
            return;
        }
        gui_key_buffer = gui_key_buffer2;
    }
}

/*
 * Resets keyboard buffer (create empty if never created before).
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
 * Adds a key to keyboard buffer.
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
        if (((key == '\r') || (key == '\n'))
            && (gui_key_buffer_size > 1)
            && (gui_key_buffer[gui_key_buffer_size - 2] != '\r')
            && (gui_key_buffer[gui_key_buffer_size - 2] != '\n'))
        {
            gui_key_paste_lines++;
        }
    }
    else
    {
        gui_key_buffer_alloc = 0;
        gui_key_buffer_size = 0;
        gui_key_paste_lines = 0;
    }
}

/*
 * Searches for a string in gui_key_buffer (array of integers).
 *
 * Argument start_index must be >= 0.
 * If max_index is negative, the search is until end of buffer.
 *
 * Returns index for string found in gui_key_buffer (not from "start_index" but
 * from beginning of gui_key_buffer), or -1 if string is not found.
 */

int
gui_key_buffer_search (int start_index, int max_index, const char *string)
{
    int i, j, length, found;

    if ((gui_key_buffer_size == 0) || !string || !string[0])
        return -1;

    length = strlen (string);

    if (gui_key_buffer_size < length)
        return -1;

    if (max_index < 0)
        max_index = gui_key_buffer_size - length;
    else if (max_index > gui_key_buffer_size - length)
        max_index = gui_key_buffer_size - length;

    for (i = start_index; i <= max_index; i++)
    {
        found = 1;
        for (j = 0; j < length; j++)
        {
            if (gui_key_buffer[i + j] != string[j])
            {
                found = 0;
                break;
            }
        }
        if (found)
            return i;
    }

    /* string not found */
    return -1;
}

/*
 * Removes some chars from gui_key_buffer.
 */

void
gui_key_buffer_remove (int index, int number)
{
    int i;

    for (i = index; i < gui_key_buffer_size - number; i++)
    {
        gui_key_buffer[i] = gui_key_buffer[i + number];
    }
    gui_key_buffer_size -= number;
}

/*
 * Removes final newline at end of paste if there is only one line to paste.
 */

void
gui_key_paste_remove_newline ()
{
    if ((gui_key_paste_lines <= 1)
        && (gui_key_buffer_size > 0)
        && ((gui_key_buffer[gui_key_buffer_size - 1] == '\r')
            || (gui_key_buffer[gui_key_buffer_size - 1] == '\n')))
    {
        gui_key_buffer_size--;
        gui_key_paste_lines = 0;
    }
}

/*
 * Replaces tabs by spaces in paste.
 */

void
gui_key_paste_replace_tabs ()
{
    int i;

    for (i = 0; i < gui_key_buffer_size; i++)
    {
        if (gui_key_buffer[i] == '\t')
            gui_key_buffer[i] = ' ';
    }
}

/*
 * Starts paste of text.
 */

void
gui_key_paste_start ()
{
    gui_key_paste_remove_newline ();
    gui_key_paste_replace_tabs ();
    gui_key_paste_pending = 1;
    gui_input_paste_pending_signal ();
}

/*
 * Returns real number of lines in buffer.
 *
 * Returns number of lines (lines+1 if last key is not return).
 */

int
gui_key_get_paste_lines ()
{
    int length;

    length = gui_key_buffer_size;

    if (length >= GUI_KEY_BRACKETED_PASTE_LENGTH)
    {
        if (gui_key_buffer_search (length - GUI_KEY_BRACKETED_PASTE_LENGTH, -1,
                                   GUI_KEY_BRACKETED_PASTE_END) >= 0)
            length -= GUI_KEY_BRACKETED_PASTE_LENGTH;
    }

    if ((length > 0)
        && (gui_key_buffer[length - 1] != '\r')
        && (gui_key_buffer[length - 1] != '\n'))
    {
        return gui_key_paste_lines + 1;
    }

    return (gui_key_paste_lines > 0) ? gui_key_paste_lines : 1;
}

/*
 * Checks pasted lines: if more than N lines, then enables paste mode and ask
 * confirmation to user (ctrl-Y=paste, ctrl-N=cancel) (N is option
 * weechat.look.paste_max_lines).
 *
 * Returns:
 *   1: paste mode has been enabled
 *   0: paste mode has not been enabled
 */

int
gui_key_paste_check (int bracketed_paste)
{
    int max_lines;

    max_lines = CONFIG_INTEGER(config_look_paste_max_lines);

    if ((max_lines < 0)
        || !gui_bar_item_used_in_at_least_one_bar (gui_bar_item_names[GUI_BAR_ITEM_INPUT_PASTE],
                                                   0, 1))
    {
        return 0;
    }

    if (!bracketed_paste && (max_lines == 0))
        max_lines = 1;
    if (gui_key_get_paste_lines () > max_lines)
    {
        /* ask user what to do */
        gui_key_paste_start ();
        return 1;
    }

    return 0;
}

/*
 * Callback for bracketed paste timer.
 */

int
gui_key_paste_bracketed_timer_cb (const void *pointer, void *data,
                                  int remaining_calls)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) remaining_calls;

    gui_key_paste_bracketed_timer = NULL;

    if (gui_key_paste_bracketed)
        gui_key_paste_bracketed_stop ();

    return WEECHAT_RC_OK;
}

/*
 * Removes timer for bracketed paste.
 */

void
gui_key_paste_bracketed_timer_remove ()
{
    if (gui_key_paste_bracketed_timer)
    {
        unhook (gui_key_paste_bracketed_timer);
        gui_key_paste_bracketed_timer = NULL;
    }
}

/*
 * Adds timer for bracketed paste.
 */

void
gui_key_paste_bracketed_timer_add ()
{
    gui_key_paste_bracketed_timer_remove ();
    gui_key_paste_bracketed_timer = hook_timer (
        NULL,
        CONFIG_INTEGER(config_look_paste_bracketed_timer_delay) * 1000,
        0, 1,
        &gui_key_paste_bracketed_timer_cb, NULL, NULL);
}

/*
 * Starts bracketed paste of text (ESC[200~ detected).
 */

void
gui_key_paste_bracketed_start ()
{
    gui_key_paste_bracketed = 1;
    gui_key_paste_bracketed_timer_add ();
}

/*
 * Stops bracketed paste of text (ESC[201~ detected or timeout while waiting for
 * this code).
 */

void
gui_key_paste_bracketed_stop ()
{
    gui_key_paste_check (1);
    gui_key_paste_bracketed = 0;
}

/*
 * Accepts paste from user.
 */

void
gui_key_paste_accept ()
{
    /*
     * add final newline if there is not in pasted text
     * (for at least 2 lines pasted)
     */
    if (CONFIG_BOOLEAN(config_look_paste_auto_add_newline)
        && (gui_key_get_paste_lines () > 1)
        && (gui_key_buffer_size > 0)
        && (gui_key_buffer[gui_key_buffer_size - 1] != '\r')
        && (gui_key_buffer[gui_key_buffer_size - 1] != '\n'))
    {
        gui_key_buffer_add ('\n');
    }

    gui_key_paste_pending = 0;
    gui_input_paste_pending_signal ();
}

/*
 * Cancels paste from user (resets buffer).
 */

void
gui_key_paste_cancel ()
{
    gui_key_buffer_reset ();
    gui_key_paste_pending = 0;
    gui_input_paste_pending_signal ();
}

/*
 * Ends keyboard (frees some data).
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
 * Returns hdata for key.
 */

struct t_hdata *
gui_key_hdata_key_cb (const void *pointer, void *data,
                      const char *hdata_name)
{
    struct t_hdata *hdata;
    int i;
    char str_list[128];

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    hdata = hdata_new (NULL, hdata_name, "prev_key", "next_key",
                       0, 0, NULL, NULL);
    if (hdata)
    {
        HDATA_VAR(struct t_gui_key, key, STRING, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_key, area_type, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_key, area_name, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_key, area_key, STRING, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_key, command, STRING, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_key, score, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_key, prev_key, POINTER, 0, NULL, hdata_name);
        HDATA_VAR(struct t_gui_key, next_key, POINTER, 0, NULL, hdata_name);
        for (i = 0; i < GUI_KEY_NUM_CONTEXTS; i++)
        {
            snprintf (str_list, sizeof (str_list),
                      "gui_keys%s%s",
                      (i == GUI_KEY_CONTEXT_DEFAULT) ? "" : "_",
                      (i == GUI_KEY_CONTEXT_DEFAULT) ? "" : gui_key_context_string[i]);
            hdata_new_list (hdata, str_list, &gui_keys[i], 0);
            snprintf (str_list, sizeof (str_list),
                      "last_gui_key%s%s",
                      (i == GUI_KEY_CONTEXT_DEFAULT) ? "" : "_",
                      (i == GUI_KEY_CONTEXT_DEFAULT) ? "" : gui_key_context_string[i]);
            hdata_new_list (hdata, str_list, &last_gui_key[i], 0);
            snprintf (str_list, sizeof (str_list),
                      "gui_default_keys%s%s",
                      (i == GUI_KEY_CONTEXT_DEFAULT) ? "" : "_",
                      (i == GUI_KEY_CONTEXT_DEFAULT) ? "" : gui_key_context_string[i]);
            hdata_new_list (hdata, str_list, &gui_default_keys[i], 0);
            snprintf (str_list, sizeof (str_list),
                      "last_gui_default_key%s%s",
                      (i == GUI_KEY_CONTEXT_DEFAULT) ? "" : "_",
                      (i == GUI_KEY_CONTEXT_DEFAULT) ? "" : gui_key_context_string[i]);
            hdata_new_list (hdata, str_list, &last_gui_default_key[i], 0);
        }
    }
    return hdata;
}

/*
 * Adds a key in an infolist.
 *
 * Returns:
 *   1: OK
 *   0: error
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
        {
            free (expanded_key);
            return 0;
        }
        free (expanded_key);
    }
    if (!infolist_new_var_integer (ptr_item, "area_type1", key->area_type[0]))
        return 0;
    if (!infolist_new_var_string (ptr_item, "area_name1", key->area_name[0]))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "area_type2", key->area_type[1]))
        return 0;
    if (!infolist_new_var_string (ptr_item, "area_name2", key->area_name[1]))
        return 0;
    if (!infolist_new_var_string (ptr_item, "area_key", key->area_key))
        return 0;
    if (!infolist_new_var_string (ptr_item, "command", key->command))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "score", key->score))
        return 0;

    return 1;
}

/*
 * Prints a key info in WeeChat log file (usually for crash dump).
 */

void
gui_key_print_log_key (struct t_gui_key *key, const char *prefix)
{
    int area;

    log_printf ("%s[key (addr:0x%lx)]", prefix, key);
    log_printf ("%s  key. . . . . . . . : '%s'", prefix, key->key);
    for (area = 0; area < 2; area++)
    {
        log_printf ("%s  area_type[%d] . . . : %d ('%s')",
                    prefix, area, key->area_type[area],
                    gui_key_focus_string[key->area_type[area]]);
        log_printf ("%s  area_name[%d] . . . : '%s'",
                    prefix, area, key->area_name[area]);
    }
    log_printf ("%s  area_key . . . . . : '%s'",  prefix, key->area_key);
    log_printf ("%s  command. . . . . . : '%s'",  prefix, key->command);
    log_printf ("%s  score. . . . . . . : %d",    prefix, key->score);
    log_printf ("%s  prev_key . . . . . : 0x%lx", prefix, key->prev_key);
    log_printf ("%s  next_key . . . . . : 0x%lx", prefix, key->next_key);
}

/*
 * Prints key infos in WeeChat log file (usually for crash dump).
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
            gui_key_print_log_key (ptr_key, "    ");
        }
    }
    else
    {
        for (i = 0; i < GUI_KEY_NUM_CONTEXTS; i++)
        {
            log_printf ("");
            log_printf ("[keys for context: %s]", gui_key_context_string[i]);
            log_printf ("  keys . . . . . . . . : 0x%lx", gui_keys[i]);
            log_printf ("  last_key . . . . . . : 0x%lx", last_gui_key[i]);
            log_printf ("  keys_count . . . . . : %d",    gui_keys_count[i]);

            for (ptr_key = gui_keys[i]; ptr_key; ptr_key = ptr_key->next_key)
            {
                log_printf ("");
                gui_key_print_log_key (ptr_key, "");
            }
        }
    }
}
