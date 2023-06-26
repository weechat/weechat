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
#include "../core/wee-config-file.h"
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

char *gui_key_modifier_list[] =
{ "meta-", "ctrl-", "shift-", NULL };

char *gui_key_alias_list[] =
{ "f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7", "f8", "f9", "f10", "f11",
  "f12", "f13", "f14", "f15", "f16", "f17", "f18", "f19", "f20",
  "home", "insert", "delete", "end", "backspace", "pgup", "pgdn",
  "up", "down", "right", "left", "tab", "return", "comma", "space", NULL };

int gui_key_debug = 0;              /* 1 for key debug: display raw codes,  */
                                    /* do not execute associated actions    */

int gui_key_verbose = 0;            /* 1 to see some messages               */

char gui_key_combo[1024];           /* buffer used for combos               */
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
    int context;

    gui_key_combo[0] = '\0';
    gui_key_grab = 0;
    gui_key_grab_count = 0;
    gui_key_last_activity_time = time (NULL);

    /* create default keys and save them in a separate list */
    for (context = 0; context < GUI_KEY_NUM_CONTEXTS; context++)
    {
        gui_keys[context] = NULL;
        last_gui_key[context] = NULL;
        gui_default_keys[context] = NULL;
        last_gui_default_key[context] = NULL;
        gui_keys_count[context] = 0;
        gui_default_keys_count[context] = 0;
        gui_key_default_bindings (context, 0);
        gui_default_keys[context] = gui_keys[context];
        last_gui_default_key[context] = last_gui_key[context];
        gui_default_keys_count[context] = gui_keys_count[context];
        gui_keys[context] = NULL;
        last_gui_key[context] = NULL;
        gui_keys_count[context] = 0;
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

    if (!context)
        return -1;

    for (i = 0; i < GUI_KEY_NUM_CONTEXTS; i++)
    {
        if (strcmp (gui_key_context_string[i], context) == 0)
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
gui_key_grab_end_timer_cb (const void *pointer, void *data, int remaining_calls)
{
    char *key_name, *key_name_alias, *key_utf8;
    struct t_gui_key *ptr_key_raw, *ptr_key;
    int rc;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) remaining_calls;

    /* get expanded name (for example: \x01+U => ctrl-u) */
    rc = gui_key_expand (gui_key_combo, &key_name, &key_name_alias);
    if (rc && key_name && key_name_alias)
    {
        /*
         * the key name should be valid UTF-8 at this point,
         * but some mouse codes can return ISO chars (for coordinates),
         * then we will convert them to UTF-8 string
         */
        if (!utf8_is_valid (key_name, -1, NULL))
        {
            key_utf8 = string_iconv_to_internal ("iso-8859-1", key_name);
            if (key_utf8)
            {
                free (key_name);
                key_name = key_utf8;
            }
            else
            {
                /* conversion failed, then just replace invalid chars by '?' */
                utf8_normalize (key_name, '?');
            }
        }
        if (!utf8_is_valid (key_name_alias, -1, NULL))
        {
            key_utf8 = string_iconv_to_internal ("iso-8859-1", key_name_alias);
            if (key_utf8)
            {
                free (key_name_alias);
                key_name_alias = key_utf8;
            }
            else
            {
                /* conversion failed, then just replace invalid chars by '?' */
                utf8_normalize (key_name_alias, '?');
            }
        }

        /* add expanded key to input buffer */
        if (gui_current_window->buffer->input)
        {
            ptr_key_raw = gui_key_search (gui_keys[GUI_KEY_CONTEXT_DEFAULT],
                                          key_name);
            ptr_key = gui_key_search (gui_keys[GUI_KEY_CONTEXT_DEFAULT],
                                      key_name_alias);
            gui_input_insert_string (gui_current_window->buffer,
                                     (ptr_key_raw) ? key_name : key_name_alias);
            /* add command bound to key (if found) */
            if (gui_key_grab_command && (ptr_key_raw || ptr_key))
            {
                gui_input_insert_string (gui_current_window->buffer, " ");
                gui_input_insert_string (
                    gui_current_window->buffer,
                    (ptr_key_raw) ?
                    ptr_key_raw->command : ptr_key->command);
            }
            gui_input_text_changed_modifier_and_signal (
                gui_current_window->buffer,
                1, /* save undo */
                1); /* stop completion */
        }
    }

    /* end grab mode */
    gui_key_grab = 0;
    gui_key_grab_count = 0;
    gui_key_grab_command = 0;
    gui_key_combo[0] = '\0';

    return WEECHAT_RC_OK;
}

/*
 * Gets internal code from user key name.
 *
 * Note: this function works with legacy keys (WeeChat < 4.0.0) and should not
 * be used any more.
 *
 * Examples:
 *   "ctrl-R" => "\x01" + "r" (lower case enforced for ctrl keys)
 *   "ctrl-r" => "\x01" + "r"
 *   "meta-a" => "\x01[a"
 *   "meta-A" => "\x01[A"
 *   "meta-wmeta2-1;3A" => "\x01" + "[w" + "\x01" + "[[1;3A"
 *
 * Note: result must be freed after use.
 */

char *
gui_key_legacy_internal_code (const char *key)
{
    char **result, str_key[2];

    if (!key)
        return NULL;

    if ((key[0] == '@') && strchr (key, ':'))
        return strdup (key);

    result = string_dyn_alloc (strlen (key) + 1);
    if (!result)
        return NULL;

    while (key[0])
    {
        if (strncmp (key, "meta2-", 6) == 0)
        {
            if (key[6])
                string_dyn_concat (result, "\x01[[", -1);
            key += 6;
        }
        else if (strncmp (key, "meta-", 5) == 0)
        {
            if (key[5])
                string_dyn_concat (result, "\x01[", -1);
            key += 5;
        }
        else if (strncmp (key, "ctrl-", 5) == 0)
        {
            if (key[5])
                string_dyn_concat (result, "\x01", -1);
            key += 5;
            if (key[0])
            {
                /*
                 * note: the terminal makes no difference between ctrl-x and
                 * ctrl-shift-x, so for now WeeChat automatically converts any
                 * ctrl-letter key to lower case: when the user tries to bind
                 * "ctrl-A", the key "ctrl-a" is actually added
                 * (lower case is forced for ctrl keys)
                 */
                str_key[0] = ((key[0] >= 'A') && (key[0] <= 'Z')) ?
                    key[0] + ('a' - 'A') : key[0];
                str_key[1] = '\0';
                string_dyn_concat (result, str_key, -1);
                key++;
            }
        }
        else if (strncmp (key, "space", 5) == 0)
        {
            string_dyn_concat (result, " ", -1);
            key += 5;
        }
        else
        {
            string_dyn_concat (result, key, 1);
            key++;
        }
    }

    return string_dyn_free (result, 0);
}

/*
 * Expands raw key code to its name and name using aliases (human readable
 * key name).
 *
 * Examples (return: rc, key name, key name with alias):
 *   "\001"             => 0, NULL,                NULL
 *   "\001j"            => 1, "ctrl-j",            "return"
 *   "\001m"            => 1, "ctrl-m",            "return"
 *   "\001r"            => 1, "ctrl-r",            "ctrl-r"
 *   "\001[A"           => 1, "meta-A",            "meta-A"
 *   "\001[a"           => 1, "meta-a",            "meta-a"
 *   "\001[[1;3D"       => 1, "meta-[1;3D",        "meta-left"
 *   "\001[w\001[[1;3A" => 1, "meta-w,meta-[1;3A", "meta-w,meta-up"
 *
 * Returns:
 *   1: OK
 *   0: error: incomplete/invalid raw key
 *
 * Note: if set to non NULL value, *key_name_raw and *key_name_alias must be
 * freed.
 */

int
gui_key_expand (const char *key, char **key_name, char **key_name_alias)
{
    const char *ptr_key_meta2;
    char **str_dyn_key, **str_dyn_key_alias, str_raw[64], str_alias[128];
    int i, modifier, ctrl, meta, meta_initial, meta2, shift, number, length;

    if (key_name)
        *key_name = NULL;
    if (key_name_alias)
        *key_name_alias = NULL;

    if (!key)
        return 0;

    str_dyn_key = NULL;
    str_dyn_key_alias = NULL;

    str_dyn_key = string_dyn_alloc ((strlen (key) * 2) + 1);
    if (!str_dyn_key)
        goto error;

    str_dyn_key_alias = string_dyn_alloc ((strlen (key) * 2) + 1);
    if (!str_dyn_key_alias)
        goto error;

    while (key[0])
    {
        ctrl = 0;
        meta = 0;
        meta2 = 0;
        shift = 0;

        if (*str_dyn_key[0])
            string_dyn_concat (str_dyn_key, ",", -1);

        if (*str_dyn_key_alias[0])
            string_dyn_concat (str_dyn_key_alias, ",", -1);

        str_raw[0] = '\0';

        while (string_strncmp (key, "\x01[\x01", 3) == 0)
        {
            /* key: meta + meta-something: increase meta and skip it */
            meta++;
            key += 2;
        }

        if (string_strncmp (key, "\x01[O", 3) == 0)
        {
            snprintf (str_raw, sizeof (str_raw), "meta-O");
            meta2 = 1;
            key += 3;
        }
        else if (string_strncmp (key, "\x01[[", 3) == 0)
        {
            snprintf (str_raw, sizeof (str_raw), "meta-[");
            meta2 = 1;
            key += 3;
        }
        else if (string_strncmp (key, "\x01[", 2) == 0)
        {
            meta++;
            key += 2;
        }
        else if (key[0] == '\x01')
        {
            ctrl = 1;
            key++;
        }

        if (!key[0])
            goto error;

        if (meta2)
        {
            ptr_key_meta2 = key;
            meta_initial = meta;
            str_alias[0] = '\0';

            if (isdigit ((unsigned char)key[0])
                && (!key[1]
                    || (isdigit ((unsigned char)key[1])
                        && (!key[2]
                            || (isdigit ((unsigned char)key[2]) && !key[3])))))
            {
                /* incomplete sequence: 1 to 3 digits with nothing after */
                goto error;
            }

            /*
             * Supported formats: "<esc>[" + one of:
             *   3~     -> delete
             *   3;5~   -> ctrl-delete
             *   15~    -> f5
             *   15;5~  -> ctrl-f5
             *   A      -> up
             *   1;5A   -> ctrl-up
             */
            if ((key[0] == '1') && (key[1] == ';')
                && ((key[2] >= '1') && (key[2] <= '9')))
            {
                /* modifier, format: "1;1" to "1;9" */
                modifier = key[2] - '0' - 1;
                if ((modifier & 0x01) || (modifier & 0x08))
                    shift = 1;
                if ((modifier & 0x02) || (modifier & 0x08))
                    meta++;
                if ((modifier & 0x04) || (modifier & 0x08))
                    ctrl = 1;
                key += 3;
            }

            if (!key[0])
                goto error;

            if (isdigit ((unsigned char)key[0])
                && isdigit ((unsigned char)key[1])
                && ((key[2] == ';') || (key[2] == '~') || (key[2] == '^')
                    || (key[2] == '$') || (key[2] == '@')))
            {
                /*
                 * format: "00;N~" to "99;N~" (urxvt: ~ ^ $ @)
                 * or "00~" to "99~" (urxvt: ~ ^ $ @)
                 */
                number = ((key[0] - '0') * 10) + (key[1] - '0');
                if ((number >= 10) && (number <= 15))
                    snprintf (str_alias, sizeof (str_alias), "f%d", number - 10);
                else if ((number >= 17) && (number <= 21))
                    snprintf (str_alias, sizeof (str_alias), "f%d", number - 11);
                else if ((number >= 23) && (number <= 26))
                    snprintf (str_alias, sizeof (str_alias), "f%d", number - 12);
                else if ((number >= 28) && (number <= 29))
                    snprintf (str_alias, sizeof (str_alias), "f%d", number - 13);
                else if ((number >= 31) && (number <= 34))
                    snprintf (str_alias, sizeof (str_alias), "f%d", number - 14);
                key += 2;
                if (key[0] == ';')
                {
                    key++;
                    if (!key[0])
                        goto error;
                    if ((key[0] >= '1') && (key[0] <= '9'))
                    {
                        modifier = key[0] - '0' - 1;
                        if ((modifier & 0x01) || (modifier & 0x08))
                            shift = 1;
                        if ((modifier & 0x02) || (modifier & 0x08))
                            meta++;
                        if ((modifier & 0x04) || (modifier & 0x08))
                            ctrl = 1;
                        key++;
                        if (!key[0])
                            goto error;
                    }
                }
                if (key[0] == '^')
                    ctrl = 1;
                else if (key[0] == '$')
                    shift = 1;
                else if (key[0] == '@')
                {
                    ctrl = 1;
                    shift = 1;
                }
                key++;
            }
            else if (isdigit ((unsigned char)key[0])
                     && ((key[1] == ';') || (key[1] == '~') || (key[1] == '^')
                         || (key[1] == '$') || (key[1] == '@')))
            {
                /*
                 * format: "0;N~" to "9;N~" (urxvt: ~ ^ $ @)
                 * or "0~" to "9~" (urxvt: ~ ^ $ @)
                 * */
                number = key[0] - '0';
                if ((number == 1) || (number == 7))
                    snprintf (str_alias, sizeof (str_alias), "home");
                else if (number == 2)
                    snprintf (str_alias, sizeof (str_alias), "insert");
                else if (number == 3)
                    snprintf (str_alias, sizeof (str_alias), "delete");
                else if ((number == 4) || (number == 8))
                    snprintf (str_alias, sizeof (str_alias), "end");
                else if (number == 5)
                    snprintf (str_alias, sizeof (str_alias), "pgup");
                else if (number == 6)
                    snprintf (str_alias, sizeof (str_alias), "pgdn");
                key++;
                if (key[0] == ';')
                {
                    key++;
                    if (!key[0])
                        goto error;
                    if ((key[0] >= '1') && (key[0] <= '9'))
                    {
                        modifier = key[0] - '0' - 1;
                        if ((modifier & 0x01) || (modifier & 0x08))
                            shift = 1;
                        if ((modifier & 0x02) || (modifier & 0x08))
                            meta++;
                        if ((modifier & 0x04) || (modifier & 0x08))
                            ctrl = 1;
                        key++;
                        if (!key[0])
                            goto error;

                    }
                }
                if (key[0] == '^')
                    ctrl = 1;
                else if (key[0] == '$')
                    shift = 1;
                else if (key[0] == '@')
                {
                    ctrl = 1;
                    shift = 1;
                }
                key++;
            }
            else if (((key[0] >= 'A') && (key[0] <= 'Z'))
                     || ((key[0] >= 'a') && (key[0] <= 'z')))
            {
                /* format: "A" to "Z" */
                if (key[0] == 'A')
                    snprintf (str_alias, sizeof (str_alias), "up");
                else if (key[0] == 'a')
                {
                    ctrl = 1;
                    snprintf (str_alias, sizeof (str_alias), "up");
                }
                else if (key[0] == 'B')
                    snprintf (str_alias, sizeof (str_alias), "down");
                else if (key[0] == 'b')
                {
                    ctrl = 1;
                    snprintf (str_alias, sizeof (str_alias), "down");
                }
                else if (key[0] == 'C')
                    snprintf (str_alias, sizeof (str_alias), "right");
                else if (key[0] == 'c')
                {
                    ctrl = 1;
                    snprintf (str_alias, sizeof (str_alias), "right");
                }
                else if (key[0] == 'D')
                    snprintf (str_alias, sizeof (str_alias), "left");
                else if (key[0] == 'd')
                {
                    ctrl = 1;
                    snprintf (str_alias, sizeof (str_alias), "left");
                }
                else if (key[0] == 'F')
                    snprintf (str_alias, sizeof (str_alias), "end");
                else if (key[0] == 'H')
                    snprintf (str_alias, sizeof (str_alias), "home");
                else if (key[0] == 'P')
                    snprintf (str_alias, sizeof (str_alias), "f1");
                else if (key[0] == 'Q')
                    snprintf (str_alias, sizeof (str_alias), "f2");
                else if (key[0] == 'R')
                    snprintf (str_alias, sizeof (str_alias), "f3");
                else if (key[0] == 'S')
                    snprintf (str_alias, sizeof (str_alias), "f4");
                else if (key[0] == 'Z')
                {
                    shift = 1;
                    snprintf (str_alias, sizeof (str_alias), "tab");
                }
                key++;
            }
            else if (key[0] == '[')
            {
                /* some sequences specific to Linux console */
                key++;
                if (!key[0])
                    goto error;
                if (key[0] == 'A')
                    snprintf (str_alias, sizeof (str_alias), "f1");
                else if (key[0] == 'B')
                    snprintf (str_alias, sizeof (str_alias), "f2");
                else if (key[0] == 'C')
                    snprintf (str_alias, sizeof (str_alias), "f3");
                else if (key[0] == 'D')
                    snprintf (str_alias, sizeof (str_alias), "f4");
                else if (key[0] == 'E')
                    snprintf (str_alias, sizeof (str_alias), "f5");
                key++;
            }
            else
            {
                /* unknown sequence */
                key = utf8_next_char (key);
            }

            length = key - ptr_key_meta2;
            memcpy (str_raw + strlen (str_raw), ptr_key_meta2, length);
            str_raw[6 + length] = '\0';

            if (!str_alias[0])
            {
                /* unknown sequence: keep raw key code as-is */
                snprintf (str_alias, sizeof (str_alias), "%s", str_raw);
                ctrl = 0;
                meta = 0;
                shift = 0;
            }

            /* add modifier(s) + key (raw) */
            if (str_raw[0])
            {
                for (i = 0; i < meta_initial; i++)
                {
                    string_dyn_concat (str_dyn_key, "meta-", -1);
                }
                string_dyn_concat (str_dyn_key, str_raw, -1);
            }

            /* add modifier(s) + key (alias) */
            if (str_alias[0])
            {
                for (i = 0; i < meta; i++)
                {
                    string_dyn_concat (str_dyn_key_alias, "meta-", -1);
                }
                if (ctrl)
                    string_dyn_concat (str_dyn_key_alias, "ctrl-", -1);
                if (shift)
                    string_dyn_concat (str_dyn_key_alias, "shift-", -1);
                string_dyn_concat (str_dyn_key_alias, str_alias, -1);
            }
        }
        else
        {
            /* automatically convert ctrl-[A-Z] to ctrl-[a-z] (lower case) */
            if (ctrl && (key[0] >= 'A') && (key[0] <= 'Z'))
            {
                snprintf (str_raw, sizeof (str_raw),
                          "%c", key[0] + ('a' - 'A'));
                key++;
            }
            else if (key[0] == ' ')
            {
                snprintf (str_raw, sizeof (str_raw), "space");
                key++;
            }
            else if (key[0] == ',')
            {
                snprintf (str_raw, sizeof (str_raw), "comma");
                key++;
            }
            else
            {
                length = utf8_char_size (key);
                memcpy (str_raw, key, length);
                str_raw[length] = '\0';
                key += length;
            }

            if (meta)
            {
                for (i = 0; i < meta; i++)
                {
                    string_dyn_concat (str_dyn_key, "meta-", -1);
                    string_dyn_concat (str_dyn_key_alias, "meta-", -1);
                }
            }

            if (ctrl
                && ((strcmp (str_raw, "h") == 0)
                    || strcmp (str_raw, "?") == 0))
            {
                string_dyn_concat (str_dyn_key, "ctrl-", -1);
                string_dyn_concat (str_dyn_key, str_raw, -1);
                string_dyn_concat (str_dyn_key_alias, "backspace", -1);
            }
            else if (ctrl && (strcmp (str_raw, "i") == 0))
            {
                string_dyn_concat (str_dyn_key, "ctrl-", -1);
                string_dyn_concat (str_dyn_key, str_raw, -1);
                string_dyn_concat (str_dyn_key_alias, "tab", -1);
            }
            else if (ctrl
                     && ((strcmp (str_raw, "j") == 0)
                         || (strcmp (str_raw, "m") == 0)))
            {
                string_dyn_concat (str_dyn_key, "ctrl-", -1);
                string_dyn_concat (str_dyn_key, str_raw, -1);
                string_dyn_concat (str_dyn_key_alias, "return", -1);
            }
            else
            {
                if (ctrl)
                {
                    string_dyn_concat (str_dyn_key, "ctrl-", -1);
                    string_dyn_concat (str_dyn_key_alias, "ctrl-", -1);
                }
                string_dyn_concat (str_dyn_key, str_raw, -1);
                string_dyn_concat (str_dyn_key_alias, str_raw, -1);
            }
        }
    }

    if (key_name)
        *key_name = string_dyn_free (str_dyn_key, 0);
    else
        string_dyn_free (str_dyn_key, 1);

    if (key_name_alias)
        *key_name_alias = string_dyn_free (str_dyn_key_alias, 0);
    else
        string_dyn_free (str_dyn_key_alias, 1);

    return 1;

error:
    if (str_dyn_key)
        string_dyn_free (str_dyn_key, 1);
    if (str_dyn_key_alias)
        string_dyn_free (str_dyn_key_alias, 1);
    return 0;
}

/*
 * Converts a legacy key to the new key name (using comma separator and alias).
 *
 * Examples:
 *   "ctrl-"            => NULL
 *   "meta-"            => NULL
 *   "ctrl-A"           => "ctrl-a"
 *   "ctrl-a"           => "ctrl-a"
 *   "ctrl-j"           => "return"
 *   "ctrl-m"           => "ctrl-m"
 *   "ctrl-Cb"          => "ctrl-c,b"
 *   "meta-space"       => "meta-space"
 *   "meta-comma"       => "meta-c,o,m,m,a"
 *   "meta-,"           => "meta-comma"
 *   "meta-,x"          => "meta-comma,x"
 *   "meta2-1;3D"       => "meta-left"
 *   "meta-wmeta2-1;3A" => "meta-w,meta-up"
 *   "meta-w,meta-up"   => "meta-w,comma,meta-u,p"
 *
 * Note: result must be freed after use.
 */

char *
gui_key_legacy_to_alias (const char *key)
{
    char *key_raw, *key_name_alias;
    int rc;

    if (!key)
        return NULL;

    if ((key[0] == '@') && strchr (key, ':'))
        return strdup (key);

    key_raw = gui_key_legacy_internal_code (key);
    if (!key_raw)
        return NULL;

    rc = gui_key_expand (key_raw, NULL, &key_name_alias);

    free (key_raw);

    if (!rc)
        return NULL;

    return key_name_alias;
}

/*
 * Attempts to fix a key in mouse context (starting with "@area:"):
 *   - transform "ctrl-alt-" to "alt-ctrl-"
 *
 * Example:
 *   "@chat:ctrl-alt-button1" => "@chat:meta-ctrl-wheelup"
 */

char *
gui_key_fix_mouse (const char *key)
{
    const char *pos;
    char **result;

    if (!key)
        return NULL;

    if (key[0] != '@')
        return strdup (key);

    pos = strchr (key, ':');
    if (!pos)
        return strdup (key);
    pos++;

    result = string_dyn_alloc (strlen (key) + 1);
    if (!result)
        return NULL;

    string_dyn_concat (result, key, pos - key);

    if (strncmp (pos, "ctrl-alt-", 9) == 0)
    {
        string_dyn_concat (result, "alt-ctrl-", -1);
        pos += 9;
    }
    string_dyn_concat (result, pos, -1);

    return string_dyn_free (result, 0);
}

/*
 * Attempts to fix key:
 *   - transform upper case ctrl keys to lower case ("ctrl-A" -> "ctrl-a")
 *   - replace " " by "space"
 *   - replace "meta2-" by "meta-["
 *   - replace "ctrl-alt-" by "alt-ctrl-" (for mouse).
 *
 * Examples of valid keys, unchanged:
 *   "ctrl-a"
 *   "meta-A"
 *   "return"
 *
 * Examples of keys fixed by this function:
 *   "ctrl-A"                 => "ctrl-a"
 *   "ctrl-C,b"               => "ctrl-c,b"
 *   " "                      => "space"
 *   "meta- "                 => "meta-space"
 *   "meta2-A"                => "meta-[A"
 *   "@chat:ctrl-alt-button1" => "@chat:alt-ctrl-wheelup"
 *
 * Note: result must be freed after use.
 */

char *
gui_key_fix (const char *key)
{
    char **result, str_key[2];
    const char *ptr_key;
    int length;

    if (!key)
        return NULL;

    if ((key[0] == '@') && strchr (key, ':'))
        return gui_key_fix_mouse (key);

    result = string_dyn_alloc (strlen (key) + 1);
    if (!result)
        return NULL;

    ptr_key = key;
    while (ptr_key && ptr_key[0])
    {
        if (ptr_key[0] == ' ')
        {
            string_dyn_concat (result, "space", -1);
            ptr_key++;
        }
        else if (strncmp (ptr_key, "ctrl-", 5) == 0)
        {
            string_dyn_concat (result, ptr_key, 5);
            ptr_key += 5;
            if (ptr_key[0])
            {
                str_key[0] = ((ptr_key[0] >= 'A') && (ptr_key[0] <= 'Z')) ?
                    ptr_key[0] + ('a' - 'A') : ptr_key[0];
                str_key[1] = '\0';
                string_dyn_concat (result, str_key, -1);
                ptr_key++;
            }
        }
        else if (strncmp (ptr_key, "meta2-", 6) == 0)
        {
            string_dyn_concat (result, "meta-[", -1);
            ptr_key += 6;
        }
        else
        {
            length = utf8_char_size (ptr_key);
            string_dyn_concat (result, ptr_key, length);
            ptr_key += length;
        }
    }

    return string_dyn_free (result, 0);
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
                && (string_strcmp (key->key, ptr_key->key) < 0)))
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
    int i;

    if (!key || !key[0])
        return 0;

    /* "@" is allowed at beginning for cursor/mouse contexts */
    if ((key[0] == '@')
        && ((context == GUI_KEY_CONTEXT_CURSOR)
            || (context == GUI_KEY_CONTEXT_MOUSE)))
    {
        return 1;
    }

    if (strncmp (key, "comma", 5) == 0)
        return 0;

    if (strncmp (key, "space", 5) == 0)
        return 0;

    for (i = 0; gui_key_modifier_list[i]; i++)
    {
        if (strncmp (key, gui_key_modifier_list[i],
                     strlen (gui_key_modifier_list[i])) == 0)
        {
            /* key is safe */
            return 1;
        }
    }

    for (i = 0; gui_key_alias_list[i]; i++)
    {
        if (strncmp (key, gui_key_alias_list[i],
                     strlen (gui_key_alias_list[i])) == 0)
        {
            /* key is safe */
            return 1;
        }
    }

    /* key is not safe */
    return 0;
}

/*
 * Checks if the key chunk seems valid.
 *
 * Example of valid key chunk:
 *   "meta-a"
 *   "meta-c"
 *   "meta-up"
 *   "ctrl-left"
 *   "ctrl-u"
 *
 * Example of raw codes or invalid keys:
 *   "meta-[A"  (raw code)
 *   "ctrl-cb"  (invalid: missing comma)
 *
 * Returns:
 *   1: key chunk seems valid
 *   0: key chunk seems either invalid or a raw code
 */

int
gui_key_chunk_seems_valid (const char *chunk)
{
    int i, found, length;

    if (!chunk || !chunk[0])
        return 0;

    /* skip modifiers */
    found = 1;
    while (found)
    {
        found = 0;
        for (i = 0; gui_key_modifier_list[i]; i++)
        {
            length = strlen (gui_key_modifier_list[i]);
            if (strncmp (chunk, gui_key_modifier_list[i], length) == 0)
            {
                chunk += length;
                found = 1;
                break;
            }
        }
    }

    /* check if it's an alias */
    found = 0;
    for (i = 0; gui_key_alias_list[i]; i++)
    {
        length = strlen (gui_key_alias_list[i]);
        if (strncmp (chunk, gui_key_alias_list[i], length) == 0)
        {
            chunk += length;
            found = 1;
            break;
        }
    }
    if (!found)
        chunk = utf8_next_char (chunk);

    if (chunk[0])
        return 0;

    return 1;
}

/*
 * Checks if the key seems valid: not a raw code, and no comma is missing.
 *
 * Example of valid keys:
 *   "meta-a"
 *   "meta-c,b"
 *   "meta-w,meta-up"
 *   "ctrl-left"
 *   "ctrl-u"
 *
 * Example of raw codes or invalid keys:
 *   "meta-[A"  (raw code)
 *   "ctrl-cb"  (invalid: missing comma)
 *
 * Returns:
 *   1: key seems valid
 *   0: key seems either invalid or a raw code
 */

int
gui_key_seems_valid (int context, const char *key)
{
    char **chunks;
    int i, rc, chunks_count;

    if (!key || !key[0])
        return 0;

    /* "@" is allowed at beginning for cursor/mouse contexts */
    if ((key[0] == '@')
        && ((context == GUI_KEY_CONTEXT_CURSOR)
            || (context == GUI_KEY_CONTEXT_MOUSE)))
    {
        return 1;
    }

    chunks = string_split (key, ",", NULL, 0, 0, &chunks_count);
    if (!chunks)
        return 0;

    rc = 1;
    for (i = 0; i < chunks_count; i++)
    {
        if (!gui_key_chunk_seems_valid (chunks[i]))
        {
            rc = 0;
            break;
        }
    }

    string_free_split (chunks);
    return rc;
}

/*
 * Callback for changes on a key option.
 */

void
gui_key_option_change_cb (const void *pointer, void *data,
                          struct t_config_option *option)
{
    struct t_gui_key *ptr_key;
    int context;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    context = config_weechat_get_key_context (option->section);
    if (context < 0)
        return;

    ptr_key = gui_key_search (gui_keys[context], option->name);
    if (!ptr_key)
        return;

    if (ptr_key->command)
        free (ptr_key->command);
    ptr_key->command = strdup (CONFIG_STRING(option));
}

/*
 * Creates a new key option.
 *
 * Returns pointer to existing or new option.
 */

struct t_config_option *
gui_key_new_option (int context, const char *name, const char *value)
{
    struct t_config_option *ptr_option;
    struct t_gui_key *ptr_key;
    char str_description[1024];

    if (!name || !value)
        return NULL;

    ptr_option = config_file_search_option (weechat_config_file,
                                            weechat_config_section_key[context],
                                            name);
    if (ptr_option)
    {
        config_file_option_set (ptr_option, value, 1);
    }
    else
    {
        snprintf (str_description, sizeof (str_description),
                  _("key \"%s\" in context \"%s\""),
                  name,
                  gui_key_context_string[context]);
        ptr_key = gui_key_search (gui_default_keys[context], name);
        ptr_option = config_file_new_option (
            weechat_config_file, weechat_config_section_key[context],
            name, "string",
            str_description,
            NULL, 0, 0,
            (ptr_key) ? ptr_key->command : "",
            value,
            0,
            NULL, NULL, NULL,
            &gui_key_option_change_cb, NULL, NULL,
            NULL, NULL, NULL);
    }

    return ptr_option;
}

/*
 * Adds a new key in keys list.
 *
 * If buffer is not null, then key is specific to buffer, otherwise it's general
 * key (for most keys).
 *
 * If create_option == 1, a config option is created as well.
 *
 * Returns pointer to new key, NULL if error.
 */

struct t_gui_key *
gui_key_new (struct t_gui_buffer *buffer, int context, const char *key,
             const char *command, int create_option)
{
    char *key_fixed;
    struct t_config_option *ptr_option;
    struct t_gui_key *new_key;

    key_fixed = NULL;
    ptr_option = NULL;

    if (!key || !command)
        goto error;

    if ((context == GUI_KEY_CONTEXT_MOUSE) && (key[0] != '@'))
    {
        if (gui_key_verbose)
        {
            gui_chat_printf (NULL,
                             _("%sInvalid key for mouse context \"%s\": it "
                               "must start with \"@area\" (see /help key)"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             key);
        }
        return NULL;
    }

    key_fixed = gui_key_fix (key);
    if (!key_fixed)
        goto error;

    ptr_option = NULL;

    if (!buffer && create_option)
    {
        ptr_option = gui_key_new_option (context, key_fixed, command);
        if (!ptr_option)
            goto error;
    }

    new_key = malloc (sizeof (*new_key));
    if (!new_key)
        goto error;

    new_key->key = key_fixed;
    new_key->chunks = string_split (key_fixed, ",", NULL, 0, 0,
                                    &new_key->chunks_count);
    new_key->command = strdup (command);
    gui_key_set_areas (new_key);
    gui_key_set_score (new_key);

    if (buffer)
    {
        gui_key_insert_sorted (&buffer->keys,
                               &buffer->last_key,
                               &buffer->keys_count,
                               new_key);
    }
    else
    {
        gui_key_insert_sorted (&gui_keys[context],
                               &last_gui_key[context],
                               &gui_keys_count[context],
                               new_key);
    }

    if (gui_key_verbose)
    {
        gui_chat_printf (NULL,
                         _("New key binding (context \"%s\"): "
                           "%s%s => %s%s"),
                         gui_key_context_string[context],
                         new_key->key,
                         GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                         GUI_COLOR(GUI_COLOR_CHAT),
                         new_key->command);
    }

    (void) hook_signal_send ("key_bind",
                             WEECHAT_HOOK_SIGNAL_STRING, new_key->key);

    return new_key;

error:
    if (gui_key_verbose)
    {
        gui_chat_printf (NULL,
                         _("%sUnable to bind key \"%s\" in context \"%s\" "
                           "(see /help key)"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         key,
                         gui_key_context_string[context]);
    }
    if (key_fixed)
        free (key_fixed);
    if (ptr_option)
        config_file_option_free (ptr_option, 0);
    return NULL;
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

    if (!key || !key[0])
        return NULL;

    for (ptr_key = keys; ptr_key; ptr_key = ptr_key->next_key)
    {
        if (strcmp (ptr_key->key, key) == 0)
            return ptr_key;
    }

    /* key not found */
    return NULL;
}

/*
 * Compare chunks with key chunks.
 *
 * Returns:
 *   2: exact match
 *   1: partial match (key_chunks starts with chunks but is longer)
 *   0: no match
 */

int
gui_key_compare_chunks (const char **chunks, int chunks_count,
                        const char **key_chunks, int key_chunks_count)
{
    int i;

    for (i = 0; i < chunks_count; i++)
    {
        if (i >= key_chunks_count)
            return 0;
        if (strcmp (chunks[i], key_chunks[i]) != 0)
            return 0;
    }

    return (chunks_count == key_chunks_count) ? 2 : 1;
}

/*
 * Searches key chunks for context default, search or cursor (not for mouse).
 * It can be part of key chunks or exact match.
 *
 * Parameter "chunks1" is the raw key split into chunks, and "chunks2" is the
 * key with alias split into chunks (at least one of the chunks must be non
 * NULL).
 *
 * Returns pointer to key found, NULL if not found.
 * In case of exact match, *exact_match is set to 1, otherwise 0.
 */

struct t_gui_key *
gui_key_search_part (struct t_gui_buffer *buffer, int context,
                     const char **chunks1, int chunks1_count,
                     const char **chunks2, int chunks2_count,
                     int *exact_match)
{
    struct t_gui_key *ptr_key, *key1_found, *key2_found;
    int rc, rc1, rc2;

    if ((!chunks1 && !chunks2) || !exact_match)
        return NULL;

    key1_found = NULL;
    key2_found = NULL;
    rc1 = 0;
    rc2 = 0;

    for (ptr_key = (buffer) ? buffer->keys : gui_keys[context]; ptr_key;
         ptr_key = ptr_key->next_key)
    {
        if (ptr_key->key
            && ((context != GUI_KEY_CONTEXT_CURSOR)
                || (ptr_key->key[0] != '@')))
        {
            if (chunks1)
            {
                rc = gui_key_compare_chunks (chunks1,
                                             chunks1_count,
                                             (const char **)ptr_key->chunks,
                                             ptr_key->chunks_count);
                if (rc > rc1)
                {
                    rc1 = rc;
                    key1_found = ptr_key;
                    /* exit immediately if raw key is an exact match */
                    if (rc == 2)
                        break;
                }
            }
            if (chunks2)
            {
                rc = gui_key_compare_chunks (chunks2,
                                             chunks2_count,
                                             (const char **)ptr_key->chunks,
                                             ptr_key->chunks_count);
                if (rc > rc2)
                {
                    rc2 = rc;
                    key2_found = ptr_key;
                }
            }
        }
    }

    if (key1_found)
    {
        *exact_match = (rc1 == 2) ? 1 : 0;
        return key1_found;
    }

    *exact_match = (rc2 == 2) ? 1 : 0;
    return key2_found;
}

/*
 * Binds a key to a command.
 *
 * If buffer is not null, then key is specific to buffer otherwise it's general
 * key (for most keys).
 *
 * If check_key == 1, the key is checked before being added.
 *
 * If key already exists, it is removed then added again with new value.
 *
 * Returns pointer to new key, NULL if error.
 */

struct t_gui_key *
gui_key_bind (struct t_gui_buffer *buffer, int context, const char *key,
              const char *command, int check_key)
{
    if (!key || !command)
        return NULL;

    if (check_key)
    {
        if (CONFIG_BOOLEAN(config_look_key_bind_safe)
            && (context != GUI_KEY_CONTEXT_MOUSE)
            && !gui_key_is_safe (context, key))
        {
            if (gui_key_verbose)
            {
                gui_chat_printf (
                    NULL,
                    _("%sIt is not safe to bind key \"%s\" because it does "
                      "not start with a ctrl or meta code (tip: use alt-k to "
                      "find key codes); if you want to bind this key anyway, "
                      "turn off option weechat.look.key_bind_safe"),
                    gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                    key);
            }
            return NULL;
        }
        if (!gui_key_seems_valid (context, key))
        {
            gui_chat_printf (
                NULL,
                _("%sWarning: key \"%s\" seems either a raw code or invalid, "
                  "it may not work (see /help key)"),
                gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                key);
        }
    }

    gui_key_unbind (buffer, context, key);

    return gui_key_new (buffer, context, key, command, 1);
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
    struct t_config_option *ptr_option;

    /* make C compiler happy */
    (void) hashtable;

    user_data = (int *)data;

    if (user_data && key && value)
    {
        /* ignore special key "__quiet" */
        if (strcmp (key, "__quiet") == 0)
            return;

        ptr_key = gui_key_search (gui_keys[user_data[0]], key);
        if (!ptr_key)
        {
            if (gui_key_new (NULL, user_data[0], key, value, 1))
                user_data[1]++;
        }
        /*
         * adjust default value (command) of key option in config, so that
         * fset buffer shows the key as modified only if the user actually
         * changed the command bound to the key
         */
        ptr_option = config_file_search_option (
            weechat_config_file,
            weechat_config_section_key[user_data[0]],
            key);
        if (ptr_option)
            config_file_option_set_default (ptr_option, value, 1);
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
    char *key_fixed;
    struct t_gui_key *ptr_key;

    key_fixed = gui_key_fix (key);
    if (!key_fixed)
        return 0;

    ptr_key = gui_key_search ((buffer) ? buffer->keys : gui_keys[context],
                              key_fixed);
    if (!ptr_key)
    {
        free (key_fixed);
        return 0;
    }

    if (buffer)
    {
        gui_key_free (-1, &buffer->keys, &buffer->last_key,
                      &buffer->keys_count, ptr_key, 0);
    }
    else
    {
        if (gui_key_verbose)
        {
            gui_chat_printf (NULL,
                             _("Key \"%s\" unbound (context: \"%s\")"),
                             key_fixed,
                             gui_key_context_string[context]);
        }
        gui_key_free (context, &gui_keys[context], &last_gui_key[context],
                      &gui_keys_count[context], ptr_key, 1);
    }

    (void) hook_signal_send ("key_unbind",
                             WEECHAT_HOOK_SIGNAL_STRING, key_fixed);

    free (key_fixed);

    return 1;
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

        /* ignore key if key for area is not matching (context: cursor) */
        if ((context == GUI_KEY_CONTEXT_CURSOR)
            && (string_strncmp (key, ptr_key->area_key,
                                utf8_strlen (ptr_key->area_key)) != 0))
        {
            continue;
        }

        /* ignore key if key for area is not matching (context: mouse) */
        if ((context == GUI_KEY_CONTEXT_MOUSE)
            && !string_match (key, ptr_key->area_key, 1))
        {
            continue;
        }

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
                            (void) input_data (ptr_buffer, command, NULL, 0);
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
 * Prints a key in debug mode:
 *   - raw combo (eg: "^[[A")
 *   - for keyboard:
 *     - key name (eg: "meta-[A")
 *     - key name with alias (eg: "up")
 *     - command bound to key or message "no key binding"
 *   - for mouse:
 *     - "mouse"
 */

void
gui_key_debug_print_key (const char *combo, const char *key_name,
                         const char *key_name_alias, const char *command,
                         int mouse)
{
    char *combo2, *ptr_combo2, str_command[4096];

    if (command)
    {
        snprintf (str_command, sizeof (str_command),
                  "-> %s\"%s%s%s\"",
                  GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                  GUI_COLOR(GUI_COLOR_CHAT),
                  command,
                  GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS));
    }
    else
    {
        snprintf (str_command, sizeof (str_command),
                  " %s[%s%s%s]",
                  GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                  GUI_COLOR(GUI_COLOR_CHAT),
                  _("no key binding"),
                  GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS));
    }

    combo2 = strdup (combo);
    for (ptr_combo2 = combo2; ptr_combo2[0]; ptr_combo2++)
    {
        if ((ptr_combo2[0] == '\x01') || (ptr_combo2[0] == '\x1B'))
            ptr_combo2[0] = '^';
    }

    if (mouse)
    {
        gui_chat_printf (
            NULL,
            "%s %s\"%s%s%s\"%s  %s[%s%s%s]",
            _("debug:"),
            GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
            GUI_COLOR(GUI_COLOR_CHAT),
            combo2,
            GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
            GUI_COLOR(GUI_COLOR_CHAT),
            GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
            GUI_COLOR(GUI_COLOR_CHAT),
            _("mouse"),
            GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS));
    }
    else
    {
        gui_chat_printf (
            NULL,
            "%s %s\"%s%s%s\"%s -> %s -> %s %s",
            _("debug:"),
            GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
            GUI_COLOR(GUI_COLOR_CHAT),
            combo2,
            GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
            GUI_COLOR(GUI_COLOR_CHAT),
            key_name,
            key_name_alias,
            str_command);
    }

    free (combo2);
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
    int i, insert_into_input, context, length, length_key, signal_sent;
    int rc, rc_expand, exact_match, chunks1_count, chunks2_count;
    struct t_gui_key *ptr_key;
    char *pos, signal_name[128], **commands;
    char *key_name, *key_name_alias, **chunks1, **chunks2;

    signal_sent = 0;

    key_name = NULL;
    key_name_alias = NULL;

    /* add key to buffer */
    insert_into_input = (gui_key_combo[0] == '\0');
    length = strlen (gui_key_combo);
    length_key = strlen (key_str);
    if (length + length_key + 1 <= (int)sizeof (gui_key_combo))
        strcat (gui_key_combo, key_str);

    if (gui_key_debug
        && !gui_mouse_event_pending
        && (strcmp (gui_key_combo, "q") == 0))
    {
        gui_key_debug = 0;
        gui_key_combo[0] = '\0';
        gui_bar_item_update ("input_text");
        goto end_no_input;
    }

    /* if we are in "show mode", increase counter and return */
    if (gui_key_grab)
    {
        if (gui_key_grab_count == 0)
        {
            hook_timer (NULL, gui_key_grab_delay, 0, 1,
                        &gui_key_grab_end_timer_cb, NULL, NULL);
        }
        gui_key_grab_count++;
        goto end_no_input;
    }

    /* mouse event pending */
    if (gui_mouse_event_pending)
    {
        pos = strstr (gui_key_combo, "\x1B[M");
        if (pos)
        {
            pos[0] = '\0';
            if (!gui_window_bare_display)
                gui_mouse_event_end ();
            gui_mouse_event_init ();
        }
        goto end_no_input;
    }

    if (strstr (gui_key_combo, "\x01[[M"))
    {
        if (gui_key_debug)
            gui_key_debug_print_key (gui_key_combo, NULL, NULL, NULL, 1);
        gui_key_combo[0] = '\0';
        gui_mouse_event_init ();
        goto end_no_input;
    }

    rc_expand = gui_key_expand (gui_key_combo, &key_name, &key_name_alias);

    ptr_key = NULL;
    exact_match = 0;

    chunks1 = string_split (key_name, ",", NULL, 0, 0, &chunks1_count);
    if (string_strcmp (key_name, key_name_alias) != 0)
    {
        chunks2 = string_split (key_name_alias, ",", NULL, 0, 0, &chunks2_count);
    }
    else
    {
        chunks2 = NULL;
        chunks2_count = 0;
    }

    context = gui_key_get_current_context ();
    switch (context)
    {
        case GUI_KEY_CONTEXT_DEFAULT:
            /* look for key combo in key table for current buffer */
            ptr_key = gui_key_search_part (
                gui_current_window->buffer,
                GUI_KEY_CONTEXT_DEFAULT,
                (const char **)chunks1, chunks1_count,
                (const char **)chunks2, chunks2_count,
                &exact_match);
            /* if key is not found for buffer, then look in general table */
            if (!ptr_key)
            {
                ptr_key = gui_key_search_part (
                    NULL,
                    GUI_KEY_CONTEXT_DEFAULT,
                    (const char **)chunks1, chunks1_count,
                    (const char **)chunks2, chunks2_count,
                    &exact_match);
            }
            break;
        case GUI_KEY_CONTEXT_SEARCH:
            ptr_key = gui_key_search_part (
                NULL,
                GUI_KEY_CONTEXT_SEARCH,
                (const char **)chunks1, chunks1_count,
                (const char **)chunks2, chunks2_count,
                &exact_match);
            if (!ptr_key)
            {
                ptr_key = gui_key_search_part (
                    NULL,
                    GUI_KEY_CONTEXT_DEFAULT,
                    (const char **)chunks1, chunks1_count,
                    (const char **)chunks2, chunks2_count,
                    &exact_match);
            }
            break;
        case GUI_KEY_CONTEXT_CURSOR:
            ptr_key = gui_key_search_part (
                NULL,
                GUI_KEY_CONTEXT_CURSOR,
                (const char **)chunks1, chunks1_count,
                (const char **)chunks2, chunks2_count,
                &exact_match);
            break;
    }

    if (chunks1)
        string_free_split (chunks1);
    if (chunks2)
        string_free_split (chunks2);

    if (ptr_key)
    {
        /*
         * key is found, but it can be partial match
         * (in this case, gui_key_combo is kept and we'll wait for
         * the next key)
         */
        if (exact_match)
        {
            /* exact combo found => execute command */
            if (gui_key_debug)
            {
                gui_key_debug_print_key (gui_key_combo, key_name,
                                         key_name_alias, ptr_key->command, 0);
                gui_key_combo[0] = '\0';
            }
            else
            {
                snprintf (signal_name, sizeof (signal_name),
                          "key_combo_%s", gui_key_context_string[context]);
                rc = hook_signal_send (signal_name,
                                       WEECHAT_HOOK_SIGNAL_STRING,
                                       gui_key_combo);
                gui_key_combo[0] = '\0';
                if ((rc != WEECHAT_RC_OK_EAT) && ptr_key->command)
                {
                    commands = string_split_command (ptr_key->command, ';');
                    if (commands)
                    {
                        for (i = 0; commands[i]; i++)
                        {
                            (void) input_data (gui_current_window->buffer,
                                               commands[i], NULL, 0);
                        }
                        string_free_split (commands);
                    }
                }
            }
        }
        goto end_no_input;
    }

    if (!gui_key_debug)
    {
        if (context == GUI_KEY_CONTEXT_CURSOR)
        {
            signal_sent = 1;
            snprintf (signal_name, sizeof (signal_name),
                      "key_combo_%s", gui_key_context_string[context]);
            if (hook_signal_send (signal_name,
                                  WEECHAT_HOOK_SIGNAL_STRING,
                                  gui_key_combo) == WEECHAT_RC_OK_EAT)
            {
                gui_key_combo[0] = '\0';
                goto end_no_input;
            }
            if (gui_key_focus (gui_key_combo, GUI_KEY_CONTEXT_CURSOR))
            {
                gui_key_combo[0] = '\0';
                goto end_no_input;
            }
        }
        if (!signal_sent)
        {
            snprintf (signal_name, sizeof (signal_name),
                      "key_combo_%s", gui_key_context_string[context]);
            if (hook_signal_send (signal_name,
                                       WEECHAT_HOOK_SIGNAL_STRING,
                                       gui_key_combo) == WEECHAT_RC_OK_EAT)
            {
                gui_key_combo[0] = '\0';
                goto end_no_input;
            }
        }
    }

    if (rc_expand && key_name_alias && key_name_alias[0])
    {
        /* key is complete */
        if (gui_key_debug)
        {
            gui_key_debug_print_key (gui_key_combo, key_name, key_name_alias,
                                     NULL, 0);
        }
        gui_key_combo[0] = '\0';
    }

    /* in debug mode, don't insert anything in input */
    if (gui_key_debug)
        goto end_no_input;

    /*
     * if this is first key and not found (even partial) => return 1
     * else return 0 (= silently discard sequence of bad keys)
     */
    rc = insert_into_input;
    goto end;

end_no_input:
    rc = 0;

end:
    if (key_name)
        free (key_name);
    if (key_name_alias)
        free (key_name_alias);
    return rc;
}

/*
 * Deletes a key binding.
 *
 * If delete_option == 1, the config option is deleted.
 */

void
gui_key_free (int context,
              struct t_gui_key **keys, struct t_gui_key **last_key,
              int *keys_count, struct t_gui_key *key, int delete_option)
{
    struct t_config_option *ptr_option;
    int i;

    if (!key)
        return;

    if (delete_option)
    {
        ptr_option = config_file_search_option (
            weechat_config_file,
            weechat_config_section_key[context],
            key->key);
        if (ptr_option)
            config_file_option_free (ptr_option, 1);
    }

    /* free memory */
    if (key->key)
        free (key->key);
    if (key->chunks)
        string_free_split (key->chunks);
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
 *
 * If delete_option == 1, the config options are deleted.
 */

void
gui_key_free_all (int context, struct t_gui_key **keys,
                  struct t_gui_key **last_key,
                  int *keys_count, int delete_option)
{
    while (*keys)
    {
        gui_key_free (context, keys, last_key, keys_count, *keys,
                      delete_option);
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
            && (gui_key_buffer_size > 1))
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
 * Removes final newline at end of paste.
 */

void
gui_key_paste_remove_newline ()
{
    if ((gui_key_buffer_size > 0)
        && ((gui_key_buffer[gui_key_buffer_size - 1] == '\r')
            || (gui_key_buffer[gui_key_buffer_size - 1] == '\n')))
    {
        gui_key_buffer_size--;
        gui_key_paste_lines--;
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
    gui_key_paste_pending = 1;
    gui_input_paste_pending_signal ();
}

/*
 * Finishes paste of text. Does necessary modifications before flush of text.
 */

void
gui_key_paste_finish ()
{
    gui_key_paste_remove_newline ();
    gui_key_paste_replace_tabs ();
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
 * confirmation to user (ctrl-y=paste, ctrl-n=cancel) (N is option
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
    gui_key_paste_pending = 0;
    gui_input_paste_pending_signal ();
    gui_key_paste_finish ();
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
    int context;

    /* free key buffer */
    if (gui_key_buffer)
        free (gui_key_buffer);

    for (context = 0; context < GUI_KEY_NUM_CONTEXTS; context++)
    {
        /* free keys */
        gui_key_free_all (context,
                          &gui_keys[context],
                          &last_gui_key[context],
                          &gui_keys_count[context],
                          0);
        /* free default keys */
        gui_key_free_all (context,
                          &gui_default_keys[context],
                          &last_gui_default_key[context],
                          &gui_default_keys_count[context],
                          0);
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
    int context;
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
        for (context = 0; context < GUI_KEY_NUM_CONTEXTS; context++)
        {
            snprintf (str_list, sizeof (str_list),
                      "gui_keys%s%s",
                      (context == GUI_KEY_CONTEXT_DEFAULT) ? "" : "_",
                      (context == GUI_KEY_CONTEXT_DEFAULT) ?
                      "" : gui_key_context_string[context]);
            hdata_new_list (hdata, str_list, &gui_keys[context], 0);
            snprintf (str_list, sizeof (str_list),
                      "last_gui_key%s%s",
                      (context == GUI_KEY_CONTEXT_DEFAULT) ? "" : "_",
                      (context == GUI_KEY_CONTEXT_DEFAULT) ?
                      "" : gui_key_context_string[context]);
            hdata_new_list (hdata, str_list, &last_gui_key[context], 0);
            snprintf (str_list, sizeof (str_list),
                      "gui_default_keys%s%s",
                      (context == GUI_KEY_CONTEXT_DEFAULT) ? "" : "_",
                      (context == GUI_KEY_CONTEXT_DEFAULT) ?
                      "" : gui_key_context_string[context]);
            hdata_new_list (hdata, str_list, &gui_default_keys[context], 0);
            snprintf (str_list, sizeof (str_list),
                      "last_gui_default_key%s%s",
                      (context == GUI_KEY_CONTEXT_DEFAULT) ? "" : "_",
                      (context == GUI_KEY_CONTEXT_DEFAULT) ?
                      "" : gui_key_context_string[context]);
            hdata_new_list (hdata, str_list, &last_gui_default_key[context], 0);
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

    if (!infolist || !key)
        return 0;

    ptr_item = infolist_new_item (infolist);
    if (!ptr_item)
        return 0;

    if (!infolist_new_var_string (ptr_item, "key", key->key))
        return 0;
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
    int context;

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
        for (context = 0; context < GUI_KEY_NUM_CONTEXTS; context++)
        {
            log_printf ("");
            log_printf ("[keys for context: %s]", gui_key_context_string[context]);
            log_printf ("  keys . . . . . . . . : 0x%lx", gui_keys[context]);
            log_printf ("  last_key . . . . . . : 0x%lx", last_gui_key[context]);
            log_printf ("  keys_count . . . . . : %d",    gui_keys_count[context]);

            for (ptr_key = gui_keys[context]; ptr_key;
                 ptr_key = ptr_key->next_key)
            {
                log_printf ("");
                gui_key_print_log_key (ptr_key, "");
            }
        }
    }
}
