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
otr_modifier_is_otr_query (const char *string)
{
    const char *ptr_string, *ptr_query;

    ptr_string = string;
    while ((ptr_string = strstr (ptr_string, "?OTR")) != NULL)
    {
        ptr_query = ptr_string + 4;
        if (ptr_query[0] == '?')
            return 1;
        if (ptr_query[0] == 'v')
        {
            ptr_query++;
            while (isalnum ((unsigned char)ptr_query[0]))
            {
                ptr_query++;
            }
            if (ptr_query[0] == '?')
                return 1;
        }
        ptr_string += 4;
    }

    return 0;
}

/*
 * Check if a string ends like an OTR message (with "." or ",").
 *
 * Return 1 if the string ends like an OTR message, 0 otherwise.
 */

int
otr_modifier_has_otr_end (const char *string)
{
    size_t length;

    length = strlen (string);

    return ((length > 0)
            && ((string[length - 1] == '.')
                || (string[length - 1] == ','))) ? 1 : 0;
}

/*
 * Add a received line to the assembly buffer of a peer (OTR messages can be
 * split over several IRC lines).
 *
 * Return 1 if a complete message is available (copied in *message, which must
 * be freed after use), 0 if more lines are expected or if an allocation error
 * occurred.
 */

int
otr_modifier_assemble (struct t_otr_peer_data *peer_data, const char *line,
                       char **message)
{
    char *new_assembly;
    size_t length;

    length = strlen (line);

    if (!peer_data->assembly && (strncmp (line, "?OTR", 4) != 0))
    {
        *message = otr_util_dupstr (line);
        return 1;
    }

    if (peer_data->assembly_length + length + 1 > OTR_MAX_ASSEMBLY)
    {
        free (peer_data->assembly);
        peer_data->assembly = NULL;
        peer_data->assembly_length = 0;
        *message = otr_util_dupstr (line);
        return 1;
    }

    new_assembly = realloc (peer_data->assembly,
                            peer_data->assembly_length + length + 1);
    if (!new_assembly)
        return 0;
    peer_data->assembly = new_assembly;
    memcpy (peer_data->assembly + peer_data->assembly_length, line,
            length + 1);
    peer_data->assembly_length += length;

    if (otr_modifier_is_otr_query (peer_data->assembly)
        || otr_modifier_has_otr_end (peer_data->assembly)
        || (strncmp (peer_data->assembly, "?OTR", 4) != 0))
    {
        *message = peer_data->assembly;
        peer_data->assembly = NULL;
        peer_data->assembly_length = 0;
        return 1;
    }

    return 0;
}

/*
 * Get the IRC tags of a message, with the space after the tags.
 *
 * Return NULL if the message has no tags.
 *
 * Note: result must be freed after use.
 */

char *
otr_modifier_get_tags (const char *message)
{
    const char *ptr_space;

    if (message[0] != '@')
        return NULL;

    ptr_space = strchr (message, ' ');
    if (!ptr_space)
        return NULL;

    return weechat_strndup (message, ptr_space - message + 1);
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
    struct t_hashtable *hashtable_in, *hashtable_out;
    struct t_otr_peer_data *ptr_peer_data;
    const char *ptr_command, *ptr_nick, *ptr_host, *ptr_args, *ptr_space;
    const char *ptr_text, *ptr_server;
    char *result, *target, *account, *peer, *assembled, *new_message, *tags;
    char *message, *plain, *own_nick;
    OtrlTLV *tlvs;
    ConnContext *ptr_context;
    int ignore, was_otr, encrypted;

    /* Make C compiler happy. */
    (void) pointer;
    (void) data;
    (void) modifier;

    if (!otr_us)
        return NULL;

    hashtable_in = weechat_hashtable_new (8,
                                          WEECHAT_HASHTABLE_STRING,
                                          WEECHAT_HASHTABLE_STRING,
                                          NULL, NULL);
    if (!hashtable_in)
        return NULL;
    weechat_hashtable_set (hashtable_in, "message", string);
    hashtable_out = weechat_info_get_hashtable ("irc_message_parse",
                                                hashtable_in);
    weechat_hashtable_free (hashtable_in);
    if (!hashtable_out)
        return NULL;

    ptr_server = modifier_data;
    result = NULL;
    target = NULL;
    account = NULL;
    peer = NULL;
    assembled = NULL;
    new_message = NULL;
    tags = NULL;
    message = NULL;
    own_nick = NULL;
    tlvs = NULL;
    ptr_context = NULL;

    ptr_command = weechat_hashtable_get (hashtable_out, "command");
    ptr_nick = weechat_hashtable_get (hashtable_out, "nick");
    ptr_host = weechat_hashtable_get (hashtable_out, "host");
    ptr_args = weechat_hashtable_get (hashtable_out, "arguments");
    if (!ptr_command || (weechat_strcasecmp (ptr_command, "PRIVMSG") != 0)
        || !ptr_nick || !ptr_nick[0] || !ptr_host || !ptr_host[0] || !ptr_args)
    {
        goto end;
    }

    ptr_space = strchr (ptr_args, ' ');
    if (!ptr_space)
        goto end;
    target = weechat_strndup (ptr_args, ptr_space - ptr_args);
    ptr_text = ptr_space + 1;
    if (ptr_text[0] == ':')
        ptr_text++;
    if (!target || otr_irc_target_is_channel (ptr_server, target))
        goto end;

    own_nick = weechat_info_get ("irc_nick", ptr_server);
    if (own_nick && (weechat_strcasecmp (own_nick, ptr_nick) == 0))
    {
        /* echo-message: the server returns our own outgoing message. */
        result = otr_echo_replace (ptr_server, string, ptr_host, target,
                                   ptr_text);
        goto end;
    }

    account = otr_irc_make_account (ptr_server);
    if (!account)
        goto end;
    peer = weechat_string_tolower (ptr_nick);
    if (!peer)
        goto end;

    /* CTCP: only a plaintext ACTION inside an encrypted session is flagged. */
    if (otr_message_is_ctcp (ptr_text))
    {
        if (otr_message_is_action (ptr_text)
            && otr_context_encrypted (otr_context_find (account, peer)))
        {
            plain = otr_message_plain_from_irc (ptr_text);
            if (plain)
            {
                tags = otr_modifier_get_tags (string);
                result = otr_message_build_in_privmsgs (
                    (tags) ? tags : "", ptr_host, target, plain,
                    OTR_UNENCRYPTED_PREFIX, 0);
                free (plain);
            }
        }
        goto end;
    }

    ptr_peer_data = otr_context_peer_data_for (account, peer);
    if (!ptr_peer_data)
        goto end;
    if (!otr_modifier_assemble (ptr_peer_data, ptr_text, &assembled))
    {
        /* Waiting for more parts */
        result = strdup ("");
        goto end;
    }

    was_otr = (strncmp (assembled, "?OTR", 4) == 0) ? 1 : 0;
    ignore = otrl_message_receiving (otr_us, &otr_callback_ops, NULL, account,
                                     OTR_PROTOCOL, peer, assembled,
                                     &new_message, &tlvs, &ptr_context,
                                     NULL, NULL);
    if (tlvs)
    {
        /*
         * The peer has ended the OTR session: libotr does not call the
         * callback "gone_insecure", so it is called here.
         */
        if (ptr_context && otrl_tlv_find (tlvs, OTRL_TLV_DISCONNECTED))
            otr_callback_gone_insecure (NULL, ptr_context);
        otrl_tlv_free (tlvs);
    }

    tags = otr_modifier_get_tags (string);

    if (ignore)
    {
        result = strdup ("");
    }
    else
    {
        encrypted = otr_context_encrypted (
            (ptr_context) ? ptr_context : otr_context_find (account, peer));
        if (new_message && was_otr && encrypted)
        {
            /* Decrypted message */
            if (otr_policy_for (account, peer, "html_filter"))
                message = otr_message_html_filter (new_message);
            else
                message = otr_util_dupstr (new_message);
            if (message)
            {
                result = otr_message_build_in_privmsgs (
                    (tags) ? tags : "", ptr_host, target, message, "", 1);
            }
            else
            {
                result = strdup ("");
            }
        }
        else if (encrypted && !was_otr)
        {
            /* Plaintext although the session is encrypted */
            result = otr_message_build_in_privmsgs (
                (tags) ? tags : "", ptr_host, target,
                (new_message) ? new_message : assembled,
                OTR_UNENCRYPTED_PREFIX, 0);
        }
        else if (new_message)
        {
            /* For example: whitespace tag stripped from plaintext */
            result = otr_message_build_in_privmsgs (
                (tags) ? tags : "", ptr_host, target, new_message, "", 0);
        }
        else if (strcmp (assembled, ptr_text) != 0)
        {
            result = otr_message_build_in_privmsgs (
                (tags) ? tags : "", ptr_host, target, assembled, "", 0);
        }
        /* Otherwise: unchanged (result == NULL) */
    }
    if (new_message)
        otrl_message_free (new_message);
    weechat_bar_item_update (OTR_BAR_ITEM);

end:
    free (message);
    free (own_nick);
    free (tags);
    free (assembled);
    free (target);
    free (account);
    free (peer);
    weechat_hashtable_free (hashtable_out);
    return result;
}

/*
 * Parse an outgoing message "PRIVMSG target :text".
 *
 * Return 1 if OK (*target must be freed after use, *text points inside the
 * message), 0 if error.
 */

int
otr_modifier_parse_out_privmsg (const char *message, char **target,
                                const char **text)
{
    const char *ptr_message, *ptr_space;

    ptr_message = message;
    while (ptr_message[0] == ' ')
    {
        ptr_message++;
    }
    if (weechat_strncasecmp (ptr_message, "PRIVMSG ", 8) != 0)
        return 0;
    ptr_message += 8;
    while (ptr_message[0] == ' ')
    {
        ptr_message++;
    }

    ptr_space = strchr (ptr_message, ' ');
    if (!ptr_space)
        return 0;
    *target = weechat_strndup (ptr_message, ptr_space - ptr_message);
    if (!*target)
        return 0;

    ptr_space++;
    if (ptr_space[0] == ':')
        ptr_space++;
    *text = ptr_space;

    return 1;
}

/*
 * Check if all targets of a comma-separated list are channels.
 *
 * Return 1 if all targets are channels, 0 otherwise.
 */

int
otr_modifier_all_targets_channels (const char *server, const char *targets)
{
    char **list_targets;
    int num_targets, all_channels, i;

    list_targets = weechat_string_split (targets, ",", NULL,
                                         WEECHAT_STRING_SPLIT_STRIP_LEFT
                                         | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                         | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                         0, &num_targets);
    if (!list_targets)
        return 0;

    all_channels = 1;
    for (i = 0; i < num_targets; i++)
    {
        if (!otr_irc_target_is_channel (server, list_targets[i]))
        {
            all_channels = 0;
            break;
        }
    }

    weechat_string_free_split (list_targets);

    return all_channels;
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
    char *target, *account, *peer, *plain, *new_message, *result, *clean;
    char *shown, *converted;
    const char *ptr_text, *ptr_server;
    ConnContext *ptr_context;
    gcry_error_t error;
    size_t length;
    int ctcp;

    /* Make C compiler happy. */
    (void) pointer;
    (void) data;
    (void) modifier;

    if (otr_injecting)
        return NULL;
    if (!otr_us)
        return strdup ("");

    ptr_server = modifier_data;
    target = NULL;
    account = NULL;
    peer = NULL;
    plain = NULL;
    new_message = NULL;
    result = NULL;
    clean = NULL;
    shown = NULL;

    if (!otr_modifier_parse_out_privmsg (string, &target, &ptr_text))
    {
        otr_buffer_error_core (
            _("could not parse an outgoing message; it was not sent"));
        return strdup ("");
    }

    if (strchr (target, ','))
    {
        if (otr_modifier_all_targets_channels (ptr_server, target))
        {
            free (target);
            return NULL;
        }
        otr_buffer_error_core (_("messages to several targets including a nick "
                                 "are not supported with OTR; not sent"));
        free (target);
        return strdup ("");
    }

    if (otr_irc_target_is_channel (ptr_server, target))
    {
        free (target);
        return NULL;
    }

    account = otr_irc_make_account (ptr_server);
    peer = weechat_string_tolower (target);
    if (!account || !peer)
    {
        otr_buffer_error_core (
            _("not connected to server %s; message not sent"), ptr_server);
        result = strdup ("");
        goto end;
    }

    ptr_context = otr_context_find (account, peer);
    if (otr_context_encrypted (ptr_context))
    {
        converted = otr_message_plain_from_irc (ptr_text);
        shown = otr_util_dupstr (converted);
        if (converted && otr_policy_for (account, peer, "html_escape"))
        {
            plain = otr_message_html_escape (converted);
            free (converted);
        }
        else
        {
            plain = converted;
        }
    }
    else
    {
        plain = otr_util_dupstr (ptr_text);
    }
    if (!plain)
    {
        result = strdup ("");
        goto end;
    }

    ctcp = otr_message_is_ctcp (ptr_text);
    if (ctcp)
        otr_no_tag++;
    error = otrl_message_sending (otr_us, &otr_callback_ops, NULL, account,
                                  OTR_PROTOCOL, peer, OTRL_INSTAG_BEST, plain,
                                  NULL, &new_message,
                                  OTRL_FRAGMENT_SEND_ALL_BUT_LAST,
                                  NULL, NULL, NULL);
    if (ctcp)
        otr_no_tag--;

    if (error)
    {
        otr_buffer_peer_print (ptr_server, peer, OTR_CONFIG_LEVEL_ERROR,
                               _("Failed to send message; it was NOT sent."));
        otr_buffer_debug ("otrl_message_sending: %s", gcry_strerror (error));
        result = strdup ("");
        goto end;
    }

    if (new_message)
    {
        clean = otr_util_irc_sanitize (new_message);
        if (!clean || !clean[0])
        {
            result = strdup ("");
        }
        else
        {
            if (shown && (strncmp (clean, "?OTR", 4) == 0))
                otr_echo_pending_add (ptr_server, peer, clean, shown);

            /* WeeChat strips trailing spaces; keep whitespace tags intact. */
            length = strlen (clean);
            weechat_asprintf (&result, "PRIVMSG %s :%s%s",
                              target, clean,
                              (clean[length - 1] == ' ') ? "\t" : "");
        }
        otrl_message_free (new_message);
    }
    /* Otherwise: not an OTR message (plaintext allowed), result stays NULL. */

    weechat_bar_item_update (OTR_BAR_ITEM);

end:
    free (shown);
    free (clean);
    free (plain);
    free (target);
    free (account);
    free (peer);
    return result;
}
