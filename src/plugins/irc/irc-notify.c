/*
 * irc-notify.c - notify lists for IRC plugin
 *
 * Copyright (C) 2010-2024 Sébastien Helleu <flashcode@flashtux.org>
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
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "irc.h"
#include "irc-notify.h"
#include "irc-color.h"
#include "irc-config.h"
#include "irc-message.h"
#include "irc-nick.h"
#include "irc-redirect.h"
#include "irc-server.h"


/* timers to run "ison" and "whois" commands */
struct t_hook *irc_notify_timer_ison = NULL;    /* timer for "ison"         */
struct t_hook *irc_notify_timer_whois = NULL;   /* timer for "whois"        */

/* hsignal for redirected commands */
struct t_hook *irc_notify_hsignal = NULL;


/*
 * Checks if a notify pointer is valid.
 *
 * If server is NULL, searches in all servers.
 *
 * Returns:
 *   1: notify exists
 *   0: notify does not exist
 */

int
irc_notify_valid (struct t_irc_server *server, struct t_irc_notify *notify)
{
    struct t_irc_server *ptr_server;
    struct t_irc_notify *ptr_notify;

    if (!notify)
        return 0;

    if (server)
    {
        for (ptr_notify = server->notify_list; ptr_notify;
             ptr_notify = ptr_notify->next_notify)
        {
            if (ptr_notify == notify)
                return 1;
        }
    }
    else
    {
        for (ptr_server = irc_servers; ptr_server;
             ptr_server = ptr_server->next_server)
        {
            for (ptr_notify = ptr_server->notify_list; ptr_notify;
                 ptr_notify = ptr_notify->next_notify)
            {
                if (ptr_notify == notify)
                    return 1;
            }
        }
    }

    /* notify not found */
    return 0;
}

/*
 * Searches for a notify.
 *
 * Returns pointer to notify found, NULL if not found.
 */

struct t_irc_notify *
irc_notify_search (struct t_irc_server *server, const char *nick)
{
    struct t_irc_notify *ptr_notify;

    if (!server || !nick)
        return NULL;

    for (ptr_notify = server->notify_list; ptr_notify;
         ptr_notify = ptr_notify->next_notify)
    {
        if (irc_server_strcasecmp (server, ptr_notify->nick, nick) == 0)
            return ptr_notify;
    }

    /* notify not found */
    return NULL;
}

/*
 * Sets server option "notify" with notify list on server.
 */

void
irc_notify_set_server_option (struct t_irc_server *server)
{
    char *str, *str2;
    struct t_irc_notify *ptr_notify;
    int total_length, length;

    if (!server)
        return;

    if (server->notify_list)
    {
        str = NULL;
        total_length = 0;
        for (ptr_notify = server->notify_list; ptr_notify;
             ptr_notify = ptr_notify->next_notify)
        {
            length = strlen (ptr_notify->nick) + 32;
            if (!str)
            {
                total_length += length + 1;
                str = malloc (total_length);
                if (str)
                    str[0] = '\0';
            }
            else
            {
                total_length += length;
                str2 = realloc (str, total_length);
                if (!str2)
                {
                    free (str);
                    return;
                }
                str = str2;
            }
            if (str)
            {
                if (str[0])
                    strcat (str, ",");
                strcat (str, ptr_notify->nick);
                if (ptr_notify->check_away)
                    strcat (str, " away");
            }
        }
        if (str)
        {
            weechat_config_option_set (server->options[IRC_SERVER_OPTION_NOTIFY],
                                       str, 0);
            free (str);
        }
    }
    else
    {
        weechat_config_option_set (server->options[IRC_SERVER_OPTION_NOTIFY],
                                   "", 0);
    }
}

/*
 * Adds a new notify.
 *
 * Returns pointer to new notify, NULL if error.
 */

struct t_irc_notify *
irc_notify_new (struct t_irc_server *server, const char *nick, int check_away)
{
    struct t_irc_notify *new_notify;

    if (!server || !nick || !nick[0]
        || ((server->monitor > 0)
            && ((server->notify_count >= server->monitor))))
    {
        return NULL;
    }

    new_notify = malloc (sizeof (*new_notify));
    if (new_notify)
    {
        new_notify->server = server;
        new_notify->nick = strdup (nick);
        new_notify->check_away = check_away;
        new_notify->is_on_server = -1;
        new_notify->away_message = NULL;
        new_notify->ison_received = 0;

        /* add notify to notify list on server */
        new_notify->prev_notify = server->last_notify;
        if (server->last_notify)
            server->last_notify->next_notify = new_notify;
        else
            server->notify_list = new_notify;
        server->last_notify = new_notify;
        new_notify->next_notify = NULL;

        server->notify_count++;
    }

    return new_notify;
}

/*
 * Checks now if a nick is connected with ison/monitor + whois (if away checking
 * is enabled).
 *
 * It is called when a notify is added.
 */

void
irc_notify_check_now (struct t_irc_notify *notify)
{
    /* don't send anything if we are not connected to the server */
    if (!notify->server->is_connected)
        return;

    if (notify->server->monitor > 0)
    {
        /* send MONITOR for nick */
        irc_server_sendf (notify->server, IRC_SERVER_SEND_OUTQ_PRIO_LOW, NULL,
                          "MONITOR + %s", notify->nick);
    }
    else
    {
        /* send ISON for nick (MONITOR not supported on server) */
        irc_redirect_new (notify->server, "ison", "notify", 1, NULL, 0, NULL);
        irc_server_sendf (notify->server, IRC_SERVER_SEND_OUTQ_PRIO_LOW, NULL,
                          "ISON :%s", notify->nick);
    }

    if (notify->check_away)
    {
        /* send WHOIS for nick */
        irc_redirect_new (notify->server, "whois", "notify", 1, notify->nick, 0,
                          "301,401");
        irc_server_sendf (notify->server, IRC_SERVER_SEND_OUTQ_PRIO_LOW, NULL,
                          "WHOIS :%s", notify->nick);
    }
}

/*
 * Builds a message with nicks (ISON or MONITOR).
 *
 * Argument "message" must be "ISON :" or "MONITOR + " or "MONITOR - ".
 * Argument "separator" must be " " for ISON and "," for MONITOR.
 * Argument "num_nicks" is set with the number of nicks added in message.
 *
 * Note: result must be freed after use.
 */

char *
irc_notify_build_message_with_nicks (struct t_irc_server *server,
                                     const char *irc_message,
                                     const char *separator,
                                     int *num_nicks)
{
    char *message, *message2;
    int length, total_length, length_separator;
    struct t_irc_notify *ptr_notify;

    *num_nicks = 0;

    length = strlen (irc_message) + 1;
    total_length = length;
    length_separator = strlen (separator);

    message = malloc (length);
    if (!message)
        return NULL;
    snprintf (message, length, "%s", irc_message);

    for (ptr_notify = server->notify_list; ptr_notify;
         ptr_notify = ptr_notify->next_notify)
    {
        length = strlen (ptr_notify->nick);
        total_length += length + length_separator;
        message2 = realloc (message, total_length);
        if (!message2)
        {
            free (message);
            message = NULL;
            break;
        }
        message = message2;
        if (*num_nicks > 0)
            strcat (message, separator);
        strcat (message, ptr_notify->nick);

        (*num_nicks)++;
    }

    return message;
}

/*
 * Sends the MONITOR message (after server connection or if the option
 * "irc.server.xxx.notify" is changed).
 */

void
irc_notify_send_monitor (struct t_irc_server *server)
{
    struct t_hashtable *hashtable;
    char *message, hash_key[32];
    const char *str_message;
    int num_nicks, number;

    message = irc_notify_build_message_with_nicks (server,
                                                   "MONITOR + ",
                                                   ",",
                                                   &num_nicks);
    if (message && (num_nicks > 0))
    {
        hashtable = irc_message_split (server, message);
        if (hashtable)
        {
            number = 1;
            while (1)
            {
                snprintf (hash_key, sizeof (hash_key), "msg%d", number);
                str_message = weechat_hashtable_get (hashtable,
                                                     hash_key);
                if (!str_message)
                    break;
                irc_server_sendf (server,
                                  IRC_SERVER_SEND_OUTQ_PRIO_LOW,
                                  NULL, "%s", str_message);
                number++;
            }
            weechat_hashtable_free (hashtable);
        }
    }
    free (message);
}

/*
 * Creates a notify list for server with option "irc.server.xxx.notify".
 */

void
irc_notify_new_for_server (struct t_irc_server *server)
{
    const char *notify;
    char **items, *pos_params, **params;
    int i, j, num_items, num_params, check_away;

    irc_notify_free_all (server);

    notify = IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_NOTIFY);
    if (!notify || !notify[0])
        return;

    items = weechat_string_split (notify, ",", NULL,
                                  WEECHAT_STRING_SPLIT_STRIP_LEFT
                                  | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                  | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                  0, &num_items);
    if (items)
    {
        for (i = 0; i < num_items; i++)
        {
            check_away = 0;
            pos_params = strchr (items[i], ' ');
            if (pos_params)
            {
                pos_params[0] = '\0';
                pos_params++;
                while (pos_params[0] == ' ')
                {
                    pos_params++;
                }
                params = weechat_string_split (
                    pos_params,
                    "/",
                    NULL,
                    WEECHAT_STRING_SPLIT_STRIP_LEFT
                    | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                    | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                    0,
                    &num_params);
                if (params)
                {
                    for (j = 0; j < num_params; j++)
                    {
                        if (weechat_strcasecmp (params[j], "away") == 0)
                            check_away = 1;
                    }
                    weechat_string_free_split (params);
                }
            }
            irc_notify_new (server, items[i], check_away);
        }
        weechat_string_free_split (items);
    }

    /* if we are using MONITOR, send it now with new nicks monitored */
    if (server->is_connected && (server->monitor > 0))
        irc_notify_send_monitor (server);
}

/*
 * Creates a notify list for all servers with option "irc.server.xxx.notify".
 */

void
irc_notify_new_for_all_servers ()
{
    struct t_irc_server *ptr_server;

    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        irc_notify_new_for_server (ptr_server);
    }
}

/*
 * Removes a notify on a server.
 */

void
irc_notify_free (struct t_irc_server *server, struct t_irc_notify *notify,
                 int remove_monitor)
{
    if (!server || !notify)
        return;

    (void) weechat_hook_signal_send ("irc_notify_removing",
                                     WEECHAT_HOOK_SIGNAL_POINTER, notify);

    /* free data */
    if (notify->nick)
    {
        if ((server->monitor > 0) && remove_monitor
            && (server->is_connected) && !irc_signal_upgrade_received)
        {
            /* remove one monitored nick */
            irc_server_sendf (notify->server,
                              IRC_SERVER_SEND_OUTQ_PRIO_LOW, NULL,
                              "MONITOR - %s", notify->nick);
        }
        free (notify->nick);
    }
    free (notify->away_message);

    /* remove notify from list */
    if (notify->prev_notify)
        (notify->prev_notify)->next_notify = notify->next_notify;
    if (notify->next_notify)
        (notify->next_notify)->prev_notify = notify->prev_notify;
    if (server->notify_list == notify)
        server->notify_list = notify->next_notify;
    if (server->last_notify == notify)
        server->last_notify = notify->prev_notify;

    free (notify);

    if (server->notify_count > 0)
        server->notify_count--;

    (void) weechat_hook_signal_send ("irc_notify_removed",
                                     WEECHAT_HOOK_SIGNAL_STRING, NULL);
}

/*
 * Removes all notify on a server.
 */

void
irc_notify_free_all (struct t_irc_server *server)
{
    /* remove all monitored nicks */
    if ((server->monitor > 0) && (server->is_connected)
        && !irc_signal_upgrade_received)
    {
        irc_server_sendf (server, IRC_SERVER_SEND_OUTQ_PRIO_LOW, NULL,
                          "MONITOR C");
    }

    /* free notify list */
    while (server->notify_list)
    {
        irc_notify_free (server, server->notify_list, 0);
    }
}

/*
 * Displays a notify.
 */

void
irc_notify_display (struct t_irc_server *server, struct t_gui_buffer *buffer,
                    struct t_irc_notify *notify)
{
    if ((notify->is_on_server < 0)
        || (!notify->is_on_server && !notify->away_message))
    {
        weechat_printf (
            buffer,
            "  %s%s%s @ %s%s%s: %s%s",
            irc_nick_color_for_msg (server, 1, NULL, notify->nick),
            notify->nick,
            IRC_COLOR_RESET,
            IRC_COLOR_CHAT_SERVER,
            notify->server->name,
            IRC_COLOR_RESET,
            (notify->is_on_server < 0) ? "" : IRC_COLOR_MESSAGE_QUIT,
            (notify->is_on_server < 0) ?
            /* TRANSLATORS: "unknown" is the status for /notify when ison answer has not been received (check pending) */
            _("unknown") :
            _("offline"));
    }
    else
    {
        weechat_printf (
            buffer,
            "  %s%s%s @ %s%s%s: %s%s %s%s%s%s%s%s",
            irc_nick_color_for_msg (server, 1, NULL, notify->nick),
            notify->nick,
            IRC_COLOR_RESET,
            IRC_COLOR_CHAT_SERVER,
            notify->server->name,
            IRC_COLOR_RESET,
            IRC_COLOR_MESSAGE_JOIN,
            _("online"),
            IRC_COLOR_RESET,
            (notify->away_message) ? " (" : "",
            (notify->away_message) ? _("away") : "",
            (notify->away_message) ? ": \"" : "",
            (notify->away_message) ? notify->away_message : "",
            (notify->away_message) ? "\")" : "");
    }
}

/*
 * Displays notify list for a server (or all servers if server is NULL).
 */

void
irc_notify_display_list (struct t_irc_server *server)
{
    struct t_irc_notify *ptr_notify;
    struct t_irc_server *ptr_server;
    int count;

    if (server)
    {
        if (server->notify_list)
        {
            weechat_printf (server->buffer, "");
            weechat_printf (server->buffer,
                            _("Notify list for %s%s%s:"),
                            IRC_COLOR_CHAT_SERVER,
                            server->name,
                            IRC_COLOR_RESET);
            for (ptr_notify = server->notify_list; ptr_notify;
                 ptr_notify = ptr_notify->next_notify)
            {
                irc_notify_display (server, server->buffer, ptr_notify);
            }
        }
        else
        {
            weechat_printf (server->buffer,
                            _("Notify list is empty on this server"));
        }
    }
    else
    {
        count = 0;
        for (ptr_server = irc_servers; ptr_server;
             ptr_server = ptr_server->next_server)
        {
            for (ptr_notify = ptr_server->notify_list; ptr_notify;
                 ptr_notify = ptr_notify->next_notify)
            {
                if (count == 0)
                {
                    weechat_printf (NULL, "");
                    weechat_printf (NULL, _("Notify list for all servers:"));
                }
                irc_notify_display (ptr_server, NULL, ptr_notify);
                count++;
            }
        }
        if (count == 0)
        {
            weechat_printf (NULL,
                            _("Notify list is empty on all servers"));
        }
    }
}

/*
 * Gets tags for message displayed (concatenation of "irc_notify" and tags from
 * option).
 */

const char *
irc_notify_get_tags (struct t_config_option *option, const char *type,
                     const char *nick)
{
    static char string[1024];
    const char *tags;

    tags = weechat_config_string (option);

    snprintf (string, sizeof (string),
              "irc_notify,irc_notify_%s,nick_%s%s%s,log3",
              type,
              nick,
              (tags && tags[0]) ? "," : "",
              (tags && tags[0]) ? tags : "");

    return string;
}

/*
 * Sends a signal on a notify event.
 */

void
irc_notify_send_signal (struct t_irc_notify *notify,
                        const char *type,
                        const char *away_message)
{
    char signal[128], *data;
    int length;

    snprintf (signal, sizeof (signal), "irc_notify_%s", type);

    length = strlen (notify->server->name) + 1 + strlen (notify->nick) + 1
        + ((away_message) ? strlen (away_message) : 0) + 1;
    data = malloc (length);
    if (data)
    {
        snprintf (data, length, "%s,%s%s%s",
                  notify->server->name,
                  notify->nick,
                  (away_message && away_message[0]) ? "," : "",
                  (away_message && away_message[0]) ? away_message : "");
    }

    (void) weechat_hook_signal_send (signal, WEECHAT_HOOK_SIGNAL_STRING, data);

    free (data);
}

/*
 * Display message about nick: "is connected", "is offline", "has connected",
 * "has quit".
 *
 * If "notify" is NULL, only "is connected" or "is offline" can be displayed.
 */

void
irc_notify_display_is_on (struct t_irc_server *server,
                          const char *nick,
                          const char *host,
                          struct t_irc_notify *notify,
                          int is_on_server)
{
    weechat_printf_date_tags (
        server->buffer,
        0,
        irc_notify_get_tags (irc_config_look_notify_tags_ison,
                             (is_on_server) ? "join" : "quit",
                             nick),
        (!notify || (notify->is_on_server < 0)) ?
        ((is_on_server) ?
         _("%snotify: %s%s%s%s%s%s%s%s%s is connected") :
         _("%snotify: %s%s%s%s%s%s%s%s%s is offline")) :
        ((is_on_server) ?
         _("%snotify: %s%s%s%s%s%s%s%s%s has connected") :
         _("%snotify: %s%s%s%s%s%s%s%s%s has quit")),
        weechat_prefix ("network"),
        irc_nick_color_for_msg (server, 1, NULL, nick),
        nick,
        (host && host[0]) ? IRC_COLOR_CHAT_DELIMITERS : "",
        (host && host[0]) ? " (" : "",
        (host && host[0]) ? IRC_COLOR_CHAT_HOST : "",
        (host && host[0]) ? host : "",
        (host && host[0]) ? IRC_COLOR_CHAT_DELIMITERS : "",
        (host && host[0]) ? ")" : "",
        (is_on_server) ? IRC_COLOR_MESSAGE_JOIN : IRC_COLOR_MESSAGE_QUIT);
}

/*
 * Sets flag "is_on_server" for a notify and display message if user was not on
 * server.
 */

void
irc_notify_set_is_on_server (struct t_irc_notify *notify, const char *host,
                             int is_on_server)
{
    if (!notify)
        return;

    /* same status, then do nothing */
    if (notify->is_on_server == is_on_server)
        return;

    irc_notify_display_is_on (notify->server, notify->nick, host,
                              notify, is_on_server);

    irc_notify_send_signal (notify, (is_on_server) ? "join" : "quit", NULL);

    notify->is_on_server = is_on_server;
}

/*
 * Sets away message for a notify and display message if away status has
 * changed.
 */

void
irc_notify_set_away_message (struct t_irc_notify *notify,
                             const char *away_message)
{
    if (!notify)
        return;

    /* same away message, then do nothing */
    if ((!notify->away_message && !away_message)
        || (notify->away_message && away_message
            && (strcmp (notify->away_message, away_message) == 0)))
        return;

    if (!notify->away_message && away_message)
    {
        weechat_printf_date_tags (
            notify->server->buffer,
            0,
            irc_notify_get_tags (
                irc_config_look_notify_tags_whois, "away", notify->nick),
            _("%snotify: %s%s%s is now away: \"%s\""),
            weechat_prefix ("network"),
            irc_nick_color_for_msg (notify->server, 1, NULL, notify->nick),
            notify->nick,
            IRC_COLOR_RESET,
            away_message);
        irc_notify_send_signal (notify, "away", away_message);
    }
    else if (notify->away_message && !away_message)
    {
        weechat_printf_date_tags (
            notify->server->buffer,
            0,
            irc_notify_get_tags (
                irc_config_look_notify_tags_whois, "back", notify->nick),
            _("%snotify: %s%s%s is back"),
            weechat_prefix ("network"),
            irc_nick_color_for_msg (notify->server, 1, NULL, notify->nick),
            notify->nick,
            IRC_COLOR_RESET);
        irc_notify_send_signal (notify, "back", NULL);
    }
    else if (notify->away_message && away_message)
    {
        weechat_printf_date_tags (
            notify->server->buffer,
            0,
            irc_notify_get_tags (
                irc_config_look_notify_tags_whois, "still_away", notify->nick),
            _("%snotify: %s%s%s is still away: \"%s\""),
            weechat_prefix ("network"),
            irc_nick_color_for_msg (notify->server, 1, NULL, notify->nick),
            notify->nick,
            IRC_COLOR_RESET,
            away_message);
        irc_notify_send_signal (notify, "still_away", away_message);
    }

    free (notify->away_message);
    notify->away_message = (away_message) ? strdup (away_message) : NULL;
}

/*
 * Callback for hsignal on redirected commands "ison" and "whois".
 */

int
irc_notify_hsignal_cb (const void *pointer, void *data, const char *signal,
                       struct t_hashtable *hashtable)
{
    const char *error, *server, *pattern, *command, *output;
    char **messages, **nicks_sent, **nicks_recv, *irc_cmd, *arguments;
    char *ptr_args, *pos;
    int i, j, num_messages, num_nicks_sent, num_nicks_recv, nick_was_sent;
    int away_message_updated, no_such_nick;
    struct t_irc_server *ptr_server;
    struct t_irc_notify *ptr_notify;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) signal;

    error = weechat_hashtable_get (hashtable, "error");
    server = weechat_hashtable_get (hashtable, "server");
    pattern = weechat_hashtable_get (hashtable, "pattern");
    command = weechat_hashtable_get (hashtable, "command");
    output = weechat_hashtable_get (hashtable, "output");

    /* if there is an error on redirection, just ignore result */
    if (error && error[0])
        return WEECHAT_RC_OK;

    /* missing things in redirection */
    if (!server || !pattern || !command || !output)
        return WEECHAT_RC_OK;

    /* search server */
    ptr_server = irc_server_search (server);
    if (!ptr_server)
        return WEECHAT_RC_OK;

    /* search for start of arguments in command sent to server */
    ptr_args = strchr (command, ' ');
    if (!ptr_args)
        return WEECHAT_RC_OK;
    ptr_args++;
    while ((ptr_args[0] == ' ') || (ptr_args[0] == ':'))
    {
        ptr_args++;
    }
    if (!ptr_args[0])
        return WEECHAT_RC_OK;

    /* read output of command */
    if (strcmp (pattern, "ison") == 0)
    {
        /* redirection of command "ison" */
        messages = weechat_string_split (
            output,
            "\n",
            NULL,
            WEECHAT_STRING_SPLIT_STRIP_LEFT
            | WEECHAT_STRING_SPLIT_STRIP_RIGHT
            | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
            0,
            &num_messages);
        if (messages)
        {
            nicks_sent = weechat_string_split (
                ptr_args,
                " ",
                NULL,
                WEECHAT_STRING_SPLIT_STRIP_LEFT
                | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                0,
                &num_nicks_sent);
            if (!nicks_sent)
                return WEECHAT_RC_OK;
            for (ptr_notify = ptr_server->notify_list;
                 ptr_notify;
                 ptr_notify = ptr_notify->next_notify)
            {
                ptr_notify->ison_received = 0;
            }
            for (i = 0; i < num_messages; i++)
            {
                irc_message_parse (ptr_server,
                                   messages[i],
                                   NULL,  /* tags */
                                   NULL,  /* message_without_tags */
                                   NULL,  /* nick */
                                   NULL,  /* user */
                                   NULL,  /* host */
                                   NULL,  /* command */
                                   NULL,  /* channel */
                                   &arguments,
                                   NULL,  /* text */
                                   NULL,  /* params */
                                   NULL,  /* num_params */
                                   NULL,  /* pos_command */
                                   NULL,  /* pos_arguments */
                                   NULL,  /* pos_channel */
                                   NULL);  /* pos_text */
                if (arguments)
                {
                    pos = strchr (arguments, ' ');
                    if (pos)
                    {
                        pos++;
                        while ((pos[0] == ' ') || (pos[0] == ':'))
                        {
                            pos++;
                        }
                        if (pos[0])
                        {
                            nicks_recv = weechat_string_split (
                                pos,
                                " ",
                                NULL,
                                WEECHAT_STRING_SPLIT_STRIP_LEFT
                                | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                0,
                                &num_nicks_recv);
                            if (nicks_recv)
                            {
                                for (j = 0; j < num_nicks_recv; j++)
                                {
                                    for (ptr_notify = ptr_server->notify_list;
                                         ptr_notify;
                                         ptr_notify = ptr_notify->next_notify)
                                    {
                                        if (irc_server_strcasecmp (ptr_server,
                                                                   ptr_notify->nick,
                                                                   nicks_recv[j]) == 0)
                                        {
                                            irc_notify_set_is_on_server (ptr_notify,
                                                                         NULL,
                                                                         1);
                                            ptr_notify->ison_received = 1;
                                        }
                                    }
                                }
                                weechat_string_free_split (nicks_recv);
                            }
                        }
                    }
                    free (arguments);
                }
            }
            for (ptr_notify = ptr_server->notify_list;
                 ptr_notify;
                 ptr_notify = ptr_notify->next_notify)
            {
                if (!ptr_notify->ison_received)
                {
                    nick_was_sent = 0;
                    for (j = 0; j < num_nicks_sent; j++)
                    {
                        if (irc_server_strcasecmp (ptr_server,
                                                   nicks_sent[j],
                                                   ptr_notify->nick) == 0)
                        {
                            nick_was_sent = 1;
                            break;
                        }
                    }
                    if (nick_was_sent)
                    {
                        irc_notify_set_is_on_server (ptr_notify, NULL, 0);
                    }
                }

            }
            weechat_string_free_split (messages);
        }
    }
    else if (strcmp (pattern, "whois") == 0)
    {
        /* redirection of command "whois" */
        ptr_notify = irc_notify_search (ptr_server, ptr_args);
        if (ptr_notify)
        {
            away_message_updated = 0;
            no_such_nick = 0;
            messages = weechat_string_split (
                output,
                "\n",
                NULL,
                WEECHAT_STRING_SPLIT_STRIP_LEFT
                | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                0,
                &num_messages);
            if (messages)
            {
                for (i = 0; i < num_messages; i++)
                {
                    irc_message_parse (ptr_server,
                                       messages[i],
                                       NULL,  /* tags */
                                       NULL,  /* message_without_tags */
                                       NULL,  /* nick */
                                       NULL,  /* user */
                                       NULL,  /* host */
                                       &irc_cmd,
                                       NULL,  /* channel */
                                       &arguments,
                                       NULL,  /* text */
                                       NULL,  /* params */
                                       NULL,  /* num_params */
                                       NULL,  /* pos_command */
                                       NULL,  /* pos_arguments */
                                       NULL,  /* pos_channel */
                                       NULL);  /* pos_text */
                    if (irc_cmd && arguments)
                    {
                        if (strcmp (irc_cmd, "401") == 0)
                        {
                            /* no such nick/channel */
                            no_such_nick = 1;
                        }
                        else if (strcmp (irc_cmd, "301") == 0)
                        {
                            /* away message */
                            pos = strchr (arguments, ':');
                            if (pos)
                            {
                                pos++;
                                /* nick is away */
                                irc_notify_set_away_message (ptr_notify, pos);
                                away_message_updated = 1;
                            }
                        }
                    }
                    free (irc_cmd);
                    free (arguments);
                }
            }
            if (!away_message_updated && !no_such_nick)
            {
                /* nick is back */
                irc_notify_set_away_message (ptr_notify, NULL);
            }
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Timer called to send "ison" command to servers.
 */

int
irc_notify_timer_ison_cb (const void *pointer, void *data, int remaining_calls)
{
    char *message, hash_key[32];
    const char *str_message;
    int num_nicks, number;
    struct t_irc_server *ptr_server;
    struct t_hashtable *hashtable;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) remaining_calls;

    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        if (ptr_server->is_connected
            && ptr_server->notify_list
            && (ptr_server->monitor == 0))
        {
            message = irc_notify_build_message_with_nicks (ptr_server,
                                                           "ISON :",
                                                           " ",
                                                           &num_nicks);
            if (message && (num_nicks > 0))
            {
                hashtable = irc_message_split (ptr_server, message);
                if (hashtable)
                {
                    number = 1;
                    while (1)
                    {
                        snprintf (hash_key, sizeof (hash_key), "msg%d", number);
                        str_message = weechat_hashtable_get (hashtable,
                                                             hash_key);
                        if (!str_message)
                            break;
                        irc_redirect_new (ptr_server, "ison", "notify", 1,
                                          NULL, 0, NULL);
                        irc_server_sendf (ptr_server,
                                          IRC_SERVER_SEND_OUTQ_PRIO_LOW,
                                          NULL, "%s", str_message);
                        number++;
                    }
                    weechat_hashtable_free (hashtable);
                }
            }
            free (message);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Timer called to send "whois" command to servers.
 */

int
irc_notify_timer_whois_cb (const void *pointer, void *data,
                           int remaining_calls)
{
    struct t_irc_server *ptr_server;
    struct t_irc_notify *ptr_notify, *ptr_next_notify;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) remaining_calls;

    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        if (ptr_server->is_connected && ptr_server->notify_list)
        {
            ptr_notify = ptr_server->notify_list;
            while (ptr_notify)
            {
                ptr_next_notify = ptr_notify->next_notify;

                if (ptr_notify->check_away)
                {
                    /*
                     * redirect whois, and get only 2 messages:
                     * 301: away message
                     * 401: no such nick/channel
                     */
                    irc_redirect_new (ptr_server, "whois", "notify", 1,
                                      ptr_notify->nick, 0, "301,401");
                    irc_server_sendf (ptr_server,
                                      IRC_SERVER_SEND_OUTQ_PRIO_LOW, NULL,
                                      "WHOIS :%s", ptr_notify->nick);
                }

                ptr_notify = ptr_next_notify;
            }
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Returns hdata for notify.
 */

struct t_hdata *
irc_notify_hdata_notify_cb (const void *pointer, void *data,
                            const char *hdata_name)
{
    struct t_hdata *hdata;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    hdata = weechat_hdata_new (hdata_name, "prev_notify", "next_notify",
                               0, 0, NULL, NULL);
    if (hdata)
    {
        WEECHAT_HDATA_VAR(struct t_irc_notify, server, POINTER, 0, NULL, "irc_server");
        WEECHAT_HDATA_VAR(struct t_irc_notify, nick, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_notify, check_away, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_notify, is_on_server, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_notify, away_message, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_notify, ison_received, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_notify, prev_notify, POINTER, 0, NULL, hdata_name);
        WEECHAT_HDATA_VAR(struct t_irc_notify, next_notify, POINTER, 0, NULL, hdata_name);
    }
    return hdata;
}

/*
 * Adds a notify in an infolist.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
irc_notify_add_to_infolist (struct t_infolist *infolist,
                            struct t_irc_notify *notify)
{
    struct t_infolist_item *ptr_item;

    if (!infolist || !notify)
        return 0;

    ptr_item = weechat_infolist_new_item (infolist);
    if (!ptr_item)
        return 0;

    if (!weechat_infolist_new_var_pointer (ptr_item, "server", notify->server))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "server_name", notify->server->name))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "nick", notify->nick))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "check_away", notify->check_away))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "is_on_server", notify->is_on_server))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "away_message", notify->away_message))
        return 0;

    return 1;
}

/*
 * Prints notify infos in WeeChat log file (usually for crash dump).
 */

void
irc_notify_print_log (struct t_irc_server *server)
{
    struct t_irc_notify *ptr_notify;

    for (ptr_notify = server->notify_list; ptr_notify;
         ptr_notify = ptr_notify->next_notify)
    {
        weechat_log_printf ("");
        weechat_log_printf ("  => notify (addr:%p):", ptr_notify);
        weechat_log_printf ("       server. . . . . . . : %p", ptr_notify->server);
        weechat_log_printf ("       nick. . . . . . . . : '%s'", ptr_notify->nick);
        weechat_log_printf ("       check_away. . . . . : %d", ptr_notify->check_away);
        weechat_log_printf ("       is_on_server. . . . : %d", ptr_notify->is_on_server);
        weechat_log_printf ("       away_message. . . . : '%s'", ptr_notify->away_message);
        weechat_log_printf ("       ison_received . . . : %d", ptr_notify->ison_received);
        weechat_log_printf ("       prev_notify . . . . : %p", ptr_notify->prev_notify);
        weechat_log_printf ("       next_notify . . . . : %p", ptr_notify->next_notify);
    }
}

/*
 * Hooks timer to send "ison" command.
 */

void
irc_notify_hook_timer_ison ()
{
    weechat_unhook (irc_notify_timer_ison);
    irc_notify_timer_ison = weechat_hook_timer (
        60 * 1000 * weechat_config_integer (irc_config_network_notify_check_ison),
        0, 0, &irc_notify_timer_ison_cb, NULL, NULL);
}

/*
 * Hooks timer to send "whois" command.
 */

void
irc_notify_hook_timer_whois ()
{
    weechat_unhook (irc_notify_timer_whois);
    irc_notify_timer_whois = weechat_hook_timer (
        60 * 1000 * weechat_config_integer (irc_config_network_notify_check_whois),
        0, 0, &irc_notify_timer_whois_cb, NULL, NULL);
}

/*
 * Hooks timers and hsignal.
 */

void
irc_notify_init ()
{
    irc_notify_hook_timer_ison ();
    irc_notify_hook_timer_whois ();

    irc_notify_hsignal = weechat_hook_hsignal ("irc_redirection_notify_*",
                                               &irc_notify_hsignal_cb,
                                               NULL, NULL);
}

/*
 * Removes timers and hsignal.
 */

void
irc_notify_end ()
{
    if (irc_notify_timer_ison)
    {
        weechat_unhook (irc_notify_timer_ison);
        irc_notify_timer_ison = NULL;
    }
    if (irc_notify_timer_whois)
    {
        weechat_unhook (irc_notify_timer_whois);
        irc_notify_timer_whois = NULL;
    }
    if (irc_notify_hsignal)
    {
        weechat_unhook (irc_notify_hsignal);
        irc_notify_hsignal = NULL;
    }
}
