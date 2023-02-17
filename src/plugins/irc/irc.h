/*
 * Copyright (C) 2003-2023 Sébastien Helleu <flashcode@flashtux.org>
 * Copyright (C) 2006 Emmanuel Bouthenot <kolter@openics.org>
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

#ifndef WEECHAT_PLUGIN_IRC_H
#define WEECHAT_PLUGIN_IRC_H

#define weechat_plugin weechat_irc_plugin
#define IRC_PLUGIN_NAME "irc"
#define IRC_PLUGIN_PRIORITY 6000

extern struct t_weechat_plugin *weechat_irc_plugin;

extern int irc_signal_quit_received;
extern int irc_signal_upgrade_received;

#endif /* WEECHAT_PLUGIN_IRC_H */
