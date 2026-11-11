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
#include "otr-context.h"
#include "otr-irc.h"
#include "otr-policy.h"
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
 * Convert a nick to lower case, according to the casemapping of the server,
 * like the IRC plugin does to compare nicks (with the default casemapping
 * "rfc1459", chars "[\]^" are also converted to "{|}~").
 *
 * Note: result must be freed after use.
 */

char *
otr_irc_nick_tolower (const char *server, const char *nick)
{
    char *info_args, *result;

    if (!nick)
        return NULL;

    weechat_asprintf (&info_args, "%s,%s", (server) ? server : "", nick);
    if (!info_args)
        return NULL;

    result = weechat_info_get ("irc_nick_tolower", info_args);

    free (info_args);

    return result;
}

/*
 * Build the OTR account name for a server: "ownnick@server" (nick in lower
 * case, see function otr_irc_nick_tolower).
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

    nick = otr_irc_nick_tolower (server, info_nick);
    if (!nick)
        goto end;

    weechat_asprintf (&account, "%s@%s", nick, server);

    free (nick);

end:
    free (info_nick);
    return account;
}

/*
 * Get the OTR account name to use with a peer (nick in lower case) on a
 * server: if an OTR session is active with this peer, the account of this
 * session is returned (so that the session continues after a change of our
 * own nick), otherwise the account built with the current nick
 * ("ownnick@server").
 *
 * Return NULL if there is no active session and if not connected to the
 * server.
 *
 * Note: result must be freed after use.
 */

char *
otr_irc_peer_account (const char *server, const char *peer)
{
    ConnContext *ptr_context;

    if (otr_us)
    {
        for (ptr_context = otr_us->context_root; ptr_context;
             ptr_context = ptr_context->next)
        {
            if ((ptr_context->msgstate != OTRL_MSGSTATE_PLAINTEXT)
                && ptr_context->username && ptr_context->accountname
                && ptr_context->protocol
                && (strcmp (ptr_context->protocol, OTR_PROTOCOL) == 0)
                && (strcmp (ptr_context->username, peer) == 0)
                && (strcmp (otr_irc_account_server (ptr_context->accountname),
                            server) == 0))
            {
                return strdup (ptr_context->accountname);
            }
        }
    }

    return otr_irc_make_account (server);
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

/*
 * Callback for signal "xxx,irc_in2_nick": when our own nick changes, send an
 * OTR query to each peer with an encrypted OTR session on this server, so that
 * the peer restarts the session with our new nick (our side of the session is
 * kept, see function otr_irc_peer_account).
 *
 * The account of the session is the one of our nick when the session started,
 * which can be different from our previous nick (if our nick changed several
 * times during the session).
 */

int
otr_irc_nick_cb (const void *pointer, void *data, const char *signal,
                 const char *type_data, void *signal_data)
{
    struct t_hashtable *hashtable_in, *hashtable_out;
    ConnContext *ptr_context;
    const char *ptr_old_nick, *ptr_new_nick, *ptr_query;
    char *server, *info_nick, *account;
    const char *ptr_comma;

    /* Make C compiler happy. */
    (void) pointer;
    (void) data;
    (void) type_data;

    if (!otr_us || !signal_data)
        return WEECHAT_RC_OK;

    ptr_comma = strchr (signal, ',');
    if (!ptr_comma)
        return WEECHAT_RC_OK;
    server = weechat_strndup (signal, ptr_comma - signal);
    if (!server)
        return WEECHAT_RC_OK;

    hashtable_out = NULL;
    info_nick = NULL;

    hashtable_in = weechat_hashtable_new (8,
                                          WEECHAT_HASHTABLE_STRING,
                                          WEECHAT_HASHTABLE_STRING,
                                          NULL, NULL);
    if (!hashtable_in)
        goto end;
    weechat_hashtable_set (hashtable_in, "server", server);
    weechat_hashtable_set (hashtable_in, "message", (const char *)signal_data);
    hashtable_out = weechat_info_get_hashtable ("irc_message_parse",
                                                hashtable_in);
    weechat_hashtable_free (hashtable_in);
    if (!hashtable_out)
        goto end;

    ptr_old_nick = weechat_hashtable_get (hashtable_out, "nick");
    ptr_new_nick = weechat_hashtable_get (hashtable_out, "param1");
    if (!ptr_old_nick || !ptr_old_nick[0] || !ptr_new_nick || !ptr_new_nick[0])
        goto end;

    /* Only our own nick change is handled. */
    info_nick = weechat_info_get ("irc_nick", server);
    if (!info_nick || (weechat_strcasecmp (info_nick, ptr_new_nick) != 0))
        goto end;

    for (ptr_context = otr_us->context_root; ptr_context;
         ptr_context = ptr_context->next)
    {
        if ((ptr_context->their_instance != OTRL_INSTAG_MASTER)
            || !ptr_context->accountname || !ptr_context->username
            || (strcmp (otr_irc_account_server (ptr_context->accountname),
                        server) != 0))
        {
            continue;
        }
        /* Only the account used with the peer (one query per peer). */
        account = otr_irc_peer_account (server, ptr_context->username);
        if (!account || (strcmp (account, ptr_context->accountname) != 0)
            || !otr_context_encrypted (
                otr_context_find (account, ptr_context->username)))
        {
            free (account);
            continue;
        }
        ptr_query = otr_policy_query (account, ptr_context->username);
        free (account);
        if (!ptr_query)
            continue;
        otr_buffer_peer_hint (
            server, ptr_context->username,
            _("Sending OTR query... Please await confirmation of the OTR "
              "session being started before sending a message."));
        otr_irc_send_raw (server, ptr_context->username, ptr_query);
    }

end:
    free (server);
    free (info_nick);
    weechat_hashtable_free (hashtable_out);
    return WEECHAT_RC_OK;
}
