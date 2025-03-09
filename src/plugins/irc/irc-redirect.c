/*
 * irc-redirect.c - redirection of IRC command output
 *
 * Copyright (C) 2010-2025 Sébastien Helleu <flashcode@flashtux.org>
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
#include <ctype.h>
#include <time.h>

#include "../weechat-plugin.h"
#include "irc.h"
#include "irc-redirect.h"
#include "irc-server.h"


struct t_irc_redirect_pattern *irc_redirect_patterns = NULL;
struct t_irc_redirect_pattern *last_irc_redirect_pattern = NULL;

/* default redirect patterns */
struct t_irc_redirect_pattern irc_redirect_patterns_default[] =
{
    { "ison", 0, 0,
      /*
       * ison: start: -
       *        stop: 303: ison
       *       extra: -
       */
      NULL,
      "303",
      NULL,
      NULL, NULL,
    },
    { "list", 0, 0,
      /*
       * list: start: 321: /list start
       *        stop: 323: end of /list
       *       extra: -
       */
      "321,322",
      "323",
      NULL,
      NULL, NULL,
    },
    { "mode_channel", 0, 0,
      /*
       * mode_channel: start: -
       *                stop: 324: mode
       *                      403: no such channel
       *                      442: not on channel
       *                      479: cannot join channel (illegal name)
       *               extra: 329: channel creation date
       */
      NULL,
      "324:1,403:1,442:1,479:1",
      "329:1",
      NULL, NULL,
    },
    { "mode_channel_ban", 0, 0, /* mode #channel b */
      /*
       * mode_channel_ban: start: 367: ban
       *                    stop: 368: end of channel ban list
       *                          403: no such channel
       *                          442: not on channel
       *                          479: cannot join channel (illegal name)
       *                   extra: -
       */
      "367:1",
      "368:1,403:1,442:1,479:1",
      NULL,
      NULL, NULL,
    },
    { "mode_channel_ban_exception", 0, 0, /* mode #channel e */
      /*
       * mode_channel_ban_exception: start: 348: ban exception
       *                              stop: 349: end of ban exceptions
       *                                    403: no such channel
       *                                    442: not on channel
       *                                    472: unknown mode char to me
       *                                    479: cannot join channel (illegal name)
       *                                    482: you're not channel operator
       *                             extra: -
       */
      "348:1",
      "349:1,403:1,442:1,472,479:1,482:1",
      NULL,
      NULL, NULL,
    },
    { "mode_channel_invite", 0, 0, /* mode #channel I */
      /*
       * mode_channel_invite: start: 346: invite
       *                       stop: 347: end of invite list
       *                             403: no such channel
       *                             442: not on channel
       *                             472: unknown mode char to me
       *                             479: cannot join channel (illegal name)
       *                             482: you're not channel operator
       *                      extra: -
       */
      "346:1",
      "347:1,403:1,442:1,472,479:1,482:1",
      NULL,
      NULL, NULL,
    },
    { "mode_user", 0, 0,
      /*
       * mode_user: start: -
       *             stop: mode: mode
       *                   221: user mode string
       *                   403: no such channel
       *                   501: unknown mode flag
       *                   502: can't change mode for other users
       *            extra; -
       */
      NULL,
      "mode:0,221:0,403:1,501,502",
      NULL,
      NULL, NULL,
    },
    { "monitor", 0, 0,
      /*
       * monitor: start: 732: list of monitored nicks
       *           stop: 733: end of a monitor list
       *          extra; -
       */
      "732:2",
      "733:1",
      NULL,
      NULL, NULL,
    },
    { "names", 0, 0,
      /*
       * names: start: 353: list of nicks on channel
       *         stop: 366: end of /names list
       *        extra; -
       */
      "353:2",
      "366:1",
      NULL,
      NULL, NULL,
    },
    { "ping", 0, 0,
      /*
       * ping: start: -
       *        stop: pong: pong
       *              402: no such server
       *       extra: -
       */
      NULL,
      "pong,402",
      NULL,
      NULL, NULL,
    },
    { "time", 0, 0,
      /*
       * time: start: -
       *        stop: 391: local time from server
       *       extra: -
       */
      NULL,
      "391",
      NULL,
      NULL, NULL,
    },
    { "topic", 0, 0,
      /*
       * topic: start: -
       *         stop: 331: no topic is set
       *               332: topic
       *               403: no such channel
       *        extra: 333: infos about topic (nick and date changed)
       */
      NULL,
      "331:1,332:1,403:1",
      "333:1",
      NULL, NULL,
    },
    { "userhost", 0, 0,
      /*
       * userhost: start: 401: no such nick/channel
       *            stop: 302: userhost
       *                  461: not enough parameters
       *           extra: -
       */
      "401:1",
      "302,461",
      NULL,
      NULL, NULL,
    },
    { "who", 0, 0,
      /*
       * who: start: 352: who
       *             354: whox
       *             401: no such nick/channel
       *       stop: 315: end of /who list
       *             403: no such channel
       *      extra: -
       */
      "352:1,354,401:1",
      "315:1,403:1",
      NULL,
      NULL, NULL,
    },
    { "whois", 0, 0,
      /*
       * whois: start: 311: whois (user)
       *         stop: 318: whois (end)
       *               401: no such nick/channel
       *               402: no such server
       *               431: no nickname given
       *               461: not enough parameters
       *        extra: 318: whois (end)
       */
      "311:1",
      "318:1,401:1,402:1,431:1,461",
      "318:1",
      NULL, NULL,
    },
    { "whowas", 0, 0,
      /*
       * whowas: start: 314: whowas (user)
       *                406: there was no such nickname
       *          stop: 369: end of whowas
       *         extra: -
       */
      "314:1,406:1",
      "369:1",
      NULL,
      NULL, NULL,
    },
    { NULL, 0, 0, NULL, NULL, NULL, NULL, NULL }
};


/*
 * Searches for a redirect pattern in list of patterns.
 *
 * Returns pointer to redirect pattern found, NULL if not found.
 */

struct t_irc_redirect_pattern *
irc_redirect_pattern_search (const char *name)
{
    struct t_irc_redirect_pattern *ptr_redirect_pattern;

    if (!name)
        return NULL;

    for (ptr_redirect_pattern = irc_redirect_patterns; ptr_redirect_pattern;
         ptr_redirect_pattern = ptr_redirect_pattern->next_redirect)
    {
        if (strcmp (ptr_redirect_pattern->name, name) == 0)
            return ptr_redirect_pattern;
    }

    /* redirect pattern not found */
    return NULL;
}

/*
 * Creates a new redirect pattern.
 *
 * Returns pointer to new redirect pattern, NULL if error.
 */

struct t_irc_redirect_pattern *
irc_redirect_pattern_new (const char *name, int temp_pattern, int timeout,
                          const char *cmd_start, const char *cmd_stop,
                          const char *cmd_extra)
{
    struct t_irc_redirect_pattern *ptr_redirect_pattern, *new_redirect_pattern;

    if (!name)
        return NULL;

    if (!cmd_stop || !cmd_stop[0])
    {
        weechat_printf (
            NULL,
            _("%s%s: missing argument \"%s\" for redirect pattern"),
            weechat_prefix ("error"), IRC_PLUGIN_NAME, "cmd_stop");
        return NULL;
    }

    /* check if redirect pattern already exists */
    ptr_redirect_pattern = irc_redirect_pattern_search (name);
    if (ptr_redirect_pattern)
    {
        weechat_printf (
            NULL,
            _("%s%s: redirect pattern \"%s\" already exists"),
            weechat_prefix ("error"), IRC_PLUGIN_NAME, name);
        return NULL;
    }

    new_redirect_pattern = malloc (sizeof (*new_redirect_pattern));
    if (!new_redirect_pattern)
        return NULL;

    /* initialize new redirect */
    new_redirect_pattern->name = strdup (name);
    new_redirect_pattern->temp_pattern = temp_pattern;
    new_redirect_pattern->timeout = (timeout > 0) ? timeout : IRC_REDIRECT_TIMEOUT_DEFAULT;
    new_redirect_pattern->cmd_start = (cmd_start) ? strdup (cmd_start) : NULL;
    new_redirect_pattern->cmd_stop = strdup (cmd_stop);
    new_redirect_pattern->cmd_extra = (cmd_extra) ? strdup (cmd_extra) : NULL;

    /* add redirect pattern to end of list */
    new_redirect_pattern->prev_redirect = last_irc_redirect_pattern;
    if (last_irc_redirect_pattern)
        last_irc_redirect_pattern->next_redirect = new_redirect_pattern;
    else
        irc_redirect_patterns = new_redirect_pattern;
    last_irc_redirect_pattern = new_redirect_pattern;
    new_redirect_pattern->next_redirect = NULL;

    return new_redirect_pattern;
}

/*
 * Frees a redirect pattern and removes it from list.
 */

void
irc_redirect_pattern_free (struct t_irc_redirect_pattern *redirect_pattern)
{
    struct t_irc_redirect_pattern *new_redirect_patterns;

    if (!redirect_pattern)
        return;

    /* remove redirect */
    if (last_irc_redirect_pattern == redirect_pattern)
        last_irc_redirect_pattern = redirect_pattern->prev_redirect;
    if (redirect_pattern->prev_redirect)
    {
        (redirect_pattern->prev_redirect)->next_redirect = redirect_pattern->next_redirect;
        new_redirect_patterns = irc_redirect_patterns;
    }
    else
        new_redirect_patterns = redirect_pattern->next_redirect;

    if (redirect_pattern->next_redirect)
        (redirect_pattern->next_redirect)->prev_redirect = redirect_pattern->prev_redirect;

    /* free data */
    free (redirect_pattern->name);
    free (redirect_pattern->cmd_start);
    free (redirect_pattern->cmd_stop);
    free (redirect_pattern->cmd_extra);

    free (redirect_pattern);

    irc_redirect_patterns = new_redirect_patterns;
}

/*
 * Frees all redirect patterns.
 */

void
irc_redirect_pattern_free_all (void)
{
    while (irc_redirect_patterns)
    {
        irc_redirect_pattern_free (irc_redirect_patterns);
    }
}

/*
 * Creates a new redirect for a command on a server (with start/stop/extra
 * commands in arguments).
 *
 * Returns pointer to new redirect, NULL if error.
 */

struct t_irc_redirect *
irc_redirect_new_with_commands (struct t_irc_server *server,
                                const char *pattern, const char *signal,
                                int count, const char *string, int timeout,
                                const char *cmd_start,
                                const char *cmd_stop,
                                const char *cmd_extra,
                                const char *cmd_filter)
{
    struct t_irc_redirect *new_redirect;
    char **items[4], *item_upper, *pos, *error;
    int i, j, num_items[4];
    long value;
    struct t_hashtable *hash_cmd[4];

    new_redirect = malloc (sizeof (*new_redirect));
    if (!new_redirect)
        return NULL;

    /* create hashtables with commands */
    for (i = 0; i < 4; i++)
    {
        hash_cmd[i] = NULL;
        items[i] = NULL;
    }
    if (cmd_start)
        items[0] = weechat_string_split (cmd_start, ",", NULL,
                                         WEECHAT_STRING_SPLIT_STRIP_LEFT
                                         | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                         | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                         0, &num_items[0]);
    if (cmd_stop)
        items[1] = weechat_string_split (cmd_stop, ",", NULL,
                                         WEECHAT_STRING_SPLIT_STRIP_LEFT
                                         | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                         | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                         0, &num_items[1]);
    if (cmd_extra)
        items[2] = weechat_string_split (cmd_extra, ",", NULL,
                                         WEECHAT_STRING_SPLIT_STRIP_LEFT
                                         | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                         | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                         0, &num_items[2]);
    if (cmd_filter)
        items[3] = weechat_string_split (cmd_filter, ",", NULL,
                                         WEECHAT_STRING_SPLIT_STRIP_LEFT
                                         | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                         | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                         0, &num_items[3]);
    for (i = 0; i < 4; i++)
    {
        if (items[i])
        {
            hash_cmd[i] = weechat_hashtable_new (32,
                                                 WEECHAT_HASHTABLE_STRING,
                                                 WEECHAT_HASHTABLE_INTEGER,
                                                 NULL, NULL);
            for (j = 0; j < num_items[i]; j++)
            {
                if (i < 3)
                {
                    value = -1;
                    pos = strchr (items[i][j], ':');
                    if (pos)
                    {
                        pos[0] = '\0';
                        error = NULL;
                        value = strtol (pos + 1, &error, 10);
                        if (!error || error[0])
                            value = -1;
                    }
                    item_upper = weechat_string_toupper (items[i][j]);
                    if (item_upper)
                    {
                        weechat_hashtable_set (hash_cmd[i], item_upper, &value);
                        free (item_upper);
                    }
                }
                else
                {
                    weechat_hashtable_set (hash_cmd[i], items[i][j], NULL);
                }
            }
            weechat_string_free_split (items[i]);
        }
    }

    /* initialize new redirect */
    new_redirect->server = server;
    new_redirect->pattern = strdup (pattern);
    new_redirect->signal = strdup (signal);
    new_redirect->count = (count >= 1) ? count : 1;
    new_redirect->current_count = 1;
    new_redirect->string = (string) ? strdup (string) : NULL;
    new_redirect->timeout = timeout;
    new_redirect->command = NULL;
    new_redirect->assigned_to_command = 0;
    new_redirect->start_time = 0;
    new_redirect->cmd_start = hash_cmd[0];
    new_redirect->cmd_stop = hash_cmd[1];
    new_redirect->cmd_extra = hash_cmd[2];
    new_redirect->cmd_start_received = 0;
    new_redirect->cmd_stop_received = 0;
    new_redirect->cmd_filter = hash_cmd[3];
    new_redirect->output = NULL;
    new_redirect->output_size = 0;

    /* add redirect to end of list */
    new_redirect->prev_redirect = server->last_redirect;
    if (server->last_redirect)
        (server->last_redirect)->next_redirect = new_redirect;
    else
        server->redirects = new_redirect;
    server->last_redirect = new_redirect;
    new_redirect->next_redirect = NULL;

    return new_redirect;
}

/*
 * Creates a new redirect for a command on a server.
 *
 * Returns pointer to new redirect, NULL if error.
 */

struct t_irc_redirect *
irc_redirect_new (struct t_irc_server *server,
                  const char *pattern, const char *signal,
                  int count, const char *string, int timeout,
                  const char *cmd_filter)
{
    struct t_irc_redirect_pattern *ptr_redirect_pattern;
    struct t_irc_redirect *new_redirect;

    if (!server->is_connected)
    {
        weechat_printf (
            NULL,
            _("%s%s: no connection to server \"%s\" for redirect"),
            weechat_prefix ("error"), IRC_PLUGIN_NAME, server->name);
        return NULL;
    }

    if (!pattern || !pattern[0])
    {
        weechat_printf (
            NULL,
            _("%s%s: missing argument \"%s\" for redirect"),
            weechat_prefix ("error"), IRC_PLUGIN_NAME, "pattern");
        return NULL;
    }
    if (!signal || !signal[0])
    {
        weechat_printf (
            NULL,
            _("%s%s: missing argument \"%s\" for redirect"),
            weechat_prefix ("error"), IRC_PLUGIN_NAME, "signal");
        return NULL;
    }

    ptr_redirect_pattern = irc_redirect_pattern_search (pattern);
    if (!ptr_redirect_pattern)
    {
        weechat_printf (
            NULL,
            _("%s%s: redirect pattern \"%s\" not found"),
            weechat_prefix ("error"), IRC_PLUGIN_NAME, pattern);
        return NULL;
    }

    new_redirect = irc_redirect_new_with_commands (
        server, pattern, signal,
        count, string,
        (timeout > 0) ? timeout : ptr_redirect_pattern->timeout,
        ptr_redirect_pattern->cmd_start,
        ptr_redirect_pattern->cmd_stop,
        ptr_redirect_pattern->cmd_extra,
        cmd_filter);

    /*
     * remove redirect pattern if it is temporary (created by external
     * plugin/script)
     */
    if (new_redirect && ptr_redirect_pattern->temp_pattern)
        irc_redirect_pattern_free (ptr_redirect_pattern);

    return new_redirect;
}

/*
 * Searches for first redirect available for server.
 *
 * Returns pointer to redirect found, NULL if not found.
 */

struct t_irc_redirect *
irc_redirect_search_available (struct t_irc_server *server)
{
    struct t_irc_redirect *ptr_redirect;

    if (!server)
        return NULL;

    for (ptr_redirect = server->redirects; ptr_redirect;
         ptr_redirect = ptr_redirect->next_redirect)
    {
        if (!ptr_redirect->assigned_to_command)
            return ptr_redirect;
    }

    /* no redirect available */
    return NULL;
}

/*
 * Initializes a redirect with IRC command sent to server.
 */

void
irc_redirect_init_command (struct t_irc_redirect *redirect,
                           const char *command)
{
    char *pos;

    if (!redirect)
        return;

    if (command)
    {
        pos = strchr (command, '\r');
        if (!pos)
            pos = strchr (command, '\n');
        if (pos)
            redirect->command = weechat_strndup (command, pos - command);
        else
            redirect->command = strdup (command);
    }
    else
        redirect->command = NULL;

    redirect->assigned_to_command = 1;
    redirect->start_time = time (NULL);

    if (weechat_irc_plugin->debug >= 2)
    {
        weechat_printf (
            redirect->server->buffer,
            _("%s: starting redirection for command \"%s\" on server \"%s\" "
              "(redirect pattern: \"%s\")"),
            IRC_PLUGIN_NAME, redirect->command, redirect->server->name,
            redirect->pattern);
    }
}

/*
 * Checks if a message matches hashtable with commands.
 *
 * Returns:
 *   1: message matches hashtable
 *   0: message does not match hashtable
 */

int
irc_redirect_message_match_hash (struct t_irc_redirect *redirect,
                                 const char *command,
                                 char **arguments_argv, int arguments_argc,
                                 struct t_hashtable *cmd_hash)
{
    int *value;

    value = weechat_hashtable_get (cmd_hash, command);
    if (!value)
        return 0;

    /*
     * if string is in redirect and that this command requires string to
     * be in message, then search for this string
     */
    if (redirect->string && redirect->string[0] && (*value >= 0))
    {
        if (!arguments_argv || (*value >= arguments_argc))
            return 0;

        if (weechat_strcasecmp (arguments_argv[*value], redirect->string) != 0)
            return 0;
    }

    return 1;
}

/*
 * Adds a message to redirect output.
 */

void
irc_redirect_message_add (struct t_irc_redirect *redirect, const char *message,
                          const char *command)
{
    char *output2;

    /*
     * if command is not for output, then don't add message
     * (it is silently ignored)
     */
    if (redirect->cmd_filter
        && !weechat_hashtable_has_key (redirect->cmd_filter, command))
        return;

    /* add message to output */
    if (redirect->output)
    {
        redirect->output_size += strlen ("\n") + strlen (message);
        output2 = realloc (redirect->output, redirect->output_size);
        if (!output2)
        {
            free (redirect->output);
            redirect->output = NULL;
            redirect->output_size = 0;
            return;
        }
        redirect->output = output2;
        strcat (redirect->output, "\n");
    }
    else
    {
        redirect->output_size = strlen (message) + 1;
        redirect->output = malloc (redirect->output_size);
        if (redirect->output)
            redirect->output[0] = '\0';
    }
    if (redirect->output)
        strcat (redirect->output, message);
}

/*
 * Ends a redirection: sends data to callback and frees redirect (if count has
 * been reached).
 */

void
irc_redirect_stop (struct t_irc_redirect *redirect, const char *error)
{
    struct t_hashtable *hashtable;
    char signal_name[1024], str_int[64];

    redirect->current_count++;

    if (error || (redirect->current_count > redirect->count))
    {
        /*
         * error or max count reached, then we run callback and remove
         * redirect
         */
        hashtable = weechat_hashtable_new (32,
                                           WEECHAT_HASHTABLE_STRING,
                                           WEECHAT_HASHTABLE_STRING,
                                           NULL, NULL);
        if (hashtable)
        {
            /* set error and output (main fields) */
            weechat_hashtable_set (hashtable, "error",
                                   (error) ? (char *)error : "");
            weechat_hashtable_set (hashtable, "output",
                                   (redirect->output) ? redirect->output : "");
            snprintf (str_int, sizeof (str_int), "%d", redirect->output_size);
            weechat_hashtable_set (hashtable, "output_size", str_int);

            /* set some other fields with values from redirect */
            weechat_hashtable_set (hashtable, "server", redirect->server->name);
            weechat_hashtable_set (hashtable, "pattern", redirect->pattern);
            weechat_hashtable_set (hashtable, "signal", redirect->signal);
            weechat_hashtable_set (hashtable, "command", redirect->command);
        }

        snprintf (signal_name, sizeof (signal_name), "irc_redirection_%s_%s",
                  redirect->signal, redirect->pattern);
        (void) weechat_hook_hsignal_send (signal_name, hashtable);

        weechat_hashtable_free (hashtable);

        irc_redirect_free (redirect);
    }
    else
    {
        /*
         * max count not yet reached, then we prepare redirect to continue
         * redirection
         */
        redirect->cmd_start_received = 0;
        redirect->cmd_stop_received = 0;
    }
}

/*
 * Tries to redirect a received message (from IRC server) to a redirect in
 * server.
 *
 * Returns:
 *   1: message has been redirected (irc plugin will discard message)
 *   0: no matching redirect was found
 */

int
irc_redirect_message (struct t_irc_server *server, const char *message,
                      const char *command, const char *arguments)
{
    struct t_irc_redirect *ptr_redirect, *ptr_next_redirect;
    int rc, match_stop, arguments_argc;
    char **arguments_argv;

    if (!server || !server->redirects || !message || !command)
        return 0;

    rc = 0;

    if (arguments && arguments[0])
    {
        arguments_argv = weechat_string_split (
            arguments,
            " ",
            NULL,
            WEECHAT_STRING_SPLIT_STRIP_LEFT
            | WEECHAT_STRING_SPLIT_STRIP_RIGHT
            | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
            0,
            &arguments_argc);
    }
    else
    {
        arguments_argv = NULL;
        arguments_argc = 0;
    }

    ptr_redirect = server->redirects;
    while (ptr_redirect)
    {
        ptr_next_redirect = ptr_redirect->next_redirect;

        if (ptr_redirect->start_time > 0)
        {
            if (ptr_redirect->cmd_stop_received)
            {
                if (ptr_redirect->cmd_extra
                    && irc_redirect_message_match_hash (ptr_redirect,
                                                        command,
                                                        arguments_argv,
                                                        arguments_argc,
                                                        ptr_redirect->cmd_extra))
                {
                    irc_redirect_message_add (ptr_redirect, message, command);
                    irc_redirect_stop (ptr_redirect, NULL);
                    rc = 1;
                    goto end;
                }
                irc_redirect_stop (ptr_redirect, NULL);
            }
            else
            {
                /* message matches a start command? */
                if (ptr_redirect->cmd_start
                    && !ptr_redirect->cmd_start_received
                    && irc_redirect_message_match_hash (ptr_redirect,
                                                        command,
                                                        arguments_argv,
                                                        arguments_argc,
                                                        ptr_redirect->cmd_start))
                {
                    /*
                     * message is a start command for redirection, then add
                     * message to output for redirection and mark start
                     * command as "received" for this redirect
                     */
                    irc_redirect_message_add (ptr_redirect, message, command);
                    ptr_redirect->cmd_start_received = 1;
                    rc = 1;
                    goto end;
                }
                /*
                 * if matching stop command, or start command received, we are
                 * in redirection: add message to output and close redirection
                 * if matching stop command
                 */
                match_stop = irc_redirect_message_match_hash (ptr_redirect,
                                                              command,
                                                              arguments_argv,
                                                              arguments_argc,
                                                              ptr_redirect->cmd_stop);
                if (match_stop || ptr_redirect->cmd_start_received)
                {
                    /*
                     * add message to output if matching stop or if command
                     * is numeric
                     */
                    irc_redirect_message_add (ptr_redirect, message, command);
                    if (match_stop)
                    {
                        ptr_redirect->cmd_stop_received = 1;
                        if (ptr_redirect->cmd_extra)
                        {
                            if (irc_redirect_message_match_hash (ptr_redirect,
                                                                 command,
                                                                 arguments_argv,
                                                                 arguments_argc,
                                                                 ptr_redirect->cmd_extra))
                            {
                                /*
                                 * this command is a stop and extra command,
                                 * then remove redirect
                                 */
                                irc_redirect_stop (ptr_redirect, NULL);
                            }
                        }
                        else
                        {
                            /*
                             * no extra command after stop, then remove
                             * redirect
                             */
                            irc_redirect_stop (ptr_redirect, NULL);
                        }
                    }
                    rc = 1;
                    goto end;
                }
            }
        }

        ptr_redirect = ptr_next_redirect;
    }

end:
    weechat_string_free_split (arguments_argv);
    return rc;
}

/*
 * Frees a redirect and removes it from list.
 */

void
irc_redirect_free (struct t_irc_redirect *redirect)
{
    struct t_irc_server *server;
    struct t_irc_redirect *new_redirects;
    int priority;
    struct t_irc_outqueue *ptr_outqueue;

    if (!redirect)
        return;

    server = redirect->server;

    /* remove redirect */
    if (server->last_redirect == redirect)
        server->last_redirect = redirect->prev_redirect;
    if (redirect->prev_redirect)
    {
        (redirect->prev_redirect)->next_redirect = redirect->next_redirect;
        new_redirects = server->redirects;
    }
    else
        new_redirects = redirect->next_redirect;

    if (redirect->next_redirect)
        (redirect->next_redirect)->prev_redirect = redirect->prev_redirect;

    /* remove any pointer to this redirect */
    for (priority = 0; priority < IRC_SERVER_NUM_OUTQUEUES_PRIO; priority++)
    {
        for (ptr_outqueue = server->outqueue[priority]; ptr_outqueue;
             ptr_outqueue = ptr_outqueue->next_outqueue)
        {
            if (ptr_outqueue->redirect == redirect)
                ptr_outqueue->redirect = NULL;
        }
    }

    /* free data */
    free (redirect->pattern);
    free (redirect->signal);
    free (redirect->string);
    free (redirect->command);
    weechat_hashtable_free (redirect->cmd_start);
    weechat_hashtable_free (redirect->cmd_stop);
    weechat_hashtable_free (redirect->cmd_extra);
    weechat_hashtable_free (redirect->cmd_filter);
    free (redirect->output);

    free (redirect);

    server->redirects = new_redirects;
}

/*
 * Frees all redirects in list.
 */

void
irc_redirect_free_all (struct t_irc_server *server)
{
    while (server->redirects)
    {
        irc_redirect_free (server->redirects);
    }
}

/*
 * Returns hdata for redirect pattern.
 */

struct t_hdata *
irc_redirect_hdata_redirect_pattern_cb (const void *pointer, void *data,
                                        const char *hdata_name)
{
    struct t_hdata *hdata;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    hdata = weechat_hdata_new (hdata_name, "prev_redirect", "next_redirect",
                               0, 0, NULL, NULL);
    if (hdata)
    {
        WEECHAT_HDATA_VAR(struct t_irc_redirect_pattern, name, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_redirect_pattern, temp_pattern, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_redirect_pattern, timeout, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_redirect_pattern, cmd_start, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_redirect_pattern, cmd_stop, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_redirect_pattern, cmd_extra, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_redirect_pattern, prev_redirect, POINTER, 0, NULL, hdata_name);
        WEECHAT_HDATA_VAR(struct t_irc_redirect_pattern, next_redirect, POINTER, 0, NULL, hdata_name);
        WEECHAT_HDATA_LIST(irc_redirect_patterns, WEECHAT_HDATA_LIST_CHECK_POINTERS);
        WEECHAT_HDATA_LIST(last_irc_redirect_pattern, 0);
    }
    return hdata;
}

/*
 * Returns hdata for redirect.
 */

struct t_hdata *
irc_redirect_hdata_redirect_cb (const void *pointer, void *data,
                                const char *hdata_name)
{
    struct t_hdata *hdata;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    hdata = weechat_hdata_new (hdata_name, "prev_redirect", "next_redirect",
                               0, 0, NULL, NULL);
    if (hdata)
    {
        WEECHAT_HDATA_VAR(struct t_irc_redirect, server, POINTER, 0, NULL, "irc_server");
        WEECHAT_HDATA_VAR(struct t_irc_redirect, pattern, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_redirect, signal, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_redirect, count, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_redirect, current_count, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_redirect, string, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_redirect, timeout, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_redirect, command, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_redirect, assigned_to_command, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_redirect, start_time, TIME, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_redirect, cmd_start, HASHTABLE, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_redirect, cmd_stop, HASHTABLE, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_redirect, cmd_extra, HASHTABLE, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_redirect, cmd_start_received, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_redirect, cmd_stop_received, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_redirect, cmd_filter, HASHTABLE, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_redirect, output, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_redirect, output_size, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_redirect, prev_redirect, POINTER, 0, NULL, hdata_name);
        WEECHAT_HDATA_VAR(struct t_irc_redirect, next_redirect, POINTER, 0, NULL, hdata_name);
    }
    return hdata;
}

/*
 * Adds a redirect pattern in an infolist.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
irc_redirect_pattern_add_to_infolist (struct t_infolist *infolist,
                                      struct t_irc_redirect_pattern *redirect_pattern)
{
    struct t_infolist_item *ptr_item;

    if (!infolist || !redirect_pattern)
        return 0;

    ptr_item = weechat_infolist_new_item (infolist);
    if (!ptr_item)
        return 0;

    if (!weechat_infolist_new_var_string (ptr_item, "name", redirect_pattern->name))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "temp_pattern", redirect_pattern->temp_pattern))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "timeout", redirect_pattern->timeout))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "cmd_start", redirect_pattern->cmd_start))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "cmd_stop", redirect_pattern->cmd_stop))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "cmd_extra", redirect_pattern->cmd_extra))
        return 0;

    return 1;
}

/*
 * Adds a redirect in an infolist.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
irc_redirect_add_to_infolist (struct t_infolist *infolist,
                              struct t_irc_redirect *redirect)
{
    struct t_infolist_item *ptr_item;

    if (!infolist || !redirect)
        return 0;

    ptr_item = weechat_infolist_new_item (infolist);
    if (!ptr_item)
        return 0;

    if (!weechat_infolist_new_var_pointer (ptr_item, "server", redirect->server))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "server_name", redirect->server->name))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "pattern", redirect->pattern))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "signal", redirect->signal))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "count", redirect->count))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "current_count", redirect->current_count))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "string", redirect->string))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "timeout", redirect->timeout))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "command", redirect->command))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "assigned_to_command", redirect->assigned_to_command))
        return 0;
    if (!weechat_infolist_new_var_time (ptr_item, "start_time", redirect->start_time))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "cmd_start", weechat_hashtable_get_string (redirect->cmd_start, "keys_values")))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "cmd_stop", weechat_hashtable_get_string (redirect->cmd_stop, "keys_values")))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "cmd_extra", weechat_hashtable_get_string (redirect->cmd_extra, "keys_values")))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "cmd_start_received", redirect->cmd_start_received))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "cmd_stop_received", redirect->cmd_stop_received))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "cmd_filter", weechat_hashtable_get_string (redirect->cmd_filter, "keys_values")))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "output", redirect->output))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "output_size", redirect->output_size))
        return 0;

    return 1;
}

/*
 * Prints redirect infos in WeeChat log file (usually for crash dump).
 */

void
irc_redirect_pattern_print_log (void)
{
    struct t_irc_redirect_pattern *ptr_redirect_pattern;

    for (ptr_redirect_pattern = irc_redirect_patterns; ptr_redirect_pattern;
         ptr_redirect_pattern = ptr_redirect_pattern->next_redirect)
    {
        weechat_log_printf ("");
        weechat_log_printf ("[redirect_pattern (addr:%p)]", ptr_redirect_pattern);
        weechat_log_printf ("  name . . . . . . . . : '%s'", ptr_redirect_pattern->name);
        weechat_log_printf ("  temp_pattern . . . . : %d", ptr_redirect_pattern->temp_pattern);
        weechat_log_printf ("  timeout. . . . . . . : %d", ptr_redirect_pattern->timeout);
        weechat_log_printf ("  cmd_start. . . . . . : '%s'", ptr_redirect_pattern->cmd_start);
        weechat_log_printf ("  cmd_stop . . . . . . : '%s'", ptr_redirect_pattern->cmd_stop);
        weechat_log_printf ("  cmd_extra. . . . . . : '%s'", ptr_redirect_pattern->cmd_extra);
        weechat_log_printf ("  prev_redirect. . . . : %p", ptr_redirect_pattern->prev_redirect);
        weechat_log_printf ("  next_redirect. . . . : %p", ptr_redirect_pattern->next_redirect);
    }
}

/*
 * Prints redirect infos in WeeChat log file (usually for crash dump).
 */

void
irc_redirect_print_log (struct t_irc_server *server)
{
    struct t_irc_redirect *ptr_redirect;

    for (ptr_redirect = server->redirects; ptr_redirect;
         ptr_redirect = ptr_redirect->next_redirect)
    {
        weechat_log_printf ("");
        weechat_log_printf ("  => redirect (addr:%p):", ptr_redirect);
        weechat_log_printf ("       server. . . . . . . : %p ('%s')",
                            ptr_redirect->server, ptr_redirect->server->name);
        weechat_log_printf ("       pattern . . . . . . : '%s'", ptr_redirect->pattern);
        weechat_log_printf ("       signal. . . . . . . : '%s'", ptr_redirect->signal);
        weechat_log_printf ("       count . . . . . . . : %d", ptr_redirect->count);
        weechat_log_printf ("       current_count . . . : %d", ptr_redirect->current_count);
        weechat_log_printf ("       string. . . . . . . : '%s'", ptr_redirect->string);
        weechat_log_printf ("       timeout . . . . . . : %d", ptr_redirect->timeout);
        weechat_log_printf ("       command . . . . . . : '%s'", ptr_redirect->command);
        weechat_log_printf ("       assigned_to_command : %d", ptr_redirect->assigned_to_command);
        weechat_log_printf ("       start_time. . . . . : %lld", (long long)ptr_redirect->start_time);
        weechat_log_printf ("       cmd_start . . . . . : %p (hashtable: '%s')",
                            ptr_redirect->cmd_start,
                            weechat_hashtable_get_string (ptr_redirect->cmd_start, "keys_values"));
        weechat_log_printf ("       cmd_stop. . . . . . : %p (hashtable: '%s')",
                            ptr_redirect->cmd_stop,
                            weechat_hashtable_get_string (ptr_redirect->cmd_stop, "keys_values"));
        weechat_log_printf ("       cmd_extra . . . . . : %p (hashtable: '%s')",
                            ptr_redirect->cmd_extra,
                            weechat_hashtable_get_string (ptr_redirect->cmd_extra, "keys_values"));
        weechat_log_printf ("       cmd_start_received. : %d", ptr_redirect->cmd_start_received);
        weechat_log_printf ("       cmd_stop_received . : %d", ptr_redirect->cmd_stop_received);
        weechat_log_printf ("       cmd_filter. . . . . : %p (hashtable: '%s')",
                            ptr_redirect->cmd_filter,
                            weechat_hashtable_get_string (ptr_redirect->cmd_filter, "keys_values"));
        weechat_log_printf ("       output. . . . . . . : '%s'", ptr_redirect->output);
        weechat_log_printf ("       output_size . . . . : %d", ptr_redirect->output_size);
        weechat_log_printf ("       prev_redirect . . . : %p", ptr_redirect->prev_redirect);
        weechat_log_printf ("       next_redirect . . . : %p", ptr_redirect->next_redirect);
    }
}

/*
 * Callback for hsignal "irc_redirect_pattern".
 *
 * It is called when other plugins/scripts are creating a redirect pattern (irc
 * plugin itself does not use this function).
 */

int
irc_redirect_pattern_hsignal_cb (const void *pointer, void *data,
                                 const char *signal,
                                 struct t_hashtable *hashtable)
{
    const char *pattern, *str_timeout, *cmd_start, *cmd_stop, *cmd_extra;
    char *error;
    int number, timeout;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) signal;

    if (!hashtable)
        return WEECHAT_RC_ERROR;

    pattern = weechat_hashtable_get (hashtable, "pattern");
    str_timeout = weechat_hashtable_get (hashtable, "timeout");
    cmd_start = weechat_hashtable_get (hashtable, "cmd_start");
    cmd_stop = weechat_hashtable_get (hashtable, "cmd_stop");
    cmd_extra = weechat_hashtable_get (hashtable, "cmd_extra");

    if (!pattern || !pattern[0])
    {
        weechat_printf (
            NULL,
            _("%s%s: missing argument \"%s\" for redirect pattern"),
            weechat_prefix ("error"), IRC_PLUGIN_NAME, "pattern");
        return WEECHAT_RC_ERROR;
    }

    if (!cmd_stop || !cmd_stop[0])
    {
        weechat_printf (
            NULL,
            _("%s%s: missing argument \"%s\" for redirect pattern"),
            weechat_prefix ("error"), IRC_PLUGIN_NAME, "cmd_stop");
        return WEECHAT_RC_ERROR;
    }

    timeout = 0;
    if (str_timeout && str_timeout[0])
    {
        error = NULL;
        number = (int)strtol (str_timeout, &error, 10);
        if (error && !error[0])
            timeout = number;
    }

    /*
     * create a temporary redirect pattern (it will be removed when a
     * redirect will use it)
     */
    irc_redirect_pattern_new (pattern, 1, timeout,
                              cmd_start, cmd_stop, cmd_extra);

    return WEECHAT_RC_OK;
}

/*
 * Callback for hsignal "irc_redirect_command".
 *
 * It is called when other plugins/scripts are redirecting an IRC command (irc
 * plugin itself does not use this function).
 */

int
irc_redirect_command_hsignal_cb (const void *pointer, void *data,
                                 const char *signal,
                                 struct t_hashtable *hashtable)
{
    const char *server, *pattern, *redirect_signal, *str_count, *string;
    const char *str_timeout, *cmd_filter;
    char *error;
    struct t_irc_server *ptr_server;
    int number, count, timeout;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) signal;

    if (!hashtable)
        return WEECHAT_RC_ERROR;

    server = weechat_hashtable_get (hashtable, "server");
    pattern = weechat_hashtable_get (hashtable, "pattern");
    redirect_signal = weechat_hashtable_get (hashtable, "signal");
    str_count = weechat_hashtable_get (hashtable, "count");
    string = weechat_hashtable_get (hashtable, "string");
    str_timeout = weechat_hashtable_get (hashtable, "timeout");
    cmd_filter = weechat_hashtable_get (hashtable, "cmd_filter");

    if (!server || !server[0])
    {
        weechat_printf (
            NULL,
            _("%s%s: missing argument \"%s\" for redirect"),
            weechat_prefix ("error"), IRC_PLUGIN_NAME, "server");
        return WEECHAT_RC_ERROR;
    }
    ptr_server = irc_server_search (server);
    if (!ptr_server)
    {
        weechat_printf (
            NULL,
            _("%s%s: server \"%s\" not found for redirect"),
            weechat_prefix ("error"), IRC_PLUGIN_NAME, server);
        return WEECHAT_RC_ERROR;
    }

    count = 1;
    if (str_count && str_count[0])
    {
        error = NULL;
        number = (int)strtol (str_count, &error, 10);
        if (error && !error[0])
            count = number;
    }

    timeout = 0;
    if (str_timeout && str_timeout[0])
    {
        error = NULL;
        number = (int)strtol (str_timeout, &error, 10);
        if (error && !error[0])
            timeout = number;
    }

    irc_redirect_new (ptr_server, pattern, redirect_signal,
                      count, string, timeout, cmd_filter);

    return WEECHAT_RC_OK;
}

/*
 * Creates default redirect patterns.
 */

void
irc_redirect_init (void)
{
    int i;

    for (i = 0; irc_redirect_patterns_default[i].name; i++)
    {
        irc_redirect_pattern_new (irc_redirect_patterns_default[i].name,
                                  0,
                                  irc_redirect_patterns_default[i].timeout,
                                  irc_redirect_patterns_default[i].cmd_start,
                                  irc_redirect_patterns_default[i].cmd_stop,
                                  irc_redirect_patterns_default[i].cmd_extra);
    }
}

/*
 * Frees all redirect patterns.
 */

void
irc_redirect_end (void)
{
    irc_redirect_pattern_free_all ();
}
