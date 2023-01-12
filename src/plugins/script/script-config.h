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

#ifndef WEECHAT_PLUGIN_SCRIPT_CONFIG_H
#define WEECHAT_PLUGIN_SCRIPT_CONFIG_H

#define SCRIPT_CONFIG_NAME "script"
#define SCRIPT_CONFIG_PRIO_NAME (TO_STR(SCRIPT_PLUGIN_PRIORITY) "|" SCRIPT_CONFIG_NAME)

struct t_script_repo;

extern struct t_config_option *script_config_look_columns;
extern struct t_config_option *script_config_look_diff_color;
extern struct t_config_option *script_config_look_diff_command;
extern struct t_config_option *script_config_look_display_source;
extern struct t_config_option *script_config_look_quiet_actions;
extern struct t_config_option *script_config_look_sort;
extern struct t_config_option *script_config_look_translate_description;
extern struct t_config_option *script_config_look_use_keys;

extern struct t_config_option *script_config_color_status_autoloaded;
extern struct t_config_option *script_config_color_status_held;
extern struct t_config_option *script_config_color_status_installed;
extern struct t_config_option *script_config_color_status_obsolete;
extern struct t_config_option *script_config_color_status_popular;
extern struct t_config_option *script_config_color_status_running;
extern struct t_config_option *script_config_color_status_unknown;
extern struct t_config_option *script_config_color_text;
extern struct t_config_option *script_config_color_text_bg;
extern struct t_config_option *script_config_color_text_bg_selected;
extern struct t_config_option *script_config_color_text_date;
extern struct t_config_option *script_config_color_text_date_selected;
extern struct t_config_option *script_config_color_text_delimiters;
extern struct t_config_option *script_config_color_text_description;
extern struct t_config_option *script_config_color_text_description_selected;
extern struct t_config_option *script_config_color_text_extension;
extern struct t_config_option *script_config_color_text_extension_selected;
extern struct t_config_option *script_config_color_text_name;
extern struct t_config_option *script_config_color_text_name_selected;
extern struct t_config_option *script_config_color_text_selected;
extern struct t_config_option *script_config_color_text_tags;
extern struct t_config_option *script_config_color_text_tags_selected;
extern struct t_config_option *script_config_color_text_version;
extern struct t_config_option *script_config_color_text_version_loaded;
extern struct t_config_option *script_config_color_text_version_loaded_selected;
extern struct t_config_option *script_config_color_text_version_selected;

extern struct t_config_option *script_config_scripts_autoload;
extern struct t_config_option *script_config_scripts_cache_expire;
extern struct t_config_option *script_config_scripts_download_enabled;
extern struct t_config_option *script_config_scripts_download_timeout;
extern struct t_config_option *script_config_scripts_hold;
extern struct t_config_option *script_config_scripts_path;
extern struct t_config_option *script_config_scripts_url;

extern const char *script_config_get_diff_command ();
extern char *script_config_get_xml_filename ();
extern char *script_config_get_script_download_filename (struct t_script_repo *script,
                                                         const char *suffix);
extern void script_config_hold (const char *name_with_extension);
extern void script_config_unhold (const char *name_with_extension);
extern int script_config_init ();
extern int script_config_read ();
extern int script_config_write ();
extern void script_config_free ();

#endif /* WEECHAT_PLUGIN_SCRIPT_CONFIG_H */
