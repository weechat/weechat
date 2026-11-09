/*
 * SPDX-FileCopyrightText: 2026 Nils Görs <weechatter@arcor.de>
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Modifiers for incoming and outgoing IRC private messages */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "../weechat-plugin.h"
#include "otr.h"
#include "otr-buffer.h"
#include "otr-callback.h"
#include "otr-config.h"
#include "otr-context.h"
#include "otr-echo.h"
#include "otr-irc.h"
#include "otr-message.h"
#include "otr-modifier.h"
#include "otr-policy.h"
#include "otr-util.h"


static int
is_otr_query (const char *s)
{
    const char *p = s, *q;

    while ((p = strstr (p, "?OTR")) != NULL)
    {
        q = p + 4;
        if (*q == '?')
            return 1;
        if (*q == 'v')
        {
            q++;
            while (isalnum ((unsigned char)*q))
                q++;
            if (*q == '?')
                return 1;
        }
        p += 4;
    }
    return 0;
}

static int
has_otr_end (const char *s)
{
    size_t l = strlen (s);

    return l > 0 && (s[l - 1] == '.' || s[l - 1] == ',');
}

/* Add the line to the peer's assembly buffer; return 1 if a complete
 * message is now available (copied to *out, malloc'd). */
static int
assemble (struct peer_data *pd, const char *line, char **out)
{
    size_t l = strlen (line);
    char *nb;

    if (!pd->asm_buf && strncmp (line, "?OTR", 4) != 0)
    {
        *out = dupstr (line);
        return 1;
    }
    if (pd->asm_len + l + 1 > OTR_MAX_ASSEMBLY)
    {
        free (pd->asm_buf);
        pd->asm_buf = NULL;
        pd->asm_len = 0;
        *out = dupstr (line);
        return 1;
    }
    nb = realloc (pd->asm_buf, pd->asm_len + l + 1);
    if (!nb)
        return 0;
    pd->asm_buf = nb;
    memcpy (pd->asm_buf + pd->asm_len, line, l + 1);
    pd->asm_len += l;

    if (is_otr_query (pd->asm_buf) || has_otr_end (pd->asm_buf)
        || strncmp (pd->asm_buf, "?OTR", 4) != 0)
    {
        *out = pd->asm_buf;
        pd->asm_buf = NULL;
        pd->asm_len = 0;
        return 1;
    }
    return 0;
}

char *
modifier_in_cb (const void *pointer, void *data, const char *modifier,
                const char *modifier_data, const char *string)
{
    struct t_hashtable *hin, *hout;
    const char *command, *nick, *host, *args, *sp, *text, *own;
    char *result = NULL, *target = NULL, *account = NULL, *peer = NULL;
    char *assembled = NULL, *newmsg = NULL, *tags = NULL, *deliver = NULL;
    const char *server = modifier_data;
    struct peer_data *pd;
    OtrlTLV *tlvs = NULL;
    ConnContext *ctx = NULL;
    int ignore, was_otr, encrypted;

    (void)pointer; (void)data; (void)modifier;

    if (!otr_us)
        return NULL;
    hin = weechat_hashtable_new (8, WEECHAT_HASHTABLE_STRING,
                                 WEECHAT_HASHTABLE_STRING, NULL, NULL);
    if (!hin)
        return NULL;
    weechat_hashtable_set (hin, "message", string);
    hout = weechat_info_get_hashtable ("irc_message_parse", hin);
    weechat_hashtable_free (hin);
    if (!hout)
        return NULL;

    command = weechat_hashtable_get (hout, "command");
    nick = weechat_hashtable_get (hout, "nick");
    host = weechat_hashtable_get (hout, "host");
    args = weechat_hashtable_get (hout, "arguments");
    if (!command || strcasecmp (command, "PRIVMSG") != 0 || !nick || !*nick
        || !host || !*host || !args)
        goto done;

    sp = strchr (args, ' ');
    if (!sp)
        goto done;
    target = strndup (args, (size_t)(sp - args));
    text = sp + 1;
    if (*text == ':')
        text++;
    if (!target || target_is_channel (server, target))
        goto done;

    own = weechat_info_get ("irc_nick", server);
    if (own && strcasecmp (own, nick) == 0)
    {
        /* echo-message: the server returns our own outgoing message */
        result = echo_replace (server, string, host, target, text);
        goto done;
    }

    /* CTCP: only a plaintext ACTION inside an encrypted session is flagged */
    account = make_account (server);
    if (!account)
        goto done;
    peer = lc_dup (nick);
    if (!peer)
        goto done;

    if (is_ctcp (text))
    {
        if (is_action (text) && ctx_encrypted (find_ctx (account, peer)))
        {
            char *plain = msg_plain_from_irc (text);

            if (plain)
            {
                if (string[0] == '@' && (sp = strchr (string, ' ')) != NULL)
                    tags = strndup (string, (size_t)(sp - string) + 1);
                result = build_in_privmsgs (tags ? tags : "", host, target,
                                            plain, UNENCRYPTED_PFX, 0);
                free (plain);
            }
        }
        goto done;
    }

    pd = peer_data_for (account, peer);
    if (!pd || !assemble (pd, text, &assembled))
    {
        if (pd)
            result = strdup (""); /* waiting for more parts */
        goto done;
    }

    was_otr = (strncmp (assembled, "?OTR", 4) == 0);
    ignore = otrl_message_receiving (otr_us, &otr_ops, NULL, account,
                                     OTR_PROTOCOL, peer, assembled, &newmsg,
                                     &tlvs, &ctx, NULL, NULL);
    if (tlvs)
        otrl_tlv_free (tlvs);

    if (string[0] == '@' && (sp = strchr (string, ' ')) != NULL)
        tags = strndup (string, (size_t)(sp - string) + 1);

    if (ignore)
    {
        result = strdup ("");
    }
    else
    {
        encrypted = ctx_encrypted (ctx ? ctx : find_ctx (account, peer));

        if (newmsg && was_otr && encrypted)
        {
            /* decrypted message */
            if (policy_for (account, peer, "html_filter"))
                deliver = html_filter (newmsg);
            else
                deliver = dupstr (newmsg);
            if (deliver)
                result = build_in_privmsgs (tags ? tags : "", host, target,
                                            deliver, "", 1);
            else
                result = strdup ("");
        }
        else if (encrypted && !was_otr)
        {
            /* plaintext although the session is encrypted */
            result = build_in_privmsgs (tags ? tags : "", host, target,
                                        newmsg ? newmsg : assembled,
                                        UNENCRYPTED_PFX, 0);
        }
        else if (newmsg)
        {
            /* e.g. whitespace tag stripped from plaintext */
            result = build_in_privmsgs (tags ? tags : "", host, target,
                                        newmsg, "", 0);
        }
        else if (strcmp (assembled, text) != 0)
        {
            result = build_in_privmsgs (tags ? tags : "", host, target,
                                        assembled, "", 0);
        }
        /* otherwise: unchanged (result == NULL) */
    }
    if (newmsg)
        otrl_message_free (newmsg);
    weechat_bar_item_update (OTR_BAR_ITEM);

done:
    free (deliver);
    free (tags);
    free (assembled);
    free (target);
    free (account);
    free (peer);
    weechat_hashtable_free (hout);
    return result;
}

/* "PRIVMSG target :text" */
static int
parse_out_privmsg (const char *s, char **target, const char **text)
{
    const char *sp;

    while (*s == ' ')
        s++;
    if (strncasecmp (s, "PRIVMSG ", 8) != 0)
        return 0;
    s += 8;
    while (*s == ' ')
        s++;
    sp = strchr (s, ' ');
    if (!sp)
        return 0;
    *target = strndup (s, (size_t)(sp - s));
    if (!*target)
        return 0;
    sp++;
    if (*sp == ':')
        sp++;
    *text = sp;
    return 1;
}

/* 1 = all comma separated targets are channels */
static int
all_targets_channels (const char *server, const char *list)
{
    char *copy = dupstr (list), *save = NULL, *t;
    int ok = 1;

    if (!copy)
        return 0;
    for (t = strtok_r (copy, ",", &save); t; t = strtok_r (NULL, ",", &save))
        if (!target_is_channel (server, t))
        {
            ok = 0;
            break;
        }
    free (copy);
    return ok;
}

/*
 * Fails closed: any problem returns an empty string, which makes WeeChat
 * drop the message instead of sending it unencrypted.
 */
char *
modifier_out_cb (const void *pointer, void *data, const char *modifier,
                 const char *modifier_data, const char *string)
{
    char *target = NULL, *account = NULL, *peer = NULL, *plain = NULL;
    char *newmsg = NULL, *result = NULL, *clean = NULL, *shown = NULL;
    const char *text, *server = modifier_data;
    ConnContext *ctx;
    gcry_error_t err;
    int ctcp;

    (void)pointer; (void)data; (void)modifier;

    if (g_injecting)
        return NULL;
    if (!otr_us)
        return strdup ("");

    if (!parse_out_privmsg (string, &target, &text))
    {
        otr_error_core (_("could not parse an outgoing message; it was not "
                        "sent"));
        return strdup ("");
    }

    if (strchr (target, ','))
    {
        if (all_targets_channels (server, target))
        {
            free (target);
            return NULL;
        }
        otr_error_core (_("messages to several targets including a nick are "
                        "not supported with OTR; not sent"));
        free (target);
        return strdup ("");
    }
    if (target_is_channel (server, target))
    {
        free (target);
        return NULL;
    }

    account = make_account (server);
    peer = lc_dup (target);
    if (!account || !peer)
    {
        otr_error_core (_("not connected to server %s; message not sent"),
                        server);
        result = strdup ("");
        goto done;
    }

    ctx = find_ctx (account, peer);
    if (ctx_encrypted (ctx))
    {
        char *conv = msg_plain_from_irc (text);

        shown = dupstr (conv);
        if (conv && policy_for (account, peer, "html_escape"))
        {
            plain = html_escape (conv);
            free (conv);
        }
        else
            plain = conv;
    }
    else
        plain = dupstr (text);
    if (!plain)
    {
        result = strdup ("");
        goto done;
    }

    ctcp = is_ctcp (text);
    if (ctcp)
        g_no_tag++;
    err = otrl_message_sending (otr_us, &otr_ops, NULL, account,
                                OTR_PROTOCOL, peer, OTRL_INSTAG_BEST, plain,
                                NULL, &newmsg, OTRL_FRAGMENT_SEND_ALL_BUT_LAST,
                                NULL, NULL, NULL);
    if (ctcp)
        g_no_tag--;

    if (err)
    {
        peer_print (server, peer, LVL_ERROR, _("Failed to send message; it was "
                    "NOT sent."));
        otr_debug ("otrl_message_sending: %s", gcry_strerror (err));
        result = strdup ("");
        goto done;
    }

    if (newmsg)
    {
        clean = irc_sanitize (newmsg);
        if (!clean || !*clean)
            result = strdup ("");
        else
        {
            size_t l = strlen (clean);

            if (shown && strncmp (clean, "?OTR", 4) == 0)
                pending_add (server, peer, clean, shown);

            /* WeeChat strips trailing spaces; keep whitespace tags intact */
            result = strf ("PRIVMSG %s :%s%s", target, clean,
                           clean[l - 1] == ' ' ? "\t" : "");
        }
        otrl_message_free (newmsg);
    }
    /* else: not an OTR message (plaintext allowed): result stays NULL */

    weechat_bar_item_update (OTR_BAR_ITEM);

done:
    free (shown);
    free (clean);
    free (plain);
    free (target);
    free (account);
    free (peer);
    return result;
}
