/*
 * Copyright (C) 2003-2017 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_FSET_CONFIG_H
#define WEECHAT_FSET_CONFIG_H 1

#define FSET_CONFIG_NAME "fset"

extern struct t_config_file *fset_config_file;

extern struct t_config_option *fset_config_look_enabled;
extern struct t_config_option *fset_config_look_help_bar;
extern struct t_config_option *fset_config_look_use_keys;
extern struct t_config_option *fset_config_look_use_mute;

extern struct t_config_option *fset_config_format_option;
extern struct t_config_option *fset_config_format_option_current;

extern struct t_config_option *fset_config_color_default_value[2];
extern struct t_config_option *fset_config_color_description[2];
extern struct t_config_option *fset_config_color_help_default_value;
extern struct t_config_option *fset_config_color_help_description;
extern struct t_config_option *fset_config_color_help_name;
extern struct t_config_option *fset_config_color_help_quotes;
extern struct t_config_option *fset_config_color_help_string_values;
extern struct t_config_option *fset_config_color_max[2];
extern struct t_config_option *fset_config_color_min[2];
extern struct t_config_option *fset_config_color_name[2];
extern struct t_config_option *fset_config_color_parent_name[2];
extern struct t_config_option *fset_config_color_quotes[2];
extern struct t_config_option *fset_config_color_string_values[2];
extern struct t_config_option *fset_config_color_type[2];
extern struct t_config_option *fset_config_color_value[2];
extern struct t_config_option *fset_config_color_value_diff[2];
extern struct t_config_option *fset_config_color_value_undef[2];

extern char *fset_config_eval_format_option_current;

extern int fset_config_init ();
extern int fset_config_read ();
extern int fset_config_write ();
extern void fset_config_free ();

#endif /* WEECHAT_FSET_CONFIG_H */
