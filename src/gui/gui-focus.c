/*
 * Copyright (C) 2011 Sebastien Helleu <flashcode@flashtux.org>
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
 * gui-focus.c: functions about focus (for cursor mode and mouse) (used by all GUI)
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>

#include "../core/weechat.h"
#include "../core/wee-hashtable.h"
#include "../plugins/plugin.h"
#include "gui-bar.h"
#include "gui-bar-window.h"
#include "gui-buffer.h"
#include "gui-focus.h"
#include "gui-window.h"


/*
 * gui_focus_get_info: get info about what is pointed by cursor at (x,y)
 */

void
gui_focus_get_info (int x, int y, struct t_gui_focus_info *focus_info)
{
    focus_info->x = x;
    focus_info->y = y;
    
    /* search window */
    focus_info->window = gui_window_search_by_xy (x, y);
    
    /* chat area in this window? */
    if (focus_info->window
        && (x >= (focus_info->window)->win_chat_x)
        && (y >= (focus_info->window)->win_chat_y)
        && (x <= (focus_info->window)->win_chat_x + (focus_info->window)->win_chat_width - 1)
        && (y <= (focus_info->window)->win_chat_y + (focus_info->window)->win_chat_height - 1))
    {
        focus_info->chat = 1;
    }
    else
        focus_info->chat = 0;
    
    /* search bar window, item, and line/col in item */
    gui_bar_window_search_by_xy (focus_info->window,
                                 x, y,
                                 &focus_info->bar_window,
                                 &focus_info->bar_item,
                                 &focus_info->item_line,
                                 &focus_info->item_col);
}

/*
 * gui_focus_to_hashtable: add two focus info into hashtable
 */

struct t_hashtable *
gui_focus_to_hashtable (struct t_gui_focus_info *focus_info, const char *key)
{
    struct t_hashtable *hashtable;
    char str_value[128];
    
    hashtable = hashtable_new (32,
                               WEECHAT_HASHTABLE_STRING,
                               WEECHAT_HASHTABLE_STRING,
                               NULL,
                               NULL);
    if (!hashtable)
        return NULL;
    
    /* key (key from keyboard or mouse event) */
    hashtable_set (hashtable, "_key", key);
    
    /* x,y */
    snprintf (str_value, sizeof (str_value), "%d", focus_info->x);
    hashtable_set (hashtable, "_x", str_value);
    snprintf (str_value, sizeof (str_value), "%d", focus_info->y);
    hashtable_set (hashtable, "_y", str_value);
    
    /* window/buffer */
    snprintf (str_value, sizeof (str_value),
              "0x%lx", (long unsigned int)focus_info->window);
    hashtable_set (hashtable, "_window", str_value);
    snprintf (str_value, sizeof (str_value),
              "0x%lx",
              (focus_info->window) ?
              (long unsigned int)((focus_info->window)->buffer) : 0);
    hashtable_set (hashtable, "_buffer", str_value);
    if (focus_info->window)
    {
        snprintf (str_value, sizeof (str_value), "%d",
                  (focus_info->window)->number);
        hashtable_set (hashtable, "_window_number", str_value);
        snprintf (str_value, sizeof (str_value), "%d",
                  ((focus_info->window)->buffer)->number);
        hashtable_set (hashtable, "_buffer_number", str_value);
        hashtable_set (hashtable, "_buffer_plugin",
                       plugin_get_name (((focus_info->window)->buffer)->plugin));
        hashtable_set (hashtable, "_buffer_name",
                       ((focus_info->window)->buffer)->name);
    }
    else
    {
        hashtable_set (hashtable, "_window_number", "*");
        hashtable_set (hashtable, "_buffer_number", "");
        hashtable_set (hashtable, "_buffer_plugin", "");
        hashtable_set (hashtable, "_buffer_name", "");
    }
    hashtable_set (hashtable, "_chat", (focus_info->chat) ? "1" : "0");
    
    /* bar/item */
    hashtable_set (hashtable, "_bar_name",
                   (focus_info->bar_window) ?
                   ((focus_info->bar_window)->bar)->name : "");
    hashtable_set (hashtable, "_bar_filling",
                   (focus_info->bar_window) ?
                   gui_bar_filling_string[gui_bar_get_filling ((focus_info->bar_window)->bar)] : "");
    hashtable_set (hashtable, "_bar_item_name",
                   (focus_info->bar_item) ? focus_info->bar_item : "");
    snprintf (str_value, sizeof (str_value), "%d", focus_info->item_line);
    hashtable_set (hashtable, "_item_line", str_value);
    snprintf (str_value, sizeof (str_value), "%d", focus_info->item_col);
    hashtable_set (hashtable, "_item_col", str_value);
    
    return hashtable;
}
