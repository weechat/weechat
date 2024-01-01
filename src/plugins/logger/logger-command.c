/*
 * logger-command.c - logger commands
 *
 * Copyright (C) 2003-2024 Sébastien Helleu <flashcode@flashtux.org>
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
#include <stdio.h>

#include "../weechat-plugin.h"
#include "logger.h"
#include "logger-buffer.h"
#include "logger-config.h"


/*
 * Displays logging status for buffers.
 */

void
logger_list ()
{
    struct t_infolist *ptr_infolist;
    struct t_logger_buffer *ptr_logger_buffer;
    struct t_gui_buffer *ptr_buffer;
    char status[128];

    weechat_printf (NULL, "");
    weechat_printf (NULL, _("Logging on buffers:"));

    ptr_infolist = weechat_infolist_get ("buffer", NULL, NULL);
    if (ptr_infolist)
    {
        while (weechat_infolist_next (ptr_infolist))
        {
            ptr_buffer = weechat_infolist_pointer (ptr_infolist, "pointer");
            if (ptr_buffer)
            {
                ptr_logger_buffer = logger_buffer_search_buffer (ptr_buffer);
                if (ptr_logger_buffer)
                {
                    snprintf (status, sizeof (status),
                              _("logging (level: %d)"),
                              ptr_logger_buffer->log_level);
                }
                else
                {
                    snprintf (status, sizeof (status), "%s", _("not logging"));
                }
                weechat_printf (NULL,
                                "  %s[%s%d%s]%s (%s) %s%s%s: %s%s%s%s",
                                weechat_color ("chat_delimiters"),
                                weechat_color ("chat"),
                                weechat_infolist_integer (ptr_infolist, "number"),
                                weechat_color ("chat_delimiters"),
                                weechat_color ("chat"),
                                weechat_infolist_string (ptr_infolist, "plugin_name"),
                                weechat_color ("chat_buffer"),
                                weechat_infolist_string (ptr_infolist, "name"),
                                weechat_color ("chat"),
                                status,
                                (ptr_logger_buffer) ? " (" : "",
                                (ptr_logger_buffer) ?
                                ((ptr_logger_buffer->log_filename) ?
                                 ptr_logger_buffer->log_filename : _("log not started")) : "",
                                (ptr_logger_buffer) ? ")" : "");
            }
        }
        weechat_infolist_free (ptr_infolist);
    }
}

/*
 * Enables/disables logging on a buffer.
 */

void
logger_set_buffer (struct t_gui_buffer *buffer, const char *value)
{
    char *name;
    struct t_config_option *ptr_option;

    name = logger_build_option_name (buffer);
    if (!name)
        return;

    if (logger_config_set_level (name, value) != WEECHAT_CONFIG_OPTION_SET_ERROR)
    {
        ptr_option = logger_config_get_level (name);
        if (ptr_option)
        {
            weechat_printf (NULL, _("%s: \"%s\" => level %d"),
                            LOGGER_PLUGIN_NAME, name,
                            weechat_config_integer (ptr_option));
        }
    }

    free (name);
}

/*
 * Callback for command "/logger".
 */

int
logger_command_cb (const void *pointer, void *data,
                   struct t_gui_buffer *buffer,
                   int argc, char **argv, char **argv_eol)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) argv_eol;

    if ((argc == 1)
        || ((argc == 2) && (weechat_strcmp (argv[1], "list") == 0)))
    {
        logger_list ();
        return WEECHAT_RC_OK;
    }

    if (weechat_strcmp (argv[1], "set") == 0)
    {
        if (argc > 2)
            logger_set_buffer (buffer, argv[2]);
        return WEECHAT_RC_OK;
    }

    if (weechat_strcmp (argv[1], "flush") == 0)
    {
        logger_buffer_flush ();
        return WEECHAT_RC_OK;
    }

    if (weechat_strcmp (argv[1], "disable") == 0)
    {
        logger_set_buffer (buffer, "0");
        return WEECHAT_RC_OK;
    }

    WEECHAT_COMMAND_ERROR;
}

/*
 * Hooks logger commands.
 */

void
logger_command_init ()
{
    weechat_hook_command (
        "logger",
        N_("logger plugin configuration"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("list"
           " || set <level>"
           " || flush"
           " || disable"),
        WEECHAT_CMD_ARGS_DESC(
            N_("raw[list]: show logging status for opened buffers"),
            N_("raw[set]: set logging level on current buffer"),
            N_("level: level for messages to be logged (0 = logging disabled, "
               "1 = a few messages (most important) .. 9 = all messages)"),
            N_("raw[flush]: write all log files now"),
            N_("raw[disable]: disable logging on current buffer (set level to 0)"),
            "",
            N_("Options \"logger.level.*\" and \"logger.mask.*\" can be used to set "
               "level or mask for a buffer, or buffers beginning with name."),
            "",
            N_("Log levels used by IRC plugin:"),
            N_("  1: user message (channel and private), "
               "notice (server and channel)"),
            N_("  2: nick change"),
            N_("  3: server message"),
            N_("  4: join/part/quit"),
            N_("  9: all other messages"),
            "",
            N_("Examples:"),
            N_("  set level to 5 for current buffer:"),
            AI("    /logger set 5"),
            N_("  disable logging for current buffer:"),
            AI("    /logger disable"),
            N_("  set level to 3 for all IRC buffers:"),
            AI("    /set logger.level.irc 3"),
            N_("  disable logging for main WeeChat buffer:"),
            AI("    /set logger.level.core.weechat 0"),
            N_("  use a directory per IRC server and a file per channel inside:"),
            AI("    /set logger.mask.irc \"$server/$channel.weechatlog\"")),
        "list"
        " || set 1|2|3|4|5|6|7|8|9"
        " || flush"
        " || disable",
        &logger_command_cb, NULL, NULL);
}
