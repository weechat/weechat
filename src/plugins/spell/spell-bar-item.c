/*
 * spell-bar-item.c - bar items for spell checker plugin
 *
 * Copyright (C) 2012 Nils Görs <weechatter@arcor.de>
 * Copyright (C) 2012-2024 Sébastien Helleu <flashcode@flashtux.org>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "spell.h"
#include "spell-config.h"


/*
 * Returns content of bar item "spell_dict": spell dictionary used on current
 * buffer.
 */

char *
spell_bar_item_dict (const void *pointer, void *data,
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

    dict_list = spell_get_dict (buffer);

    return (dict_list) ? strdup (dict_list) : NULL;
}

/*
 * Returns content of bar item "spell_suggest": spell checker suggestions.
 */

char *
spell_bar_item_suggest (const void *pointer, void *data,
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

    if (!spell_enabled)
        return NULL;

    if (!buffer)
        return NULL;

    ptr_suggestions = weechat_buffer_get_string (buffer,
                                                 "localvar_spell_suggest");
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

    suggestions = weechat_string_split (pos, "/", NULL,
                                        WEECHAT_STRING_SPLIT_STRIP_LEFT
                                        | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                        | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                        0, &num_suggestions);
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
                        spell_config_color_suggestion_delimiter_dict)),
                -1);
            weechat_string_dyn_concat (
                str_suggest,
                weechat_config_string (
                    spell_config_look_suggestion_delimiter_dict),
                -1);
        }
        suggestions2 = weechat_string_split (
            suggestions[i],
            ",",
            NULL,
            WEECHAT_STRING_SPLIT_STRIP_LEFT
            | WEECHAT_STRING_SPLIT_STRIP_RIGHT
            | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
            0,
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
                                spell_config_color_suggestion_delimiter_word)),
                        -1);
                    weechat_string_dyn_concat (
                        str_suggest,
                        weechat_config_string (
                            spell_config_look_suggestion_delimiter_word),
                        -1);
                }
                weechat_string_dyn_concat (
                    str_suggest,
                    weechat_color (
                        weechat_config_string (
                            spell_config_color_suggestion)),
                    -1);
                weechat_string_dyn_concat (str_suggest, suggestions2[j], -1);
            }
            weechat_string_free_split (suggestions2);
        }
    }
    weechat_string_free_split (suggestions);

end:
    return weechat_string_dyn_free (str_suggest, 0);
}

/*
 * Initializes spell bar items.
 */

void
spell_bar_item_init ()
{
    weechat_bar_item_new ("spell_dict",
                          &spell_bar_item_dict, NULL, NULL);
    weechat_bar_item_new ("spell_suggest",
                          &spell_bar_item_suggest, NULL, NULL);
}
