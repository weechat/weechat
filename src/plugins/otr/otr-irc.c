/*
 * SPDX-FileCopyrightText: 2026 Nils Görs <weechatter@arcor.de>
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* IRC functions for OTR plugin */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "otr.h"
#include "otr-buffer.h"
#include "otr-irc.h"
#include "otr-util.h"


/*
 * Search the IRC buffer for a server and a nick.
 *
 * Return pointer to buffer, NULL if not found.
 */

struct t_gui_buffer *
otr_irc_buffer (const char *server, const char *nick)
{
    char *info_args, *info;
    void *ptr_buffer;

    weechat_asprintf (&info_args, "%s,%s", server, nick);
    if (!info_args)
        return NULL;

    info = weechat_info_get ("irc_buffer", info_args);
    free (info_args);

    ptr_buffer = NULL;
    if (info && info[0] && (sscanf (info, "%p", &ptr_buffer) != 1))
        ptr_buffer = NULL;

    free (info);

    return (struct t_gui_buffer *)ptr_buffer;
}

/*
 * Check if a message target is an IRC channel, including a channel with a
 * status message prefix (for example "@#channel").
 *
 * Return 1 if the target is a channel, 0 otherwise.
 */

int
otr_irc_target_is_channel (const char *server, const char *target)
{
    char *info_args, *info, *statusmsg;
    int is_channel;

    if (!target || !target[0])
        return 0;

    is_channel = 0;

    weechat_asprintf (&info_args, "%s,%s", server, target);
    if (info_args)
    {
        info = weechat_info_get ("irc_is_channel", info_args);
        is_channel = (info && (strcmp (info, "1") == 0)) ? 1 : 0;
        free (info);
        free (info_args);
    }

    if (!is_channel && target[1])
    {
        /* Status message prefix, for example "@#channel" */
        statusmsg = NULL;
        weechat_asprintf (&info_args, "%s,STATUSMSG", server);
        if (info_args)
        {
            statusmsg = weechat_info_get ("irc_server_isupport_value",
                                          info_args);
            free (info_args);
        }
        if (statusmsg && statusmsg[0] && strchr (statusmsg, target[0]))
        {
            weechat_asprintf (&info_args, "%s,%s", server, target + 1);
            if (info_args)
            {
                info = weechat_info_get ("irc_is_channel", info_args);
                is_channel = (info && (strcmp (info, "1") == 0)) ? 1 : 0;
                free (info);
                free (info_args);
            }
        }
        free (statusmsg);
    }

    return is_channel;
}

/*
 * Build the OTR account name for a server: "ownnick@server" (nick in lower
 * case).
 *
 * Return NULL if not connected to the server.
 *
 * Note: result must be freed after use.
 */

char *
otr_irc_make_account (const char *server)
{
    char *info_nick, *nick, *account;

    account = NULL;

    info_nick = weechat_info_get ("irc_nick", server);
    if (!info_nick || !info_nick[0])
        goto end;

    nick = weechat_string_tolower (info_nick);
    if (!nick)
        goto end;

    weechat_asprintf (&account, "%s@%s", nick, server);

    free (nick);

end:
    free (info_nick);
    return account;
}

/*
 * Get the server part of an OTR account name ("nick@server").
 *
 * Return pointer to the server in the account name, an empty string if there is
 * no server.
 */

const char *
otr_irc_account_server (const char *account)
{
    const char *ptr_at;

    ptr_at = strchr (account, '@');

    return (ptr_at) ? ptr_at + 1 : "";
}

/*
 * Copy own nick part of an OTR account name ("nick@server") into nick, a buffer
 * of size bytes (the nick is truncated if needed).
 */

void
otr_irc_account_nick (const char *account, char *nick, size_t size)
{
    const char *ptr_at;
    size_t length;

    ptr_at = strchr (account, '@');
    length = (ptr_at) ? (size_t)(ptr_at - account) : strlen (account);
    if (length >= size)
        length = size - 1;

    memcpy (nick, account, length);
    nick[length] = '\0';
}

/*
 * Send a PRIVMSG to a nick on a server with command /quote; the outgoing
 * modifier lets it through untouched.
 */

void
otr_irc_send_raw (const char *server, const char *nick, const char *text)
{
    char *server2, *nick2, *text2, *command;

    command = NULL;

    server2 = otr_util_irc_sanitize (server);
    nick2 = otr_util_irc_sanitize (nick);
    text2 = otr_util_irc_sanitize (text);
    if (!server2 || !nick2 || !text2)
        goto end;

    weechat_asprintf (&command, "/quote -server %s PRIVMSG %s :%s",
                      server2, nick2, text2);
    if (!command)
        goto end;

    otr_buffer_debug ("inject to %s on %s (len %d)",
                      nick, server, (int)strlen (text));
    otr_injecting++;
    weechat_command (NULL, command);
    otr_injecting--;

end:
    free (command);
    free (server2);
    free (nick2);
    free (text2);
}
