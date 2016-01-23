/*
 * xfer-command.c - xfer command
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
#include <string.h>
#include <time.h>

#include "../weechat-plugin.h"
#include "xfer.h"
#include "xfer-buffer.h"
#include "xfer-chat.h"
#include "xfer-command.h"
#include "xfer-config.h"


/*
 * Callback for command "/me": sends a ctcp action to remote host.
 */

int
xfer_command_me (const void *pointer, void *data,
                 struct t_gui_buffer *buffer, int argc,
                 char **argv, char **argv_eol)
{
    struct t_xfer *ptr_xfer;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) argc;
    (void) argv;

    ptr_xfer = xfer_search_by_buffer (buffer);

    if (!ptr_xfer)
    {
        weechat_printf (NULL,
                        _("%s%s: can't find xfer for buffer \"%s\""),
                        weechat_prefix ("error"), XFER_PLUGIN_NAME,
                        weechat_buffer_get_string (buffer, "name"));
        return WEECHAT_RC_ERROR;
    }

    if (!XFER_HAS_ENDED(ptr_xfer->status))
    {
        xfer_chat_sendf (ptr_xfer, "\001ACTION %s\001\r\n",
                         (argv_eol[1]) ? argv_eol[1] : "");
        weechat_printf_date_tags (buffer,
                                  0,
                                  "no_highlight",
                                  "%s%s%s %s%s",
                                  weechat_prefix ("action"),
                                  weechat_color ("chat_nick_self"),
                                  ptr_xfer->local_nick,
                                  weechat_color ("chat"),
                                  (argv_eol[1]) ? argv_eol[1] : "");
    }

    return WEECHAT_RC_OK;
}

/*
 * Displays a list of xfer.
 */

void
xfer_command_xfer_list (int full)
{
    struct t_xfer *ptr_xfer;
    int i;
    char date[128];
    unsigned long long pct_complete;
    struct tm *date_tmp;

    if (xfer_list)
    {
        weechat_printf (NULL, "");
        weechat_printf (NULL, _("Xfer list:"));
        i = 1;
        for (ptr_xfer = xfer_list; ptr_xfer; ptr_xfer = ptr_xfer->next_xfer)
        {
            /* xfer info */
            if (XFER_IS_FILE(ptr_xfer->type))
            {
                if (ptr_xfer->size == 0)
                {
                    if (ptr_xfer->status == XFER_STATUS_DONE)
                        pct_complete = 100;
                    else
                        pct_complete = 0;
                }
                else
                    pct_complete = (unsigned long long)(((float)(ptr_xfer->pos)/(float)(ptr_xfer->size)) * 100);

                weechat_printf (NULL,
                                _("%3d. %s (%s), file: \"%s\" (local: "
                                  "\"%s\"), %s %s, status: %s%s%s "
                                  "(%llu %%)"),
                                i,
                                xfer_type_string[ptr_xfer->type],
                                xfer_protocol_string[ptr_xfer->protocol],
                                ptr_xfer->filename,
                                ptr_xfer->local_filename,
                                (XFER_IS_SEND(ptr_xfer->type)) ?
                                _("sent to") : _("received from"),
                                ptr_xfer->remote_nick,
                                weechat_color (
                                    weechat_config_string (
                                        xfer_config_color_status[ptr_xfer->status])),
                                _(xfer_status_string[ptr_xfer->status]),
                                weechat_color ("chat"),
                                pct_complete);
            }
            else
            {
                date[0] = '\0';
                date_tmp = localtime (&(ptr_xfer->start_time));
                if (date_tmp)
                {
                    if (strftime (date, sizeof (date),
                                  "%a, %d %b %Y %H:%M:%S", date_tmp) == 0)
                        date[0] = '\0';
                }
                weechat_printf (NULL,
                                /* TRANSLATORS: "%s" after "started on" is a date */
                                _("%3d. %s, chat with %s (local nick: %s), "
                                  "started on %s, status: %s%s"),
                                i,
                                xfer_type_string[ptr_xfer->type],
                                ptr_xfer->remote_nick,
                                ptr_xfer->local_nick,
                                date,
                                weechat_color (
                                    weechat_config_string (
                                        xfer_config_color_status[ptr_xfer->status])),
                                _(xfer_status_string[ptr_xfer->status]));
            }

            if (full)
            {
                /* second line of xfer info */
                if (XFER_IS_FILE(ptr_xfer->type))
                {
                    weechat_printf (NULL,
                                    _("     plugin: %s (id: %s), file: %llu "
                                      "bytes (position: %llu), address: "
                                      "%s (port %d)"),
                                    ptr_xfer->plugin_name,
                                    ptr_xfer->plugin_id,
                                    ptr_xfer->size,
                                    ptr_xfer->pos,
                                    ptr_xfer->remote_address_str,
                                    ptr_xfer->port);
                    date[0] = '\0';
                    date_tmp = localtime (&(ptr_xfer->start_transfer.tv_sec));
                    if (date_tmp)
                    {
                        if (strftime (date, sizeof (date),
                                      "%a, %d %b %Y %H:%M:%S", date_tmp) == 0)
                            date[0] = '\0';
                    }
                    weechat_printf (NULL,
                                    /* TRANSLATORS: "%s" after "started on" is a date */
                                    _("     fast_send: %s, blocksize: %d, "
                                      "started on %s"),
                                    (ptr_xfer->fast_send) ? _("yes") : _("no"),
                                    ptr_xfer->blocksize,
                                    date);
                }
            }
            i++;
        }
    }
    else
        weechat_printf (NULL, _("No xfer"));
}

/*
 * Callback for command "/xfer".
 */

int
xfer_command_xfer (const void *pointer, void *data,
                   struct t_gui_buffer *buffer, int argc,
                   char **argv, char **argv_eol)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) buffer;
    (void) argv_eol;

    if ((argc > 1) && (weechat_strcmp (argv[1], "list") == 0))
    {
        xfer_command_xfer_list (0);
        return WEECHAT_RC_OK;
    }

    if ((argc > 1) && (weechat_strcmp (argv[1], "listfull") == 0))
    {
        xfer_command_xfer_list (1);
        return WEECHAT_RC_OK;
    }

    if (!xfer_buffer)
        xfer_buffer_open ();

    if (xfer_buffer)
    {
        weechat_buffer_set (xfer_buffer, "display", "1");

        if (argc > 1)
        {
            if (strcmp (argv[1], "up") == 0)
            {
                if (xfer_buffer_selected_line > 0)
                    xfer_buffer_selected_line--;
            }
            else if (strcmp (argv[1], "down") == 0)
            {
                if (xfer_buffer_selected_line < xfer_count - 1)
                    xfer_buffer_selected_line++;
            }
        }
    }

    xfer_buffer_refresh (NULL);

    return WEECHAT_RC_OK;
}

/*
 * Hooks commands.
 */

void
xfer_command_init (void)
{
    struct t_hook *ptr_hook;

    ptr_hook = weechat_hook_command (
        "me",
        N_("send a CTCP action to remote host"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") may be translated */
        N_("<message>"),
        WEECHAT_CMD_ARGS_DESC(
            N_("message: message to send")),
        NULL,
        &xfer_command_me, NULL, NULL);
    XFER_COMMAND_KEEP_SPACES;
    weechat_hook_command (
        "xfer",
        N_("xfer control"),
        "[list|listfull]",
        WEECHAT_CMD_ARGS_DESC(
            N_("raw[list]: list xfer"),
            N_("raw[listfull]: list xfer (verbose)"),
            "",
            N_("Without argument, this command opens buffer with xfer list.")),
        "list|listfull",
        &xfer_command_xfer, NULL, NULL);
}
