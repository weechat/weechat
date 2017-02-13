/*
 * weechat-php.c - PHP plugin for WeeChat
 *
 * Copyright (C) 2006-2017 Adam Saponara <as@php.net>
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

#include <sapi/embed/php_embed.h>
#include <php.h>
#include <php_ini.h>
#include <ext/standard/info.h>

#include "../weechat-plugin.h"
#include "../plugin-script.h"
#include "weechat-php.h"
#include "weechat-php-api.h"

WEECHAT_PLUGIN_NAME(PHP_PLUGIN_NAME);
WEECHAT_PLUGIN_DESCRIPTION(N_("Support of PHP scripts"));
WEECHAT_PLUGIN_AUTHOR("Adam Saponara <as@php.net>");
WEECHAT_PLUGIN_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_LICENSE(WEECHAT_LICENSE);
WEECHAT_PLUGIN_PRIORITY(3000);

struct t_weechat_plugin *weechat_php_plugin;
struct t_hashtable *weechat_php_func_map = NULL;
int php_quiet = 0;

struct t_plugin_script *php_scripts = NULL;
struct t_plugin_script *last_php_script = NULL;
struct t_plugin_script *php_current_script = NULL;
struct t_plugin_script *php_registered_script = NULL;
const char *php_current_script_filename = NULL;
char *php_action_install_list = NULL;
char *php_action_remove_list = NULL;
char *php_action_autoload_list = NULL;

const zend_function_entry weechat_functions[] = {
    PHP_FE(weechat_register, NULL)
    PHP_FE(weechat_bar_item_remove, NULL)
    PHP_FE(weechat_bar_item_search, NULL)
    PHP_FE(weechat_bar_item_update, NULL)
    PHP_FE(weechat_bar_new, NULL)
    PHP_FE(weechat_bar_remove, NULL)
    PHP_FE(weechat_bar_search, NULL)
    PHP_FE(weechat_bar_set, NULL)
    PHP_FE(weechat_bar_update, NULL)
    PHP_FE(weechat_buffer_clear, NULL)
    PHP_FE(weechat_buffer_close, NULL)
    PHP_FE(weechat_buffer_get_integer, NULL)
    PHP_FE(weechat_buffer_get_pointer, NULL)
    PHP_FE(weechat_buffer_get_string, NULL)
    PHP_FE(weechat_buffer_match_list, NULL)
    PHP_FE(weechat_buffer_merge, NULL)
    PHP_FE(weechat_buffer_search, NULL)
    PHP_FE(weechat_buffer_search_main, NULL)
    PHP_FE(weechat_buffer_set, NULL)
    PHP_FE(weechat_buffer_string_replace_local_var, NULL)
    PHP_FE(weechat_buffer_unmerge, NULL)
    PHP_FE(weechat_color, NULL)
    PHP_FE(weechat_config_boolean, NULL)
    PHP_FE(weechat_config_boolean_default, NULL)
    PHP_FE(weechat_config_color, NULL)
    PHP_FE(weechat_config_color_default, NULL)
    PHP_FE(weechat_config_free, NULL)
    PHP_FE(weechat_config_get, NULL)
    PHP_FE(weechat_config_integer, NULL)
    PHP_FE(weechat_config_integer_default, NULL)
    PHP_FE(weechat_config_option_default_is_null, NULL)
    PHP_FE(weechat_config_option_free, NULL)
    PHP_FE(weechat_config_option_is_null, NULL)
    PHP_FE(weechat_config_option_rename, NULL)
    PHP_FE(weechat_config_option_reset, NULL)
    PHP_FE(weechat_config_option_set, NULL)
    PHP_FE(weechat_config_option_set_null, NULL)
    PHP_FE(weechat_config_option_unset, NULL)
    PHP_FE(weechat_config_read, NULL)
    PHP_FE(weechat_config_reload, NULL)
    PHP_FE(weechat_config_search_option, NULL)
    PHP_FE(weechat_config_search_section, NULL)
    PHP_FE(weechat_config_section_free, NULL)
    PHP_FE(weechat_config_section_free_options, NULL)
    PHP_FE(weechat_config_string, NULL)
    PHP_FE(weechat_config_string_default, NULL)
    PHP_FE(weechat_config_string_to_boolean, NULL)
    PHP_FE(weechat_config_write, NULL)
    PHP_FE(weechat_config_write_line, NULL)
    PHP_FE(weechat_config_write_option, NULL)
    PHP_FE(weechat_gettext, NULL)
    PHP_FE(weechat_hdata_char, NULL)
    PHP_FE(weechat_hdata_check_pointer, NULL)
    PHP_FE(weechat_hdata_get, NULL)
    PHP_FE(weechat_hdata_get_list, NULL)
    PHP_FE(weechat_hdata_get_string, NULL)
    PHP_FE(weechat_hdata_get_var, NULL)
    PHP_FE(weechat_hdata_get_var_array_size, NULL)
    PHP_FE(weechat_hdata_get_var_array_size_string, NULL)
    PHP_FE(weechat_hdata_get_var_hdata, NULL)
    PHP_FE(weechat_hdata_get_var_offset, NULL)
    PHP_FE(weechat_hdata_get_var_type, NULL)
    PHP_FE(weechat_hdata_get_var_type_string, NULL)
    PHP_FE(weechat_hdata_hashtable, NULL)
    PHP_FE(weechat_hdata_integer, NULL)
    PHP_FE(weechat_hdata_long, NULL)
    PHP_FE(weechat_hdata_move, NULL)
    PHP_FE(weechat_hdata_pointer, NULL)
    PHP_FE(weechat_hdata_search, NULL)
    PHP_FE(weechat_hdata_string, NULL)
    PHP_FE(weechat_hdata_time, NULL)
    PHP_FE(weechat_hdata_update, NULL)
    PHP_FE(weechat_hook_completion_get_string, NULL)
    PHP_FE(weechat_hook_completion_list_add, NULL)
    PHP_FE(weechat_hook_hsignal_send, NULL)
    PHP_FE(weechat_hook_modifier_exec, NULL)
    PHP_FE(weechat_hook_set, NULL)
    PHP_FE(weechat_hook_signal_send, NULL)
    PHP_FE(weechat_iconv_from_internal, NULL)
    PHP_FE(weechat_iconv_to_internal, NULL)
    PHP_FE(weechat_info_get, NULL)
    PHP_FE(weechat_info_get_hashtable, NULL)
    PHP_FE(weechat_infolist_fields, NULL)
    PHP_FE(weechat_infolist_free, NULL)
    PHP_FE(weechat_infolist_get, NULL)
    PHP_FE(weechat_infolist_integer, NULL)
    PHP_FE(weechat_infolist_new, NULL)
    PHP_FE(weechat_infolist_new_item, NULL)
    PHP_FE(weechat_infolist_new_var_integer, NULL)
    PHP_FE(weechat_infolist_new_var_pointer, NULL)
    PHP_FE(weechat_infolist_new_var_string, NULL)
    PHP_FE(weechat_infolist_new_var_time, NULL)
    PHP_FE(weechat_infolist_next, NULL)
    PHP_FE(weechat_infolist_pointer, NULL)
    PHP_FE(weechat_infolist_prev, NULL)
    PHP_FE(weechat_infolist_reset_item_cursor, NULL)
    PHP_FE(weechat_infolist_search_var, NULL)
    PHP_FE(weechat_infolist_string, NULL)
    PHP_FE(weechat_infolist_time, NULL)
    PHP_FE(weechat_key_bind, NULL)
    PHP_FE(weechat_key_unbind, NULL)
    PHP_FE(weechat_list_add, NULL)
    PHP_FE(weechat_list_casesearch, NULL)
    PHP_FE(weechat_list_casesearch_pos, NULL)
    PHP_FE(weechat_list_free, NULL)
    PHP_FE(weechat_list_get, NULL)
    PHP_FE(weechat_list_new, NULL)
    PHP_FE(weechat_list_next, NULL)
    PHP_FE(weechat_list_prev, NULL)
    PHP_FE(weechat_list_remove, NULL)
    PHP_FE(weechat_list_remove_all, NULL)
    PHP_FE(weechat_list_search, NULL)
    PHP_FE(weechat_list_search_pos, NULL)
    PHP_FE(weechat_list_set, NULL)
    PHP_FE(weechat_list_size, NULL)
    PHP_FE(weechat_list_string, NULL)
    PHP_FE(weechat_mkdir, NULL)
    PHP_FE(weechat_mkdir_home, NULL)
    PHP_FE(weechat_mkdir_parents, NULL)
    PHP_FE(weechat_ngettext, NULL)
    PHP_FE(weechat_nicklist_add_group, NULL)
    PHP_FE(weechat_nicklist_add_nick, NULL)
    PHP_FE(weechat_nicklist_group_get_integer, NULL)
    PHP_FE(weechat_nicklist_group_get_pointer, NULL)
    PHP_FE(weechat_nicklist_group_get_string, NULL)
    PHP_FE(weechat_nicklist_group_set, NULL)
    PHP_FE(weechat_nicklist_nick_get_integer, NULL)
    PHP_FE(weechat_nicklist_nick_get_pointer, NULL)
    PHP_FE(weechat_nicklist_nick_get_string, NULL)
    PHP_FE(weechat_nicklist_nick_set, NULL)
    PHP_FE(weechat_nicklist_remove_all, NULL)
    PHP_FE(weechat_nicklist_remove_group, NULL)
    PHP_FE(weechat_nicklist_remove_nick, NULL)
    PHP_FE(weechat_nicklist_search_group, NULL)
    PHP_FE(weechat_nicklist_search_nick, NULL)
    PHP_FE(weechat_plugin_get_name, NULL)
    PHP_FE(weechat_bar_item_new, NULL)
    PHP_FE(weechat_buffer_new, NULL)
    PHP_FE(weechat_charset_set, NULL)
    PHP_FE(weechat_command, NULL)
    PHP_FE(weechat_config_get_plugin, NULL)
    PHP_FE(weechat_config_is_set_plugin, NULL)
    PHP_FE(weechat_config_new, NULL)
    PHP_FE(weechat_config_new_option, NULL)
    PHP_FE(weechat_config_new_section, NULL)
    PHP_FE(weechat_config_set_desc_plugin, NULL)
    PHP_FE(weechat_config_set_plugin, NULL)
    PHP_FE(weechat_config_unset_plugin, NULL)
    PHP_FE(weechat_hook_command, NULL)
    PHP_FE(weechat_hook_command_run, NULL)
    PHP_FE(weechat_hook_completion, NULL)
    PHP_FE(weechat_hook_config, NULL)
    PHP_FE(weechat_hook_connect, NULL)
    PHP_FE(weechat_hook_fd, NULL)
    PHP_FE(weechat_hook_focus, NULL)
    PHP_FE(weechat_hook_hsignal, NULL)
    PHP_FE(weechat_hook_info, NULL)
    PHP_FE(weechat_hook_info_hashtable, NULL)
    PHP_FE(weechat_hook_infolist, NULL)
    PHP_FE(weechat_hook_modifier, NULL)
    PHP_FE(weechat_hook_print, NULL)
    PHP_FE(weechat_hook_process, NULL)
    PHP_FE(weechat_hook_process_hashtable, NULL)
    PHP_FE(weechat_hook_signal, NULL)
    PHP_FE(weechat_hook_timer, NULL)
    PHP_FE(weechat_log_printf, NULL)
    PHP_FE(weechat_printf, NULL)
    PHP_FE(weechat_printf_date_tags, NULL)
    PHP_FE(weechat_printf_y, NULL)
    PHP_FE(weechat_upgrade_new, NULL)
    PHP_FE(weechat_prefix, NULL)
    PHP_FE(weechat_string_eval_expression, NULL)
    PHP_FE(weechat_string_eval_path_home, NULL)
    PHP_FE(weechat_string_has_highlight, NULL)
    PHP_FE(weechat_string_has_highlight_regex, NULL)
    PHP_FE(weechat_string_input_for_buffer, NULL)
    PHP_FE(weechat_string_is_command_char, NULL)
    PHP_FE(weechat_string_mask_to_regex, NULL)
    PHP_FE(weechat_string_match, NULL)
    PHP_FE(weechat_string_remove_color, NULL)
    PHP_FE(weechat_string_replace, NULL)
    PHP_FE(weechat_strlen_screen, NULL)
    PHP_FE(weechat_unhook, NULL)
    PHP_FE(weechat_unhook_all, NULL)
    PHP_FE(weechat_upgrade_close, NULL)
    PHP_FE(weechat_upgrade_read, NULL)
    PHP_FE(weechat_upgrade_write_object, NULL)
    PHP_FE(weechat_window_get_integer, NULL)
    PHP_FE(weechat_window_get_pointer, NULL)
    PHP_FE(weechat_window_get_string, NULL)
    PHP_FE(weechat_window_search_with_buffer, NULL)
    PHP_FE(weechat_window_set_title, NULL)
    PHP_FE_END
};

PHP_MINIT_FUNCTION(weechat)
{
    (void)type;

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

void
weechat_php_hashtable_to_array_cb (void *data,
                                 struct t_hashtable *hashtable,
                                 const char *key,
                                 const char *value)
{
    (void) hashtable;
    add_assoc_string((zval*)data, key, (char*)value);
}

void
weechat_php_hashtable_to_array (struct t_hashtable *hashtable, zval *arr)
{
    array_init(arr);
    weechat_hashtable_map_string (hashtable,
                                  &weechat_php_hashtable_to_array_cb,
                                  arr);
}

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
    if (!hashtable) return NULL;

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
        params = safe_emalloc (sizeof(zval), argc, 0);

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
    memset(&fci, 0, sizeof(zend_fcall_info));
    memset(&fci_cache, 0, sizeof(zend_fcall_info_cache));

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
                ret_value = strdup ((char *) Z_STRVAL(zretval));
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
                WEECHAT_SCRIPT_MSG_WRONG_ARGS(PHP_CURRENT_SCRIPT_NAME, function);
            }
        }
        else
        {
            weechat_printf (NULL,
                            weechat_gettext ("%s%s: unable to run function \"%s\""),
                            weechat_prefix ("error"), PHP_PLUGIN_NAME, function);
        }
    }
    zend_end_try();

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

int
weechat_php_load (const char *filename)
{
    zend_file_handle file_handle;

    php_current_script = NULL;
    php_registered_script = NULL;
    php_current_script_filename = filename;

    file_handle.filename = filename;
    file_handle.free_filename = 0;
    file_handle.type = ZEND_HANDLE_FILENAME;
    file_handle.opened_path = NULL;
    file_handle.handle.fp = NULL;

    zend_try
    {
        php_execute_script (&file_handle);
    }
    zend_end_try();

    if (!php_registered_script)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: function \"register\" not "
                                         "found (or failed) in file \"%s\""),
                        weechat_prefix ("error"), PHP_PLUGIN_NAME, filename);
        return 0;
    }
    php_current_script = php_registered_script;

    plugin_script_set_buffer_callbacks (weechat_php_plugin,
                                        php_scripts,
                                        php_current_script,
                                        weechat_php_buffer_new_input_callback,
                                        weechat_php_buffer_new_close_callback);

    (void) weechat_hook_signal_send ("php_script_loaded",
                                     WEECHAT_HOOK_SIGNAL_STRING,
                                     php_current_script->filename);

    return 1;
}

void
weechat_php_load_cb (void *data, const char *filename)
{
    (void) data;
    weechat_php_load (filename);
}

void
weechat_php_unload (struct t_plugin_script *script)
{
    int *rc;
    char *filename;

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

void
weechat_php_unload_name (const char *name)
{
    struct t_plugin_script *ptr_script;

    ptr_script = plugin_script_search (weechat_php_plugin, php_scripts, name);
    if (ptr_script)
    {
        weechat_php_unload (ptr_script);
    }
    else
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: script \"%s\" not loaded"),
                        weechat_prefix ("error"), PHP_PLUGIN_NAME, name);
    }
}

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
            weechat_php_load (filename);
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

void
weechat_php_unload_all ()
{
    while (php_scripts)
    {
        weechat_php_unload (php_scripts);
    }
}

int
weechat_php_command_cb (const void *pointer, void *data,
                        struct t_gui_buffer *buffer,
                        int argc, char **argv, char **argv_eol)
{
    char *ptr_name, *path_script;

    (void) pointer;
    (void) data;
    (void) buffer;

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
                ptr_name += 3;
                while (ptr_name[0] == ' ')
                {
                    ptr_name++;
                }
            }
            if (weechat_strcasecmp (argv[1], "load") == 0)
            {
                path_script = plugin_script_search_path (weechat_php_plugin,
                                                         ptr_name);
                weechat_php_load ((path_script) ? path_script : ptr_name);
                if (path_script)
                    free (path_script);
            }
            else if (weechat_strcasecmp (argv[1], "reload") == 0)
            {
                weechat_php_reload_name (ptr_name);
            }
            else if (weechat_strcasecmp (argv[1], "unload") == 0)
            {
                weechat_php_unload_name (ptr_name);
            }
        }
        else
            WEECHAT_COMMAND_ERROR;
    }

    return WEECHAT_RC_OK;
}

int
weechat_php_completion_cb (const void *pointer, void *data,
                           const char *completion_item,
                           struct t_gui_buffer *buffer,
                           struct t_gui_completion *completion)
{
    (void) pointer;
    (void) data;
    (void) completion_item;
    (void) buffer;

    plugin_script_completion (weechat_php_plugin, completion, php_scripts);

    return WEECHAT_RC_OK;
}

struct t_hdata *
weechat_php_hdata_cb (const void *pointer, void *data,
                      const char *hdata_name)
{
    (void) pointer;
    (void) data;

    return plugin_script_hdata_script (weechat_plugin,
                                       &php_scripts, &last_php_script,
                                       hdata_name);
}

struct t_infolist *
weechat_php_infolist_cb (const void *pointer, void *data,
                         const char *infolist_name,
                         void *obj_pointer, const char *arguments)
{
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

int
weechat_php_signal_debug_dump_cb (const void *pointer, void *data,
                                  const char *signal,
                                  const char *type_data, void *signal_data)
{
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

int
weechat_php_signal_debug_libs_cb (const void *pointer, void *data,
                                  const char *signal,
                                  const char *type_data, void *signal_data)
{
    (void) pointer;
    (void) data;
    (void) signal;
    (void) type_data;
    (void) signal_data;

#ifdef PHP_VERSION
    weechat_printf (NULL, "  %s: %s", PHP_PLUGIN_NAME, PHP_VERSION);
#else
    weechat_printf (NULL, "  %s: (?)", PHP_PLUGIN_NAME);
#endif /* PHP_VERSION */

    return WEECHAT_RC_OK;
}

int
weechat_php_timer_action_cb (const void *pointer, void *data,
                             int remaining_calls)
{
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

int
weechat_php_signal_script_action_cb (const void *pointer, void *data,
                                     const char *signal,
                                     const char *type_data,
                                     void *signal_data)
{
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
php_weechat_startup(sapi_module_struct *sapi_module)
{
    return php_module_startup (sapi_module, &weechat_module_entry, 1);
}

size_t
php_weechat_ub_write(const char *str, size_t str_length)
{
    weechat_printf (NULL, "php: %s", str);
    return str_length + 5;
}

void
php_weechat_sapi_error(int type, const char *format, ...)
{
    (void) type;
    weechat_va_format (format);
    php_weechat_ub_write (vbuffer, strlen (vbuffer));
    free (vbuffer);
}

void
php_weechat_log_message(char *message, int syslog_type_int)
{
    (void) syslog_type_int;
    php_weechat_ub_write (message, strlen (message));
}

int
weechat_plugin_init (struct t_weechat_plugin *plugin, int argc, char *argv[])
{
    struct t_plugin_script_init init;

    weechat_php_plugin = plugin;

    init.callback_command = &weechat_php_command_cb;
    init.callback_completion = &weechat_php_completion_cb;
    init.callback_hdata = &weechat_php_hdata_cb;
    init.callback_infolist = &weechat_php_infolist_cb;
    init.callback_signal_debug_dump = &weechat_php_signal_debug_dump_cb;
    init.callback_signal_debug_libs = &weechat_php_signal_debug_libs_cb;
    init.callback_signal_script_action = &weechat_php_signal_script_action_cb;
    init.callback_load_file = &weechat_php_load_cb;

    php_embed_module.startup = php_weechat_startup;
    php_embed_module.ub_write = php_weechat_ub_write;
    php_embed_module.flush = NULL;
    php_embed_module.sapi_error = php_weechat_sapi_error;
    php_embed_module.log_message = php_weechat_log_message;

    php_embed_init (0, NULL);

    plugin_script_init (weechat_php_plugin, argc, argv, &init);

    plugin_script_display_short_list (weechat_php_plugin,
                                      php_scripts);

    return WEECHAT_RC_OK;
}

int
weechat_plugin_end (struct t_weechat_plugin *plugin)
{

    plugin_script_end (plugin, &php_scripts, &weechat_php_unload_all);

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

    return WEECHAT_RC_OK;
}

static void
weechat_php_func_map_free_val (struct t_hashtable *hashtable, const void *key, void *value)
{
    zval_dtor ((zval *)value);
    efree ((zval *)value);
}

static void
weechat_php_func_map_free_key (struct t_hashtable *hashtable, void *key)
{
    free ((char *)key);
}

zval *
weechat_php_func_map_get (const char* func_name) {
    if (!weechat_php_func_map)
    {
        return NULL;
    }
    return weechat_hashtable_get (weechat_php_func_map, func_name);
}

char *
weechat_php_func_map_add (zval *ofunc)
{
    zval *func;
    char *func_name;

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

    func = (zval *)safe_emalloc (sizeof(zval), 1, 0);
    ZVAL_COPY(func, ofunc);
    func_name = plugin_script_ptr2str (func);

    weechat_hashtable_set (weechat_php_func_map, func_name, func);

    return func_name;
}
