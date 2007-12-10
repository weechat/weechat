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

#define IRC_COLOR_CHAT weechat_color("color_chat")
#define IRC_COLOR_CHAT_CHANNEL weechat_color("color_chat_channel")
#define IRC_COLOR_CHAT_DELIMITERS weechat_color("color_chat_delimiters")
#define IRC_COLOR_CHAT_NICK weechat_color("color_chat_nick")
#define IRC_COLOR_CHAT_SERVER weechat_color("color_chat_server")

extern struct t_weechat_plugin *weechat_irc_plugin;
extern struct t_hook *irc_timer_check_away;

extern gnutls_certificate_credentials gnutls_xcred;

#endif /* irc.h */
