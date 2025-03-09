/*
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

#ifndef WEECHAT_PLUGIN_IRC_REDIRECT_H
#define WEECHAT_PLUGIN_IRC_REDIRECT_H

#include <time.h>

#define IRC_REDIRECT_TIMEOUT_DEFAULT 60

struct t_irc_server;

/* template for redirections (IRC plugin creates some templates at startup) */

struct t_irc_redirect_pattern
{
    char *name;                     /* name                                  */
    int temp_pattern;               /* temporary pattern (created by         */
                                    /* external plugin/script)               */
    int timeout;                    /* default timeout (in seconds)          */
    char *cmd_start;                /* command(s) starting redirection       */
                                    /* (can be NULL or empty)                */
    char *cmd_stop;                 /* command(s) stopping redirection       */
                                    /* (not NULL, at least one command)      */
    char *cmd_extra;                /* extra command(s) after end commands   */
    struct t_irc_redirect_pattern *prev_redirect; /* link to previous redir. */
    struct t_irc_redirect_pattern *next_redirect; /* link to next redir.     */
};

/* command redirection (created when a command is redirected) */

struct t_irc_redirect
{
    struct t_irc_server *server;    /* server for this redirection           */
    char *pattern;                  /* name of pattern used for this redir.  */
    char *signal;                   /* name of signal sent after redirection */
    int count;                      /* how many times redirect is executed   */
    int current_count;              /* current count                         */
    char *string;                   /* we search this string in messages     */
    int timeout;                    /* timeout (in seconds)                  */
    char *command;                  /* command sent to server, which is      */
                                    /* redirected                            */
    int assigned_to_command;        /* 1 if assigned to a command            */
    time_t start_time;              /* time when command is sent to server   */
                                    /* (this is beginning of this redirect)  */
    struct t_hashtable *cmd_start;  /* command(s) starting redirection       */
                                    /* (can be NULL or empty)                */
    struct t_hashtable *cmd_stop;   /* command(s) stopping redirection       */
                                    /* (not NULL, at least one command)      */
    struct t_hashtable *cmd_extra;  /* extra command(s) after end command(s) */
    int cmd_start_received;         /* one of start commands received ?      */
    int cmd_stop_received;          /* one of stop commands received ?       */
    struct t_hashtable *cmd_filter; /* command(s) to add to output           */
                                    /* (if NULL or empty, all cmds are sent) */
    char *output;                   /* output of IRC command (gradually      */
                                    /* filled with IRC messages)             */
    int output_size;                /* size (in bytes) of output string      */
    struct t_irc_redirect *prev_redirect; /* link to previous redirect       */
    struct t_irc_redirect *next_redirect; /* link to next redirect           */
};

extern struct t_irc_redirect_pattern *irc_redirect_patterns;
extern struct t_irc_redirect_pattern *last_irc_redirect_pattern;

extern struct t_irc_redirect_pattern *irc_redirect_pattern_new (const char *name,
                                                                int temp_pattern,
                                                                int timeout,
                                                                const char *cmd_start,
                                                                const char *cmd_stop,
                                                                const char *cmd_extra);
extern struct t_irc_redirect *irc_redirect_new_with_commands (struct t_irc_server *server,
                                                              const char *pattern,
                                                              const char *signal,
                                                              int count,
                                                              const char *string,
                                                              int timeout,
                                                              const char *cmd_start,
                                                              const char *cmd_stop,
                                                              const char *cmd_extra,
                                                              const char *cmd_filter);
extern struct t_irc_redirect *irc_redirect_new (struct t_irc_server *server,
                                                const char *pattern,
                                                const char *signal,
                                                int count,
                                                const char *string,
                                                int timeout,
                                                const char *cmd_filter);
extern struct t_irc_redirect *irc_redirect_search_available (struct t_irc_server *server);
extern void irc_redirect_init_command (struct t_irc_redirect *redirect,
                                       const char *command);
extern void irc_redirect_stop (struct t_irc_redirect *redirect,
                               const char *error);
extern int irc_redirect_message (struct t_irc_server *server,
                                 const char *message, const char *command,
                                 const char *arguments);
extern void irc_redirect_free (struct t_irc_redirect *redirect);
extern void irc_redirect_free_all (struct t_irc_server *server);
extern struct t_hdata *irc_redirect_hdata_redirect_pattern_cb (const void *pointer,
                                                               void *data,
                                                               const char *hdata_name);
extern struct t_hdata *irc_redirect_hdata_redirect_cb (const void *pointer,
                                                       void *data,
                                                       const char *hdata_name);
extern int irc_redirect_pattern_add_to_infolist (struct t_infolist *infolist,
                                                 struct t_irc_redirect_pattern *redirect_pattern);
extern int irc_redirect_add_to_infolist (struct t_infolist *infolist,
                                         struct t_irc_redirect *redirect);
extern void irc_redirect_pattern_print_log (void);
extern void irc_redirect_print_log (struct t_irc_server *server);
extern int irc_redirect_pattern_hsignal_cb (const void *pointer, void *data,
                                            const char *signal,
                                            struct t_hashtable *hashtable);
extern int irc_redirect_command_hsignal_cb (const void *pointer, void *data,
                                            const char *signal,
                                            struct t_hashtable *hashtable);
extern void irc_redirect_init (void);
extern void irc_redirect_end (void);

#endif /* WEECHAT_PLUGIN_IRC_REDIRECT_H */
