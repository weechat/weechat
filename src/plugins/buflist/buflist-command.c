/*
 * buflist-command.c - buflist command
 *
 * Copyright (C) 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
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

#include <stdlib.h>

#include "../weechat-plugin.h"
#include "buflist.h"
#include "buflist-bar-item.h"
#include "buflist-command.h"
#include "buflist-config.h"


/*
 * Callback for command "/buflist".
 */

int
buflist_command_buflist (const void *pointer, void *data,
                         struct t_gui_buffer *buffer, int argc,
                         char **argv, char **argv_eol)
{
    int i, index;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) buffer;
    (void) argv_eol;

    if (argc == 1)
        return WEECHAT_RC_OK;

    if (weechat_strcmp (argv[1], "enable") == 0)
    {
        weechat_config_option_set (buflist_config_look_enabled, "on", 1);
        return WEECHAT_RC_OK;
    }

    if (weechat_strcmp (argv[1], "disable") == 0)
    {
        weechat_config_option_set (buflist_config_look_enabled, "off", 1);
        return WEECHAT_RC_OK;
    }

    if (weechat_strcmp (argv[1], "toggle") == 0)
    {
        weechat_config_option_set (buflist_config_look_enabled, "toggle", 1);
        return WEECHAT_RC_OK;
    }

    if (weechat_strcmp (argv[1], "bar") == 0)
    {
        buflist_add_bar ();
        return WEECHAT_RC_OK;
    }

    if (weechat_strcmp (argv[1], "refresh") == 0)
    {
        if (argc > 2)
        {
            for (i = 2; i < argc; i++)
            {
                index = buflist_bar_item_get_index (argv[i]);
                if (index >= 0)
                    buflist_bar_item_update (index, 0);
            }
        }
        else
        {
            /* refresh all bar items used */
            buflist_bar_item_update (-1, 0);
        }
        return WEECHAT_RC_OK;
    }

    WEECHAT_COMMAND_ERROR;
}

/*
 * Hooks buflist commands.
 */

void
buflist_command_init (void)
{
    weechat_hook_command (
        "buflist",
        N_("bar item with list of buffers"),
        "enable|disable|toggle"
        " || bar"
        " || refresh [<item>[,<item>...]]",
        WEECHAT_CMD_ARGS_DESC(
            N_("raw[enable]: enable buflist"),
            N_("raw[disable]: disable buflist"),
            N_("raw[toggle]: toggle buflist"),
            N_("raw[bar]: add the \"buflist\" bar"),
            N_("raw[refresh]: force the refresh of some bar items (if no item "
               "is given, all bar items used are refreshed, according to option "
               "buflist.look.use_items)"),
            "",
            N_("The lines with buffers are displayed using string evaluation "
               "(see /help eval for the format), with these options:"),
            N_("  - buflist.look.display_conditions: conditions to display a "
               "buffer in the list"),
            N_("  - buflist.format.buffer: format for a buffer which is not "
               "current buffer"),
            N_("  - buflist.format.buffer_current: format for the current buffer"),
            "",
            N_("The following variables can be used in these options:"),
            N_("  - bar item data (see hdata \"bar_item\" in API doc for a "
               "complete list), for example:"),
            AI("    - ${bar_item.name}"),
            N_("  - window data, where the bar item is displayed (there's no "
               "window in root bars, see hdata \"window\" in API doc for a "
               "complete list), for example:"),
            AI("    - ${window.number}"),
            AI("    - ${window.buffer.full_name}"),
            N_("  - buffer data (see hdata \"buffer\" in API doc for a complete "
               "list), for example:"),
            AI("    - ${buffer.number}"),
            AI("    - ${buffer.name}"),
            AI("    - ${buffer.full_name}"),
            AI("    - ${buffer.short_name}"),
            AI("    - ${buffer.nicklist_nicks_count}"),
            N_("  - irc_server: IRC server data, defined only on an IRC buffer "
               "(see hdata \"irc_server\" in API doc)"),
            N_("  - irc_channel: IRC channel data, defined only on an IRC channel "
               "buffer (see hdata \"irc_channel\" in API doc)"),
            N_("  - extra variables added by buflist for convenience:"),
            N_("    - ${format_buffer}: the evaluated value of option "
               "buflist.format.buffer; this can be used in option "
               "buflist.format.buffer_current to just change the background color "
               "for example"),
            N_("    - ${current_buffer}: a boolean (\"0\" or \"1\"), \"1\" if "
               "this is the current buffer; it can be used in a condition: "
               "${if:${current_buffer}?...:...}"),
            N_("    - ${merged}: a boolean (\"0\" or \"1\"), \"1\" if the "
               "buffer is merged with at least another buffer; it can be used "
               "in a condition: ${if:${merged}?...:...}"),
            N_("    - ${format_number}: indented number with separator "
               "(evaluation of option buflist.format.number)"),
            N_("    - ${number}: indented number, for example \" 1\" if there "
               "are between 10 and 99 buffers; for merged buffers, this variable "
               "is set with number for the first buffer and spaces for the next "
               "buffers with same number"),
            N_("    - ${number2}: indented number, for example \" 1\" if there "
               "are between 10 and 99 buffers"),
            N_("    - ${number_displayed}: \"1\" if the number is displayed, "
               "otherwise \"0\""),
            N_("    - ${indent}: indentation for name (channel, private and list "
               "buffers are indented) (evaluation of "
               "option buflist.format.indent)"),
            N_("    - ${format_nick_prefix}: colored nick prefix for a channel "
               "(evaluation of option buflist.format.nick_prefix)"),
            N_("    - ${color_nick_prefix}: color of nick prefix for a channel "
               "(set only if the option buflist.look.nick_prefix is enabled)"),
            N_("    - ${nick_prefix}: nick prefix for a channel "
               "(set only if the option buflist.look.nick_prefix is enabled)"),
            N_("    - ${format_name}: formatted name (evaluation of option "
               "buflist.format.name)"),
            N_("    - ${name}: the short name (if set), with a fallback on the "
               "name"),
            N_("    - ${color_hotlist}: the color depending on the highest "
               "hotlist level for the buffer (evaluation of option "
               "buflist.format.hotlist_xxx where xxx is the level)"),
            N_("    - ${format_hotlist}: the formatted hotlist (evaluation "
               "of option buflist.format.hotlist)"),
            N_("    - ${hotlist}: the raw hotlist"),
            N_("    - ${hotlist_priority}: \"none\", \"low\", \"message\", "
               "\"private\" or \"highlight\""),
            N_("    - ${hotlist_priority_number}: -1 = none, 0 = low, 1 = message, "
               "2 = private, 3 = highlight"),
            N_("    - ${format_lag}: the lag for an IRC server buffer, empty if "
               "there's no lag (evaluation of option buflist.format.lag)"),
            N_("    - ${format_tls_version}: indicator of TLS version for a server "
               "buffer, empty for channels (evaluation of option "
               "buflist.format.tls_version)")),
        "enable|disable|toggle"
        " || bar"
        " || refresh %(buflist_items_used)|%*",
        &buflist_command_buflist, NULL, NULL);
}
