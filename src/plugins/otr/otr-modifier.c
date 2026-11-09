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


/*
 * Check if a string contains an OTR query ("?OTR?" or "?OTRv...?").
 *
 * Return 1 if the string contains an OTR query, 0 otherwise.
 */

int
otr_modifier_is_otr_query (const char *s)
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

/*
 * Check if a string ends like an OTR message (with "." or ",").
 *
 * Return 1 if the string ends like an OTR message, 0 otherwise.
 */

int
otr_modifier_has_otr_end (const char *s)
{
    size_t l = strlen (s);

    return l > 0 && (s[l - 1] == '.' || s[l - 1] == ',');
}

/*
 * Add a received line to the assembly buffer of a peer (OTR messages can be
 * split over several IRC lines).
 *
 * Return 1 if a complete message is available (copied in *out, which must be
 * freed after use), 0 if more lines are expected or if an allocation error
 * occurred.
 */

int
otr_modifier_assemble (struct t_otr_peer_data *pd, const char *line, char **out)
{
    size_t l = strlen (line);
    char *nb;

    if (!pd->asm_buf && strncmp (line, "?OTR", 4) != 0)
    {
        *out = otr_util_dupstr (line);
        return 1;
    }
    if (pd->asm_len + l + 1 > OTR_MAX_ASSEMBLY)
    {
        free (pd->asm_buf);
        pd->asm_buf = NULL;
        pd->asm_len = 0;
        *out = otr_util_dupstr (line);
        return 1;
    }
    nb = realloc (pd->asm_buf, pd->asm_len + l + 1);
    if (!nb)
        return 0;
    pd->asm_buf = nb;
    memcpy (pd->asm_buf + pd->asm_len, line, l + 1);
    pd->asm_len += l;

    if (otr_modifier_is_otr_query (pd->asm_buf)
        || otr_modifier_has_otr_end (pd->asm_buf)
        || strncmp (pd->asm_buf, "?OTR", 4) != 0)
    {
        *out = pd->asm_buf;
        pd->asm_buf = NULL;
        pd->asm_len = 0;
        return 1;
    }
    return 0;
}

/*
 * Callback for modifier "irc_in_privmsg": decrypt incoming OTR private messages
 * and replace the echo of own OTR messages.
 *
 * Return new message(s), an empty string to drop the message, NULL to keep it
 * unchanged.
 */

char *
otr_modifier_in_cb (const void *pointer, void *data, const char *modifier,
                    const char *modifier_data, const char *string)
{
    struct t_hashtable *hin, *hout;
    const char *command, *nick, *host, *args, *sp, *text, *own;
    char *result = NULL, *target = NULL, *account = NULL, *peer = NULL;
    char *assembled = NULL, *newmsg = NULL, *tags = NULL, *deliver = NULL;
    const char *server = modifier_data;
    struct t_otr_peer_data *pd;
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
    if (!target || otr_irc_target_is_channel (server, target))
        goto done;

    own = weechat_info_get ("irc_nick", server);
    if (own && strcasecmp (own, nick) == 0)
    {
        /* echo-message: the server returns our own outgoing message. */
        result = otr_echo_replace (server, string, host, target, text);
        goto done;
    }

    /* CTCP: only a plaintext ACTION inside an encrypted session is flagged. */
    account = otr_irc_make_account (server);
    if (!account)
        goto done;
    peer = otr_util_lc_dup (nick);
    if (!peer)
        goto done;

    if (otr_message_is_ctcp (text))
    {
        if (otr_message_is_action (text)
            && otr_context_encrypted (otr_context_find (account, peer)))
        {
            char *plain = otr_message_plain_from_irc (text);

            if (plain)
            {
                if (string[0] == '@' && (sp = strchr (string, ' ')) != NULL)
                    tags = strndup (string, (size_t)(sp - string) + 1);
                result = otr_message_build_in_privmsgs (
                    tags ? tags : "", host, target, plain,
                    OTR_UNENCRYPTED_PREFIX, 0);
                free (plain);
            }
        }
        goto done;
    }

    pd = otr_context_peer_data_for (account, peer);
    if (!pd || !otr_modifier_assemble (pd, text, &assembled))
    {
        if (pd)
            result = strdup (""); /* Waiting for more parts */
        goto done;
    }

    was_otr = (strncmp (assembled, "?OTR", 4) == 0);
    ignore = otrl_message_receiving (otr_us, &otr_callback_ops, NULL, account,
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
        encrypted = otr_context_encrypted (
            ctx ? ctx : otr_context_find (account, peer));

        if (newmsg && was_otr && encrypted)
        {
            /* Decrypted message */
            if (otr_policy_for (account, peer, "html_filter"))
                deliver = otr_message_html_filter (newmsg);
            else
                deliver = otr_util_dupstr (newmsg);
            if (deliver)
                result = otr_message_build_in_privmsgs (tags ? tags : "", host,
                                                        target,
                                                        deliver, "", 1);
            else
                result = strdup ("");
        }
        else if (encrypted && !was_otr)
        {
            /* Plaintext although the session is encrypted */
            result = otr_message_build_in_privmsgs (tags ? tags : "", host,
                                                    target,
                                                    newmsg ? newmsg : assembled,
                                                    OTR_UNENCRYPTED_PREFIX, 0);
        }
        else if (newmsg)
        {
            /* For example: whitespace tag stripped from plaintext */
            result = otr_message_build_in_privmsgs (tags ? tags : "", host,
                                                    target,
                                                    newmsg, "", 0);
        }
        else if (strcmp (assembled, text) != 0)
        {
            result = otr_message_build_in_privmsgs (tags ? tags : "", host,
                                                    target,
                                                    assembled, "", 0);
        }
        /* Otherwise: unchanged (result == NULL) */
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

/*
 * Parse an outgoing message "PRIVMSG target :text".
 *
 * Return 1 if OK (*target must be freed after use, *text points inside the
 * message), 0 if error.
 */

int
otr_modifier_parse_out_privmsg (const char *s, char **target, const char **text)
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

/*
 * Check if all targets of a comma-separated list are channels.
 *
 * Return 1 if all targets are channels, 0 otherwise.
 */

int
otr_modifier_all_targets_channels (const char *server, const char *list)
{
    char *copy = otr_util_dupstr (list), *save = NULL, *t;
    int ok = 1;

    if (!copy)
        return 0;
    for (t = strtok_r (copy, ",", &save); t; t = strtok_r (NULL, ",", &save))
        if (!otr_irc_target_is_channel (server, t))
        {
            ok = 0;
            break;
        }
    free (copy);
    return ok;
}

/*
 * Callback for modifier "irc_out_privmsg": encrypt outgoing private messages
 * with OTR.
 *
 * Fail closed: any problem returns an empty string, which makes WeeChat drop
 * the message instead of sending it unencrypted.
 *
 * Return new message, NULL to send it unchanged (channel, or plain text allowed
 * by policies), an empty string to drop it.
 */

char *
otr_modifier_out_cb (const void *pointer, void *data, const char *modifier,
                     const char *modifier_data, const char *string)
{
    char *target = NULL, *account = NULL, *peer = NULL, *plain = NULL;
    char *newmsg = NULL, *result = NULL, *clean = NULL, *shown = NULL;
    const char *text, *server = modifier_data;
    ConnContext *ctx;
    gcry_error_t err;
    int ctcp;

    (void)pointer; (void)data; (void)modifier;

    if (otr_injecting)
        return NULL;
    if (!otr_us)
        return strdup ("");

    if (!otr_modifier_parse_out_privmsg (string, &target, &text))
    {
        otr_buffer_error_core (
            _("could not parse an outgoing message; it was not sent"));
        return strdup ("");
    }

    if (strchr (target, ','))
    {
        if (otr_modifier_all_targets_channels (server, target))
        {
            free (target);
            return NULL;
        }
        otr_buffer_error_core (_("messages to several targets including a nick "
                               "are not supported with OTR; not sent"));
        free (target);
        return strdup ("");
    }
    if (otr_irc_target_is_channel (server, target))
    {
        free (target);
        return NULL;
    }

    account = otr_irc_make_account (server);
    peer = otr_util_lc_dup (target);
    if (!account || !peer)
    {
        otr_buffer_error_core (
            _("not connected to server %s; message not sent"), server);
        result = strdup ("");
        goto done;
    }

    ctx = otr_context_find (account, peer);
    if (otr_context_encrypted (ctx))
    {
        char *conv = otr_message_plain_from_irc (text);

        shown = otr_util_dupstr (conv);
        if (conv && otr_policy_for (account, peer, "html_escape"))
        {
            plain = otr_message_html_escape (conv);
            free (conv);
        }
        else
            plain = conv;
    }
    else
        plain = otr_util_dupstr (text);
    if (!plain)
    {
        result = strdup ("");
        goto done;
    }

    ctcp = otr_message_is_ctcp (text);
    if (ctcp)
        otr_no_tag++;
    err = otrl_message_sending (otr_us, &otr_callback_ops, NULL, account,
                                OTR_PROTOCOL, peer, OTRL_INSTAG_BEST, plain,
                                NULL, &newmsg, OTRL_FRAGMENT_SEND_ALL_BUT_LAST,
                                NULL, NULL, NULL);
    if (ctcp)
        otr_no_tag--;

    if (err)
    {
        otr_buffer_peer_print (
            server, peer, OTR_CONFIG_LEVEL_ERROR,
            _("Failed to send message; it was NOT sent."));
        otr_buffer_debug ("otrl_message_sending: %s", gcry_strerror (err));
        result = strdup ("");
        goto done;
    }

    if (newmsg)
    {
        clean = otr_util_irc_sanitize (newmsg);
        if (!clean || !*clean)
            result = strdup ("");
        else
        {
            size_t l = strlen (clean);

            if (shown && strncmp (clean, "?OTR", 4) == 0)
                otr_echo_pending_add (server, peer, clean, shown);

            /* WeeChat strips trailing spaces; keep whitespace tags intact. */
            result = otr_util_strf ("PRIVMSG %s :%s%s", target, clean,
                                    clean[l - 1] == ' ' ? "\t" : "");
        }
        otrl_message_free (newmsg);
    }
    /* Otherwise: not an OTR message (plaintext allowed), result stays NULL. */

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
