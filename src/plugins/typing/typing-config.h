/*
 * Copyright (C) 2021-2023 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_PLUGIN_TYPING_CONFIG_H
#define WEECHAT_PLUGIN_TYPING_CONFIG_H

#define TYPING_CONFIG_NAME "typing"
#define TYPING_CONFIG_PRIO_NAME (TO_STR(TYPING_PLUGIN_PRIORITY) "|" TYPING_CONFIG_NAME)

extern struct t_config_option *typing_config_look_delay_purge_paused;
extern struct t_config_option *typing_config_look_delay_purge_typing;
extern struct t_config_option *typing_config_look_delay_set_paused;
extern struct t_config_option *typing_config_look_enabled_nicks;
extern struct t_config_option *typing_config_look_enabled_self;
extern struct t_config_option *typing_config_look_input_min_chars;
extern struct t_config_option *typing_config_look_item_max_length;

extern int typing_config_init ();
extern int typing_config_read ();
extern int typing_config_write ();
extern void typing_config_free ();

#endif /* WEECHAT_PLUGIN_TYPING_CONFIG_H */
