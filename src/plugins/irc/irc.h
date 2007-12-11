/*
 * Copyright (c) 2003-2007 by FlashCode <flashcode@flashtux.org>
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

#include "../weechat-plugin.h"

#define weechat_plugin weechat_irc_plugin

#define IRC_GET_SERVER(__buffer)                                        \
    struct t_weechat_plugin *buffer_plugin = NULL;                      \
    struct t_irc_server *ptr_server = NULL;                             \
    buffer_plugin = weechat_buffer_get (__buffer, "plugin");            \
    if (buffer_plugin == weechat_irc_plugin)                            \
        ptr_server = irc_server_search (                                \
            weechat_buffer_get (__buffer, "category"));

#define IRC_GET_SERVER_CHANNEL(__buffer)                                \
    struct t_weechat_plugin *buffer_plugin = NULL;                      \
    struct t_irc_server *ptr_server = NULL;                             \
    struct t_irc_channel *ptr_channel = NULL;                           \
    buffer_plugin = weechat_buffer_get (__buffer, "plugin");            \
    if (buffer_plugin == weechat_irc_plugin)                            \
    {                                                                   \
        ptr_server = irc_server_search (                                \
            weechat_buffer_get (__buffer, "category"));                 \
        ptr_channel = irc_channel_search (                              \
            ptr_server, weechat_buffer_get (__buffer, "name"));         \
    }

#define IRC_COLOR_CHAT weechat_color("color_chat")
#define IRC_COLOR_CHAT_CHANNEL weechat_color("color_chat_channel")
#define IRC_COLOR_CHAT_DELIMITERS weechat_color("color_chat_delimiters")
#define IRC_COLOR_CHAT_HIGHLIGHT weechat_color("color_chat_highlight")
#define IRC_COLOR_CHAT_HOST weechat_color("color_chat_host")
#define IRC_COLOR_CHAT_NICK weechat_color("color_chat_nick")
#define IRC_COLOR_CHAT_NICK_OTHER weechat_color("color_chat_nick_other")
#define IRC_COLOR_CHAT_SERVER weechat_color("color_chat_server")
#define IRC_COLOR_INFOBAR_HIGHLIGHT weechat_color("color_infobar_highlight")
#define IRC_COLOR_NICKLIST_PREFIX1 weechat_color("color_nicklist_prefix1")
#define IRC_COLOR_NICKLIST_PREFIX2 weechat_color("color_nicklist_prefix2")
#define IRC_COLOR_NICKLIST_PREFIX3 weechat_color("color_nicklist_prefix3")
#define IRC_COLOR_NICKLIST_PREFIX4 weechat_color("color_nicklist_prefix4")
#define IRC_COLOR_NICKLIST_PREFIX5 weechat_color("color_nicklist_prefix5")

extern struct t_weechat_plugin *weechat_irc_plugin;
extern struct t_hook *irc_hook_timer_check_away;

extern gnutls_certificate_credentials gnutls_xcred;

#endif /* irc.h */
