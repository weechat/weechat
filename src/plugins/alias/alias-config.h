/*
 * Copyright (C) 2003-2023 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_PLUGIN_ALIAS_CONFIG_H
#define WEECHAT_PLUGIN_ALIAS_CONFIG_H

#define ALIAS_CONFIG_NAME "alias"
#define ALIAS_CONFIG_PRIO_NAME (TO_STR(ALIAS_PLUGIN_PRIORITY) "|" ALIAS_CONFIG_NAME)

#define ALIAS_CONFIG_VERSION 2

extern struct t_config_file *alias_config_file;
extern struct t_config_section *alias_config_section_cmd;
extern struct t_config_section *alias_config_section_completion;

extern char *alias_default[][3];

extern void alias_config_cmd_new_option (const char *name,
                                         const char *command);
extern void alias_config_completion_new_option (const char *name,
                                                const char *completion);
extern int alias_config_init ();
extern int alias_config_read ();
extern int alias_config_write ();

#endif /* WEECHAT_PLUGIN_ALIAS_CONFIG_H */
