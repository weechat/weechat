/*
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

#ifndef WEECHAT_PHP_H
#define WEECHAT_PHP_H 1

#define weechat_plugin weechat_php_plugin
#define PHP_PLUGIN_NAME "php"
#define PHP_WEECHAT_VERSION "0.1"

#define PHP_CURRENT_SCRIPT_NAME ((php_current_script) ? php_current_script->name : "-")

struct t_php_const
{
    char *name;
    int int_value;
    char *str_value;
};

extern int php_quiet;
extern struct t_weechat_plugin *weechat_php_plugin;

extern struct t_plugin_script *php_scripts;
extern struct t_plugin_script *last_php_script;
extern struct t_plugin_script *php_current_script;
extern struct t_plugin_script *php_registered_script;
extern const char *php_current_script_filename;

extern void weechat_php_hashtable_to_array (struct t_hashtable *hashtable, zval *arr);
extern struct t_hashtable *weechat_php_array_to_hashtable (zval* arr,
                                                           int size,
                                                           const char *type_keys,
                                                           const char *type_values);
extern void *weechat_php_exec (struct t_plugin_script *script,
                               int ret_type,
                               const char *function,
                               const char *format, void **argv);

#define weechat_php_nullchar_to_tab(__zstr)                             \
    do {                                                                \
        char *__tmp = ZSTR_VAL(__zstr);                                 \
        size_t __i;                                                     \
        for (__i = 0; __i < ZSTR_LEN(__zstr); __i++)                    \
            if (*(__tmp + __i) == '\0')                                 \
                *(__tmp + __i) = '\t';                                  \
    } while (0)

#define weechat_php_tab_to_nullchar(__str)                              \
    do {                                                                \
        char *__tmp = (char *)__str;                                    \
        int __i;                                                        \
        int __l = strlen(__str);                                        \
        for (__i = 0; __i < __l; __i++)                                 \
            if (*(__tmp + __i) == '\t')                                 \
                *(__tmp + __i) = '\0';                                  \
    } while (0)

#endif /* WEECHAT_PHP_H */
