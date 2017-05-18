/*
 * fset-mouse.c - mouse actions for Fast Set plugin
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
#include "fset.h"
#include "fset-mouse.h"
#include "fset-bar-item.h"
#include "fset-buffer.h"
#include "fset-config.h"


/*
 * Callback called when a mouse action occurs in fset bar item.
 */

struct t_hashtable *
fset_focus_cb (const void *pointer, void *data, struct t_hashtable *info)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;

    return info;
}

/*
 * Callback called when a mouse action occurs in fset bar or bar item.
 */

int
fset_hsignal_cb (const void *pointer, void *data, const char *signal,
                 struct t_hashtable *hashtable)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) signal;

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

    weechat_hook_focus (FSET_BAR_ITEM_NAME, &fset_focus_cb, NULL, NULL);

    weechat_hook_hsignal(FSET_MOUSE_HSIGNAL,
                         &fset_hsignal_cb, NULL, NULL);

    weechat_hashtable_set (
        keys,
        "@chat(" FSET_PLUGIN_NAME "." FSET_BUFFER_NAME "):button1*",
        "hsignal:" FSET_MOUSE_HSIGNAL);
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
