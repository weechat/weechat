/*
 * fset-bar-item.c - bar item for Fast Set plugin
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

#include "../weechat-plugin.h"
#include "fset.h"
#include "fset-bar-item.h"
#include "fset-config.h"


struct t_gui_bar_item *fset_bar_item_fset = NULL;


/*
 * Updates fset bar item if fset is enabled.
 */

void
fset_bar_item_update ()
{
    if (weechat_config_boolean (fset_config_look_enabled))
        weechat_bar_item_update (FSET_BAR_ITEM_NAME);
}

/*
 * Returns content of bar item "buffer_plugin": bar item with buffer plugin.
 */

char *
fset_bar_item_fset_cb (const void *pointer, void *data,
                       struct t_gui_bar_item *item,
                       struct t_gui_window *window,
                       struct t_gui_buffer *buffer,
                       struct t_hashtable *extra_info)
{
    return NULL;
}

/*
 * Initializes fset bar items.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
fset_bar_item_init ()
{
    fset_bar_item_fset = weechat_bar_item_new (
        FSET_BAR_ITEM_NAME,
        &fset_bar_item_fset_cb, NULL, NULL);

    return 1;
}

/*
 * Ends fset bar items.
 */

void
fset_bar_item_end ()
{
    weechat_bar_item_remove (fset_bar_item_fset);
}
