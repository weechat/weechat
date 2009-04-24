/*
 * Copyright (c) 2009 by FlashCode <flashcode@flashtux.org>
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


#ifndef __WEECHAT_JABBER_H
#define __WEECHAT_JABBER_H 1

#define weechat_plugin weechat_jabber_plugin
#define JABBER_PLUGIN_NAME "jabber"

#define JABBER_GET_SERVER(__buffer)                                     \
    struct t_weechat_plugin *buffer_plugin = NULL;                      \
    struct t_jabber_server *ptr_server = NULL;                          \
    buffer_plugin = weechat_buffer_get_pointer (__buffer, "plugin");    \
    if (buffer_plugin == weechat_jabber_plugin)                         \
        jabber_buffer_get_server_muc (__buffer, &ptr_server, NULL);

#define JABBER_GET_SERVER_MUC(__buffer)                                 \
    struct t_weechat_plugin *buffer_plugin = NULL;                      \
    struct t_jabber_server *ptr_server = NULL;                          \
    struct t_jabber_muc *ptr_muc = NULL;                                \
    buffer_plugin = weechat_buffer_get_pointer (__buffer, "plugin");    \
    if (buffer_plugin == weechat_jabber_plugin)                         \
    {                                                                   \
        jabber_buffer_get_server_muc (__buffer, &ptr_server, &ptr_muc); \
    }

#define JABBER_COLOR_CHAT weechat_color("chat")
#define JABBER_COLOR_CHAT_CHANNEL weechat_color("chat_channel")
#define JABBER_COLOR_CHAT_DELIMITERS weechat_color("chat_delimiters")
#define JABBER_COLOR_CHAT_HOST weechat_color("chat_host")
#define JABBER_COLOR_CHAT_NICK weechat_color("chat_nick")
#define JABBER_COLOR_CHAT_NICK_SELF weechat_color("chat_nick_self")
#define JABBER_COLOR_CHAT_NICK_OTHER weechat_color("chat_nick_other")
#define JABBER_COLOR_CHAT_SERVER weechat_color("chat_server")
#define JABBER_COLOR_CHAT_VALUE weechat_color("chat_value")
#define JABBER_COLOR_NICKLIST_PREFIX1 weechat_color("nicklist_prefix1")
#define JABBER_COLOR_NICKLIST_PREFIX2 weechat_color("nicklist_prefix2")
#define JABBER_COLOR_NICKLIST_PREFIX3 weechat_color("nicklist_prefix3")
#define JABBER_COLOR_NICKLIST_PREFIX4 weechat_color("nicklist_prefix4")
#define JABBER_COLOR_NICKLIST_PREFIX5 weechat_color("nicklist_prefix5")
#define JABBER_COLOR_BAR_FG weechat_color("bar_fg")
#define JABBER_COLOR_BAR_BG weechat_color("bar_bg")
#define JABBER_COLOR_BAR_DELIM weechat_color("bar_delim")
#define JABBER_COLOR_STATUS_NUMBER weechat_color(weechat_config_string(weechat_config_get("weechat.color.status_number")))
#define JABBER_COLOR_STATUS_NAME weechat_color(weechat_config_string(weechat_config_get("weechat.color.status_name")))
#define JABBER_COLOR_MESSAGE_JOIN weechat_color(weechat_config_string(jabber_config_color_message_join))
#define JABBER_COLOR_MESSAGE_QUIT weechat_color(weechat_config_string(jabber_config_color_message_quit))
#define JABBER_COLOR_INPUT_NICK weechat_color(weechat_config_string(jabber_config_color_input_nick))
#define JABBER_COLOR_NICK_IN_SERVER_MESSAGE(nick)                          \
    ((nick && weechat_config_boolean(jabber_config_look_color_nicks_in_server_messages)) ? \
     nick->color : JABBER_COLOR_CHAT_NICK)

extern struct t_weechat_plugin *weechat_jabber_plugin;

#endif /* jabber.h */
