/*
 * Copyright (C) 2014 Sébastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __WEECHAT_TRIGGER_CONFIG_H
#define __WEECHAT_TRIGGER_CONFIG_H 1

#define TRIGGER_CONFIG_NAME "trigger"
#define TRIGGER_CONFIG_SECTION_TRIGGER "trigger"

extern struct t_config_file *trigger_config_file;
extern struct t_config_section *trigger_config_section_trigger;

extern struct t_config_option *trigger_config_color_regex;
extern struct t_config_option *trigger_config_color_replace;
extern struct t_config_option *trigger_config_color_trigger;
extern struct t_config_option *trigger_config_color_trigger_disabled;

extern struct t_config_option *trigger_config_create_option (const char *trigger_name,
                                                             int index_option,
                                                             const char *value);
extern int trigger_config_init ();
extern int trigger_config_read ();
extern int trigger_config_write ();
extern void trigger_config_free ();

#endif /* __WEECHAT_TRIGGER_CONFIG_H */
