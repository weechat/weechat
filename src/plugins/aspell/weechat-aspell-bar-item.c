/*
 * Copyright (C) 2012 Nils Görs <weechatter@arcor.de>
 * Copyright (C) 2012 Sebastien Helleu <flashcode@flashtux.org>
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
 * weechat-aspell-bar-item.c: bar items for aspell plugin
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "weechat-aspell.h"


/*
 * weechat_aspell_bar_item_dict: bar item with aspell dictionary used on
 *                               current buffer
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
        dict_list = weechat_aspell_get_dict (buffer);
        if (dict_list)
            return strdup (dict_list);
    }
    return NULL;
}

/*
 * weechat_aspell_bar_item_init: initialize aspell bar items
 */

void
weechat_aspell_bar_item_init ()
{
    weechat_bar_item_new ("aspell_dict", &weechat_aspell_bar_item_dict, NULL);
}
