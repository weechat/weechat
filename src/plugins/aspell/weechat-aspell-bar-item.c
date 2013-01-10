/*
 * weechat-aspell-bar-item.c - bar items for aspell plugin
 *
 * Copyright (C) 2012 Nils Görs <weechatter@arcor.de>
 * Copyright (C) 2012-2013 Sebastien Helleu <flashcode@flashtux.org>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "weechat-aspell.h"


/*
 * Returns content of bar item "aspell_dict": aspell dictionary used on current
 * buffer.
 */

char *
weechat_aspell_bar_item_dict (void *data, struct t_gui_bar_item *item,
                              struct t_gui_window *window)
{
    struct t_gui_buffer *buffer;
    const char *dict_list;

    /* make C compiler happy */
    (void) data;
    (void) item;

    if (!window)
        window = weechat_current_window ();

    buffer = weechat_window_get_pointer (window, "buffer");
    if (buffer)
    {
        dict_list = weechat_buffer_get_string (buffer,
                                               "localvar_aspell_dict");
        if (dict_list)
            return strdup (dict_list);
    }

    return NULL;
}

/*
 * Returns content of bar item "aspell_suggest": aspell suggestions.
 */

char *
weechat_aspell_bar_item_suggest (void *data, struct t_gui_bar_item *item,
                                 struct t_gui_window *window)
{
    struct t_gui_buffer *buffer;
    const char *suggestions, *pos;
    char str_delim[128], *suggestions2;

    /* make C compiler happy */
    (void) data;
    (void) item;

    if (!aspell_enabled)
        return NULL;

    if (!window)
        window = weechat_current_window ();

    buffer = weechat_window_get_pointer (window, "buffer");
    if (buffer)
    {
        suggestions = weechat_buffer_get_string (buffer,
                                                 "localvar_aspell_suggest");
        if (suggestions)
        {
            pos = strchr (suggestions, ':');
            if (pos)
                pos++;
            else
                pos = suggestions;
            snprintf (str_delim, sizeof (str_delim),
                      "%s/%s",
                      weechat_color ("bar_delim"),
                      weechat_color ("bar_fg"));
            suggestions2 = weechat_string_replace (pos, "/", str_delim);
            if (suggestions2)
                return suggestions2;
            return strdup (pos);
        }
    }

    return NULL;
}

/*
 * Initializes aspell bar items.
 */

void
weechat_aspell_bar_item_init ()
{
    weechat_bar_item_new ("aspell_dict", &weechat_aspell_bar_item_dict, NULL);
    weechat_bar_item_new ("aspell_suggest", &weechat_aspell_bar_item_suggest, NULL);
}
