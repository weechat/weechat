/*
 * buflist-command.c - buflist command
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
#include <time.h>

#include "../weechat-plugin.h"
#include "buflist.h"
#include "buflist-bar-item.h"
#include "buflist-command.h"


/*
 * Callback for command "/buflist".
 */

int
buflist_command_buflist (const void *pointer, void *data,
                         struct t_gui_buffer *buffer, int argc,
                         char **argv, char **argv_eol)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) buffer;
    (void) argc;
    (void) argv;
    (void) argv_eol;

    if (argc == 1)
        return WEECHAT_RC_OK;

    if (weechat_strcasecmp (argv[1], "refresh") == 0)
    {
        weechat_bar_item_update (BUFLIST_BAR_ITEM_NAME);
        return WEECHAT_RC_OK;
    }

    WEECHAT_COMMAND_ERROR;
}

/*
 * Hooks buflist commands.
 */

void
buflist_command_init ()
{
    weechat_hook_command (
        "buflist",
        N_("bar item with list of buffers"),
        "refresh",
        N_("refresh: force the refresh of the \"buflist\" bar item\n"
           "\n"
           "The lines with buffers are displayed using string evaluation "
           "(see /help eval for the format), with these options:\n"
           "  - buflist.look.display_conditions: conditions to display a "
           "buffer in the list\n"
           "  - buflist.format.buffer: format for a buffer which is not "
           "current buffer\n"
           "  - buflist.format.buffer_current: format for the current buffer\n"
           "\n"
           "The following variables can be used in these options:\n"
           "  - buffer data (see hdata \"buffer\" in API doc for a complete "
           "list), for example:\n"
           "    - ${buffer.number}\n"
           "    - ${buffer.name}\n"
           "    - ${buffer.full_name}\n"
           "    - ${buffer.short_name}\n"
           "    - ${buffer.nicklist_nicks_count}\n"
           "  - extra variables added by buflist for convenience:\n"
           "    - ${format_buffer}: the evaluated value of option "
           "buflist.format.buffer; this can be used in option "
           "buflist.format.buffer_current to just change the background color "
           "for example\n"
           "    - ${format_number}: indented number with separator "
           "(evaluation of option buflist.format.number)\n"
           "    - ${number}: indented number, for example \" 1\" if there "
           "are between 10 and 99 buffers\n"
           "    - ${indent}: indentation for name (channel and private "
           "buffers are indented)\n"
           "    - ${nick_prefix}: nick prefix for a channel (set only "
           "if the option buflist.look.nick_prefix is enabled)\n"
           "    - ${name}: the short name (if set), with a fallback on the "
           "name\n"
           "    - ${color_hotlist}: the color depending on the highest "
           "hotlist level for the buffer (evaluation of option "
           "buflist.format.hotlist_xxx where xxx is the level)\n"
           "    - ${format_hotlist}: the formatted hotlist (evaluation "
           "of option buflist.format.hotlist)\n"
           "    - ${hotlist}: the raw hotlist\n"
           "    - ${format_lag}: the lag for an IRC server buffer, empty if "
           "there's no lag (evaluation of option buflist.format.lag)"),
        "refresh",
        &buflist_command_buflist, NULL, NULL);
}
