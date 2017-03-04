/*
 * buflist.c - Bar with list of buffers
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
#include "buflist.h"
#include "buflist-bar-item.h"
#include "buflist-config.h"


WEECHAT_PLUGIN_NAME(BUFLIST_PLUGIN_NAME);
WEECHAT_PLUGIN_DESCRIPTION(N_("Buffers list"));
WEECHAT_PLUGIN_AUTHOR("Sébastien Helleu <flashcode@flashtux.org>");
WEECHAT_PLUGIN_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_LICENSE(WEECHAT_LICENSE);
WEECHAT_PLUGIN_PRIORITY(8000);

struct t_weechat_plugin *weechat_buflist_plugin = NULL;


/*
 * Callback for a signal on a buffer.
 */

int
buflist_signal_buffer_cb (const void *pointer, void *data,
                          const char *signal, const char *type_data,
                          void *signal_data)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) signal;
    (void) type_data;
    (void) signal_data;

    weechat_bar_item_update ("buflist");

    return WEECHAT_RC_OK;
}

/*
 * Initializes buflist plugin.
 */

int
weechat_plugin_init (struct t_weechat_plugin *plugin, int argc, char *argv[])
{
    char *signals_buffers[] =
    { "buffer_opened", "buffer_closed", "buffer_merged", "buffer_unmerged",
      "buffer_moved", "buffer_renamed", "buffer_switch", "buffer_hidden",
      "buffer_unhidden", "buffer_localvar_added", "buffer_localvar_changed",
      "window_switch", NULL
    };
    int i;

    /* make C compiler happy */
    (void) argc;
    (void) argv;

    weechat_plugin = plugin;

    if (!buflist_config_init ())
        return WEECHAT_RC_ERROR;

    buflist_config_read ();

    /* hook some signals */
    for (i = 0; signals_buffers[i]; i++)
    {
        weechat_hook_signal (signals_buffers[i],
                             &buflist_signal_buffer_cb, NULL, NULL);
    }

    buflist_bar_item_init ();

    weechat_bar_new (BUFLIST_BAR_NAME, "off", "0", "root", "", "left",
                     "columns_vertical", "vertical", "0", "0",
                     "default", "default", "default", "on",
                     BUFLIST_BAR_ITEM_NAME);

    return WEECHAT_RC_OK;
}

/*
 * Ends buflist plugin.
 */

int
weechat_plugin_end (struct t_weechat_plugin *plugin)
{
    /* make C compiler happy */
    (void) plugin;

    buflist_config_write ();
    buflist_config_free ();

    return WEECHAT_RC_OK;
}
