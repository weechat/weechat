/*
 * fifo-command.c - fifo command
 *
 * Copyright (C) 2003-2021 Sébastien Helleu <flashcode@flashtux.org>
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
#include "fifo.h"
#include "fifo-config.h"


/*
 * Callback for command "/fifo": manages FIFO pipe.
 */

int
fifo_command_fifo (const void *pointer, void *data,
                   struct t_gui_buffer *buffer, int argc,
                   char **argv, char **argv_eol)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) buffer;
    (void) argv_eol;

    if (argc == 1)
    {
        if (fifo_fd != -1)
        {
            weechat_printf (NULL,
                            _("%s: pipe is enabled (file: %s)"),
                            FIFO_PLUGIN_NAME,
                            fifo_filename);
        }
        else
        {
            weechat_printf (NULL,
                            _("%s: pipe is disabled"), FIFO_PLUGIN_NAME);
        }
        return WEECHAT_RC_OK;
    }

    /* enable pipe */
    if (weechat_strcasecmp (argv[1], "enable") == 0)
    {
        weechat_config_option_set (fifo_config_file_enabled, "on", 1);
        return WEECHAT_RC_OK;
    }

    /* disable pipe */
    if (weechat_strcasecmp (argv[1], "disable") == 0)
    {
        weechat_config_option_set (fifo_config_file_enabled, "off", 1);
        return WEECHAT_RC_OK;
    }

    /* toggle pipe */
    if (weechat_strcasecmp (argv[1], "toggle") == 0)
    {
        weechat_config_option_set (
            fifo_config_file_enabled,
            (weechat_config_boolean (fifo_config_file_enabled)) ? "off" : "on",
            1);
        return WEECHAT_RC_OK;
    }

    WEECHAT_COMMAND_ERROR;
}

/*
 * Hooks fifo command.
 */

void
fifo_command_init ()
{
    weechat_hook_command (
        "fifo",
        N_("fifo plugin configuration"),
        N_("enable|disable|toggle"),
        N_(" enable: enable FIFO pipe\n"
           "disable: disable FIFO pipe\n"
           " toggle: toggle FIFO pipe\n"
           "\n"
           "FIFO pipe is used as remote control of WeeChat: you can send "
           "commands or text to the FIFO pipe from your shell.\n"
           "By default the FIFO pipe is in ~/.weechat/weechat_fifo\n"
           "\n"
           "The expected format is one of:\n"
           "  plugin.buffer *text or command here\n"
           "  *text or command here\n"
           "\n"
           "For example to change your freenode nick:\n"
           "  echo 'irc.server.freenode */nick newnick' "
           ">~/.weechat/weechat_fifo\n"
           "\n"
           "Please read the user's guide for more info and examples.\n"
           "\n"
           "Examples:\n"
           "  /fifo toggle"),
        "enable|disable|toggle",
        &fifo_command_fifo, NULL, NULL);
}
