/*
 * Copyright (C) 2006-2017 Adam Saponara <as@php.net>
 * Copyright (C) 2017-2023 Sébastien Helleu <flashcode@flashtux.org>
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
 *
 * OpenSSL licensing:
 *
 *   Additional permission under GNU GPL version 3 section 7:
 *
 *   If you modify the Program, or any covered work, by linking or
 *   combining it with the OpenSSL project's OpenSSL library (or a
 *   modified version of that library), containing parts covered by the
 *   terms of the OpenSSL or SSLeay licenses, the licensors of the Program
 *   grant you additional permission to convey the resulting work.
 *   Corresponding Source for a non-source form of such a combination
 *   shall include the source code for the parts of OpenSSL used as well
 *   as that of the covered work.
 */

#ifndef WEECHAT_PLUGIN_PHP_API_H
#define WEECHAT_PLUGIN_PHP_API_H

extern struct zval* weechat_php_api_funcs[];
extern struct t_php_const weechat_php_api_consts[];

extern int weechat_php_api_buffer_input_data_cb (const void *pointer,
                                                 void *data,
                                                 struct t_gui_buffer *buffer,
                                                 const char *input_data);
extern int weechat_php_api_buffer_close_cb (const void *pointer,
                                            void *data,
                                            struct t_gui_buffer *buffer);

PHP_FUNCTION(weechat_register);
PHP_FUNCTION(weechat_plugin_get_name);
PHP_FUNCTION(weechat_charset_set);
PHP_FUNCTION(weechat_iconv_to_internal);
PHP_FUNCTION(weechat_iconv_from_internal);
PHP_FUNCTION(weechat_gettext);
PHP_FUNCTION(weechat_ngettext);
PHP_FUNCTION(weechat_strlen_screen);
PHP_FUNCTION(weechat_string_match);
PHP_FUNCTION(weechat_string_match_list);
PHP_FUNCTION(weechat_string_has_highlight);
PHP_FUNCTION(weechat_string_has_highlight_regex);
PHP_FUNCTION(weechat_string_mask_to_regex);
PHP_FUNCTION(weechat_string_format_size);
PHP_FUNCTION(weechat_string_parse_size);
PHP_FUNCTION(weechat_string_color_code_size);
PHP_FUNCTION(weechat_string_remove_color);
PHP_FUNCTION(weechat_string_is_command_char);
PHP_FUNCTION(weechat_string_input_for_buffer);
PHP_FUNCTION(weechat_string_eval_expression);
PHP_FUNCTION(weechat_string_eval_path_home);
PHP_FUNCTION(weechat_mkdir_home);
PHP_FUNCTION(weechat_mkdir);
PHP_FUNCTION(weechat_mkdir_parents);
PHP_FUNCTION(weechat_list_new);
PHP_FUNCTION(weechat_list_add);
PHP_FUNCTION(weechat_list_search);
PHP_FUNCTION(weechat_list_search_pos);
PHP_FUNCTION(weechat_list_casesearch);
PHP_FUNCTION(weechat_list_casesearch_pos);
PHP_FUNCTION(weechat_list_get);
PHP_FUNCTION(weechat_list_set);
PHP_FUNCTION(weechat_list_next);
PHP_FUNCTION(weechat_list_prev);
PHP_FUNCTION(weechat_list_string);
PHP_FUNCTION(weechat_list_size);
PHP_FUNCTION(weechat_list_remove);
PHP_FUNCTION(weechat_list_remove_all);
PHP_FUNCTION(weechat_list_free);
PHP_FUNCTION(weechat_config_new);
PHP_FUNCTION(weechat_config_set_version);
PHP_FUNCTION(weechat_config_new_section);
PHP_FUNCTION(weechat_config_search_section);
PHP_FUNCTION(weechat_config_new_option);
PHP_FUNCTION(weechat_config_search_option);
PHP_FUNCTION(weechat_config_string_to_boolean);
PHP_FUNCTION(weechat_config_option_reset);
PHP_FUNCTION(weechat_config_option_set);
PHP_FUNCTION(weechat_config_option_set_null);
PHP_FUNCTION(weechat_config_option_unset);
PHP_FUNCTION(weechat_config_option_rename);
PHP_FUNCTION(weechat_config_option_is_null);
PHP_FUNCTION(weechat_config_option_default_is_null);
PHP_FUNCTION(weechat_config_boolean);
PHP_FUNCTION(weechat_config_boolean_default);
PHP_FUNCTION(weechat_config_integer);
PHP_FUNCTION(weechat_config_integer_default);
PHP_FUNCTION(weechat_config_string);
PHP_FUNCTION(weechat_config_string_default);
PHP_FUNCTION(weechat_config_color);
PHP_FUNCTION(weechat_config_color_default);
PHP_FUNCTION(weechat_config_write_option);
PHP_FUNCTION(weechat_config_write_line);
PHP_FUNCTION(weechat_config_write);
PHP_FUNCTION(weechat_config_read);
PHP_FUNCTION(weechat_config_reload);
PHP_FUNCTION(weechat_config_option_free);
PHP_FUNCTION(weechat_config_section_free_options);
PHP_FUNCTION(weechat_config_section_free);
PHP_FUNCTION(weechat_config_free);
PHP_FUNCTION(weechat_config_get);
PHP_FUNCTION(weechat_config_get_plugin);
PHP_FUNCTION(weechat_config_is_set_plugin);
PHP_FUNCTION(weechat_config_set_plugin);
PHP_FUNCTION(weechat_config_set_desc_plugin);
PHP_FUNCTION(weechat_config_unset_plugin);
PHP_FUNCTION(weechat_key_bind);
PHP_FUNCTION(weechat_key_unbind);
PHP_FUNCTION(weechat_prefix);
PHP_FUNCTION(weechat_color);
PHP_FUNCTION(weechat_print);
PHP_FUNCTION(weechat_print_date_tags);
PHP_FUNCTION(weechat_print_y);
PHP_FUNCTION(weechat_print_y_date_tags);
PHP_FUNCTION(weechat_log_print);
PHP_FUNCTION(weechat_hook_command);
PHP_FUNCTION(weechat_hook_completion);
PHP_FUNCTION(weechat_hook_completion_get_string);
PHP_FUNCTION(weechat_hook_completion_list_add);
PHP_FUNCTION(weechat_hook_command_run);
PHP_FUNCTION(weechat_hook_timer);
PHP_FUNCTION(weechat_hook_fd);
PHP_FUNCTION(weechat_hook_process);
PHP_FUNCTION(weechat_hook_process_hashtable);
PHP_FUNCTION(weechat_hook_connect);
PHP_FUNCTION(weechat_hook_line);
PHP_FUNCTION(weechat_hook_print);
PHP_FUNCTION(weechat_hook_signal);
PHP_FUNCTION(weechat_hook_signal_send);
PHP_FUNCTION(weechat_hook_hsignal);
PHP_FUNCTION(weechat_hook_hsignal_send);
PHP_FUNCTION(weechat_hook_config);
PHP_FUNCTION(weechat_hook_modifier);
PHP_FUNCTION(weechat_hook_modifier_exec);
PHP_FUNCTION(weechat_hook_info);
PHP_FUNCTION(weechat_hook_info_hashtable);
PHP_FUNCTION(weechat_hook_infolist);
PHP_FUNCTION(weechat_hook_focus);
PHP_FUNCTION(weechat_hook_set);
PHP_FUNCTION(weechat_unhook);
PHP_FUNCTION(weechat_unhook_all);
PHP_FUNCTION(weechat_buffer_new);
PHP_FUNCTION(weechat_buffer_new_props);
PHP_FUNCTION(weechat_buffer_search);
PHP_FUNCTION(weechat_buffer_search_main);
PHP_FUNCTION(weechat_current_buffer);
PHP_FUNCTION(weechat_buffer_clear);
PHP_FUNCTION(weechat_buffer_close);
PHP_FUNCTION(weechat_buffer_merge);
PHP_FUNCTION(weechat_buffer_unmerge);
PHP_FUNCTION(weechat_buffer_get_integer);
PHP_FUNCTION(weechat_buffer_get_string);
PHP_FUNCTION(weechat_buffer_get_pointer);
PHP_FUNCTION(weechat_buffer_set);
PHP_FUNCTION(weechat_buffer_string_replace_local_var);
PHP_FUNCTION(weechat_buffer_match_list);
PHP_FUNCTION(weechat_current_window);
PHP_FUNCTION(weechat_window_search_with_buffer);
PHP_FUNCTION(weechat_window_get_integer);
PHP_FUNCTION(weechat_window_get_string);
PHP_FUNCTION(weechat_window_get_pointer);
PHP_FUNCTION(weechat_window_set_title);
PHP_FUNCTION(weechat_nicklist_add_group);
PHP_FUNCTION(weechat_nicklist_search_group);
PHP_FUNCTION(weechat_nicklist_add_nick);
PHP_FUNCTION(weechat_nicklist_search_nick);
PHP_FUNCTION(weechat_nicklist_remove_group);
PHP_FUNCTION(weechat_nicklist_remove_nick);
PHP_FUNCTION(weechat_nicklist_remove_all);
PHP_FUNCTION(weechat_nicklist_group_get_integer);
PHP_FUNCTION(weechat_nicklist_group_get_string);
PHP_FUNCTION(weechat_nicklist_group_get_pointer);
PHP_FUNCTION(weechat_nicklist_group_set);
PHP_FUNCTION(weechat_nicklist_nick_get_integer);
PHP_FUNCTION(weechat_nicklist_nick_get_string);
PHP_FUNCTION(weechat_nicklist_nick_get_pointer);
PHP_FUNCTION(weechat_nicklist_nick_set);
PHP_FUNCTION(weechat_bar_item_search);
PHP_FUNCTION(weechat_bar_item_new);
PHP_FUNCTION(weechat_bar_item_update);
PHP_FUNCTION(weechat_bar_item_remove);
PHP_FUNCTION(weechat_bar_search);
PHP_FUNCTION(weechat_bar_new);
PHP_FUNCTION(weechat_bar_set);
PHP_FUNCTION(weechat_bar_update);
PHP_FUNCTION(weechat_bar_remove);
PHP_FUNCTION(weechat_command);
PHP_FUNCTION(weechat_command_options);
PHP_FUNCTION(weechat_completion_new);
PHP_FUNCTION(weechat_completion_search);
PHP_FUNCTION(weechat_completion_get_string);
PHP_FUNCTION(weechat_completion_list_add);
PHP_FUNCTION(weechat_completion_free);
PHP_FUNCTION(weechat_info_get);
PHP_FUNCTION(weechat_info_get_hashtable);
PHP_FUNCTION(weechat_infolist_new);
PHP_FUNCTION(weechat_infolist_new_item);
PHP_FUNCTION(weechat_infolist_new_var_integer);
PHP_FUNCTION(weechat_infolist_new_var_string);
PHP_FUNCTION(weechat_infolist_new_var_pointer);
PHP_FUNCTION(weechat_infolist_new_var_time);
PHP_FUNCTION(weechat_infolist_search_var);
PHP_FUNCTION(weechat_infolist_get);
PHP_FUNCTION(weechat_infolist_next);
PHP_FUNCTION(weechat_infolist_prev);
PHP_FUNCTION(weechat_infolist_reset_item_cursor);
PHP_FUNCTION(weechat_infolist_fields);
PHP_FUNCTION(weechat_infolist_integer);
PHP_FUNCTION(weechat_infolist_string);
PHP_FUNCTION(weechat_infolist_pointer);
PHP_FUNCTION(weechat_infolist_time);
PHP_FUNCTION(weechat_infolist_free);
PHP_FUNCTION(weechat_hdata_get);
PHP_FUNCTION(weechat_hdata_get_var_offset);
PHP_FUNCTION(weechat_hdata_get_var_type_string);
PHP_FUNCTION(weechat_hdata_get_var_array_size);
PHP_FUNCTION(weechat_hdata_get_var_array_size_string);
PHP_FUNCTION(weechat_hdata_get_var_hdata);
PHP_FUNCTION(weechat_hdata_get_list);
PHP_FUNCTION(weechat_hdata_check_pointer);
PHP_FUNCTION(weechat_hdata_move);
PHP_FUNCTION(weechat_hdata_search);
PHP_FUNCTION(weechat_hdata_char);
PHP_FUNCTION(weechat_hdata_integer);
PHP_FUNCTION(weechat_hdata_long);
PHP_FUNCTION(weechat_hdata_string);
PHP_FUNCTION(weechat_hdata_pointer);
PHP_FUNCTION(weechat_hdata_time);
PHP_FUNCTION(weechat_hdata_hashtable);
PHP_FUNCTION(weechat_hdata_compare);
PHP_FUNCTION(weechat_hdata_update);
PHP_FUNCTION(weechat_hdata_get_string);
PHP_FUNCTION(weechat_upgrade_new);
PHP_FUNCTION(weechat_upgrade_write_object);
PHP_FUNCTION(weechat_upgrade_read);
PHP_FUNCTION(weechat_upgrade_close);
PHP_FUNCTION(forget_class);
PHP_FUNCTION(forget_function);

#endif /* WEECHAT_PLUGIN_PHP_API_H */
