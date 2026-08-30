/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_IRC_COMMAND_H
#define WEECHAT_PLUGIN_IRC_COMMAND_H

struct t_irc_server;
struct t_irc_channel;

#define IRC_COMMAND_CALLBACK(__command)                                 \
    int                                                                 \
    irc_command_##__command (const void *pointer, void *data,           \
                             struct t_gui_buffer *buffer,               \
                             int argc, char **argv, char **argv_eol)

#define IRC_COMMAND_CHECK_SERVER(__command,                             \
                                 __check_connection,                    \
                                 __check_socket)                        \
    if (!ptr_server)                                                    \
    {                                                                   \
        weechat_printf (NULL,                                           \
                        _("%s%s: command \"%s\" must be executed on "   \
                          "irc buffer (server, channel or private)"),   \
                        weechat_prefix ("error"), IRC_PLUGIN_NAME,      \
                        __command);                                     \
        return WEECHAT_RC_OK;                                           \
    }                                                                   \
    if ((__check_connection && !ptr_server->is_connected)               \
        || (__check_socket && !ptr_server->fake_server                  \
            && (ptr_server->sock < 0)))                                 \
    {                                                                   \
        weechat_printf (NULL,                                           \
                        _("%s%s: command \"%s\" must be executed on "   \
                          "connected irc server"),                      \
                        weechat_prefix ("error"), IRC_PLUGIN_NAME,      \
                        __command);                                     \
        return WEECHAT_RC_OK;                                           \
    }

#define IRC_COMMAND_KEEP_SPACES weechat_hook_set (ptr_hook, "keep_spaces_right", "1")

/*
 * List of supported capabilities
 * (enabled if supported by the server + completion in command /cap)
 */
#define IRC_COMMAND_CAP_SUPPORTED                                       \
    "account-notify|account-tag|away-notify|batch|cap-notify|chghost|"  \
    "draft/multiline|echo-message|extended-join|invite-notify|"         \
    "message-tags|multi-prefix|server-time|setname|userhost-in-names"

/* List of supported CTCPs (for completion in command /ctcp) */
#define IRC_COMMAND_CTCP_SUPPORTED_COMPLETION \
    "action|clientinfo|ping|source|time|version"

extern void irc_command_away_server (struct t_irc_server *server,
                                     const char *arguments,
                                     int reset_unread_marker);
extern void irc_command_join_server (struct t_irc_server *server,
                                     const char *arguments,
                                     int manual_join,
                                     int noswitch);
extern void irc_command_mode_server (struct t_irc_server *server,
                                     const char *command,
                                     struct t_irc_channel *channel,
                                     const char *arguments,
                                     int flags);
extern void irc_command_part_channel (struct t_irc_server *server,
                                      const char *channel_name,
                                      const char *part_message);
extern void irc_command_quit_server (struct t_irc_server *server,
                                     const char *arguments);
extern void irc_command_init (void);

#endif /* WEECHAT_PLUGIN_IRC_COMMAND_H */
