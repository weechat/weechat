/*
 * Copyright (C) 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_PLUGIN_CHARSET_H
#define WEECHAT_PLUGIN_CHARSET_H

#define weechat_plugin weechat_charset_plugin
#define CHARSET_PLUGIN_NAME "charset"
#define CHARSET_PLUGIN_PRIORITY 16000

#define CHARSET_CONFIG_NAME "charset"
#define CHARSET_CONFIG_PRIO_NAME (TO_STR(CHARSET_PLUGIN_PRIORITY) "|" CHARSET_CONFIG_NAME)

extern struct t_weechat_plugin *weechat_charset_plugin;

#endif /* WEECHAT_PLUGIN_CHARSET_H */
