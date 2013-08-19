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
#include "weechat-aspell-config.h"


/*
 * Returns content of bar item "aspell_dict": aspell dictionary used on current
 * buffer.
 */

char *
weechat_aspell_bar_item_dict (void *data, struct t_gui_bar_item *item,
                              struct t_gui_window *window,
                              struct t_gui_buffer *buffer,
                              struct t_hashtable *extra_info)
{
    const char *dict_list;

    /* make C compiler happy */
    (void) data;
    (void) item;
    (void) window;
    (void) extra_info;

    if (!buffer)
        return NULL;

    dict_list = weechat_aspell_get_dict (buffer);

    return (dict_list) ? strdup (dict_list) : NULL;
}

/*
 * Returns content of bar item "aspell_suggest": aspell suggestions.
 */

char *
weechat_aspell_bar_item_suggest (void *data, struct t_gui_bar_item *item,
                                 struct t_gui_window *window,
                                 struct t_gui_buffer *buffer,
                                 struct t_hashtable *extra_info)
{
    const char *ptr_suggestions, *pos;
    char **suggestions, *suggestions2;
    int i, num_suggestions, length;

    /* make C compiler happy */
    (void) data;
    (void) item;
    (void) window;
    (void) extra_info;

    if (!aspell_enabled)
        return NULL;

    if (!buffer)
        return NULL;

    ptr_suggestions = weechat_buffer_get_string (buffer,
                                                 "localvar_aspell_suggest");
    if (!ptr_suggestions)
        return NULL;

    pos = strchr (ptr_suggestions, ':');
    if (pos)
        pos++;
    else
        pos = ptr_suggestions;
    suggestions = weechat_string_split (pos, "/", 0, 0, &num_suggestions);
    if (suggestions)
    {
        length = 64 + 1;
        for (i = 0; i < num_suggestions; i++)
        {
            length += strlen (suggestions[i]) + 64;
        }
        suggestions2 = malloc (length);
        if (suggestions2)
        {
            suggestions2[0] = '\0';
            strcat (suggestions2,
                    weechat_color (weechat_config_string (weechat_aspell_config_color_suggestions)));
            for (i = 0; i < num_suggestions; i++)
            {
                if (i > 0)
                {
                    strcat (suggestions2, weechat_color ("bar_delim"));
                    strcat (suggestions2, "/");
                    strcat (suggestions2,
                            weechat_color (weechat_config_string (weechat_aspell_config_color_suggestions)));
                }
                strcat (suggestions2, suggestions[i]);
            }
            weechat_string_free_split (suggestions);
            return suggestions2;
        }
        weechat_string_free_split (suggestions);
    }
    return strdup (pos);
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
