/*
 * buflist-mouse.c - mouse actions for buflist
 *
 * Copyright (C) 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
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
    const char *ptr_bar_item_name, *ptr_bar_item_line, *keys, *ptr_value;
    long item_line;
    char *error, str_value[128], **list_keys;
    int i, item_index, num_keys, type;
    struct t_gui_buffer *ptr_buffer;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    ptr_buffer = NULL;

    /* check bar item name */
    ptr_bar_item_name = weechat_hashtable_get (info, "_bar_item_name");
    item_index = buflist_bar_item_get_index (ptr_bar_item_name);
    if (item_index < 0)
        goto end;

    if (!buflist_list_buffers[item_index])
        goto end;

    /* check bar item line */
    ptr_bar_item_line = weechat_hashtable_get (info, "_bar_item_line");
    if (!ptr_bar_item_line)
        goto end;
    error = NULL;
    item_line = strtol (ptr_bar_item_line, &error, 10);
    if (!error || error[0])
        goto end;
    if ((item_line < 0)
        || (item_line >= weechat_arraylist_size (buflist_list_buffers[item_index])))
    {
        goto end;
    }

    /* check if buffer pointer is still valid */
    ptr_buffer = weechat_arraylist_get (buflist_list_buffers[item_index],
                                        item_line);
    if (!ptr_buffer)
        goto end;
    if (!weechat_hdata_check_pointer (
            buflist_hdata_buffer,
            weechat_hdata_get_list (buflist_hdata_buffer, "gui_buffers"),
            ptr_buffer))
    {
        ptr_buffer = NULL;
    }

end:
    /* get list of keys */
    keys = weechat_hdata_get_string (buflist_hdata_buffer, "var_keys");
    list_keys = weechat_string_split (keys, ",", NULL,
                                      WEECHAT_STRING_SPLIT_STRIP_LEFT
                                      | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                      | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                      0, &num_keys);
    if (!list_keys)
        return info;

    /* browse keys and add values in hashtable */
    for (i = 0; i < num_keys; i++)
    {
        type = weechat_hdata_get_var_type (buflist_hdata_buffer, list_keys[i]);
        switch (type)
        {
            case WEECHAT_HDATA_CHAR:
                snprintf (str_value, sizeof (str_value),
                          "%c",
                          weechat_hdata_char (buflist_hdata_buffer,
                                              ptr_buffer, list_keys[i]));
                weechat_hashtable_set (info, list_keys[i], str_value);
                break;
            case WEECHAT_HDATA_INTEGER:
                snprintf (str_value, sizeof (str_value),
                          "%d",
                          (ptr_buffer) ?
                          weechat_hdata_integer (buflist_hdata_buffer,
                                                 ptr_buffer, list_keys[i]) : -1);
                weechat_hashtable_set (info, list_keys[i], str_value);
                break;
            case WEECHAT_HDATA_LONG:
                snprintf (str_value, sizeof (str_value),
                          "%ld",
                          (ptr_buffer) ?
                          weechat_hdata_long (buflist_hdata_buffer,
                                              ptr_buffer, list_keys[i]) : -1);
                weechat_hashtable_set (info, list_keys[i], str_value);
                break;
            case WEECHAT_HDATA_LONGLONG:
                snprintf (str_value, sizeof (str_value),
                          "%lld",
                          (ptr_buffer) ?
                          weechat_hdata_longlong (buflist_hdata_buffer,
                                                  ptr_buffer, list_keys[i]) : 0);
                weechat_hashtable_set (info, list_keys[i], str_value);
                break;
            case WEECHAT_HDATA_STRING:
            case WEECHAT_HDATA_SHARED_STRING:
                ptr_value = weechat_hdata_string (buflist_hdata_buffer,
                                                  ptr_buffer,
                                                  list_keys[i]);
                weechat_hashtable_set (info, list_keys[i],
                                       (ptr_value) ? ptr_value : "");
                break;
            case WEECHAT_HDATA_TIME:
                snprintf (str_value, sizeof (str_value),
                          "%lld",
                          (ptr_buffer) ?
                          (long long)weechat_hdata_time (buflist_hdata_buffer,
                                                         ptr_buffer, list_keys[i]) : -1);
                weechat_hashtable_set (info, list_keys[i], str_value);
                break;
            default:  /* ignore other types */
                break;
        }
    }

    /* add pointer and plugin name */
    snprintf (str_value, sizeof (str_value), "0x%lx", (unsigned long)ptr_buffer);
    weechat_hashtable_set (info, "pointer", str_value);
    weechat_hashtable_set (info, "plugin",
                           weechat_buffer_get_string (ptr_buffer, "plugin"));

    /* add some local variables */
    ptr_value = weechat_buffer_get_string (ptr_buffer, "localvar_type");
    weechat_hashtable_set (info, "localvar_type",
                           (ptr_value) ? ptr_value : "");
    ptr_value = weechat_buffer_get_string (ptr_buffer, "localvar_server");
    weechat_hashtable_set (info, "localvar_server",
                           (ptr_value) ? ptr_value : "");
    ptr_value = weechat_buffer_get_string (ptr_buffer, "localvar_channel");
    weechat_hashtable_set (info, "localvar_channel",
                           (ptr_value) ? ptr_value : "");
    ptr_value = weechat_buffer_get_string (ptr_buffer, "localvar_lag");
    weechat_hashtable_set (info, "localvar_lag",
                           (ptr_value) ? ptr_value : "");

    weechat_string_free_split (list_keys);

    return info;
}

/*
 * Moves a buffer after a mouse gesture in buflist bar.
 */

void
buflist_mouse_move_buffer (const char *key, struct t_gui_buffer *buffer,
                           int number2)
{
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
        if (weechat_string_match (key, "*gesture-right*", 1)
            || weechat_string_match (key, "*gesture-down*", 1))
        {
            number2 = 999999;
            ptr_last_gui_buffer = weechat_hdata_get_list (buflist_hdata_buffer,
                                                          "last_gui_buffer");
            if (ptr_last_gui_buffer)
            {
                number2 = weechat_hdata_integer (buflist_hdata_buffer,
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
 * Switches to previous/next buffer displayed in an item, starting from
 * current buffer.
 */

void
buflist_mouse_move_current_buffer (const char *item_name, int direction)
{
    struct t_gui_buffer *ptr_current_buffer, *ptr_buffer, *gui_buffers;
    char str_command[1024];
    int i, size, index_item, index_current, index2, number_current;
    int number, number2;

    if (!item_name)
        return;

    index_item = buflist_bar_item_get_index (item_name);
    if (index_item < 0)
        return;

    if (!buflist_list_buffers[index_item])
        return;

    size = weechat_arraylist_size (buflist_list_buffers[index_item]);
    if (size <= 0)
        return;

    ptr_current_buffer = weechat_current_buffer ();
    if (!ptr_current_buffer)
        return;

    index_current = -1;
    for (i = 0; i < size; i++)
    {
        if ((struct t_gui_buffer *)weechat_arraylist_get (
                buflist_list_buffers[index_item], i) == ptr_current_buffer)
        {
            index_current = i;
            break;
        }
    }

    if (index_current < 0)
        return;

    number_current = weechat_buffer_get_integer (ptr_current_buffer, "number");

    gui_buffers = weechat_hdata_get_list (buflist_hdata_buffer, "gui_buffers");

    /* search previous/next buffer with a different number */
    index2 = index_current;
    while (1)
    {
        if (direction < 0)
        {
            index2--;
            if (index2 < 0)
                index2 = size - 1;
        }
        else
        {
            index2++;
            if (index2 >= size)
                index2 = 0;
        }
        if (index2 == index_current)
            return;
        ptr_buffer = (struct t_gui_buffer *)weechat_arraylist_get (
            buflist_list_buffers[index_item], index2);
        if (!ptr_buffer)
            return;
        if (!weechat_hdata_check_pointer (buflist_hdata_buffer,
                                          gui_buffers, ptr_buffer))
            return;
        number2 = weechat_buffer_get_integer (ptr_buffer, "number");
        if (number2 != number_current)
            break;
    }

    /* search first buffer with the number found */
    for (i = 0; i < size; i++)
    {
        ptr_buffer = (struct t_gui_buffer *)weechat_arraylist_get (
            buflist_list_buffers[index_item], i);
        if (!ptr_buffer)
            break;
        number = weechat_buffer_get_integer (ptr_buffer, "number");
        if (number == number2)
            break;
    }
    if (i >= size)
        return;

    /* switch to the buffer found */
    snprintf (str_command, sizeof (str_command),
              "/buffer %s",
              weechat_buffer_get_string (ptr_buffer, "full_name"));
    weechat_command (NULL, str_command);
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

    rc = sscanf (ptr_pointer, "%p", &ptr_buffer);
    if ((rc == EOF) || (rc == 0))
        return WEECHAT_RC_OK;

    error = NULL;
    number = strtol (ptr_number, &error, 10);
    if (!error || error[0])
        return WEECHAT_RC_OK;
    error = NULL;
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
                weechat_command (NULL, "/buffer jump prev_visited");
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
            weechat_command (NULL, "/buffer jump next_visited");
        }
    }
    else if (weechat_string_match (ptr_key, "*wheelup", 1))
    {
        if (weechat_config_boolean (buflist_config_look_mouse_wheel))
        {
            buflist_mouse_move_current_buffer (
                (const char *)weechat_hashtable_get (hashtable, "_bar_item_name"),
                -1);  /* previous buffer */
        }
    }
    else if (weechat_string_match (ptr_key, "*wheeldown", 1))
    {
        if (weechat_config_boolean (buflist_config_look_mouse_wheel))
        {
            buflist_mouse_move_current_buffer (
                (const char *)weechat_hashtable_get (hashtable, "_bar_item_name"),
                +1);  /* next buffer */
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
buflist_mouse_init (void)
{
    int i;

    for (i = 0; i < BUFLIST_BAR_NUM_ITEMS; i++)
    {
        weechat_hook_focus (buflist_bar_item_get_name (i),
                            &buflist_focus_cb, NULL, NULL);
    }

    weechat_hook_hsignal (BUFLIST_MOUSE_HSIGNAL,
                          &buflist_hsignal_cb, NULL, NULL);

    return 1;
}

/*
 * Ends mouse.
 */

void
buflist_mouse_end (void)
{
}
