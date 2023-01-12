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

#ifndef WEECHAT_PLUGIN_PHP_H
#define WEECHAT_PLUGIN_PHP_H

#define weechat_plugin weechat_php_plugin
#define PHP_PLUGIN_NAME "php"
#define PHP_PLUGIN_PRIORITY 4030

#define PHP_WEECHAT_VERSION "0.1"

#define PHP_CURRENT_SCRIPT_NAME ((php_current_script) ? php_current_script->name : "-")

struct t_php_const
{
    char *name;
    int int_value;
    char *str_value;
};

extern struct t_weechat_plugin *weechat_php_plugin;

extern struct t_plugin_script_data php_data;

extern struct t_hashtable *weechat_php_function_map;

extern int php_quiet;
extern struct t_plugin_script *php_scripts;
extern struct t_plugin_script *last_php_script;
extern struct t_plugin_script *php_current_script;
extern struct t_plugin_script *php_registered_script;
extern const char *php_current_script_filename;

extern void weechat_php_hashtable_to_array (struct t_hashtable *hashtable,
                                            zval *arr);
extern struct t_hashtable *weechat_php_array_to_hashtable (zval* arr,
                                                           int size,
                                                           const char *type_keys,
                                                           const char *type_values);
extern zval *weechat_php_func_map_get (const char *func_name);
extern const char *weechat_php_func_map_add (zval *ofunc);
extern void *weechat_php_exec (struct t_plugin_script *script,
                               int ret_type,
                               const char *function,
                               const char *format, void **argv);

#endif /* WEECHAT_PLUGIN_PHP_H */
