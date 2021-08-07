/*
 * weechat-php.c - PHP plugin for WeeChat
 *
 * Copyright (C) 2006-2017 Adam Saponara <as@php.net>
 * Copyright (C) 2017-2021 Sébastien Helleu <flashcode@flashtux.org>
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

#include <sapi/embed/php_embed.h>
#include <php.h>
#include <php_ini.h>
#include <ext/standard/info.h>

#include "../weechat-plugin.h"
#include "../plugin-script.h"
#include "weechat-php.h"
#include "weechat-php-api.h"
#if PHP_VERSION_ID >= 80000
#include "weechat-php_arginfo.h"
#else
#include "weechat-php_legacy_arginfo.h"
#endif

WEECHAT_PLUGIN_NAME(PHP_PLUGIN_NAME);
WEECHAT_PLUGIN_DESCRIPTION(N_("Support of PHP scripts"));
WEECHAT_PLUGIN_AUTHOR("Adam Saponara <as@php.net>");
WEECHAT_PLUGIN_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_LICENSE(WEECHAT_LICENSE);
WEECHAT_PLUGIN_PRIORITY(4000);

struct t_weechat_plugin *weechat_php_plugin = NULL;

struct t_plugin_script_data php_data;

struct t_config_file *php_config_file = NULL;
struct t_config_option *php_config_look_check_license = NULL;
struct t_config_option *php_config_look_eval_keep_context = NULL;

int php_quiet = 0;

struct t_plugin_script *php_script_eval = NULL;
int php_eval_mode = 0;
int php_eval_send_input = 0;
int php_eval_exec_commands = 0;
struct t_gui_buffer *php_eval_buffer = NULL;

struct t_plugin_script *php_scripts = NULL;
struct t_plugin_script *last_php_script = NULL;
struct t_plugin_script *php_current_script = NULL;
struct t_plugin_script *php_registered_script = NULL;
const char *php_current_script_filename = NULL;
struct t_hashtable *weechat_php_func_map = NULL;

/*
 * string used to execute action "install":
 * when signal "php_script_install" is received, name of string
 * is added to this string, to be installed later by a timer (when nothing is
 * running in script)
 */
char *php_action_install_list = NULL;

/*
 * string used to execute action "remove":
 * when signal "php_script_remove" is received, name of string
 * is added to this string, to be removed later by a timer (when nothing is
 * running in script)
 */
char *php_action_remove_list = NULL;

/*
 * string used to execute action "autoload":
 * when signal "php_script_autoload" is received, name of string
 * is added to this string, to autoload or disable autoload later by a timer
 * (when nothing is running in script)
 */
char *php_action_autoload_list = NULL;

const zend_function_entry weechat_functions[] = {
    PHP_FE(weechat_register, arginfo_weechat_register)
    PHP_FE(weechat_plugin_get_name, arginfo_weechat_plugin_get_name)
    PHP_FE(weechat_charset_set, arginfo_weechat_charset_set)
    PHP_FE(weechat_iconv_to_internal, arginfo_weechat_iconv_to_internal)
    PHP_FE(weechat_iconv_from_internal, arginfo_weechat_iconv_from_internal)
    PHP_FE(weechat_gettext, arginfo_weechat_gettext)
    PHP_FE(weechat_ngettext, arginfo_weechat_ngettext)
    PHP_FE(weechat_strlen_screen, arginfo_weechat_strlen_screen)
    PHP_FE(weechat_string_match, arginfo_weechat_string_match)
    PHP_FE(weechat_string_match_list, arginfo_weechat_string_match_list)
    PHP_FE(weechat_string_has_highlight, arginfo_weechat_string_has_highlight)
    PHP_FE(weechat_string_has_highlight_regex, arginfo_weechat_string_has_highlight_regex)
    PHP_FE(weechat_string_mask_to_regex, arginfo_weechat_string_mask_to_regex)
    PHP_FE(weechat_string_format_size, arginfo_weechat_string_format_size)
    PHP_FE(weechat_string_color_code_size, arginfo_weechat_string_color_code_size)
    PHP_FE(weechat_string_remove_color, arginfo_weechat_string_remove_color)
    PHP_FE(weechat_string_is_command_char, arginfo_weechat_string_is_command_char)
    PHP_FE(weechat_string_input_for_buffer, arginfo_weechat_string_input_for_buffer)
    PHP_FE(weechat_string_eval_expression, arginfo_weechat_string_eval_expression)
    PHP_FE(weechat_string_eval_path_home, arginfo_weechat_string_eval_path_home)
    PHP_FE(weechat_mkdir_home, arginfo_weechat_mkdir_home)
    PHP_FE(weechat_mkdir, arginfo_weechat_mkdir)
    PHP_FE(weechat_mkdir_parents, arginfo_weechat_mkdir_parents)
    PHP_FE(weechat_list_new, arginfo_weechat_list_new)
    PHP_FE(weechat_list_add, arginfo_weechat_list_add)
    PHP_FE(weechat_list_search, arginfo_weechat_list_search)
    PHP_FE(weechat_list_search_pos, arginfo_weechat_list_search_pos)
    PHP_FE(weechat_list_casesearch, arginfo_weechat_list_casesearch)
    PHP_FE(weechat_list_casesearch_pos, arginfo_weechat_list_casesearch_pos)
    PHP_FE(weechat_list_get, arginfo_weechat_list_get)
    PHP_FE(weechat_list_set, arginfo_weechat_list_set)
    PHP_FE(weechat_list_next, arginfo_weechat_list_next)
    PHP_FE(weechat_list_prev, arginfo_weechat_list_prev)
    PHP_FE(weechat_list_string, arginfo_weechat_list_string)
    PHP_FE(weechat_list_size, arginfo_weechat_list_size)
    PHP_FE(weechat_list_remove, arginfo_weechat_list_remove)
    PHP_FE(weechat_list_remove_all, arginfo_weechat_list_remove_all)
    PHP_FE(weechat_list_free, arginfo_weechat_list_free)
    PHP_FE(weechat_config_new, arginfo_weechat_config_new)
    PHP_FE(weechat_config_new_section, arginfo_weechat_config_new_section)
    PHP_FE(weechat_config_search_section, arginfo_weechat_config_search_section)
    PHP_FE(weechat_config_new_option, arginfo_weechat_config_new_option)
    PHP_FE(weechat_config_search_option, arginfo_weechat_config_search_option)
    PHP_FE(weechat_config_string_to_boolean, arginfo_weechat_config_string_to_boolean)
    PHP_FE(weechat_config_option_reset, arginfo_weechat_config_option_reset)
    PHP_FE(weechat_config_option_set, arginfo_weechat_config_option_set)
    PHP_FE(weechat_config_option_set_null, arginfo_weechat_config_option_set_null)
    PHP_FE(weechat_config_option_unset, arginfo_weechat_config_option_unset)
    PHP_FE(weechat_config_option_rename, arginfo_weechat_config_option_rename)
    PHP_FE(weechat_config_option_is_null, arginfo_weechat_config_option_is_null)
    PHP_FE(weechat_config_option_default_is_null, arginfo_weechat_config_option_default_is_null)
    PHP_FE(weechat_config_boolean, arginfo_weechat_config_boolean)
    PHP_FE(weechat_config_boolean_default, arginfo_weechat_config_boolean_default)
    PHP_FE(weechat_config_integer, arginfo_weechat_config_integer)
    PHP_FE(weechat_config_integer_default, arginfo_weechat_config_integer_default)
    PHP_FE(weechat_config_string, arginfo_weechat_config_string)
    PHP_FE(weechat_config_string_default, arginfo_weechat_config_string_default)
    PHP_FE(weechat_config_color, arginfo_weechat_config_color)
    PHP_FE(weechat_config_color_default, arginfo_weechat_config_color_default)
    PHP_FE(weechat_config_write_option, arginfo_weechat_config_write_option)
    PHP_FE(weechat_config_write_line, arginfo_weechat_config_write_line)
    PHP_FE(weechat_config_write, arginfo_weechat_config_write)
    PHP_FE(weechat_config_read, arginfo_weechat_config_read)
    PHP_FE(weechat_config_reload, arginfo_weechat_config_reload)
    PHP_FE(weechat_config_option_free, arginfo_weechat_config_option_free)
    PHP_FE(weechat_config_section_free_options, arginfo_weechat_config_section_free_options)
    PHP_FE(weechat_config_section_free, arginfo_weechat_config_section_free)
    PHP_FE(weechat_config_free, arginfo_weechat_config_free)
    PHP_FE(weechat_config_get, arginfo_weechat_config_get)
    PHP_FE(weechat_config_get_plugin, arginfo_weechat_config_get_plugin)
    PHP_FE(weechat_config_is_set_plugin, arginfo_weechat_config_is_set_plugin)
    PHP_FE(weechat_config_set_plugin, arginfo_weechat_config_set_plugin)
    PHP_FE(weechat_config_set_desc_plugin, arginfo_weechat_config_set_desc_plugin)
    PHP_FE(weechat_config_unset_plugin, arginfo_weechat_config_unset_plugin)
    PHP_FE(weechat_key_bind, arginfo_weechat_key_bind)
    PHP_FE(weechat_key_unbind, arginfo_weechat_key_unbind)
    PHP_FE(weechat_prefix, arginfo_weechat_prefix)
    PHP_FE(weechat_color, arginfo_weechat_color)
    PHP_FE(weechat_print, arginfo_weechat_print)
    PHP_FE(weechat_print_date_tags, arginfo_weechat_print_date_tags)
    PHP_FE(weechat_print_y, arginfo_weechat_print_y)
    PHP_FE(weechat_log_print, arginfo_weechat_log_print)
    PHP_FE(weechat_hook_command, arginfo_weechat_hook_command)
    PHP_FE(weechat_hook_completion, arginfo_weechat_hook_completion)
    PHP_FE(weechat_hook_completion_get_string, arginfo_weechat_hook_completion_get_string)
    PHP_FE(weechat_hook_completion_list_add, arginfo_weechat_hook_completion_list_add)
    PHP_FE(weechat_hook_command_run, arginfo_weechat_hook_command_run)
    PHP_FE(weechat_hook_timer, arginfo_weechat_hook_timer)
    PHP_FE(weechat_hook_fd, arginfo_weechat_hook_fd)
    PHP_FE(weechat_hook_process, arginfo_weechat_hook_process)
    PHP_FE(weechat_hook_process_hashtable, arginfo_weechat_hook_process_hashtable)
    PHP_FE(weechat_hook_connect, arginfo_weechat_hook_connect)
    PHP_FE(weechat_hook_line, arginfo_weechat_hook_line)
    PHP_FE(weechat_hook_print, arginfo_weechat_hook_print)
    PHP_FE(weechat_hook_signal, arginfo_weechat_hook_signal)
    PHP_FE(weechat_hook_signal_send, arginfo_weechat_hook_signal_send)
    PHP_FE(weechat_hook_hsignal, arginfo_weechat_hook_hsignal)
    PHP_FE(weechat_hook_hsignal_send, arginfo_weechat_hook_hsignal_send)
    PHP_FE(weechat_hook_config, arginfo_weechat_hook_config)
    PHP_FE(weechat_hook_modifier, arginfo_weechat_hook_modifier)
    PHP_FE(weechat_hook_modifier_exec, arginfo_weechat_hook_modifier_exec)
    PHP_FE(weechat_hook_info, arginfo_weechat_hook_info)
    PHP_FE(weechat_hook_info_hashtable, arginfo_weechat_hook_info_hashtable)
    PHP_FE(weechat_hook_infolist, arginfo_weechat_hook_infolist)
    PHP_FE(weechat_hook_focus, arginfo_weechat_hook_focus)
    PHP_FE(weechat_hook_set, arginfo_weechat_hook_set)
    PHP_FE(weechat_unhook, arginfo_weechat_unhook)
    PHP_FE(weechat_unhook_all, arginfo_weechat_unhook_all)
    PHP_FE(weechat_buffer_new, arginfo_weechat_buffer_new)
    PHP_FE(weechat_buffer_search, arginfo_weechat_buffer_search)
    PHP_FE(weechat_buffer_search_main, arginfo_weechat_buffer_search_main)
    PHP_FE(weechat_current_buffer, arginfo_weechat_current_buffer)
    PHP_FE(weechat_buffer_clear, arginfo_weechat_buffer_clear)
    PHP_FE(weechat_buffer_close, arginfo_weechat_buffer_close)
    PHP_FE(weechat_buffer_merge, arginfo_weechat_buffer_merge)
    PHP_FE(weechat_buffer_unmerge, arginfo_weechat_buffer_unmerge)
    PHP_FE(weechat_buffer_get_integer, arginfo_weechat_buffer_get_integer)
    PHP_FE(weechat_buffer_get_string, arginfo_weechat_buffer_get_string)
    PHP_FE(weechat_buffer_get_pointer, arginfo_weechat_buffer_get_pointer)
    PHP_FE(weechat_buffer_set, arginfo_weechat_buffer_set)
    PHP_FE(weechat_buffer_string_replace_local_var, arginfo_weechat_buffer_string_replace_local_var)
    PHP_FE(weechat_buffer_match_list, arginfo_weechat_buffer_match_list)
    PHP_FE(weechat_current_window, arginfo_weechat_current_window)
    PHP_FE(weechat_window_search_with_buffer, arginfo_weechat_window_search_with_buffer)
    PHP_FE(weechat_window_get_integer, arginfo_weechat_window_get_integer)
    PHP_FE(weechat_window_get_string, arginfo_weechat_window_get_string)
    PHP_FE(weechat_window_get_pointer, arginfo_weechat_window_get_pointer)
    PHP_FE(weechat_window_set_title, arginfo_weechat_window_set_title)
    PHP_FE(weechat_nicklist_add_group, arginfo_weechat_nicklist_add_group)
    PHP_FE(weechat_nicklist_search_group, arginfo_weechat_nicklist_search_group)
    PHP_FE(weechat_nicklist_add_nick, arginfo_weechat_nicklist_add_nick)
    PHP_FE(weechat_nicklist_search_nick, arginfo_weechat_nicklist_search_nick)
    PHP_FE(weechat_nicklist_remove_group, arginfo_weechat_nicklist_remove_group)
    PHP_FE(weechat_nicklist_remove_nick, arginfo_weechat_nicklist_remove_nick)
    PHP_FE(weechat_nicklist_remove_all, arginfo_weechat_nicklist_remove_all)
    PHP_FE(weechat_nicklist_group_get_integer, arginfo_weechat_nicklist_group_get_integer)
    PHP_FE(weechat_nicklist_group_get_string, arginfo_weechat_nicklist_group_get_string)
    PHP_FE(weechat_nicklist_group_get_pointer, arginfo_weechat_nicklist_group_get_pointer)
    PHP_FE(weechat_nicklist_group_set, arginfo_weechat_nicklist_group_set)
    PHP_FE(weechat_nicklist_nick_get_integer, arginfo_weechat_nicklist_nick_get_integer)
    PHP_FE(weechat_nicklist_nick_get_string, arginfo_weechat_nicklist_nick_get_string)
    PHP_FE(weechat_nicklist_nick_get_pointer, arginfo_weechat_nicklist_nick_get_pointer)
    PHP_FE(weechat_nicklist_nick_set, arginfo_weechat_nicklist_nick_set)
    PHP_FE(weechat_bar_item_search, arginfo_weechat_bar_item_search)
    PHP_FE(weechat_bar_item_new, arginfo_weechat_bar_item_new)
    PHP_FE(weechat_bar_item_update, arginfo_weechat_bar_item_update)
    PHP_FE(weechat_bar_item_remove, arginfo_weechat_bar_item_remove)
    PHP_FE(weechat_bar_search, arginfo_weechat_bar_search)
    PHP_FE(weechat_bar_new, arginfo_weechat_bar_new)
    PHP_FE(weechat_bar_set, arginfo_weechat_bar_set)
    PHP_FE(weechat_bar_update, arginfo_weechat_bar_update)
    PHP_FE(weechat_bar_remove, arginfo_weechat_bar_remove)
    PHP_FE(weechat_command, arginfo_weechat_command)
    PHP_FE(weechat_command_options, arginfo_weechat_command_options)
    PHP_FE(weechat_completion_new, arginfo_weechat_completion_new)
    PHP_FE(weechat_completion_search, arginfo_weechat_completion_search)
    PHP_FE(weechat_completion_get_string, arginfo_weechat_completion_get_string)
    PHP_FE(weechat_completion_list_add, arginfo_weechat_completion_list_add)
    PHP_FE(weechat_completion_free, arginfo_weechat_completion_free)
    PHP_FE(weechat_info_get, arginfo_weechat_info_get)
    PHP_FE(weechat_info_get_hashtable, arginfo_weechat_info_get_hashtable)
    PHP_FE(weechat_infolist_new, arginfo_weechat_infolist_new)
    PHP_FE(weechat_infolist_new_item, arginfo_weechat_infolist_new_item)
    PHP_FE(weechat_infolist_new_var_integer, arginfo_weechat_infolist_new_var_integer)
    PHP_FE(weechat_infolist_new_var_string, arginfo_weechat_infolist_new_var_string)
    PHP_FE(weechat_infolist_new_var_pointer, arginfo_weechat_infolist_new_var_pointer)
    PHP_FE(weechat_infolist_new_var_time, arginfo_weechat_infolist_new_var_time)
    PHP_FE(weechat_infolist_search_var, arginfo_weechat_infolist_search_var)
    PHP_FE(weechat_infolist_get, arginfo_weechat_infolist_get)
    PHP_FE(weechat_infolist_next, arginfo_weechat_infolist_next)
    PHP_FE(weechat_infolist_prev, arginfo_weechat_infolist_prev)
    PHP_FE(weechat_infolist_reset_item_cursor, arginfo_weechat_infolist_reset_item_cursor)
    PHP_FE(weechat_infolist_fields, arginfo_weechat_infolist_fields)
    PHP_FE(weechat_infolist_integer, arginfo_weechat_infolist_integer)
    PHP_FE(weechat_infolist_string, arginfo_weechat_infolist_string)
    PHP_FE(weechat_infolist_pointer, arginfo_weechat_infolist_pointer)
    PHP_FE(weechat_infolist_time, arginfo_weechat_infolist_time)
    PHP_FE(weechat_infolist_free, arginfo_weechat_infolist_free)
    PHP_FE(weechat_hdata_get, arginfo_weechat_hdata_get)
    PHP_FE(weechat_hdata_get_var_offset, arginfo_weechat_hdata_get_var_offset)
    PHP_FE(weechat_hdata_get_var_type_string, arginfo_weechat_hdata_get_var_type_string)
    PHP_FE(weechat_hdata_get_var_array_size, arginfo_weechat_hdata_get_var_array_size)
    PHP_FE(weechat_hdata_get_var_array_size_string, arginfo_weechat_hdata_get_var_array_size_string)
    PHP_FE(weechat_hdata_get_var_hdata, arginfo_weechat_hdata_get_var_hdata)
    PHP_FE(weechat_hdata_get_list, arginfo_weechat_hdata_get_list)
    PHP_FE(weechat_hdata_check_pointer, arginfo_weechat_hdata_check_pointer)
    PHP_FE(weechat_hdata_move, arginfo_weechat_hdata_move)
    PHP_FE(weechat_hdata_search, arginfo_weechat_hdata_search)
    PHP_FE(weechat_hdata_char, arginfo_weechat_hdata_char)
    PHP_FE(weechat_hdata_integer, arginfo_weechat_hdata_integer)
    PHP_FE(weechat_hdata_long, arginfo_weechat_hdata_long)
    PHP_FE(weechat_hdata_string, arginfo_weechat_hdata_string)
    PHP_FE(weechat_hdata_pointer, arginfo_weechat_hdata_pointer)
    PHP_FE(weechat_hdata_time, arginfo_weechat_hdata_time)
    PHP_FE(weechat_hdata_hashtable, arginfo_weechat_hdata_hashtable)
    PHP_FE(weechat_hdata_compare, arginfo_weechat_hdata_compare)
    PHP_FE(weechat_hdata_update, arginfo_weechat_hdata_update)
    PHP_FE(weechat_hdata_get_string, arginfo_weechat_hdata_get_string)
    PHP_FE(weechat_upgrade_new, arginfo_weechat_upgrade_new)
    PHP_FE(weechat_upgrade_write_object, arginfo_weechat_upgrade_write_object)
    PHP_FE(weechat_upgrade_read, arginfo_weechat_upgrade_read)
    PHP_FE(weechat_upgrade_close, arginfo_weechat_upgrade_close)
    PHP_FE(forget_class, arginfo_forget_class)
    PHP_FE(forget_function, arginfo_forget_function)
    PHP_FE_END
};

PHP_MINIT_FUNCTION(weechat)
{
    /* make C compiler happy */
    (void) type;

    /* Register integer constants */
    #define PHP_WEECHAT_CONSTANT(NAME) \
        zend_register_long_constant(#NAME, sizeof(#NAME)-1, (NAME), CONST_CS | CONST_PERSISTENT, module_number)
    PHP_WEECHAT_CONSTANT(WEECHAT_RC_OK);
    PHP_WEECHAT_CONSTANT(WEECHAT_RC_OK_EAT);
    PHP_WEECHAT_CONSTANT(WEECHAT_RC_ERROR);
    PHP_WEECHAT_CONSTANT(WEECHAT_CONFIG_READ_OK);
    PHP_WEECHAT_CONSTANT(WEECHAT_CONFIG_READ_MEMORY_ERROR);
    PHP_WEECHAT_CONSTANT(WEECHAT_CONFIG_READ_FILE_NOT_FOUND);
    PHP_WEECHAT_CONSTANT(WEECHAT_CONFIG_WRITE_OK);
    PHP_WEECHAT_CONSTANT(WEECHAT_CONFIG_WRITE_ERROR);
    PHP_WEECHAT_CONSTANT(WEECHAT_CONFIG_WRITE_MEMORY_ERROR);
    PHP_WEECHAT_CONSTANT(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED);
    PHP_WEECHAT_CONSTANT(WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE);
    PHP_WEECHAT_CONSTANT(WEECHAT_CONFIG_OPTION_SET_ERROR);
    PHP_WEECHAT_CONSTANT(WEECHAT_CONFIG_OPTION_SET_OPTION_NOT_FOUND);
    PHP_WEECHAT_CONSTANT(WEECHAT_CONFIG_OPTION_UNSET_OK_NO_RESET);
    PHP_WEECHAT_CONSTANT(WEECHAT_CONFIG_OPTION_UNSET_OK_RESET);
    PHP_WEECHAT_CONSTANT(WEECHAT_CONFIG_OPTION_UNSET_OK_REMOVED);
    PHP_WEECHAT_CONSTANT(WEECHAT_CONFIG_OPTION_UNSET_ERROR);
    PHP_WEECHAT_CONSTANT(WEECHAT_HDATA_OTHER);
    PHP_WEECHAT_CONSTANT(WEECHAT_HDATA_CHAR);
    PHP_WEECHAT_CONSTANT(WEECHAT_HDATA_INTEGER);
    PHP_WEECHAT_CONSTANT(WEECHAT_HDATA_LONG);
    PHP_WEECHAT_CONSTANT(WEECHAT_HDATA_STRING);
    PHP_WEECHAT_CONSTANT(WEECHAT_HDATA_POINTER);
    PHP_WEECHAT_CONSTANT(WEECHAT_HDATA_TIME);
    PHP_WEECHAT_CONSTANT(WEECHAT_HDATA_HASHTABLE);
    PHP_WEECHAT_CONSTANT(WEECHAT_HDATA_SHARED_STRING);
    PHP_WEECHAT_CONSTANT(WEECHAT_HDATA_LIST_CHECK_POINTERS);
    PHP_WEECHAT_CONSTANT(WEECHAT_HOOK_PROCESS_RUNNING);
    PHP_WEECHAT_CONSTANT(WEECHAT_HOOK_PROCESS_ERROR);
    PHP_WEECHAT_CONSTANT(WEECHAT_HOOK_PROCESS_CHILD);
    PHP_WEECHAT_CONSTANT(WEECHAT_HOOK_CONNECT_OK);
    PHP_WEECHAT_CONSTANT(WEECHAT_HOOK_CONNECT_ADDRESS_NOT_FOUND);
    PHP_WEECHAT_CONSTANT(WEECHAT_HOOK_CONNECT_IP_ADDRESS_NOT_FOUND);
    PHP_WEECHAT_CONSTANT(WEECHAT_HOOK_CONNECT_CONNECTION_REFUSED);
    PHP_WEECHAT_CONSTANT(WEECHAT_HOOK_CONNECT_PROXY_ERROR);
    PHP_WEECHAT_CONSTANT(WEECHAT_HOOK_CONNECT_LOCAL_HOSTNAME_ERROR);
    PHP_WEECHAT_CONSTANT(WEECHAT_HOOK_CONNECT_GNUTLS_INIT_ERROR);
    PHP_WEECHAT_CONSTANT(WEECHAT_HOOK_CONNECT_GNUTLS_HANDSHAKE_ERROR);
    PHP_WEECHAT_CONSTANT(WEECHAT_HOOK_CONNECT_MEMORY_ERROR);
    PHP_WEECHAT_CONSTANT(WEECHAT_HOOK_CONNECT_TIMEOUT);
    PHP_WEECHAT_CONSTANT(WEECHAT_HOOK_CONNECT_SOCKET_ERROR);
    PHP_WEECHAT_CONSTANT(WEECHAT_HOOK_CONNECT_GNUTLS_CB_VERIFY_CERT);
    PHP_WEECHAT_CONSTANT(WEECHAT_HOOK_CONNECT_GNUTLS_CB_SET_CERT);
    #undef PHP_WEECHAT_CONSTANT

    /* Register string constants */
    #define PHP_WEECHAT_CONSTANT(NAME) \
        zend_register_string_constant(#NAME, sizeof(#NAME)-1, (NAME), CONST_CS | CONST_PERSISTENT, module_number)
    PHP_WEECHAT_CONSTANT(WEECHAT_PLUGIN_API_VERSION);
    PHP_WEECHAT_CONSTANT(WEECHAT_CONFIG_OPTION_NULL);
    PHP_WEECHAT_CONSTANT(WEECHAT_LIST_POS_SORT);
    PHP_WEECHAT_CONSTANT(WEECHAT_LIST_POS_BEGINNING);
    PHP_WEECHAT_CONSTANT(WEECHAT_LIST_POS_END);
    PHP_WEECHAT_CONSTANT(WEECHAT_HASHTABLE_INTEGER);
    PHP_WEECHAT_CONSTANT(WEECHAT_HASHTABLE_STRING);
    PHP_WEECHAT_CONSTANT(WEECHAT_HASHTABLE_POINTER);
    PHP_WEECHAT_CONSTANT(WEECHAT_HASHTABLE_BUFFER);
    PHP_WEECHAT_CONSTANT(WEECHAT_HASHTABLE_TIME);
    PHP_WEECHAT_CONSTANT(WEECHAT_HOTLIST_LOW);
    PHP_WEECHAT_CONSTANT(WEECHAT_HOTLIST_MESSAGE);
    PHP_WEECHAT_CONSTANT(WEECHAT_HOTLIST_PRIVATE);
    PHP_WEECHAT_CONSTANT(WEECHAT_HOTLIST_HIGHLIGHT);
    PHP_WEECHAT_CONSTANT(WEECHAT_HOOK_SIGNAL_STRING);
    PHP_WEECHAT_CONSTANT(WEECHAT_HOOK_SIGNAL_INT);
    PHP_WEECHAT_CONSTANT(WEECHAT_HOOK_SIGNAL_POINTER);
    #undef PHP_WEECHAT_CONSTANT
    return SUCCESS;
}

zend_module_entry weechat_module_entry = {
    STANDARD_MODULE_HEADER,
    "weechat",
    weechat_functions,
    PHP_MINIT(weechat),
    NULL,
    NULL,
    NULL,
    NULL,
    PHP_WEECHAT_VERSION,
    STANDARD_MODULE_PROPERTIES
};

/*
 * Callback called for each key/value in a hashtable.
 */

void
weechat_php_hashtable_to_array_cb (void *data,
                                   struct t_hashtable *hashtable,
                                   const char *key,
                                   const char *value)
{
    /* make C compiler happy */
    (void) hashtable;

    add_assoc_string ((zval*)data, key, (char*)value);
}

/*
 * Converts a WeeChat hashtable to a PHP array.
 */

void
weechat_php_hashtable_to_array (struct t_hashtable *hashtable, zval *arr)
{
    array_init (arr);
    weechat_hashtable_map_string (hashtable,
                                  &weechat_php_hashtable_to_array_cb,
                                  arr);
}

/*
 * Converts a PHP array to a WeeChat hashtable.
 *
 * Note: hashtable must be freed after use.
 */

struct t_hashtable *
weechat_php_array_to_hashtable (zval *arr,
                                int size,
                                const char *type_keys,
                                const char *type_values)
{
    struct t_hashtable *hashtable;
    zend_string *key;
    zval *val;

    hashtable = weechat_hashtable_new (size, type_keys, type_values,
                                       NULL, NULL);
    if (!hashtable)
        return NULL;

    ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(arr), key, val) {
        if (strcmp (type_values, WEECHAT_HASHTABLE_STRING) == 0)
        {
            weechat_hashtable_set (hashtable,
                                   ZSTR_VAL(key),
                                   Z_STRVAL_P(val));
        }
        else if (strcmp (type_values, WEECHAT_HASHTABLE_POINTER) == 0)
        {
            weechat_hashtable_set (hashtable,
                                   ZSTR_VAL(key),
                                   plugin_script_str2ptr (
                                       weechat_php_plugin,
                                       NULL, NULL,
                                       Z_STRVAL_P(val)));
        }
    } ZEND_HASH_FOREACH_END();

    return hashtable;
}

static void
weechat_php_func_map_free_val (struct t_hashtable *hashtable,
                               const void *key, void *value)
{
    /* make C compiler happy */
    (void) hashtable;
    (void) key;

    zval_dtor ((zval *)value);
    efree ((zval *)value);
}

static void
weechat_php_func_map_free_key (struct t_hashtable *hashtable, void *key)
{
    /* make C compiler happy */
    (void) hashtable;

    free ((char *)key);
}

zval *
weechat_php_func_map_get (const char* func_name)
{
    if (!weechat_php_func_map)
        return NULL;

    return weechat_hashtable_get (weechat_php_func_map, func_name);
}

const char *
weechat_php_func_map_add (zval *ofunc)
{
    zval *func;
    const char *func_name;

    if (!weechat_php_func_map)
    {
        weechat_php_func_map = weechat_hashtable_new (32,
                                                      WEECHAT_HASHTABLE_STRING,
                                                      WEECHAT_HASHTABLE_POINTER,
                                                      NULL, NULL);
        weechat_hashtable_set_pointer (weechat_php_func_map,
                                       "callback_free_value",
                                       weechat_php_func_map_free_val);
        weechat_hashtable_set_pointer (weechat_php_func_map,
                                       "callback_free_key",
                                       weechat_php_func_map_free_key);
    }

    func = (zval *)safe_emalloc (sizeof (zval), 1, 0);
    ZVAL_COPY(func, ofunc);
    func_name = plugin_script_ptr2str (func);

    weechat_hashtable_set (weechat_php_func_map, func_name, func);

    return func_name;
}

/*
 * Executes a PHP function.
 */

void *
weechat_php_exec (struct t_plugin_script *script, int ret_type,
                  const char *function, const char *format, void **argv)
{
    int argc, i;
    zval *params;
    zval zretval;
    void *ret_value;
    int *ret_i;
    zend_fcall_info fci;
    zend_fcall_info_cache fci_cache;
    struct t_plugin_script *old_php_current_script;
    zval *zfunc;

    /* Save old script */
    old_php_current_script = php_current_script;
    php_current_script = script;

    /* Build func args */
    if (!format || !format[0])
    {
        argc = 0;
        params = NULL;
    }
    else
    {
        argc = strlen (format);
        params = safe_emalloc (sizeof (zval), argc, 0);

        for (i = 0; i < argc; i++)
        {
            switch (format[i])
            {
                case 's': /* string */
                    ZVAL_STRING(&params[i], (char *)argv[i]);
                    break;
                case 'i': /* integer */
                    ZVAL_LONG(&params[i], *((int *)argv[i]));
                    break;
                case 'h': /* hash */
                    weechat_php_hashtable_to_array ((struct t_hashtable *)argv[i],
                                                    &params[i]);
                    break;
            }
        }
    }

    /* Invoke func */
    ret_value = NULL;
    memset (&fci, 0, sizeof (zend_fcall_info));
    memset (&fci_cache, 0, sizeof (zend_fcall_info_cache));

    zfunc = weechat_php_func_map_get (function);
    if (zfunc && zend_fcall_info_init (zfunc, 0, &fci, &fci_cache, NULL, NULL) == SUCCESS)
    {
        fci.params = params;
        fci.param_count = argc;
        fci.retval = &zretval;
    }

    zend_try
    {
        if (zfunc && zend_call_function (&fci, &fci_cache) == SUCCESS)
        {
            if (ret_type == WEECHAT_SCRIPT_EXEC_STRING)
            {
                convert_to_string (&zretval);
                ret_value = strdup ((char *)Z_STRVAL(zretval));
            }
            else if (ret_type == WEECHAT_SCRIPT_EXEC_POINTER)
            {
                convert_to_string (&zretval);
                ret_value = plugin_script_str2ptr (weechat_php_plugin,
                                                   script->name, function,
                                                   (char *)Z_STRVAL(zretval));
            }
            else if (ret_type == WEECHAT_SCRIPT_EXEC_INT)
            {
                convert_to_long (&zretval);
                ret_i = malloc (sizeof (*ret_i));
                *ret_i = Z_LVAL(zretval);
                ret_value = ret_i;
            }
            else if (ret_type == WEECHAT_SCRIPT_EXEC_HASHTABLE)
            {
                ret_value = weechat_php_array_to_hashtable (&zretval,
                                                            WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                                            WEECHAT_HASHTABLE_STRING,
                                                            WEECHAT_HASHTABLE_STRING);
            }
            else
            {
                if (ret_type != WEECHAT_SCRIPT_EXEC_IGNORE)
                {
                    weechat_printf (NULL,
                                    weechat_gettext ("%s%s: function \"%s\" "
                                                     "must return a valid "
                                                     "value"),
                                    weechat_prefix ("error"), PHP_PLUGIN_NAME,
                                    function);
                }
            }
        }
        else
        {
            weechat_printf (NULL,
                            weechat_gettext ("%s%s: unable to run function \"%s\""),
                            weechat_prefix ("error"), PHP_PLUGIN_NAME, function);
        }
    }
    zend_end_try ();

    if ((ret_type != WEECHAT_SCRIPT_EXEC_IGNORE) && !ret_value)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: error in function \"%s\""),
                        weechat_prefix ("error"), PHP_PLUGIN_NAME, function);
    }

    /* Cleanup */
    if (params)
    {
        for (i = 0; i < argc; i++)
        {
            zval_ptr_dtor (&params[i]);
        }
        efree (params);
    }

    /* Restore old script */
    php_current_script = old_php_current_script;

    return ret_value;
}

/*
 * Loads a PHP script.
 *
 * If code is NULL, the content of filename is read and executed.
 * If code is not NULL, it is executed (the file is not read).
 *
 * Returns pointer to new registered script, NULL if error.
 */

struct t_plugin_script *
weechat_php_load (const char *filename, const char *code)
{
    zend_file_handle file_handle;

    /* make C compiler happy */
    /* TODO: implement load of code in PHP */
    (void) code;

    if ((weechat_php_plugin->debug >= 2) || !php_quiet)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s: loading script \"%s\""),
                        PHP_PLUGIN_NAME, filename);
    }

    php_current_script = NULL;
    php_registered_script = NULL;
    php_current_script_filename = filename;

    memset (&file_handle, 0, sizeof (file_handle));
    file_handle.type = ZEND_HANDLE_FILENAME;
    file_handle.filename = zend_string_init(filename, strlen(filename), 0);

    zend_try
    {
        php_execute_script (&file_handle);
    }
    zend_end_try ();

    if (!php_registered_script)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: function \"register\" not "
                                         "found (or failed) in file \"%s\""),
                        weechat_prefix ("error"), PHP_PLUGIN_NAME, filename);
        return NULL;
    }
    php_current_script = php_registered_script;

    plugin_script_set_buffer_callbacks (weechat_php_plugin,
                                        php_scripts,
                                        php_current_script,
                                        &weechat_php_api_buffer_input_data_cb,
                                        &weechat_php_api_buffer_close_cb);

    (void) weechat_hook_signal_send ("php_script_loaded",
                                     WEECHAT_HOOK_SIGNAL_STRING,
                                     php_current_script->filename);

    return php_current_script;
}

/*
 * Callback for script_auto_load() function.
 */

void
weechat_php_load_cb (void *data, const char *filename)
{
    /* make C compiler happy */
    (void) data;

    weechat_php_load (filename, NULL);
}

/*
 * Unloads a PHP script.
 */

void
weechat_php_unload (struct t_plugin_script *script)
{
    int *rc;
    char *filename;

    if ((weechat_php_plugin->debug >= 2) || !php_quiet)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s: unloading script \"%s\""),
                        PHP_PLUGIN_NAME, script->name);
    }

    if (script->shutdown_func && script->shutdown_func[0])
    {
        rc = (int *)weechat_php_exec (script,
                                      WEECHAT_SCRIPT_EXEC_INT,
                                      script->shutdown_func,
                                      NULL, NULL);
        if (rc)
            free (rc);
    }

    filename = strdup (script->filename);

    if (php_current_script == script)
        php_current_script = (php_current_script->prev_script) ?
            php_current_script->prev_script : php_current_script->next_script;

    plugin_script_remove (weechat_php_plugin, &php_scripts, &last_php_script, script);

    (void) weechat_hook_signal_send ("php_script_unloaded",
                                     WEECHAT_HOOK_SIGNAL_STRING, filename);
    if (filename)
        free (filename);
}

/*
 * Unloads a PHP script by name.
 */

void
weechat_php_unload_name (const char *name)
{
    struct t_plugin_script *ptr_script;

    ptr_script = plugin_script_search (weechat_php_plugin, php_scripts, name);
    if (ptr_script)
    {
        weechat_php_unload (ptr_script);
        if (!php_quiet)
        {
            weechat_printf (NULL,
                            weechat_gettext ("%s: script \"%s\" unloaded"),
                            PHP_PLUGIN_NAME, name);
        }
    }
    else
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: script \"%s\" not loaded"),
                        weechat_prefix ("error"), PHP_PLUGIN_NAME, name);
    }
}

/*
 * Unloads all PHP scripts.
 */

void
weechat_php_unload_all ()
{
    while (php_scripts)
    {
        weechat_php_unload (php_scripts);
    }
}

/*
 * Reloads a PHP script by name.
 */

void
weechat_php_reload_name (const char *name)
{
    struct t_plugin_script *ptr_script;
    char *filename;

    ptr_script = plugin_script_search (weechat_php_plugin, php_scripts, name);
    if (ptr_script)
    {
        filename = strdup (ptr_script->filename);
        if (filename)
        {
            weechat_php_unload (ptr_script);
            if (!php_quiet)
            {
                weechat_printf (NULL,
                                weechat_gettext ("%s: script \"%s\" unloaded"),
                                PHP_PLUGIN_NAME, name);
            }
            weechat_php_load (filename, NULL);
            free (filename);
        }
    }
    else
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: script \"%s\" not loaded"),
                        weechat_prefix ("error"), PHP_PLUGIN_NAME, name);
    }
}

/*
 * Evaluates PHP source code.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
weechat_php_eval (struct t_gui_buffer *buffer, int send_to_buffer_as_input,
                  int exec_commands, const char *code)
{
    /* TODO: implement PHP eval */
    (void) buffer;
    (void) send_to_buffer_as_input;
    (void) exec_commands;
    (void) code;

    return 1;
}

/*
 * Callback for command "/php".
 */

int
weechat_php_command_cb (const void *pointer, void *data,
                        struct t_gui_buffer *buffer,
                        int argc, char **argv, char **argv_eol)
{
    char *ptr_name, *ptr_code, *path_script;
    int i, send_to_buffer_as_input, exec_commands;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    if (argc == 1)
    {
        plugin_script_display_list (weechat_php_plugin, php_scripts,
                                    NULL, 0);
    }
    else if (argc == 2)
    {
        if (weechat_strcasecmp (argv[1], "list") == 0)
        {
            plugin_script_display_list (weechat_php_plugin, php_scripts,
                                        NULL, 0);
        }
        else if (weechat_strcasecmp (argv[1], "listfull") == 0)
        {
            plugin_script_display_list (weechat_php_plugin, php_scripts,
                                        NULL, 1);
        }
        else if (weechat_strcasecmp (argv[1], "autoload") == 0)
        {
            plugin_script_auto_load (weechat_php_plugin, &weechat_php_load_cb);
        }
        else if (weechat_strcasecmp (argv[1], "reload") == 0)
        {
            weechat_php_unload_all ();
            plugin_script_auto_load (weechat_php_plugin, &weechat_php_load_cb);
        }
        else if (weechat_strcasecmp (argv[1], "unload") == 0)
        {
            weechat_php_unload_all ();
        }
        else if (weechat_strcasecmp (argv[1], "version") == 0)
        {
            plugin_script_display_interpreter (weechat_php_plugin, 0);
        }
        else
            WEECHAT_COMMAND_ERROR;
    }
    else
    {
        if (weechat_strcasecmp (argv[1], "list") == 0)
        {
            plugin_script_display_list (weechat_php_plugin, php_scripts,
                                        argv_eol[2], 0);
        }
        else if (weechat_strcasecmp (argv[1], "listfull") == 0)
        {
            plugin_script_display_list (weechat_php_plugin, php_scripts,
                                        argv_eol[2], 1);
        }
        else if ((weechat_strcasecmp (argv[1], "load") == 0)
                 || (weechat_strcasecmp (argv[1], "reload") == 0)
                 || (weechat_strcasecmp (argv[1], "unload") == 0))
        {
            ptr_name = argv_eol[2];
            if (strncmp (ptr_name, "-q ", 3) == 0)
            {
                php_quiet = 1;
                ptr_name += 3;
                while (ptr_name[0] == ' ')
                {
                    ptr_name++;
                }
            }
            if (weechat_strcasecmp (argv[1], "load") == 0)
            {
                /* load PHP script */
                path_script = plugin_script_search_path (weechat_php_plugin,
                                                         ptr_name);
                weechat_php_load ((path_script) ? path_script : ptr_name,
                                  NULL);
                if (path_script)
                    free (path_script);
            }
            else if (weechat_strcasecmp (argv[1], "reload") == 0)
            {
                /* reload one PHP script */
                weechat_php_reload_name (ptr_name);
            }
            else if (weechat_strcasecmp (argv[1], "unload") == 0)
            {
                /* unload PHP script */
                weechat_php_unload_name (ptr_name);
            }
            php_quiet = 0;
        }
        else if (weechat_strcasecmp (argv[1], "eval") == 0)
        {
            send_to_buffer_as_input = 0;
            exec_commands = 0;
            ptr_code = argv_eol[2];
            for (i = 2; i < argc; i++)
            {
                if (argv[i][0] == '-')
                {
                    if (strcmp (argv[i], "-o") == 0)
                    {
                        if (i + 1 >= argc)
                            WEECHAT_COMMAND_ERROR;
                        send_to_buffer_as_input = 1;
                        exec_commands = 0;
                        ptr_code = argv_eol[i + 1];
                    }
                    else if (strcmp (argv[i], "-oc") == 0)
                    {
                        if (i + 1 >= argc)
                            WEECHAT_COMMAND_ERROR;
                        send_to_buffer_as_input = 1;
                        exec_commands = 1;
                        ptr_code = argv_eol[i + 1];
                    }
                }
                else
                    break;
            }
            if (!weechat_php_eval (buffer, send_to_buffer_as_input,
                                   exec_commands, ptr_code))
                WEECHAT_COMMAND_ERROR;
            /* TODO: implement /php eval */
            weechat_printf (NULL,
                            _("%sCommand \"/%s eval\" is not yet implemented"),
                            weechat_prefix ("error"),
                            weechat_php_plugin->name);
        }
        else
            WEECHAT_COMMAND_ERROR;
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds PHP scripts to completion list.
 */

int
weechat_php_completion_cb (const void *pointer, void *data,
                           const char *completion_item,
                           struct t_gui_buffer *buffer,
                           struct t_gui_completion *completion)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;
    (void) buffer;

    plugin_script_completion (weechat_php_plugin, completion, php_scripts);

    return WEECHAT_RC_OK;
}

/*
 * Returns hdata for PHP scripts.
 */

struct t_hdata *
weechat_php_hdata_cb (const void *pointer, void *data,
                      const char *hdata_name)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;

    return plugin_script_hdata_script (weechat_plugin,
                                       &php_scripts, &last_php_script,
                                       hdata_name);
}

/*
 * Returns PHP info "php_eval".
 */

char *
weechat_php_info_eval_cb (const void *pointer, void *data,
                          const char *info_name,
                          const char *arguments)
{
    const char *not_implemented = "not yet implemented";

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;
    (void) arguments;

    return strdup (not_implemented);
}

/*
 * Returns infolist with PHP scripts.
 */

struct t_infolist *
weechat_php_infolist_cb (const void *pointer, void *data,
                         const char *infolist_name,
                         void *obj_pointer, const char *arguments)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;

    if (!infolist_name || !infolist_name[0])
        return NULL;

    if (weechat_strcasecmp (infolist_name, "php_script") == 0)
    {
        return plugin_script_infolist_list_scripts (weechat_php_plugin,
                                                    php_scripts, obj_pointer,
                                                    arguments);
    }

    return NULL;
}

/*
 * Dumps PHP plugin data in WeeChat log file.
 */

int
weechat_php_signal_debug_dump_cb (const void *pointer, void *data,
                                  const char *signal,
                                  const char *type_data, void *signal_data)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) signal;
    (void) type_data;

    if (!signal_data
        || (weechat_strcasecmp ((char *)signal_data, PHP_PLUGIN_NAME) == 0))
    {
        plugin_script_print_log (weechat_php_plugin, php_scripts);
    }

    return WEECHAT_RC_OK;
}

/*
 * Timer for executing actions.
 */

int
weechat_php_timer_action_cb (const void *pointer, void *data,
                             int remaining_calls)
{
    /* make C compiler happy */
    (void) data;
    (void) remaining_calls;

    if (pointer)
    {
        if (pointer == &php_action_install_list)
        {
            plugin_script_action_install (weechat_php_plugin,
                                          php_scripts,
                                          &weechat_php_unload,
                                          &weechat_php_load,
                                          &php_quiet,
                                          &php_action_install_list);
        }
        else if (pointer == &php_action_remove_list)
        {
            plugin_script_action_remove (weechat_php_plugin,
                                         php_scripts,
                                         &weechat_php_unload,
                                         &php_quiet,
                                         &php_action_remove_list);
        }
        else if (pointer == &php_action_autoload_list)
        {
            plugin_script_action_autoload (weechat_php_plugin,
                                           &php_quiet,
                                           &php_action_autoload_list);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback called when a script action is asked (install/remove/autoload a
 * script).
 */

int
weechat_php_signal_script_action_cb (const void *pointer, void *data,
                                     const char *signal,
                                     const char *type_data,
                                     void *signal_data)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;

    if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_STRING) == 0)
    {
        if (strcmp (signal, "php_script_install") == 0)
        {
            plugin_script_action_add (&php_action_install_list,
                                      (const char *)signal_data);
            weechat_hook_timer (1, 0, 1,
                                &weechat_php_timer_action_cb,
                                &php_action_install_list, NULL);
        }
        else if (strcmp (signal, "php_script_remove") == 0)
        {
            plugin_script_action_add (&php_action_remove_list,
                                      (const char *)signal_data);
            weechat_hook_timer (1, 0, 1,
                                &weechat_php_timer_action_cb,
                                &php_action_remove_list, NULL);
        }
        else if (strcmp (signal, "php_script_autoload") == 0)
        {
            plugin_script_action_add (&php_action_autoload_list,
                                      (const char *)signal_data);
            weechat_hook_timer (1, 0, 1,
                                &weechat_php_timer_action_cb,
                                &php_action_autoload_list, NULL);
        }
    }

    return WEECHAT_RC_OK;
}

int
php_weechat_startup (sapi_module_struct *sapi_module)
{
    return php_module_startup (sapi_module, &weechat_module_entry, 1);
}

size_t
php_weechat_ub_write (const char *str, size_t str_length)
{
    weechat_printf (NULL, "php: %s", str);
    return str_length + 5;
}

void
php_weechat_sapi_error (int type, const char *format, ...)
{
    /* make C compiler happy */
    (void) type;

    weechat_va_format (format);
    if (vbuffer)
    {
        php_weechat_ub_write (vbuffer, strlen (vbuffer));
        free (vbuffer);
    }
}

#if PHP_VERSION_ID >= 80000
/* PHP >= 8.0 */
void
php_weechat_log_message (const char *message, int syslog_type_int)
{
    /* make C compiler happy */
    (void) syslog_type_int;

    php_weechat_ub_write (message, strlen (message));
}
#elif PHP_VERSION_ID >= 70100
/* 7.1 <= PHP < 8.0 */
void
php_weechat_log_message (char *message, int syslog_type_int)
{
    /* make C compiler happy */
    (void) syslog_type_int;

    php_weechat_ub_write (message, strlen (message));
}
#else
/* PHP 7.0 */
void
php_weechat_log_message (char *message)
{
    php_weechat_ub_write (message, strlen (message));
}
#endif

/*
 * Initializes PHP plugin.
 */

int
weechat_plugin_init (struct t_weechat_plugin *plugin, int argc, char *argv[])
{
    weechat_php_plugin = plugin;

    /* set interpreter name and version */
    weechat_hashtable_set (plugin->variables, "interpreter_name",
                           plugin->name);
#ifdef PHP_VERSION
    weechat_hashtable_set (plugin->variables, "interpreter_version",
                           PHP_VERSION);
#else
    weechat_hashtable_set (plugin->variables, "interpreter_version",
                           "");
#endif /* PHP_VERSION */

    php_data.config_file = &php_config_file;
    php_data.config_look_check_license = &php_config_look_check_license;
    php_data.config_look_eval_keep_context = &php_config_look_eval_keep_context;
    php_data.scripts = &php_scripts;
    php_data.last_script = &last_php_script;
    php_data.callback_command = &weechat_php_command_cb;
    php_data.callback_completion = &weechat_php_completion_cb;
    php_data.callback_hdata = &weechat_php_hdata_cb;
    php_data.callback_info_eval = &weechat_php_info_eval_cb;
    php_data.callback_infolist = &weechat_php_infolist_cb;
    php_data.callback_signal_debug_dump = &weechat_php_signal_debug_dump_cb;
    php_data.callback_signal_script_action = &weechat_php_signal_script_action_cb;
    php_data.callback_load_file = &weechat_php_load_cb;
    php_data.unload_all = &weechat_php_unload_all;

    php_embed_module.startup = php_weechat_startup;
    php_embed_module.ub_write = php_weechat_ub_write;
    php_embed_module.flush = NULL;
    php_embed_module.sapi_error = php_weechat_sapi_error;
    php_embed_module.log_message = php_weechat_log_message;

    php_embed_init (0, NULL);

    PG(report_zend_debug) = 0;  /* Turn off --enable-debug output */

    php_quiet = 1;
    plugin_script_init (weechat_php_plugin, argc, argv, &php_data);
    php_quiet = 0;

    plugin_script_display_short_list (weechat_php_plugin,
                                      php_scripts);

    /* init OK */
    return WEECHAT_RC_OK;
}

/*
 * Ends PHP plugin.
 */

int
weechat_plugin_end (struct t_weechat_plugin *plugin)
{
    /* unload all scripts */
    php_quiet = 1;
    if (php_script_eval)
    {
        weechat_php_unload (php_script_eval);
        php_script_eval = NULL;
    }
    plugin_script_end (plugin, &php_data);
    php_quiet = 0;

    if (weechat_php_func_map)
    {
        weechat_hashtable_remove_all (weechat_php_func_map);
        weechat_hashtable_free (weechat_php_func_map);
        weechat_php_func_map = NULL;
    }

    php_embed_shutdown ();

    if (php_action_install_list)
        free (php_action_install_list);
    if (php_action_remove_list)
        free (php_action_remove_list);
    if (php_action_autoload_list)
        free (php_action_autoload_list);
    /* weechat_string_dyn_free (php_buffer_output, 1); */

    return WEECHAT_RC_OK;
}
