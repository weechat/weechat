/*
 * irc-protocol.c - implementation of IRC protocol
 *
 * Copyright (C) 2003-2023 Sébastien Helleu <flashcode@flashtux.org>
 * Copyright (C) 2006 Emmanuel Bouthenot <kolter@openics.org>
 * Copyright (C) 2014 Shawn Smith <ShawnSmith0828@gmail.com>
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

/* this define is needed for strptime() (not on OpenBSD/Sun) */
#if !defined(__OpenBSD__) && !defined(__sun)
#define _XOPEN_SOURCE 700
#endif

#ifndef __USE_XOPEN
#define __USE_XOPEN
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <time.h>

#include "../weechat-plugin.h"
#include "irc.h"
#include "irc-protocol.h"
#include "irc-bar-item.h"
#include "irc-batch.h"
#include "irc-buffer.h"
#include "irc-channel.h"
#include "irc-color.h"
#include "irc-command.h"
#include "irc-config.h"
#include "irc-ctcp.h"
#include "irc-ignore.h"
#include "irc-input.h"
#include "irc-join.h"
#include "irc-message.h"
#include "irc-mode.h"
#include "irc-modelist.h"
#include "irc-msgbuffer.h"
#include "irc-nick.h"
#include "irc-notify.h"
#include "irc-sasl.h"
#include "irc-server.h"
#include "irc-tag.h"
#include "irc-typing.h"


/*
 * Checks if a command is numeric.
 *
 * Returns:
 *   1: all chars are numeric
 *   0: command has other chars (not numeric)
 */

int
irc_protocol_is_numeric_command (const char *command)
{
    if (!command || !command[0])
        return 0;

    while (command && command[0])
    {
        if (!isdigit ((unsigned char)command[0]))
            return 0;
        command++;
    }

    return 1;
}

/*
 * Gets log level for IRC command.
 */

int
irc_protocol_log_level_for_command (const char *command)
{
    if (!command || !command[0])
        return 0;

    if ((strcmp (command, "privmsg") == 0)
        || (strcmp (command, "notice") == 0))
        return 1;

    if (strcmp (command, "nick") == 0)
        return 2;

    if ((strcmp (command, "join") == 0)
        || (strcmp (command, "part") == 0)
        || (strcmp (command, "quit") == 0)
        || (strcmp (command, "nick_back") == 0))
        return 4;

    return 3;
}

/*
 * Adds IRC tag key/value in a dynamic string.
 *
 * As commas are not allowed in WeeChat tags, they are replaced by semicolons.
 */

void
irc_protocol_tags_add_cb (void *data,
                          struct t_hashtable *hashtable,
                          const void *key,
                          const void *value)
{
    const char *ptr_key, *ptr_value;
    char **str_tags, *str_temp;

    /* make C compiler happy */
    (void) hashtable;

    ptr_key = (const char *)key;
    ptr_value = (const char *)value;

    str_tags = (char **)data;

    if (*str_tags[0])
        weechat_string_dyn_concat (str_tags, ",", -1);

    weechat_string_dyn_concat (str_tags, "irc_tag_", -1);

    /* key */
    str_temp = weechat_string_replace (ptr_key, ",", ";");
    weechat_string_dyn_concat (str_tags, str_temp, -1);
    if (str_temp)
        free (str_temp);

    /* separator between key and value */
    if (ptr_value)
        weechat_string_dyn_concat (str_tags, "=", -1);

    /* value */
    str_temp = weechat_string_replace (ptr_value, ",", ";");
    weechat_string_dyn_concat (str_tags, str_temp, -1);
    if (str_temp)
        free (str_temp);
}

/*
 * Builds tags list with IRC command and optional tags and nick.
 */

const char *
irc_protocol_tags (struct t_irc_server *server, const char *command,
                   struct t_hashtable *irc_msg_tags, const char *extra_tags,
                   const char *nick, const char *address)
{
    static char string[4096];
    int log_level, is_numeric, has_irc_tags;
    const char *ptr_tag_batch;
    char str_log_level[32], **str_irc_tags;
    struct t_irc_batch *ptr_batch;

    str_log_level[0] = '\0';

    is_numeric = irc_protocol_is_numeric_command (command);
    has_irc_tags = (irc_msg_tags
                    && weechat_hashtable_get_integer (irc_msg_tags,
                                                      "items_count") > 0);

    if (!command && !has_irc_tags && !extra_tags && !nick)
        return NULL;

    if (has_irc_tags)
    {
        str_irc_tags = weechat_string_dyn_alloc (256);
        weechat_hashtable_map (irc_msg_tags, irc_protocol_tags_add_cb,
                               str_irc_tags);
        if (server)
        {
            ptr_tag_batch = weechat_hashtable_get (irc_msg_tags, "batch");
            if (ptr_tag_batch && ptr_tag_batch)
            {
                ptr_batch = irc_batch_search (server, ptr_tag_batch);
                if (ptr_batch)
                {
                    if (*str_irc_tags[0])
                        weechat_string_dyn_concat (str_irc_tags, ",", -1);
                    weechat_string_dyn_concat (str_irc_tags,
                                               "irc_batch_type_", -1);
                    weechat_string_dyn_concat (str_irc_tags,
                                               ptr_batch->type, -1);
                }
            }
        }
    }
    else
    {
        str_irc_tags = NULL;
    }

    if (command && command[0])
    {
        log_level = irc_protocol_log_level_for_command (command);
        if (log_level > 0)
        {
            snprintf (str_log_level, sizeof (str_log_level),
                      ",log%d", log_level);
        }
    }

    snprintf (string, sizeof (string),
              "%s%s%s%s%s%s%s%s%s%s%s%s%s",
              (command && command[0]) ? "irc_" : "",
              (command && command[0]) ? command : "",
              (is_numeric) ? "," : "",
              (is_numeric) ? "irc_numeric" : "",
              (str_irc_tags && *str_irc_tags[0]) ? "," : "",
              (str_irc_tags && *str_irc_tags[0]) ? *str_irc_tags : "",
              (extra_tags && extra_tags[0]) ? "," : "",
              (extra_tags && extra_tags[0]) ? extra_tags : "",
              (nick && nick[0]) ? ",nick_" : "",
              (nick && nick[0]) ? nick : "",
              (address && address[0]) ? ",host_" : "",
              (address && address[0]) ? address : "",
              str_log_level);

    if (str_irc_tags)
        weechat_string_dyn_free (str_irc_tags, 1);

    return string;
}

/*
 * Builds a string with nick and optional address.
 *
 * If server_message is 1, the nick is colored according to option
 * irc.look.color_nicks_in_server_messages.
 *
 * Argument nickname is mandatory, address can be NULL.
 * If nickname and address are NULL, an empty string is returned.
 */

const char *
irc_protocol_nick_address (struct t_irc_server *server,
                           int server_message,
                           struct t_irc_nick *nick,
                           const char *nickname,
                           const char *address)
{
    static char string[1024];

    string[0] = '\0';

    if (nickname && address && address[0] && (strcmp (nickname, address) != 0))
    {
        /* display nick and address if they are different */
        snprintf (string, sizeof (string),
                  "%s%s %s(%s%s%s)%s",
                  irc_nick_color_for_msg (server, server_message, nick,
                                          nickname),
                  nickname,
                  IRC_COLOR_CHAT_DELIMITERS,
                  IRC_COLOR_CHAT_HOST,
                  address,
                  IRC_COLOR_CHAT_DELIMITERS,
                  IRC_COLOR_RESET);
    }
    else if (nickname)
    {
        /* display only nick if no address or if nick == address */
        snprintf (string, sizeof (string),
                  "%s%s%s",
                  irc_nick_color_for_msg (server, server_message, nick,
                                          nickname),
                  nickname,
                  IRC_COLOR_RESET);
    }

    return string;
}

/*
 * Parses date/time received in a "time" tag.
 *
 * Returns value of time (timestamp), 0 if error.
 */

time_t
irc_protocol_parse_time (const char *time)
{
    time_t time_value, time_msg, time_gm, time_local;
    struct tm tm_date, tm_date_gm, tm_date_local;
    long value;
    char *time2, *pos, *error;

    if (!time || !time[0])
        return 0;

    time_value = 0;

    if (strchr (time, '-'))
    {
        /* date is with ISO 8601 format: "2012-11-24T07:41:02.018Z" */
        /* initialize structure, because strptime does not do it */
        memset (&tm_date, 0, sizeof (struct tm));
        if (strptime (time, "%Y-%m-%dT%H:%M:%S", &tm_date))
        {
            if (tm_date.tm_year > 0)
            {
                time_msg = mktime (&tm_date);
                gmtime_r (&time_msg, &tm_date_gm);
                localtime_r (&time_msg, &tm_date_local);
                time_gm = mktime (&tm_date_gm);
                time_local = mktime (&tm_date_local);
                time_value = mktime (&tm_date_local) + (time_local - time_gm);
            }
        }
    }
    else
    {
        /* date is with timestamp format: "1353403519.478" */
        time2 = strdup (time);
        if (time2)
        {
            pos = strchr (time2, '.');
            if (pos)
                pos[0] = '\0';
            pos = strchr (time2, ',');
            if (pos)
                pos[0] = '\0';
            value = strtol (time2, &error, 10);
            if (error && !error[0] && (value >= 0))
                time_value = (int)value;
            free (time2);
        }
    }

    return time_value;
}

/*
 * Builds a string with concatenation of IRC command parameters, from
 * arg_start to arg_end.
 *
 * Note: result must be free after use.
 */

char *
irc_protocol_string_params (const char **params, int arg_start, int arg_end)
{
    char *result;

    result = weechat_string_rebuild_split_string (params, " ",
                                                  arg_start, arg_end);
    return (result) ? result : strdup ("");
}

/*
 * Prints a FAIL/WARN/NOTE command.
 *
 * Called by callbacks for commands: FAIL, WARN, NOTE.
 *
 * Commands look like:
 *   FAIL * NEED_REGISTRATION :You need to be registered to continue
 *   FAIL ACC REG_INVALID_CALLBACK REGISTER :Email address is not valid
 *   FAIL BOX BOXES_INVALID STACK CLOCKWISE :Given boxes are not supported
 *   WARN REHASH CERTS_EXPIRED :Certificate [xxx] has expired
 *   NOTE * OPER_MESSAGE :The message
 */

void
irc_protocol_print_error_warning_msg (struct t_irc_server *server,
                                      time_t date,
                                      struct t_hashtable *tags,
                                      const char *command,
                                      const char *prefix,
                                      const char *label,
                                      const char **params,
                                      int num_params)
{
    const char *ptr_command;
    char *str_context;

    ptr_command = (strcmp (params[0], "*") != 0) ?
        params[0] : NULL;

    str_context = (num_params > 2) ?
        irc_protocol_string_params (params, 1, num_params - 2) : NULL;

    weechat_printf_date_tags (
        irc_msgbuffer_get_target_buffer (server, NULL, command, NULL, NULL),
        date,
        irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
        "%s%s%s%s%s%s[%s%s%s]%s %s",
        (prefix) ? prefix : "",
        (label) ? label : "",
        (label) ? " " : "",
        (ptr_command) ? ptr_command : "",
        (ptr_command) ? " " : "",
        IRC_COLOR_CHAT_DELIMITERS,
        IRC_COLOR_RESET,
        (str_context) ? str_context : "",
        IRC_COLOR_CHAT_DELIMITERS,
        IRC_COLOR_RESET,
        params[num_params - 1]);

    if (str_context)
        free (str_context);
}

/*
 * Callback for the IRC command "ACCOUNT": account info about a nick
 * (with capability "account-notify").
 *
 * Command looks like:
 *   ACCOUNT *
 *   ACCOUNT :accountname
 */

IRC_PROTOCOL_CALLBACK(account)
{
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick;
    struct t_irc_channel_speaking *ptr_nick_speaking;
    const char *pos_account;
    char str_account[512];
    int cap_account_notify, local_account, smart_filter;

    IRC_PROTOCOL_MIN_PARAMS(1);

    local_account = (irc_server_strcasecmp (server, nick, server->nick) == 0);

    pos_account = params[0];
    if (strcmp (pos_account, "*") == 0)
        pos_account = NULL;

    str_account[0] = '\0';
    if (pos_account)
    {
        snprintf (str_account, sizeof (str_account),
                  "%s%s",
                  irc_nick_color_for_msg (server, 1, NULL, pos_account),
                  pos_account);
    }

    cap_account_notify = weechat_hashtable_has_key (server->cap_list,
                                                    "account-notify");

    for (ptr_channel = server->channels; ptr_channel;
         ptr_channel = ptr_channel->next_channel)
    {
        switch (ptr_channel->type)
        {
            case IRC_CHANNEL_TYPE_PRIVATE:
                if (!ignored
                    && weechat_config_boolean (irc_config_look_display_account_message)
                    && (irc_server_strcasecmp (server,
                                               ptr_channel->name, nick) == 0))
                {
                    weechat_printf_date_tags (
                        irc_msgbuffer_get_target_buffer (
                            server, NULL, command, NULL, ptr_channel->buffer),
                        date,
                        irc_protocol_tags (
                            server,
                            command,
                            tags,
                            NULL,
                            nick, address),
                        (pos_account) ? _("%s%s%s%s has identified as %s") : _("%s%s%s%s has unidentified"),
                        weechat_prefix ("network"),
                        irc_nick_color_for_msg (server, 1, NULL, nick),
                        nick,
                        IRC_COLOR_MESSAGE_ACCOUNT,
                        (pos_account) ? str_account : NULL);
                }
                break;
            case IRC_CHANNEL_TYPE_CHANNEL:
                ptr_nick = irc_nick_search (server, ptr_channel, nick);
                if (ptr_nick)
                {
                    if (!ignored
                        && weechat_config_boolean (irc_config_look_display_account_message))
                    {
                        ptr_nick_speaking = ((weechat_config_boolean (irc_config_look_smart_filter))
                                             && (weechat_config_boolean (irc_config_look_smart_filter_account))) ?
                            irc_channel_nick_speaking_time_search (server, ptr_channel, nick, 1) : NULL;
                        smart_filter = (!local_account
                                        && weechat_config_boolean (irc_config_look_smart_filter)
                                        && weechat_config_boolean (irc_config_look_smart_filter_account)
                                        && !ptr_nick_speaking);

                        weechat_printf_date_tags (
                            irc_msgbuffer_get_target_buffer (
                                server, NULL, command, NULL, ptr_channel->buffer),
                            date,
                            irc_protocol_tags (
                                server,
                                command,
                                tags,
                                (smart_filter) ? "irc_smart_filter" : NULL,
                                nick, address),
                            (pos_account) ? _("%s%s%s%s has identified as %s") : _("%s%s%s%s has unidentified"),
                            weechat_prefix ("network"),
                            irc_nick_color_for_msg (server, 1, ptr_nick, nick),
                            nick,
                            IRC_COLOR_MESSAGE_ACCOUNT,
                            (pos_account) ? str_account : NULL);
                    }
                    if (ptr_nick->account)
                        free (ptr_nick->account);
                    ptr_nick->account = (cap_account_notify && pos_account) ?
                        strdup (pos_account) : NULL;
                }
                break;
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "AUTHENTICATE".
 *
 * Command looks like:
 *   AUTHENTICATE +
 *   AUTHENTICATE QQDaUzXAmVffxuzFy77XWBGwABBQAgdinelBrKZaR3wE7nsIETuTVY=
 */

IRC_PROTOCOL_CALLBACK(authenticate)
{
    int sasl_mechanism;
    char *sasl_username, *sasl_password, *sasl_key, *answer;
    char *sasl_error;

    IRC_PROTOCOL_MIN_PARAMS(1);

    if (!irc_server_sasl_enabled (server))
        return WEECHAT_RC_OK;

    irc_server_sasl_get_creds (server, &sasl_username, &sasl_password,
                               &sasl_key);

    sasl_mechanism = IRC_SERVER_OPTION_INTEGER(
        server, IRC_SERVER_OPTION_SASL_MECHANISM);

    answer = NULL;
    sasl_error = NULL;
    switch (sasl_mechanism)
    {
        case IRC_SASL_MECHANISM_PLAIN:
            answer = irc_sasl_mechanism_plain (sasl_username, sasl_password);
            break;
        case IRC_SASL_MECHANISM_SCRAM_SHA_1:
            answer = irc_sasl_mechanism_scram (
                server, "sha1", params[0],
                sasl_username, sasl_password, &sasl_error);
            break;
        case IRC_SASL_MECHANISM_SCRAM_SHA_256:
            answer = irc_sasl_mechanism_scram (
                server, "sha256", params[0],
                sasl_username, sasl_password, &sasl_error);
            break;
        case IRC_SASL_MECHANISM_SCRAM_SHA_512:
            answer = irc_sasl_mechanism_scram (
                server, "sha512", params[0],
                sasl_username, sasl_password, &sasl_error);
            break;
        case IRC_SASL_MECHANISM_ECDSA_NIST256P_CHALLENGE:
            answer = irc_sasl_mechanism_ecdsa_nist256p_challenge (
                server, params[0],
                sasl_username, sasl_key, &sasl_error);
            break;
        case IRC_SASL_MECHANISM_EXTERNAL:
            answer = strdup ("+");
            break;
    }
    if (answer)
    {
        if (sasl_error)
        {
            weechat_printf (server->buffer,
                            _("%s%s: SASL error: %s"),
                            weechat_prefix ("error"),
                            IRC_PLUGIN_NAME,
                            sasl_error);
        }
        else
        {
            server->sasl_mechanism_used = sasl_mechanism;
        }
        irc_server_sendf (server, 0, NULL, "AUTHENTICATE %s", answer);
        free (answer);
    }
    else
    {
        weechat_printf (server->buffer,
                        _("%s%s: SASL error: %s"),
                        weechat_prefix ("error"),
                        IRC_PLUGIN_NAME,
                        (sasl_error) ? sasl_error : _("internal error"));
        irc_server_sendf (server, 0, NULL, "CAP END");
    }
    if (sasl_username)
        free (sasl_username);
    if (sasl_password)
        free (sasl_password);
    if (sasl_key)
        free (sasl_key);
    if (sasl_error)
        free (sasl_error);

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "AWAY": away info about a nick (with capability
 * "away-notify").
 *
 * Command looks like:
 *   AWAY
 *   AWAY :I am away
 */

IRC_PROTOCOL_CALLBACK(away)
{
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick;

    IRC_PROTOCOL_MIN_PARAMS(0);
    IRC_PROTOCOL_CHECK_NICK;

    for (ptr_channel = server->channels; ptr_channel;
         ptr_channel = ptr_channel->next_channel)
    {
        ptr_nick = irc_nick_search (server, ptr_channel, nick);
        if (ptr_nick)
        {
            irc_nick_set_away (server, ptr_channel, ptr_nick,
                               (num_params > 0));
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "BATCH": start/end batched events
 * (with capability "batch").
 *
 * Command looks like:
 *   BATCH +yXNAbvnRHTRBv netsplit irc.hub other.host
 *   BATCH -yXNAbvnRHTRBv
 */

IRC_PROTOCOL_CALLBACK(batch)
{
    char *str_params;

    IRC_PROTOCOL_MIN_PARAMS(1);

    /* do nothing (but ignore BATCH) if capability "batch" is not enabled */
    if (!weechat_hashtable_has_key (server->cap_list, "batch"))
        return WEECHAT_RC_OK;

    if (params[0][0] == '+')
    {
        /* start batch */
        if (num_params < 2)
            return WEECHAT_RC_ERROR;
        str_params = (num_params > 2) ?
            irc_protocol_string_params (params, 2, num_params - 1) : NULL;
        irc_batch_start_batch (
            server,
            params[0] + 1,  /* reference */
            weechat_hashtable_get (tags, "batch"),  /* parent ref */
            params[1],  /* type */
            str_params);
        if (str_params)
            free (str_params);
    }
    else if (params[0][0] == '-')
    {
        /* end batch */
        irc_batch_end_batch (server, params[0] + 1);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for IRC server capabilities string hashtable map.
 */

void
irc_protocol_cap_print_cb (void *data,
                           struct t_hashtable *hashtable,
                           const char *key, const char *value)
{
    char **str_caps;

    /* make C compiler happy */
    (void) hashtable;

    str_caps = (char **)data;

    if (*str_caps[0])
        weechat_string_dyn_concat (str_caps, " ", -1);
    weechat_string_dyn_concat (str_caps, key, -1);
    if (value)
    {
        weechat_string_dyn_concat (str_caps, "=", -1);
        weechat_string_dyn_concat (str_caps, value, -1);
    }
}

/*
 * Get capabilities to enable on the server (server option "capabilities" with
 * "sasl" if requested, "*" is replaced by all supported capabilities).
 *
 * Note: result must be freed after use.
 */

char *
irc_protocol_cap_to_enable (const char *capabilities, int sasl_requested)
{
    char **str_caps, **caps, *supported_caps;
    int i, num_caps;

    str_caps = weechat_string_dyn_alloc (128);
    if (!str_caps)
        return NULL;

    if (capabilities && capabilities[0])
    {
        caps = weechat_string_split (
            capabilities,
            ",",
            NULL,
            WEECHAT_STRING_SPLIT_STRIP_LEFT
            | WEECHAT_STRING_SPLIT_STRIP_RIGHT
            | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
            0,
            &num_caps);
        if (caps)
        {
            for (i = 0; i < num_caps; i++)
            {
                if (strcmp (caps[i], "*") == 0)
                {
                    supported_caps = weechat_string_replace (
                        IRC_COMMAND_CAP_SUPPORTED,
                        "|",
                        ",");
                    if (supported_caps)
                    {
                        if (*str_caps[0])
                            weechat_string_dyn_concat (str_caps, ",", -1);
                        weechat_string_dyn_concat (str_caps, supported_caps, -1);
                        free (supported_caps);
                    }
                }
                else
                {
                    if (*str_caps[0])
                        weechat_string_dyn_concat (str_caps, ",", -1);
                    weechat_string_dyn_concat (str_caps, caps[i], -1);
                }
            }
            weechat_string_free_split (caps);
        }
    }

    if (sasl_requested)
    {
        if (*str_caps[0])
            weechat_string_dyn_concat (str_caps, ",", -1);
        weechat_string_dyn_concat (str_caps, "sasl", -1);
    }

    return weechat_string_dyn_free (str_caps, 0);
}

/*
 * Requests capabilities for IRC server after synchronization.
 */

void
irc_protocol_cap_sync_req (struct t_irc_server *server,
                           const char *caps_server,
                           const char *caps_req)
{
    char modifier_data[4096], *new_caps_req;
    const char *ptr_caps_req;

    snprintf (modifier_data, sizeof (modifier_data),
              "%s,%s",
              server->name,
              (caps_server) ? caps_server : "");
    new_caps_req = weechat_hook_modifier_exec ("irc_cap_sync_req",
                                               modifier_data,
                                               caps_req);

    /* no changes in new caps requested */
    if (new_caps_req && (strcmp (caps_req, new_caps_req) == 0))
    {
        free (new_caps_req);
        new_caps_req = NULL;
    }

    /* caps not dropped? */
    if (!new_caps_req || new_caps_req[0])
    {
        ptr_caps_req = (new_caps_req) ? new_caps_req : caps_req;
        weechat_printf (
            server->buffer,
            _("%s%s: client capability, requesting: %s"),
            weechat_prefix ("network"), IRC_PLUGIN_NAME,
            ptr_caps_req);
        irc_server_sendf (server, 0, NULL, "CAP REQ :%s", ptr_caps_req);
    }

    if (new_caps_req)
        free (new_caps_req);
}

/*
 * Synchronizes requested capabilities for IRC server.
 */

void
irc_protocol_cap_sync (struct t_irc_server *server, int sasl)
{
    char *str_caps_server, **caps_server, *caps_to_enable;
    char **list_caps_to_enable, **cap_req;
    const char *ptr_caps_server, *ptr_cap_option;
    int sasl_requested, sasl_to_do, sasl_fail;
    int i, num_caps_server;

    str_caps_server = NULL;

    sasl_requested = (sasl) ? irc_server_sasl_enabled (server) : 0;
    sasl_to_do = 0;

    ptr_cap_option = IRC_SERVER_OPTION_STRING(
        server,
        IRC_SERVER_OPTION_CAPABILITIES);

    cap_req = weechat_string_dyn_alloc (128);

    caps_to_enable = irc_protocol_cap_to_enable (ptr_cap_option,
                                                 sasl_requested);
    list_caps_to_enable = weechat_string_split (
        caps_to_enable,
        ",",
        NULL,
        WEECHAT_STRING_SPLIT_STRIP_LEFT
        | WEECHAT_STRING_SPLIT_STRIP_RIGHT
        | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
        0,
        NULL);
    if (list_caps_to_enable)
    {
        ptr_caps_server = weechat_hashtable_get_string (server->cap_ls, "keys");
        str_caps_server = (ptr_caps_server) ?
            weechat_string_replace (ptr_caps_server, ",", " ") : NULL;
        caps_server = weechat_string_split (
            (ptr_caps_server) ? ptr_caps_server : "",
            ",",
            NULL,
            WEECHAT_STRING_SPLIT_STRIP_LEFT
            | WEECHAT_STRING_SPLIT_STRIP_RIGHT
            | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
            0,
            &num_caps_server);
        if (caps_server)
        {
            for (i = 0; i < num_caps_server; i++)
            {
                if (!weechat_hashtable_has_key (server->cap_list,
                                                caps_server[i])
                    && weechat_string_match_list (caps_server[i],
                                                  (const char **)list_caps_to_enable,
                                                  0))
                {
                    if (sasl && strcmp (caps_server[i], "sasl") == 0)
                        sasl_to_do = 1;
                    if (*cap_req[0])
                        weechat_string_dyn_concat (cap_req, " ", -1);
                    weechat_string_dyn_concat (cap_req, caps_server[i], -1);
                }
            }
            weechat_string_free_split (caps_server);
        }

        irc_protocol_cap_sync_req (server, str_caps_server, *cap_req);

        if (sasl)
        {
            if (!sasl_to_do)
                irc_server_sendf (server, 0, NULL, "CAP END");
            if (sasl_requested && !sasl_to_do)
            {
                weechat_printf (
                    server->buffer,
                    _("%s%s: client capability: SASL not supported"),
                    weechat_prefix ("network"), IRC_PLUGIN_NAME);

                if (weechat_config_boolean (irc_config_network_sasl_fail_unavailable))
                {
                    /* same handling as for sasl_end_fail */
                    sasl_fail = IRC_SERVER_OPTION_INTEGER(server, IRC_SERVER_OPTION_SASL_FAIL);
                    if ((sasl_fail == IRC_SERVER_SASL_FAIL_RECONNECT)
                        || (sasl_fail == IRC_SERVER_SASL_FAIL_DISCONNECT))
                    {
                        irc_server_disconnect (
                            server, 0,
                            (sasl_fail == IRC_SERVER_SASL_FAIL_RECONNECT) ? 1 : 0);
                    }
                }
            }
        }
        weechat_string_free_split (list_caps_to_enable);
    }

    if (str_caps_server)
        free (str_caps_server);
    if (caps_to_enable)
        free (caps_to_enable);
    if (cap_req)
        weechat_string_dyn_free (cap_req, 1);
}

/*
 * Callback for the IRC command "CAP": client capability.
 *
 * Command looks like:
 *   CAP * LS :identify-msg multi-prefix sasl
 *   CAP * LIST :identify-msg multi-prefix
 *   CAP * ACK :identify-msg
 *   CAP * NAK :multi-prefix
 *   CAP * NEW :batch
 *   CAP * DEL :identify-msg multi-prefix
 */

IRC_PROTOCOL_CALLBACK(cap)
{
    char **caps_supported, **caps_added, **caps_removed;
    char **caps_enabled, *pos_value, *str_name, **str_caps;
    char str_msg_auth[512], *str_msg_auth_upper, **str_caps_enabled;
    char **str_caps_disabled, *str_params;
    int arg_caps, num_caps_supported, num_caps_added, num_caps_removed;
    int num_caps_enabled, sasl_to_do, sasl_mechanism;
    int i, j, timeout, last_reply;

    IRC_PROTOCOL_MIN_PARAMS(2);

    if (strcmp (params[1], "LS") == 0)
    {
        /* list of capabilities supported by the server */
        if (num_params < 3)
            return WEECHAT_RC_OK;

        if ((num_params > 3) && (strcmp (params[2], "*") == 0))
        {
            arg_caps = 3;
            last_reply = 0;
        }
        else
        {
            arg_caps = 2;
            last_reply = 1;
        }

        if (!server->checking_cap_ls)
        {
            weechat_hashtable_remove_all (server->cap_ls);
            server->checking_cap_ls = 1;
        }

        if (last_reply)
            server->checking_cap_ls = 0;

        for (i = arg_caps; i < num_params; i++)
        {
            caps_supported = weechat_string_split (
                params[i],
                " ",
                NULL,
                WEECHAT_STRING_SPLIT_STRIP_LEFT
                | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                0,
                &num_caps_supported);
            if (!caps_supported)
                continue;
            for (j = 0; j < num_caps_supported; j++)
            {
                pos_value = strstr (caps_supported[j], "=");
                if (pos_value)
                {
                    str_name = weechat_strndup (caps_supported[j],
                                                pos_value - caps_supported[j]);
                    if (str_name)
                    {
                        weechat_hashtable_set (server->cap_ls,
                                               str_name, pos_value + 1);
                        if (strcmp (str_name, "draft/multiline") == 0)
                        {
                            irc_message_parse_cap_multiline_value (
                                server, pos_value + 1);
                        }
                        free (str_name);
                    }
                }
                else
                {
                    weechat_hashtable_set (server->cap_ls,
                                           caps_supported[j], NULL);
                }
            }
            weechat_string_free_split (caps_supported);
        }

        if (last_reply)
        {
            str_caps = weechat_string_dyn_alloc (128);
            weechat_hashtable_map_string (server->cap_ls,
                                          irc_protocol_cap_print_cb,
                                          str_caps);
            weechat_printf_date_tags (
                server->buffer, date, "irc_cap,log3",
                _("%s%s: client capability, server supports: %s"),
                weechat_prefix ("network"),
                IRC_PLUGIN_NAME,
                *str_caps);
            weechat_string_dyn_free (str_caps, 1);
        }

        /* auto-enable capabilities only when connecting to server */
        if (last_reply && !server->is_connected)
            irc_protocol_cap_sync (server, 1);
    }
    else if (strcmp (params[1], "LIST") == 0)
    {
        /* list of capabilities currently enabled */
        if (num_params < 3)
            return WEECHAT_RC_OK;

        if ((num_params > 3) && (strcmp (params[2], "*") == 0))
        {
            arg_caps = 3;
            last_reply = 0;
        }
        else
        {
            arg_caps = 2;
            last_reply = 1;
        }

        if (!server->checking_cap_list)
        {
            weechat_hashtable_remove_all (server->cap_list);
            irc_server_buffer_set_input_multiline (server, 0);
            server->checking_cap_list = 1;
        }

        if (last_reply)
            server->checking_cap_list = 0;

        for (i = arg_caps; i < num_params; i++)
        {
            caps_enabled = weechat_string_split (
                params[i],
                " ",
                NULL,
                WEECHAT_STRING_SPLIT_STRIP_LEFT
                | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                0,
                &num_caps_enabled);
            if (!caps_enabled)
                continue;
            for (j = 0; j < num_caps_enabled; j++)
            {
                pos_value = strstr (caps_enabled[j], "=");
                if (pos_value)
                {
                    str_name = weechat_strndup (caps_enabled[j],
                                                pos_value - caps_enabled[j]);
                    if (str_name)
                    {
                        weechat_hashtable_set (server->cap_list,
                                               str_name, pos_value + 1);
                        if (strcmp (str_name, "draft/multiline") == 0)
                            irc_server_buffer_set_input_multiline (server, 1);
                        free (str_name);
                    }
                }
                else
                {
                    weechat_hashtable_set (server->cap_list,
                                           caps_enabled[j], NULL);
                    if (strcmp (caps_enabled[j], "draft/multiline") == 0)
                        irc_server_buffer_set_input_multiline (server, 1);
                }
            }
            weechat_string_free_split (caps_enabled);
        }

        if (last_reply)
        {
            str_caps = weechat_string_dyn_alloc (128);
            weechat_hashtable_map_string (server->cap_list,
                                          irc_protocol_cap_print_cb,
                                          str_caps);
            weechat_printf_date_tags (
                server->buffer, date, "irc_cap,log3",
                _("%s%s: client capability, currently enabled: %s"),
                weechat_prefix ("network"),
                IRC_PLUGIN_NAME,
                *str_caps);
            weechat_string_dyn_free (str_caps, 1);
        }
    }
    else if (strcmp (params[1], "ACK") == 0)
    {
        /* capabilities acknowledged */
        if (num_params < 3)
            return WEECHAT_RC_OK;

        sasl_to_do = 0;
        str_caps_enabled = weechat_string_dyn_alloc (128);
        str_caps_disabled = weechat_string_dyn_alloc (128);

        for (i = 2; i < num_params; i++)
        {
            caps_supported = weechat_string_split (
                params[i],
                " ",
                NULL,
                WEECHAT_STRING_SPLIT_STRIP_LEFT
                | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                0,
                &num_caps_supported);
            if (!caps_supported)
                continue;
            for (j = 0; j < num_caps_supported; j++)
            {
                if (caps_supported[j][0] == '-')
                {
                    if (*str_caps_disabled[0])
                        weechat_string_dyn_concat (str_caps_disabled, " ", -1);
                    weechat_string_dyn_concat (str_caps_disabled,
                                               caps_supported[j] + 1,
                                               -1);
                    weechat_hashtable_remove (server->cap_list,
                                              caps_supported[j] + 1);
                    if (strcmp (caps_supported[j] + 1, "draft/multiline") == 0)
                        irc_server_buffer_set_input_multiline (server, 0);
                }
                else
                {
                    if (*str_caps_enabled[0])
                        weechat_string_dyn_concat (str_caps_enabled, " ", -1);
                    weechat_string_dyn_concat (str_caps_enabled,
                                               caps_supported[j],
                                               -1);
                    weechat_hashtable_set (server->cap_list,
                                           caps_supported[j], NULL);
                    if (strcmp (caps_supported[j], "draft/multiline") == 0)
                        irc_server_buffer_set_input_multiline (server, 1);
                    if (strcmp (caps_supported[j], "sasl") == 0)
                        sasl_to_do = 1;
                }
            }
            weechat_string_free_split (caps_supported);
        }
        if (*str_caps_enabled[0] && *str_caps_disabled[0])
        {
            weechat_printf_date_tags (
                server->buffer, date, "irc_cap,log3",
                _("%s%s: client capability, enabled: %s, disabled: %s"),
                weechat_prefix ("network"), IRC_PLUGIN_NAME,
                *str_caps_enabled, *str_caps_disabled);
        }
        else if (*str_caps_enabled[0])
        {
            weechat_printf_date_tags (
                server->buffer, date, "irc_cap,log3",
                _("%s%s: client capability, enabled: %s"),
                weechat_prefix ("network"), IRC_PLUGIN_NAME,
                *str_caps_enabled);
        }
        else if (*str_caps_disabled[0])
        {
            weechat_printf_date_tags (
                server->buffer, date, "irc_cap,log3",
                _("%s%s: client capability, disabled: %s"),
                weechat_prefix ("network"), IRC_PLUGIN_NAME,
                *str_caps_disabled);
        }
        weechat_string_dyn_free (str_caps_enabled, 1);
        weechat_string_dyn_free (str_caps_disabled, 1);

        if (sasl_to_do)
        {
            sasl_mechanism = IRC_SERVER_OPTION_INTEGER(
                server, IRC_SERVER_OPTION_SASL_MECHANISM);
            if ((sasl_mechanism >= 0)
                && (sasl_mechanism < IRC_NUM_SASL_MECHANISMS))
            {
                snprintf (str_msg_auth, sizeof (str_msg_auth),
                          "AUTHENTICATE %s",
                          irc_sasl_mechanism_string[sasl_mechanism]);
                str_msg_auth_upper = weechat_string_toupper (str_msg_auth);
                if (str_msg_auth_upper)
                {
                    irc_server_sendf (server, 0, NULL, str_msg_auth_upper);
                    free (str_msg_auth_upper);
                }
                if (server->hook_timer_sasl)
                    weechat_unhook (server->hook_timer_sasl);
                timeout = IRC_SERVER_OPTION_INTEGER(
                    server, IRC_SERVER_OPTION_SASL_TIMEOUT);
                server->hook_timer_sasl = weechat_hook_timer (
                    timeout * 1000,
                    0, 1,
                    &irc_server_timer_sasl_cb,
                    server, NULL);
            }
        }
    }
    else if (strcmp (params[1], "NAK") == 0)
    {
        /* capabilities rejected */
        if (num_params < 3)
            return WEECHAT_RC_OK;

        str_params = irc_protocol_string_params (params, 2, num_params - 1);
        weechat_printf_date_tags (
            server->buffer, date, "irc_cap,log3",
            _("%s%s: client capability, refused: %s"),
            weechat_prefix ("error"), IRC_PLUGIN_NAME, str_params);
        if (str_params)
            free (str_params);
        if (!server->is_connected)
            irc_server_sendf (server, 0, NULL, "CAP END");
    }
    else if (strcmp (params[1], "NEW") == 0)
    {
        /* new capabilities available */
        if (num_params < 3)
            return WEECHAT_RC_OK;

        str_params = irc_protocol_string_params (params, 2, num_params - 1);
        weechat_printf_date_tags (
            server->buffer, date, "irc_cap,log3",
            _("%s%s: client capability, now available: %s"),
            weechat_prefix ("network"), IRC_PLUGIN_NAME, str_params);
        if (str_params)
            free (str_params);
        for (i = 2; i < num_params; i++)
        {
            caps_added = weechat_string_split (
                params[i],
                " ",
                NULL,
                WEECHAT_STRING_SPLIT_STRIP_LEFT
                | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                0,
                &num_caps_added);
            if (!caps_added)
                continue;
            for (j = 0; j < num_caps_added; j++)
            {
                pos_value = strstr (caps_added[j], "=");
                if (pos_value)
                {
                    str_name = weechat_strndup (caps_added[j],
                                                pos_value - caps_added[j]);
                    if (str_name)
                    {
                        weechat_hashtable_set (server->cap_ls,
                                               str_name, pos_value + 1);
                        free (str_name);
                    }
                }
                else
                {
                    weechat_hashtable_set (server->cap_ls,
                                           caps_added[j], NULL);
                }
            }
            weechat_string_free_split (caps_added);
        }

        /* TODO: SASL Reauthentication */
        irc_protocol_cap_sync (server, 0);
    }
    else if (strcmp (params[1], "DEL") == 0)
    {
        /* capabilities no longer available */
        if (num_params < 3)
            return WEECHAT_RC_OK;

        str_params = irc_protocol_string_params (params, 2, num_params - 1);
        weechat_printf_date_tags (
            server->buffer, date, "irc_cap,log3",
            _("%s%s: client capability, removed: %s"),
            weechat_prefix ("network"), IRC_PLUGIN_NAME, str_params);
        if (str_params)
            free (str_params);
        for (i = 2; i < num_params; i++)
        {
            caps_removed = weechat_string_split (
                params[i],
                " ",
                NULL,
                WEECHAT_STRING_SPLIT_STRIP_LEFT
                | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                0,
                &num_caps_removed);
            if (!caps_removed)
                continue;
            for (j = 0; j < num_caps_removed; j++)
            {
                weechat_hashtable_remove (server->cap_ls, caps_removed[j]);
                weechat_hashtable_remove (server->cap_list, caps_removed[j]);
                if (strcmp (caps_removed[j], "draft/multiline") == 0)
                    irc_server_buffer_set_input_multiline (server, 0);
            }
            weechat_string_free_split (caps_removed);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "CHGHOST": user/host change of a nick (with
 * capability "chghost"): https://ircv3.net/specs/extensions/chghost
 *
 * Command looks like:
 *   CHGHOST user new.host.goes.here
 *   CHGHOST newuser host
 *   CHGHOST newuser new.host.goes.here
 *   CHGHOST newuser :new.host.goes.here
 */

IRC_PROTOCOL_CALLBACK(chghost)
{
    int length, local_chghost, smart_filter;
    char *str_host, str_tags[512];
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick;
    struct t_irc_channel_speaking *ptr_nick_speaking;

    IRC_PROTOCOL_MIN_PARAMS(2);
    IRC_PROTOCOL_CHECK_NICK;

    local_chghost = (irc_server_strcasecmp (server, nick, server->nick) == 0);

    length = strlen (params[0]) + 1 + strlen (params[1]) + 1;
    str_host = malloc (length);
    if (!str_host)
    {
        weechat_printf (
            server->buffer,
            _("%s%s: not enough memory for \"%s\" command"),
            weechat_prefix ("error"), IRC_PLUGIN_NAME, "chghost");
        return WEECHAT_RC_OK;
    }
    snprintf (str_host, length, "%s@%s", params[0], params[1]);

    if (local_chghost)
        irc_server_set_host (server, str_host);

    for (ptr_channel = server->channels; ptr_channel;
         ptr_channel = ptr_channel->next_channel)
    {
        switch (ptr_channel->type)
        {
            case IRC_CHANNEL_TYPE_PRIVATE:
                if (!ignored
                    && (irc_server_strcasecmp (server,
                                               ptr_channel->name, nick) == 0))
                {
                    snprintf (str_tags, sizeof (str_tags),
                              "new_host_%s", str_host);
                    weechat_printf_date_tags (
                        irc_msgbuffer_get_target_buffer (
                            server, NULL, command, NULL, ptr_channel->buffer),
                        date,
                        irc_protocol_tags (server, command, tags, str_tags,
                                           nick, address),
                        _("%s%s%s%s (%s%s%s)%s has changed host to %s%s"),
                        weechat_prefix ("network"),
                        irc_nick_color_for_msg (server, 1, NULL, nick),
                        nick,
                        IRC_COLOR_CHAT_DELIMITERS,
                        IRC_COLOR_CHAT_HOST,
                        (address) ? address : "",
                        IRC_COLOR_CHAT_DELIMITERS,
                        IRC_COLOR_MESSAGE_CHGHOST,
                        IRC_COLOR_CHAT_HOST,
                        str_host);
                }
                break;
            case IRC_CHANNEL_TYPE_CHANNEL:
                ptr_nick = irc_nick_search (server, ptr_channel, nick);
                if (ptr_nick)
                {
                    if (!ignored)
                    {
                        ptr_nick_speaking = ((weechat_config_boolean (irc_config_look_smart_filter))
                                             && (weechat_config_boolean (irc_config_look_smart_filter_chghost))) ?
                            irc_channel_nick_speaking_time_search (server, ptr_channel, nick, 1) : NULL;
                        smart_filter = (!local_chghost
                                        && weechat_config_boolean (irc_config_look_smart_filter)
                                        && weechat_config_boolean (irc_config_look_smart_filter_chghost)
                                        && !ptr_nick_speaking);
                        snprintf (str_tags, sizeof (str_tags),
                                  "new_host_%s%s%s",
                                  str_host,
                                  (smart_filter) ? "," : "",
                                  (smart_filter) ? "irc_smart_filter" : "");
                        weechat_printf_date_tags (
                            irc_msgbuffer_get_target_buffer (
                                server, NULL, command, NULL, ptr_channel->buffer),
                            date,
                            irc_protocol_tags (server, command, tags, str_tags,
                                               nick, address),
                            _("%s%s%s%s (%s%s%s)%s has changed host to %s%s"),
                            weechat_prefix ("network"),
                            irc_nick_color_for_msg (server, 1, ptr_nick, nick),
                            nick,
                            IRC_COLOR_CHAT_DELIMITERS,
                            IRC_COLOR_CHAT_HOST,
                            (address) ? address : "",
                            IRC_COLOR_CHAT_DELIMITERS,
                            IRC_COLOR_MESSAGE_CHGHOST,
                            IRC_COLOR_CHAT_HOST,
                            str_host);
                    }
                    irc_nick_set_host (ptr_nick, str_host);
                }
                break;
        }
    }

    free (str_host);

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "ERROR".
 *
 * Command looks like:
 *   ERROR :Closing Link: irc.server.org (Bad Password)
 */

IRC_PROTOCOL_CALLBACK(error)
{
    char *str_error;

    IRC_PROTOCOL_MIN_PARAMS(1);

    str_error = irc_protocol_string_params (params, 0, num_params - 1);

    weechat_printf_date_tags (
        irc_msgbuffer_get_target_buffer (server, NULL, command, NULL, NULL),
        date,
        irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
        "%s%s",
        weechat_prefix ("error"),
        str_error);

    if (str_error && (strncmp (str_error, "Closing Link", 12) == 0))
        irc_server_disconnect (server, !server->is_connected, 1);

    if (str_error)
        free (str_error);

    return WEECHAT_RC_OK;
}

/*
 * Callback for an IRC error command (used by many error messages, but not for
 * command "ERROR").
 *
 * Command looks like:
 *   401 nick nick2 :No such nick/channel
 *   402 nick server :No such server
 *   404 nick #channel :Cannot send to channel
 */

IRC_PROTOCOL_CALLBACK(generic_error)
{
    int arg_error, force_server_buffer;
    char *str_error, str_target[512];
    const char *pos_channel, *pos_nick;
    struct t_irc_channel *ptr_channel;
    struct t_gui_buffer *ptr_buffer;

    IRC_PROTOCOL_MIN_PARAMS(2);

    arg_error = (irc_server_strcasecmp (server, params[0], server->nick) == 0) ?
        1 : 0;

    pos_channel = NULL;
    ptr_channel = NULL;
    pos_nick = NULL;
    str_target[0] = '\0';

    if (params[arg_error + 1])
    {
        /*
         * force display on server buffer for these messages:
         *   - 432: erroneous nickname
         *   - 433: nickname already in use
         *   - 437: nick/channel temporarily unavailable
         */
        force_server_buffer = ((strcmp (command, "432") == 0)
                               || (strcmp (command, "433") == 0)
                               || (strcmp (command, "437") == 0));
        if (!force_server_buffer
            && irc_channel_is_channel (server, params[arg_error]))
        {
            pos_channel = params[arg_error];
            ptr_channel = irc_channel_search (server, pos_channel);
            snprintf (str_target, sizeof (str_target),
                      "%s%s%s: ",
                      IRC_COLOR_CHAT_CHANNEL,
                      pos_channel,
                      IRC_COLOR_RESET);
            arg_error++;
        }
        else if (strcmp (params[arg_error], "*") != 0)
        {
            pos_nick = params[arg_error];
            snprintf (str_target, sizeof (str_target),
                      "%s%s%s: ",
                      irc_nick_color_for_msg (server, 1, NULL, pos_nick),
                      pos_nick,
                      IRC_COLOR_RESET);
            arg_error++;
        }
    }

    ptr_buffer = (ptr_channel) ? ptr_channel->buffer : server->buffer;

    str_error = irc_protocol_string_params (params, arg_error, num_params - 1);

    weechat_printf_date_tags (
        irc_msgbuffer_get_target_buffer (
            server, pos_nick, command,
            ((strcmp (command, "401") == 0)
             || (strcmp (command, "402") == 0)) ? "whois" : NULL,
            ptr_buffer),
        date,
        irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
        "%s%s%s",
        weechat_prefix ("network"),
        str_target,
        str_error);

    if (str_error)
        free (str_error);

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "FAIL".
 *
 * Command looks like:
 *   FAIL * NEED_REGISTRATION :You need to be registered to continue
 *   FAIL ACC REG_INVALID_CALLBACK REGISTER :Email address is not valid
 *   FAIL BOX BOXES_INVALID STACK CLOCKWISE :Given boxes are not supported
 */

IRC_PROTOCOL_CALLBACK(fail)
{
    IRC_PROTOCOL_MIN_PARAMS(2);

    irc_protocol_print_error_warning_msg (
        server,
        date,
        tags,
        command,
        weechat_prefix ("error"),
        _("Failure:"),
        params,
        num_params);

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "INVITE".
 *
 * Command looks like:
 *   INVITE mynick :#channel
 *
 * With invite-notify capability, the whole message looks like:
 *   :<inviter> INVITE <target> <channel>
 *   :ChanServ!ChanServ@example.com INVITE Attila #channel
 *
 * For more information, see:
 * https://ircv3.net/specs/extensions/invite-notify
 */

IRC_PROTOCOL_CALLBACK(invite)
{
    IRC_PROTOCOL_MIN_PARAMS(2);
    IRC_PROTOCOL_CHECK_NICK;

    if (ignored)
        return WEECHAT_RC_OK;

    if (irc_server_strcasecmp (server, params[0], server->nick) == 0)
    {
        weechat_printf_date_tags (
            irc_msgbuffer_get_target_buffer (server, nick, command, NULL, NULL),
            date,
            irc_protocol_tags (server, command, tags, "notify_highlight", nick,
                               address),
            _("%sYou have been invited to %s%s%s by %s%s%s"),
            weechat_prefix ("network"),
            IRC_COLOR_CHAT_CHANNEL,
            params[1],
            IRC_COLOR_RESET,
            irc_nick_color_for_msg (server, 1, NULL, nick),
            nick,
            IRC_COLOR_RESET);
    }
    else
    {
        /* CAP invite-notify */
        /* imitate numeric 341 output */
        weechat_printf_date_tags (
            irc_msgbuffer_get_target_buffer (server, nick, command, NULL, NULL),
            date,
            irc_protocol_tags (server, command, tags, NULL, nick, address),
            _("%s%s%s%s has invited %s%s%s to %s%s%s"),
            weechat_prefix ("network"),
            irc_nick_color_for_msg (server, 1, NULL, nick),
            nick,
            IRC_COLOR_RESET,
            irc_nick_color_for_msg (server, 1, NULL, params[0]),
            params[0],
            IRC_COLOR_RESET,
            IRC_COLOR_CHAT_CHANNEL,
            params[1],
            IRC_COLOR_RESET);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "JOIN".
 *
 * Command looks like:
 *   JOIN #channel
 *   JOIN :#channel
 *
 * With extended-join capability:
 *   JOIN #channel * :real name
 *   JOIN #channel account :real name
 *
 * For more information, see:
 * https://ircv3.net/specs/extensions/extended-join
 */

IRC_PROTOCOL_CALLBACK(join)
{
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick;
    struct t_irc_channel_speaking *ptr_nick_speaking;
    const char *pos_account, *pos_realname;
    char str_account[512], str_realname[512];
    int local_join, display_host, smart_filter;

    IRC_PROTOCOL_MIN_PARAMS(1);
    IRC_PROTOCOL_CHECK_NICK;

    local_join = (irc_server_strcasecmp (server, nick, server->nick) == 0);

    pos_account = ((num_params > 1) && (strcmp (params[1], "*") != 0)) ?
        params[1] : NULL;
    pos_realname = (num_params > 2) ? params[2] : NULL;

    str_account[0] = '\0';
    if (pos_account
        && weechat_config_boolean (irc_config_look_display_extended_join))
    {
        snprintf (str_account, sizeof (str_account),
                  "%s [%s%s%s]",
                  IRC_COLOR_CHAT_DELIMITERS,
                  IRC_COLOR_CHAT_HOST,
                  pos_account,
                  IRC_COLOR_CHAT_DELIMITERS);
    }

    str_realname[0] = '\0';
    if (pos_realname
        && weechat_config_boolean (irc_config_look_display_extended_join))
    {
        snprintf (str_realname, sizeof (str_realname),
                  "%s (%s%s%s)",
                  IRC_COLOR_CHAT_DELIMITERS,
                  IRC_COLOR_CHAT_HOST,
                  pos_realname,
                  IRC_COLOR_CHAT_DELIMITERS);
    }

    ptr_channel = irc_channel_search (server, params[0]);
    if (ptr_channel)
    {
        ptr_channel->part = 0;
    }
    else
    {
        /*
         * if someone else joins and channel is not opened, then just
         * ignore it (we should receive our self join first)
         */
        if (!local_join)
            return WEECHAT_RC_OK;

        ptr_channel = irc_channel_new (server, IRC_CHANNEL_TYPE_CHANNEL,
                                       params[0], 1, 1);
        if (!ptr_channel)
        {
            weechat_printf (server->buffer,
                            _("%s%s: cannot create new channel \"%s\""),
                            weechat_prefix ("error"), IRC_PLUGIN_NAME,
                            params[0]);
            return WEECHAT_RC_OK;
        }
    }

    /*
     * local join? clear nicklist to be sure it is empty (when using znc, after
     * reconnection to network, we receive a JOIN for channel with existing
     * nicks in irc plugin, so we need to clear the nicklist now)
     */
    if (local_join)
        irc_nick_free_all (server, ptr_channel);

    /* reset some variables if joining new channel */
    if (!ptr_channel->nicks)
    {
        irc_channel_set_topic (ptr_channel, NULL);
        if (ptr_channel->modes)
        {
            free (ptr_channel->modes);
            ptr_channel->modes = NULL;
        }
        ptr_channel->limit = 0;
        weechat_hashtable_remove_all (ptr_channel->join_msg_received);
        ptr_channel->checking_whox = 0;
    }

    /* add nick in channel */
    ptr_nick = irc_nick_new (server, ptr_channel, nick, address, NULL, 0,
                             (pos_account) ? pos_account : NULL,
                             (pos_realname) ? pos_realname : NULL);

    /* rename the nick if it was in list with a different case */
    irc_channel_nick_speaking_rename_if_present (server, ptr_channel, nick);

    if (!ignored)
    {
        ptr_nick_speaking = ((weechat_config_boolean (irc_config_look_smart_filter))
                             && (weechat_config_boolean (irc_config_look_smart_filter_join))) ?
            irc_channel_nick_speaking_time_search (server, ptr_channel, nick, 1) : NULL;
        display_host = (local_join) ?
            weechat_config_boolean (irc_config_look_display_host_join_local) :
            weechat_config_boolean (irc_config_look_display_host_join);

        /*
         * "smart" filter the join message if it's not a join from myself, if
         * smart filtering is enabled, and if nick was not speaking in channel
         */
        smart_filter = (!local_join
                        && weechat_config_boolean (irc_config_look_smart_filter)
                        && weechat_config_boolean (irc_config_look_smart_filter_join)
                        && !ptr_nick_speaking);

        /* display the join */
        weechat_printf_date_tags (
            irc_msgbuffer_get_target_buffer (server, NULL, command, NULL,
                                             ptr_channel->buffer),
            date,
            irc_protocol_tags (server,
                               command,
                               tags,
                               (smart_filter) ? "irc_smart_filter" : NULL,
                               nick, address),
            _("%s%s%s%s%s%s%s%s%s%s%s%s has joined %s%s%s"),
            weechat_prefix ("join"),
            irc_nick_color_for_msg (server, 1, ptr_nick, nick),
            nick,
            str_account,
            str_realname,
            IRC_COLOR_CHAT_DELIMITERS,
            (display_host) ? " (" : "",
            IRC_COLOR_CHAT_HOST,
            (display_host) ? address : "",
            IRC_COLOR_CHAT_DELIMITERS,
            (display_host) ? ")" : "",
            IRC_COLOR_MESSAGE_JOIN,
            IRC_COLOR_CHAT_CHANNEL,
            params[0],
            IRC_COLOR_MESSAGE_JOIN);

        /*
         * if join is smart filtered, save the nick in hashtable, and if nick
         * is speaking shortly after the join, it will be unmasked
         * (option irc.look.smart_filter_join_unmask)
         */
        if (smart_filter)
        {
            irc_channel_join_smart_filtered_add (ptr_channel, nick,
                                                 time (NULL));
        }

        /* display message in private if private has flag "has_quit_server" */
        if (!local_join)
        {
            irc_channel_display_nick_back_in_pv (server, ptr_nick, nick);
            irc_channel_set_topic_private_buffers (server, ptr_nick, nick,
                                                   address);
        }
    }

    if (local_join)
    {
        irc_server_set_host (server, address);
        irc_bar_item_update_channel ();
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "KICK".
 *
 * Command looks like:
 *   KICK #channel nick :kick reason
 */

IRC_PROTOCOL_CALLBACK(kick)
{
    const char *pos_comment, *ptr_autorejoin;
    int rejoin;
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick, *ptr_nick_kicked;

    IRC_PROTOCOL_MIN_PARAMS(2);
    IRC_PROTOCOL_CHECK_NICK;

    pos_comment = (num_params > 2) ? params[2] : NULL;

    ptr_channel = irc_channel_search (server, params[0]);
    if (!ptr_channel)
        return WEECHAT_RC_OK;

    ptr_nick = irc_nick_search (server, ptr_channel, nick);
    ptr_nick_kicked = irc_nick_search (server, ptr_channel, params[1]);

    if (pos_comment)
    {
        weechat_printf_date_tags (
            irc_msgbuffer_get_target_buffer (server, NULL, command, NULL,
                                             ptr_channel->buffer),
            date,
            irc_protocol_tags (server, command, tags, NULL, NULL, address),
            _("%s%s%s%s has kicked %s%s%s %s(%s%s%s)"),
            weechat_prefix ("quit"),
            irc_nick_color_for_msg (server, 1, ptr_nick, nick),
            nick,
            IRC_COLOR_MESSAGE_KICK,
            irc_nick_color_for_msg (server, 1, ptr_nick_kicked, params[1]),
            params[1],
            IRC_COLOR_MESSAGE_KICK,
            IRC_COLOR_CHAT_DELIMITERS,
            IRC_COLOR_REASON_KICK,
            pos_comment,
            IRC_COLOR_CHAT_DELIMITERS);
    }
    else
    {
        weechat_printf_date_tags (
            irc_msgbuffer_get_target_buffer (server, NULL, command, NULL,
                                             ptr_channel->buffer),
            date,
            irc_protocol_tags (server, command, tags, NULL, NULL, address),
            _("%s%s%s%s has kicked %s%s%s"),
            weechat_prefix ("quit"),
            irc_nick_color_for_msg (server, 1, ptr_nick, nick),
            nick,
            IRC_COLOR_MESSAGE_KICK,
            irc_nick_color_for_msg (server, 1, ptr_nick_kicked, params[1]),
            params[1],
            IRC_COLOR_MESSAGE_KICK);
    }

    if (irc_server_strcasecmp (server, params[1], server->nick) == 0)
    {
        /*
         * my nick was kicked => free all nicks, channel is not active any
         * more
         */
        irc_nick_free_all (server, ptr_channel);

        irc_channel_modelist_set_state (ptr_channel,
                                        IRC_MODELIST_STATE_MODIFIED);

        /* read option "autorejoin" in server */
        rejoin = IRC_SERVER_OPTION_BOOLEAN(server, IRC_SERVER_OPTION_AUTOREJOIN);

        /*
         * if buffer has a local variable "autorejoin", use it
         * (it has higher priority than server option
         */
        ptr_autorejoin = weechat_buffer_get_string (ptr_channel->buffer,
                                                    "localvar_autorejoin");
        if (ptr_autorejoin)
            rejoin = weechat_config_string_to_boolean (ptr_autorejoin);

        if (rejoin)
        {
            if (IRC_SERVER_OPTION_INTEGER(server,
                                          IRC_SERVER_OPTION_AUTOREJOIN_DELAY) == 0)
            {
                /* immediately rejoin if delay is 0 */
                irc_channel_rejoin (server, ptr_channel, 0, 1);
            }
            else
            {
                /* rejoin channel later, according to delay */
                ptr_channel->hook_autorejoin =
                    weechat_hook_timer (
                        IRC_SERVER_OPTION_INTEGER(server,
                                                  IRC_SERVER_OPTION_AUTOREJOIN_DELAY) * 1000,
                        0, 1,
                        &irc_channel_autorejoin_cb,
                        ptr_channel, NULL);
            }
        }

        irc_bar_item_update_channel ();
    }
    else
    {
        /*
         * someone was kicked from channel (but not me) => remove only this
         * nick
         */
        if (ptr_nick_kicked)
            irc_nick_free (server, ptr_channel, ptr_nick_kicked);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "KILL".
 *
 * Command looks like:
 *   KILL nick :kill reason
 */

IRC_PROTOCOL_CALLBACK(kill)
{
    const char *pos_comment;
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick, *ptr_nick_killed;

    IRC_PROTOCOL_MIN_PARAMS(1);
    IRC_PROTOCOL_CHECK_NICK;

    pos_comment = (num_params > 1) ? params[1] : NULL;

    for (ptr_channel = server->channels; ptr_channel;
         ptr_channel = ptr_channel->next_channel)
    {
        ptr_nick = irc_nick_search (server, ptr_channel, nick);
        ptr_nick_killed = irc_nick_search (server, ptr_channel, params[0]);

        if (pos_comment)
        {
            weechat_printf_date_tags (
                irc_msgbuffer_get_target_buffer (server, NULL, command, NULL,
                                                 ptr_channel->buffer),
                date,
                irc_protocol_tags (server, command, tags, NULL, NULL, address),
                _("%s%sYou were killed by %s%s%s %s(%s%s%s)"),
                weechat_prefix ("quit"),
                IRC_COLOR_MESSAGE_KICK,
                irc_nick_color_for_msg (server, 1, ptr_nick, nick),
                nick,
                IRC_COLOR_MESSAGE_KICK,
                IRC_COLOR_CHAT_DELIMITERS,
                IRC_COLOR_REASON_KICK,
                pos_comment,
                IRC_COLOR_CHAT_DELIMITERS);
        }
        else
        {
            weechat_printf_date_tags (
                irc_msgbuffer_get_target_buffer (server, NULL, command, NULL,
                                                 ptr_channel->buffer),
                date,
                irc_protocol_tags (server, command, tags, NULL, NULL, address),
                _("%s%sYou were killed by %s%s%s"),
                weechat_prefix ("quit"),
                IRC_COLOR_MESSAGE_KICK,
                irc_nick_color_for_msg (server, 1, ptr_nick, nick),
                nick,
                IRC_COLOR_MESSAGE_KICK);
        }

        if (irc_server_strcasecmp (server, params[0], server->nick) == 0)
        {
            /*
             * my nick was killed => free all nicks, channel is not active any
             * more
             */
            irc_nick_free_all (server, ptr_channel);

            irc_channel_modelist_set_state (ptr_channel,
                                            IRC_MODELIST_STATE_MODIFIED);

            irc_bar_item_update_channel ();
        }
        else
        {
            /*
             * someone was killed on channel (but not me) => remove only this
             * nick
             */
            if (ptr_nick_killed)
                irc_nick_free (server, ptr_channel, ptr_nick_killed);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for an IRC KNOCK reply.
 *
 * Commands look like:
 *   711 nick #channel :Your KNOCK has been delivered.
 *   712 nick #channel :Too many KNOCKs (channel).
 *   713 nick #channel :Channel is open.
 *   714 nick #channel :You are already on that channel.
 */

IRC_PROTOCOL_CALLBACK(knock_reply)
{
    char *str_message;

    IRC_PROTOCOL_MIN_PARAMS(3);

    str_message = irc_protocol_string_params (params, 2, num_params - 1);

    weechat_printf_date_tags (
        irc_msgbuffer_get_target_buffer (server, params[0], command, NULL, NULL),
        date,
        irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
        "%s%s%s%s: %s",
        weechat_prefix ("network"),
        IRC_COLOR_CHAT_CHANNEL,
        params[1],
        IRC_COLOR_RESET,
        str_message);

    if (str_message)
        free (str_message);

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "MODE".
 *
 * Command looks like:
 *   MODE #test +nt
 *   MODE #test +o nick
 *   MODE #test +o :nick
 */

IRC_PROTOCOL_CALLBACK(mode)
{
    char *msg_modes_args, *modes_args;
    int smart_filter, local_mode;
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick;
    struct t_gui_buffer *ptr_buffer;

    IRC_PROTOCOL_MIN_PARAMS(2);
    IRC_PROTOCOL_CHECK_NICK;

    msg_modes_args = (num_params > 2) ?
        irc_protocol_string_params (params, 2, num_params - 1) : NULL;

    if (irc_channel_is_channel (server, params[0]))
    {
        smart_filter = 0;
        ptr_channel = irc_channel_search (server, params[0]);
        if (ptr_channel)
        {
            smart_filter = irc_mode_channel_set (server, ptr_channel, host,
                                                 params[1], msg_modes_args);
        }
        local_mode = (irc_server_strcasecmp (server, nick, server->nick) == 0);
        ptr_nick = irc_nick_search (server, ptr_channel, nick);
        ptr_buffer = (ptr_channel) ? ptr_channel->buffer : server->buffer;
        modes_args = irc_mode_get_arguments (msg_modes_args);
        weechat_printf_date_tags (
            irc_msgbuffer_get_target_buffer (server, NULL, command, NULL,
                                             ptr_buffer),
            date,
            irc_protocol_tags (server,
                               command,
                               tags,
                               (smart_filter && !local_mode) ?
                               "irc_smart_filter" : NULL,
                               NULL, address),
            _("%sMode %s%s %s[%s%s%s%s%s]%s by %s%s"),
            weechat_prefix ("network"),
            IRC_COLOR_CHAT_CHANNEL,
            (ptr_channel) ? ptr_channel->name : params[0],
            IRC_COLOR_CHAT_DELIMITERS,
            IRC_COLOR_RESET,
            params[1],
            (modes_args && modes_args[0]) ? " " : "",
            (modes_args && modes_args[0]) ? modes_args : "",
            IRC_COLOR_CHAT_DELIMITERS,
            IRC_COLOR_RESET,
            irc_nick_color_for_msg (server, 1, ptr_nick, nick),
            nick);
        if (modes_args)
            free (modes_args);
    }
    else
    {
        weechat_printf_date_tags (
            irc_msgbuffer_get_target_buffer (server, NULL, command, NULL, NULL),
            date,
            irc_protocol_tags (server, command, tags, NULL, NULL, address),
            _("%sUser mode %s[%s%s%s]%s by %s%s"),
            weechat_prefix ("network"),
            IRC_COLOR_CHAT_DELIMITERS,
            IRC_COLOR_RESET,
            params[1],
            IRC_COLOR_CHAT_DELIMITERS,
            IRC_COLOR_RESET,
            irc_nick_color_for_msg (server, 1, NULL, nick),
            nick);
        irc_mode_user_set (server, params[1], 0);
    }

    if (msg_modes_args)
        free (msg_modes_args);

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "NICK".
 *
 * Command looks like:
 *   NICK :newnick
 */

IRC_PROTOCOL_CALLBACK(nick)
{
    struct t_irc_channel *ptr_channel, *ptr_channel_new_nick;
    struct t_irc_nick *ptr_nick, *ptr_nick_found;
    char *old_color, *new_color, str_tags[512];
    int local_nick, smart_filter;
    struct t_irc_channel_speaking *ptr_nick_speaking;

    IRC_PROTOCOL_MIN_PARAMS(1);
    IRC_PROTOCOL_CHECK_NICK;

    if (!params[0][0])
        return WEECHAT_RC_OK;

    local_nick = (irc_server_strcasecmp (server, nick, server->nick) == 0) ?
        1 : 0;

    if (local_nick)
    {
        irc_server_set_nick (server, params[0]);
        irc_server_set_host (server, address);
    }

    ptr_nick_found = NULL;

    /* first display message in server buffer if it's local nick */
    if (local_nick)
    {
        /* temporary disable hotlist */
        weechat_buffer_set (NULL, "hotlist", "-");

        snprintf (str_tags, sizeof (str_tags),
                  "irc_nick1_%s,irc_nick2_%s",
                  nick,
                  params[0]);
        weechat_printf_date_tags (
            server->buffer,
            date,
            irc_protocol_tags (server, command, tags, str_tags, NULL, address),
            _("%sYou are now known as %s%s%s"),
            weechat_prefix ("network"),
            IRC_COLOR_CHAT_NICK_SELF,
            params[0],
            IRC_COLOR_RESET);

        /* enable hotlist */
        weechat_buffer_set (NULL, "hotlist", "+");
    }

    ptr_channel_new_nick = irc_channel_search (server, params[0]);

    for (ptr_channel = server->channels; ptr_channel;
         ptr_channel = ptr_channel->next_channel)
    {
        switch (ptr_channel->type)
        {
            case IRC_CHANNEL_TYPE_PRIVATE:
                /*
                 * rename private buffer if this is with "old nick"
                 * or if it's with "new nick" but different case
                 * (only if another buffer for the nick doesn't exist)
                 */
                if ((!ptr_channel_new_nick
                     || (ptr_channel_new_nick == ptr_channel))
                    && ((irc_server_strcasecmp (server,
                                                ptr_channel->name, nick) == 0)
                        || ((irc_server_strcasecmp (server,
                                                    ptr_channel->name, params[0]) == 0)
                            && (strcmp (ptr_channel->name, params[0]) != 0))))
                {
                    /* rename private buffer */
                    irc_channel_pv_rename (server, ptr_channel, params[0]);

                    /* display message */
                    if (weechat_config_boolean (irc_config_look_display_pv_nick_change))
                    {
                        if (weechat_config_boolean (irc_config_look_color_nicks_in_server_messages))
                        {
                            if (weechat_config_boolean (irc_config_look_color_pv_nick_like_channel))
                            {
                                old_color = irc_nick_find_color (nick);
                                new_color = irc_nick_find_color (params[0]);
                            }
                            else
                            {
                                old_color = strdup (IRC_COLOR_CHAT_NICK_OTHER);
                                new_color = strdup (IRC_COLOR_CHAT_NICK_OTHER);
                            }
                        }
                        else
                        {
                            old_color = strdup (IRC_COLOR_CHAT_NICK);
                            new_color = strdup (IRC_COLOR_CHAT_NICK);
                        }
                        snprintf (str_tags, sizeof (str_tags),
                                  "irc_nick1_%s,irc_nick2_%s",
                                  nick,
                                  params[0]);
                        weechat_printf_date_tags (
                            ptr_channel->buffer,
                            date,
                            irc_protocol_tags (server, command, tags, str_tags,
                                               NULL, address),
                            _("%s%s%s%s is now known as %s%s%s"),
                            weechat_prefix ("network"),
                            old_color,
                            nick,
                            IRC_COLOR_RESET,
                            new_color,
                            params[0],
                            IRC_COLOR_RESET);
                        if (old_color)
                            free (old_color);
                        if (new_color)
                            free (new_color);
                    }
                }
                break;
            case IRC_CHANNEL_TYPE_CHANNEL:
                /* rename nick in nicklist if found */
                ptr_nick = irc_nick_search (server, ptr_channel, nick);
                if (ptr_nick)
                {
                    ptr_nick_found = ptr_nick;

                    /* set host in nick if needed */
                    irc_nick_set_host (ptr_nick, address);

                    /* change nick and display message on channel */
                    old_color = strdup (ptr_nick->color);
                    irc_nick_change (server, ptr_channel, ptr_nick, params[0]);
                    if (local_nick)
                    {
                        /* temporary disable hotlist */
                        weechat_buffer_set (NULL, "hotlist", "-");

                        snprintf (str_tags, sizeof (str_tags),
                                  "irc_nick1_%s,irc_nick2_%s",
                                  nick,
                                  params[0]);
                        weechat_printf_date_tags (ptr_channel->buffer,
                                                  date,
                                                  irc_protocol_tags (server,
                                                                     command,
                                                                     tags,
                                                                     str_tags,
                                                                     NULL,
                                                                     address),
                                                  _("%sYou are now known as "
                                                    "%s%s%s"),
                                                  weechat_prefix ("network"),
                                                  IRC_COLOR_CHAT_NICK_SELF,
                                                  params[0],
                                                  IRC_COLOR_RESET);

                        /* enable hotlist */
                        weechat_buffer_set (NULL, "hotlist", "+");
                    }
                    else
                    {
                        if (!irc_ignore_check (server, ptr_channel->name,
                                               nick, host))
                        {
                            ptr_nick_speaking = ((weechat_config_boolean (irc_config_look_smart_filter))
                                                 && (weechat_config_boolean (irc_config_look_smart_filter_nick))) ?
                                irc_channel_nick_speaking_time_search (server, ptr_channel, nick, 1) : NULL;
                            smart_filter = (weechat_config_boolean (irc_config_look_smart_filter)
                                            && weechat_config_boolean (irc_config_look_smart_filter_nick)
                                            && !ptr_nick_speaking);
                            snprintf (str_tags, sizeof (str_tags),
                                      "%sirc_nick1_%s,irc_nick2_%s",
                                      (smart_filter) ? "irc_smart_filter," : "",
                                      nick,
                                      params[0]);
                            weechat_printf_date_tags (
                                ptr_channel->buffer,
                                date,
                                irc_protocol_tags (server, command, tags,
                                                   str_tags, NULL, address),
                                _("%s%s%s%s is now known as %s%s%s"),
                                weechat_prefix ("network"),
                                weechat_config_boolean (irc_config_look_color_nicks_in_server_messages) ?
                                old_color : IRC_COLOR_CHAT_NICK,
                                nick,
                                IRC_COLOR_RESET,
                                irc_nick_color_for_msg (server, 1, ptr_nick,
                                                        params[0]),
                                params[0],
                                IRC_COLOR_RESET);
                        }
                        irc_channel_nick_speaking_rename (ptr_channel,
                                                          nick, params[0]);
                        irc_channel_nick_speaking_time_rename (server,
                                                               ptr_channel,
                                                               nick, params[0]);
                        irc_channel_join_smart_filtered_rename (ptr_channel,
                                                                nick,
                                                                params[0]);
                    }

                    if (old_color)
                        free (old_color);
                }
                break;
        }
    }

    if (!local_nick)
    {
        irc_channel_display_nick_back_in_pv (server, ptr_nick_found, params[0]);
        irc_channel_set_topic_private_buffers (server, ptr_nick_found,
                                               params[0], address);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "NOTE".
 *
 * Command looks like:
 *   NOTE * OPER_MESSAGE :The message
 */

IRC_PROTOCOL_CALLBACK(note)
{
    IRC_PROTOCOL_MIN_PARAMS(2);

    irc_protocol_print_error_warning_msg (
        server,
        date,
        tags,
        command,
        weechat_prefix ("network"),
        _("Note:"),
        params,
        num_params);

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "NOTICE".
 *
 * Command looks like:
 *   NOTICE mynick :notice text
 *   NOTICE #channel :notice text
 *   NOTICE @#channel :notice text for channel ops
 */

IRC_PROTOCOL_CALLBACK(notice)
{
    char *notice_args, *pos, end_char, *channel, str_tags[1024];
    const char *pos_target, *pos_args, *nick_address;
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick;
    int notify_private, is_channel, is_channel_orig, nick_is_me, display_host;
    int cap_echo_message, msg_already_received;
    time_t time_now;
    struct t_gui_buffer *ptr_buffer;

    IRC_PROTOCOL_MIN_PARAMS(2);

    if (ignored)
        return WEECHAT_RC_OK;

    notice_args = irc_protocol_string_params (params, 1, num_params - 1);
    if (!notice_args)
        return WEECHAT_RC_ERROR;

    nick_is_me = (irc_server_strcasecmp (server, server->nick, nick) == 0);

    pos_args = notice_args;

    pos_target = params[0];
    is_channel = irc_channel_is_channel (server, pos_target + 1);
    if (is_channel
        && irc_server_prefix_char_statusmsg (server, pos_target[0]))
    {
        pos_target++;
    }

    if (nick && (pos_args[0] == '\01'))
    {
        cap_echo_message = weechat_hashtable_has_key (server->cap_list,
                                                      "echo-message");
        msg_already_received = weechat_hashtable_has_key (
            server->echo_msg_recv, irc_message);
        if (!msg_already_received && cap_echo_message)
        {
            time_now = time (NULL);
            weechat_hashtable_set (server->echo_msg_recv,
                                   irc_message, &time_now);
        }
        if (!cap_echo_message || !msg_already_received)
        {
            irc_ctcp_display_reply_from_nick (server, date, tags, command, nick,
                                              address, pos_args);
        }
        if (msg_already_received)
            weechat_hashtable_remove (server->echo_msg_recv, irc_message);
    }
    else
    {
        channel = NULL;
        is_channel = irc_channel_is_channel (server, pos_target);
        is_channel_orig = is_channel;
        if (is_channel)
            channel = strdup (pos_target);
        else if (weechat_config_boolean (irc_config_look_notice_welcome_redirect))
        {
            end_char = ' ';
            switch (pos_args[0])
            {
                case '[':
                    end_char = ']';
                    break;
                case '(':
                    end_char = ')';
                    break;
                case '{':
                    end_char = '}';
                    break;
                case '<':
                    end_char = '>';
                    break;
            }
            if (end_char != ' ')
            {
                pos = strchr (pos_args, end_char);
                if (pos && (pos > pos_args + 1))
                {
                    channel = weechat_strndup (pos_args + 1,
                                               pos - pos_args - 1);
                    if (channel && irc_channel_search (server, channel))
                    {
                        is_channel = 1;
                        pos_args = pos + 1;
                        while (pos_args[0] == ' ')
                        {
                            pos_args++;
                        }
                    }
                }
            }
        }
        if (is_channel)
        {
            /* notice for channel */
            ptr_channel = irc_channel_search (server, channel);

            /*
             * unmask a smart filtered join if it is in hashtable
             * "join_smart_filtered" of channel
             */
            if (ptr_channel)
                irc_channel_join_smart_filtered_unmask (ptr_channel, nick);

            if (ptr_channel
                && weechat_config_boolean (irc_config_look_typing_status_nicks))
            {
                irc_typing_channel_set_nick (ptr_channel, nick,
                                             IRC_CHANNEL_TYPING_STATE_OFF);
            }

            ptr_nick = irc_nick_search (server, ptr_channel, nick);
            if (nick_is_me)
            {
                snprintf (str_tags, sizeof (str_tags),
                          "self_msg,notify_none,no_highlight");
            }
            else
            {
                snprintf (str_tags, sizeof (str_tags),
                          "%s",
                          (is_channel_orig) ?
                          "notify_message" :
                          weechat_config_string (irc_config_look_notice_welcome_tags));
            }
            weechat_printf_date_tags (
                (ptr_channel) ? ptr_channel->buffer : server->buffer,
                date,
                irc_protocol_tags (server, command, tags, str_tags,
                                   nick, address),
                "%s%s%s%s%s(%s%s%s%s)%s%s%s%s%s: %s",
                weechat_prefix ("network"),
                IRC_COLOR_NOTICE,
                (is_channel_orig) ? "" : "Pv",
                /* TRANSLATORS: "Notice" is command name in IRC protocol (translation is frequently the same word) */
                _("Notice"),
                IRC_COLOR_CHAT_DELIMITERS,
                irc_nick_mode_for_display (server, ptr_nick, 0),
                irc_nick_color_for_msg (server, 0, ptr_nick, nick),
                (nick && nick[0]) ? nick : "?",
                IRC_COLOR_CHAT_DELIMITERS,
                IRC_COLOR_RESET,
                (is_channel_orig) ? " -> " : "",
                (is_channel_orig) ? IRC_COLOR_CHAT_CHANNEL : "",
                (is_channel_orig) ? params[0] : "",
                (is_channel_orig) ? IRC_COLOR_RESET : "",
                pos_args);
        }
        else
        {
            /* notice for user */
            notify_private = 0;
            if (server->is_connected
                && nick
                && (weechat_strcasecmp (nick, "nickserv") != 0)
                && (weechat_strcasecmp (nick, "chanserv") != 0)
                && (weechat_strcasecmp (nick, "memoserv") != 0))
            {
                /*
                 * add tag "notify_private" only if:
                 *   - server is connected (message 001 already received)
                 * and:
                 *   - notice is from a non-empty nick different from
                 *     nickserv/chanserv/memoserv
                 */
                notify_private = 1;
            }

            ptr_channel = NULL;
            if (nick
                && weechat_config_integer (irc_config_look_notice_as_pv) != IRC_CONFIG_LOOK_NOTICE_AS_PV_NEVER)
            {
                ptr_channel = irc_channel_search (server, nick);
                if (!ptr_channel
                    && weechat_config_integer (irc_config_look_notice_as_pv) == IRC_CONFIG_LOOK_NOTICE_AS_PV_ALWAYS)
                {
                    ptr_channel = irc_channel_new (server,
                                                   IRC_CHANNEL_TYPE_PRIVATE,
                                                   nick, 0, 0);
                    if (!ptr_channel)
                    {
                        weechat_printf (server->buffer,
                                        _("%s%s: cannot create new "
                                          "private buffer \"%s\""),
                                        weechat_prefix ("error"),
                                        IRC_PLUGIN_NAME, nick);
                    }
                }
            }

            if (ptr_channel)
            {
                /* rename buffer if open with nick case not matching */
                if (strcmp (ptr_channel->name, nick) != 0)
                    irc_channel_pv_rename (server, ptr_channel, nick);

                if (weechat_config_boolean (irc_config_look_typing_status_nicks))
                {
                    irc_typing_channel_set_nick (ptr_channel, nick,
                                                 IRC_CHANNEL_TYPING_STATE_OFF);
                }

                if (!ptr_channel->topic)
                    irc_channel_set_topic (ptr_channel, address);

                weechat_printf_date_tags (
                    ptr_channel->buffer,
                    date,
                    irc_protocol_tags (server, command, tags, "notify_private",
                                       nick, address),
                    "%s%s%s%s: %s",
                    weechat_prefix ("network"),
                    irc_nick_color_for_msg (server, 0, NULL, nick),
                    nick,
                    IRC_COLOR_RESET,
                    pos_args);
                if ((ptr_channel->type == IRC_CHANNEL_TYPE_PRIVATE)
                    && ptr_channel->has_quit_server)
                {
                    ptr_channel->has_quit_server = 0;
                }
            }
            else
            {
                ptr_buffer = irc_msgbuffer_get_target_buffer (server, nick,
                                                              command, NULL,
                                                              NULL);
                /*
                 * if notice is sent from myself (for example another WeeChat
                 * via relay), then display message of outgoing notice
                 */
                if (nick && (irc_server_strcasecmp (server, server->nick, nick) == 0))
                {
                    weechat_printf_date_tags (
                        ptr_buffer,
                        date,
                        irc_protocol_tags (server,
                                           command,
                                           tags,
                                           (notify_private) ? "notify_private" : NULL,
                                           server->nick, address),
                        "%s%s%s%s -> %s%s%s: %s",
                        weechat_prefix ("network"),
                        IRC_COLOR_NOTICE,
                        /* TRANSLATORS: "Notice" is command name in IRC protocol (translation is frequently the same word) */
                        _("Notice"),
                        IRC_COLOR_RESET,
                        irc_nick_color_for_msg (server, 0, NULL, pos_target),
                        pos_target,
                        IRC_COLOR_RESET,
                        pos_args);
                }
                else
                {
                    display_host = weechat_config_boolean (
                        irc_config_look_display_host_notice);
                    nick_address = irc_protocol_nick_address (
                        server,
                        0,
                        NULL,
                        nick,
                        (display_host) ? address : NULL);
                    weechat_printf_date_tags (
                        ptr_buffer,
                        date,
                        irc_protocol_tags (server,
                                           command,
                                           tags,
                                           (notify_private) ? "notify_private" : NULL,
                                           nick, address),
                        "%s%s%s%s",
                        weechat_prefix ("network"),
                        nick_address,
                        (nick_address[0]) ? ": " : "",
                        pos_args);
                }
            }
        }
        if (channel)
            free (channel);
    }

    free (notice_args);

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "PART".
 *
 * Command looks like:
 *   PART #channel :part message
 *
 * On undernet server, it can be:
 *   PART :#channel
 *   PART #channel :part message
 */

IRC_PROTOCOL_CALLBACK(part)
{
    char *str_comment;
    int local_part, display_host;
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick;
    struct t_irc_channel_speaking *ptr_nick_speaking;

    IRC_PROTOCOL_MIN_PARAMS(1);
    IRC_PROTOCOL_CHECK_NICK;

    ptr_channel = irc_channel_search (server, params[0]);
    if (!ptr_channel)
        return WEECHAT_RC_OK;

    str_comment = (num_params > 1) ?
        irc_protocol_string_params (params, 1, num_params - 1) : NULL;

    ptr_nick = irc_nick_search (server, ptr_channel, nick);

    local_part = (irc_server_strcasecmp (server, nick, server->nick) == 0);

    /* display part message */
    if (!ignored)
    {
        ptr_nick_speaking = NULL;
        if (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL)
        {
            ptr_nick_speaking = ((weechat_config_boolean (irc_config_look_smart_filter))
                                 && (weechat_config_boolean (irc_config_look_smart_filter_quit))) ?
                irc_channel_nick_speaking_time_search (server, ptr_channel, nick, 1) : NULL;
        }
        display_host = weechat_config_boolean (irc_config_look_display_host_quit);
        if (str_comment && str_comment[0])
        {
            weechat_printf_date_tags (
                irc_msgbuffer_get_target_buffer (
                    server, NULL, command, NULL, ptr_channel->buffer),
                date,
                irc_protocol_tags (server,
                                   command,
                                   tags,
                                   (local_part
                                    || (ptr_channel->type != IRC_CHANNEL_TYPE_CHANNEL)
                                    || !weechat_config_boolean (irc_config_look_smart_filter)
                                    || !weechat_config_boolean (irc_config_look_smart_filter_quit)
                                    || ptr_nick_speaking) ?
                                   NULL : "irc_smart_filter",
                                   nick, address),
                _("%s%s%s%s%s%s%s%s%s%s has left %s%s%s %s(%s%s%s)"),
                weechat_prefix ("quit"),
                irc_nick_color_for_msg (server, 1, ptr_nick, nick),
                nick,
                IRC_COLOR_CHAT_DELIMITERS,
                (display_host) ? " (" : "",
                IRC_COLOR_CHAT_HOST,
                (display_host) ? address : "",
                IRC_COLOR_CHAT_DELIMITERS,
                (display_host) ? ")" : "",
                IRC_COLOR_MESSAGE_QUIT,
                IRC_COLOR_CHAT_CHANNEL,
                ptr_channel->name,
                IRC_COLOR_MESSAGE_QUIT,
                IRC_COLOR_CHAT_DELIMITERS,
                IRC_COLOR_REASON_QUIT,
                str_comment,
                IRC_COLOR_CHAT_DELIMITERS);
        }
        else
        {
            weechat_printf_date_tags (
                irc_msgbuffer_get_target_buffer (
                    server, NULL, command, NULL, ptr_channel->buffer),
                date,
                irc_protocol_tags (server,
                                   command,
                                   tags,
                                   (local_part
                                    || (ptr_channel->type != IRC_CHANNEL_TYPE_CHANNEL)
                                    || !weechat_config_boolean (irc_config_look_smart_filter)
                                    || !weechat_config_boolean (irc_config_look_smart_filter_quit)
                                    || ptr_nick_speaking) ?
                                   NULL : "irc_smart_filter",
                                   nick, address),
                _("%s%s%s%s%s%s%s%s%s%s has left %s%s%s"),
                weechat_prefix ("quit"),
                irc_nick_color_for_msg (server, 1, ptr_nick, nick),
                nick,
                IRC_COLOR_CHAT_DELIMITERS,
                (display_host) ? " (" : "",
                IRC_COLOR_CHAT_HOST,
                (display_host) ? address : "",
                IRC_COLOR_CHAT_DELIMITERS,
                (display_host) ? ")" : "",
                IRC_COLOR_MESSAGE_QUIT,
                IRC_COLOR_CHAT_CHANNEL,
                ptr_channel->name,
                IRC_COLOR_MESSAGE_QUIT);
        }
    }

    /* part request was issued by local client ? */
    if (local_part)
    {
        if (weechat_config_boolean (irc_config_look_typing_status_nicks))
            irc_typing_channel_reset (ptr_channel);

        irc_nick_free_all (server, ptr_channel);

        irc_channel_modelist_set_state (ptr_channel,
                                        IRC_MODELIST_STATE_MODIFIED);

        /* cycling ? => rejoin channel immediately */
        if (ptr_channel->cycle)
        {
            ptr_channel->cycle = 0;
            irc_channel_rejoin (server, ptr_channel, 1, 1);
        }
        else
        {
            if (weechat_config_boolean (irc_config_look_part_closes_buffer))
                weechat_buffer_close (ptr_channel->buffer);
            else
                ptr_channel->part = 1;
        }
        irc_bar_item_update_channel ();
    }
    else
    {
        /* part from another user */
        if (weechat_config_boolean (irc_config_look_typing_status_nicks))
        {
            irc_typing_channel_set_nick (ptr_channel, nick,
                                         IRC_CHANNEL_TYPING_STATE_OFF);
        }
        if (ptr_nick)
        {
            irc_channel_join_smart_filtered_remove (ptr_channel,
                                                    ptr_nick->name);
            irc_nick_free (server, ptr_channel, ptr_nick);
        }
    }

    if (str_comment)
        free (str_comment);

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "PING".
 *
 * Command looks like:
 *   PING :arguments
 */

IRC_PROTOCOL_CALLBACK(ping)
{
    char *str_params;

    IRC_PROTOCOL_MIN_PARAMS(1);

    str_params = irc_protocol_string_params (params, 0, num_params - 1);

    irc_server_sendf (server, 0, NULL, "PONG :%s", str_params);

    if (str_params)
        free (str_params);

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "PONG".
 *
 * Command looks like:
 *   PONG server :arguments
 */

IRC_PROTOCOL_CALLBACK(pong)
{
    struct timeval tv;
    char *str_params;

    IRC_PROTOCOL_MIN_PARAMS(0);

    if (server->lag_check_time.tv_sec != 0)
    {
        /* calculate lag (time diff with lag check) */
        gettimeofday (&tv, NULL);
        server->lag = (int)(weechat_util_timeval_diff (&(server->lag_check_time),
                                                       &tv) / 1000);

        /* schedule next lag check */
        server->lag_check_time.tv_sec = 0;
        server->lag_check_time.tv_usec = 0;
        server->lag_next_check = time (NULL) +
            weechat_config_integer (irc_config_network_lag_check);

        /* refresh lag bar item if needed */
        if (server->lag != server->lag_displayed)
        {
            server->lag_displayed = server->lag;
            irc_server_set_lag (server);
        }
    }
    else
    {
        str_params = (num_params > 1) ?
            irc_protocol_string_params (params, 1, num_params - 1) : NULL;
        weechat_printf_date_tags (
            irc_msgbuffer_get_target_buffer (server, NULL, command, NULL, NULL),
            date,
            irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
            "PONG%s%s",
            (str_params) ? ": " : "",
            (str_params) ? str_params : "");
        if (str_params)
            free (str_params);
    }

    return WEECHAT_RC_OK;
}

/*
 * Displays a CTCP sent, that was received by PRIVMSG if the origin nick
 * is self.
 *
 * Parameter "arguments" is the message arguments, for example:
 *
 *   \01VERSION\01
 *   \01TEST some arguments\01
 */

void
irc_protocol_privmsg_display_ctcp_send (struct t_irc_server *server,
                                        const char *target,
                                        const char *address,
                                        const char *arguments)
{
    const char *pos_space, *pos_end;
    char *ctcp_type, *ctcp_args;

    if (!arguments || !arguments[0])
        return;

    pos_end = strchr (arguments + 1, '\01');
    if (!pos_end)
        return;

    pos_space = strchr (arguments + 1, ' ');

    ctcp_type = weechat_strndup (
        arguments + 1,
        (pos_space) ?
        pos_space - arguments - 1 : pos_end - arguments - 1);
    ctcp_args = (pos_space) ?
        weechat_strndup (pos_space + 1, pos_end - pos_space - 1) : NULL;

    irc_input_user_message_display (
        server,
        target,
        address,
        "privmsg",
        ctcp_type,
        ctcp_args,
        0);  /* decode_colors */

    if (ctcp_type)
        free (ctcp_type);
    if (ctcp_args)
        free (ctcp_args);
}

/*
 * Callback for the IRC command "PRIVMSG".
 *
 * Command looks like:
 *   PRIVMSG #channel :message for channel here
 *   PRIVMSG @#channel :message for channel ops here
 *   PRIVMSG mynick :message for private here
 *   PRIVMSG #channel :\01ACTION is testing action\01
 *   PRIVMSG mynick :\01ACTION is testing action\01
 *   PRIVMSG #channel :\01VERSION\01
 *   PRIVMSG mynick :\01VERSION\01
 *   PRIVMSG mynick :\01DCC SEND file.txt 1488915698 50612 128\01
 */

IRC_PROTOCOL_CALLBACK(privmsg)
{
    char *msg_args, *msg_args2, str_tags[1024], *str_color, *color;
    const char *pos_target, *remote_nick, *pv_tags;
    int status_msg, is_channel, nick_is_me, cap_echo_message;
    int msg_already_received;
    time_t time_now;
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick;

    IRC_PROTOCOL_MIN_PARAMS(2);
    IRC_PROTOCOL_CHECK_NICK;

    if (ignored)
        return WEECHAT_RC_OK;

    msg_args = irc_protocol_string_params (params, 1, num_params - 1);
    if (!msg_args)
        return WEECHAT_RC_ERROR;

    nick_is_me = (irc_server_strcasecmp (server, server->nick, nick) == 0);

    pos_target = params[0];
    status_msg = 0;
    is_channel = irc_channel_is_channel (server, pos_target);
    if (!is_channel
        && irc_channel_is_channel (server, pos_target + 1)
        && irc_server_prefix_char_statusmsg (server, pos_target[0]))
    {
        is_channel = 1;
        status_msg = 1;
        pos_target++;
    }

    /* receiver is a channel ? */
    if (is_channel)
    {
        ptr_channel = irc_channel_search (server, pos_target);
        if (ptr_channel)
        {
            /*
             * unmask a smart filtered join if it is in hashtable
             * "join_smart_filtered" of channel
             */
            irc_channel_join_smart_filtered_unmask (ptr_channel, nick);

            /* CTCP to channel */
            if (msg_args[0] == '\01')
            {
                if (nick_is_me)
                {
                    irc_protocol_privmsg_display_ctcp_send (
                        server, params[0], address, msg_args);
                }
                else
                {
                    irc_ctcp_recv (server, date, tags, command, ptr_channel,
                                   params[0], address, nick, NULL, msg_args,
                                   irc_message);
                }
                goto end;
            }

            /* other message */
            if (weechat_config_boolean (irc_config_look_typing_status_nicks))
            {
                irc_typing_channel_set_nick (ptr_channel, nick,
                                             IRC_CHANNEL_TYPING_STATE_OFF);
            }

            ptr_nick = irc_nick_search (server, ptr_channel, nick);

            if (ptr_nick)
                irc_nick_set_host (ptr_nick, address);

            if (status_msg)
            {
                /* message to channel ops/voiced (to "@#channel" or "+#channel") */
                weechat_printf_date_tags (
                    ptr_channel->buffer,
                    date,
                    irc_protocol_tags (
                        server, command, tags,
                        (nick_is_me) ?
                        "self_msg,notify_none,no_highlight" : "notify_message",
                        nick, address),
                    "%s%s%s(%s%s%s%s)%s -> %s%s%s: %s",
                    weechat_prefix ("network"),
                    "Msg",
                    IRC_COLOR_CHAT_DELIMITERS,
                    irc_nick_mode_for_display (server, ptr_nick, 0),
                    irc_nick_color_for_msg (server, 0, ptr_nick, nick),
                    (nick && nick[0]) ? nick : "?",
                    IRC_COLOR_CHAT_DELIMITERS,
                    IRC_COLOR_RESET,
                    IRC_COLOR_CHAT_CHANNEL,
                    params[0],
                    IRC_COLOR_RESET,
                    msg_args);
            }
            else
            {
                /* standard message (to "#channel") */
                if (nick_is_me)
                {
                    str_color = irc_color_for_tags (
                        weechat_config_color (
                            weechat_config_get ("weechat.color.chat_nick_self")));
                    snprintf (str_tags, sizeof (str_tags),
                              "self_msg,notify_none,no_highlight,prefix_nick_%s",
                              (str_color) ? str_color : "default");
                }
                else
                {
                    color = irc_nick_find_color_name (
                        (ptr_nick) ? ptr_nick->name : nick);
                    str_color = irc_color_for_tags (color);
                    if (color)
                        free (color);
                    snprintf (str_tags, sizeof (str_tags),
                              "notify_message,prefix_nick_%s",
                              (str_color) ? str_color : "default");
                }
                if (str_color)
                    free (str_color);
                weechat_printf_date_tags (
                    ptr_channel->buffer,
                    date,
                    irc_protocol_tags (server, command, tags, str_tags, nick,
                                       address),
                    "%s%s",
                    irc_nick_as_prefix (server, ptr_nick,
                                        (ptr_nick) ? NULL : nick,
                                        NULL),
                    msg_args);
            }

            irc_channel_nick_speaking_add (
                ptr_channel,
                nick,
                weechat_string_has_highlight (msg_args,
                                              server->nick));
            irc_channel_nick_speaking_time_remove_old (ptr_channel);
            irc_channel_nick_speaking_time_add (server, ptr_channel, nick,
                                                time (NULL));
        }
    }
    else
    {
        remote_nick = (nick_is_me) ? pos_target : nick;

        /* private message received => display it */
        ptr_channel = irc_channel_search (server, remote_nick);

        /* CTCP to user */
        if (msg_args[0] == '\01')
        {
            cap_echo_message = weechat_hashtable_has_key (server->cap_list,
                                                          "echo-message");
            msg_already_received = weechat_hashtable_has_key (
                server->echo_msg_recv, irc_message);
            if (!msg_already_received && cap_echo_message)
            {
                time_now = time (NULL);
                weechat_hashtable_set (server->echo_msg_recv,
                                       irc_message, &time_now);
            }
            if (nick_is_me && cap_echo_message && !msg_already_received)
            {
                irc_protocol_privmsg_display_ctcp_send (
                    server, remote_nick, address, msg_args);
            }
            else
            {
                irc_ctcp_recv (server, date, tags, command, NULL, params[0],
                               address, nick, remote_nick, msg_args, irc_message);
            }
            if (msg_already_received)
                weechat_hashtable_remove (server->echo_msg_recv, irc_message);
            goto end;
        }

        if (ptr_channel)
        {
            /* rename buffer if open with nick case not matching */
            if (strcmp (ptr_channel->name, remote_nick) != 0)
                irc_channel_pv_rename (server, ptr_channel, remote_nick);
        }
        else
        {
            ptr_channel = irc_channel_new (server,
                                           IRC_CHANNEL_TYPE_PRIVATE,
                                           remote_nick, 0, 0);
            if (!ptr_channel)
            {
                weechat_printf (server->buffer,
                                _("%s%s: cannot create new "
                                  "private buffer \"%s\""),
                                weechat_prefix ("error"),
                                IRC_PLUGIN_NAME, remote_nick);
                goto end;
            }
        }

        if (weechat_config_boolean (irc_config_look_typing_status_nicks))
        {
            irc_typing_channel_set_nick (ptr_channel, nick,
                                         IRC_CHANNEL_TYPING_STATE_OFF);
        }

        irc_channel_set_topic (ptr_channel, address);

        if (nick_is_me)
        {
            str_color = irc_color_for_tags (
                weechat_config_color (
                    weechat_config_get ("weechat.color.chat_nick_self")));
        }
        else
        {
            if (weechat_config_boolean (irc_config_look_color_pv_nick_like_channel))
            {
                color = irc_nick_find_color_name (nick);
                str_color = irc_color_for_tags (color);
                if (color)
                    free (color);
            }
            else
            {
                str_color = irc_color_for_tags (
                    weechat_config_color (
                        weechat_config_get ("weechat.color.chat_nick_other")));
            }
        }
        if (nick_is_me)
        {
            snprintf (str_tags, sizeof (str_tags),
                      "self_msg,notify_none,no_highlight,prefix_nick_%s",
                      (str_color) ? str_color : "default");
        }
        else
        {
            pv_tags = weechat_config_string (irc_config_look_pv_tags);
            snprintf (str_tags, sizeof (str_tags),
                      "%s%sprefix_nick_%s",
                      (pv_tags && pv_tags[0]) ? pv_tags : "",
                      (pv_tags && pv_tags[0]) ? "," : "",
                      (str_color) ? str_color : "default");
        }
        if (str_color)
            free (str_color);
        msg_args2 = (nick_is_me) ?
            irc_message_hide_password (server, remote_nick, msg_args) : NULL;
        weechat_printf_date_tags (
            ptr_channel->buffer,
            date,
            irc_protocol_tags (server, command, tags, str_tags, nick, address),
            "%s%s",
            irc_nick_as_prefix (
                server, NULL, nick,
                (nick_is_me) ?
                IRC_COLOR_CHAT_NICK_SELF : irc_nick_color_for_pv (ptr_channel, nick)),
            (msg_args2) ? msg_args2 : msg_args);
        if (msg_args2)
            free (msg_args2);

        if (ptr_channel->has_quit_server)
            ptr_channel->has_quit_server = 0;

        (void) weechat_hook_signal_send ("irc_pv",
                                         WEECHAT_HOOK_SIGNAL_STRING,
                                         (char *)irc_message);
    }

end:
    if (msg_args)
        free (msg_args);
    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "QUIT".
 *
 * Command looks like:
 *   QUIT :quit message
 */

IRC_PROTOCOL_CALLBACK(quit)
{
    char *str_quit_msg;
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick;
    struct t_irc_channel_speaking *ptr_nick_speaking;
    int local_quit, display_host;

    IRC_PROTOCOL_MIN_PARAMS(0);
    IRC_PROTOCOL_CHECK_NICK;

    str_quit_msg = (num_params > 0) ?
        irc_protocol_string_params (params, 0, num_params - 1) : NULL;

    for (ptr_channel = server->channels; ptr_channel;
         ptr_channel = ptr_channel->next_channel)
    {
        if (weechat_config_boolean (irc_config_look_typing_status_nicks))
        {
            irc_typing_channel_set_nick (ptr_channel, nick,
                                         IRC_CHANNEL_TYPING_STATE_OFF);
        }

        if (ptr_channel->type == IRC_CHANNEL_TYPE_PRIVATE)
            ptr_nick = NULL;
        else
            ptr_nick = irc_nick_search (server, ptr_channel, nick);

        if (ptr_nick
            || (irc_server_strcasecmp (server, ptr_channel->name, nick) == 0))
        {
            local_quit = (irc_server_strcasecmp (server, nick, server->nick) == 0);
            if (!irc_ignore_check (server, ptr_channel->name, nick, host))
            {
                /* display quit message */
                ptr_nick_speaking = NULL;
                if (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL)
                {
                    ptr_nick_speaking = ((weechat_config_boolean (irc_config_look_smart_filter))
                                         && (weechat_config_boolean (irc_config_look_smart_filter_quit))) ?
                        irc_channel_nick_speaking_time_search (server, ptr_channel, nick, 1) : NULL;
                }
                if (ptr_channel->type == IRC_CHANNEL_TYPE_PRIVATE)
                {
                    ptr_channel->has_quit_server = 1;
                }
                display_host = weechat_config_boolean (irc_config_look_display_host_quit);
                if (str_quit_msg && str_quit_msg[0])
                {
                    weechat_printf_date_tags (
                        irc_msgbuffer_get_target_buffer (
                            server, NULL, command, NULL, ptr_channel->buffer),
                        date,
                        irc_protocol_tags (
                            server,
                            command,
                            tags,
                            (local_quit
                             || (ptr_channel->type != IRC_CHANNEL_TYPE_CHANNEL)
                             || !weechat_config_boolean (irc_config_look_smart_filter)
                             || !weechat_config_boolean (irc_config_look_smart_filter_quit)
                             || ptr_nick_speaking) ?
                            NULL : "irc_smart_filter",
                            nick, address),
                        _("%s%s%s%s%s%s%s%s%s%s has quit %s(%s%s%s)"),
                        weechat_prefix ("quit"),
                        (ptr_channel->type == IRC_CHANNEL_TYPE_PRIVATE) ?
                        irc_nick_color_for_pv (ptr_channel, nick) : irc_nick_color_for_msg (server, 1, ptr_nick, nick),
                        nick,
                        IRC_COLOR_CHAT_DELIMITERS,
                        (display_host) ? " (" : "",
                        IRC_COLOR_CHAT_HOST,
                        (display_host) ? address : "",
                        IRC_COLOR_CHAT_DELIMITERS,
                        (display_host) ? ")" : "",
                        IRC_COLOR_MESSAGE_QUIT,
                        IRC_COLOR_CHAT_DELIMITERS,
                        IRC_COLOR_REASON_QUIT,
                        str_quit_msg,
                        IRC_COLOR_CHAT_DELIMITERS);
                }
                else
                {
                    weechat_printf_date_tags (
                        irc_msgbuffer_get_target_buffer (
                            server, NULL, command, NULL, ptr_channel->buffer),
                        date,
                        irc_protocol_tags (
                            server,
                            command,
                            tags,
                            (local_quit
                             || (ptr_channel->type != IRC_CHANNEL_TYPE_CHANNEL)
                             || !weechat_config_boolean (irc_config_look_smart_filter)
                             || !weechat_config_boolean (irc_config_look_smart_filter_quit)
                             || ptr_nick_speaking) ?
                            NULL : "irc_smart_filter",
                            nick, address),
                        _("%s%s%s%s%s%s%s%s%s%s has quit"),
                        weechat_prefix ("quit"),
                        (ptr_channel->type == IRC_CHANNEL_TYPE_PRIVATE) ?
                        irc_nick_color_for_pv (ptr_channel, nick) : irc_nick_color_for_msg (server, 1, ptr_nick, nick),
                        nick,
                        IRC_COLOR_CHAT_DELIMITERS,
                        (display_host) ? " (" : "",
                        IRC_COLOR_CHAT_HOST,
                        (display_host) ? address : "",
                        IRC_COLOR_CHAT_DELIMITERS,
                        (display_host) ? ")" : "",
                        IRC_COLOR_MESSAGE_QUIT);
                }
            }
            if (!local_quit && ptr_nick)
            {
                irc_channel_join_smart_filtered_remove (ptr_channel,
                                                        ptr_nick->name);
            }
            if (ptr_nick)
                irc_nick_free (server, ptr_channel, ptr_nick);
        }
    }

    if (str_quit_msg)
        free (str_quit_msg);

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "SETNAME": set real name
 * (received when capability "setname" is enabled).
 *
 * Command looks like:
 *   SETNAME :the realname
 */

IRC_PROTOCOL_CALLBACK(setname)
{
    int local_setname, setname_enabled, smart_filter;
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick;
    struct t_irc_channel_speaking *ptr_nick_speaking;
    char *str_realname, *realname_color;

    IRC_PROTOCOL_MIN_PARAMS(1);
    IRC_PROTOCOL_CHECK_NICK;

    local_setname = (irc_server_strcasecmp (server, nick, server->nick) == 0);

    str_realname = irc_protocol_string_params (params, 0, num_params - 1);
    if (!str_realname)
        return WEECHAT_RC_ERROR;

    realname_color = irc_color_decode (
        str_realname,
        weechat_config_boolean (irc_config_network_colors_receive));

    setname_enabled = (weechat_hashtable_has_key (server->cap_list, "setname"));

    for (ptr_channel = server->channels; ptr_channel;
            ptr_channel = ptr_channel->next_channel)
    {
        switch (ptr_channel->type)
        {
            case IRC_CHANNEL_TYPE_PRIVATE:
                if (!ignored
                    && !local_setname
                    && (irc_server_strcasecmp (server,
                                               ptr_channel->name, nick) == 0))
                {
                    weechat_printf_date_tags (
                        irc_msgbuffer_get_target_buffer (
                            server, NULL, command, NULL, ptr_channel->buffer),
                        date,
                        irc_protocol_tags (server, command, tags, NULL, NULL,
                                           NULL),
                        _("%s%s%s%s has changed real name to %s\"%s%s%s\"%s"),
                        weechat_prefix ("network"),
                        irc_nick_color_for_msg (server, 1, NULL, nick),
                        nick,
                        IRC_COLOR_MESSAGE_SETNAME,
                        IRC_COLOR_CHAT_DELIMITERS,
                        IRC_COLOR_RESET,
                        (realname_color) ? realname_color : "",
                        IRC_COLOR_CHAT_DELIMITERS,
                        IRC_COLOR_RESET);
                }
                break;
            case IRC_CHANNEL_TYPE_CHANNEL:
                ptr_nick = irc_nick_search (server, ptr_channel, nick);
                if (ptr_nick)
                {
                    if (!ignored && !local_setname)
                    {
                        ptr_nick_speaking = ((weechat_config_boolean (irc_config_look_smart_filter))
                                             && (weechat_config_boolean (irc_config_look_smart_filter_setname))) ?
                            irc_channel_nick_speaking_time_search (server, ptr_channel, nick, 1) : NULL;
                        smart_filter = (!local_setname
                                        && weechat_config_boolean (irc_config_look_smart_filter)
                                        && weechat_config_boolean (irc_config_look_smart_filter_setname)
                                        && !ptr_nick_speaking);

                        weechat_printf_date_tags (
                            irc_msgbuffer_get_target_buffer (
                                server, NULL, command, NULL, ptr_channel->buffer),
                            date,
                            irc_protocol_tags (
                                server,
                                command,
                                tags,
                                (smart_filter) ? "irc_smart_filter" : NULL,
                                NULL,
                                NULL),
                            _("%s%s%s%s has changed real name to %s\"%s%s%s\"%s"),
                            weechat_prefix ("network"),
                            irc_nick_color_for_msg (server, 1, NULL, nick),
                            nick,
                            IRC_COLOR_MESSAGE_SETNAME,
                            IRC_COLOR_CHAT_DELIMITERS,
                            IRC_COLOR_RESET,
                            (realname_color) ? realname_color : "",
                            IRC_COLOR_CHAT_DELIMITERS,
                            IRC_COLOR_RESET);
                    }
                    if (setname_enabled)
                    {
                        if (ptr_nick->realname)
                            free (ptr_nick->realname);
                        ptr_nick->realname = strdup (str_realname);
                    }
                }
                break;
        }
    }

    if (!ignored && local_setname)
    {
        weechat_printf_date_tags (
            irc_msgbuffer_get_target_buffer (server, NULL, command, NULL, NULL),
            date,
            irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
            _("%s%sYour real name has been set to %s\"%s%s%s\"%s"),
            weechat_prefix ("network"),
            IRC_COLOR_MESSAGE_SETNAME,
            IRC_COLOR_CHAT_DELIMITERS,
            IRC_COLOR_RESET,
            (realname_color) ? realname_color : "",
            IRC_COLOR_CHAT_DELIMITERS,
            IRC_COLOR_RESET);
    }

    if (realname_color)
        free (realname_color);

    free (str_realname);

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "TAGMSG": message with tags but no text content
 * (received when capability "message-tags" is enabled).
 *
 * Whole message looks like:
 *   @msgid=6gqz7dxd22v7r3x9pvu;+typing=active :nick!user@host TAGMSG #channel
 *   @msgid=6gqz7dxd22v7r3x9pvu;+typing=active :nick!user@host TAGMSG :#channel
 */

IRC_PROTOCOL_CALLBACK(tagmsg)
{
    struct t_irc_channel *ptr_channel;
    const char *ptr_typing_value;
    int state;

    IRC_PROTOCOL_MIN_PARAMS(1);

    if (ignored)
        return WEECHAT_RC_OK;

    if (!tags)
        return WEECHAT_RC_OK;

    /* ignore if coming from self nick (if echo-message is enabled) */
    if (irc_server_strcasecmp (server, server->nick, nick) == 0)
        return WEECHAT_RC_OK;

    ptr_channel = NULL;
    if (irc_channel_is_channel (server, params[0]))
        ptr_channel = irc_channel_search (server, params[0]);
    else if (irc_server_strcasecmp (server, params[0], server->nick) == 0)
        ptr_channel = irc_channel_search (server, nick);
    if (!ptr_channel)
        return WEECHAT_RC_OK;

    if (weechat_config_boolean (irc_config_look_typing_status_nicks))
    {
        ptr_typing_value = weechat_hashtable_get (tags, "+typing");
        if (ptr_typing_value && ptr_typing_value[0])
        {
            if (strcmp (ptr_typing_value, "active") == 0)
                state = IRC_CHANNEL_TYPING_STATE_ACTIVE;
            else if (strcmp (ptr_typing_value, "paused") == 0)
                state = IRC_CHANNEL_TYPING_STATE_PAUSED;
            else
                state = IRC_CHANNEL_TYPING_STATE_OFF;
            irc_typing_channel_set_nick (ptr_channel, nick, state);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for an IRC command with mode and reason (numeric).
 */

IRC_PROTOCOL_CALLBACK(server_mode_reason)
{
    char *str_params;
    const char *pos_mode;
    int arg_text;

    IRC_PROTOCOL_MIN_PARAMS(1);

    /* skip nickname if at beginning of server message */
    if (irc_server_strcasecmp (server, server->nick, params[0]) == 0)
    {
        if (num_params < 2)
            return WEECHAT_RC_OK;
        pos_mode = params[1];
        arg_text = 2;
    }
    else
    {
        pos_mode = params[0];
        arg_text = 1;
    }

    str_params = irc_protocol_string_params (params, arg_text, num_params - 1);

    weechat_printf_date_tags (
            irc_msgbuffer_get_target_buffer (server, NULL, command, NULL, NULL),
            date,
            irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
            "%s%s%s%s",
            weechat_prefix ("network"),
            pos_mode,
            (str_params && str_params[0]) ? ": " : "",
            (str_params && str_params[0]) ? str_params : "");

    if (str_params)
        free (str_params);

    return WEECHAT_RC_OK;
}

/*
 * Callback for a numeric IRC command.
 */

IRC_PROTOCOL_CALLBACK(numeric)
{
    int arg_text;
    char *str_params;

    IRC_PROTOCOL_MIN_PARAMS(1);

    arg_text = ((irc_server_strcasecmp (server, server->nick, params[0]) == 0)
                || (strcmp (params[0], "*") == 0)) ?
        1 : 0;

    str_params = irc_protocol_string_params (params, arg_text, num_params - 1);

    if (str_params)
    {
        weechat_printf_date_tags (
            irc_msgbuffer_get_target_buffer (server, NULL, command, NULL, NULL),
            date,
            irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
            "%s%s",
            weechat_prefix ("network"),
            str_params);
        free (str_params);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "TOPIC".
 *
 * Command looks like:
 *   TOPIC #channel :new topic for channel
 */

IRC_PROTOCOL_CALLBACK(topic)
{
    char *str_topic, *old_topic_color, *topic_color;
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick;
    struct t_gui_buffer *ptr_buffer;

    IRC_PROTOCOL_MIN_PARAMS(1);
    IRC_PROTOCOL_CHECK_NICK;

    if (!irc_channel_is_channel (server, params[0]))
    {
        weechat_printf (server->buffer,
                        _("%s%s: \"%s\" command received without channel"),
                        weechat_prefix ("error"), IRC_PLUGIN_NAME, "topic");
        return WEECHAT_RC_OK;
    }

    str_topic = (num_params > 1) ?
        irc_protocol_string_params (params, 1, num_params - 1) : NULL;

    ptr_channel = irc_channel_search (server, params[0]);
    ptr_nick = irc_nick_search (server, ptr_channel, nick);
    ptr_buffer = (ptr_channel) ? ptr_channel->buffer : server->buffer;

    /*
     * unmask a smart filtered join if it is in hashtable
     * "join_smart_filtered" of channel
     */
    if (ptr_channel)
        irc_channel_join_smart_filtered_unmask (ptr_channel, nick);

    if (str_topic && str_topic[0])
    {
        topic_color = irc_color_decode (
            str_topic,
            weechat_config_boolean (irc_config_network_colors_receive));
        if (weechat_config_boolean (irc_config_look_display_old_topic)
            && ptr_channel && ptr_channel->topic && ptr_channel->topic[0])
        {
            old_topic_color = irc_color_decode (
                ptr_channel->topic,
                weechat_config_boolean (irc_config_network_colors_receive));
            weechat_printf_date_tags (
                irc_msgbuffer_get_target_buffer (
                    server, NULL, command, NULL, ptr_buffer),
                date,
                irc_protocol_tags (server, command, tags, NULL, NULL, address),
                _("%s%s%s%s has changed topic for %s%s%s from \"%s%s%s\" to "
                  "\"%s%s%s\""),
                weechat_prefix ("network"),
                irc_nick_color_for_msg (server, 1, ptr_nick, nick),
                nick,
                IRC_COLOR_RESET,
                IRC_COLOR_CHAT_CHANNEL,
                params[0],
                IRC_COLOR_RESET,
                IRC_COLOR_TOPIC_OLD,
                (old_topic_color) ? old_topic_color : ptr_channel->topic,
                IRC_COLOR_RESET,
                IRC_COLOR_TOPIC_NEW,
                (topic_color) ? topic_color : str_topic,
                IRC_COLOR_RESET);
            if (old_topic_color)
                free (old_topic_color);
        }
        else
        {
            weechat_printf_date_tags (
                irc_msgbuffer_get_target_buffer (
                    server, NULL, command, NULL, ptr_buffer),
                date,
                irc_protocol_tags (server, command, tags, NULL, NULL, address),
                _("%s%s%s%s has changed topic for %s%s%s to \"%s%s%s\""),
                weechat_prefix ("network"),
                irc_nick_color_for_msg (server, 1, ptr_nick, nick),
                nick,
                IRC_COLOR_RESET,
                IRC_COLOR_CHAT_CHANNEL,
                params[0],
                IRC_COLOR_RESET,
                IRC_COLOR_TOPIC_NEW,
                (topic_color) ? topic_color : str_topic,
                IRC_COLOR_RESET);
        }
        if (topic_color)
            free (topic_color);
    }
    else
    {
        if (weechat_config_boolean (irc_config_look_display_old_topic)
            && ptr_channel && ptr_channel->topic && ptr_channel->topic[0])
        {
            old_topic_color = irc_color_decode (
                ptr_channel->topic,
                weechat_config_boolean (irc_config_network_colors_receive));
            weechat_printf_date_tags (
                irc_msgbuffer_get_target_buffer (
                    server, NULL, command, NULL, ptr_buffer),
                date,
                irc_protocol_tags (server, command, tags, NULL, NULL, address),
                _("%s%s%s%s has unset topic for %s%s%s (old topic: "
                  "\"%s%s%s\")"),
                weechat_prefix ("network"),
                irc_nick_color_for_msg (server, 1, ptr_nick, nick),
                nick,
                IRC_COLOR_RESET,
                IRC_COLOR_CHAT_CHANNEL,
                params[0],
                IRC_COLOR_RESET,
                IRC_COLOR_TOPIC_OLD,
                (old_topic_color) ? old_topic_color : ptr_channel->topic,
                IRC_COLOR_RESET);
            if (old_topic_color)
                free (old_topic_color);
        }
        else
        {
            weechat_printf_date_tags (
                irc_msgbuffer_get_target_buffer (
                    server, NULL, command, NULL, ptr_buffer),
                date,
                irc_protocol_tags (server, command, tags, NULL, NULL, address),
                _("%s%s%s%s has unset topic for %s%s%s"),
                weechat_prefix ("network"),
                irc_nick_color_for_msg (server, 1, ptr_nick, nick),
                nick,
                IRC_COLOR_RESET,
                IRC_COLOR_CHAT_CHANNEL,
                params[0],
                IRC_COLOR_RESET);
        }
    }

    if (ptr_channel)
    {
        irc_channel_set_topic (ptr_channel,
                               (str_topic && str_topic[0]) ? str_topic : NULL);
    }

    if (str_topic)
        free (str_topic);

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "WALLOPS".
 *
 * Command looks like:
 *   WALLOPS :message from admin
 */

IRC_PROTOCOL_CALLBACK(wallops)
{
    const char *nick_address;
    char *str_message;

    IRC_PROTOCOL_MIN_PARAMS(1);

    if (ignored)
        return WEECHAT_RC_OK;

    nick_address = irc_protocol_nick_address (server, 0, NULL, nick, address);

    str_message = irc_protocol_string_params (params, 0, num_params - 1);

    weechat_printf_date_tags (
        irc_msgbuffer_get_target_buffer (server, nick, command, NULL, NULL),
        date,
        irc_protocol_tags (server, command, tags, "notify_private", nick,
                           address),
        _("%sWallops from %s: %s"),
        weechat_prefix ("network"),
        (nick_address[0]) ? nick_address : "?",
        str_message);

    if (str_message)
        free (str_message);

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "WARN".
 *
 * Command looks like:
 *   WARN REHASH CERTS_EXPIRED :Certificate [xxx] has expired
 */

IRC_PROTOCOL_CALLBACK(warn)
{
    IRC_PROTOCOL_MIN_PARAMS(2);

    irc_protocol_print_error_warning_msg (
        server,
        date,
        tags,
        command,
        weechat_prefix ("error"),
        _("Warning:"),
        params,
        num_params);

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "001": connected to IRC server.
 *
 * Command looks like:
 *   001 mynick :Welcome to the dancer-ircd Network
 */

IRC_PROTOCOL_CALLBACK(001)
{
    char **commands, **ptr_command, *command2, *command3, *slash_command;
    char *away_msg, *usermode;
    const char *ptr_server_command;
    int length;

    IRC_PROTOCOL_MIN_PARAMS(1);

    if (irc_server_strcasecmp (server, server->nick, params[0]) != 0)
        irc_server_set_nick (server, params[0]);

    IRC_PROTOCOL_RUN_CALLBACK(numeric);

    /* connection to IRC server is OK! */
    server->is_connected = 1;
    server->reconnect_delay = 0;
    server->monitor_time = time (NULL) + 5;
    irc_server_set_tls_version (server);

    if (server->hook_timer_connection)
    {
        weechat_unhook (server->hook_timer_connection);
        server->hook_timer_connection = NULL;
    }
    server->lag_next_check = time (NULL) +
        weechat_config_integer (irc_config_network_lag_check);
    irc_server_set_buffer_title (server);

    /* set away message if user was away (before disconnection for example) */
    if (server->away_message && server->away_message[0])
    {
        away_msg = strdup (server->away_message);
        if (away_msg)
        {
            irc_command_away_server (server, away_msg, 0);
            free (away_msg);
        }
    }

    /* send signal "irc_server_connected" with server name */
    (void) weechat_hook_signal_send ("irc_server_connected",
                                     WEECHAT_HOOK_SIGNAL_STRING, server->name);

    /* set usermode when connected */
    usermode = irc_server_eval_expression (
        server,
        IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_USERMODE));
    if (usermode && usermode[0])
    {
        irc_server_sendf (server,
                          IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                          "MODE %s %s",
                          server->nick, usermode);
    }
    if (usermode)
        free (usermode);

    /* execute command when connected */
    ptr_server_command = IRC_SERVER_OPTION_STRING(server,
                                                  IRC_SERVER_OPTION_COMMAND);
    if (ptr_server_command && ptr_server_command[0])
    {
        /* split command on ';' which can be escaped with '\;' */
        commands = weechat_string_split_command (ptr_server_command, ';');
        if (commands)
        {
            for (ptr_command = commands; *ptr_command; ptr_command++)
            {
                command2 = irc_server_eval_expression (server, *ptr_command);
                if (command2)
                {
                    command3 = irc_message_replace_vars (server, NULL,
                                                         command2);
                    if (command3)
                    {
                        if (weechat_string_is_command_char (command3))
                        {
                            weechat_command (server->buffer, command3);
                        }
                        else
                        {
                            length = 1 + strlen(command3) + 1;
                            slash_command = malloc (length);
                            if (slash_command)
                            {
                                snprintf (slash_command, length,
                                          "/%s", command3);
                                weechat_command (server->buffer,
                                                 slash_command);
                                free (slash_command);
                            }
                        }
                        free (command3);
                    }
                    free (command2);
                }
            }
            weechat_string_free_split_command (commands);
        }

        if (IRC_SERVER_OPTION_INTEGER(server, IRC_SERVER_OPTION_COMMAND_DELAY) > 0)
            server->command_time = time (NULL) + 1;
        else
            irc_server_autojoin_channels (server);
    }
    else
    {
        irc_server_autojoin_channels (server);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "005": some infos from server.
 *
 * Command looks like:
 *   005 mynick MODES=4 CHANLIMIT=#:20 NICKLEN=16 USERLEN=10 HOSTLEN=63
 *     TOPICLEN=450 KICKLEN=450 CHANNELLEN=30 KEYLEN=23 CHANTYPES=#
 *     PREFIX=(ov)@+ CASEMAPPING=ascii CAPAB IRCD=dancer
 *     :are available on this server
 */

IRC_PROTOCOL_CALLBACK(005)
{
    char *str_info, *error, *isupport2;
    int i, arg_last, length_isupport, length, casemapping, utf8mapping;
    long value;

    IRC_PROTOCOL_MIN_PARAMS(2);

    IRC_PROTOCOL_RUN_CALLBACK(numeric);

    arg_last = (strstr (irc_message, " :")) ? num_params - 2 : num_params - 1;

    for (i = 1; i <= arg_last; i++)
    {
        if (strncmp (params[i], "PREFIX=", 7) == 0)
        {
            /* save prefix */
            irc_server_set_prefix_modes_chars (server, params[i] + 7);
        }
        else if (strncmp (params[i], "LINELEN=", 8) == 0)
        {
            /* save max message length */
            error = NULL;
            value = strtol (params[i] + 8, &error, 10);
            if (error && !error[0] && (value > 0))
                server->msg_max_length = (int)value;
        }
        else if (strncmp (params[i], "NICKLEN=", 8) == 0)
        {
            /* save max nick length */
            error = NULL;
            value = strtol (params[i] + 8, &error, 10);
            if (error && !error[0] && (value > 0))
                server->nick_max_length = (int)value;
        }
        else if (strncmp (params[i], "USERLEN=", 8) == 0)
        {
            /* save max user length */
            error = NULL;
            value = strtol (params[i] + 8, &error, 10);
            if (error && !error[0] && (value > 0))
                server->user_max_length = (int)value;
        }
        else if (strncmp (params[i], "HOSTLEN=", 8) == 0)
        {
            /* save max host length */
            error = NULL;
            value = strtol (params[i] + 8, &error, 10);
            if (error && !error[0] && (value > 0))
                server->host_max_length = (int)value;
        }
        else if (strncmp (params[i], "CASEMAPPING=", 12) == 0)
        {
            /* save casemapping */
            casemapping = irc_server_search_casemapping (params[i] + 12);
            if (casemapping >= 0)
                server->casemapping = casemapping;
        }
        else if (strncmp (params[i], "UTF8MAPPING=", 12) == 0)
        {
            /* save utf8mapping */
            utf8mapping = irc_server_search_utf8mapping (params[i] + 12);
            if (utf8mapping >= 0)
                server->utf8mapping = utf8mapping;
        }
        else if (strcmp (params[i], "UTF8ONLY") == 0)
        {
            /* save utf8only */
            server->utf8only = 1;
        }
        else if (strncmp (params[i], "CHANTYPES=", 10) == 0)
        {
            /* save chantypes */
            if (server->chantypes)
                free (server->chantypes);
            server->chantypes = strdup (params[i] + 10);
        }
        else if (strncmp (params[i], "CHANMODES=", 10) == 0)
        {
            /* save chanmodes */
            if (server->chanmodes)
                free (server->chanmodes);
            server->chanmodes = strdup (params[i] + 10);
        }
        else if (strncmp (params[i], "MONITOR=", 8) == 0)
        {
            /* save monitor (limit) */
            error = NULL;
            value = strtol (params[i] + 8, &error, 10);
            if (error && !error[0] && (value > 0))
                server->monitor = (int)value;
        }
        else if (strncmp (params[i], "CLIENTTAGDENY=", 14) == 0)
        {
            /* save client tag deny */
            irc_server_set_clienttagdeny (server, params[i] + 14);
        }
    }

    /* save whole message (concatenate to existing isupport, if any) */
    str_info = irc_protocol_string_params (params, 1, arg_last);
    if (str_info && str_info[0])
    {
        length = strlen (str_info);
        if (server->isupport)
        {
            length_isupport = strlen (server->isupport);
            isupport2 = realloc (server->isupport,
                                 length_isupport +  /* existing */
                                 1 +  /* space */
                                 length +  /* new */
                                 1);
            if (isupport2)
            {
                server->isupport = isupport2;
                strcat (server->isupport, " ");
                strcat (server->isupport, str_info);
            }
        }
        else
        {
            server->isupport = strdup (str_info);
        }
    }
    if (str_info)
        free (str_info);

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "008": server notice mask.
 *
 * Command looks like:
 *   008 nick +Zbfkrsuy :Server notice mask
 */

IRC_PROTOCOL_CALLBACK(008)
{
    char *str_params;

    IRC_PROTOCOL_MIN_PARAMS(2);

    str_params = irc_protocol_string_params (params, 1, num_params - 1);

    weechat_printf_date_tags (
        irc_msgbuffer_get_target_buffer (server, params[0], command, NULL, NULL),
        date,
        irc_protocol_tags (server, command, tags, NULL, NULL, address),
        _("%sServer notice mask for %s%s%s: %s"),
        weechat_prefix ("network"),
        irc_nick_color_for_msg (server, 1, NULL, params[0]),
        params[0],
        IRC_COLOR_RESET,
        str_params);

    if (str_params)
        free (str_params);

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "221": user mode string.
 *
 * Command looks like:
 *   221 nick :+s
 */

IRC_PROTOCOL_CALLBACK(221)
{
    char *str_modes;

    IRC_PROTOCOL_MIN_PARAMS(2);

    str_modes = irc_protocol_string_params (params, 1, num_params - 1);

    weechat_printf_date_tags (
        irc_msgbuffer_get_target_buffer (server, params[0], command, NULL, NULL),
        date,
        irc_protocol_tags (server, command, tags, NULL, NULL, address),
        _("%sUser mode for %s%s%s is %s[%s%s%s]"),
        weechat_prefix ("network"),
        irc_nick_color_for_msg (server, 1, NULL, params[0]),
        params[0],
        IRC_COLOR_RESET,
        IRC_COLOR_CHAT_DELIMITERS,
        IRC_COLOR_RESET,
        str_modes,
        IRC_COLOR_CHAT_DELIMITERS);

    if (irc_server_strcasecmp (server, params[0], server->nick) == 0)
        irc_mode_user_set (server, str_modes, 1);

    if (str_modes)
        free (str_modes);

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "301": away message.
 *
 * Command is received when we are talking to a user in private and that remote
 * user is away (we receive away message).
 *
 * Command looks like:
 *   301 mynick nick :away message for nick
 */

IRC_PROTOCOL_CALLBACK(301)
{
    char *str_away_msg;
    struct t_irc_channel *ptr_channel;
    struct t_gui_buffer *ptr_buffer;

    IRC_PROTOCOL_MIN_PARAMS(1);

    if (num_params < 3)
        return WEECHAT_RC_OK;

    str_away_msg = irc_protocol_string_params (params, 2, num_params - 1);
    if (!str_away_msg)
        return WEECHAT_RC_ERROR;

    /* look for private buffer to display message */
    ptr_channel = irc_channel_search (server, params[1]);
    if (!weechat_config_boolean (irc_config_look_display_pv_away_once)
        || !ptr_channel
        || !(ptr_channel->away_message)
        || (strcmp (ptr_channel->away_message, str_away_msg) != 0))
    {
        ptr_buffer = (ptr_channel) ? ptr_channel->buffer : server->buffer;
        weechat_printf_date_tags (
            irc_msgbuffer_get_target_buffer (
                server, params[1], command, "whois", ptr_buffer),
            date,
            irc_protocol_tags (server, command, tags, NULL, NULL, address),
            _("%s%s[%s%s%s]%s is away: %s"),
            weechat_prefix ("network"),
            IRC_COLOR_CHAT_DELIMITERS,
            irc_nick_color_for_msg (server, 1, NULL, params[1]),
            params[1],
            IRC_COLOR_CHAT_DELIMITERS,
            IRC_COLOR_RESET,
            str_away_msg);
        if (ptr_channel)
        {
            if (ptr_channel->away_message)
                free (ptr_channel->away_message);
            ptr_channel->away_message = strdup (str_away_msg);
        }
    }

    free (str_away_msg);

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "303": ison.
 *
 * Command looks like:
 *   303 mynick :nick1 nick2
 */

IRC_PROTOCOL_CALLBACK(303)
{
    char *str_nicks;

    IRC_PROTOCOL_MIN_PARAMS(2);

    str_nicks = irc_protocol_string_params (params, 1, num_params - 1);

    weechat_printf_date_tags (
        irc_msgbuffer_get_target_buffer (server, NULL, command, NULL, NULL),
        date,
        irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
        _("%sUsers online: %s%s"),
        weechat_prefix ("network"),
        IRC_COLOR_CHAT_NICK,
        str_nicks);

    if (str_nicks)
        free (str_nicks);

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "305": unaway.
 *
 * Command looks like:
 *   305 mynick :Does this mean you're really back?
 */

IRC_PROTOCOL_CALLBACK(305)
{
    char *str_away_msg;

    IRC_PROTOCOL_MIN_PARAMS(1);

    if (num_params > 1)
    {
        str_away_msg = irc_protocol_string_params (params, 1, num_params - 1);
        weechat_printf_date_tags (
            irc_msgbuffer_get_target_buffer (
                server, NULL, command, "unaway", NULL),
            date,
            irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
            "%s%s",
            weechat_prefix ("network"),
            str_away_msg);
        if (str_away_msg)
            free (str_away_msg);
    }

    server->is_away = 0;
    server->away_time = 0;

    weechat_bar_item_update ("away");

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "306": now away.
 *
 * Command looks like:
 *   306 mynick :We'll miss you
 */

IRC_PROTOCOL_CALLBACK(306)
{
    char *str_away_msg;

    IRC_PROTOCOL_MIN_PARAMS(1);

    if (num_params > 1)
    {
        str_away_msg = irc_protocol_string_params (params, 1, num_params - 1);
        weechat_printf_date_tags (
            irc_msgbuffer_get_target_buffer (
                server, NULL, command, "away", NULL),
            date,
            irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
            "%s%s",
            weechat_prefix ("network"),
            str_away_msg);
        if (str_away_msg)
            free (str_away_msg);
    }

    server->is_away = 1;
    server->away_time = time (NULL);

    weechat_bar_item_update ("away");

    return WEECHAT_RC_OK;
}

/*
 * Callback for the whois commands with nick and message.
 *
 * Command looks like:
 *   319 mynick nick :some text here
 *
 * On InspIRCd server (not a whois reply):
 *   223 mynick :EXEMPT #help
 */

IRC_PROTOCOL_CALLBACK(whois_nick_msg)
{
    char *str_params;

    IRC_PROTOCOL_MIN_PARAMS(2);

    if (num_params >= 3)
    {
        str_params = irc_protocol_string_params (params, 2, num_params - 1);
        weechat_printf_date_tags (
            irc_msgbuffer_get_target_buffer (
                server, params[1], command, "whois", NULL),
            date,
            irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
            "%s%s[%s%s%s] %s%s",
            weechat_prefix ("network"),
            IRC_COLOR_CHAT_DELIMITERS,
            irc_nick_color_for_msg (server, 1, NULL, params[1]),
            params[1],
            IRC_COLOR_CHAT_DELIMITERS,
            IRC_COLOR_RESET,
            str_params);
        if (str_params)
            free (str_params);
    }
    else
    {
        /*
         * not enough parameters: this should not be a whois command so we
         * display the arguments as-is
         */
        IRC_PROTOCOL_RUN_CALLBACK(numeric);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for the whowas commands with nick and message.
 *
 * Command looks like:
 *   369 mynick nick :some text here
 */

IRC_PROTOCOL_CALLBACK(whowas_nick_msg)
{
    char *str_params;

    IRC_PROTOCOL_MIN_PARAMS(2);

    if (num_params >= 3)
    {
        str_params = irc_protocol_string_params (params, 2, num_params - 1);
        weechat_printf_date_tags (
            irc_msgbuffer_get_target_buffer (
                server, params[1], command, "whowas", NULL),
            date,
            irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
            "%s%s[%s%s%s] %s%s",
            weechat_prefix ("network"),
            IRC_COLOR_CHAT_DELIMITERS,
            irc_nick_color_for_msg (server, 1, NULL, params[1]),
            params[1],
            IRC_COLOR_CHAT_DELIMITERS,
            IRC_COLOR_RESET,
            str_params);
        if (str_params)
            free (str_params);
    }
    else
    {
        /*
         * not enough parameters: this should not be a whowas command so we
         * display the arguments as-is
         */
        IRC_PROTOCOL_RUN_CALLBACK(numeric);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "311": whois, user.
 *
 * Command looks like:
 *   311 mynick nick user host * :realname here
 */

IRC_PROTOCOL_CALLBACK(311)
{
    char *str_realname;

    IRC_PROTOCOL_MIN_PARAMS(3);

    if (num_params < 6)
    {
        IRC_PROTOCOL_RUN_CALLBACK(whois_nick_msg);
    }
    else
    {
        str_realname = irc_protocol_string_params (params, 5, num_params - 1);
        weechat_printf_date_tags (
            irc_msgbuffer_get_target_buffer (
                server, params[1], command, "whois", NULL),
            date,
            irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
            "%s%s[%s%s%s] (%s%s@%s%s)%s: %s",
            weechat_prefix ("network"),
            IRC_COLOR_CHAT_DELIMITERS,
            irc_nick_color_for_msg (server, 1, NULL, params[1]),
            params[1],
            IRC_COLOR_CHAT_DELIMITERS,
            IRC_COLOR_CHAT_HOST,
            params[2],
            params[3],
            IRC_COLOR_CHAT_DELIMITERS,
            IRC_COLOR_RESET,
            str_realname);
        if (str_realname)
            free (str_realname);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "312": whois, server.
 *
 * Command looks like:
 *   312 mynick nick tungsten.libera.chat :Umeå, SE
 */

IRC_PROTOCOL_CALLBACK(312)
{
    char *str_server;

    IRC_PROTOCOL_MIN_PARAMS(3);

    if (num_params < 4)
    {
        IRC_PROTOCOL_RUN_CALLBACK(whois_nick_msg);
    }
    else
    {
        str_server = irc_protocol_string_params (params, 3, num_params - 1);
        weechat_printf_date_tags (
            irc_msgbuffer_get_target_buffer (
                server, params[1], command, "whois", NULL),
            date,
            irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
            "%s%s[%s%s%s] %s%s %s(%s%s%s)",
            weechat_prefix ("network"),
            IRC_COLOR_CHAT_DELIMITERS,
            irc_nick_color_for_msg (server, 1, NULL, params[1]),
            params[1],
            IRC_COLOR_CHAT_DELIMITERS,
            IRC_COLOR_RESET,
            params[2],
            IRC_COLOR_CHAT_DELIMITERS,
            IRC_COLOR_RESET,
            str_server,
            IRC_COLOR_CHAT_DELIMITERS);
        if (str_server)
            free (str_server);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "314": whowas.
 *
 * Command looks like:
 *   314 mynick nick user host * :realname here
 */

IRC_PROTOCOL_CALLBACK(314)
{
    char *str_realname;

    IRC_PROTOCOL_MIN_PARAMS(3);

    if (num_params < 6)
    {
        IRC_PROTOCOL_RUN_CALLBACK(whowas_nick_msg);
    }
    else
    {
        str_realname = irc_protocol_string_params (params, 5, num_params - 1);
        weechat_printf_date_tags (
            irc_msgbuffer_get_target_buffer (
                server, params[1], command, "whowas", NULL),
            date,
            irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
            _("%s%s[%s%s%s] (%s%s@%s%s)%s was %s"),
            weechat_prefix ("network"),
            IRC_COLOR_CHAT_DELIMITERS,
            irc_nick_color_for_msg (server, 1, NULL, params[1]),
            params[1],
            IRC_COLOR_CHAT_DELIMITERS,
            IRC_COLOR_CHAT_HOST,
            params[2],
            params[3],
            IRC_COLOR_CHAT_DELIMITERS,
            IRC_COLOR_RESET,
            str_realname);
        if (str_realname)
            free (str_realname);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "315": end of /who.
 *
 * Command looks like:
 *   315 mynick #channel :End of /WHO list.
 */

IRC_PROTOCOL_CALLBACK(315)
{
    char *str_text;
    struct t_irc_channel *ptr_channel;

    IRC_PROTOCOL_MIN_PARAMS(3);

    ptr_channel = irc_channel_search (server, params[1]);
    if (ptr_channel && (ptr_channel->checking_whox > 0))
    {
        ptr_channel->checking_whox--;
    }
    else
    {
        str_text = irc_protocol_string_params (params, 2, num_params - 1);
        weechat_printf_date_tags (
            irc_msgbuffer_get_target_buffer (
                server, NULL, command, "who", NULL),
            date,
            irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
            "%s%s[%s%s%s]%s %s",
            weechat_prefix ("network"),
            IRC_COLOR_CHAT_DELIMITERS,
            IRC_COLOR_CHAT_CHANNEL,
            params[1],
            IRC_COLOR_CHAT_DELIMITERS,
            IRC_COLOR_RESET,
            str_text);
        if (str_text)
            free (str_text);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "317": whois, idle.
 *
 * Command looks like:
 *   317 mynick nick 122877 1205327880 :seconds idle, signon time
 */

IRC_PROTOCOL_CALLBACK(317)
{
    int idle_time, day, hour, min, sec;
    time_t datetime;
    struct t_gui_buffer *ptr_buffer;

    IRC_PROTOCOL_MIN_PARAMS(4);

    idle_time = atoi (params[2]);
    day = idle_time / (60 * 60 * 24);
    hour = (idle_time % (60 * 60 * 24)) / (60 * 60);
    min = ((idle_time % (60 * 60 * 24)) % (60 * 60)) / 60;
    sec = ((idle_time % (60 * 60 * 24)) % (60 * 60)) % 60;

    datetime = (time_t)(atol (params[3]));

    ptr_buffer = irc_msgbuffer_get_target_buffer (server, params[1],
                                                  command, "whois", NULL);

    if (day > 0)
    {
        weechat_printf_date_tags (
            ptr_buffer,
            date,
            irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
            _("%s%s[%s%s%s]%s idle: %s%d %s%s, %s%02d %s%s %s%02d %s%s %s%02d "
              "%s%s, signon at: %s%s"),
            weechat_prefix ("network"),
            IRC_COLOR_CHAT_DELIMITERS,
            irc_nick_color_for_msg (server, 1, NULL, params[1]),
            params[1],
            IRC_COLOR_CHAT_DELIMITERS,
            IRC_COLOR_RESET,
            IRC_COLOR_CHAT_CHANNEL,
            day,
            IRC_COLOR_RESET,
            NG_("day", "days", day),
            IRC_COLOR_CHAT_CHANNEL,
            hour,
            IRC_COLOR_RESET,
            NG_("hour", "hours", hour),
            IRC_COLOR_CHAT_CHANNEL,
            min,
            IRC_COLOR_RESET,
            NG_("minute", "minutes", min),
            IRC_COLOR_CHAT_CHANNEL,
            sec,
            IRC_COLOR_RESET,
            NG_("second", "seconds", sec),
            IRC_COLOR_CHAT_CHANNEL,
            weechat_util_get_time_string (&datetime));
    }
    else
    {
        weechat_printf_date_tags (
            ptr_buffer,
            date,
            irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
            _("%s%s[%s%s%s]%s idle: %s%02d %s%s %s%02d %s%s %s%02d %s%s, "
              "signon at: %s%s"),
            weechat_prefix ("network"),
            IRC_COLOR_CHAT_DELIMITERS,
            irc_nick_color_for_msg (server, 1, NULL, params[1]),
            params[1],
            IRC_COLOR_CHAT_DELIMITERS,
            IRC_COLOR_RESET,
            IRC_COLOR_CHAT_CHANNEL,
            hour,
            IRC_COLOR_RESET,
            NG_("hour", "hours", hour),
            IRC_COLOR_CHAT_CHANNEL,
            min,
            IRC_COLOR_RESET,
            NG_("minute", "minutes", min),
            IRC_COLOR_CHAT_CHANNEL,
            sec,
            IRC_COLOR_RESET,
            NG_("second", "seconds", sec),
            IRC_COLOR_CHAT_CHANNEL,
            weechat_util_get_time_string (&datetime));
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "321": /list start.
 *
 * Command looks like:
 *   321 mynick Channel :Users  Name
 */

IRC_PROTOCOL_CALLBACK(321)
{
    char *str_params;

    IRC_PROTOCOL_MIN_PARAMS(2);

    str_params = irc_protocol_string_params (params, 2, num_params - 1);

    weechat_printf_date_tags (
        irc_msgbuffer_get_target_buffer (
            server, NULL, command, "list", NULL),
        date,
        irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
        "%s%s%s%s",
        weechat_prefix ("network"),
        params[1],
        (str_params && str_params[0]) ? " " : "",
        (str_params && str_params[0]) ? str_params : "");

    if (str_params)
        free (str_params);

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "322": channel for /list.
 *
 * Command looks like:
 *   322 mynick #channel 3 :topic of channel
 */

IRC_PROTOCOL_CALLBACK(322)
{
    char *str_topic;

    IRC_PROTOCOL_MIN_PARAMS(3);

    if (!server->cmd_list_regexp ||
        (regexec (server->cmd_list_regexp, params[1], 0, NULL, 0) == 0))
    {
        str_topic = irc_protocol_string_params (params, 3, num_params - 1);
        weechat_printf_date_tags (
            irc_msgbuffer_get_target_buffer (
                server, NULL, command, "list", NULL),
            date,
            irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
            "%s%s%s%s(%s%s%s)%s%s%s",
            weechat_prefix ("network"),
            IRC_COLOR_CHAT_CHANNEL,
            params[1],
            IRC_COLOR_CHAT_DELIMITERS,
            IRC_COLOR_RESET,
            params[2],
            IRC_COLOR_CHAT_DELIMITERS,
            IRC_COLOR_RESET,
            (str_topic && str_topic[0]) ? ": " : "",
            (str_topic && str_topic[0]) ? str_topic : "");
        if (str_topic)
            free (str_topic);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "323": end of /list.
 *
 * Command looks like:
 *   323 mynick :End of /LIST
 */

IRC_PROTOCOL_CALLBACK(323)
{
    char *str_params;

    IRC_PROTOCOL_MIN_PARAMS(1);

    str_params = irc_protocol_string_params (params, 1, num_params - 1);

    weechat_printf_date_tags (
        irc_msgbuffer_get_target_buffer (server, NULL, command, "list", NULL),
        date,
        irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
        "%s%s",
        weechat_prefix ("network"),
        str_params);

    if (str_params)
        free (str_params);

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "324": channel mode.
 *
 * Command looks like:
 *   324 mynick #channel +nt
 */

IRC_PROTOCOL_CALLBACK(324)
{
    char *str_modes, *str_modes_args;
    struct t_irc_channel *ptr_channel;

    IRC_PROTOCOL_MIN_PARAMS(2);

    str_modes = (num_params > 2) ?
        irc_protocol_string_params (params, 2, num_params - 1) : NULL;
    str_modes_args = (num_params > 3) ?
        irc_protocol_string_params (params, 3, num_params - 1) : NULL;

    ptr_channel = irc_channel_search (server, params[1]);
    if (ptr_channel)
    {
        irc_channel_set_modes (ptr_channel, str_modes);
        if (num_params > 2)
        {
            (void) irc_mode_channel_set (server, ptr_channel, host,
                                         str_modes, str_modes_args);
        }
    }
    if (!ptr_channel
        || (weechat_hashtable_has_key (ptr_channel->join_msg_received, command)
            || weechat_hashtable_has_key (irc_config_hashtable_display_join_message, command)))
    {
        weechat_printf_date_tags (
            irc_msgbuffer_get_target_buffer (
                server, NULL, command, NULL,
                (ptr_channel) ? ptr_channel->buffer : NULL),
            date,
            irc_protocol_tags (server, command, tags, NULL, NULL, address),
            _("%sMode %s%s %s[%s%s%s]"),
            weechat_prefix ("network"),
            IRC_COLOR_CHAT_CHANNEL,
            params[1],
            IRC_COLOR_CHAT_DELIMITERS,
            IRC_COLOR_RESET,
            (str_modes) ? str_modes : "",
            IRC_COLOR_CHAT_DELIMITERS);
    }

    if (ptr_channel)
        weechat_hashtable_set (ptr_channel->join_msg_received, command, "1");

    if (str_modes)
        free (str_modes);
    if (str_modes_args)
        free (str_modes_args);

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "327": whois, host.
 *
 * Command looks like:
 *   327 mynick nick host ip :real hostname/ip
 */

IRC_PROTOCOL_CALLBACK(327)
{
    char *str_realname;
    struct t_gui_buffer *ptr_buffer;

    IRC_PROTOCOL_MIN_PARAMS(3);

    if (num_params < 4)
    {
        IRC_PROTOCOL_RUN_CALLBACK(whois_nick_msg);
    }
    else
    {
        str_realname = (num_params > 4) ?
            irc_protocol_string_params (params, 4, num_params - 1) : NULL;

        ptr_buffer = irc_msgbuffer_get_target_buffer (server, params[1],
                                                      command, "whois", NULL);

        if (str_realname && str_realname[0])
        {
            weechat_printf_date_tags (
                ptr_buffer,
                date,
                irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
                "%s%s[%s%s%s] %s%s %s %s(%s%s%s)",
                weechat_prefix ("network"),
                IRC_COLOR_CHAT_DELIMITERS,
                irc_nick_color_for_msg (server, 1, NULL, params[1]),
                params[1],
                IRC_COLOR_CHAT_DELIMITERS,
                IRC_COLOR_CHAT_HOST,
                params[2],
                params[3],
                IRC_COLOR_CHAT_DELIMITERS,
                IRC_COLOR_RESET,
                str_realname,
                IRC_COLOR_CHAT_DELIMITERS);
        }
        else
        {
            weechat_printf_date_tags (
                ptr_buffer,
                date,
                irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
                "%s%s[%s%s%s] %s%s %s",
                weechat_prefix ("network"),
                IRC_COLOR_CHAT_DELIMITERS,
                irc_nick_color_for_msg (server, 1, NULL, params[1]),
                params[1],
                IRC_COLOR_CHAT_DELIMITERS,
                IRC_COLOR_CHAT_HOST,
                params[2],
                params[3]);
        }

        if (str_realname)
            free (str_realname);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "328": channel URL.
 *
 * Command looks like:
 *   328 mynick #channel :https://example.com/
 */

IRC_PROTOCOL_CALLBACK(328)
{
    char *str_url;
    struct t_irc_channel *ptr_channel;

    IRC_PROTOCOL_MIN_PARAMS(3);

    ptr_channel = irc_channel_search (server, params[1]);
    if (ptr_channel)
    {
        str_url = irc_protocol_string_params (params, 2, num_params - 1);
        weechat_printf_date_tags (
            irc_msgbuffer_get_target_buffer (
                server, NULL, command, NULL, ptr_channel->buffer),
            date,
            irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
            _("%sURL for %s%s%s: %s"),
            weechat_prefix ("network"),
            IRC_COLOR_CHAT_CHANNEL,
            params[1],
            IRC_COLOR_RESET,
            str_url);
        if (str_url)
            free (str_url);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "329": channel creation date.
 *
 * Command looks like:
 *   329 mynick #channel 1205327894
 */

IRC_PROTOCOL_CALLBACK(329)
{
    struct t_irc_channel *ptr_channel;
    time_t datetime;

    IRC_PROTOCOL_MIN_PARAMS(3);

    ptr_channel = irc_channel_search (server, params[1]);

    datetime = (time_t)(atol (params[2]));

    if (ptr_channel)
    {
        if (weechat_hashtable_has_key (ptr_channel->join_msg_received, command)
            || weechat_hashtable_has_key (irc_config_hashtable_display_join_message, command))
        {
            weechat_printf_date_tags (
                irc_msgbuffer_get_target_buffer (
                    server, NULL, command, NULL, ptr_channel->buffer),
                date,
                irc_protocol_tags (server, command, tags, NULL,
                                   NULL, NULL),
                /* TRANSLATORS: "%s" after "created on" is a date */
                _("%sChannel created on %s"),
                weechat_prefix ("network"),
                weechat_util_get_time_string (&datetime));
        }
    }
    else
    {
        weechat_printf_date_tags (
            irc_msgbuffer_get_target_buffer (server, NULL, command, NULL, NULL),
            date,
            irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
            /* TRANSLATORS: "%s" after "created on" is a date */
            _("%sChannel %s%s%s created on %s"),
            weechat_prefix ("network"),
            IRC_COLOR_CHAT_CHANNEL,
            params[1],
            IRC_COLOR_RESET,
            weechat_util_get_time_string (&datetime));
    }

    if (ptr_channel)
        weechat_hashtable_set (ptr_channel->join_msg_received, command, "1");

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC commands "330" (whois, is logged in as), and "343"
 * (whois, is opered as).
 *
 * Commands look like:
 *   330 mynick nick1 nick2 :is logged in as
 *   330 mynick #channel https://example.com/
 *   343 mynick nick1 nick2 :is opered as
 */

IRC_PROTOCOL_CALLBACK(330_343)
{
    char *str_text;
    struct t_irc_channel *ptr_channel;
    struct t_gui_buffer *ptr_buffer;

    IRC_PROTOCOL_MIN_PARAMS(3);

    if (num_params >= 4)
    {
        str_text = irc_protocol_string_params (params, 3, num_params - 1);
        weechat_printf_date_tags (
            irc_msgbuffer_get_target_buffer (
                server, params[1], command, "whois", NULL),
            date,
            irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
            "%s%s[%s%s%s] %s%s %s%s",
            weechat_prefix ("network"),
            IRC_COLOR_CHAT_DELIMITERS,
            irc_nick_color_for_msg (server, 1, NULL, params[1]),
            params[1],
            IRC_COLOR_CHAT_DELIMITERS,
            IRC_COLOR_RESET,
            str_text,
            irc_nick_color_for_msg (server, 1, NULL, params[2]),
            params[2]);
        if (str_text)
            free (str_text);
    }
    else
    {
        ptr_channel = (irc_channel_is_channel (server, params[1])) ?
            irc_channel_search (server, params[1]) : NULL;
        ptr_buffer = (ptr_channel) ? ptr_channel->buffer : server->buffer;
        str_text = irc_protocol_string_params (params, 2, num_params - 1);
        weechat_printf_date_tags (
            irc_msgbuffer_get_target_buffer (
                server, params[1], command, "whois", ptr_buffer),
            date,
            irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
            "%s%s[%s%s%s] %s%s",
            weechat_prefix ("network"),
            IRC_COLOR_CHAT_DELIMITERS,
            irc_nick_color_for_msg (server, 1, NULL, params[1]),
            params[1],
            IRC_COLOR_CHAT_DELIMITERS,
            IRC_COLOR_RESET,
            str_text);
        if (str_text)
            free (str_text);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "331": no topic for channel.
 *
 * Command looks like:
 *   331 mynick #channel :There isn't a topic.
 */

IRC_PROTOCOL_CALLBACK(331)
{
    struct t_irc_channel *ptr_channel;
    struct t_gui_buffer *ptr_buffer;

    IRC_PROTOCOL_MIN_PARAMS(2);

    ptr_channel = irc_channel_search (server, params[1]);
    ptr_buffer = (ptr_channel) ? ptr_channel->buffer : server->buffer;
    weechat_printf_date_tags (
        irc_msgbuffer_get_target_buffer (
            server, params[1], command, NULL, ptr_buffer),
        date,
        irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
        _("%sNo topic set for channel %s%s"),
        weechat_prefix ("network"),
        IRC_COLOR_CHAT_CHANNEL,
        params[1]);

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "332": topic of channel.
 *
 * Command looks like:
 *   332 mynick #channel :topic of channel
 */

IRC_PROTOCOL_CALLBACK(332)
{
    char *str_topic, *topic_no_color, *topic_color;
    struct t_irc_channel *ptr_channel;
    struct t_gui_buffer *ptr_buffer;

    IRC_PROTOCOL_MIN_PARAMS(2);

    str_topic = (num_params >= 3) ?
        irc_protocol_string_params (params, 2, num_params - 1) : NULL;

    ptr_channel = irc_channel_search (server, params[1]);

    if (ptr_channel && ptr_channel->nicks)
    {
        if (str_topic)
        {
            topic_no_color = (weechat_config_boolean (irc_config_network_colors_receive)) ?
                NULL : irc_color_decode (str_topic, 0);
            irc_channel_set_topic (ptr_channel,
                                   (topic_no_color) ? topic_no_color : str_topic);
            if (topic_no_color)
                free (topic_no_color);
        }
        ptr_buffer = ptr_channel->buffer;
    }
    else
        ptr_buffer = server->buffer;

    topic_color = NULL;
    if (str_topic)
    {
        topic_color = irc_color_decode (
            str_topic,
            (weechat_config_boolean (irc_config_network_colors_receive)) ? 1 : 0);
    }

    if (!ptr_channel
        || (weechat_hashtable_has_key (ptr_channel->join_msg_received, command))
        || weechat_hashtable_has_key (irc_config_hashtable_display_join_message, command))
    {
        weechat_printf_date_tags (
            irc_msgbuffer_get_target_buffer (
                server, NULL, command, NULL, ptr_buffer),
            date,
            irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
            _("%sTopic for %s%s%s is \"%s%s%s\""),
            weechat_prefix ("network"),
            IRC_COLOR_CHAT_CHANNEL,
            params[1],
            IRC_COLOR_RESET,
            IRC_COLOR_TOPIC_CURRENT,
            (topic_color) ? topic_color : ((str_topic) ? str_topic : ""),
            IRC_COLOR_RESET);
    }

    if (topic_color)
        free (topic_color);

    if (ptr_channel)
        weechat_hashtable_set (ptr_channel->join_msg_received, command, "1");

    if (str_topic)
        free (str_topic);

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "333": infos about topic (nick / date).
 *
 * Command looks like:
 *   333 mynick #channel nick!user@host 1205428096
 *   333 mynick #channel 1205428096
 */

IRC_PROTOCOL_CALLBACK(333)
{
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick;
    time_t datetime;
    const char *topic_nick, *topic_address;

    IRC_PROTOCOL_MIN_PARAMS(3);

    topic_nick = (num_params > 3) ? irc_message_get_nick_from_host (params[2]) : NULL;
    topic_address = (num_params > 3) ? irc_message_get_address_from_host (params[2]) : NULL;
    if (topic_nick && topic_address && strcmp (topic_nick, topic_address) == 0)
        topic_address = NULL;

    ptr_channel = irc_channel_search (server, params[1]);
    ptr_nick = (ptr_channel) ?
        irc_nick_search (server, ptr_channel, topic_nick) : NULL;
    datetime = (num_params > 3) ?
        (time_t)(atol (params[3])) : (time_t)(atol (params[2]));

    if (!topic_nick && (datetime == 0))
        return WEECHAT_RC_OK;

    if (ptr_channel && ptr_channel->nicks)
    {
        if (weechat_hashtable_has_key (ptr_channel->join_msg_received, command)
            || weechat_hashtable_has_key (irc_config_hashtable_display_join_message, command))
        {
            if (topic_nick)
            {
                weechat_printf_date_tags (
                    irc_msgbuffer_get_target_buffer (
                        server, NULL, command, NULL, ptr_channel->buffer),
                    date,
                    irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
                    /* TRANSLATORS: "%s" after "on" is a date */
                    _("%sTopic set by %s%s%s%s%s%s%s%s%s on %s"),
                    weechat_prefix ("network"),
                    irc_nick_color_for_msg (server, 1, ptr_nick, topic_nick),
                    topic_nick,
                    IRC_COLOR_CHAT_DELIMITERS,
                    (topic_address && topic_address[0]) ? " (" : "",
                    IRC_COLOR_CHAT_HOST,
                    (topic_address) ? topic_address : "",
                    IRC_COLOR_CHAT_DELIMITERS,
                    (topic_address && topic_address[0]) ? ")" : "",
                    IRC_COLOR_RESET,
                    weechat_util_get_time_string (&datetime));
            }
            else
            {
                weechat_printf_date_tags (
                    irc_msgbuffer_get_target_buffer (
                        server, NULL, command, NULL, ptr_channel->buffer),
                    date,
                    irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
                    /* TRANSLATORS: "%s" after "on" is a date */
                    _("%sTopic set on %s"),
                    weechat_prefix ("network"),
                    weechat_util_get_time_string (&datetime));
            }
        }
    }
    else
    {
        if (topic_nick)
        {
            weechat_printf_date_tags (
                irc_msgbuffer_get_target_buffer (
                    server, NULL, command, NULL, NULL),
                date,
                irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
                /* TRANSLATORS: "%s" after "on" is a date */
                _("%sTopic for %s%s%s set by %s%s%s%s%s%s%s%s%s on %s"),
                weechat_prefix ("network"),
                IRC_COLOR_CHAT_CHANNEL,
                params[1],
                IRC_COLOR_RESET,
                irc_nick_color_for_msg (server, 1, ptr_nick, topic_nick),
                topic_nick,
                IRC_COLOR_CHAT_DELIMITERS,
                (topic_address && topic_address[0]) ? " (" : "",
                IRC_COLOR_CHAT_HOST,
                (topic_address) ? topic_address : "",
                IRC_COLOR_CHAT_DELIMITERS,
                (topic_address && topic_address[0]) ? ")" : "",
                IRC_COLOR_RESET,
                weechat_util_get_time_string (&datetime));
        }
        else
        {
            weechat_printf_date_tags (
                irc_msgbuffer_get_target_buffer (
                    server, NULL, command, NULL, NULL),
                date,
                irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
                /* TRANSLATORS: "%s" after "on" is a date */
                _("%sTopic for %s%s%s set on %s"),
                weechat_prefix ("network"),
                IRC_COLOR_CHAT_CHANNEL,
                params[1],
                IRC_COLOR_RESET,
                weechat_util_get_time_string (&datetime));
        }
    }

    if (ptr_channel)
        weechat_hashtable_set (ptr_channel->join_msg_received, command, "1");

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "338": whois, host.
 *
 * Command looks like:
 *   338 mynick nick host :actually using host
 *
 * On Rizon server:
 *   338 mynick nick :is actually user@host [ip]
 */

IRC_PROTOCOL_CALLBACK(338)
{
    char *str_text;

    IRC_PROTOCOL_MIN_PARAMS(3);

    if (num_params < 4)
    {
        IRC_PROTOCOL_RUN_CALLBACK(whois_nick_msg);
    }
    else
    {
        str_text = irc_protocol_string_params (params, 3, num_params - 1);
        weechat_printf_date_tags (
            irc_msgbuffer_get_target_buffer (
                server, params[1], command, "whois", NULL),
            date,
            irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
            "%s%s[%s%s%s]%s %s %s%s",
            weechat_prefix ("network"),
            IRC_COLOR_CHAT_DELIMITERS,
            irc_nick_color_for_msg (server, 1, NULL, params[1]),
            params[1],
            IRC_COLOR_CHAT_DELIMITERS,
            IRC_COLOR_RESET,
            str_text,
            IRC_COLOR_CHAT_HOST,
            params[2]);
        if (str_text)
            free (str_text);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "341": inviting.
 *
 * Command looks like:
 *   341 mynick nick #channel
 *   341 mynick nick :#channel
 */

IRC_PROTOCOL_CALLBACK(341)
{
    IRC_PROTOCOL_MIN_PARAMS(3);

    weechat_printf_date_tags (
        irc_msgbuffer_get_target_buffer (server, params[0], command, NULL, NULL),
        date,
        irc_protocol_tags (server, command, tags, NULL, params[0], address),
        _("%s%s%s%s has invited %s%s%s to %s%s%s"),
        weechat_prefix ("network"),
        irc_nick_color_for_msg (server, 1, NULL, params[0]),
        params[0],
        IRC_COLOR_RESET,
        irc_nick_color_for_msg (server, 1, NULL, params[1]),
        params[1],
        IRC_COLOR_RESET,
        IRC_COLOR_CHAT_CHANNEL,
        params[2],
        IRC_COLOR_RESET);

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "344": channel reop or whois geo info.
 *
 * Command looks like, on IRCnet:
 *   344 mynick #channel nick!user@host
 *
 * Command looks like, on UnrealIRCd:
 *   344 mynick nick FR :is connecting from France
 */

IRC_PROTOCOL_CALLBACK(344)
{
    char *str_host;

    IRC_PROTOCOL_MIN_PARAMS(3);

    if (irc_channel_is_channel (server, params[1]))
    {
        /* channel reop (IRCnet) */
        str_host = irc_protocol_string_params (params, 2, num_params - 1);
        weechat_printf_date_tags (
            irc_msgbuffer_get_target_buffer (server, NULL, command, "reop", NULL),
            date,
            irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
            _("%sChannel reop %s%s%s: %s%s"),
            weechat_prefix ("network"),
            IRC_COLOR_CHAT_CHANNEL,
            params[1],
            IRC_COLOR_RESET,
            IRC_COLOR_CHAT_HOST,
            str_host);
        if (str_host)
            free (str_host);
    }
    else
    {
        /* whois, geo info (UnrealIRCd) */
        IRC_PROTOCOL_RUN_CALLBACK(whois_nick_msg);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "345": end of channel reop.
 *
 * Command looks like:
 *   345 mynick #channel :End of Channel Reop List
 */

IRC_PROTOCOL_CALLBACK(345)
{
    char *str_params;

    IRC_PROTOCOL_MIN_PARAMS(3);

    str_params = irc_protocol_string_params (params, 2, num_params - 1);

    weechat_printf_date_tags (
        irc_msgbuffer_get_target_buffer (server, NULL, command, "reop", NULL),
        date,
        irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
        "%s%s%s%s: %s",
        weechat_prefix ("network"),
        IRC_COLOR_CHAT_CHANNEL,
        params[1],
        IRC_COLOR_RESET,
        str_params);

    if (str_params)
        free (str_params);

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "346": channel invite list.
 *
 * Command looks like:
 *   346 mynick #channel invitemask nick!user@host 1205590879
 *   346 mynick #channel invitemask
 */

IRC_PROTOCOL_CALLBACK(346)
{
    struct t_irc_channel *ptr_channel;
    struct t_gui_buffer *ptr_buffer;
    struct t_irc_modelist *ptr_modelist;
    time_t datetime;
    const char *nick_address;
    char str_number[64];

    IRC_PROTOCOL_MIN_PARAMS(3);

    ptr_channel = irc_channel_search (server, params[1]);
    ptr_buffer = (ptr_channel && ptr_channel->nicks) ?
        ptr_channel->buffer : server->buffer;
    ptr_modelist = irc_modelist_search (ptr_channel, 'I');

    if (ptr_modelist)
    {
        /* start receiving new list */
        if (ptr_modelist->state != IRC_MODELIST_STATE_RECEIVING)
        {
            irc_modelist_item_free_all (ptr_modelist);
            ptr_modelist->state = IRC_MODELIST_STATE_RECEIVING;
        }

        snprintf (str_number, sizeof (str_number),
                  "%s[%s%d%s] ",
                  IRC_COLOR_CHAT_DELIMITERS,
                  IRC_COLOR_RESET,
                  ((ptr_modelist->last_item) ? ptr_modelist->last_item->number + 1 : 0) + 1,
                  IRC_COLOR_CHAT_DELIMITERS);
    }
    else
        str_number[0] = '\0';

    if (num_params >= 4)
    {
        nick_address = irc_protocol_nick_address (
            server, 1, NULL, irc_message_get_nick_from_host (params[3]),
            irc_message_get_address_from_host (params[3]));
        if (num_params >= 5)
        {
            datetime = (time_t)(atol (params[4]));
            if (ptr_modelist)
                irc_modelist_item_new (ptr_modelist, params[2], params[3], datetime);
            weechat_printf_date_tags (
                irc_msgbuffer_get_target_buffer (
                    server, NULL, command, "invitelist", ptr_buffer),
                date,
                irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
                /* TRANSLATORS: "%s" after "on" is a date */
                _("%s%s[%s%s%s] %s%s%s%s invited by %s on %s"),
                weechat_prefix ("network"),
                IRC_COLOR_CHAT_DELIMITERS,
                IRC_COLOR_CHAT_CHANNEL,
                params[1],
                IRC_COLOR_CHAT_DELIMITERS,
                str_number,
                IRC_COLOR_CHAT_HOST,
                params[2],
                IRC_COLOR_RESET,
                (nick_address[0]) ? nick_address : "?",
                weechat_util_get_time_string (&datetime));
        }
        else
        {
            if (ptr_modelist)
                irc_modelist_item_new (ptr_modelist, params[2], params[3], 0);
            weechat_printf_date_tags (
                irc_msgbuffer_get_target_buffer (
                    server, NULL, command, "invitelist", ptr_buffer),
                date,
                irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
                _("%s%s[%s%s%s] %s%s%s%s invited by %s"),
                weechat_prefix ("network"),
                IRC_COLOR_CHAT_DELIMITERS,
                IRC_COLOR_CHAT_CHANNEL,
                params[1],
                IRC_COLOR_CHAT_DELIMITERS,
                str_number,
                IRC_COLOR_CHAT_HOST,
                params[2],
                IRC_COLOR_RESET,
                (nick_address[0]) ? nick_address : "?");
        }
    }
    else
    {
        if (ptr_modelist)
            irc_modelist_item_new (ptr_modelist, params[2], NULL, 0);
        weechat_printf_date_tags (
            irc_msgbuffer_get_target_buffer (
                server, NULL, command, "invitelist", ptr_buffer),
            date,
            irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
            _("%s%s[%s%s%s] %s%s%s%s invited"),
            weechat_prefix ("network"),
            IRC_COLOR_CHAT_DELIMITERS,
            IRC_COLOR_CHAT_CHANNEL,
            params[1],
            IRC_COLOR_CHAT_DELIMITERS,
            str_number,
            IRC_COLOR_CHAT_HOST,
            params[2],
            IRC_COLOR_RESET);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "347": end of channel invite list.
 *
 * Command looks like:
 *   347 mynick #channel :End of Channel Invite List
 */

IRC_PROTOCOL_CALLBACK(347)
{
    char *str_params;
    struct t_irc_channel *ptr_channel;
    struct t_gui_buffer *ptr_buffer;
    struct t_irc_modelist *ptr_modelist;

    IRC_PROTOCOL_MIN_PARAMS(2);

    str_params = (num_params > 2) ?
        irc_protocol_string_params (params, 2, num_params - 1) : NULL;

    ptr_channel = irc_channel_search (server, params[1]);
    ptr_buffer = (ptr_channel && ptr_channel->nicks) ?
        ptr_channel->buffer : server->buffer;
    ptr_modelist = irc_modelist_search (ptr_channel, 'I');
    if (ptr_modelist)
    {
        if (ptr_modelist->state != IRC_MODELIST_STATE_RECEIVING)
        {
            /*
             * remove all items if no invite was received before
             * the end of invite list
             */
            irc_modelist_item_free_all (ptr_modelist);
        }
        ptr_modelist->state = IRC_MODELIST_STATE_RECEIVED;
    }
    weechat_printf_date_tags (
        irc_msgbuffer_get_target_buffer (
            server, NULL, command, "invitelist", ptr_buffer),
        date,
        irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
        "%s%s[%s%s%s]%s%s%s",
        weechat_prefix ("network"),
        IRC_COLOR_CHAT_DELIMITERS,
        IRC_COLOR_CHAT_CHANNEL,
        params[1],
        IRC_COLOR_CHAT_DELIMITERS,
        IRC_COLOR_RESET,
        (str_params) ? " " : "",
        (str_params) ? str_params : "");

    if (str_params)
        free (str_params);

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "348": channel exception list.
 *
 * Command looks like (nick2 is nick who set exception on nick1):
 *   348 mynick #channel nick1!user1@host1 nick2!user2@host2 1205585109
 */

IRC_PROTOCOL_CALLBACK(348)
{
    struct t_irc_channel *ptr_channel;
    struct t_gui_buffer *ptr_buffer;
    struct t_irc_modelist *ptr_modelist;
    time_t datetime;
    const char *nick_address;
    char str_number[64];

    IRC_PROTOCOL_MIN_PARAMS(3);

    ptr_channel = irc_channel_search (server, params[1]);
    ptr_buffer = (ptr_channel && ptr_channel->nicks) ?
        ptr_channel->buffer : server->buffer;
    ptr_modelist = irc_modelist_search (ptr_channel, 'e');

    if (ptr_modelist)
    {
        /* start receiving new list */
        if (ptr_modelist->state != IRC_MODELIST_STATE_RECEIVING)
        {
            irc_modelist_item_free_all (ptr_modelist);
            ptr_modelist->state = IRC_MODELIST_STATE_RECEIVING;
        }

        snprintf (str_number, sizeof (str_number),
                  " %s[%s%d%s]",
                  IRC_COLOR_CHAT_DELIMITERS,
                  IRC_COLOR_RESET,
                  ((ptr_modelist->last_item) ? ptr_modelist->last_item->number + 1 : 0) + 1,
                  IRC_COLOR_CHAT_DELIMITERS);
    }
    else
        str_number[0] = '\0';

    if (num_params >= 4)
    {
        nick_address = irc_protocol_nick_address (
            server, 1, NULL, irc_message_get_nick_from_host (params[3]),
            irc_message_get_address_from_host (params[3]));
        if (num_params >= 5)
        {
            datetime = (time_t)(atol (params[4]));
            if (ptr_modelist)
                irc_modelist_item_new (ptr_modelist, params[2], params[3], datetime);
            weechat_printf_date_tags (
                irc_msgbuffer_get_target_buffer (
                    server, NULL, command, "exceptionlist", ptr_buffer),
                date,
                irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
                /* TRANSLATORS: "%s" after "on" is a date */
                _("%s%s[%s%s%s]%s%s exception %s%s%s by %s on %s"),
                weechat_prefix ("network"),
                IRC_COLOR_CHAT_DELIMITERS,
                IRC_COLOR_CHAT_CHANNEL,
                params[1],
                IRC_COLOR_CHAT_DELIMITERS,
                str_number,
                IRC_COLOR_RESET,
                IRC_COLOR_CHAT_HOST,
                params[2],
                IRC_COLOR_RESET,
                (nick_address[0]) ? nick_address : "?",
                weechat_util_get_time_string (&datetime));
        }
        else
        {
            if (ptr_modelist)
                irc_modelist_item_new (ptr_modelist, params[2], params[3], 0);
            weechat_printf_date_tags (
                irc_msgbuffer_get_target_buffer (
                    server, NULL, command, "exceptionlist", ptr_buffer),
                date,
                irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
                _("%s%s[%s%s%s]%s%s exception %s%s%s by %s"),
                weechat_prefix ("network"),
                IRC_COLOR_CHAT_DELIMITERS,
                IRC_COLOR_CHAT_CHANNEL,
                params[1],
                IRC_COLOR_CHAT_DELIMITERS,
                str_number,
                IRC_COLOR_RESET,
                IRC_COLOR_CHAT_HOST,
                params[2],
                IRC_COLOR_RESET,
                (nick_address[0]) ? nick_address : "?");
        }
    }
    else
    {
        if (ptr_modelist)
            irc_modelist_item_new (ptr_modelist, params[2], NULL, 0);
        weechat_printf_date_tags (
            irc_msgbuffer_get_target_buffer (
                server, NULL, command, "exceptionlist", ptr_buffer),
            date,
            irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
            _("%s%s[%s%s%s]%s%s exception %s%s"),
            weechat_prefix ("network"),
            IRC_COLOR_CHAT_DELIMITERS,
            IRC_COLOR_CHAT_CHANNEL,
            params[1],
            IRC_COLOR_CHAT_DELIMITERS,
            str_number,
            IRC_COLOR_RESET,
            IRC_COLOR_CHAT_HOST,
            params[2]);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "349": end of channel exception list.
 *
 * Command looks like:
 *   349 mynick #channel :End of Channel Exception List
 */

IRC_PROTOCOL_CALLBACK(349)
{
    char *str_params;
    struct t_irc_channel *ptr_channel;
    struct t_gui_buffer *ptr_buffer;
    struct t_irc_modelist *ptr_modelist;

    IRC_PROTOCOL_MIN_PARAMS(2);

    str_params = (num_params > 2) ?
        irc_protocol_string_params (params, 2, num_params - 1) : NULL;

    ptr_channel = irc_channel_search (server, params[1]);
    ptr_buffer = (ptr_channel && ptr_channel->nicks) ?
        ptr_channel->buffer : server->buffer;
    ptr_modelist = irc_modelist_search (ptr_channel, 'e');
    if (ptr_modelist)
    {
        if (ptr_modelist->state != IRC_MODELIST_STATE_RECEIVING)
        {
            /*
             * remove all items if no exception was received before
             * the end of exception list
             */
            irc_modelist_item_free_all (ptr_modelist);
        }
        ptr_modelist->state = IRC_MODELIST_STATE_RECEIVED;
    }
    weechat_printf_date_tags (
        irc_msgbuffer_get_target_buffer (
            server, NULL, command, "exceptionlist", ptr_buffer),
        date,
        irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
        "%s%s[%s%s%s]%s%s%s",
        weechat_prefix ("network"),
        IRC_COLOR_CHAT_DELIMITERS,
        IRC_COLOR_CHAT_CHANNEL,
        params[1],
        IRC_COLOR_CHAT_DELIMITERS,
        IRC_COLOR_RESET,
        (str_params) ? " " : "",
        (str_params) ? str_params : "");

    if (str_params)
        free (str_params);

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "312": whois, gateway.
 *
 * Command looks like:
 *   350 mynick nick * * :is connected via the WebIRC gateway
 *   350 mynick nick real_hostname real_ip :is connected via the WebIRC gateway
 */

IRC_PROTOCOL_CALLBACK(350)
{
    char *str_params, str_host[1024];
    int has_real_hostmask, has_real_ip;

    IRC_PROTOCOL_MIN_PARAMS(2);

    if (num_params >= 5)
    {
        str_host[0] = '\0';
        has_real_hostmask = (strcmp (params[2], "*") != 0);
        has_real_ip = (strcmp (params[3], "*") != 0);
        if (has_real_hostmask || has_real_ip)
        {
            snprintf (str_host, sizeof (str_host),
                      "%s(%s%s%s%s%s%s%s) ",
                      IRC_COLOR_CHAT_DELIMITERS,
                      IRC_COLOR_CHAT_HOST,
                      (has_real_hostmask) ? params[2] : "",
                      (has_real_hostmask && has_real_ip) ? IRC_COLOR_CHAT_DELIMITERS : "",
                      (has_real_hostmask && has_real_ip) ? ", " : "",
                      (has_real_hostmask && has_real_ip) ? IRC_COLOR_CHAT_HOST : "",
                      (has_real_ip) ? params[3] : "",
                      IRC_COLOR_CHAT_DELIMITERS);
        }
        str_params = irc_protocol_string_params (params, 4, num_params - 1);
        weechat_printf_date_tags (
            irc_msgbuffer_get_target_buffer (
                server, params[1], command, "whois", NULL),
            date,
            irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
            "%s%s[%s%s%s] %s%s%s",
            weechat_prefix ("network"),
            IRC_COLOR_CHAT_DELIMITERS,
            irc_nick_color_for_msg (server, 1, NULL, params[1]),
            params[1],
            IRC_COLOR_CHAT_DELIMITERS,
            str_host,
            IRC_COLOR_RESET,
            str_params);
        if (str_params)
            free (str_params);

    }
    else
    {
        /* not enough parameters: display with the default whois callback */
        IRC_PROTOCOL_RUN_CALLBACK(whois_nick_msg);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "351": server version.
 *
 * Command looks like:
 *   351 mynick dancer-ircd-1.0.36(2006/07/23_13:11:50). server :iMZ dncrTS/v4
 */

IRC_PROTOCOL_CALLBACK(351)
{
    char *str_params;
    struct t_gui_buffer *ptr_buffer;

    IRC_PROTOCOL_MIN_PARAMS(3);

    ptr_buffer = irc_msgbuffer_get_target_buffer (server, NULL, command, NULL,
                                                  NULL);

    if (num_params > 3)
    {
        str_params = irc_protocol_string_params (params, 3, num_params - 1);
        weechat_printf_date_tags (
            ptr_buffer,
            date,
            irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
            "%s%s %s (%s)",
            weechat_prefix ("network"),
            params[1],
            params[2],
            str_params);
        if (str_params)
            free (str_params);
    }
    else
    {
        weechat_printf_date_tags (
            ptr_buffer,
            date,
            irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
            "%s%s %s",
            weechat_prefix ("network"),
            params[1],
            params[2]);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "352": who.
 *
 * Command looks like:
 *   352 mynick #channel user host server nick status :hopcount real name
 */

IRC_PROTOCOL_CALLBACK(352)
{
    char *str_host, *str_hopcount, *str_realname;
    const char *pos;
    int length;
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick;

    IRC_PROTOCOL_MIN_PARAMS(3);

    /* silently ignore malformed 352 message (missing infos) */
    if (num_params < 6)
        return WEECHAT_RC_OK;

    str_hopcount = NULL;
    str_realname = NULL;
    if (num_params >= 8)
    {
        pos = strchr (params[num_params - 1],  ' ');
        if (pos)
        {
            str_hopcount = weechat_strndup (params[num_params - 1],
                                            pos - params[num_params - 1]);
            while (pos[0] == ' ')
            {
                pos++;
            }
            if (pos[0])
                str_realname = strdup (pos);
        }
        else
        {
            str_hopcount = strdup (params[num_params - 1]);
        }
    }

    ptr_channel = irc_channel_search (server, params[1]);
    ptr_nick = (ptr_channel) ?
        irc_nick_search (server, ptr_channel, params[5]) : NULL;

    /* update host in nick */
    if (ptr_nick)
    {
        length = strlen (params[2]) + 1 + strlen (params[3]) + 1;
        str_host = malloc (length);
        if (str_host)
        {
            snprintf (str_host, length, "%s@%s", params[2], params[3]);
            irc_nick_set_host (ptr_nick, str_host);
            free (str_host);
        }
    }

    /* update away flag in nick */
    if (ptr_channel && ptr_nick && (num_params >= 7) && (params[6][0] != '*'))
    {
        irc_nick_set_away (server, ptr_channel, ptr_nick,
                           (params[6][0] == 'G') ? 1 : 0);
    }

    /* update realname in nick */
    if (ptr_channel && ptr_nick && str_realname)
    {
        if (ptr_nick->realname)
            free (ptr_nick->realname);
        ptr_nick->realname = strdup (str_realname);
    }

    /* display output of who (manual who from user) */
    if (!ptr_channel || (ptr_channel->checking_whox <= 0))
    {
        weechat_printf_date_tags (
            irc_msgbuffer_get_target_buffer (
                server, NULL, command, "who", NULL),
            date,
            irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
            "%s%s[%s%s%s] %s%s %s(%s%s@%s%s)%s %s%s%s%s%s(%s%s%s)",
            weechat_prefix ("network"),
            IRC_COLOR_CHAT_DELIMITERS,
            IRC_COLOR_CHAT_CHANNEL,
            params[1],
            IRC_COLOR_CHAT_DELIMITERS,
            irc_nick_color_for_msg (server, 1, NULL, params[5]),
            params[5],
            IRC_COLOR_CHAT_DELIMITERS,
            IRC_COLOR_CHAT_HOST,
            params[2],
            params[3],
            IRC_COLOR_CHAT_DELIMITERS,
            IRC_COLOR_RESET,
            (num_params >= 7) ? params[6] : "",
            (num_params >= 7) ? " " : "",
            (str_hopcount) ? str_hopcount : "",
            (str_hopcount) ? " " : "",
            IRC_COLOR_CHAT_DELIMITERS,
            IRC_COLOR_RESET,
            (str_realname) ? str_realname : "",
            IRC_COLOR_CHAT_DELIMITERS);
    }

    if (str_hopcount)
        free (str_hopcount);
    if (str_realname)
        free (str_realname);

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "353": list of users on a channel.
 *
 * Command looks like:
 *   353 mynick #channel :mynick nick1 @nick2 +nick3
 *   353 mynick = #channel :mynick nick1 @nick2 +nick3
 */

IRC_PROTOCOL_CALLBACK(353)
{
    char *str_params, **str_nicks, **nicks, *nickname;
    const char *pos_channel, *pos_nick, *pos_host;
    char *prefixes, *color;
    int i, num_nicks;
    struct t_irc_channel *ptr_channel;

    IRC_PROTOCOL_MIN_PARAMS(3);

    if (irc_channel_is_channel (server, params[1]))
    {
        pos_channel = params[1];
        str_params = irc_protocol_string_params (params, 2, num_params - 1);
    }
    else
    {
        if (num_params < 4)
            return WEECHAT_RC_ERROR;
        pos_channel = params[2];
        str_params = irc_protocol_string_params (params, 3, num_params - 1);
    }

    ptr_channel = irc_channel_search (server, pos_channel);

    nicks = weechat_string_split (
        str_params,
        " ",
        NULL,
        WEECHAT_STRING_SPLIT_STRIP_LEFT
        | WEECHAT_STRING_SPLIT_STRIP_RIGHT
        | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
        0,
        &num_nicks);

    /*
     * for a channel without buffer, prepare a string that will be built
     * with nicks and colors
     */
    str_nicks = (ptr_channel) ? NULL : weechat_string_dyn_alloc (1024);

    for (i = 0; i < num_nicks; i++)
    {
        /* skip and save prefix(es) */
        pos_nick = nicks[i];
        while (pos_nick[0]
               && (irc_server_get_prefix_char_index (server, pos_nick[0]) >= 0))
        {
            pos_nick++;
        }
        prefixes = (pos_nick > nicks[i]) ?
            weechat_strndup (nicks[i], pos_nick - nicks[i]) : NULL;

        /* extract nick from host */
        pos_host = strchr (pos_nick, '!');
        if (pos_host)
        {
            nickname = weechat_strndup (pos_nick, pos_host - pos_nick);
            pos_host++;
        }
        else
        {
            nickname = strdup (pos_nick);
        }

        /* add or update nick on channel */
        if (nickname)
        {
            if (ptr_channel && ptr_channel->nicks)
            {
                if (!irc_nick_new (server, ptr_channel, nickname, pos_host,
                                   prefixes, 0, NULL, NULL))
                {
                    weechat_printf (
                        server->buffer,
                        _("%s%s: cannot create nick \"%s\" for channel \"%s\""),
                        weechat_prefix ("error"), IRC_PLUGIN_NAME, nickname,
                        ptr_channel->name);
                }
            }
            else if (!ptr_channel && str_nicks)
            {
                if (*str_nicks[0])
                {
                    weechat_string_dyn_concat (str_nicks, IRC_COLOR_RESET, -1);
                    weechat_string_dyn_concat (str_nicks, " ", -1);
                }
                if (prefixes)
                {
                    weechat_string_dyn_concat (
                        str_nicks,
                        weechat_color (
                            irc_nick_get_prefix_color_name (server,
                                                            prefixes[0])),
                        -1);
                    weechat_string_dyn_concat (str_nicks, prefixes, -1);
                }
                if (weechat_config_boolean (irc_config_look_color_nicks_in_names))
                {
                    if (irc_server_strcasecmp (server, nickname, server->nick) == 0)
                    {
                        weechat_string_dyn_concat (str_nicks,
                                                   IRC_COLOR_CHAT_NICK_SELF,
                                                   -1);
                    }
                    else
                    {
                        color = irc_nick_find_color (nickname);
                        weechat_string_dyn_concat (str_nicks, color, -1);
                        if (color)
                            free (color);
                    }
                }
                else
                {
                    weechat_string_dyn_concat (str_nicks, IRC_COLOR_RESET, -1);
                }
                weechat_string_dyn_concat (str_nicks, nickname, -1);
            }
            free (nickname);
        }
        if (prefixes)
            free (prefixes);
    }

    if (!ptr_channel)
    {
        weechat_printf_date_tags (
            irc_msgbuffer_get_target_buffer (
                server, NULL, command, "names", NULL),
            date,
            irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
            _("%sNicks %s%s%s: %s[%s%s%s]"),
            weechat_prefix ("network"),
            IRC_COLOR_CHAT_CHANNEL,
            pos_channel,
            IRC_COLOR_RESET,
            IRC_COLOR_CHAT_DELIMITERS,
            IRC_COLOR_RESET,
            (str_nicks) ? *str_nicks : "",
            IRC_COLOR_CHAT_DELIMITERS);
    }

    if (str_params)
        free (str_params);
    if (str_nicks)
        weechat_string_dyn_free (str_nicks, 1);
    if (nicks)
        weechat_string_free_split (nicks);

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "354": WHOX output
 *
 * Command looks like:
 *   354 mynick #channel user host server nick status hopcount account :real name
 */

IRC_PROTOCOL_CALLBACK(354)
{
    char *str_params, *str_host;
    int length;
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick;

    IRC_PROTOCOL_MIN_PARAMS(2);

    ptr_channel = irc_channel_search (server, params[1]);

    /*
     * if there are less than 9 arguments, we are unable to parse the message,
     * some infos are missing but we don't know which ones; in this case we
     * just display the message as-is
     */
    if (num_params < 9)
    {
        if (!ptr_channel || (ptr_channel->checking_whox <= 0))
        {
            str_params = irc_protocol_string_params (params, 2, num_params - 1);
            weechat_printf_date_tags (
                irc_msgbuffer_get_target_buffer (
                    server, NULL, command, "who", NULL),
                date,
                irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
                "%s%s[%s%s%s]%s%s%s",
                weechat_prefix ("network"),
                IRC_COLOR_CHAT_DELIMITERS,
                IRC_COLOR_CHAT_CHANNEL,
                params[1],
                IRC_COLOR_CHAT_DELIMITERS,
                IRC_COLOR_RESET,
                (str_params && str_params[0]) ? " " : "",
                (str_params && str_params[0]) ? str_params : "");
            if (str_params)
                free (str_params);
        }
        return WEECHAT_RC_OK;
    }

    ptr_nick = (ptr_channel) ?
        irc_nick_search (server, ptr_channel, params[5]) : NULL;

    /* update host in nick */
    if (ptr_nick)
    {
        length = strlen (params[2]) + 1 + strlen (params[3]) + 1;
        str_host = malloc (length);
        if (str_host)
        {
            snprintf (str_host, length, "%s@%s", params[2], params[3]);
            irc_nick_set_host (ptr_nick, str_host);
            free (str_host);
        }
    }

    /* update away flag in nick */
    if (ptr_channel && ptr_nick && (params[6][0] != '*'))
    {
        irc_nick_set_away (server, ptr_channel, ptr_nick,
                           (params[6][0] == 'G') ? 1 : 0);
    }

    /* update account in nick */
    if (ptr_nick)
    {
        if (ptr_nick->account)
            free (ptr_nick->account);
        if (ptr_channel
            && weechat_hashtable_has_key (server->cap_list, "account-notify"))
        {
            ptr_nick->account = strdup (params[8]);
        }
        else
        {
            ptr_nick->account = NULL;
        }
    }

    /* update realname in nick */
    if (ptr_nick)
    {
        if (ptr_nick->realname)
            free (ptr_nick->realname);
        ptr_nick->realname = (ptr_channel && (num_params >= 10)) ?
            strdup (params[9]) : NULL;
    }

    /* display output of who (manual who from user) */
    if (!ptr_channel || (ptr_channel->checking_whox <= 0))
    {
        weechat_printf_date_tags (
            irc_msgbuffer_get_target_buffer (
                server, NULL, command, "who", NULL),
            date,
            irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
            "%s%s[%s%s%s] %s%s %s[%s%s%s] (%s%s@%s%s)%s %s %s %s(%s%s%s)",
            weechat_prefix ("network"),
            IRC_COLOR_CHAT_DELIMITERS,
            IRC_COLOR_CHAT_CHANNEL,
            params[1],
            IRC_COLOR_CHAT_DELIMITERS,
            irc_nick_color_for_msg (server, 1, NULL, params[5]),
            params[5],
            IRC_COLOR_CHAT_DELIMITERS,
            IRC_COLOR_CHAT_HOST,
            params[8],
            IRC_COLOR_CHAT_DELIMITERS,
            IRC_COLOR_CHAT_HOST,
            params[2],
            params[3],
            IRC_COLOR_CHAT_DELIMITERS,
            IRC_COLOR_RESET,
            params[6],
            params[7],
            IRC_COLOR_CHAT_DELIMITERS,
            IRC_COLOR_RESET,
            (num_params >= 10) ? params[9] : "",
            IRC_COLOR_CHAT_DELIMITERS);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "366": end of /names list.
 *
 * Command looks like:
 *   366 mynick #channel :End of /NAMES list.
 */

IRC_PROTOCOL_CALLBACK(366)
{
    struct t_irc_channel *ptr_channel;
    struct t_infolist *infolist;
    struct t_config_option *ptr_option;
    int num_nicks, num_ops, num_halfops, num_voiced, num_regular;
    char *str_params, str_count[1024], **str_nicks, *color;
    const char *prefix, *prefix_color, *nickname;

    IRC_PROTOCOL_MIN_PARAMS(3);

    ptr_channel = irc_channel_search (server, params[1]);

    if (ptr_channel && ptr_channel->nicks)
    {
        /* display users on channel */
        if (weechat_hashtable_has_key (ptr_channel->join_msg_received, "353")
            || weechat_hashtable_has_key (irc_config_hashtable_display_join_message, "353"))
        {
            infolist = weechat_infolist_get ("nicklist", ptr_channel->buffer, NULL);
            if (infolist)
            {
                str_nicks = weechat_string_dyn_alloc (1024);
                while (weechat_infolist_next (infolist))
                {
                    if (strcmp (weechat_infolist_string (infolist, "type"),
                                "nick") == 0)
                    {
                        if (*str_nicks[0])
                        {
                            weechat_string_dyn_concat (str_nicks,
                                                       IRC_COLOR_RESET,
                                                       -1);
                            weechat_string_dyn_concat (str_nicks, " ", -1);
                        }
                        prefix = weechat_infolist_string (infolist, "prefix");
                        if (prefix[0] && (prefix[0] != ' '))
                        {
                            prefix_color = weechat_infolist_string (infolist,
                                                                    "prefix_color");
                            if (strchr (prefix_color, '.'))
                            {
                                ptr_option = weechat_config_get (weechat_infolist_string (infolist,
                                                                                          "prefix_color"));
                                if (ptr_option)
                                {
                                    weechat_string_dyn_concat (
                                        str_nicks,
                                        weechat_color (
                                            weechat_config_string (ptr_option)),
                                        -1);
                                }
                            }
                            else
                            {
                                weechat_string_dyn_concat (
                                    str_nicks,
                                    weechat_color (prefix_color),
                                    -1);
                            }
                            weechat_string_dyn_concat (str_nicks, prefix, -1);
                        }
                        nickname = weechat_infolist_string (infolist, "name");
                        if (weechat_config_boolean (irc_config_look_color_nicks_in_names))
                        {
                            if (irc_server_strcasecmp (server, nickname, server->nick) == 0)
                            {
                                weechat_string_dyn_concat (
                                    str_nicks,
                                    IRC_COLOR_CHAT_NICK_SELF,
                                    -1);
                            }
                            else
                            {
                                color = irc_nick_find_color (nickname);
                                weechat_string_dyn_concat (str_nicks, color, -1);
                                if (color)
                                    free (color);
                            }
                        }
                        else
                        {
                            weechat_string_dyn_concat (str_nicks,
                                                       IRC_COLOR_RESET,
                                                       -1);
                        }
                        weechat_string_dyn_concat (str_nicks, nickname, -1);
                    }
                }
                weechat_printf_date_tags (
                    irc_msgbuffer_get_target_buffer (
                        server, NULL, command, "names",
                        ptr_channel->buffer),
                    date,
                    irc_protocol_tags (
                        server, command, tags, NULL, NULL, NULL),
                    _("%sNicks %s%s%s: %s[%s%s]"),
                    weechat_prefix ("network"),
                    IRC_COLOR_CHAT_CHANNEL,
                    ptr_channel->name,
                    IRC_COLOR_RESET,
                    IRC_COLOR_CHAT_DELIMITERS,
                    *str_nicks,
                    IRC_COLOR_CHAT_DELIMITERS);
                weechat_string_dyn_free (str_nicks, 1);
                weechat_infolist_free (infolist);
            }
        }

        /*
         * display the number of total nicks, ops, halfops, voiced and
         * regular on the channel
         */
        if (weechat_hashtable_has_key (ptr_channel->join_msg_received, "366")
            || weechat_hashtable_has_key (irc_config_hashtable_display_join_message, "366"))
        {
            irc_nick_count (server, ptr_channel, &num_nicks, &num_ops,
                            &num_halfops, &num_voiced, &num_regular);
            str_nicks = weechat_string_dyn_alloc (1024);
            if (irc_server_get_prefix_mode_index (server, 'o') >= 0)
            {
                snprintf (str_count, sizeof (str_count),
                          "%s%s%d%s %s",
                          (*str_nicks[0]) ? ", " : "",
                          IRC_COLOR_CHAT_CHANNEL,
                          num_ops,
                          IRC_COLOR_RESET,
                          NG_("op", "ops", num_ops));
                weechat_string_dyn_concat (str_nicks, str_count, -1);
            }
            if (irc_server_get_prefix_mode_index (server, 'h') >= 0)
            {
                snprintf (str_count, sizeof (str_count),
                          "%s%s%d%s %s",
                          (*str_nicks[0]) ? ", " : "",
                          IRC_COLOR_CHAT_CHANNEL,
                          num_halfops,
                          IRC_COLOR_RESET,
                          NG_("halfop", "halfops", num_halfops));
                weechat_string_dyn_concat (str_nicks, str_count, -1);
            }
            if (irc_server_get_prefix_mode_index (server, 'v') >= 0)
            {
                snprintf (str_count, sizeof (str_count),
                          "%s%s%d%s %s",
                          (*str_nicks[0]) ? ", " : "",
                          IRC_COLOR_CHAT_CHANNEL,
                          num_voiced,
                          IRC_COLOR_RESET,
                          NG_("voiced", "voiced", num_voiced));
                weechat_string_dyn_concat (str_nicks, str_count, -1);
            }
            snprintf (
                str_count, sizeof (str_count),
                /* TRANSLATORS: number of "regular" nicks on a channel (ie not op/halfop/voiced), for example: "56 regular" */
                NG_("%s%s%d%s regular", "%s%s%d%s regular", num_regular),
                (*str_nicks[0]) ? ", " : "",
                IRC_COLOR_CHAT_CHANNEL,
                num_regular,
                IRC_COLOR_RESET);
            weechat_string_dyn_concat (str_nicks, str_count, -1);
            weechat_printf_date_tags (
                irc_msgbuffer_get_target_buffer (
                    server, NULL, command, "names", ptr_channel->buffer),
                date,
                irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
                _("%sChannel %s%s%s: %s%d%s %s %s(%s%s)"),
                weechat_prefix ("network"),
                IRC_COLOR_CHAT_CHANNEL,
                ptr_channel->name,
                IRC_COLOR_RESET,
                IRC_COLOR_CHAT_CHANNEL,
                num_nicks,
                IRC_COLOR_RESET,
                NG_("nick", "nicks", num_nicks),
                IRC_COLOR_CHAT_DELIMITERS,
                *str_nicks,
                IRC_COLOR_CHAT_DELIMITERS);
            weechat_string_dyn_free (str_nicks, 1);
        }

        if (!weechat_hashtable_has_key (ptr_channel->join_msg_received, command))
        {
            irc_command_mode_server (server, "MODE", ptr_channel, NULL,
                                     IRC_SERVER_SEND_OUTQ_PRIO_LOW);
            irc_channel_check_whox (server, ptr_channel);
        }
    }
    else
    {
        str_params = irc_protocol_string_params (params, 2, num_params - 1);
        weechat_printf_date_tags (
            irc_msgbuffer_get_target_buffer (
                server, NULL, command, "names", NULL),
            date,
            irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
            "%s%s%s%s: %s",
            weechat_prefix ("network"),
            IRC_COLOR_CHAT_CHANNEL,
            params[1],
            IRC_COLOR_RESET,
            str_params);
        if (str_params)
            free (str_params);
    }

    if (ptr_channel)
    {
        weechat_hashtable_set (ptr_channel->join_msg_received, "353", "1");
        weechat_hashtable_set (ptr_channel->join_msg_received, "366", "1");
    }

    weechat_bar_item_update ("input_prompt");

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "367": banlist.
 *
 * Command looks like:
 *   367 mynick #channel banmask nick!user@host 1205590879
 */

IRC_PROTOCOL_CALLBACK(367)
{
    struct t_irc_channel *ptr_channel;
    struct t_gui_buffer *ptr_buffer;
    struct t_irc_modelist *ptr_modelist;
    time_t datetime;
    const char *nick_address;
    char str_number[64];

    IRC_PROTOCOL_MIN_PARAMS(3);

    ptr_channel = irc_channel_search (server, params[1]);
    ptr_buffer = (ptr_channel && ptr_channel->nicks) ?
        ptr_channel->buffer : server->buffer;
    ptr_modelist = (ptr_channel) ?
        irc_modelist_search (ptr_channel, 'b') : NULL;

    str_number[0] = '\0';
    if (ptr_modelist)
    {
        /* start receiving new list */
        if (ptr_modelist->state != IRC_MODELIST_STATE_RECEIVING)
        {
            irc_modelist_item_free_all (ptr_modelist);
            ptr_modelist->state = IRC_MODELIST_STATE_RECEIVING;
        }

        snprintf (str_number, sizeof (str_number),
                  "%s[%s%d%s] ",
                  IRC_COLOR_CHAT_DELIMITERS,
                  IRC_COLOR_RESET,
                  ((ptr_modelist->last_item) ? ptr_modelist->last_item->number + 1 : 0) + 1,
                  IRC_COLOR_CHAT_DELIMITERS);
    }

    if (num_params >= 4)
    {
        nick_address = irc_protocol_nick_address (
            server, 1, NULL, irc_message_get_nick_from_host (params[3]),
            irc_message_get_address_from_host (params[3]));
        if (num_params >= 5)
        {
            datetime = (time_t)(atol (params[4]));
            if (ptr_modelist)
            {
                irc_modelist_item_new (ptr_modelist, params[2], params[3],
                                       datetime);
            }
            weechat_printf_date_tags (
                irc_msgbuffer_get_target_buffer (
                    server, NULL, command, "banlist", ptr_buffer),
                date,
                irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
                /* TRANSLATORS: "%s" after "on" is a date */
                _("%s%s[%s%s%s] %s%s%s%s banned by %s on %s"),
                weechat_prefix ("network"),
                IRC_COLOR_CHAT_DELIMITERS,
                IRC_COLOR_CHAT_CHANNEL,
                params[1],
                IRC_COLOR_CHAT_DELIMITERS,
                str_number,
                IRC_COLOR_CHAT_HOST,
                params[2],
                IRC_COLOR_RESET,
                (nick_address[0]) ? nick_address : "?",
                weechat_util_get_time_string (&datetime));
        }
        else
        {
            if (ptr_modelist)
                irc_modelist_item_new (ptr_modelist, params[2], params[3], 0);
            weechat_printf_date_tags (
                irc_msgbuffer_get_target_buffer (
                    server, NULL, command, "banlist", ptr_buffer),
                date,
                irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
                _("%s%s[%s%s%s] %s%s%s%s banned by %s"),
                weechat_prefix ("network"),
                IRC_COLOR_CHAT_DELIMITERS,
                IRC_COLOR_CHAT_CHANNEL,
                params[1],
                IRC_COLOR_CHAT_DELIMITERS,
                str_number,
                IRC_COLOR_CHAT_HOST,
                params[2],
                IRC_COLOR_RESET,
                (nick_address[0]) ? nick_address : "?");
        }
    }
    else
    {
        if (ptr_modelist)
            irc_modelist_item_new (ptr_modelist, params[2], NULL, 0);
        weechat_printf_date_tags (
            irc_msgbuffer_get_target_buffer (
                server, NULL, command, "banlist", ptr_buffer),
            date,
            irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
            _("%s%s[%s%s%s] %s%s%s%s banned"),
            weechat_prefix ("network"),
            IRC_COLOR_CHAT_DELIMITERS,
            IRC_COLOR_CHAT_CHANNEL,
            params[1],
            IRC_COLOR_CHAT_DELIMITERS,
            str_number,
            IRC_COLOR_CHAT_HOST,
            params[2],
            IRC_COLOR_RESET);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "368": end of banlist.
 *
 * Command looks like:
 *   368 mynick #channel :End of Channel Ban List
 */

IRC_PROTOCOL_CALLBACK(368)
{
    char *str_params;
    struct t_irc_channel *ptr_channel;
    struct t_gui_buffer *ptr_buffer;
    struct t_irc_modelist *ptr_modelist;

    IRC_PROTOCOL_MIN_PARAMS(2);

    str_params = (num_params > 2) ?
        irc_protocol_string_params (params, 2, num_params - 1) : NULL;

    ptr_channel = irc_channel_search (server, params[1]);
    ptr_buffer = (ptr_channel && ptr_channel->nicks) ?
        ptr_channel->buffer : server->buffer;
    ptr_modelist = (ptr_channel) ?
        irc_modelist_search (ptr_channel, 'b') : NULL;

    if (ptr_modelist)
    {
        if (ptr_modelist->state != IRC_MODELIST_STATE_RECEIVING)
        {
            /*
             * remove all items if no ban was received before
             * the end of ban list
             */
            irc_modelist_item_free_all (ptr_modelist);
        }
        ptr_modelist->state = IRC_MODELIST_STATE_RECEIVED;
    }

    weechat_printf_date_tags (
        irc_msgbuffer_get_target_buffer (
            server, NULL, command, "banlist", ptr_buffer),
        date,
        irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
        "%s%s[%s%s%s]%s%s%s",
        weechat_prefix ("network"),
        IRC_COLOR_CHAT_DELIMITERS,
        IRC_COLOR_CHAT_CHANNEL,
        params[1],
        IRC_COLOR_CHAT_DELIMITERS,
        IRC_COLOR_RESET,
        (str_params) ? " " : "",
        (str_params) ? str_params : "");

    if (str_params)
        free (str_params);

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "432": erroneous nickname.
 *
 * Command looks like (not connected to server):
 *   432 * nick :Erroneous Nickname
 *
 * Command looks like (connected to server):
 *   432 mynick nick :Erroneous Nickname
 */

IRC_PROTOCOL_CALLBACK(432)
{
    const char *alternate_nick;
    struct t_gui_buffer *ptr_buffer;

    IRC_PROTOCOL_RUN_CALLBACK(generic_error);

    if (!server->is_connected)
    {
        ptr_buffer = irc_msgbuffer_get_target_buffer (server, NULL,
                                                      command, NULL, NULL);

        alternate_nick = irc_server_get_alternate_nick (server);
        if (!alternate_nick)
        {
            weechat_printf_date_tags (
                ptr_buffer, date, NULL,
                _("%s%s: all declared nicknames are already in use or "
                  "invalid, closing connection with server"),
                weechat_prefix ("error"), IRC_PLUGIN_NAME);
            irc_server_disconnect (server, 0, 1);
            return WEECHAT_RC_OK;
        }

        weechat_printf_date_tags (
            ptr_buffer, date, NULL,
            _("%s%s: nickname \"%s\" is invalid, trying nickname \"%s\""),
            weechat_prefix ("error"), IRC_PLUGIN_NAME,
            server->nick, alternate_nick);

        irc_server_set_nick (server, alternate_nick);

        irc_server_sendf (
            server, 0, NULL,
            "NICK %s%s",
            (server->nick && strchr (server->nick, ':')) ? ":" : "",
            server->nick);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "433": nickname already in use.
 *
 * Command looks like (not connected to server):
 *   433 * nick :Nickname is already in use.
 *
 * Command looks like (connected to server):
 *   433 mynick nick :Nickname is already in use.
 */

IRC_PROTOCOL_CALLBACK(433)
{
    const char *alternate_nick;
    struct t_gui_buffer *ptr_buffer;

    if (!server->is_connected)
    {
        ptr_buffer = irc_msgbuffer_get_target_buffer (server, NULL,
                                                      command, NULL, NULL);

        alternate_nick = irc_server_get_alternate_nick (server);
        if (!alternate_nick)
        {
            weechat_printf_date_tags (
                ptr_buffer, date, NULL,
                _("%s%s: all declared nicknames are already in use, closing "
                  "connection with server"),
                weechat_prefix ("error"), IRC_PLUGIN_NAME);
            irc_server_disconnect (server, 0, 1);
            return WEECHAT_RC_OK;
        }

        weechat_printf_date_tags (
            ptr_buffer, date, NULL,
            _("%s%s: nickname \"%s\" is already in use, trying nickname "
              "\"%s\""),
            weechat_prefix ("network"), IRC_PLUGIN_NAME,
            server->nick, alternate_nick);

        irc_server_set_nick (server, alternate_nick);

        irc_server_sendf (
            server, 0, NULL,
            "NICK %s%s",
            (server->nick && strchr (server->nick, ':')) ? ":" : "",
            server->nick);
    }
    else
    {
        IRC_PROTOCOL_RUN_CALLBACK(generic_error);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "437": nick/channel temporarily unavailable.
 *
 * Command looks like:
 *   437 * mynick :Nick/channel is temporarily unavailable
 */

IRC_PROTOCOL_CALLBACK(437)
{
    const char *alternate_nick;
    struct t_gui_buffer *ptr_buffer;

    IRC_PROTOCOL_RUN_CALLBACK(generic_error);

    if (!server->is_connected)
    {
        if ((num_params >= 2)
            && (irc_server_strcasecmp (server, server->nick, params[1]) == 0))
        {
            ptr_buffer = irc_msgbuffer_get_target_buffer (server, NULL,
                                                          command, NULL, NULL);

            alternate_nick = irc_server_get_alternate_nick (server);
            if (!alternate_nick)
            {
                weechat_printf_date_tags (
                    ptr_buffer, date, NULL,
                    _("%s%s: all declared nicknames are already in use or "
                      "invalid, closing connection with server"),
                    weechat_prefix ("error"), IRC_PLUGIN_NAME);
                irc_server_disconnect (server, 0, 1);
                return WEECHAT_RC_OK;
            }

            weechat_printf_date_tags (
                ptr_buffer, date, NULL,
                _("%s%s: nickname \"%s\" is unavailable, trying nickname "
                  "\"%s\""),
                weechat_prefix ("error"), IRC_PLUGIN_NAME,
                server->nick, alternate_nick);

            irc_server_set_nick (server, alternate_nick);

            irc_server_sendf (
                server, 0, NULL,
                "NICK %s%s",
                (server->nick && strchr (server->nick, ':')) ? ":" : "",
                server->nick);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "438": not authorized to change nickname.
 *
 * Command looks like:
 *   438 mynick newnick :Nick change too fast. Please wait 30 seconds.
 */

IRC_PROTOCOL_CALLBACK(438)
{
    char *str_params;
    struct t_gui_buffer *ptr_buffer;

    IRC_PROTOCOL_MIN_PARAMS(2);

    ptr_buffer = irc_msgbuffer_get_target_buffer (server, NULL,
                                                  command, NULL, NULL);

    if (num_params >= 3)
    {
        str_params = irc_protocol_string_params (params, 2, num_params - 1);
        weechat_printf_date_tags (
            ptr_buffer,
            date,
            irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
            "%s%s (%s => %s)",
            weechat_prefix ("network"),
            str_params,
            params[0],
            params[1]);
        if (str_params)
            free (str_params);
    }
    else
    {
        weechat_printf_date_tags (
            ptr_buffer,
            date,
            irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
            "%s%s %s",
            weechat_prefix ("network"),
            params[0],
            params[1]);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "470": forwarding to another channel.
 *
 * Command looks like:
 *   470 mynick #channel ##channel :Forwarding to another channel
 */

IRC_PROTOCOL_CALLBACK(470)
{
    struct t_gui_buffer *ptr_buffer;
    struct t_gui_lines *own_lines;
    const char *short_name, *localvar_channel;
    char *old_channel_lower, *new_channel_lower, *buffer_name;
    int lines_count;

    IRC_PROTOCOL_RUN_CALLBACK(generic_error);

    if ((num_params >= 3) && !irc_channel_search (server, params[1]))
    {
        ptr_buffer = irc_channel_search_buffer (server,
                                                IRC_CHANNEL_TYPE_CHANNEL,
                                                params[1]);
        if (ptr_buffer)
        {
            short_name = weechat_buffer_get_string (ptr_buffer, "short_name");
            localvar_channel = weechat_buffer_get_string (ptr_buffer,
                                                          "localvar_channel");
            if (!short_name
                || (localvar_channel
                    && (strcmp (localvar_channel, short_name) == 0)))
            {
                /*
                 * update the short_name only if it was not changed by the
                 * user
                 */
                weechat_buffer_set (ptr_buffer, "short_name", params[2]);
            }
            buffer_name = irc_buffer_build_name (server->name, params[2]);
            weechat_buffer_set (ptr_buffer, "name", buffer_name);
            weechat_buffer_set (ptr_buffer, "localvar_set_channel", params[2]);
            if (buffer_name)
                free (buffer_name);

            /*
             * check if logger backlog should be displayed for the new channel
             * name: it is displayed only if the buffer is currently completely
             * empty (no messages at all)
             */
            lines_count = 0;
            own_lines = weechat_hdata_pointer (weechat_hdata_get ("buffer"),
                                               ptr_buffer, "own_lines");
            if (own_lines)
            {
                lines_count = weechat_hdata_integer (
                    weechat_hdata_get ("lines"),
                    own_lines, "lines_count");
            }
            if (lines_count == 0)
            {
                (void) weechat_hook_signal_send ("logger_backlog",
                                                 WEECHAT_HOOK_SIGNAL_POINTER,
                                                 ptr_buffer);
            }
            if (IRC_SERVER_OPTION_BOOLEAN(server,
                                          IRC_SERVER_OPTION_AUTOJOIN_DYNAMIC))
            {
                irc_join_rename_channel_in_autojoin (server, params[1],
                                                     params[2]);
            }
        }

        old_channel_lower = weechat_string_tolower (params[1]);
        if (old_channel_lower)
        {
            new_channel_lower = weechat_string_tolower (params[2]);
            if (new_channel_lower)
            {
                if (weechat_hashtable_has_key (server->join_manual,
                                               old_channel_lower))
                {
                    weechat_hashtable_set (server->join_manual,
                                           new_channel_lower,
                                           weechat_hashtable_get (
                                               server->join_manual,
                                               old_channel_lower));
                    weechat_hashtable_remove (server->join_manual,
                                              old_channel_lower);
                }
                if (weechat_hashtable_has_key (server->join_noswitch,
                                               old_channel_lower))
                {
                    weechat_hashtable_set (server->join_noswitch,
                                           new_channel_lower,
                                           weechat_hashtable_get (
                                               server->join_noswitch,
                                               old_channel_lower));
                    weechat_hashtable_remove (server->join_noswitch,
                                              old_channel_lower);
                }
                free (new_channel_lower);
            }
            free (old_channel_lower);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC commands "524", "704", "705", and "706": help reply.
 *
 * Commands look like:
 *   704 mynick topic :First help line of <topic>
 *   705 mynick topic :The <topic> is blah blah
 *   705 mynick topic :and this
 *   705 mynick topic :and that.
 *   706 mynick topic :End of /HELPOP
 *
 * Or:
 *   524 mynick topic :help not found
 */

IRC_PROTOCOL_CALLBACK(help)
{
    char *str_message;

    IRC_PROTOCOL_MIN_PARAMS(2);

    if (ignored)
        return WEECHAT_RC_OK;

    str_message = irc_protocol_string_params (params, 2, num_params - 1);

    weechat_printf_date_tags (
        irc_msgbuffer_get_target_buffer (server, nick, command, NULL, NULL),
        date,
        irc_protocol_tags (server, command, tags, "notify_private", nick,
                           address),
        "%s%s",
        weechat_prefix ("network"),
        str_message);

    if (str_message)
        free (str_message);

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "710": has asked for an invite (knock).
 *
 * Command looks like:
 *   710 #channel #channel nick!user@host :has asked for an invite.
 */

IRC_PROTOCOL_CALLBACK(710)
{
    struct t_irc_channel *ptr_channel;
    const char *nick_address;
    char *str_message;

    IRC_PROTOCOL_MIN_PARAMS(3);

    if (ignored)
        return WEECHAT_RC_OK;

    ptr_channel = irc_channel_search (server, params[1]);
    if (!ptr_channel)
        return WEECHAT_RC_ERROR;

    nick_address = irc_protocol_nick_address (
        server, 1, NULL, irc_message_get_nick_from_host (params[2]),
        irc_message_get_address_from_host (params[2]));

    str_message = irc_protocol_string_params (params, 3, num_params - 1);

    weechat_printf_date_tags (
        irc_msgbuffer_get_target_buffer (
            server, NULL, command, NULL, ptr_channel->buffer),
        date,
        irc_protocol_tags (server, command, tags, "notify_message", NULL, NULL),
        "%s%s %s",
        weechat_prefix ("network"),
        (nick_address[0]) ? nick_address : "?",
        (str_message && str_message[0]) ?
        str_message : _("has asked for an invite"));

    if (str_message)
        free (str_message);

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "728": quietlist.
 *
 * Command looks like:
 *   728 mynick #channel mode quietmask nick!user@host 1351350090
 */

IRC_PROTOCOL_CALLBACK(728)
{
    struct t_irc_channel *ptr_channel;
    struct t_gui_buffer *ptr_buffer;
    struct t_irc_modelist *ptr_modelist;
    time_t datetime;
    const char *nick_address;
    char str_number[64];

    IRC_PROTOCOL_MIN_PARAMS(4);

    ptr_channel = irc_channel_search (server, params[1]);
    ptr_buffer = (ptr_channel && ptr_channel->nicks) ?
        ptr_channel->buffer : server->buffer;
    ptr_modelist = (ptr_channel) ?
        irc_modelist_search (ptr_channel, params[2][0]) : NULL;

    str_number[0] = '\0';
    if (ptr_modelist)
    {
        /* start receiving new list */
        if (ptr_modelist->state != IRC_MODELIST_STATE_RECEIVING)
        {
            irc_modelist_item_free_all (ptr_modelist);
            ptr_modelist->state = IRC_MODELIST_STATE_RECEIVING;
        }

        snprintf (str_number, sizeof (str_number),
                  "%s[%s%d%s] ",
                  IRC_COLOR_CHAT_DELIMITERS,
                  IRC_COLOR_RESET,
                  ((ptr_modelist->last_item) ? ptr_modelist->last_item->number + 1 : 0) + 1,
                  IRC_COLOR_CHAT_DELIMITERS);
    }

    if (num_params >= 5)
    {
        nick_address = irc_protocol_nick_address (
            server, 1, NULL, irc_message_get_nick_from_host (params[4]),
            irc_message_get_address_from_host (params[4]));
        if (num_params >= 6)
        {
            datetime = (time_t)(atol (params[5]));
            if (ptr_modelist)
            {
                irc_modelist_item_new (ptr_modelist, params[3], params[4],
                                       datetime);
            }
            weechat_printf_date_tags (
                irc_msgbuffer_get_target_buffer (
                    server, NULL, command, "quietlist", ptr_buffer),
                date,
                irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
                /* TRANSLATORS: "%s" after "on" is a date */
                _("%s%s[%s%s%s] %s%s%s%s quieted by %s on %s"),
                weechat_prefix ("network"),
                IRC_COLOR_CHAT_DELIMITERS,
                IRC_COLOR_CHAT_CHANNEL,
                params[1],
                IRC_COLOR_CHAT_DELIMITERS,
                str_number,
                IRC_COLOR_CHAT_HOST,
                params[3],
                IRC_COLOR_RESET,
                (nick_address[0]) ? nick_address : "?",
                weechat_util_get_time_string (&datetime));
        }
        else
        {
            if (ptr_modelist)
                irc_modelist_item_new (ptr_modelist, params[3], params[4], 0);
            weechat_printf_date_tags (
                irc_msgbuffer_get_target_buffer (
                    server, NULL, command, "quietlist", ptr_buffer),
                date,
                irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
                _("%s%s[%s%s%s] %s%s%s%s quieted by %s"),
                weechat_prefix ("network"),
                IRC_COLOR_CHAT_DELIMITERS,
                IRC_COLOR_CHAT_CHANNEL,
                params[1],
                IRC_COLOR_CHAT_DELIMITERS,
                str_number,
                IRC_COLOR_CHAT_HOST,
                params[3],
                IRC_COLOR_RESET,
                (nick_address[0]) ? nick_address : "?");
        }
    }
    else
    {
        if (ptr_modelist)
            irc_modelist_item_new (ptr_modelist, params[3], NULL, 0);
        weechat_printf_date_tags (
            irc_msgbuffer_get_target_buffer (
                server, NULL, command, "quietlist", ptr_buffer),
            date,
            irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
            _("%s%s[%s%s%s] %s%s%s%s quieted"),
            weechat_prefix ("network"),
            IRC_COLOR_CHAT_DELIMITERS,
            IRC_COLOR_CHAT_CHANNEL,
            params[1],
            IRC_COLOR_CHAT_DELIMITERS,
            str_number,
            IRC_COLOR_CHAT_HOST,
            params[3],
            IRC_COLOR_RESET);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "729": end of quietlist.
 *
 * Command looks like:
 *   729 mynick #channel mode :End of Channel Quiet List
 */

IRC_PROTOCOL_CALLBACK(729)
{
    char *str_params;
    struct t_irc_channel *ptr_channel;
    struct t_gui_buffer *ptr_buffer;
    struct t_irc_modelist *ptr_modelist;

    IRC_PROTOCOL_MIN_PARAMS(3);

    str_params = (num_params > 3) ?
        irc_protocol_string_params (params, 3, num_params - 1) : NULL;

    ptr_channel = irc_channel_search (server, params[1]);
    ptr_buffer = (ptr_channel && ptr_channel->nicks) ?
        ptr_channel->buffer : server->buffer;
    ptr_modelist = (ptr_channel) ?
        irc_modelist_search (ptr_channel, params[2][0]) : NULL;

    if (ptr_modelist)
    {
        if (ptr_modelist->state != IRC_MODELIST_STATE_RECEIVING)
        {
            /*
             * remove all items if no quiet was received before
             * the end of quiet list
             */
            irc_modelist_item_free_all (ptr_modelist);
        }
        ptr_modelist->state = IRC_MODELIST_STATE_RECEIVED;
    }

    weechat_printf_date_tags (
        irc_msgbuffer_get_target_buffer (
            server, NULL, command, "quietlist", ptr_buffer),
        date,
        irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
        "%s%s[%s%s%s]%s%s%s",
        weechat_prefix ("network"),
        IRC_COLOR_CHAT_DELIMITERS,
        IRC_COLOR_CHAT_CHANNEL,
        params[1],
        IRC_COLOR_CHAT_DELIMITERS,
        IRC_COLOR_RESET,
        (str_params) ? " " : "",
        (str_params) ? str_params : "");

    if (str_params)
        free (str_params);

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "730": monitored nicks are online
 * (RPL_MONONLINE).
 *
 * Command looks like:
 *   730 mynick :nick1!user1@host1,nick2!user2@host2
 */

IRC_PROTOCOL_CALLBACK(730)
{
    struct t_irc_notify *ptr_notify;
    const char *monitor_nick, *monitor_host;
    char *str_nicks, **nicks;
    int i, num_nicks;

    IRC_PROTOCOL_MIN_PARAMS(2);

    str_nicks = irc_protocol_string_params (params, 1, num_params - 1);

    nicks = weechat_string_split (str_nicks,
                                  ",",
                                  NULL,
                                  WEECHAT_STRING_SPLIT_STRIP_LEFT
                                  | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                  | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                  0,
                                  &num_nicks);
    if (nicks)
    {
        for (i = 0; i < num_nicks; i++)
        {
            monitor_nick = irc_message_get_nick_from_host (nicks[i]);
            monitor_host = strchr (nicks[i], '!');
            if (monitor_host)
                monitor_host++;
            ptr_notify = irc_notify_search (server, monitor_nick);
            if (ptr_notify)
                irc_notify_set_is_on_server (ptr_notify, monitor_host, 1);
        }
        weechat_string_free_split (nicks);
    }

    if (str_nicks)
        free (str_nicks);

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "731": monitored nicks are offline
 * (RPL_MONOFFLINE).
 *
 * Command looks like:
 *   731 mynick :nick1!user1@host1,nick2!user2@host2
 */

IRC_PROTOCOL_CALLBACK(731)
{
    struct t_irc_notify *ptr_notify;
    const char *monitor_nick, *monitor_host;
    char *str_nicks, **nicks;
    int i, num_nicks;

    IRC_PROTOCOL_MIN_PARAMS(2);

    str_nicks = irc_protocol_string_params (params, 1, num_params - 1);

    nicks = weechat_string_split (str_nicks,
                                  ",",
                                  NULL,
                                  WEECHAT_STRING_SPLIT_STRIP_LEFT
                                  | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                  | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                  0,
                                  &num_nicks);
    if (nicks)
    {
        for (i = 0; i < num_nicks; i++)
        {
            monitor_nick = irc_message_get_nick_from_host (nicks[i]);
            monitor_host = strchr (nicks[i], '!');
            if (monitor_host)
                monitor_host++;
            ptr_notify = irc_notify_search (server, monitor_nick);
            if (ptr_notify)
                irc_notify_set_is_on_server (ptr_notify, monitor_host, 0);
        }
        weechat_string_free_split (nicks);
    }

    if (str_nicks)
        free (str_nicks);

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "732": list of monitored nicks (RPL_MONLIST).
 *
 * Command looks like:
 *   732 mynick :nick1!user1@host1,nick2!user2@host2
 */

IRC_PROTOCOL_CALLBACK(732)
{
    char *str_nicks;

    IRC_PROTOCOL_MIN_PARAMS(1);

    str_nicks = (num_params > 1) ?
        irc_protocol_string_params (params, 1, num_params - 1) : NULL;

    weechat_printf_date_tags (
        irc_msgbuffer_get_target_buffer (
            server, NULL, command, "monitor", NULL),
        date,
        irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
        "%s%s",
        weechat_prefix ("network"),
        (str_nicks) ? str_nicks : "");

    if (str_nicks)
        free (str_nicks);

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "733": end of a monitor list (RPL_ENDOFMONLIST).
 *
 * Command looks like:
 *   733 mynick :End of MONITOR list
 */

IRC_PROTOCOL_CALLBACK(733)
{
    char *str_params;

    IRC_PROTOCOL_MIN_PARAMS(1);

    str_params = (num_params > 1) ?
        irc_protocol_string_params (params, 1, num_params - 1) : NULL;

    weechat_printf_date_tags (
        irc_msgbuffer_get_target_buffer (
            server, NULL, command, "monitor", NULL),
        date,
        irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
        "%s%s",
        weechat_prefix ("network"),
        (str_params) ? str_params : "");

    if (str_params)
        free (str_params);

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "734": monitor list is full (ERR_MONLISTFULL)
 *
 * Command looks like:
 *   734 mynick limit nick1,nick2 :Monitor list is full.
 */

IRC_PROTOCOL_CALLBACK(734)
{
    char *str_params;

    IRC_PROTOCOL_MIN_PARAMS(3);

    str_params = (num_params > 3) ?
        irc_protocol_string_params (params, 3, num_params - 1) : NULL;

    weechat_printf_date_tags (
        irc_msgbuffer_get_target_buffer (
            server, NULL, command, "monitor", NULL),
        date,
        irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
        "%s%s (%s)",
        weechat_prefix ("error"),
        (str_params) ? str_params : "",
        params[1]);

    if (str_params)
        free (str_params);

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "900": logged in as (SASL).
 *
 * Command looks like:
 *   900 mynick nick!user@host mynick :You are now logged in as mynick
 *   900 * * mynick :You are now logged in as mynick
 */

IRC_PROTOCOL_CALLBACK(900)
{
    char *str_params;
    const char *pos_nick_host;

    IRC_PROTOCOL_MIN_PARAMS(4);

    pos_nick_host = (strcmp (params[1], "*") != 0) ? params[1] : NULL;

    str_params = irc_protocol_string_params (params, 3, num_params - 1);

    if (pos_nick_host)
    {
        weechat_printf_date_tags (
            irc_msgbuffer_get_target_buffer (server, NULL, command, NULL, NULL),
            date,
            irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
            "%s%s %s(%s%s%s)",
            weechat_prefix ("network"),
            str_params,
            IRC_COLOR_CHAT_DELIMITERS,
            IRC_COLOR_CHAT_HOST,
            pos_nick_host,
            IRC_COLOR_CHAT_DELIMITERS);
    }
    else
    {
        weechat_printf_date_tags (
            irc_msgbuffer_get_target_buffer (server, NULL, command, NULL, NULL),
            date,
            irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
            "%s%s",
            weechat_prefix ("network"),
            str_params);
    }

    irc_server_free_sasl_data (server);

    if (str_params)
        free (str_params);

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC command "901": you are now logged out.
 *
 * Command looks like:
 *   901 mynick nick!user@host :You are now logged out.
 */

IRC_PROTOCOL_CALLBACK(901)
{
    IRC_PROTOCOL_MIN_PARAMS(3);

    weechat_printf_date_tags (
        irc_msgbuffer_get_target_buffer (server, NULL, command, NULL, NULL),
        date,
        irc_protocol_tags (server, command, tags, NULL, NULL, NULL),
        "%s%s",
        weechat_prefix ("network"),
        params[2]);

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC commands "903" and "907" (SASL OK).
 *
 * Commands look like:
 *   903 nick :Authentication successful
 *   903 * :Authentication successful
 */

IRC_PROTOCOL_CALLBACK(sasl_end_ok)
{
    if (server->hook_timer_sasl)
    {
        weechat_unhook (server->hook_timer_sasl);
        server->hook_timer_sasl = NULL;
    }

    IRC_PROTOCOL_RUN_CALLBACK(numeric);

    server->authentication_method = IRC_SERVER_AUTH_METHOD_SASL;

    if (!server->is_connected)
        irc_server_sendf (server, 0, NULL, "CAP END");

    irc_server_free_sasl_data (server);

    return WEECHAT_RC_OK;
}

/*
 * Callback for the IRC commands "902", "904", "905", "906" (SASL failed).
 *
 * Commands look like:
 *   904 nick :SASL authentication failed
 */

IRC_PROTOCOL_CALLBACK(sasl_end_fail)
{
    int sasl_fail;

    if (server->hook_timer_sasl)
    {
        weechat_unhook (server->hook_timer_sasl);
        server->hook_timer_sasl = NULL;
    }

    server->authentication_method = IRC_SERVER_AUTH_METHOD_NONE;
    server->sasl_mechanism_used = -1;

    IRC_PROTOCOL_RUN_CALLBACK(numeric);

    sasl_fail = IRC_SERVER_OPTION_INTEGER(server, IRC_SERVER_OPTION_SASL_FAIL);
    if (!server->is_connected
        && ((sasl_fail == IRC_SERVER_SASL_FAIL_RECONNECT)
            || (sasl_fail == IRC_SERVER_SASL_FAIL_DISCONNECT)))
    {
        irc_server_disconnect (
            server, 0,
            (sasl_fail == IRC_SERVER_SASL_FAIL_RECONNECT) ? 1 : 0);
        return WEECHAT_RC_OK;
    }

    if (!server->is_connected)
        irc_server_sendf (server, 0, NULL, "CAP END");

    irc_server_free_sasl_data (server);

    return WEECHAT_RC_OK;
}

/*
 * Executes action when an IRC command is received.
 *
 * Argument "irc_message" is the full message without optional tags.
 *
 * If ignore_batch_tag == 0, a message with tag "batch=xxx" is stored in this
 * batch reference for further processing.
 * If ignore_batch_tag == 1, the "batch" tag is ignored and message is
 * processed immediately (this is the case when command BATCH is received
 * with "-reference", which means end of batched events).
 */

void
irc_protocol_recv_command (struct t_irc_server *server,
                           const char *irc_message,
                           const char *msg_command,
                           const char *msg_channel,
                           int ignore_batch_tag)
{
    int i, cmd_found, return_code, decode_color, keep_trailing_spaces;
    int message_ignored, num_params;
    char *message_colors_decoded, *msg_to_parse, *pos_space, *tags, **params;
    struct t_irc_channel *ptr_channel;
    t_irc_recv_func *cmd_recv_func;
    const char *cmd_name, *ptr_msg_after_tags, *ptr_batch_ref;
    time_t date;
    const char *nick1, *address1, *host1;
    char *nick, *address, *address_color, *host, *host_no_color, *host_color;
    struct t_hashtable *hash_tags;

    struct t_irc_protocol_msg irc_protocol_messages[] = {
        /* format: "command", decode_color, keep_trailing_spaces, func_cb   */
        IRCB(account, 1, 0, account),    /* account (cap "account-notify")  */
        IRCB(authenticate, 1, 0, authenticate), /* authenticate             */
        IRCB(away, 1, 0, away),          /* away (cap "away-notify")        */
        IRCB(batch, 1, 0, batch),        /* batch (cap "batch")             */
        IRCB(cap, 1, 0, cap),            /* client capability               */
        IRCB(chghost, 1, 0, chghost),    /* user/host change (cap "chghost")*/
        IRCB(error, 1, 0, error),        /* error received from server      */
        IRCB(fail, 1, 0, fail),          /* error received from server      */
        IRCB(invite, 1, 0, invite),      /* invite a nick on a channel      */
        IRCB(join, 1, 0, join),          /* join a channel                  */
        IRCB(kick, 1, 1, kick),          /* kick a user                     */
        IRCB(kill, 1, 1, kill),          /* close client-server connection  */
        IRCB(mode, 1, 0, mode),          /* change channel or user mode     */
        IRCB(nick, 1, 0, nick),          /* change current nickname         */
        IRCB(note, 1, 0, note),          /* note received from server       */
        IRCB(notice, 1, 1, notice),      /* send notice message to user     */
        IRCB(part, 1, 1, part),          /* leave a channel                 */
        IRCB(ping, 1, 0, ping),          /* ping server                     */
        IRCB(pong, 1, 0, pong),          /* answer to a ping message        */
        IRCB(privmsg, 1, 1, privmsg),    /* message received                */
        IRCB(quit, 1, 1, quit),          /* close all connections and quit  */
        IRCB(setname, 0, 1, setname),    /* set realname                    */
        IRCB(tagmsg, 0, 0, tagmsg),      /* tag message                     */
        IRCB(topic, 0, 1, topic),        /* get/set channel topic           */
        IRCB(wallops, 1, 1, wallops),    /* wallops                         */
        IRCB(warn, 1, 0, warn),          /* warning received from server    */
        IRCB(001, 1, 0, 001),            /* a server message                */
        IRCB(005, 1, 0, 005),            /* a server message                */
        IRCB(008, 1, 0, 008),            /* server notice mask              */
        IRCB(221, 1, 0, 221),            /* user mode string                */
        IRCB(223, 1, 0, whois_nick_msg), /* whois (charset is)              */
        IRCB(264, 1, 0, whois_nick_msg), /* whois (encrypted connection)    */
        IRCB(275, 1, 0, whois_nick_msg), /* whois (secure connection)       */
        IRCB(276, 1, 0, whois_nick_msg), /* whois (client cert. fingerprint)*/
        IRCB(301, 1, 1, 301),            /* away message                    */
        IRCB(303, 1, 0, 303),            /* ison                            */
        IRCB(305, 1, 0, 305),            /* unaway                          */
        IRCB(306, 1, 0, 306),            /* now away                        */
        IRCB(307, 1, 0, whois_nick_msg), /* whois (registered nick)         */
        IRCB(310, 1, 0, whois_nick_msg), /* whois (help mode)               */
        IRCB(311, 1, 0, 311),            /* whois (user)                    */
        IRCB(312, 1, 0, 312),            /* whois (server)                  */
        IRCB(313, 1, 0, whois_nick_msg), /* whois (operator)                */
        IRCB(314, 1, 0, 314),            /* whowas                          */
        IRCB(315, 1, 0, 315),            /* end of /who list                */
        IRCB(317, 1, 0, 317),            /* whois (idle)                    */
        IRCB(318, 1, 0, whois_nick_msg), /* whois (end)                     */
        IRCB(319, 1, 0, whois_nick_msg), /* whois (channels)                */
        IRCB(320, 1, 0, whois_nick_msg), /* whois (identified user)         */
        IRCB(321, 1, 0, 321),            /* /list start                     */
        IRCB(322, 1, 1, 322),            /* channel (for /list)             */
        IRCB(323, 1, 0, 323),            /* end of /list                    */
        IRCB(324, 1, 0, 324),            /* channel mode                    */
        IRCB(326, 1, 0, whois_nick_msg), /* whois (has oper privs)          */
        IRCB(327, 1, 0, 327),            /* whois (host)                    */
        IRCB(328, 1, 0, 328),            /* channel URL                     */
        IRCB(329, 1, 0, 329),            /* channel creation date           */
        IRCB(330, 1, 0, 330_343),        /* is logged in as                 */
        IRCB(331, 1, 0, 331),            /* no topic for channel            */
        IRCB(332, 0, 1, 332),            /* topic of channel                */
        IRCB(333, 1, 0, 333),            /* topic info (nick/date)          */
        IRCB(335, 1, 0, whois_nick_msg), /* is a bot on                     */
        IRCB(338, 1, 0, 338),            /* whois (host)                    */
        IRCB(341, 1, 0, 341),            /* inviting                        */
        IRCB(343, 1, 0, 330_343),        /* is opered as                    */
        IRCB(344, 1, 0, 344),            /* channel reop / whois (geo info) */
        IRCB(345, 1, 0, 345),            /* end of channel reop list        */
        IRCB(346, 1, 0, 346),            /* invite list                     */
        IRCB(347, 1, 0, 347),            /* end of invite list              */
        IRCB(348, 1, 0, 348),            /* channel exception list          */
        IRCB(349, 1, 0, 349),            /* end of channel exception list   */
        IRCB(350, 1, 0, 350),            /* whois (gateway)                 */
        IRCB(351, 1, 0, 351),            /* server version                  */
        IRCB(352, 1, 0, 352),            /* who                             */
        IRCB(353, 1, 0, 353),            /* list of nicks on channel        */
        IRCB(354, 1, 0, 354),            /* whox                            */
        IRCB(366, 1, 0, 366),            /* end of /names list              */
        IRCB(367, 1, 0, 367),            /* banlist                         */
        IRCB(368, 1, 0, 368),            /* end of banlist                  */
        IRCB(369, 1, 0, whowas_nick_msg), /* whowas (end)                   */
        IRCB(378, 1, 0, whois_nick_msg), /* whois (connecting from)         */
        IRCB(379, 1, 0, whois_nick_msg), /* whois (using modes)             */
        IRCB(401, 1, 0, generic_error),  /* no such nick/channel            */
        IRCB(402, 1, 0, generic_error),  /* no such server                  */
        IRCB(403, 1, 0, generic_error),  /* no such channel                 */
        IRCB(404, 1, 0, generic_error),  /* cannot send to channel          */
        IRCB(405, 1, 0, generic_error),  /* too many channels               */
        IRCB(406, 1, 0, generic_error),  /* was no such nick                */
        IRCB(407, 1, 0, generic_error),  /* was no such nick                */
        IRCB(409, 1, 0, generic_error),  /* no origin                       */
        IRCB(410, 1, 0, generic_error),  /* no services                     */
        IRCB(411, 1, 0, generic_error),  /* no recipient                    */
        IRCB(412, 1, 0, generic_error),  /* no text to send                 */
        IRCB(413, 1, 0, generic_error),  /* no toplevel                     */
        IRCB(414, 1, 0, generic_error),  /* wilcard in toplevel domain      */
        IRCB(415, 1, 0, generic_error),  /* cannot send message to channel  */
        IRCB(421, 1, 0, generic_error),  /* unknown command                 */
        IRCB(422, 1, 0, generic_error),  /* MOTD is missing                 */
        IRCB(423, 1, 0, generic_error),  /* no administrative info          */
        IRCB(424, 1, 0, generic_error),  /* file error                      */
        IRCB(431, 1, 0, generic_error),  /* no nickname given               */
        IRCB(432, 1, 0, 432),            /* erroneous nickname              */
        IRCB(433, 1, 0, 433),            /* nickname already in use         */
        IRCB(436, 1, 0, generic_error),  /* nickname collision              */
        IRCB(437, 1, 0, 437),            /* nick/channel unavailable        */
        IRCB(438, 1, 0, 438),            /* not auth. to change nickname    */
        IRCB(441, 1, 0, generic_error),  /* user not in channel             */
        IRCB(442, 1, 0, generic_error),  /* not on channel                  */
        IRCB(443, 1, 0, generic_error),  /* user already on channel         */
        IRCB(444, 1, 0, generic_error),  /* user not logged in              */
        IRCB(445, 1, 0, generic_error),  /* summon has been disabled        */
        IRCB(446, 1, 0, generic_error),  /* users has been disabled         */
        IRCB(451, 1, 0, generic_error),  /* you are not registered          */
        IRCB(461, 1, 0, generic_error),  /* not enough parameters           */
        IRCB(462, 1, 0, generic_error),  /* you may not register            */
        IRCB(463, 1, 0, generic_error),  /* host not privileged             */
        IRCB(464, 1, 0, generic_error),  /* password incorrect              */
        IRCB(465, 1, 0, generic_error),  /* banned from this server         */
        IRCB(467, 1, 0, generic_error),  /* channel key already set         */
        IRCB(470, 1, 0, 470),            /* forwarding to another channel   */
        IRCB(471, 1, 0, generic_error),  /* channel is already full         */
        IRCB(472, 1, 0, generic_error),  /* unknown mode char to me         */
        IRCB(473, 1, 0, generic_error),  /* cannot join (invite only)       */
        IRCB(474, 1, 0, generic_error),  /* cannot join (banned)            */
        IRCB(475, 1, 0, generic_error),  /* cannot join (bad key)           */
        IRCB(476, 1, 0, generic_error),  /* bad channel mask                */
        IRCB(477, 1, 0, generic_error),  /* channel doesn't support modes   */
        IRCB(481, 1, 0, generic_error),  /* you're not an IRC operator      */
        IRCB(482, 1, 0, generic_error),  /* you're not channel operator     */
        IRCB(483, 1, 0, generic_error),  /* you can't kill a server!        */
        IRCB(484, 1, 0, generic_error),  /* your connection is restricted!  */
        IRCB(485, 1, 0, generic_error),  /* user immune from kick/deop      */
        IRCB(487, 1, 0, generic_error),  /* network split                   */
        IRCB(491, 1, 0, generic_error),  /* no O-lines for your host        */
        IRCB(501, 1, 0, generic_error),  /* unknown mode flag               */
        IRCB(502, 1, 0, generic_error),  /* can't chg mode for other users  */
        IRCB(524, 1, 0, help),           /* HELP/HELPOP (help not found)    */
        IRCB(671, 1, 0, whois_nick_msg), /* whois (secure connection)       */
        IRCB(704, 1, 0, help),           /* start of HELP/HELPOP            */
        IRCB(705, 1, 0, help),           /* body of HELP/HELPOP             */
        IRCB(706, 1, 0, help),           /* end of HELP/HELPOP              */
        IRCB(710, 1, 0, 710),            /* knock: has asked for an invite  */
        IRCB(711, 1, 0, knock_reply),    /* knock: has been delivered       */
        IRCB(712, 1, 0, knock_reply),    /* knock: too many knocks          */
        IRCB(713, 1, 0, knock_reply),    /* knock: channel is open          */
        IRCB(714, 1, 0, knock_reply),    /* knock: already on that channel  */
        IRCB(728, 1, 0, 728),            /* quietlist                       */
        IRCB(729, 1, 0, 729),            /* end of quietlist                */
        IRCB(730, 1, 0, 730),            /* monitored nicks online          */
        IRCB(731, 1, 0, 731),            /* monitored nicks offline         */
        IRCB(732, 1, 0, 732),            /* list of monitored nicks         */
        IRCB(733, 1, 0, 733),            /* end of monitor list             */
        IRCB(734, 1, 0, 734),            /* monitor list is full            */
        IRCB(742, 1, 0, generic_error),  /* mode cannot be set              */
        IRCB(900, 1, 0, 900),            /* logged in as (SASL)             */
        IRCB(901, 1, 0, 901),            /* you are now logged out          */
        IRCB(902, 1, 0, sasl_end_fail),  /* SASL auth failed (acc. locked)  */
        IRCB(903, 1, 0, sasl_end_ok),    /* SASL auth successful            */
        IRCB(904, 1, 0, sasl_end_fail),  /* SASL auth failed                */
        IRCB(905, 1, 0, sasl_end_fail),  /* SASL message too long           */
        IRCB(906, 1, 0, sasl_end_fail),  /* SASL authentication aborted     */
        IRCB(907, 1, 0, sasl_end_ok),    /* already completed SASL auth     */
        IRCB(936, 1, 0, generic_error),  /* censored word                   */
        IRCB(973, 1, 0, server_mode_reason), /* whois (secure conn.)        */
        IRCB(974, 1, 0, server_mode_reason), /* whois (secure conn.)        */
        IRCB(975, 1, 0, server_mode_reason), /* whois (secure conn.)        */
        { NULL, 0, 0, NULL },
    };

    if (!msg_command)
        return;

    nick = NULL;
    address = NULL;
    address_color = NULL;
    host = NULL;
    host_no_color = NULL;
    host_color = NULL;
    message_colors_decoded = NULL;
    msg_to_parse = NULL;
    date = 0;
    hash_tags = NULL;

    ptr_msg_after_tags = irc_message;

    /* get tags as hashtable */
    if (irc_message && (irc_message[0] == '@'))
    {
        pos_space = strchr (irc_message, ' ');
        if (pos_space)
        {
            tags = weechat_strndup (irc_message + 1,
                                    pos_space - (irc_message + 1));
            if (tags)
            {
                hash_tags = weechat_hashtable_new (32,
                                                   WEECHAT_HASHTABLE_STRING,
                                                   WEECHAT_HASHTABLE_STRING,
                                                   NULL, NULL);
                if (hash_tags)
                {
                    irc_tag_parse (tags, hash_tags, NULL);
                    date = irc_protocol_parse_time (
                        weechat_hashtable_get (hash_tags, "time"));
                }
                free (tags);
            }
            ptr_msg_after_tags = pos_space;
            while (ptr_msg_after_tags[0] == ' ')
            {
                ptr_msg_after_tags++;
            }
        }
        else
            ptr_msg_after_tags = NULL;
    }

    /* if message is not BATCH but has a batch tag, just store it for later */
    if (!ignore_batch_tag
        && hash_tags
        && (weechat_strcasecmp (msg_command, "batch") != 0)
        && weechat_hashtable_has_key (server->cap_list, "batch"))
    {
        ptr_batch_ref = weechat_hashtable_get (hash_tags, "batch");
        if (ptr_batch_ref)
        {
            if (irc_batch_add_message (server, ptr_batch_ref, irc_message))
                goto end;
        }
    }

    /* get nick/host/address from IRC message */
    nick1 = NULL;
    address1 = NULL;
    host1 = NULL;
    if (ptr_msg_after_tags && (ptr_msg_after_tags[0] == ':'))
    {
        nick1 = irc_message_get_nick_from_host (ptr_msg_after_tags);
        address1 = irc_message_get_address_from_host (ptr_msg_after_tags);
        host1 = ptr_msg_after_tags + 1;
    }
    nick = (nick1) ? strdup (nick1) : NULL;
    address = (address1) ? strdup (address1) : NULL;
    address_color = (address) ?
        irc_color_decode (
            address,
            weechat_config_boolean (irc_config_network_colors_receive)) :
        NULL;
    host = (host1) ? strdup (host1) : NULL;
    if (host)
    {
        pos_space = strchr (host, ' ');
        if (pos_space)
            pos_space[0] = '\0';
    }
    host_no_color = (host) ? irc_color_decode (host, 0) : NULL;
    host_color = (host) ?
        irc_color_decode (
            host,
            weechat_config_boolean (irc_config_network_colors_receive)) :
        NULL;

    /* check if message is ignored or not */
    ptr_channel = NULL;
    if (msg_channel)
        ptr_channel = irc_channel_search (server, msg_channel);
    message_ignored = irc_ignore_check (
        server,
        (ptr_channel) ? ptr_channel->name : msg_channel,
        nick,
        host_no_color);

    /* send signal with received command, even if command is ignored */
    return_code = irc_server_send_signal (server, "irc_raw_in", msg_command,
                                          irc_message, NULL);
    if (return_code == WEECHAT_RC_OK_EAT)
        goto end;

    /* send signal with received command, only if message is not ignored */
    if (!message_ignored)
    {
        return_code = irc_server_send_signal (server, "irc_in", msg_command,
                                              irc_message, NULL);
        if (return_code == WEECHAT_RC_OK_EAT)
            goto end;
    }

    /* look for IRC command */
    cmd_found = -1;
    for (i = 0; irc_protocol_messages[i].name; i++)
    {
        if (weechat_strcasecmp (irc_protocol_messages[i].name,
                                msg_command) == 0)
        {
            cmd_found = i;
            break;
        }
    }

    /* command not found */
    if (cmd_found < 0)
    {
        /* for numeric commands, we use default recv function */
        if (irc_protocol_is_numeric_command (msg_command))
        {
            cmd_name = msg_command;
            decode_color = 1;
            keep_trailing_spaces = 0;
            cmd_recv_func = irc_protocol_cb_numeric;
        }
        else
        {
            weechat_printf (server->buffer,
                            _("%s%s: command \"%s\" not found: \"%s\""),
                            weechat_prefix ("error"), IRC_PLUGIN_NAME,
                            msg_command, irc_message);
            goto end;
        }
    }
    else
    {
        cmd_name = irc_protocol_messages[cmd_found].name;
        decode_color = irc_protocol_messages[cmd_found].decode_color;
        keep_trailing_spaces = irc_protocol_messages[cmd_found].keep_trailing_spaces;
        cmd_recv_func = irc_protocol_messages[cmd_found].recv_function;
    }

    if ((cmd_recv_func != NULL) && ptr_msg_after_tags)
    {
        if (decode_color)
        {
            message_colors_decoded = irc_color_decode (
                ptr_msg_after_tags,
                weechat_config_boolean (irc_config_network_colors_receive));
        }
        else
        {
            message_colors_decoded = strdup (ptr_msg_after_tags);
        }
        msg_to_parse = (keep_trailing_spaces) ?
            strdup (message_colors_decoded) :
            weechat_string_strip (message_colors_decoded, 0, 1, " ");

        irc_message_parse (server,
                           msg_to_parse,
                           NULL,  /* tags */
                           NULL,  /* message_without_tags */
                           NULL,  /* nick */
                           NULL,  /* user */
                           NULL,  /* host */
                           NULL,  /* command */
                           NULL,  /* channel */
                           NULL,  /* arguments */
                           NULL,  /* text */
                           &params,
                           &num_params,
                           NULL,  /* pos_command */
                           NULL,  /* pos_arguments */
                           NULL,  /* pos_channel */
                           NULL);  /* pos_text */

        return_code = (int) (cmd_recv_func) (server,
                                             date,
                                             msg_to_parse,
                                             hash_tags,
                                             nick,
                                             address_color,
                                             host_color,
                                             cmd_name,
                                             message_ignored,
                                             (const char **)params,
                                             num_params);

        if (params)
            weechat_string_free_split (params);

        if (return_code == WEECHAT_RC_ERROR)
        {
            weechat_printf (server->buffer,
                            _("%s%s: failed to parse command \"%s\" (please "
                              "report to developers): \"%s\""),
                            weechat_prefix ("error"), IRC_PLUGIN_NAME,
                            cmd_name, irc_message);
        }

        /* send signal with received command (if message is not ignored) */
        if (!message_ignored)
        {
            (void) irc_server_send_signal (server, "irc_in2", msg_command,
                                           irc_message, NULL);
        }
    }

    /* send signal with received command, even if command is ignored */
    (void) irc_server_send_signal (server, "irc_raw_in2", msg_command,
                                   irc_message, NULL);

end:
    if (nick)
        free (nick);
    if (address)
        free (address);
    if (address_color)
        free (address_color);
    if (host)
        free (host);
    if (host_no_color)
        free (host_no_color);
    if (host_color)
        free (host_color);
    if (message_colors_decoded)
        free (message_colors_decoded);
    if (msg_to_parse)
        free (msg_to_parse);
    if (hash_tags)
        weechat_hashtable_free (hash_tags);
}
