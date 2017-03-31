/*
 * buflist-mouse.c - mouse actions for buflist
 *
 * Copyright (C) 2003-2017 Sébastien Helleu <flashcode@flashtux.org>
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "buflist.h"
#include "buflist-mouse.h"
#include "buflist-bar-item.h"
#include "buflist-config.h"


/*
 * Callback called when a mouse action occurs in buflist bar item.
 */

struct t_hashtable *
buflist_focus_cb (const void *pointer, void *data, struct t_hashtable *info)
{
    const char *ptr_bar_item_name, *ptr_bar_item_line;
    long item_line;
    char *error, str_pointer[128], str_number[32];
    struct t_gui_buffer *ptr_buffer;
    struct t_hdata *ptr_hdata;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    if (!buflist_list_buffers)
        goto error;

    /* check bar item name */
    ptr_bar_item_name = weechat_hashtable_get (info, "_bar_item_name");
    if (strcmp (ptr_bar_item_name, BUFLIST_BAR_ITEM_NAME) != 0)
        goto error;

    /* check bar item line */
    ptr_bar_item_line = weechat_hashtable_get (info, "_bar_item_line");
    if (!ptr_bar_item_line)
        goto error;
    item_line = strtol (ptr_bar_item_line, &error, 10);
    if (!error || error[0])
        goto error;
    if ((item_line < 0)
        || (item_line >= weechat_arraylist_size (buflist_list_buffers)))
    {
        goto error;
    }

    /* check if buffer pointer is still valid */
    ptr_buffer = weechat_arraylist_get (buflist_list_buffers, item_line);
    if (!ptr_buffer)
        goto error;
    ptr_hdata = weechat_hdata_get ("buffer");
    if (!weechat_hdata_check_pointer (
            ptr_hdata,
            weechat_hdata_get_list (ptr_hdata, "gui_buffers"),
            ptr_buffer))
    {
        goto error;
    }

    snprintf (str_pointer, sizeof (str_pointer),
              "0x%lx", (long unsigned int)ptr_buffer);
    snprintf (str_number, sizeof (str_number), "%d",
              weechat_buffer_get_integer (ptr_buffer, "number"));

    weechat_hashtable_set (info, "pointer", str_pointer);
    weechat_hashtable_set (info, "number", str_number);
    weechat_hashtable_set (info, "full_name",
                           weechat_buffer_get_string (ptr_buffer, "full_name"));

    return info;

error:
    weechat_hashtable_set (info, "pointer", "");
    weechat_hashtable_set (info, "number", "-1");
    weechat_hashtable_set (info, "full_name", "");

    return info;
}

/*
 * Moves a buffer after a mouse gesture in buflist bar.
 */

void
buflist_mouse_move_buffer (const char *key, struct t_gui_buffer *buffer,
                           int number2)
{
    struct t_hdata *ptr_hdata;
    struct t_gui_buffer *ptr_last_gui_buffer;
    char str_command[128];

    if (!weechat_config_boolean (buflist_config_look_mouse_move_buffer))
        return;

    if (number2 < 0)
    {
        /*
         * if number is now known (end of gesture outside buflist),
         * then set it according to mouse gesture
         */
        number2 = 1;
        if (weechat_string_match (key, "*gesture-right", 1)
            || weechat_string_match (key, "*gesture-down", 1))
        {
            number2 = 999999;
            ptr_hdata = weechat_hdata_get ("buffer");
            ptr_last_gui_buffer = weechat_hdata_get_list (ptr_hdata,
                                                          "last_gui_buffer");
            if (ptr_last_gui_buffer)
            {
                number2 = weechat_hdata_integer (ptr_hdata,
                                                 ptr_last_gui_buffer,
                                                 "number") + 1;
            }
        }
    }

    snprintf (str_command, sizeof (str_command),
              "/buffer move %d", number2);
    weechat_command (buffer, str_command);
}

/*
 * Callback called when a mouse action occurs in buflist bar or bar item.
 */

int
buflist_hsignal_cb (const void *pointer, void *data, const char *signal,
                    struct t_hashtable *hashtable)
{
    const char *ptr_key, *ptr_pointer, *ptr_number, *ptr_number2;
    const char *ptr_full_name;
    struct t_gui_buffer *ptr_buffer;
    char *error, str_command[1024];
    long number, number2;
    long unsigned int value;
    int rc, current_buffer_number;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) signal;

    ptr_key = weechat_hashtable_get (hashtable, "_key");
    ptr_pointer = weechat_hashtable_get (hashtable, "pointer");
    ptr_number = weechat_hashtable_get (hashtable, "number");
    ptr_number2 = weechat_hashtable_get (hashtable, "number2");
    ptr_full_name = weechat_hashtable_get (hashtable, "full_name");

    if (!ptr_key || !ptr_pointer || !ptr_number || !ptr_number2
        || !ptr_full_name)
    {
        return WEECHAT_RC_OK;
    }

    rc = sscanf (ptr_pointer, "%lx", &value);
    if ((rc == EOF) || (rc == 0))
        return WEECHAT_RC_OK;
    ptr_buffer = (struct t_gui_buffer *)value;

    number = strtol (ptr_number, &error, 10);
    if (!error || error[0])
        return WEECHAT_RC_OK;
    number2 = strtol (ptr_number2, &error, 10);
    if (!error || error[0])
        return WEECHAT_RC_OK;

    current_buffer_number = weechat_buffer_get_integer (
        weechat_current_buffer (), "number");

    if (strcmp (ptr_key, "button1") == 0)
    {
        /* left mouse button */
        if (number == number2)
        {
            if (weechat_config_boolean (
                    buflist_config_look_mouse_jump_visited_buffer)
                && (current_buffer_number == number))
            {
                weechat_command (NULL, "/input jump_previously_visited_buffer");
            }
            else
            {
                snprintf (str_command, sizeof (str_command),
                          "/buffer %s", ptr_full_name);
                weechat_command (NULL, str_command);
            }
        }
        else
        {
            /* move buffer */
            buflist_mouse_move_buffer (ptr_key, ptr_buffer, number2);
        }
    }
    else if (strcmp (ptr_key, "button2") == 0)
    {
        if (weechat_config_boolean (
                buflist_config_look_mouse_jump_visited_buffer)
            && (current_buffer_number == number))
        {
            weechat_command (NULL, "/input jump_next_visited_buffer");
        }
    }
    else if (weechat_string_match (ptr_key, "*wheelup", 1))
    {
        if (weechat_config_boolean (buflist_config_look_mouse_wheel))
        {
            weechat_command (NULL, "/buffer -1");
        }
    }
    else if (weechat_string_match (ptr_key, "*wheeldown", 1))
    {
        if (weechat_config_boolean (buflist_config_look_mouse_wheel))
        {
            weechat_command (NULL, "/buffer +1");
        }
    }
    else
    {
        /* move buffer */
        buflist_mouse_move_buffer (ptr_key, ptr_buffer, number2);
    }

    return WEECHAT_RC_OK;
}

/*
 * Initializes mouse.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
buflist_mouse_init ()
{
    struct t_hashtable *keys;

    keys = weechat_hashtable_new (4,
                                  WEECHAT_HASHTABLE_STRING,
                                  WEECHAT_HASHTABLE_STRING,
                                  NULL, NULL);
    if (!keys)
        return 0;

    weechat_hook_focus (BUFLIST_BAR_ITEM_NAME, &buflist_focus_cb, NULL, NULL);

    weechat_hook_hsignal(BUFLIST_MOUSE_HSIGNAL,
                         &buflist_hsignal_cb, NULL, NULL);

    weechat_hashtable_set (keys,
                           "@item(" BUFLIST_BAR_ITEM_NAME "):button1*",
                           "hsignal:" BUFLIST_MOUSE_HSIGNAL);
    weechat_hashtable_set (keys,
                           "@item(" BUFLIST_BAR_ITEM_NAME "):button2*",
                           "hsignal:" BUFLIST_MOUSE_HSIGNAL);
    weechat_hashtable_set (keys,
                           "@bar(" BUFLIST_BAR_NAME "):ctrl-wheelup",
                           "hsignal:" BUFLIST_MOUSE_HSIGNAL);
    weechat_hashtable_set (keys,
                           "@bar(" BUFLIST_BAR_NAME "):ctrl-wheeldown",
                           "hsignal:" BUFLIST_MOUSE_HSIGNAL);
    weechat_hashtable_set (keys, "__quiet", "1");
    weechat_key_bind ("mouse", keys);

    weechat_hashtable_free (keys);

    return 1;
}

/*
 * Ends mouse.
 */

void
buflist_mouse_end ()
{
}
