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


struct t_gui_buffer *
otr_irc_buffer (const char *server, const char *nick)
{
    char *args = otr_util_strf ("%s,%s", server, nick);
    const char *s;
    void *p = NULL;

    if (!args)
        return NULL;
    s = weechat_info_get ("irc_buffer", args);
    free (args);
    if (s && *s && sscanf (s, "%p", &p) == 1)
        return (struct t_gui_buffer *)p;
    return NULL;
}

int
otr_irc_target_is_channel (const char *server, const char *target)
{
    char *args;
    const char *r, *statusmsg;
    int res = 0;

    if (!target || !*target)
        return 0;
    args = otr_util_strf ("%s,%s", server, target);
    if (args)
    {
        r = weechat_info_get ("irc_is_channel", args);
        res = (r && strcmp (r, "1") == 0);
        free (args);
    }
    if (!res)
    {
        /* status message prefix, e.g. "@#channel" */
        args = otr_util_strf ("%s,STATUSMSG", server);
        if (args)
        {
            statusmsg = weechat_info_get ("irc_server_isupport_value", args);
            if (statusmsg && *statusmsg && strchr (statusmsg, target[0])
                && target[1])
            {
                free (args);
                args = otr_util_strf ("%s,%s", server, target + 1);
                if (args)
                {
                    r = weechat_info_get ("irc_is_channel", args);
                    res = (r && strcmp (r, "1") == 0);
                }
            }
            free (args);
        }
    }
    return res;
}

/* "ownnick@server" (nick lowercased); NULL if not connected */
char *
otr_irc_make_account (const char *server)
{
    const char *nick = weechat_info_get ("irc_nick", server);
    char *lc, *r;

    if (!nick || !*nick)
        return NULL;
    lc = otr_util_lc_dup (nick);
    if (!lc)
        return NULL;
    r = otr_util_strf ("%s@%s", lc, server);
    free (lc);
    return r;
}

const char *
otr_irc_account_server (const char *account)
{
    const char *at = strchr (account, '@');

    return at ? at + 1 : "";
}

/* copy own nick part of account into dst */
void
otr_irc_account_nick (const char *account, char *dst, size_t n)
{
    const char *at = strchr (account, '@');
    size_t l = at ? (size_t)(at - account) : strlen (account);

    if (l >= n)
        l = n - 1;
    memcpy (dst, account, l);
    dst[l] = '\0';
}

void
otr_irc_send_raw (const char *server, const char *nick, const char *text)
{
    char *s = otr_util_irc_sanitize (server), *n = otr_util_irc_sanitize (nick),
         *t = otr_util_irc_sanitize (text), *cmd = NULL;

    if (s && n && t)
        cmd = otr_util_strf ("/quote -server %s PRIVMSG %s :%s", s, n, t);
    if (cmd)
    {
        otr_buffer_debug ("inject to %s on %s (len %d)", nick, server,
                          (int)strlen (text));
        otr_injecting++;
        weechat_command (NULL, cmd);
        otr_injecting--;
    }
    free (cmd);
    free (s);
    free (n);
    free (t);
}
