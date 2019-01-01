/*
 * Copyright (C) 2006 Emmanuel Bouthenot <kolter@openics.org>
 * Copyright (C) 2006-2019 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_PLUGIN_ASPELL_CONFIG_H
#define WEECHAT_PLUGIN_ASPELL_CONFIG_H

#define ASPELL_CONFIG_NAME "aspell"


extern struct t_config_option *weechat_aspell_config_color_misspelled;
extern struct t_config_option *weechat_aspell_config_color_suggestion;
extern struct t_config_option *weechat_aspell_config_color_suggestion_delimiter_dict;
extern struct t_config_option *weechat_aspell_config_color_suggestion_delimiter_word;

extern struct t_config_option *weechat_aspell_config_check_commands;
extern struct t_config_option *weechat_aspell_config_check_default_dict;
extern struct t_config_option *weechat_aspell_config_check_during_search;
extern struct t_config_option *weechat_aspell_config_check_enabled;
extern struct t_config_option *weechat_aspell_config_check_real_time;
extern struct t_config_option *weechat_aspell_config_check_suggestions;
extern struct t_config_option *weechat_aspell_config_check_word_min_length;

extern struct t_config_option *weechat_aspell_config_look_suggestion_delimiter_dict;
extern struct t_config_option *weechat_aspell_config_look_suggestion_delimiter_word;

extern char **weechat_aspell_commands_to_check;
extern int weechat_aspell_count_commands_to_check;
extern int *weechat_aspell_length_commands_to_check;

extern struct t_config_option *weechat_aspell_config_get_dict (const char *name);
extern int weechat_aspell_config_set_dict (const char *name, const char *value);
extern int weechat_aspell_config_init ();
extern int weechat_aspell_config_read ();
extern int weechat_aspell_config_write ();
extern void weechat_aspell_config_free ();

#endif /* WEECHAT_PLUGIN_ASPELL_CONFIG_H */
