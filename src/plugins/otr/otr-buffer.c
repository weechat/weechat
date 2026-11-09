/*
 * SPDX-FileCopyrightText: 2026 Nils Görs <weechatter@arcor.de>
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Display and buffer functions for OTR plugin */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "otr.h"
#include "otr-buffer.h"
#include "otr-callback.h"
#include "otr-command.h"
#include "otr-config.h"
#include "otr-context.h"
#include "otr-irc.h"
#include "otr-util.h"


void
otr_debug (const char *fmt, ...)
{
    struct t_gui_buffer *b;
    va_list ap;
    char tmp[1024];

    if (!opt_debug || !weechat_config_boolean (opt_debug))
        return;
    b = weechat_buffer_search ("otr", "OTR Debug");
    if (!b)
    {
        b = weechat_buffer_new ("OTR Debug", NULL, NULL, NULL, NULL, NULL,
                                NULL);
        if (!b)
            return;
        weechat_buffer_set (b, "title", "OTR Debug");
        weechat_buffer_set (b, "localvar_set_no_log", "1");
    }
    va_start (ap, fmt);
    vsnprintf (tmp, sizeof (tmp), fmt, ap);
    va_end (ap);
    weechat_printf (b, "otr debug\t%s", tmp);
}

void
otr_print_str (struct t_gui_buffer *buf, int col, const char *msg)
{
    char *prefix, *copy, *line, *nl;
    char colorbuf[64];
    const char *c;

    prefix = weechat_string_eval_expression (
        weechat_config_string (opt_look[LOOK_PREFIX]), NULL, NULL, NULL);
    c = weechat_color (weechat_config_color (opt_color[col]));
    snprintf (colorbuf, sizeof (colorbuf), "%s", c ? c : "");
    copy = dupstr (msg);
    if (!copy)
    {
        free (prefix);
        return;
    }
    line = copy;
    while (line)
    {
        nl = strchr (line, '\n');
        if (nl)
            *nl = '\0';
        weechat_printf (buf, "%s\t%s%s", prefix ? prefix : "otr", colorbuf,
                        line);
        line = nl ? nl + 1 : NULL;
    }
    free (copy);
    free (prefix);
}

void
otr_error_core (const char *fmt, ...)
{
    char tmp[1024];
    va_list ap;

    va_start (ap, fmt);
    vsnprintf (tmp, sizeof (tmp), fmt, ap);
    va_end (ap);
    weechat_printf (NULL, "%sotr: %s", weechat_prefix ("error"), tmp);
}

int
buf_is_private (struct t_gui_buffer *buf)
{
    const char *t;

    if (!buf)
        return 0;
    t = weechat_buffer_get_string (buf, "localvar_type");
    return t && strcmp (t, "private") == 0;
}

void
peer_print (const char *server, const char *nick, int col, const char *msg)
{
    struct t_gui_buffer *buf = irc_buffer (server, nick);
    char *tmp = NULL;

    if (g_shutdown)
        return;

    if (nick && *nick && !buf_is_private (buf))
    {
        tmp = strf ("[%s] %s", nick, msg);
        if (tmp)
            msg = tmp;
    }
    otr_print_str (buf, col, msg);
    free (tmp);
}

void
ctx_print (ConnContext *c, int col, const char *fmt, ...)
{
    va_list ap, ap2;
    int n;
    char *s;

    va_start (ap, fmt);
    va_copy (ap2, ap);
    n = vsnprintf (NULL, 0, fmt, ap);
    va_end (ap);
    if (n < 0)
    {
        va_end (ap2);
        return;
    }
    s = malloc ((size_t)n + 1);
    if (s)
    {
        vsnprintf (s, (size_t)n + 1, fmt, ap2);
        peer_print (account_server (c->accountname), c->username, col, s);
        free (s);
    }
    va_end (ap2);
}

void
ctx_hint (ConnContext *c, const char *msg)
{
    if (weechat_config_boolean (opt_hints))
        ctx_print (c, LVL_HINT, "%s", msg);
}

void
peer_hint (const char *server, const char *nick, const char *msg)
{
    if (weechat_config_boolean (opt_hints))
        peer_print (server, nick, LVL_HINT, msg);
}

int
buffer_switch_cb (const void *pointer, void *data, const char *signal,
                  const char *type_data, void *signal_data)
{
    (void)pointer; (void)data; (void)signal; (void)type_data;
    (void)signal_data;
    weechat_bar_item_update (OTR_BAR_ITEM);
    return WEECHAT_RC_OK;
}

/* end the OTR session when its private buffer is closed */
int
buffer_closing_cb (const void *pointer, void *data, const char *signal,
                   const char *type_data, void *signal_data)
{
    struct t_gui_buffer *buf = signal_data;
    struct peer_ref p;
    char *account, *peer;

    (void)pointer; (void)data; (void)signal; (void)type_data;
    if (peer_args (buf, NULL, 0, &p) && resolve (&p, &account, &peer))
    {
        ConnContext *ctx = find_ctx (account, peer);

        if (ctx && ctx->msgstate != OTRL_MSGSTATE_PLAINTEXT)
            otrl_message_disconnect_all_instances (otr_us, &otr_ops, NULL,
                                                   account, OTR_PROTOCOL,
                                                   peer);
        free (account);
        free (peer);
    }
    return WEECHAT_RC_OK;
}
