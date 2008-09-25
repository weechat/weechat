/*
 * Copyright (c) 2003-2008 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef __WEECHAT_IRC_H
#define __WEECHAT_IRC_H 1

#ifdef HAVE_GNUTLS
#include <gnutls/gnutls.h>
#endif

#define weechat_plugin weechat_irc_plugin
#define IRC_PLUGIN_NAME "irc"

#define IRC_GET_SERVER(__buffer)                                        \
    struct t_weechat_plugin *buffer_plugin = NULL;                      \
    struct t_irc_server *ptr_server = NULL;                             \
    buffer_plugin = weechat_buffer_get_pointer (__buffer, "plugin");    \
    if (buffer_plugin == weechat_irc_plugin)                            \
        irc_buffer_get_server_channel (__buffer, &ptr_server, NULL);

#define IRC_GET_SERVER_CHANNEL(__buffer)                                \
    struct t_weechat_plugin *buffer_plugin = NULL;                      \
    struct t_irc_server *ptr_server = NULL;                             \
    struct t_irc_channel *ptr_channel = NULL;                           \
    buffer_plugin = weechat_buffer_get_pointer (__buffer, "plugin");    \
    if (buffer_plugin == weechat_irc_plugin)                            \
    {                                                                   \
        irc_buffer_get_server_channel (__buffer, &ptr_server,           \
                                       &ptr_channel);                   \
    }

#define IRC_COLOR_CHAT weechat_color("chat")
#define IRC_COLOR_CHAT_CHANNEL weechat_color("chat_channel")
#define IRC_COLOR_CHAT_DELIMITERS weechat_color("chat_delimiters")
#define IRC_COLOR_CHAT_HOST weechat_color("chat_host")
#define IRC_COLOR_CHAT_NICK weechat_color("chat_nick")
#define IRC_COLOR_CHAT_NICK_SELF weechat_color("chat_nick_self")
#define IRC_COLOR_CHAT_NICK_OTHER weechat_color("chat_nick_other")
#define IRC_COLOR_CHAT_SERVER weechat_color("chat_server")
#define IRC_COLOR_NICKLIST_PREFIX1 weechat_color("nicklist_prefix1")
#define IRC_COLOR_NICKLIST_PREFIX2 weechat_color("nicklist_prefix2")
#define IRC_COLOR_NICKLIST_PREFIX3 weechat_color("nicklist_prefix3")
#define IRC_COLOR_NICKLIST_PREFIX4 weechat_color("nicklist_prefix4")
#define IRC_COLOR_NICKLIST_PREFIX5 weechat_color("nicklist_prefix5")
#define IRC_COLOR_BAR_FG weechat_color("bar_fg")
#define IRC_COLOR_BAR_BG weechat_color("bar_bg")
#define IRC_COLOR_BAR_DELIM weechat_color("bar_delim")
#define IRC_COLOR_STATUS_NUMBER weechat_color(weechat_config_string(weechat_config_get("weechat.color.status_number")))
#define IRC_COLOR_STATUS_NAME weechat_color(weechat_config_string(weechat_config_get("weechat.color.status_name")))
#define IRC_COLOR_INPUT_NICK weechat_color(weechat_config_string(weechat_config_get("weechat.color.input_nick")))

extern struct t_weechat_plugin *weechat_irc_plugin;
extern struct t_hook *irc_hook_timer_check_away;

extern int irc_debug;

#endif /* irc.h */
