/*
 * Copyright (C) 2010-2013 Sebastien Helleu <flashcode@flashtux.org>
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

#ifndef __WEECHAT_RMODIFIER_CONFIG_H
#define __WEECHAT_RMODIFIER_CONFIG_H 1

#define RMODIFIER_CONFIG_NAME "rmodifier"

extern struct t_config_file *rmodifier_config_file;
extern struct t_config_section *rmodifier_config_section_modifier;

extern struct t_config_option *rmodifier_config_look_hide_char;

extern char *rmodifier_config_default_list[][4];

extern void rmodifier_config_modifier_new_option (const char *name,
                                                  const char *modifiers,
                                                  const char *regex,
                                                  const char *groups);
extern int rmodifier_config_init ();
extern int rmodifier_config_read ();
extern int rmodifier_config_write ();

#endif /* __WEECHAT_RMODIFIER_CONFIG_H */
