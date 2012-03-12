/*
 * Copyright (C) 2010-2012 Sebastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * irc-notify.c: notify lists for IRC plugin
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
 * irc_notify_valid: check if a notify pointer exists for a server
 *                   if server is NULL, search in all servers
 *                   return 1 if notify exists
 *                          0 if notify is not found
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
 * irc_notify_search: search a notify
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
 * irc_notify_set_server_option: set server option "notify" with notify
 *                               list on server
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
                    if (str)
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
 * irc_notify_new: add new notify
 */

struct t_irc_notify *
irc_notify_new (struct t_irc_server *server, const char *nick, int check_away)
{
    struct t_irc_notify *new_notify;

    if (!server || !nick || !nick[0])
        return NULL;

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
        if (server->notify_list)
            server->last_notify->next_notify = new_notify;
        else
            server->notify_list = new_notify;
        server->last_notify = new_notify;
        new_notify->next_notify = NULL;
    }

    return new_notify;
}

/*
 * irc_notify_check_now: check now ison/whois for a notify (called when a
 *                       notify is added)
 */

void
irc_notify_check_now (struct t_irc_notify *notify)
{
    if (notify->server->is_connected)
    {
        /* send the ISON for nick */
        irc_redirect_new (notify->server, "ison", "notify", 1,
                          NULL, 0, NULL);
        irc_server_sendf (notify->server,
                          IRC_SERVER_SEND_OUTQ_PRIO_LOW, NULL,
                          "ISON :%s", notify->nick);

        if (notify->check_away)
        {
            /* send the WHOIS for nick */
            irc_redirect_new (notify->server, "whois", "notify", 1,
                              notify->nick, 0, "301,401");
            irc_server_sendf (notify->server,
                              IRC_SERVER_SEND_OUTQ_PRIO_LOW, NULL,
                              "WHOIS :%s", notify->nick);
        }
    }
}

/*
 * irc_notify_new_for_server: create notify list for server with option
 *                            "irc.server.xxx.notify"
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

    items = weechat_string_split (notify, ",", 0, 0, &num_items);

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
                params = weechat_string_split (pos_params, "/", 0, 0,
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
}

/*
 * irc_notify_new_for_all_servers: create notify list for all servers with
 *                                 option "irc.server.xxx.notify"
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
 * irc_notify_free: remove a notify on a server
 */

void
irc_notify_free (struct t_irc_server *server, struct t_irc_notify *notify)
{
    weechat_hook_signal_send ("irc_notify_removing",
                              WEECHAT_HOOK_SIGNAL_POINTER, notify);

    /* free data */
    if (notify->nick)
        free (notify->nick);
    if (notify->away_message)
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

    weechat_hook_signal_send ("irc_notify_removed",
                              WEECHAT_HOOK_SIGNAL_STRING, NULL);
}

/*
 * irc_notify_free_all: remove all notify on a server
 */

void
irc_notify_free_all (struct t_irc_server *server)
{
    while (server->notify_list)
    {
        irc_notify_free (server, server->notify_list);
    }
}

/*
 * irc_notify_display: display a notify
 */

void
irc_notify_display (struct t_gui_buffer *buffer, struct t_irc_notify *notify)
{
    if ((notify->is_on_server < 0)
        || (!notify->is_on_server && !notify->away_message))
    {
        weechat_printf (buffer,
                        "  %s%s%s @ %s%s%s: %s%s",
                        irc_nick_color_for_server_message (NULL, notify->nick),
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
        weechat_printf (buffer,
                        "  %s%s%s @ %s%s%s: %s%s %s%s%s%s%s%s",
                        irc_nick_color_for_server_message (NULL, notify->nick),
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
 * irc_notify_display_list: display notify list for a server
 *                          (or all servers if server is NULL)
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
                irc_notify_display (server->buffer, ptr_notify);
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
                irc_notify_display (NULL, ptr_notify);
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
 * irc_notify_get_tags: get tags for message displayed
 *                      (concatenation of "irc_notify" and tags from option)
 */

const char *
irc_notify_get_tags (struct t_config_option *option, const char *type,
                     const char *nick)
{
    static char string[1024];
    const char *tags;

    tags = weechat_config_string (option);

    snprintf (string, sizeof (string), "irc_notify,irc_notify_%s,nick_%s%s%s",
              type,
              nick,
              (tags && tags[0]) ? "," : "",
              (tags && tags[0]) ? tags : "");

    return string;
}

/*
 * irc_notify_send_signal: send signal on notify event
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

    weechat_hook_signal_send (signal, WEECHAT_HOOK_SIGNAL_STRING, data);

    if (data)
        free (data);
}

/*
 * irc_notify_set_is_on_server: set flag "is_on_server" for a notify
 *                              and display message if user was not on server
 */

void
irc_notify_set_is_on_server (struct t_irc_notify *notify,
                             int is_on_server)
{
    if (!notify)
        return;

    /* same status, then do nothing */
    if (notify->is_on_server == is_on_server)
        return;

    weechat_printf_tags (notify->server->buffer,
                         irc_notify_get_tags (irc_config_look_notify_tags_ison,
                                              (is_on_server) ? "join" : "quit",
                                              notify->nick),
                         (notify->is_on_server < 0) ?
                         ((is_on_server) ?
                          _("%snotify: %s%s%s is connected") :
                          _("%snotify: %s%s%s is offline")) :
                         ((is_on_server) ?
                          _("%snotify: %s%s%s has joined") :
                          _("%snotify: %s%s%s has quit")),
                         weechat_prefix ("network"),
                         irc_nick_color_for_server_message (NULL, notify->nick),
                         notify->nick,
                         (is_on_server) ?
                         IRC_COLOR_MESSAGE_JOIN : IRC_COLOR_MESSAGE_QUIT);
    irc_notify_send_signal (notify, (is_on_server) ? "join" : "quit", NULL);

    notify->is_on_server = is_on_server;
}

/*
 * irc_notify_set_away_message: set away message for a notify
 *                              and display message if away status has changed
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
        weechat_printf_tags (notify->server->buffer,
                             irc_notify_get_tags (irc_config_look_notify_tags_whois,
                                                  "away",
                                                  notify->nick),
                             _("%snotify: %s%s%s is now away: \"%s\""),
                             weechat_prefix ("network"),
                             irc_nick_color_for_server_message (NULL, notify->nick),
                             notify->nick,
                             IRC_COLOR_RESET,
                             away_message);
        irc_notify_send_signal (notify, "away", away_message);
    }
    else if (notify->away_message && !away_message)
    {
        weechat_printf_tags (notify->server->buffer,
                             irc_notify_get_tags (irc_config_look_notify_tags_whois,
                                                  "back",
                                                  notify->nick),
                             _("%snotify: %s%s%s is back"),
                             weechat_prefix ("network"),
                             irc_nick_color_for_server_message (NULL, notify->nick),
                             notify->nick,
                             IRC_COLOR_RESET);
        irc_notify_send_signal (notify, "back", NULL);
    }
    else if (notify->away_message && away_message)
    {
        weechat_printf_tags (notify->server->buffer,
                             irc_notify_get_tags (irc_config_look_notify_tags_whois,
                                                  "still_away",
                                                  notify->nick),
                             _("%snotify: %s%s%s is still away: \"%s\""),
                             weechat_prefix ("network"),
                             irc_nick_color_for_server_message (NULL, notify->nick),
                             notify->nick,
                             IRC_COLOR_RESET,
                             away_message);
        irc_notify_send_signal (notify, "still_away", away_message);
    }

    if (notify->away_message)
        free (notify->away_message);
    notify->away_message = (away_message) ? strdup (away_message) : NULL;
}

/*
 * irc_notify_hsignal_cb: callback for hsignal on redirected commands
 *                        "ison" and "whois"
 */

int
irc_notify_hsignal_cb (void *data, const char *signal,
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
        messages = weechat_string_split (output, "\n", 0, 0, &num_messages);
        if (messages)
        {
            nicks_sent = weechat_string_split (ptr_args, " ", 0, 0,
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
                irc_message_parse (ptr_server, messages[i], NULL, NULL, NULL,
                                   NULL, &arguments);
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
                            nicks_recv = weechat_string_split (pos, " ", 0, 0,
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
                                            irc_notify_set_is_on_server (ptr_notify, 1);
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
                        irc_notify_set_is_on_server (ptr_notify, 0);
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
            messages = weechat_string_split (output, "\n", 0, 0, &num_messages);
            if (messages)
            {
                for (i = 0; i < num_messages; i++)
                {
                    irc_message_parse (ptr_server, messages[0], NULL, NULL,
                                       &irc_cmd, NULL, &arguments);
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
                    if (irc_cmd)
                        free (irc_cmd);
                    if (arguments)
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
 * irc_notify_timer_ison_cb: timer called to send "ison" command to servers
 */

int
irc_notify_timer_ison_cb (void *data, int remaining_calls)
{
    char *message, *message2, hash_key[32];
    const char *str_message;
    int total_length, length, nicks_added, number;
    struct t_irc_server *ptr_server;
    struct t_irc_notify *ptr_notify, *ptr_next_notify;
    struct t_hashtable *hashtable;

    /* make C compiler happy */
    (void) data;
    (void) remaining_calls;

    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        if (ptr_server->is_connected && ptr_server->notify_list)
        {
            message = malloc (7);
            if (!message)
                continue;

            snprintf (message, 7, "ISON :");
            total_length = 7;
            nicks_added = 0;

            ptr_notify = ptr_server->notify_list;
            while (ptr_notify)
            {
                ptr_next_notify = ptr_notify->next_notify;

                length = strlen (ptr_notify->nick);
                total_length += length + 1;
                message2 = realloc (message, total_length);
                if (!message2)
                {
                    if (message)
                        free (message);
                    message = NULL;
                    break;
                }
                message = message2;
                if (nicks_added > 0)
                    strcat (message, " ");
                strcat (message, ptr_notify->nick);
                nicks_added++;

                ptr_notify = ptr_next_notify;
            }

            if (message && (nicks_added > 0))
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
                                          NULL, str_message);
                        number++;
                    }
                    weechat_hashtable_free (hashtable);
                }
            }

            if (message)
                free (message);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * irc_notify_timer_whois_cb: timer called to send "whois" command to servers
 */

int
irc_notify_timer_whois_cb (void *data, int remaining_calls)
{
    struct t_irc_server *ptr_server;
    struct t_irc_notify *ptr_notify, *ptr_next_notify;

    /* make C compiler happy */
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
 * irc_notify_hdata_notify_cb: return hdata for notify
 */

struct t_hdata *
irc_notify_hdata_notify_cb (void *data, const char *hdata_name)
{
    struct t_hdata *hdata;

    /* make C compiler happy */
    (void) data;

    hdata = weechat_hdata_new (hdata_name, "prev_notify", "next_notify");
    if (hdata)
    {
        WEECHAT_HDATA_VAR(struct t_irc_notify, server, POINTER, "irc_server");
        WEECHAT_HDATA_VAR(struct t_irc_notify, nick, STRING, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_notify, check_away, INTEGER, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_notify, is_on_server, INTEGER, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_notify, away_message, STRING, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_notify, ison_received, INTEGER, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_notify, prev_notify, POINTER, hdata_name);
        WEECHAT_HDATA_VAR(struct t_irc_notify, next_notify, POINTER, hdata_name);
    }
    return hdata;
}

/*
 * irc_notify_add_to_infolist: add a notify in an infolist
 *                             return 1 if ok, 0 if error
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
 * irc_notify_print_log: print notify infos in log (usually for crash dump)
 */

void
irc_notify_print_log (struct t_irc_server *server)
{
    struct t_irc_notify *ptr_notify;

    for (ptr_notify = server->notify_list; ptr_notify;
         ptr_notify = ptr_notify->next_notify)
    {
        weechat_log_printf ("");
        weechat_log_printf ("  => notify (addr:0x%lx):", ptr_notify);
        weechat_log_printf ("       server. . . . . . . : 0x%lx", ptr_notify->server);
        weechat_log_printf ("       nick. . . . . . . . : '%s'",  ptr_notify->nick);
        weechat_log_printf ("       check_away. . . . . : %d",    ptr_notify->check_away);
        weechat_log_printf ("       is_on_server. . . . : %d",    ptr_notify->is_on_server);
        weechat_log_printf ("       away_message. . . . : '%s'",  ptr_notify->away_message);
        weechat_log_printf ("       ison_received . . . : %d",    ptr_notify->ison_received);
        weechat_log_printf ("       prev_notify . . . . : 0x%lx", ptr_notify->prev_notify);
        weechat_log_printf ("       next_notify . . . . : 0x%lx", ptr_notify->next_notify);
    }
}

/*
 * irc_notify_hook_timer_ison: hook timer to send "ison" command
 */

void
irc_notify_hook_timer_ison ()
{
    if (irc_notify_timer_ison)
        weechat_unhook (irc_notify_timer_ison);

    irc_notify_timer_ison = weechat_hook_timer (
        60 * 1000 * weechat_config_integer (irc_config_network_notify_check_ison),
        0, 0, &irc_notify_timer_ison_cb, NULL);
}

/*
 * irc_notify_hook_timer_whois: hook timer to send "whois" command
 */

void
irc_notify_hook_timer_whois ()
{
    if (irc_notify_timer_whois)
        weechat_unhook (irc_notify_timer_whois);

    irc_notify_timer_whois = weechat_hook_timer (
        60 * 1000 * weechat_config_integer (irc_config_network_notify_check_whois),
        0, 0, &irc_notify_timer_whois_cb, NULL);
}

/*
 * irc_notify_init: hook timers and hsignals
 */

void
irc_notify_init ()
{
    irc_notify_hook_timer_ison ();
    irc_notify_hook_timer_whois ();

    irc_notify_hsignal = weechat_hook_hsignal ("irc_redirection_notify_*",
                                               &irc_notify_hsignal_cb,
                                               NULL);
}

/*
 * irc_notify_end: remove timers and hsignals
 */

void
irc_notify_end ()
{
    if (irc_notify_timer_ison)
        weechat_unhook (irc_notify_timer_ison);
    if (irc_notify_timer_whois)
        weechat_unhook (irc_notify_timer_whois);
    if (irc_notify_hsignal)
        weechat_unhook (irc_notify_hsignal);
}
