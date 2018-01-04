/*
 * weechat-aspell-bar-item.c - bar items for aspell plugin
 *
 * Copyright (C) 2012 Nils Görs <weechatter@arcor.de>
 * Copyright (C) 2012-2018 Sébastien Helleu <flashcode@flashtux.org>
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
weechat_aspell_bar_item_dict (const void *pointer, void *data,
                              struct t_gui_bar_item *item,
                              struct t_gui_window *window,
                              struct t_gui_buffer *buffer,
                              struct t_hashtable *extra_info)
{
    const char *dict_list;

    /* make C compiler happy */
    (void) pointer;
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
weechat_aspell_bar_item_suggest (const void *pointer, void *data,
                                 struct t_gui_bar_item *item,
                                 struct t_gui_window *window,
                                 struct t_gui_buffer *buffer,
                                 struct t_hashtable *extra_info)
{
    const char *ptr_suggestions, *pos;
    char **suggestions, **suggestions2, **str_suggest;
    int i, j, num_suggestions, num_suggestions2;

    /* make C compiler happy */
    (void) pointer;
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

    str_suggest = weechat_string_dyn_alloc (256);
    if (!str_suggest)
        return NULL;

    suggestions = weechat_string_split (pos, "/", 0, 0, &num_suggestions);
    if (!suggestions)
        goto end;

    for (i = 0; i < num_suggestions; i++)
    {
        if (i > 0)
        {
            weechat_string_dyn_concat (
                str_suggest,
                weechat_color (
                    weechat_config_string (
                        weechat_aspell_config_color_suggestion_delimiter_dict)));
            weechat_string_dyn_concat (
                str_suggest,
                weechat_config_string (
                    weechat_aspell_config_look_suggestion_delimiter_dict));
        }
        suggestions2 = weechat_string_split (suggestions[i], ",", 0, 0,
                                             &num_suggestions2);
        if (suggestions2)
        {
            for (j = 0; j < num_suggestions2; j++)
            {
                if (j > 0)
                {
                    weechat_string_dyn_concat (
                        str_suggest,
                        weechat_color (
                            weechat_config_string (
                                weechat_aspell_config_color_suggestion_delimiter_word)));
                    weechat_string_dyn_concat (
                        str_suggest,
                        weechat_config_string (
                            weechat_aspell_config_look_suggestion_delimiter_word));
                }
                weechat_string_dyn_concat (
                    str_suggest,
                    weechat_color (
                        weechat_config_string (
                            weechat_aspell_config_color_suggestion)));
                weechat_string_dyn_concat (str_suggest, suggestions2[j]);
            }
            weechat_string_free_split (suggestions2);
        }
    }
    weechat_string_free_split (suggestions);

end:
    return weechat_string_dyn_free (str_suggest, 0);
}

/*
 * Initializes aspell bar items.
 */

void
weechat_aspell_bar_item_init ()
{
    weechat_bar_item_new ("aspell_dict",
                          &weechat_aspell_bar_item_dict, NULL, NULL);
    weechat_bar_item_new ("aspell_suggest",
                          &weechat_aspell_bar_item_suggest, NULL, NULL);
}
