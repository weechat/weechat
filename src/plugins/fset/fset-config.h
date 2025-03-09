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

#ifndef WEECHAT_PLUGIN_FSET_CONFIG_H
#define WEECHAT_PLUGIN_FSET_CONFIG_H

#define FSET_CONFIG_NAME "fset"
#define FSET_CONFIG_PRIO_NAME (TO_STR(FSET_PLUGIN_PRIORITY) "|" FSET_CONFIG_NAME)

extern struct t_config_file *fset_config_file;

extern struct t_config_option *fset_config_look_auto_refresh;
extern struct t_config_option *fset_config_look_auto_unmark;
extern struct t_config_option *fset_config_look_condition_catch_set;
extern struct t_config_option *fset_config_look_export_help_default;
extern struct t_config_option *fset_config_look_format_number;
extern struct t_config_option *fset_config_look_marked_string;
extern struct t_config_option *fset_config_look_scroll_horizontal;
extern struct t_config_option *fset_config_look_show_plugins_desc;
extern struct t_config_option *fset_config_look_sort;
extern struct t_config_option *fset_config_look_unmarked_string;
extern struct t_config_option *fset_config_look_use_color_value;
extern struct t_config_option *fset_config_look_use_keys;
extern struct t_config_option *fset_config_look_use_mute;

extern struct t_config_option *fset_config_format_export_help;
extern struct t_config_option *fset_config_format_export_option;
extern struct t_config_option *fset_config_format_export_option_null;
extern struct t_config_option *fset_config_format_option[2];

extern struct t_config_option *fset_config_color_allowed_values[2];
extern struct t_config_option *fset_config_color_color_name[2];
extern struct t_config_option *fset_config_color_default_value[2];
extern struct t_config_option *fset_config_color_description[2];
extern struct t_config_option *fset_config_color_file[2];
extern struct t_config_option *fset_config_color_file_changed[2];
extern struct t_config_option *fset_config_color_help_default_value;
extern struct t_config_option *fset_config_color_help_description;
extern struct t_config_option *fset_config_color_help_name;
extern struct t_config_option *fset_config_color_help_quotes;
extern struct t_config_option *fset_config_color_help_values;
extern struct t_config_option *fset_config_color_index[2];
extern struct t_config_option *fset_config_color_line_marked_bg[2];
extern struct t_config_option *fset_config_color_line_selected_bg[2];
extern struct t_config_option *fset_config_color_marked[2];
extern struct t_config_option *fset_config_color_max[2];
extern struct t_config_option *fset_config_color_min[2];
extern struct t_config_option *fset_config_color_name[2];
extern struct t_config_option *fset_config_color_name_changed[2];
extern struct t_config_option *fset_config_color_option[2];
extern struct t_config_option *fset_config_color_option_changed[2];
extern struct t_config_option *fset_config_color_parent_name[2];
extern struct t_config_option *fset_config_color_parent_value[2];
extern struct t_config_option *fset_config_color_quotes[2];
extern struct t_config_option *fset_config_color_quotes_changed[2];
extern struct t_config_option *fset_config_color_section[2];
extern struct t_config_option *fset_config_color_section_changed[2];
extern struct t_config_option *fset_config_color_string_values[2];
extern struct t_config_option *fset_config_color_title_count_options;
extern struct t_config_option *fset_config_color_title_current_option;
extern struct t_config_option *fset_config_color_title_filter;
extern struct t_config_option *fset_config_color_title_marked_options;
extern struct t_config_option *fset_config_color_title_sort;
extern struct t_config_option *fset_config_color_type[2];
extern struct t_config_option *fset_config_color_unmarked[2];
extern struct t_config_option *fset_config_color_value[2];
extern struct t_config_option *fset_config_color_value_changed[2];
extern struct t_config_option *fset_config_color_value_undef[2];

extern char **fset_config_auto_refresh;
extern char **fset_config_sort_fields;
extern int fset_config_sort_fields_count;
extern int fset_config_format_option_num_lines[2];

extern int fset_config_init (void);
extern int fset_config_read (void);
extern int fset_config_write (void);
extern void fset_config_free (void);

#endif /* WEECHAT_PLUGIN_FSET_CONFIG_H */
