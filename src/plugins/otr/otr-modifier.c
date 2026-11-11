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
#include <time.h>

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
 * Multiline batches removed from outgoing messages (keys: "server,ref"), see
 * function otr_modifier_remove_batch.
 */
struct t_hashtable *otr_modifier_removed_batches = NULL;

/*
 * Timer used to drop stale OTR messages being assembled (created when an
 * assembly starts, removed when there is no assembly anymore).
 */
struct t_hook *otr_modifier_assembly_timer = NULL;


/*
 * Check if a line is the continuation of an OTR encoded message split over
 * several IRC lines: only base64 chars, optionally followed by "." (end of the
 * OTR message).
 *
 * Return 1 if the line is a continuation of an OTR encoded message, 0
 * otherwise.
 */

int
otr_modifier_is_base64_part (const char *line)
{
    const char *ptr_line;

    if (!line[0])
        return 0;

    for (ptr_line = line; ptr_line[0]; ptr_line++)
    {
        if (isalnum ((unsigned char)ptr_line[0]) || (ptr_line[0] == '+')
            || (ptr_line[0] == '/') || (ptr_line[0] == '='))
        {
            continue;
        }
        if ((ptr_line[0] == '.') && !ptr_line[1] && (ptr_line > line))
            continue;
        return 0;
    }

    return 1;
}

/*
 * Drop the OTR message being assembled for a peer (incomplete message).
 */

void
otr_modifier_drop_assembly (struct t_otr_peer_data *peer_data)
{
    if (!peer_data || !peer_data->assembly)
        return;

    otr_buffer_debug ("incomplete OTR message dropped (length %d)",
                      (int)peer_data->assembly_length);
    free (peer_data->assembly);
    peer_data->assembly = NULL;
    peer_data->assembly_length = 0;
    peer_data->assembly_time = 0;
}

/*
 * Check if the OTR message being assembled for a peer is stale: its first line
 * was received more than OTR_ASSEMBLY_TIMEOUT seconds ago.
 *
 * Return 1 if the assembly is stale, 0 otherwise (or if there is no assembly).
 */

int
otr_modifier_assembly_stale (struct t_otr_peer_data *peer_data, time_t now)
{
    return (peer_data && peer_data->assembly
            && (now - peer_data->assembly_time > OTR_ASSEMBLY_TIMEOUT)) ?
        1 : 0;
}

/*
 * Callback of timer used to drop stale OTR messages being assembled (the peer
 * never sent the last lines), so that memory is not kept for all peers.
 *
 * The timer is removed when there is no OTR message being assembled anymore
 * (it is created again when an assembly starts).
 */

int
otr_modifier_assembly_timer_cb (const void *pointer, void *data,
                                int remaining_calls)
{
    ConnContext *ptr_context;
    struct t_otr_peer_data *ptr_peer_data;
    time_t now;
    int assembly_found;

    /* Make C compiler happy. */
    (void) pointer;
    (void) data;
    (void) remaining_calls;

    assembly_found = 0;

    if (otr_us)
    {
        now = time (NULL);
        for (ptr_context = otr_us->context_root; ptr_context;
             ptr_context = ptr_context->next)
        {
            ptr_peer_data = (struct t_otr_peer_data *)ptr_context->app_data;
            if (otr_modifier_assembly_stale (ptr_peer_data, now))
                otr_modifier_drop_assembly (ptr_peer_data);
            if (ptr_peer_data && ptr_peer_data->assembly)
                assembly_found = 1;
        }
    }

    if (!assembly_found && otr_modifier_assembly_timer)
    {
        weechat_unhook (otr_modifier_assembly_timer);
        otr_modifier_assembly_timer = NULL;
    }

    return WEECHAT_RC_OK;
}

/*
 * Add a received line to the assembly buffer of a peer: an OTR encoded message
 * ("?OTR:...") that does not end with "." has been split over several IRC
 * lines, and the next lines (only base64 chars) are added until the final ".".
 *
 * Any other line (plain text, OTR query, OTR fragment, OTR error) is returned
 * as-is; if an OTR message was being assembled, it is dropped (it is dropped
 * too if its first line was received more than OTR_ASSEMBLY_TIMEOUT seconds
 * ago).
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

    /* Too late for the next lines: drop the incomplete OTR message. */
    if (otr_modifier_assembly_stale (peer_data, time (NULL)))
        otr_modifier_drop_assembly (peer_data);

    if (peer_data->assembly)
    {
        if (otr_modifier_is_base64_part (line)
            && (peer_data->assembly_length + length + 1 <= OTR_MAX_ASSEMBLY))
        {
            new_assembly = realloc (peer_data->assembly,
                                    peer_data->assembly_length + length + 1);
            if (!new_assembly)
                return 0;
            peer_data->assembly = new_assembly;
            memcpy (peer_data->assembly + peer_data->assembly_length, line,
                    length + 1);
            peer_data->assembly_length += length;
            if (line[length - 1] != '.')
                return 0;
            *message = peer_data->assembly;
            peer_data->assembly = NULL;
            peer_data->assembly_length = 0;
            peer_data->assembly_time = 0;
            return 1;
        }
        /* Not a continuation: drop the incomplete OTR message. */
        otr_modifier_drop_assembly (peer_data);
    }

    if ((strncmp (line, "?OTR:", 5) == 0) && (line[length - 1] != '.')
        && (length + 1 <= OTR_MAX_ASSEMBLY))
    {
        /* Start of an OTR encoded message split over several IRC lines. */
        peer_data->assembly = otr_util_dupstr (line);
        if (!peer_data->assembly)
            return 0;
        peer_data->assembly_length = length;
        peer_data->assembly_time = time (NULL);
        if (!otr_modifier_assembly_timer)
        {
            otr_modifier_assembly_timer = weechat_hook_timer (
                OTR_ASSEMBLY_TIMEOUT * 1000, 0, 0,
                &otr_modifier_assembly_timer_cb, NULL, NULL);
        }
        return 0;
    }

    *message = otr_util_dupstr (line);

    return (*message) ? 1 : 0;
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
    struct t_otr_callback_data callback_data;
    OtrlTLV *tlvs;
    ConnContext *ptr_context;
    int ignore, was_otr, encrypted, disconnected;

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
    callback_data.unencrypted = NULL;
    callback_data.last_injected = NULL;
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
    if (otr_irc_nick_equal (ptr_server, own_nick, ptr_nick))
    {
        /* echo-message: the server returns our own outgoing message. */
        result = otr_echo_replace (ptr_server, string, ptr_host, target,
                                   ptr_text);
        goto end;
    }

    peer = otr_irc_nick_tolower (ptr_server, ptr_nick);
    if (!peer)
        goto end;
    account = otr_irc_peer_account (ptr_server, peer);
    if (!account)
        goto end;

    /*
     * CTCP: only a plaintext ACTION is flagged, in the same cases as libotr
     * does for other plaintext messages: OTR session started (encrypted or
     * finished) or encryption required by policy.
     */
    if (otr_message_is_ctcp (ptr_text))
    {
        ptr_context = otr_context_find (account, peer);
        if (otr_message_is_action (ptr_text)
            && ((ptr_context
                 && (ptr_context->msgstate != OTRL_MSGSTATE_PLAINTEXT))
                || otr_policy_for (account, peer, "require_encryption")))
        {
            plain = otr_message_plain_from_irc (ptr_text);
            if (plain)
            {
                tags = otr_message_get_tags (string);
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

    /*
     * OTR message: the OTR tag can be anywhere in the message (like in libotr,
     * which decrypts the message even if there is text before the tag).
     */
    was_otr = (strstr (assembled, "?OTR")) ? 1 : 0;
    ignore = otrl_message_receiving (otr_us, &otr_callback_ops,
                                     &callback_data, account, OTR_PROTOCOL,
                                     peer, assembled,
                                     &new_message, &tlvs, &ptr_context,
                                     NULL, NULL);
    disconnected = 0;
    if (tlvs)
    {
        /*
         * The peer has ended the OTR session (libotr does not call the
         * callback "gone_insecure"): ask the user to do the same.
         */
        if (ptr_context && otrl_tlv_find (tlvs, OTRL_TLV_DISCONNECTED))
        {
            disconnected = 1;
            otr_context_smp_reset (otr_context_peer_data_ctx (ptr_context));
            otr_buffer_ctx_print (
                ptr_context, OTR_CONFIG_LEVEL_INFO,
                _("%s has ended the private conversation. You should do the "
                  "same:\n/otr finish"),
                ptr_context->username);
        }
        otrl_tlv_free (tlvs);
    }

    tags = otr_message_get_tags (string);

    if (ignore)
    {
        if (callback_data.unencrypted)
        {
            /* Plaintext although the session is encrypted. */
            result = otr_message_build_in_privmsgs (
                (tags) ? tags : "", ptr_host, target,
                callback_data.unencrypted,
                OTR_UNENCRYPTED_PREFIX, 0);
        }
        else
        {
            result = strdup ("");
        }
    }
    else
    {
        /*
         * With the TLV "disconnected", the message has been decrypted but
         * libotr has already ended the session.
         */
        encrypted = disconnected
            || otr_context_encrypted (
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
    free (callback_data.unencrypted);
    free (callback_data.last_injected);
    free (tags);
    free (assembled);
    free (target);
    free (account);
    free (peer);
    weechat_hashtable_free (hashtable_out);
    return result;
}

/*
 * Parse an outgoing message "PRIVMSG target :text", with optional IRC tags
 * ("@tags PRIVMSG target :text").
 *
 * Return:
 *    1: PRIVMSG parsed (*tags is NULL if there are no tags, otherwise it
 *       includes the space after the tags; *tags and *target must be freed
 *       after use, *text points inside the message)
 *    0: not a PRIVMSG (for example "BATCH" of a multiline message, which is
 *       sent with modifier "irc_out_privmsg")
 *   -1: invalid PRIVMSG or memory error
 */

int
otr_modifier_parse_out_privmsg (const char *message, char **tags,
                                char **target, const char **text)
{
    const char *ptr_message, *ptr_space;

    *tags = NULL;
    *target = NULL;

    ptr_message = message;
    if (ptr_message[0] == '@')
    {
        ptr_space = strchr (ptr_message, ' ');
        if (!ptr_space)
            return 0;
        ptr_message = ptr_space + 1;
    }
    while (ptr_message[0] == ' ')
    {
        ptr_message++;
    }
    if (weechat_strncasecmp (ptr_message, "PRIVMSG ", 8) != 0)
        return 0;
    ptr_message += 8;

    if (message[0] == '@')
    {
        *tags = otr_message_get_tags (message);
        if (!*tags)
            return -1;
    }
    while (ptr_message[0] == ' ')
    {
        ptr_message++;
    }

    ptr_space = strchr (ptr_message, ' ');
    if (!ptr_space)
        goto error;
    *target = weechat_strndup (ptr_message, ptr_space - ptr_message);
    if (!*target)
        goto error;

    ptr_space++;
    if (ptr_space[0] == ':')
        ptr_space++;
    *text = ptr_space;

    return 1;

error:
    free (*tags);
    *tags = NULL;
    return -1;
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
 * Check if messages sent to a peer must be held until the OTR session is
 * encrypted: policy "require_encryption" is enabled and the session is not
 * started yet.
 *
 * libotr would keep only the last message (and send an OTR query for each
 * message), so messages are held by the plugin and sent when the session is
 * encrypted (see function otr_context_send_held_messages).
 *
 * Return 1 if messages must be held, 0 otherwise.
 */

int
otr_modifier_must_hold (const char *account, const char *peer)
{
    ConnContext *ptr_context;

    ptr_context = otr_context_find (account, peer);
    if (ptr_context && (ptr_context->msgstate != OTRL_MSGSTATE_PLAINTEXT))
        return 0;

    return (otr_policy_for (account, peer, "require_encryption")
            && otr_policy_query (account, peer)) ? 1 : 0;
}

/*
 * Hold a message sent to a peer until the OTR session is encrypted: a message
 * is displayed and an OTR query is sent with the first message held.
 */

void
otr_modifier_hold_message (const char *server, const char *account,
                           const char *peer, const char *message)
{
    struct t_otr_peer_data *ptr_peer_data;
    ConnContext *ptr_context;
    int first;

    ptr_peer_data = otr_context_peer_data_for (account, peer);
    if (!ptr_peer_data)
        return;

    first = (ptr_peer_data->held_messages) ? 0 : 1;
    if (!otr_context_hold_message (ptr_peer_data, message))
        return;

    ptr_context = otrl_context_find (otr_us, peer, account, OTR_PROTOCOL,
                                     OTRL_INSTAG_MASTER, 0, NULL, NULL, NULL);
    if (ptr_context)
    {
        otr_callback_ops.handle_msg_event (
            NULL, OTRL_MSGEVENT_ENCRYPTION_REQUIRED, ptr_context, NULL, 0);
    }

    if (first)
        otr_irc_send_raw (server, peer, otr_policy_query (account, peer));
}

/*
 * Callback for changes on OTR policy options: messages held until the OTR
 * session is encrypted are discarded if they must not be held anymore
 * (encryption not required anymore or OTR disabled), so that they are never
 * sent later, after newer messages.
 */

int
otr_modifier_policy_changed_cb (const void *pointer, void *data,
                                const char *option, const char *value)
{
    ConnContext *ptr_context, *ptr_best_context;
    struct t_otr_peer_data *ptr_peer_data;

    /* Make C compiler happy. */
    (void) pointer;
    (void) data;
    (void) option;
    (void) value;

    if (!otr_us)
        return WEECHAT_RC_OK;

    for (ptr_context = otr_us->context_root; ptr_context;
         ptr_context = ptr_context->next)
    {
        if (ptr_context->their_instance != OTRL_INSTAG_MASTER)
            continue;
        ptr_peer_data = (struct t_otr_peer_data *)ptr_context->app_data;
        if (!ptr_peer_data || !ptr_peer_data->held_messages)
            continue;
        /* OTR session started: messages held are being sent. */
        ptr_best_context = otr_context_find (ptr_context->accountname,
                                             ptr_context->username);
        if (ptr_best_context
            && (ptr_best_context->msgstate != OTRL_MSGSTATE_PLAINTEXT))
        {
            continue;
        }
        if (otr_modifier_must_hold (ptr_context->accountname,
                                    ptr_context->username))
        {
            continue;
        }
        otr_context_drop_held_messages (ptr_context, ptr_peer_data, 1);
    }

    return WEECHAT_RC_OK;
}

/*
 * Check a line "BATCH" of an outgoing multiline message (IRCv3 capability
 * "draft/multiline"): the batch is removed if it is sent to a nick with an
 * OTR session started (encrypted or finished, where messages are not sent at
 * all: the batch would be empty), or if messages to the nick are held until
 * the session is encrypted.
 *
 * Servers hold the lines of a batch until its end, while all OTR fragments of
 * a line except the last one are sent immediately (out of the batch): the
 * fragments would be received in the wrong order. Without the batch, each
 * line is encrypted and sent as a separate message.
 *
 * Return 1 if the line must be dropped (batch removed), 0 otherwise.
 */

int
otr_modifier_remove_batch (const char *server, const char *message)
{
    char **items, *key, *peer, *account;
    ConnContext *ptr_context;
    int num_items, removed;

    if (weechat_strncasecmp (message, "BATCH ", 6) != 0)
        return 0;

    items = weechat_string_split (message + 6, " ", NULL,
                                  WEECHAT_STRING_SPLIT_STRIP_LEFT
                                  | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                  | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                  0, &num_items);
    if (!items)
        return 0;

    removed = 0;
    key = NULL;
    peer = NULL;
    account = NULL;

    if ((num_items < 1) || !items[0][1]
        || ((items[0][0] != '+') && (items[0][0] != '-')))
    {
        goto end;
    }

    weechat_asprintf (&key, "%s,%s", server, items[0] + 1);
    if (!key)
        goto end;

    if (items[0][0] == '-')
    {
        /* End of a batch removed. */
        if (otr_modifier_removed_batches
            && weechat_hashtable_has_key (otr_modifier_removed_batches, key))
        {
            weechat_hashtable_remove (otr_modifier_removed_batches, key);
            removed = 1;
        }
        goto end;
    }

    /* Start of a multiline batch sent to a nick. */
    if ((num_items != 3) || (strcmp (items[1], "draft/multiline") != 0)
        || otr_irc_target_is_channel (server, items[2]))
    {
        goto end;
    }
    peer = otr_irc_nick_tolower (server, items[2]);
    account = (peer) ? otr_irc_peer_account (server, peer) : NULL;
    if (!account)
        goto end;
    ptr_context = otr_context_find (account, peer);
    if ((!ptr_context || (ptr_context->msgstate == OTRL_MSGSTATE_PLAINTEXT))
        && !otr_modifier_must_hold (account, peer))
    {
        goto end;
    }
    if (!otr_modifier_removed_batches)
    {
        otr_modifier_removed_batches = weechat_hashtable_new (
            8,
            WEECHAT_HASHTABLE_STRING,
            WEECHAT_HASHTABLE_STRING,
            NULL, NULL);
        if (!otr_modifier_removed_batches)
            goto end;
    }
    weechat_hashtable_set (otr_modifier_removed_batches, key, NULL);
    removed = 1;

end:
    free (key);
    free (peer);
    free (account);
    weechat_string_free_split (items);
    return removed;
}

/*
 * Remove tag "batch" from IRC tags of an outgoing message if this batch has
 * been removed (see function otr_modifier_remove_batch).
 *
 * Tags are in format "@tag1;tag2 " (with "@" and the space after tags).
 *
 * Return new tags (same format), NULL if there are no tags anymore.
 *
 * Note: result must be freed after use.
 */

char *
otr_modifier_remove_batch_tag (const char *server, const char *tags)
{
    char *tags2, **items, **result, *key;
    int i, num_items, removed;
    size_t length;

    if (!tags || (tags[0] != '@'))
        return NULL;

    /* Copy tags without "@" and the space after tags. */
    length = strlen (tags + 1);
    if ((length > 0) && (tags[length] == ' '))
        length--;
    tags2 = weechat_strndup (tags + 1, length);
    if (!tags2)
        return NULL;

    items = weechat_string_split (tags2, ";", NULL, 0, 0, &num_items);
    free (tags2);
    result = weechat_string_dyn_alloc (64);
    if (!items || !result)
    {
        weechat_string_free_split (items);
        weechat_string_dyn_free (result, 1);
        return strdup (tags);
    }

    for (i = 0; i < num_items; i++)
    {
        if (strncmp (items[i], "batch=", 6) == 0)
        {
            weechat_asprintf (&key, "%s,%s", server, items[i] + 6);
            removed = (key && otr_modifier_removed_batches
                       && weechat_hashtable_has_key (
                           otr_modifier_removed_batches, key)) ? 1 : 0;
            free (key);
            if (removed)
                continue;
        }
        weechat_string_dyn_concat (result, ((*result)[0]) ? ";" : "@", -1);
        weechat_string_dyn_concat (result, items[i], -1);
    }
    weechat_string_free_split (items);

    if (!(*result)[0])
    {
        weechat_string_dyn_free (result, 1);
        return NULL;
    }
    weechat_string_dyn_concat (result, " ", -1);

    return weechat_string_dyn_free (result, 0);
}

/*
 * Free multiline batches removed from outgoing messages.
 */

void
otr_modifier_free_removed_batches (void)
{
    weechat_hashtable_free (otr_modifier_removed_batches);
    otr_modifier_removed_batches = NULL;
}

/*
 * Callback for modifier "irc_out_privmsg": encrypt outgoing private messages
 * with OTR.
 *
 * Fail closed: any problem returns an empty string, which makes WeeChat drop
 * the message instead of sending it unencrypted.
 *
 * CTCP other than ACTION (VERSION, DCC...) are sent unencrypted: the IRC
 * plugin of the peer could not handle them if they were encrypted.
 *
 * Return new message, NULL to send it unchanged (channel, CTCP other than
 * ACTION, or plain text allowed by policies), an empty string to drop it.
 */

char *
otr_modifier_out_cb (const void *pointer, void *data, const char *modifier,
                     const char *modifier_data, const char *string)
{
    char *tags, *target, *account, *peer, *plain, *new_message, *result;
    char *clean, *shown, *converted, *new_tags;
    const char *ptr_text, *ptr_server;
    struct t_otr_peer_data *ptr_peer_data;
    ConnContext *ptr_context, *ptr_master_context;
    gcry_error_t error;
    size_t length;
    int action, batch_removed, started;
    struct t_otr_callback_data callback_data;
    OtrlFragmentPolicy fragment_policy;

    /* Make C compiler happy. */
    (void) pointer;
    (void) data;
    (void) modifier;

    if (otr_injecting)
        return NULL;
    if (!otr_us)
        return strdup ("");

    ptr_server = modifier_data;
    tags = NULL;
    target = NULL;
    account = NULL;
    peer = NULL;
    plain = NULL;
    new_message = NULL;
    result = NULL;
    clean = NULL;
    shown = NULL;
    callback_data.unencrypted = NULL;
    callback_data.last_injected = NULL;

    switch (otr_modifier_parse_out_privmsg (string, &tags, &target, &ptr_text))
    {
        case 0:
            /*
             * Not a PRIVMSG (no text to encrypt): leave it unchanged, except
             * a line "BATCH" of a batch removed.
             */
            return (otr_modifier_remove_batch (ptr_server, string)) ?
                strdup ("") : NULL;
        case 1:
            break;
        default:
            otr_buffer_error_core (
                _("could not parse an outgoing message; it was not sent"));
            return strdup ("");
    }

    /* Line of a multiline batch removed: remove the tag "batch". */
    batch_removed = 0;
    if (tags)
    {
        new_tags = otr_modifier_remove_batch_tag (ptr_server, tags);
        batch_removed = (!new_tags || (strcmp (new_tags, tags) != 0)) ? 1 : 0;
        free (tags);
        tags = new_tags;
    }

    if (strchr (target, ','))
    {
        if (!otr_modifier_all_targets_channels (ptr_server, target))
        {
            otr_buffer_error_core (_("messages to several targets including a "
                                     "nick are not supported with OTR; not "
                                     "sent"));
            result = strdup ("");
        }
        goto end;
    }

    if (otr_irc_target_is_channel (ptr_server, target))
        goto end;

    /* CTCP other than ACTION: sent unencrypted. */
    if (otr_message_is_ctcp (ptr_text) && !otr_message_is_action (ptr_text))
        goto end;

    peer = otr_irc_nick_tolower (ptr_server, target);
    account = (peer) ? otr_irc_peer_account (ptr_server, peer) : NULL;
    if (!account || !peer)
    {
        otr_buffer_error_core (
            _("not connected to server %s; message not sent"), ptr_server);
        result = strdup ("");
        goto end;
    }

    /* Encryption required, no OTR session yet: message held. */
    if (otr_modifier_must_hold (account, peer))
    {
        otr_modifier_hold_message (ptr_server, account, peer, ptr_text);
        result = strdup ("");
        goto end;
    }

    ptr_context = otr_context_find (account, peer);
    if (otr_context_encrypted (ptr_context))
    {
        /*
         * Messages held are not sent yet (they are sent with a timer when
         * the session becomes encrypted): the message is held after them, to
         * keep the order of messages.
         */
        ptr_master_context = otrl_context_find (otr_us, peer, account,
                                                OTR_PROTOCOL,
                                                OTRL_INSTAG_MASTER, 0,
                                                NULL, NULL, NULL);
        ptr_peer_data = (ptr_master_context) ?
            (struct t_otr_peer_data *)ptr_master_context->app_data : NULL;
        if (ptr_peer_data && ptr_peer_data->held_messages
            && otr_context_hold_message (ptr_peer_data, ptr_text))
        {
            result = strdup ("");
            goto end;
        }
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

    /*
     * Line of a multiline batch removed: the modifier result would be sent
     * with immediate priority, before the fragments injected with high
     * priority (anti-flood): all fragments are injected, in order.
     */
    fragment_policy = (batch_removed) ?
        OTRL_FRAGMENT_SEND_ALL : OTRL_FRAGMENT_SEND_ALL_BUT_LAST;

    /* OTR session started (encrypted or finished): never send in plain text. */
    started = (ptr_context
               && (ptr_context->msgstate != OTRL_MSGSTATE_PLAINTEXT)) ? 1 : 0;

    /* No whitespace tag after a CTCP ACTION sent in plain text. */
    action = otr_message_is_action (ptr_text);
    if (action)
        otr_no_tag++;
    error = otrl_message_sending (otr_us, &otr_callback_ops, &callback_data,
                                  account, OTR_PROTOCOL, peer,
                                  OTRL_INSTAG_BEST, plain, NULL, &new_message,
                                  fragment_policy, NULL, NULL, NULL);
    if (action)
        otr_no_tag--;

    if (error)
    {
        otr_buffer_peer_print (ptr_server, peer, OTR_CONFIG_LEVEL_ERROR,
                               _("Failed to send message; it was NOT sent."));
        otr_buffer_debug ("otrl_message_sending: %s", gcry_strerror (error));
        result = strdup ("");
        goto end;
    }

    if (new_message && batch_removed)
    {
        /* All fragments have been injected: nothing else to send. */
        clean = (callback_data.last_injected) ?
            otr_util_irc_sanitize (callback_data.last_injected) : NULL;
        if (shown && clean && (strncmp (clean, "?OTR", 4) == 0))
            otr_echo_pending_add (ptr_server, peer, clean, shown);
        result = strdup ("");
        otrl_message_free (new_message);
    }
    else if (new_message)
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
            weechat_asprintf (&result, "%sPRIVMSG %s :%s%s",
                              (tags) ? tags : "", target, clean,
                              (clean[length - 1] == ' ') ? "\t" : "");
        }
        otrl_message_free (new_message);
    }
    else if (started)
    {
        /*
         * No OTR message although the OTR session is started: OTR disabled
         * by policies (allow_v2 and allow_v3 off) or encryption error
         * (already displayed by libotr); the message is not sent.
         */
        if (!otr_policy_query (account, peer))
        {
            otr_buffer_peer_print (
                ptr_server, peer, OTR_CONFIG_LEVEL_ERROR,
                _("Failed to send message; it was NOT sent."));
            otr_buffer_peer_print (
                ptr_server, peer, OTR_CONFIG_LEVEL_ERROR,
                _("OTR is disabled by policy (allow_v2 and allow_v3 are both "
                  "off)."));
        }
        result = strdup ("");
    }
    /* Otherwise: not an OTR message (plaintext allowed), result stays NULL. */

    weechat_bar_item_update (OTR_BAR_ITEM);

end:
    /* Message sent unchanged: remove the tag of a batch removed. */
    if (!result && batch_removed)
    {
        weechat_asprintf (&result, "%sPRIVMSG %s :%s",
                          (tags) ? tags : "", target, ptr_text);
    }
    free (callback_data.unencrypted);
    free (callback_data.last_injected);
    free (shown);
    free (clean);
    free (plain);
    free (tags);
    free (target);
    free (account);
    free (peer);
    return result;
}
