/*
 * fset-mouse.c - mouse actions for Fast Set plugin
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "fset.h"
#include "fset-mouse.h"
#include "fset-bar-item.h"
#include "fset-buffer.h"
#include "fset-config.h"
#include "fset-option.h"


/*
 * Callback called when a mouse action occurs in fset bar item.
 */

struct t_hashtable *
fset_mouse_focus_cb (const void *pointer, void *data, struct t_hashtable *info)
{
    const char *buffer;
    int rc;
    unsigned long value;
    struct t_gui_buffer *ptr_buffer;
    long y;
    char *error, str_value[128];
    struct t_fset_option *ptr_fset_option;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    if (!fset_buffer)
        return info;

    buffer = weechat_hashtable_get (info, "_buffer");
    if (!buffer)
        return info;

    rc = sscanf (buffer, "%lx", &value);
    if ((rc == EOF) || (rc == 0))
        return info;

    ptr_buffer = (struct t_gui_buffer *)value;

    if (!ptr_buffer || (ptr_buffer != fset_buffer))
        return info;

    error = NULL;
    y = strtol (weechat_hashtable_get (info, "_chat_line_y"), &error, 10);
    if (!error || error[0])
        return info;

    if (y < 0)
        return info;

    ptr_fset_option = weechat_arraylist_get (fset_options, y);
    if (!ptr_fset_option)
        return info;

    snprintf (str_value, sizeof (str_value),
              "0x%lx", (unsigned long)ptr_fset_option);
    weechat_hashtable_set (info, "fset_option", str_value);
    weechat_hashtable_set (info, "fset_option_name", ptr_fset_option->name);
    weechat_hashtable_set (info, "fset_option_parent_name", ptr_fset_option->parent_name);
    weechat_hashtable_set (info, "fset_option_type", fset_option_type_string[ptr_fset_option->type]);
    weechat_hashtable_set (info, "fset_option_default_value", ptr_fset_option->default_value);
    weechat_hashtable_set (info, "fset_option_value", ptr_fset_option->value);
    weechat_hashtable_set (info, "fset_option_parent_value", ptr_fset_option->parent_value);
    weechat_hashtable_set (info, "fset_option_min", ptr_fset_option->min);
    weechat_hashtable_set (info, "fset_option_max", ptr_fset_option->max);
    weechat_hashtable_set (info, "fset_option_description", ptr_fset_option->description);
    weechat_hashtable_set (info, "fset_option_string_values", ptr_fset_option->string_values);
    weechat_hashtable_set (info, "fset_option_marked", (ptr_fset_option->marked) ? "1" : "0");

    return info;
}

/*
 * Get distance between x and x2 (as a positive integer);
 */

int
fset_mouse_get_distance_x (struct t_hashtable *hashtable)
{
    int distance, x, x2;
    char *error;

    distance = 0;
    error = NULL;
    x = (int)strtol (weechat_hashtable_get (hashtable, "_chat_line_x"),
                     &error, 10);
    if (error && !error[0])
    {
        error = NULL;
        x2 = (int)strtol (weechat_hashtable_get (hashtable, "_chat_line_x2"),
                          &error, 10);
        if (error && !error[0])
        {
            distance = (x2 - x) / 3;
            if (distance < 0)
                distance *= -1;
            else if (distance == 0)
                distance = 1;
        }
    }
    return distance;
}

/*
 * Gets coordinates: y, y2, chat_line_y, chat_line_y2 from hashtable.
 *
 * Returns:
 *   1: OK, all coordinates are set
 *   0: error (coordinates must not be used)
 */

int
fset_mouse_get_coords (struct t_hashtable *hashtable,
                       int *y, int *y2, int *chat_line_y, int *chat_line_y2)
{
    char *error;
    const char *ptr_value;

    ptr_value = weechat_hashtable_get (hashtable, "_y");
    if (!ptr_value)
        return 0;
    error = NULL;
    *y = (int)strtol (ptr_value, &error, 10);
    if (!error || error[0])
        return 0;

    ptr_value = weechat_hashtable_get (hashtable, "_y2");
    if (!ptr_value)
        return 0;
    error = NULL;
    *y2 = (int)strtol (ptr_value, &error, 10);
    if (!error || error[0])
        return 0;

    ptr_value = weechat_hashtable_get (hashtable, "_chat_line_y");
    if (!ptr_value)
        return 0;
    error = NULL;
    *chat_line_y = (int)strtol (ptr_value, &error, 10);
    if (!error || error[0])
        return 0;

    ptr_value = weechat_hashtable_get (hashtable, "_chat_line_y2");
    if (!ptr_value)
        return 0;
    error = NULL;
    *chat_line_y2 = (int)strtol (ptr_value, &error, 10);
    if (!error || error[0])
        return 0;

    return 1;
}

/*
 * Callback called when a mouse action occurs in fset bar or bar item.
 */

int
fset_mouse_hsignal_cb (const void *pointer, void *data, const char *signal,
                       struct t_hashtable *hashtable)
{
    const char *ptr_key, *ptr_chat_line_y, *ptr_fset_option_pointer;
    char str_command[1024];
    struct t_fset_option *ptr_fset_option;
    unsigned long value;
    int rc, distance, num_options, y, y2, chat_line_y, chat_line_y2;
    int min_y, max_y, i;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) signal;

    if (!fset_buffer)
        return WEECHAT_RC_OK;

    ptr_key = weechat_hashtable_get (hashtable, "_key");
    ptr_chat_line_y = weechat_hashtable_get (hashtable, "_chat_line_y");
    ptr_fset_option_pointer = weechat_hashtable_get (hashtable, "fset_option");

    if (!ptr_key || !ptr_chat_line_y || !ptr_fset_option_pointer)
        return WEECHAT_RC_OK;

    rc = sscanf (ptr_fset_option_pointer, "%lx", &value);
    if ((rc == EOF) || (rc == 0))
        return WEECHAT_RC_OK;
    ptr_fset_option = (struct t_fset_option *)value;
    if (!ptr_fset_option)
        return WEECHAT_RC_OK;

    snprintf (str_command, sizeof (str_command),
              "/fset -go %s",
              ptr_chat_line_y);
    weechat_command (fset_buffer, str_command);

    if (weechat_string_match (ptr_key, "button2-gesture-left*", 1))
    {
        distance = fset_mouse_get_distance_x (hashtable);
        if ((ptr_fset_option->type == FSET_OPTION_TYPE_INTEGER)
            || (ptr_fset_option->type == FSET_OPTION_TYPE_COLOR))
        {
            snprintf (str_command, sizeof (str_command),
                      "/fset -add -%d",
                      distance);
        }
        else
        {
            snprintf (str_command, sizeof (str_command), "/fset -set");
        }
        weechat_command (fset_buffer, str_command);
    }
    else if (weechat_string_match (ptr_key, "button2-gesture-right*", 1))
    {
        distance = fset_mouse_get_distance_x (hashtable);
        if ((ptr_fset_option->type == FSET_OPTION_TYPE_INTEGER)
            || (ptr_fset_option->type == FSET_OPTION_TYPE_COLOR))
        {
            snprintf (str_command, sizeof (str_command),
                      "/fset -add %d",
                      distance);
        }
        else
        {
            snprintf (str_command, sizeof (str_command), "/fset -append");
        }
        weechat_command (fset_buffer, str_command);
    }
    else if (weechat_string_match (ptr_key, "button2*", 1))
    {
        if (fset_mouse_get_coords (hashtable, &y, &y2,
                                   &chat_line_y, &chat_line_y2))
        {
            if (y == y2)
            {
                /* toggle or set option */
                snprintf (
                    str_command, sizeof (str_command),
                    "/fset %s",
                    (ptr_fset_option->type == FSET_OPTION_TYPE_BOOLEAN) ?
                    "-toggle" : "-set");
                weechat_command (fset_buffer, str_command);
            }
            else if ((chat_line_y >= 0) || (chat_line_y2 >= 0))
            {
                /* mark/unmark multiple options */
                num_options = weechat_arraylist_size (fset_options);
                if (chat_line_y < 0)
                    chat_line_y = (y > y2) ? 0 : num_options - 1;
                else if (chat_line_y2 < 0)
                    chat_line_y2 = (y > y2) ? 0 : num_options - 1;
                min_y = (chat_line_y < chat_line_y2) ?
                    chat_line_y : chat_line_y2;
                max_y = (chat_line_y > chat_line_y2) ?
                    chat_line_y : chat_line_y2;
                if (min_y < 0)
                    min_y = 0;
                if (max_y > num_options - 1)
                    max_y = num_options - 1;
                for (i = min_y; i <= max_y; i++)
                {
                    ptr_fset_option = weechat_arraylist_get (fset_options, i);
                    if (ptr_fset_option)
                    {
                        fset_option_toggle_mark (ptr_fset_option, NULL);
                    }
                }
                snprintf (str_command, sizeof (str_command),
                          "/fset -go %d",
                          chat_line_y2);
                weechat_command (fset_buffer, str_command);
            }
        }
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
fset_mouse_init ()
{
    struct t_hashtable *keys;

    keys = weechat_hashtable_new (4,
                                  WEECHAT_HASHTABLE_STRING,
                                  WEECHAT_HASHTABLE_STRING,
                                  NULL, NULL);
    if (!keys)
        return 0;

    weechat_hook_focus ("chat", &fset_mouse_focus_cb, NULL, NULL);

    weechat_hook_hsignal (FSET_MOUSE_HSIGNAL,
                          &fset_mouse_hsignal_cb, NULL, NULL);

    weechat_hashtable_set (
        keys,
        "@chat(" FSET_PLUGIN_NAME "." FSET_BUFFER_NAME "):button1",
        "/window ${_window_number};/fset -go ${_chat_line_y}");
    weechat_hashtable_set (
        keys,
        "@chat(" FSET_PLUGIN_NAME "." FSET_BUFFER_NAME "):button2*",
        "hsignal:" FSET_MOUSE_HSIGNAL);
    weechat_hashtable_set (
        keys,
        "@chat(" FSET_PLUGIN_NAME "." FSET_BUFFER_NAME "):wheelup",
        "/fset -up 5");
    weechat_hashtable_set (
        keys,
        "@chat(" FSET_PLUGIN_NAME "." FSET_BUFFER_NAME "):wheeldown",
        "/fset -down 5");
    weechat_hashtable_set (keys, "__quiet", "1");
    weechat_key_bind ("mouse", keys);

    weechat_hashtable_free (keys);

    return 1;
}

/*
 * Ends mouse.
 */

void
fset_mouse_end ()
{
}
