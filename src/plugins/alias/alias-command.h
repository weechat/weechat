/*
 * Copyright (C) 2013-2025 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_PLUGIN_ALIAS_COMMAND_H
#define WEECHAT_PLUGIN_ALIAS_COMMAND_H

#define ALIAS_COMMAND_KEEP_SPACES weechat_hook_set (ptr_hook, "keep_spaces_right", "1")

extern void alias_command_init (void);

#endif /* WEECHAT_PLUGIN_ALIAS_COMMAND_H */
