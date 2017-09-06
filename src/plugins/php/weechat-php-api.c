/*
 * weechat-php-api.c - PHP API functions
 *
 * Copyright (C) 2006-2017 Adam Saponara <as@php.net>
 * Copyright (C) 2017 Sébastien Helleu <flashcode@flashtux.org>
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
#include "../plugin-script-api.h"
#include "weechat-php.h"


#define API_FUNC_INIT(__name)                                           \
    char *php_function_name = #__name;                                  \
    (void) php_function_name

#define API_PTR2STR(__pointer)                                          \
    plugin_script_ptr2str (__pointer)

#define API_STR2PTR(__string)                                           \
    plugin_script_str2ptr (weechat_php_plugin,                          \
                           PHP_CURRENT_SCRIPT_NAME,                     \
                           php_function_name, __string)

#define SAFE_RETURN_STRING(__c)                                         \
    RETURN_STRING((__c) ? (__c) : "")

#define weechat_php_get_function_name(__zfunc, __str)                   \
    char *(__str);                                                      \
    do                                                                  \
    {                                                                   \
        if (!zend_is_callable (__zfunc, 0, NULL))                       \
        {                                                               \
            php_error_docref (NULL, E_WARNING, "Expected callable");    \
            RETURN_FALSE;                                               \
        }                                                               \
        (__str) = weechat_php_func_map_add (__zfunc);                   \
    }                                                                   \
    while (0)

static char weechat_php_empty_arg[1] = { '\0' };

/*
 * Registers a PHP script.
 */

PHP_FUNCTION(weechat_register)
{
    zend_string *name, *author, *version, *license, *description, *charset;
    zval *shutdown_func;
    char *shutdown_func_name;

    API_FUNC_INIT(weechat_register);

    if (zend_parse_parameters (ZEND_NUM_ARGS() TSRMLS_CC, "SSSSSzS",
                               &name, &author, &version, &license, &description,
                               &shutdown_func, &charset) == FAILURE)
    {
        return;
    }

    if (php_registered_script)
    {
        /* script already registered */
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: script \"%s\" already "
                                         "registered (register ignored)"),
                        weechat_prefix ("error"), PHP_PLUGIN_NAME,
                        php_registered_script->name);
        RETURN_FALSE;
    }

    php_current_script = NULL;
    php_registered_script = NULL;

    if (plugin_script_search (weechat_php_plugin, php_scripts, ZSTR_VAL(name)))
    {
        /* another script already exists with same name */
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to register script "
                                         "\"%s\" (another script already "
                                         "exists with this name)"),
                        weechat_prefix ("error"), PHP_PLUGIN_NAME,
                        ZSTR_VAL(name));
        RETURN_FALSE;
    }

    /* resolve shutdown func */
    shutdown_func_name = NULL;
    if (zend_is_callable (shutdown_func, 0, NULL))
    {
        weechat_php_get_function_name (shutdown_func, shutdown_func_name_tmp);
        shutdown_func_name = shutdown_func_name_tmp;
    }

    /* register script */
    php_current_script = plugin_script_add (weechat_php_plugin,
                                            &php_scripts, &last_php_script,
                                            (php_current_script_filename) ?
                                            php_current_script_filename : "",
                                            ZSTR_VAL(name),
                                            ZSTR_VAL(author),
                                            ZSTR_VAL(version),
                                            ZSTR_VAL(license),
                                            ZSTR_VAL(description),
                                            shutdown_func_name,
                                            ZSTR_VAL(charset));
    if (php_current_script)
    {
        php_registered_script = php_current_script;
        if ((weechat_php_plugin->debug >= 2) || !php_quiet)
        {
            weechat_printf (NULL,
                            weechat_gettext ("%s: registered script \"%s\", "
                                             "version %s (%s)"),
                            PHP_PLUGIN_NAME, ZSTR_VAL(name), ZSTR_VAL(version),
                            ZSTR_VAL(description));
        }
    }
    else
    {
        RETURN_FALSE;
    }

    RETURN_TRUE;
}

static void
weechat_php_cb (const void *pointer, void *data, void **func_argv,
                const char *func_types, int func_type, void *rc)
{
    struct t_plugin_script *script;
    const char *ptr_function, *ptr_data;
    void *ret;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    func_argv[0] = ptr_data ? (char *)ptr_data : weechat_php_empty_arg;

    if (!ptr_function || !ptr_function[0])
    {
        goto weechat_php_cb_err;
    }

    ret = weechat_php_exec (script, func_type, ptr_function,
                            func_types, func_argv);

    if (!ret)
    {
        goto weechat_php_cb_err;
    }

    if (func_type == WEECHAT_SCRIPT_EXEC_INT)
    {
        *((int *)rc) = *((int *)ret);
        free (ret);
    }
    else if (func_type == WEECHAT_SCRIPT_EXEC_HASHTABLE)
    {
        *((struct t_hashtable **)rc) = (struct t_hashtable *)ret;
    }
    else
    {
        *((char **)rc) = (char *)ret;
    }
    return;

weechat_php_cb_err:
    if (func_type == WEECHAT_SCRIPT_EXEC_INT)
    {
        *((int *)rc) = WEECHAT_RC_ERROR;
    }
    else if (func_type == WEECHAT_SCRIPT_EXEC_HASHTABLE)
    {
        *((struct t_hashtable **)rc) = NULL;
    }
    else
    {
        *((char **)rc) = NULL;
    }
}

/*
 * Wrappers for functions in scripting API.
 *
 * For more info about these functions, look at their implementation in WeeChat
 * core.
 */

PHP_FUNCTION(weechat_plugin_get_name)
{
    const char *retval;

    API_FUNC_INIT(weechat_plugin_get_name);

    if (zend_parse_parameters_none () == FAILURE)
    {
        return;
    }
    retval = weechat_plugin_get_name (weechat_php_plugin);
    SAFE_RETURN_STRING(retval);
}

PHP_FUNCTION(weechat_charset_set)
{
    zend_string *z_charset;
    char *charset;

    API_FUNC_INIT(weechat_charset_set);

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_charset) == FAILURE)
    {
        return;
    }
    charset = ZSTR_VAL(z_charset);
    plugin_script_api_charset_set (php_current_script, (const char *)charset);
    RETURN_NULL();
}

PHP_FUNCTION(weechat_iconv_to_internal)
{
    zend_string *z_charset, *z_string;
    char *charset, *string, *retval;

    API_FUNC_INIT(weechat_iconv_to_internal);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SS", &z_charset, &z_string) == FAILURE)
    {
        return;
    }
    charset = ZSTR_VAL(z_charset);
    string = ZSTR_VAL(z_string);
    retval = weechat_iconv_to_internal ((const char *)charset,
                                        (const char *)string);
    SAFE_RETURN_STRING(retval);
}

PHP_FUNCTION(weechat_iconv_from_internal)
{
    zend_string *z_charset, *z_string;
    char *charset, *string, *retval;

    API_FUNC_INIT(weechat_iconv_from_internal);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SS", &z_charset, &z_string) == FAILURE)
    {
        return;
    }
    charset = ZSTR_VAL(z_charset);
    string = ZSTR_VAL(z_string);
    retval = weechat_iconv_from_internal ((const char *)charset,
                                          (const char *)string);
    SAFE_RETURN_STRING(retval);
}

PHP_FUNCTION(weechat_gettext)
{
    zend_string *z_string;
    char *string;
    const char *retval;

    API_FUNC_INIT(weechat_gettext);

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_string) == FAILURE)
    {
        return;
    }
    string = ZSTR_VAL(z_string);
    retval = weechat_gettext ((const char *)string);
    SAFE_RETURN_STRING(retval);
}

PHP_FUNCTION(weechat_ngettext)
{
    zend_string *z_single, *z_plural;
    zend_long z_count;
    char *single, *plural;
    int count;
    const char *retval;

    API_FUNC_INIT(weechat_ngettext);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SSl", &z_single, &z_plural,
                               &z_count) == FAILURE)
    {
        return;
    }
    single = ZSTR_VAL(z_single);
    plural = ZSTR_VAL(z_plural);
    count = (int)z_count;
    retval = weechat_ngettext ((const char *)single,
                               (const char *)plural,
                               count);
    SAFE_RETURN_STRING(retval);
}

PHP_FUNCTION(weechat_strlen_screen)
{
    zend_string *z_string;
    char *string;
    int retval;

    API_FUNC_INIT(weechat_strlen_screen);

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_string) == FAILURE)
    {
        return;
    }
    string = ZSTR_VAL(z_string);
    retval = weechat_strlen_screen ((const char *)string);
    RETURN_LONG(retval);
}

PHP_FUNCTION(weechat_string_match)
{
    zend_string *z_string, *z_mask;
    zend_long z_case_sensitive;
    int case_sensitive, retval;
    char *string, *mask;

    API_FUNC_INIT(weechat_string_match);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SSl", &z_string, &z_mask,
                               &z_case_sensitive) == FAILURE)
    {
        return;
    }
    string = ZSTR_VAL(z_string);
    mask = ZSTR_VAL(z_mask);
    case_sensitive = (int)z_case_sensitive;
    retval = weechat_string_match ((const char *)string,
                                   (const char *)mask,
                                   case_sensitive);
    RETURN_LONG(retval);
}

PHP_FUNCTION(weechat_string_has_highlight)
{
    zend_string *z_string, *z_highlight_words;
    char *string, *highlight_words;
    int retval;

    API_FUNC_INIT(weechat_string_has_highlight);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SS", &z_string, &z_highlight_words) == FAILURE)
    {
        return;
    }
    string = ZSTR_VAL(z_string);
    highlight_words = ZSTR_VAL(z_highlight_words);
    retval = weechat_string_has_highlight ((const char *)string,
                                           (const char *)highlight_words);
    RETURN_LONG(retval);
}

PHP_FUNCTION(weechat_string_has_highlight_regex)
{
    zend_string *z_string, *z_regex;
    char *string, *regex;
    int retval;

    API_FUNC_INIT(weechat_string_has_highlight_regex);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SS", &z_string, &z_regex) == FAILURE)
    {
        return;
    }
    string = ZSTR_VAL(z_string);
    regex = ZSTR_VAL(z_regex);
    retval = weechat_string_has_highlight_regex ((const char *)string,
                                                 (const char *)regex);
    RETURN_LONG(retval);
}

PHP_FUNCTION(weechat_string_mask_to_regex)
{
    zend_string *z_mask;
    char *mask, *retval;

    API_FUNC_INIT(weechat_string_mask_to_regex);

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_mask) == FAILURE)
    {
        return;
    }
    mask = ZSTR_VAL(z_mask);
    retval = weechat_string_mask_to_regex ((const char *)mask);
    SAFE_RETURN_STRING(retval);
}

PHP_FUNCTION(weechat_string_remove_color)
{
    zend_string *z_string, *z_replacement;
    char *string, *replacement, *retval;

    API_FUNC_INIT(weechat_string_remove_color);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SS", &z_string, &z_replacement) == FAILURE)
    {
        return;
    }
    string = ZSTR_VAL(z_string);
    replacement = ZSTR_VAL(z_replacement);
    retval = weechat_string_remove_color ((const char *)string,
                                          (const char *)replacement);
    SAFE_RETURN_STRING(retval);
}

PHP_FUNCTION(weechat_string_is_command_char)
{
    zend_string *z_string;
    char *string;
    int retval;

    API_FUNC_INIT(weechat_string_is_command_char);

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_string) == FAILURE)
    {
        return;
    }
    string = ZSTR_VAL(z_string);
    retval = weechat_string_is_command_char ((const char *)string);
    RETURN_LONG(retval);
}

PHP_FUNCTION(weechat_string_input_for_buffer)
{
    zend_string *z_string;
    char *string;
    const char *retval;

    API_FUNC_INIT(weechat_string_input_for_buffer);

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_string) == FAILURE)
    {
        return;
    }
    string = ZSTR_VAL(z_string);
    retval = weechat_string_input_for_buffer ((const char *)string);
    SAFE_RETURN_STRING(retval);
}

PHP_FUNCTION(weechat_string_eval_expression)
{
    zend_string *z_expr;
    zval *z_pointers, *z_extra_vars, *z_options;
    char *expr, *retval;
    struct t_hashtable *pointers, *extra_vars, *options;

    API_FUNC_INIT(weechat_string_eval_expression);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "Saaa", &z_expr, &z_pointers, &z_extra_vars,
                               &z_options) == FAILURE)
    {
        return;
    }
    expr = ZSTR_VAL(z_expr);
    pointers = weechat_php_array_to_hashtable (
        z_pointers,
        WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING);
    extra_vars = weechat_php_array_to_hashtable (
        z_extra_vars,
        WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING);
    options = weechat_php_array_to_hashtable (
        z_options,
        WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING);
    retval = weechat_string_eval_expression ((const char *)expr,
                                             pointers,
                                             extra_vars,
                                             options);
    SAFE_RETURN_STRING(retval);
}

PHP_FUNCTION(weechat_string_eval_path_home)
{
    zend_string *z_path;
    zval *z_pointers, *z_extra_vars, *z_options;
    char *path, *retval;
    struct t_hashtable *pointers, *extra_vars, *options;

    API_FUNC_INIT(weechat_string_eval_path_home);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "Saaa", &z_path, &z_pointers, &z_extra_vars,
                               &z_options) == FAILURE)
    {
        return;
    }
    path = ZSTR_VAL(z_path);
    pointers = weechat_php_array_to_hashtable (
        z_pointers,
        WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING);
    extra_vars = weechat_php_array_to_hashtable (
        z_extra_vars,
        WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING);
    options = weechat_php_array_to_hashtable (
        z_options,
        WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING);
    retval = weechat_string_eval_path_home ((const char *)path,
                                            pointers,
                                            extra_vars,
                                            options);
    SAFE_RETURN_STRING(retval);
}

PHP_FUNCTION(weechat_mkdir_home)
{
    zend_string *z_directory;
    zend_long z_mode;
    char *directory;
    int mode, retval;

    API_FUNC_INIT(weechat_mkdir_home);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "Sl", &z_directory, &z_mode) == FAILURE)
    {
        return;
    }
    directory = ZSTR_VAL(z_directory);
    mode = (int)z_mode;
    retval = weechat_mkdir_home ((const char *)directory, mode);
    RETURN_LONG(retval);
}

PHP_FUNCTION(weechat_mkdir)
{
    zend_string *z_directory;
    zend_long z_mode;
    char *directory;
    int mode, retval;

    API_FUNC_INIT(weechat_mkdir);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "Sl", &z_directory, &z_mode) == FAILURE)
    {
        return;
    }
    directory = ZSTR_VAL(z_directory);
    mode = (int)z_mode;
    retval = weechat_mkdir ((const char *)directory, mode);
    RETURN_LONG(retval);
}

PHP_FUNCTION(weechat_mkdir_parents)
{
    zend_string *z_directory;
    zend_long z_mode;
    char *directory;
    int mode, retval;

    API_FUNC_INIT(weechat_mkdir_parents);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "Sl", &z_directory, &z_mode) == FAILURE)
    {
        return;
    }
    directory = ZSTR_VAL(z_directory);
    mode = (int)z_mode;
    retval = weechat_mkdir_parents ((const char *)directory, mode);
    RETURN_LONG(retval);
}

PHP_FUNCTION(weechat_list_new)
{
    struct t_weelist *retval;

    API_FUNC_INIT(weechat_list_new);

    if (zend_parse_parameters_none () == FAILURE)
    {
        return;
    }
    retval = weechat_list_new ();
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

PHP_FUNCTION(weechat_list_add)
{
    zend_string *z_weelist, *z_data, *z_where, *z_user_data;
    struct t_weelist *weelist;
    char *data, *where;
    void *user_data;
    struct t_weelist_item *retval;

    API_FUNC_INIT(weechat_list_add);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SSSS", &z_weelist, &z_data, &z_where,
                               &z_user_data) == FAILURE)
    {
        return;
    }
    weelist = (struct t_weelist *)API_STR2PTR(ZSTR_VAL(z_weelist));
    data = ZSTR_VAL(z_data);
    where = ZSTR_VAL(z_where);
    user_data = (void *)API_STR2PTR(ZSTR_VAL(z_user_data));
    retval = weechat_list_add (weelist,
                               (const char *)data,
                               (const char *)where,
                               user_data);
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

PHP_FUNCTION(weechat_list_search)
{
    zend_string *z_weelist, *z_data;
    struct t_weelist *weelist;
    char *data;
    struct t_weelist_item *retval;

    API_FUNC_INIT(weechat_list_search);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SS", &z_weelist, &z_data) == FAILURE)
    {
        return;
    }
    weelist = (struct t_weelist *)API_STR2PTR(ZSTR_VAL(z_weelist));
    data = ZSTR_VAL(z_data);
    retval = weechat_list_search (weelist, (const char *)data);
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

PHP_FUNCTION(weechat_list_search_pos)
{
    zend_string *z_weelist, *z_data;
    struct t_weelist *weelist;
    char *data;
    int retval;

    API_FUNC_INIT(weechat_list_search_pos);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SS", &z_weelist, &z_data) == FAILURE)
    {
        return;
    }
    weelist = (struct t_weelist *)API_STR2PTR(ZSTR_VAL(z_weelist));
    data = ZSTR_VAL(z_data);
    retval = weechat_list_search_pos (weelist, (const char *)data);
    RETURN_LONG(retval);
}

PHP_FUNCTION(weechat_list_casesearch)
{
    zend_string *z_weelist, *z_data;
    struct t_weelist *weelist;
    char *data;
    struct t_weelist_item *retval;

    API_FUNC_INIT(weechat_list_casesearch);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SS", &z_weelist, &z_data) == FAILURE)
    {
        return;
    }
    weelist = (struct t_weelist *)API_STR2PTR(ZSTR_VAL(z_weelist));
    data = ZSTR_VAL(z_data);
    retval = weechat_list_casesearch (weelist, (const char *)data);
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

PHP_FUNCTION(weechat_list_casesearch_pos)
{
    zend_string *z_weelist, *z_data;
    struct t_weelist *weelist;
    char *data;
    int retval;

    API_FUNC_INIT(weechat_list_casesearch_pos);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SS", &z_weelist, &z_data) == FAILURE)
    {
        return;
    }
    weelist = (struct t_weelist *)API_STR2PTR(ZSTR_VAL(z_weelist));
    data = ZSTR_VAL(z_data);
    retval = weechat_list_casesearch_pos (weelist, (const char *)data);
    RETURN_LONG(retval);
}

PHP_FUNCTION(weechat_list_get)
{
    zend_string *z_weelist;
    zend_long z_position;
    struct t_weelist *weelist;
    int position;
    struct t_weelist_item *retval;

    API_FUNC_INIT(weechat_list_get);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "Sl", &z_weelist, &z_position) == FAILURE)
    {
        return;
    }
    weelist = (struct t_weelist *)API_STR2PTR(ZSTR_VAL(z_weelist));
    position = (int)z_position;
    retval = weechat_list_get (weelist, position);
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

PHP_FUNCTION(weechat_list_set)
{
    zend_string *z_item, *z_value;
    struct t_weelist_item *item;
    char *value;

    API_FUNC_INIT(weechat_list_set);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SS", &z_item, &z_value) == FAILURE)
    {
        return;
    }
    item = (struct t_weelist_item *)API_STR2PTR(ZSTR_VAL(z_item));
    value = ZSTR_VAL(z_value);
    weechat_list_set (item, (const char *)value);
    RETURN_NULL();
}

PHP_FUNCTION(weechat_list_next)
{
    zend_string *z_item;
    struct t_weelist_item *item, *retval;

    API_FUNC_INIT(weechat_list_next);

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_item) == FAILURE)
    {
        return;
    }
    item = (struct t_weelist_item *)API_STR2PTR(ZSTR_VAL(z_item));
    retval = weechat_list_next (item);
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

PHP_FUNCTION(weechat_list_prev)
{
    zend_string *z_item;
    struct t_weelist_item *item, *retval;

    API_FUNC_INIT(weechat_list_prev);

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_item) == FAILURE)
    {
        return;
    }
    item = (struct t_weelist_item *)API_STR2PTR(ZSTR_VAL(z_item));
    retval = weechat_list_prev (item);
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

PHP_FUNCTION(weechat_list_string)
{
    zend_string *z_item;
    const char *retval;
    struct t_weelist_item *item;

    API_FUNC_INIT(weechat_list_string);

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_item) == FAILURE)
    {
        return;
    }
    item = (struct t_weelist_item *)API_STR2PTR(ZSTR_VAL(z_item));
    retval = weechat_list_string (item);
    SAFE_RETURN_STRING(retval);
}

PHP_FUNCTION(weechat_list_size)
{
    zend_string *z_weelist;
    struct t_weelist *weelist;
    int retval;

    API_FUNC_INIT(weechat_list_size);

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_weelist) == FAILURE)
    {
        return;
    }
    weelist = (struct t_weelist *)API_STR2PTR(ZSTR_VAL(z_weelist));
    retval = weechat_list_size (weelist);
    RETURN_LONG(retval);
}

PHP_FUNCTION(weechat_list_remove)
{
    zend_string *z_weelist, *z_item;
    struct t_weelist *weelist;
    struct t_weelist_item *item;

    API_FUNC_INIT(weechat_list_remove);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SS", &z_weelist, &z_item) == FAILURE)
    {
        return;
    }
    weelist = (struct t_weelist *)API_STR2PTR(ZSTR_VAL(z_weelist));
    item = (struct t_weelist_item *)API_STR2PTR(ZSTR_VAL(z_item));
    weechat_list_remove (weelist, item);
    RETURN_NULL();
}

PHP_FUNCTION(weechat_list_remove_all)
{
    zend_string *z_weelist;
    struct t_weelist *weelist;

    API_FUNC_INIT(weechat_list_remove_all);

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_weelist) == FAILURE)
    {
        return;
    }
    weelist = (struct t_weelist *)API_STR2PTR(ZSTR_VAL(z_weelist));
    weechat_list_remove_all (weelist);
    RETURN_NULL();
}

PHP_FUNCTION(weechat_list_free)
{
    zend_string *z_weelist;
    struct t_weelist *weelist;

    API_FUNC_INIT(weechat_list_free);

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_weelist) == FAILURE)
    {
        return;
    }
    weelist = (struct t_weelist *)API_STR2PTR(ZSTR_VAL(z_weelist));
    weechat_list_free (weelist);
    RETURN_NULL();
}

static int
weechat_php_config_new_callback_reload (const void *pointer, void *data,
                                        struct t_config_file *config_file)
{
    int rc;
    void *func_argv[2];

    func_argv[1] = API_PTR2STR(config_file);

    weechat_php_cb (pointer, data, func_argv, "ss",
                    WEECHAT_SCRIPT_EXEC_INT, &rc);

    if (func_argv[1])
        free (func_argv[1]);

    return rc;
}

PHP_FUNCTION(weechat_config_new)
{
    zend_string *z_name;
    zval *z_callback_reload;
    zend_string *z_data;
    char *name, *data;
    struct t_config_file *retval;

    API_FUNC_INIT(weechat_config_new);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SzS", &z_name, &z_callback_reload,
                               &z_data) == FAILURE)
    {
        return;
    }
    name = ZSTR_VAL(z_name);
    weechat_php_get_function_name (z_callback_reload, callback_reload_name);
    data = ZSTR_VAL(z_data);
    retval = plugin_script_api_config_new (
        weechat_php_plugin,
        php_current_script,
        (const char *)name,
        weechat_php_config_new_callback_reload,
        (const char *)callback_reload_name,
        (const char *)data);
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

static int
weechat_php_config_new_section_callback_read (const void *pointer, void *data,
                                              struct t_config_file *config_file,
                                              struct t_config_section *section,
                                              const char *option_name,
                                              const char *value)
{
    int rc;
    void *func_argv[5];

    func_argv[1] = API_PTR2STR(config_file);
    func_argv[2] = API_PTR2STR(section);
    func_argv[3] = option_name ? (char *)option_name : weechat_php_empty_arg;
    func_argv[4] = value ? (char *)value : weechat_php_empty_arg;

    weechat_php_cb (pointer, data, func_argv, "sssss",
                    WEECHAT_SCRIPT_EXEC_INT, &rc);

    if (func_argv[1])
        free (func_argv[1]);
    if (func_argv[2])
        free (func_argv[2]);

    return rc;
}

static int
weechat_php_config_new_section_callback_write (const void *pointer, void *data,
                                               struct t_config_file *config_file,
                                               const char *section_name)
{
    int rc;
    void *func_argv[3];

    func_argv[1] = API_PTR2STR(config_file);
    func_argv[2] = section_name ? (char *)section_name : weechat_php_empty_arg;

    weechat_php_cb (pointer, data, func_argv, "sss",
                    WEECHAT_SCRIPT_EXEC_INT, &rc);

    if (func_argv[1])
        free (func_argv[1]);

    return rc;
}

static int
weechat_php_config_new_section_callback_write_default (const void *pointer,
                                                       void *data,
                                                       struct t_config_file *config_file,
                                                       const char *section_name)
{
    int rc;
    void *func_argv[3];

    func_argv[1] = API_PTR2STR(config_file);
    func_argv[2] = section_name ? (char *)section_name : weechat_php_empty_arg;

    weechat_php_cb (pointer, data, func_argv, "sss",
                    WEECHAT_SCRIPT_EXEC_INT, &rc);

    if (func_argv[1])
        free (func_argv[1]);

    return rc;
}

static int
weechat_php_config_new_section_callback_create_option (const void *pointer,
                                                       void *data,
                                                       struct t_config_file *config_file,
                                                       struct t_config_section *section,
                                                       const char *option_name,
                                                       const char *value)
{
    int rc;
    void *func_argv[5];

    func_argv[1] = API_PTR2STR(config_file);
    func_argv[2] = API_PTR2STR(section);
    func_argv[3] = option_name ? (char *)option_name : weechat_php_empty_arg;
    func_argv[4] = value ? (char *)value : weechat_php_empty_arg;

    weechat_php_cb (pointer, data, func_argv, "sssss",
                    WEECHAT_SCRIPT_EXEC_INT, &rc);

    if (func_argv[1])
        free (func_argv[1]);
    if (func_argv[2])
        free (func_argv[2]);

    return rc;
}

static int
weechat_php_config_new_section_callback_delete_option (const void *pointer,
                                                       void *data,
                                                       struct t_config_file *config_file,
                                                       struct t_config_section *section,
                                                       struct t_config_option *option)
{
    int rc;
    void *func_argv[4];

    func_argv[1] = API_PTR2STR(config_file);
    func_argv[2] = API_PTR2STR(section);
    func_argv[3] = API_PTR2STR(option);

    weechat_php_cb (pointer, data, func_argv, "ssss",
                    WEECHAT_SCRIPT_EXEC_INT, &rc);

    if (func_argv[1])
        free (func_argv[1]);
    if (func_argv[2])
        free (func_argv[2]);
    if (func_argv[3])
        free (func_argv[3]);

    return rc;
}

PHP_FUNCTION(weechat_config_new_section)
{
    zend_string *z_config_file, *z_name;
    zend_long z_user_can_add_options, z_user_can_delete_options;
    zval *z_callback_read, *z_callback_write, *z_callback_write_default;
    zval *z_callback_create_option, *z_callback_delete_option;
    zend_string *z_data_read, *z_data_write, *z_data_write_default;
    zend_string *z_data_create_option, *z_data_delete_option;
    struct t_config_section *retval;
    struct t_config_file *config_file;
    char *name;
    int user_can_add_options, user_can_delete_options;
    char *data_read, *data_write, *data_write_default;
    char *data_create_option, *data_delete_option;

    API_FUNC_INIT(weechat_config_new_section);

    if (zend_parse_parameters (
            ZEND_NUM_ARGS(), "SSllzSzSzSzSzS", &z_config_file, &z_name,
            &z_user_can_add_options, &z_user_can_delete_options,
            &z_callback_read, &z_data_read, &z_callback_write, &z_data_write,
            &z_callback_write_default, &z_data_write_default,
            &z_callback_create_option, &z_data_create_option,
            &z_callback_delete_option, &z_data_delete_option) == FAILURE)
    {
        return;
    }
    config_file = (struct t_config_file *)API_STR2PTR(ZSTR_VAL(z_config_file));
    name = ZSTR_VAL(z_name);
    user_can_add_options = (int)z_user_can_add_options;
    user_can_delete_options = (int)z_user_can_delete_options;
    weechat_php_get_function_name (z_callback_read, callback_read_name);
    data_read = ZSTR_VAL(z_data_read);
    weechat_php_get_function_name (z_callback_write, callback_write_name);
    data_write = ZSTR_VAL(z_data_write);
    weechat_php_get_function_name (z_callback_write_default,
                                   callback_write_default_name);
    data_write_default = ZSTR_VAL(z_data_write_default);
    weechat_php_get_function_name (z_callback_create_option,
                                   callback_create_option_name);
    data_create_option = ZSTR_VAL(z_data_create_option);
    weechat_php_get_function_name (z_callback_delete_option,
                                   callback_delete_option_name);
    data_delete_option = ZSTR_VAL(z_data_delete_option);
    retval = plugin_script_api_config_new_section (
        weechat_php_plugin,
        php_current_script,
        config_file,
        (const char *)name,
        user_can_add_options,
        user_can_delete_options,
        weechat_php_config_new_section_callback_read,
        (const char *)callback_read_name,
        (const char *)data_read,
        weechat_php_config_new_section_callback_write,
        (const char *)callback_write_name,
        (const char *)data_write,
        weechat_php_config_new_section_callback_write_default,
        (const char *)callback_write_default_name,
        (const char *)data_write_default,
        weechat_php_config_new_section_callback_create_option,
        (const char *)callback_create_option_name,
        (const char *)data_create_option,
        weechat_php_config_new_section_callback_delete_option,
        (const char *)callback_delete_option_name,
        (const char *)data_delete_option);
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

PHP_FUNCTION(weechat_config_search_section)
{
    zend_string *z_config_file, *z_section_name;
    struct t_config_file *config_file;
    char *section_name;
    struct t_config_section *retval;

    API_FUNC_INIT(weechat_config_search_section);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SS", &z_config_file,
                               &z_section_name) == FAILURE)
    {
        return;
    }
    config_file = (struct t_config_file *)API_STR2PTR(ZSTR_VAL(z_config_file));
    section_name = ZSTR_VAL(z_section_name);
    retval = weechat_config_search_section (config_file,
                                            (const char *)section_name);
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

static int
weechat_php_config_new_option_callback_check_value (const void *pointer,
                                                    void *data,
                                                    struct t_config_option *option,
                                                    const char *value)
{
    int rc;
    void *func_argv[3];

    func_argv[1] = API_PTR2STR(option);
    func_argv[2] = value ? (char *)value : weechat_php_empty_arg;

    weechat_php_cb (pointer, data, func_argv, "sss",
                    WEECHAT_SCRIPT_EXEC_INT, &rc);

    if (func_argv[1])
        free (func_argv[1]);

    return rc;
}

static void
weechat_php_config_new_option_callback_change (const void *pointer,
                                               void *data,
                                               struct t_config_option *option)
{
    int *rc;
    void *func_argv[2];

    func_argv[1] = API_PTR2STR(option);

    weechat_php_cb (pointer, data, func_argv, "ss",
                    WEECHAT_SCRIPT_EXEC_INT, &rc);

    if (func_argv[1])
        free (func_argv[1]);
}

static void
weechat_php_config_new_option_callback_delete (const void *pointer,
                                               void *data,
                                               struct t_config_option *option)
{
    int rc;
    void *func_argv[2];

    func_argv[1] = API_PTR2STR(option);

    weechat_php_cb (pointer, data, func_argv, "ss",
                   WEECHAT_SCRIPT_EXEC_INT, &rc);

    if (func_argv[1])
        free (func_argv[1]);
}

PHP_FUNCTION(weechat_config_new_option)
{
    zend_string *z_config_file, *z_section, *z_name, *z_type, *z_description;
    zend_string *z_string_values, *z_default_value, *z_value;
    zend_string *z_data_check_value, *z_data_change, *z_data_delete;
    zend_long z_min, z_max, z_null_value_allowed;
    zval *z_callback_check_value, *z_callback_change, *z_callback_delete;
    struct t_config_file *config_file;
    struct t_config_section *section;
    char *name, *type, *description, *string_values, *default_value, *value;
    char *data_check_value, *data_change, *data_delete;
    int min, max, null_value_allowed;
    struct t_config_option *retval;

    API_FUNC_INIT(weechat_config_new_option);

    if (zend_parse_parameters (
            ZEND_NUM_ARGS(), "SSSSSSllSSlzSzSzS", &z_config_file, &z_section,
            &z_name, &z_type, &z_description, &z_string_values, &z_min, &z_max,
            &z_default_value, &z_value, &z_null_value_allowed,
            &z_callback_check_value, &z_data_check_value, &z_callback_change,
            &z_data_change, &z_callback_delete, &z_data_delete) == FAILURE)
    {
        return;
    }
    config_file = (struct t_config_file *)API_STR2PTR(ZSTR_VAL(z_config_file));
    section = (struct t_config_section *)API_STR2PTR(ZSTR_VAL(z_section));
    name = ZSTR_VAL(z_name);
    type = ZSTR_VAL(z_type);
    description = ZSTR_VAL(z_description);
    string_values = ZSTR_VAL(z_string_values);
    min = (int)z_min;
    max = (int)z_max;
    default_value = ZSTR_VAL(z_default_value);
    value = ZSTR_VAL(z_value);
    null_value_allowed = (int)z_null_value_allowed;
    weechat_php_get_function_name (z_callback_check_value,
                                   callback_check_value_name);
    data_check_value = ZSTR_VAL(z_data_check_value);
    weechat_php_get_function_name (z_callback_change, callback_change_name);
    data_change = ZSTR_VAL(z_data_change);
    weechat_php_get_function_name (z_callback_delete, callback_delete_name);
    data_delete = ZSTR_VAL(z_data_delete);
    retval = plugin_script_api_config_new_option (
        weechat_php_plugin,
        php_current_script,
        config_file,
        section,
        (const char *)name,
        (const char *)type,
        (const char *)description,
        (const char *)string_values,
        min,
        max,
        (const char *)default_value,
        (const char *)value,
        null_value_allowed,
        weechat_php_config_new_option_callback_check_value,
        (const char *)callback_check_value_name,
        (const char *)data_check_value,
        weechat_php_config_new_option_callback_change,
        (const char *)callback_change_name,
        (const char *)data_change,
        weechat_php_config_new_option_callback_delete,
        (const char *)callback_delete_name,
        (const char *)data_delete);
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

PHP_FUNCTION(weechat_config_search_option)
{
    zend_string *z_config_file, *z_section, *z_option_name;
    struct t_config_file *config_file;
    struct t_config_section *section;
    char *option_name;
    struct t_config_option *retval;

    API_FUNC_INIT(weechat_config_search_option);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SSS", &z_config_file, &z_section,
                               &z_option_name) == FAILURE)
    {
        return;
    }
    config_file = (struct t_config_file *)API_STR2PTR(ZSTR_VAL(z_config_file));
    section = (struct t_config_section *)API_STR2PTR(ZSTR_VAL(z_section));
    option_name = ZSTR_VAL(z_option_name);
    retval = weechat_config_search_option (config_file, section,
                                           (const char *)option_name);
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

PHP_FUNCTION(weechat_config_string_to_boolean)
{
    zend_string *z_text;
    char *text;
    int retval;

    API_FUNC_INIT(weechat_config_string_to_boolean);

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_text) == FAILURE)
    {
        return;
    }
    text = ZSTR_VAL(z_text);
    retval = weechat_config_string_to_boolean ((const char *)text);
    RETURN_LONG(retval);
}

PHP_FUNCTION(weechat_config_option_reset)
{
    zend_string *z_option;
    zend_long z_run_callback;
    struct t_config_option *option;
    int run_callback, retval;

    API_FUNC_INIT(weechat_config_option_reset);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "Sl", &z_option, &z_run_callback) == FAILURE)
    {
        return;
    }
    option = (struct t_config_option *)API_STR2PTR(ZSTR_VAL(z_option));
    run_callback = (int)z_run_callback;
    retval = weechat_config_option_reset (option, run_callback);
    RETURN_LONG(retval);
}

PHP_FUNCTION(weechat_config_option_set)
{
    zend_string *z_option, *z_value;
    zend_long z_run_callback;
    struct t_config_option *option;
    char *value;
    int run_callback, retval;

    API_FUNC_INIT(weechat_config_option_set);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SSl", &z_option, &z_value,
                               &z_run_callback) == FAILURE)
    {
        return;
    }
    option = (struct t_config_option *)API_STR2PTR(ZSTR_VAL(z_option));
    value = ZSTR_VAL(z_value);
    run_callback = (int)z_run_callback;
    retval = weechat_config_option_set (option, (const char *)value,
                                        run_callback);
    RETURN_LONG(retval);
}

PHP_FUNCTION(weechat_config_option_set_null)
{
    zend_string *z_option;
    zend_long z_run_callback;
    struct t_config_option *option;
    int run_callback, retval;

    API_FUNC_INIT(weechat_config_option_set_null);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "Sl", &z_option, &z_run_callback) == FAILURE)
    {
        return;
    }
    option = (struct t_config_option *)API_STR2PTR(ZSTR_VAL(z_option));
    run_callback = (int)z_run_callback;
    retval = weechat_config_option_set_null (option, run_callback);
    RETURN_LONG(retval);
}

PHP_FUNCTION(weechat_config_option_unset)
{
    zend_string *z_option;
    struct t_config_option *option;
    int retval;

    API_FUNC_INIT(weechat_config_option_unset);

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_option) == FAILURE)
    {
        return;
    }
    option = (struct t_config_option *)API_STR2PTR(ZSTR_VAL(z_option));
    retval = weechat_config_option_unset (option);
    RETURN_LONG(retval);
}

PHP_FUNCTION(weechat_config_option_rename)
{
    zend_string *z_option, *z_new_name;
    struct t_config_option *option;
    char *new_name;

    API_FUNC_INIT(weechat_config_option_rename);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SS", &z_option, &z_new_name) == FAILURE)
    {
        return;
    }
    option = (struct t_config_option *)API_STR2PTR(ZSTR_VAL(z_option));
    new_name = ZSTR_VAL(z_new_name);
    weechat_config_option_rename (option, (const char *)new_name);
    RETURN_NULL();
}

PHP_FUNCTION(weechat_config_option_is_null)
{
    zend_string *z_option;
    struct t_config_option *option;
    int retval;

    API_FUNC_INIT(weechat_config_option_is_null);

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_option) == FAILURE)
    {
        return;
    }
    option = (struct t_config_option *)API_STR2PTR(ZSTR_VAL(z_option));
    retval = weechat_config_option_is_null (option);
    RETURN_LONG(retval);
}

PHP_FUNCTION(weechat_config_option_default_is_null)
{
    zend_string *z_option;
    struct t_config_option *option;
    int retval;

    API_FUNC_INIT(weechat_config_option_default_is_null);

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_option) == FAILURE)
    {
        return;
    }
    option = (struct t_config_option *)API_STR2PTR(ZSTR_VAL(z_option));
    retval = weechat_config_option_default_is_null (option);
    RETURN_LONG(retval);
}

PHP_FUNCTION(weechat_config_boolean)
{
    zend_string *z_option;
    struct t_config_option *option;
    int retval;

    API_FUNC_INIT(weechat_config_boolean);

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_option) == FAILURE)
    {
        return;
    }
    option = (struct t_config_option *)API_STR2PTR(ZSTR_VAL(z_option));
    retval = weechat_config_boolean (option);
    RETURN_LONG(retval);
}

PHP_FUNCTION(weechat_config_boolean_default)
{
    zend_string *z_option;
    struct t_config_option *option;
    int retval;

    API_FUNC_INIT(weechat_config_boolean_default);

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_option) == FAILURE)
    {
        return;
    }
    option = (struct t_config_option *)API_STR2PTR(ZSTR_VAL(z_option));
    retval = weechat_config_boolean_default (option);
    RETURN_LONG(retval);
}

PHP_FUNCTION(weechat_config_integer)
{
    zend_string *z_option;
    struct t_config_option *option;
    int retval;

    API_FUNC_INIT(weechat_config_integer);

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_option) == FAILURE)
    {
        return;
    }
    option = (struct t_config_option *)API_STR2PTR(ZSTR_VAL(z_option));
    retval = weechat_config_integer (option);
    RETURN_LONG(retval);
}

PHP_FUNCTION(weechat_config_integer_default)
{
    zend_string *z_option;
    struct t_config_option *option;
    int retval;

    API_FUNC_INIT(weechat_config_integer_default);

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_option) == FAILURE)
    {
        return;
    }
    option = (struct t_config_option *)API_STR2PTR(ZSTR_VAL(z_option));
    retval = weechat_config_integer_default (option);
    RETURN_LONG(retval);
}

PHP_FUNCTION(weechat_config_string)
{
    zend_string *z_option;
    struct t_config_option *option;
    const char *retval;

    API_FUNC_INIT(weechat_config_string);

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_option) == FAILURE)
    {
        return;
    }
    option = (struct t_config_option *)API_STR2PTR(ZSTR_VAL(z_option));
    retval = weechat_config_string (option);
    SAFE_RETURN_STRING(retval);
}

PHP_FUNCTION(weechat_config_string_default)
{
    zend_string *z_option;
    struct t_config_option *option;
    const char *retval;

    API_FUNC_INIT(weechat_config_string_default);

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_option) == FAILURE)
    {
        return;
    }
    option = (struct t_config_option *)API_STR2PTR(ZSTR_VAL(z_option));
    retval = weechat_config_string_default (option);
    SAFE_RETURN_STRING(retval);
}

PHP_FUNCTION(weechat_config_color)
{
    zend_string *z_option;
    struct t_config_option *option;
    const char *retval;

    API_FUNC_INIT(weechat_config_color);

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_option) == FAILURE)
    {
        return;
    }
    option = (struct t_config_option *)API_STR2PTR(ZSTR_VAL(z_option));
    retval = weechat_config_color (option);
    SAFE_RETURN_STRING(retval);
}

PHP_FUNCTION(weechat_config_color_default)
{
    zend_string *z_option;
    struct t_config_option *option;
    const char *retval;

    API_FUNC_INIT(weechat_config_color_default);

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_option) == FAILURE)
    {
        return;
    }
    option = (struct t_config_option *)API_STR2PTR(ZSTR_VAL(z_option));
    retval = weechat_config_color_default (option);
    SAFE_RETURN_STRING(retval);
}

PHP_FUNCTION(weechat_config_write_option)
{
    zend_string *z_config_file, *z_option;
    struct t_config_file *config_file;
    struct t_config_option *option;
    int retval;

    API_FUNC_INIT(weechat_config_write_option);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SS", &z_config_file, &z_option) == FAILURE)
    {
        return;
    }
    config_file = (struct t_config_file *)API_STR2PTR(ZSTR_VAL(z_config_file));
    option = (struct t_config_option *)API_STR2PTR(ZSTR_VAL(z_option));
    retval = weechat_config_write_option (config_file, option);
    RETURN_LONG(retval);
}

PHP_FUNCTION(weechat_config_write_line)
{
    zend_string *z_config_file, *z_option_name, *z_value;
    struct t_config_file *config_file;
    char *option_name, *value;
    int retval;

    API_FUNC_INIT(weechat_config_write_line);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SSS", &z_config_file, &z_option_name,
                               &z_value) == FAILURE)
    {
        return;
    }
    config_file = (struct t_config_file *)API_STR2PTR(ZSTR_VAL(z_config_file));
    option_name = ZSTR_VAL(z_option_name);
    value = ZSTR_VAL(z_value);
    retval = weechat_config_write_line (config_file,
                                        (const char *)option_name,
                                        (const char *)value);
    RETURN_LONG(retval);
}

PHP_FUNCTION(weechat_config_write)
{
    zend_string *z_config_file;
    struct t_config_file *config_file;
    int retval;

    API_FUNC_INIT(weechat_config_write);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "S", &z_config_file) == FAILURE)
    {
        return;
    }
    config_file = (struct t_config_file *)API_STR2PTR(ZSTR_VAL(z_config_file));
    retval = weechat_config_write (config_file);
    RETURN_LONG(retval);
}

PHP_FUNCTION(weechat_config_read)
{
    zend_string *z_config_file;
    struct t_config_file *config_file;
    int retval;

    API_FUNC_INIT(weechat_config_read);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "S", &z_config_file) == FAILURE)
    {
        return;
    }
    config_file = (struct t_config_file *)API_STR2PTR(ZSTR_VAL(z_config_file));
    retval = weechat_config_read (config_file);
    RETURN_LONG(retval);
}

PHP_FUNCTION(weechat_config_reload)
{
    zend_string *z_config_file;
    struct t_config_file *config_file;
    int retval;

    API_FUNC_INIT(weechat_config_reload);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "S", &z_config_file) == FAILURE)
    {
        return;
    }
    config_file = (struct t_config_file *)API_STR2PTR(ZSTR_VAL(z_config_file));
    retval = weechat_config_reload (config_file);
    RETURN_LONG(retval);
}

PHP_FUNCTION(weechat_config_option_free)
{
    zend_string *z_option;
    struct t_config_option *option;

    API_FUNC_INIT(weechat_config_option_free);

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_option) == FAILURE)
    {
        return;
    }
    option = (struct t_config_option *)API_STR2PTR(ZSTR_VAL(z_option));
    weechat_config_option_free (option);
    RETURN_NULL();
}

PHP_FUNCTION(weechat_config_section_free_options)
{
    zend_string *z_section;
    struct t_config_section *section;

    API_FUNC_INIT(weechat_config_section_free_options);

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_section) == FAILURE)
    {
        return;
    }
    section = (struct t_config_section *)API_STR2PTR(ZSTR_VAL(z_section));
    weechat_config_section_free_options (section);
    RETURN_NULL();
}

PHP_FUNCTION(weechat_config_section_free)
{
    zend_string *z_section;
    struct t_config_section *section;

    API_FUNC_INIT(weechat_config_section_free);

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_section) == FAILURE)
    {
        return;
    }
    section = (struct t_config_section *)API_STR2PTR(ZSTR_VAL(z_section));
    weechat_config_section_free (section);
    RETURN_NULL();
}

PHP_FUNCTION(weechat_config_free)
{
    zend_string *z_config_file;
    struct t_config_file *config_file;

    API_FUNC_INIT(weechat_config_free);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "S", &z_config_file) == FAILURE)
    {
        return;
    }
    config_file = (struct t_config_file *)API_STR2PTR(ZSTR_VAL(z_config_file));
    weechat_config_free (config_file);
    RETURN_NULL();
}

PHP_FUNCTION(weechat_config_get)
{
    zend_string *z_option_name;
    char *option_name;
    struct t_config_option *retval;

    API_FUNC_INIT(weechat_config_get);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "S", &z_option_name) == FAILURE)
    {
        return;
    }
    option_name = ZSTR_VAL(z_option_name);
    retval = weechat_config_get ((const char *)option_name);
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

PHP_FUNCTION(weechat_config_get_plugin)
{
    zend_string *z_option;
    char *option;
    const char *retval;

    API_FUNC_INIT(weechat_config_get_plugin);

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_option) == FAILURE)
    {
        return;
    }
    option = ZSTR_VAL(z_option);
    retval = plugin_script_api_config_get_plugin (weechat_php_plugin,
                                                  php_current_script,
                                                  (const char *)option);
    SAFE_RETURN_STRING(retval);
}

PHP_FUNCTION(weechat_config_is_set_plugin)
{
    zend_string *z_option;
    char *option;
    int retval;

    API_FUNC_INIT(weechat_config_is_set_plugin);

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_option) == FAILURE)
    {
        return;
    }
    option = ZSTR_VAL(z_option);
    retval = plugin_script_api_config_is_set_plugin (weechat_php_plugin,
                                                     php_current_script,
                                                     (const char *)option);
    RETURN_LONG(retval);
}

PHP_FUNCTION(weechat_config_set_plugin)
{
    API_FUNC_INIT(weechat_config_set_plugin);
    zend_string *z_option, *z_value;
    char *option, *value;
    int retval;

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SS", &z_option, &z_value) == FAILURE)
    {
        return;
    }
    option = ZSTR_VAL(z_option);
    value = ZSTR_VAL(z_value);
    retval = plugin_script_api_config_set_plugin (weechat_php_plugin,
                                                  php_current_script,
                                                  (const char *)option,
                                                  (const char *)value);
    RETURN_LONG(retval);
}

PHP_FUNCTION(weechat_config_set_desc_plugin)
{
    zend_string *z_option, *z_description;
    char *option, *description;

    API_FUNC_INIT(weechat_config_set_desc_plugin);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SS", &z_option, &z_description) == FAILURE)
    {
        return;
    }
    option = ZSTR_VAL(z_option);
    description = ZSTR_VAL(z_description);
    plugin_script_api_config_set_desc_plugin (weechat_php_plugin,
                                              php_current_script,
                                              (const char *)option,
                                              (const char *)description);
    RETURN_NULL();
}

PHP_FUNCTION(weechat_config_unset_plugin)
{
    zend_string *z_option;
    char *option;
    int retval;

    API_FUNC_INIT(weechat_config_unset_plugin);

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_option) == FAILURE)
    {
        return;
    }
    option = ZSTR_VAL(z_option);
    retval = plugin_script_api_config_unset_plugin (weechat_php_plugin,
                                                    php_current_script,
                                                    (const char *)option);
    RETURN_LONG(retval);
}

PHP_FUNCTION(weechat_key_bind)
{
    zend_string *z_context;
    zval *z_keys;
    char *context;
    struct t_hashtable *keys;
    int retval;

    API_FUNC_INIT(weechat_key_bind);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "Sa", &z_context, &z_keys) == FAILURE)
    {
        return;
    }
    context = ZSTR_VAL(z_context);
    keys = weechat_php_array_to_hashtable (z_keys,
                                           WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                           WEECHAT_HASHTABLE_STRING,
                                           WEECHAT_HASHTABLE_STRING);
    retval = weechat_key_bind ((const char *)context, keys);
    RETURN_LONG(retval);
}

PHP_FUNCTION(weechat_key_unbind)
{
    zend_string *z_context, *z_key;
    int retval;
    char *context, *key;

    API_FUNC_INIT(weechat_key_unbind);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SS", &z_context, &z_key) == FAILURE)
    {
        return;
    }
    context = ZSTR_VAL(z_context);
    key = ZSTR_VAL(z_key);
    retval = weechat_key_unbind ((const char *)context, (const char *)key);
    RETURN_LONG(retval);
}

PHP_FUNCTION(weechat_prefix)
{
    zend_string *z_prefix;
    char *prefix;
    const char *retval;

    API_FUNC_INIT(weechat_prefix);

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_prefix) == FAILURE)
    {
        return;
    }
    prefix = ZSTR_VAL(z_prefix);
    retval = weechat_prefix ((const char *)prefix);
    SAFE_RETURN_STRING(retval);
}

PHP_FUNCTION(weechat_color)
{
    zend_string *z_color_name;
    char *color_name;
    const char *retval;

    API_FUNC_INIT(weechat_color);

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_color_name) == FAILURE)
    {
        return;
    }
    color_name = ZSTR_VAL(z_color_name);
    retval = weechat_color ((const char *)color_name);
    SAFE_RETURN_STRING(retval);
}

PHP_FUNCTION(weechat_printf)
{
    zend_string *z_buffer, *z_format;
    struct t_gui_buffer *buffer;
    char *format;

    API_FUNC_INIT(weechat_printf);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SS", &z_buffer, &z_format) == FAILURE)
    {
        return;
    }
    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));
    format = ZSTR_VAL(z_format);
    plugin_script_api_printf (weechat_php_plugin, php_current_script, buffer,
                              "%s", format);
    RETURN_NULL();
}

PHP_FUNCTION(weechat_printf_date_tags)
{
    zend_string *z_buffer, *z_tags, *z_format;
    zend_long z_date;
    struct t_gui_buffer *buffer;
    time_t date;
    char *tags, *format;

    API_FUNC_INIT(weechat_printf_date_tags);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SlSS", &z_buffer, &z_date, &z_tags,
                               &z_format) == FAILURE)
    {
        return;
    }
    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));
    date = (time_t)z_date;
    tags = ZSTR_VAL(z_tags);
    format = ZSTR_VAL(z_format);
    plugin_script_api_printf_date_tags (weechat_php_plugin,
                                        php_current_script,
                                        buffer,
                                        date,
                                        (const char *)tags,
                                        "%s",
                                        format);
    RETURN_NULL();
}

PHP_FUNCTION(weechat_printf_y)
{
    zend_string *z_buffer, *z_format;
    zend_long z_y;
    struct t_gui_buffer *buffer;
    int y;
    char *format;

    API_FUNC_INIT(weechat_printf_y);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SlS", &z_buffer, &z_y, &z_format) == FAILURE)
    {
        return;
    }
    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));
    y = (int)z_y;
    format = ZSTR_VAL(z_format);
    plugin_script_api_printf_y (weechat_php_plugin,
                                php_current_script,
                                buffer,
                                y,
                                "%s",
                                format);
    RETURN_NULL();
}

PHP_FUNCTION(weechat_log_printf)
{
    zend_string *z_format;
    char *format;

    API_FUNC_INIT(weechat_log_printf);

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_format) == FAILURE)
    {
        return;
    }
    format = ZSTR_VAL(z_format);
    plugin_script_api_log_printf (weechat_php_plugin, php_current_script,
                                  "%s", format);
    RETURN_NULL();
}

static int
weechat_php_hook_command_callback (const void *pointer, void *data,
                                   struct t_gui_buffer *buffer,
                                   int argc, char **argv, char **argv_eol)
{
    int rc, i, *argi;
    void *func_argv[4];
    struct t_hashtable *args;

    /* make C compiler happy */
    (void) argv_eol;

    args = weechat_hashtable_new (argc,
                                  WEECHAT_HASHTABLE_INTEGER,
                                  WEECHAT_HASHTABLE_STRING,
                                  NULL,
                                  NULL);
    argi = malloc (sizeof (int) * argc);

    for (i = 0; i < argc; i++)
    {
        *(argi + i) = i;
        weechat_hashtable_set (args, argi+i, argv[i]);
    }

    func_argv[1] = API_PTR2STR(buffer);
    func_argv[2] = &argc;
    func_argv[3] = args;

    weechat_php_cb (pointer, data, func_argv, "ssih",
                    WEECHAT_SCRIPT_EXEC_INT, &rc);

    free (argi);
    weechat_hashtable_free (args);
    if (func_argv[1])
        free (func_argv[1]);

    return rc;
}

PHP_FUNCTION(weechat_hook_command)
{
    zend_string *z_command, *z_description, *z_args, *z_args_description;
    zend_string *z_completion, *z_data;
    zval *z_callback;
    char *command, *description, *args, *args_description, *completion, *data;
    struct t_hook *retval;

    API_FUNC_INIT(weechat_hook_command);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SSSSSzS", &z_command, &z_description, &z_args,
                               &z_args_description, &z_completion, &z_callback,
                               &z_data) == FAILURE)
    {
        return;
    }
    command = ZSTR_VAL(z_command);
    description = ZSTR_VAL(z_description);
    args = ZSTR_VAL(z_args);
    args_description = ZSTR_VAL(z_args_description);
    completion = ZSTR_VAL(z_completion);
    weechat_php_get_function_name (z_callback, callback_name);
    data = ZSTR_VAL(z_data);
    retval = plugin_script_api_hook_command (weechat_php_plugin,
                                             php_current_script,
                                             (const char *)command,
                                             (const char *)description,
                                             (const char *)args,
                                             (const char *)args_description,
                                             (const char *)completion,
                                             weechat_php_hook_command_callback,
                                             (const char *)callback_name,
                                             (const char *)data);
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

static int
weechat_php_hook_completion_callback (const void *pointer, void *data,
                                      const char *completion_item,
                                      struct t_gui_buffer *buffer,
                                      struct t_gui_completion *completion)
{
    int rc;
    void *func_argv[4];

    func_argv[1] = completion_item ? (char *)completion_item : weechat_php_empty_arg;
    func_argv[2] = API_PTR2STR(buffer);
    func_argv[3] = API_PTR2STR(completion);

    weechat_php_cb (pointer, data, func_argv, "ssss",
                    WEECHAT_SCRIPT_EXEC_INT, &rc);

    if (func_argv[2])
        free (func_argv[2]);
    if (func_argv[3])
        free (func_argv[3]);

    return rc;
}

PHP_FUNCTION(weechat_hook_completion)
{
    zend_string *z_completion, *z_description, *z_data;
    zval *z_callback;
    char *completion, *description, *data;
    struct t_hook *retval;

    API_FUNC_INIT(weechat_hook_completion);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SSzS", &z_completion, &z_description,
                               &z_callback, &z_data) == FAILURE)
    {
        return;
    }
    completion = ZSTR_VAL(z_completion);
    description = ZSTR_VAL(z_description);
    weechat_php_get_function_name (z_callback, callback_name);
    data = ZSTR_VAL(z_data);
    retval = plugin_script_api_hook_completion (weechat_php_plugin,
                                                php_current_script,
                                                (const char *)completion,
                                                (const char *)description,
                                                weechat_php_hook_completion_callback,
                                                (const char *)callback_name,
                                                (const char *)data);
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

PHP_FUNCTION(weechat_hook_completion_get_string)
{
    zend_string *z_completion, *z_property;
    struct t_gui_completion *completion;
    char *property;
    const char *retval;

    API_FUNC_INIT(weechat_hook_completion_get_string);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SS", &z_completion, &z_property) == FAILURE)
    {
        return;
    }
    completion = (struct t_gui_completion *)API_STR2PTR(ZSTR_VAL(z_completion));
    property = ZSTR_VAL(z_property);
    retval = weechat_hook_completion_get_string (completion,
                                                 (const char *)property);
    SAFE_RETURN_STRING(retval);
}

PHP_FUNCTION(weechat_hook_completion_list_add)
{
    zend_string *z_completion, *z_word, *z_where;
    zend_long z_nick_completion;
    struct t_gui_completion *completion;
    char *word, *where;
    int nick_completion;

    API_FUNC_INIT(weechat_hook_completion_list_add);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SSlS", &z_completion, &z_word,
                               &z_nick_completion, &z_where) == FAILURE)
    {
        return;
    }
    completion = (struct t_gui_completion *)API_STR2PTR(ZSTR_VAL(z_completion));
    word = ZSTR_VAL(z_word);
    nick_completion = (int)z_nick_completion;
    where = ZSTR_VAL(z_where);
    weechat_hook_completion_list_add (completion,
                                      (const char *)word,
                                      nick_completion,
                                      (const char *)where);
    RETURN_NULL();
}

static int
weechat_php_hook_command_run_callback (const void *pointer, void *data,
                                       struct t_gui_buffer *buffer,
                                       const char *command)
{
    int rc;
    void *func_argv[3];

    func_argv[1] = API_PTR2STR(buffer);
    func_argv[2] = command ? (char *)command : weechat_php_empty_arg;

    weechat_php_cb (pointer, data, func_argv, "sss",
                    WEECHAT_SCRIPT_EXEC_INT, &rc);

    if (func_argv[1])
        free (func_argv[1]);

    return rc;
}

PHP_FUNCTION(weechat_hook_command_run)
{
    zend_string *z_command, *z_data;
    zval *z_callback;
    char *command, *data;
    struct t_hook *retval;

    API_FUNC_INIT(weechat_hook_command_run);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SzS", &z_command, &z_callback,
                               &z_data) == FAILURE)
    {
        return;
    }
    command = ZSTR_VAL(z_command);
    weechat_php_get_function_name (z_callback, callback_name);
    data = ZSTR_VAL(z_data);
    retval = plugin_script_api_hook_command_run (weechat_php_plugin,
                                                 php_current_script,
                                                 (const char *)command,
                                                 weechat_php_hook_command_run_callback,
                                                 (const char *)callback_name,
                                                 (const char *)data);
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

static int weechat_php_hook_timer_callback (const void *pointer, void *data,
                                            int remaining_calls)
{
    int rc;
    void *func_argv[2];

    func_argv[1] = &remaining_calls;

    weechat_php_cb (pointer, data, func_argv, "si",
                    WEECHAT_SCRIPT_EXEC_INT, &rc);

    return rc;
}

PHP_FUNCTION(weechat_hook_timer)
{
    zend_long z_interval, z_align_second, z_max_calls;
    zval *z_callback;
    zend_string *z_data;
    int interval, align_second, max_calls;
    char *data;
    struct t_hook *retval;

    API_FUNC_INIT(weechat_hook_timer);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "lllzS", &z_interval, &z_align_second,
                               &z_max_calls, &z_callback, &z_data) == FAILURE)
    {
        return;
    }
    interval = (int)z_interval;
    align_second = (int)z_align_second;
    max_calls = (int)z_max_calls;
    weechat_php_get_function_name (z_callback, callback_name);
    data = ZSTR_VAL(z_data);
    retval = plugin_script_api_hook_timer (weechat_php_plugin,
                                           php_current_script,
                                           interval,
                                           align_second,
                                           max_calls,
                                           weechat_php_hook_timer_callback,
                                           (const char *)callback_name,
                                           (const char *)data);
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

static int weechat_php_hook_fd_callback (const void *pointer, void *data,
                                         int fd)
{
    int rc;
    void *func_argv[2];

    func_argv[1] = &fd;

    weechat_php_cb (pointer, data, func_argv, "si",
                    WEECHAT_SCRIPT_EXEC_INT, &rc);

    return rc;
}

PHP_FUNCTION(weechat_hook_fd)
{
    zend_long z_fd, z_flag_read, z_flag_write, z_flag_exception;
    zval *z_callback;
    zend_string *z_data;
    int fd, flag_read, flag_write, flag_exception;
    char *data;
    struct t_hook *retval;

    API_FUNC_INIT(weechat_hook_fd);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "llllzS", &z_fd, &z_flag_read, &z_flag_write,
                               &z_flag_exception, &z_callback,
                               &z_data) == FAILURE)
    {
        return;
    }
    fd = (int)z_fd;
    flag_read = (int)z_flag_read;
    flag_write = (int)z_flag_write;
    flag_exception = (int)z_flag_exception;
    weechat_php_get_function_name (z_callback, callback_name);
    data = ZSTR_VAL(z_data);
    retval = plugin_script_api_hook_fd (weechat_php_plugin,
                                        php_current_script, fd,
                                        flag_read,
                                        flag_write,
                                        flag_exception,
                                        weechat_php_hook_fd_callback,
                                        (const char *)callback_name,
                                        (const char *)data);
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

static int
weechat_php_hook_process_callback (const void *pointer, void *data,
                                   const char *command, int return_code,
                                   const char *out, const char *err)
{
    int rc;
    void *func_argv[5];

    func_argv[1] = command ? (char *)command : weechat_php_empty_arg;
    func_argv[2] = &return_code;
    func_argv[3] = out ? (char *)out : weechat_php_empty_arg;
    func_argv[4] = err ? (char *)err : weechat_php_empty_arg;

    weechat_php_cb (pointer, data, func_argv, "ssiss",
                    WEECHAT_SCRIPT_EXEC_INT, &rc);

    return rc;
}

PHP_FUNCTION(weechat_hook_process)
{
    zend_string *z_command, *z_data;
    zend_long z_timeout;
    zval *z_callback;
    char *command, *data;
    int timeout;
    struct t_hook *retval;

    API_FUNC_INIT(weechat_hook_process);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SlzS", &z_command, &z_timeout, &z_callback,
                               &z_data) == FAILURE)
    {
        return;
    }
    command = ZSTR_VAL(z_command);
    timeout = (int)z_timeout;
    weechat_php_get_function_name (z_callback, callback_name);
    data = ZSTR_VAL(z_data);
    retval = plugin_script_api_hook_process (weechat_php_plugin,
                                             php_current_script,
                                             (const char *)command,
                                             timeout,
                                             weechat_php_hook_process_callback,
                                             (const char *)callback_name,
                                             (const char *)data);
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

static int
weechat_php_hook_process_hashtable_callback (const void *pointer, void *data,
                                             const char *command,
                                             int return_code,
                                             const char *out, const char *err)
{
    int rc;
    void *func_argv[5];

    func_argv[1] = command ? (char *)command : weechat_php_empty_arg;
    func_argv[2] = &return_code;
    func_argv[3] = out ? (char *)out : weechat_php_empty_arg;
    func_argv[4] = err ? (char *)err : weechat_php_empty_arg;

    weechat_php_cb (pointer, data, func_argv, "ssiss",
                    WEECHAT_SCRIPT_EXEC_INT, &rc);

    return rc;
}

PHP_FUNCTION(weechat_hook_process_hashtable)
{
    zend_string *z_command, *z_data;
    zval *z_options, *z_callback;
    zend_long z_timeout;
    char *command, *data;
    struct t_hashtable *options;
    int timeout;
    struct t_hook *retval;

    API_FUNC_INIT(weechat_hook_process_hashtable);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SalzS", &z_command, &z_options, &z_timeout,
                               &z_callback, &z_data) == FAILURE)
    {
        return;
    }
    command = ZSTR_VAL(z_command);
    options = weechat_php_array_to_hashtable (
        z_options,
        WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING);
    timeout = (int)z_timeout;
    weechat_php_get_function_name (z_callback, callback_name);
    data = ZSTR_VAL(z_data);
    retval = plugin_script_api_hook_process_hashtable (weechat_php_plugin,
                                                       php_current_script,
                                                       (const char *)command,
                                                       options,
                                                       timeout,
                                                       weechat_php_hook_process_hashtable_callback,
                                                       (const char *)callback_name,
                                                       (const char *)data);
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

static int
weechat_php_hook_connect_callback (const void *pointer, void *data, int status,
                                   int gnutls_rc, int sock, const char *error,
                                   const char *ip_address)
{
    int rc;
    void *func_argv[6];

    func_argv[1] = &status;
    func_argv[2] = &gnutls_rc;
    func_argv[3] = &sock;
    func_argv[4] = error ? (char *)error : weechat_php_empty_arg;
    func_argv[5] = ip_address ? (char *)ip_address : weechat_php_empty_arg;

    weechat_php_cb (pointer, data, func_argv, "siiiss",
                    WEECHAT_SCRIPT_EXEC_INT, &rc);

    return rc;
}

PHP_FUNCTION(weechat_hook_connect)
{
    zend_string *z_proxy, *z_address, *z_gnutls_sess, *z_gnutls_cb;
    zend_string *z_gnutls_priorities, *z_local_hostname, *z_data;
    zend_long z_port, z_ipv6, z_retry, z_gnutls_dhkey_size;
    zval *z_callback;
    char *proxy, *address, *gnutls_priorities, *local_hostname, *data;
    int port, ipv6, retry;
    void *gnutls_sess, *gnutls_cb;
    int gnutls_dhkey_size;
    struct t_hook *retval;

    API_FUNC_INIT(weechat_hook_connect);

    if (zend_parse_parameters (
            ZEND_NUM_ARGS(), "SSlllSSlSSzS", &z_proxy, &z_address, &z_port,
            &z_ipv6, &z_retry, &z_gnutls_sess, &z_gnutls_cb,
            &z_gnutls_dhkey_size, &z_gnutls_priorities, &z_local_hostname,
            &z_callback, &z_data) == FAILURE)
    {
        return;
    }
    proxy = ZSTR_VAL(z_proxy);
    address = ZSTR_VAL(z_address);
    port = (int)z_port;
    ipv6 = (int)z_ipv6;
    retry = (int)z_retry;
    gnutls_sess = (void *)API_STR2PTR(ZSTR_VAL(z_gnutls_sess));
    gnutls_cb = (void *)API_STR2PTR(ZSTR_VAL(z_gnutls_cb));
    gnutls_dhkey_size = (int)z_gnutls_dhkey_size;
    gnutls_priorities = ZSTR_VAL(z_gnutls_priorities);
    local_hostname = ZSTR_VAL(z_local_hostname);
    weechat_php_get_function_name (z_callback, callback_name);
    data = ZSTR_VAL(z_data);
    retval = plugin_script_api_hook_connect (weechat_php_plugin,
                                             php_current_script,
                                             (const char *)proxy,
                                             (const char *)address,
                                             port,
                                             ipv6,
                                             retry,
                                             gnutls_sess,
                                             gnutls_cb,
                                             gnutls_dhkey_size,
                                             (const char *)gnutls_priorities,
                                             (const char *)local_hostname,
                                             weechat_php_hook_connect_callback,
                                             (const char *)callback_name,
                                             (const char *)data);
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

static int
weechat_php_hook_print_callback (const void *pointer, void *data,
                                 struct t_gui_buffer *buffer, time_t date,
                                 int tags_count, const char **tags,
                                 int displayed, int highlight,
                                 const char *prefix, const char *message)
{
    int rc;
    void *func_argv[9];

    func_argv[1] = API_PTR2STR(buffer);
    func_argv[2] = &date;
    func_argv[3] = &tags_count;
    func_argv[4] = tags ? (char *)tags : weechat_php_empty_arg;
    func_argv[5] = &displayed;
    func_argv[6] = &highlight;
    func_argv[7] = prefix ? (char *)prefix : weechat_php_empty_arg;
    func_argv[8] = message ? (char *)message : weechat_php_empty_arg;

    weechat_php_cb (pointer, data, func_argv, "ssiisiiss",
                    WEECHAT_SCRIPT_EXEC_INT, &rc);

    if (func_argv[1])
        free (func_argv[1]);

    return rc;
}

PHP_FUNCTION(weechat_hook_print)
{
    zend_string *z_buffer, *z_tags, *z_message, *z_data;
    zend_long z_strip_colors;
    zval *z_callback;
    struct t_gui_buffer *buffer;
    char *tags, *message, *data;
    int strip_colors;
    struct t_hook *retval;

    API_FUNC_INIT(weechat_hook_print);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SSSlzS", &z_buffer, &z_tags, &z_message,
                               &z_strip_colors, &z_callback,
                               &z_data) == FAILURE)
    {
        return;
    }
    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));
    tags = ZSTR_VAL(z_tags);
    message = ZSTR_VAL(z_message);
    strip_colors = (int)z_strip_colors;
    weechat_php_get_function_name (z_callback, callback_name);
    data = ZSTR_VAL(z_data);
    retval = plugin_script_api_hook_print (weechat_php_plugin,
                                           php_current_script, buffer,
                                           (const char *)tags,
                                           (const char *)message,
                                           strip_colors,
                                           weechat_php_hook_print_callback,
                                           (const char *)callback_name,
                                           (const char *)data);
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

static int
weechat_php_hook_signal_callback (const void *pointer, void *data,
                                  const char *signal, const char *type_data,
                                  void *signal_data)
{
    int rc;
    void *func_argv[4];

    func_argv[1] = signal ? (char *)signal : weechat_php_empty_arg;
    func_argv[2] = type_data ? (char *)type_data : weechat_php_empty_arg;
    func_argv[3] = signal_data ? (char *)signal_data : weechat_php_empty_arg;

    weechat_php_cb (pointer, data, func_argv, "ssss",
                    WEECHAT_SCRIPT_EXEC_INT, &rc);

    return rc;
}

PHP_FUNCTION(weechat_hook_signal)
{
    zend_string *z_signal, *z_data;
    zval *z_callback;
    char *signal, *data;
    struct t_hook *retval;

    API_FUNC_INIT(weechat_hook_signal);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SzS", &z_signal, &z_callback,
                               &z_data) == FAILURE)
    {
        return;
    }
    signal = ZSTR_VAL(z_signal);
    weechat_php_get_function_name (z_callback, callback_name);
    data = ZSTR_VAL(z_data);
    retval = plugin_script_api_hook_signal (weechat_php_plugin,
                                            php_current_script,
                                            (const char *)signal,
                                            weechat_php_hook_signal_callback,
                                            (const char *)callback_name,
                                            (const char *)data);
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

PHP_FUNCTION(weechat_hook_signal_send)
{
    zend_string *z_signal, *z_type_data, *z_signal_data;
    char *signal, *type_data;
    void *signal_data;
    int retval;

    API_FUNC_INIT(weechat_hook_signal_send);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SSS", &z_signal, &z_type_data,
                               &z_signal_data) == FAILURE)
    {
        return;
    }
    signal = ZSTR_VAL(z_signal);
    type_data = ZSTR_VAL(z_type_data);
    signal_data = (void *)API_STR2PTR(ZSTR_VAL(z_signal_data));
    retval = weechat_hook_signal_send ((const char *)signal,
                                       (const char *)type_data, signal_data);
    RETURN_LONG(retval);
}

static int weechat_php_hook_hsignal_callback (const void *pointer, void *data,
                                              const char *signal,
                                              struct t_hashtable *hashtable)
{
    int rc;
    void *func_argv[3];

    func_argv[1] = signal ? (char *)signal : weechat_php_empty_arg;
    func_argv[2] = hashtable;

    weechat_php_cb (pointer, data, func_argv, "ssh",
                    WEECHAT_SCRIPT_EXEC_INT, &rc);

    return rc;
}

PHP_FUNCTION(weechat_hook_hsignal)
{
    zend_string *z_signal, *z_data;
    zval *z_callback;
    struct t_hook *retval;
    char *signal, *data;

    API_FUNC_INIT(weechat_hook_hsignal);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SzS", &z_signal, &z_callback,
                               &z_data) == FAILURE)
    {
        return;
    }
    signal = ZSTR_VAL(z_signal);
    weechat_php_get_function_name (z_callback, callback_name);
    data = ZSTR_VAL(z_data);
    retval = plugin_script_api_hook_hsignal (weechat_php_plugin,
                                             php_current_script,
                                             (const char *)signal,
                                             weechat_php_hook_hsignal_callback,
                                             (const char *)callback_name,
                                             (const char *)data);
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

PHP_FUNCTION(weechat_hook_hsignal_send)
{
    zend_string *z_signal;
    zval *z_hashtable;
    char *signal;
    struct t_hashtable *hashtable;
    int retval;

    API_FUNC_INIT(weechat_hook_hsignal_send);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "Sa", &z_signal, &z_hashtable) == FAILURE)
    {
        return;
    }
    signal = ZSTR_VAL(z_signal);
    hashtable = weechat_php_array_to_hashtable (
        z_hashtable,
        WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING);
    retval = weechat_hook_hsignal_send ((const char *)signal, hashtable);
    RETURN_LONG(retval);
}

static int
weechat_php_hook_config_callback (const void *pointer, void *data,
                                  const char *option, const char *value)
{
    int rc;
    void *func_argv[3];

    func_argv[1] = option ? (char *)option : weechat_php_empty_arg;
    func_argv[2] = value ? (char *)value : weechat_php_empty_arg;

    weechat_php_cb (pointer, data, func_argv, "sss",
                    WEECHAT_SCRIPT_EXEC_INT, &rc);

    return rc;
}

PHP_FUNCTION(weechat_hook_config)
{
    zend_string *z_option, *z_data;
    zval *z_callback;
    char *option, *data;
    struct t_hook *retval;

    API_FUNC_INIT(weechat_hook_config);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SzS", &z_option, &z_callback,
                               &z_data) == FAILURE)
    {
        return;
    }
    option = ZSTR_VAL(z_option);
    weechat_php_get_function_name (z_callback, callback_name);
    data = ZSTR_VAL(z_data);
    retval = plugin_script_api_hook_config (weechat_php_plugin,
                                            php_current_script,
                                            (const char *)option,
                                            weechat_php_hook_config_callback,
                                            (const char *)callback_name,
                                            (const char *)data);
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

static char *
weechat_php_hook_modifier_callback (const void *pointer, void *data,
                                    const char *modifier,
                                    const char *modifier_data,
                                    const char *string)
{
    char *rc;
    void *func_argv[4];

    func_argv[1] = modifier ? (char *)modifier : weechat_php_empty_arg;
    func_argv[2] = modifier_data ? (char *)modifier_data : weechat_php_empty_arg;
    func_argv[3] = string ? (char *)string : weechat_php_empty_arg;

    weechat_php_cb (pointer, data, func_argv, "ssss",
                    WEECHAT_SCRIPT_EXEC_STRING, &rc);

    return rc;
}

PHP_FUNCTION(weechat_hook_modifier)
{
    zend_string *z_modifier, *z_data;
    zval *z_callback;
    char *modifier, *data;
    struct t_hook *retval;

    API_FUNC_INIT(weechat_hook_modifier);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SzS", &z_modifier, &z_callback,
                               &z_data) == FAILURE)
    {
        return;
    }
    modifier = ZSTR_VAL(z_modifier);
    weechat_php_get_function_name (z_callback, callback_name);
    data = ZSTR_VAL(z_data);
    retval = plugin_script_api_hook_modifier (weechat_php_plugin,
                                              php_current_script,
                                              (const char *)modifier,
                                              weechat_php_hook_modifier_callback,
                                              (const char *)callback_name,
                                              (const char *)data);
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

PHP_FUNCTION(weechat_hook_modifier_exec)
{
    zend_string *z_modifier, *z_modifier_data, *z_string;
    char *modifier, *modifier_data, *string, *retval;

    API_FUNC_INIT(weechat_hook_modifier_exec);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SSS", &z_modifier, &z_modifier_data,
                               &z_string) == FAILURE)
    {
        return;
    }
    modifier = ZSTR_VAL(z_modifier);
    modifier_data = ZSTR_VAL(z_modifier_data);
    string = ZSTR_VAL(z_string);
    retval = weechat_hook_modifier_exec ((const char *)modifier,
                                         (const char *)modifier_data,
                                         (const char *)string);
    SAFE_RETURN_STRING(retval);
}

static const char *weechat_php_hook_info_callback (const void *pointer,
                                                   void *data,
                                                   const char *info_name,
                                                   const char *arguments)
{
    char *rc;
    void *func_argv[3];

    func_argv[1] = info_name ? (char *)info_name : weechat_php_empty_arg;
    func_argv[2] = arguments ? (char *)arguments : weechat_php_empty_arg;

    weechat_php_cb (pointer, data, func_argv, "sss",
                    WEECHAT_SCRIPT_EXEC_STRING, &rc);

    return rc;
}

PHP_FUNCTION(weechat_hook_info)
{
    zend_string *z_info_name, *z_description, *z_args_description, *z_data;
    zval *z_callback;
    char *info_name, *description, *args_description, *data;
    struct t_hook *retval;

    API_FUNC_INIT(weechat_hook_info);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SSSzS", &z_info_name, &z_description,
                               &z_args_description, &z_callback,
                               &z_data) == FAILURE)
    {
        return;
    }
    info_name = ZSTR_VAL(z_info_name);
    description = ZSTR_VAL(z_description);
    args_description = ZSTR_VAL(z_args_description);
    weechat_php_get_function_name (z_callback, callback_name);
    data = ZSTR_VAL(z_data);
    retval = plugin_script_api_hook_info (weechat_php_plugin,
                                          php_current_script,
                                          (const char *)info_name,
                                          (const char *)description,
                                          (const char *)args_description,
                                          weechat_php_hook_info_callback,
                                          (const char *)callback_name,
                                          (const char *)data);
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

struct t_hashtable *
weechat_php_api_hook_info_hashtable_callback (const void *pointer, void *data,
                                              const char *info_name,
                                              struct t_hashtable *hashtable)
{
    struct t_hashtable *rc;
    void *func_argv[3];

    func_argv[1] = (info_name) ? (char *)info_name : weechat_php_empty_arg;
    func_argv[2] = hashtable;

    weechat_php_cb (pointer, data, func_argv, "ssh",
                    WEECHAT_SCRIPT_EXEC_HASHTABLE, &rc);

    return rc;
}

PHP_FUNCTION(weechat_hook_info_hashtable)
{
    zend_string *z_info_name, *z_description, *z_args_description;
    zend_string *z_output_description, *z_data;
    zval *z_callback;
    char *info_name, *description, *args_description, *output_description;
    char *data;
    struct t_hook *retval;

    API_FUNC_INIT(weechat_hook_info_hashtable);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SSSSSzS", &z_info_name, &z_description,
                               &z_args_description, &z_output_description,
                               &z_callback, &z_data) == FAILURE)
    {
        return;
    }
    info_name = ZSTR_VAL(z_info_name);
    description = ZSTR_VAL(z_description);
    args_description = ZSTR_VAL(z_args_description);
    output_description = ZSTR_VAL(z_output_description);
    data = ZSTR_VAL(z_data);
    weechat_php_get_function_name (z_callback, callback_name);
    retval = plugin_script_api_hook_info_hashtable (weechat_php_plugin,
                                                    php_current_script,
                                                    info_name,
                                                    description,
                                                    args_description,
                                                    output_description,
                                                    weechat_php_api_hook_info_hashtable_callback,
                                                    callback_name,
                                                    data);
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

struct t_infolist *
weechat_php_api_hook_infolist_callback (const void *pointer, void *data,
                                        const char *info_name,
                                        void *obj_pointer,
                                        const char *arguments)
{
    struct t_infolist *rc;
    void *func_argv[4];

    func_argv[1] = (info_name) ? (char *)info_name : weechat_php_empty_arg;
    func_argv[2] = API_PTR2STR(obj_pointer);
    func_argv[3] = (arguments) ? (char *)arguments : weechat_php_empty_arg;

    weechat_php_cb (pointer, data, func_argv, "ssss",
                    WEECHAT_SCRIPT_EXEC_STRING, &rc);

    if (func_argv[2])
        free (func_argv[2]);

    return rc;
}

PHP_FUNCTION(weechat_hook_infolist)
{
    zend_string *z_infolist_name, *z_description, *z_pointer_description;
    zend_string *z_args_description, *z_data;
    zval *z_callback;
    char *infolist_name, *description, *pointer_description, *args_description;
    char *data;
    struct t_hook *retval;

    API_FUNC_INIT(weechat_hook_infolist);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SSSSzS", &z_infolist_name, &z_description,
                               &z_pointer_description, &z_args_description,
                               &z_callback, &z_data) == FAILURE)
    {
        return;
    }
    infolist_name = ZSTR_VAL(z_infolist_name);
    description = ZSTR_VAL(z_description);
    pointer_description = ZSTR_VAL(z_pointer_description);
    args_description = ZSTR_VAL(z_args_description);
    data = ZSTR_VAL(z_data);
    weechat_php_get_function_name (z_callback, callback_name);
    retval = plugin_script_api_hook_infolist (weechat_php_plugin,
                                              php_current_script,
                                              infolist_name,
                                              description,
                                              pointer_description,
                                              args_description,
                                              weechat_php_api_hook_infolist_callback,
                                              callback_name,
                                              data);
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

struct t_hashtable *
weechat_php_api_hook_focus_callback (const void *pointer, void *data,
                                     struct t_hashtable *info)
{
    struct t_hashtable *rc;
    void *func_argv[2];

    func_argv[1] = info;

    weechat_php_cb (pointer, data, func_argv, "sh",
                    WEECHAT_SCRIPT_EXEC_HASHTABLE, &rc);

    return rc;
}

PHP_FUNCTION(weechat_hook_focus)
{
    zend_string *z_area, *z_data;
    zval *z_callback;
    char *area, *data;
    struct t_hook *retval;

    API_FUNC_INIT(weechat_hook_focus);

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SzS", &z_area, &z_callback,
                               &z_data) == FAILURE)
    {
        return;
    }
    area = ZSTR_VAL(z_area);
    data = ZSTR_VAL(z_data);
    weechat_php_get_function_name (z_callback, callback_name);
    retval = plugin_script_api_hook_focus (weechat_php_plugin,
                                           php_current_script,
                                           area,
                                           weechat_php_api_hook_focus_callback,
                                           callback_name,
                                           data);
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

PHP_FUNCTION(weechat_hook_set)
{
    zend_string *z_hook, *z_property, *z_value;
    struct t_hook *hook;
    char *property, *value;

    API_FUNC_INIT(weechat_hook_set);

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SSS", &z_hook, &z_property,
                               &z_value) == FAILURE)
    {
        return;
    }
    hook = (struct t_hook *)API_STR2PTR(ZSTR_VAL(z_hook));
    property = ZSTR_VAL(z_property);
    value = ZSTR_VAL(z_value);
    weechat_hook_set (hook, (const char *)property, (const char *)value);
    RETURN_NULL();
}

PHP_FUNCTION(weechat_unhook)
{
    zend_string *z_hook;
    struct t_hook *hook;

    API_FUNC_INIT(weechat_unhook);

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_hook) == FAILURE)
    {
        return;
    }
    hook = (struct t_hook *)API_STR2PTR(ZSTR_VAL(z_hook));
    weechat_unhook (hook);
    RETURN_NULL();
}

PHP_FUNCTION(weechat_unhook_all)
{
    zend_string *z_subplugin;
    char *subplugin;

    API_FUNC_INIT(weechat_unhook_all);

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_subplugin) == FAILURE)
    {
        return;
    }
    subplugin = ZSTR_VAL(z_subplugin);
    weechat_unhook_all ((const char *)subplugin);
    RETURN_NULL();
}

int weechat_php_buffer_new_input_callback (const void *pointer, void *data,
                                           struct t_gui_buffer *buffer,
                                           const char *input_data)
{
    int rc;
    void *func_argv[3];

    func_argv[1] = API_PTR2STR(buffer);
    func_argv[2] = input_data ? (char *)input_data : weechat_php_empty_arg;

    weechat_php_cb (pointer, data, func_argv, "sss",
                    WEECHAT_SCRIPT_EXEC_INT, &rc);

    if (func_argv[1])
        free (func_argv[1]);

    return rc;
}

int weechat_php_buffer_new_close_callback (const void *pointer, void *data,
                                           struct t_gui_buffer *buffer)
{
    int rc;
    void *func_argv[2];

    func_argv[1] = API_PTR2STR(buffer);

    weechat_php_cb (pointer, data, func_argv, "ss",
                    WEECHAT_SCRIPT_EXEC_INT, &rc);

    if (func_argv[1])
        free (func_argv[1]);

    return rc;
}

PHP_FUNCTION(weechat_buffer_new)
{
    zend_string *z_name, *z_data_input, *z_data_close;
    zval *z_input_callback, *z_close_callback;
    char *name, *data_input, *data_close;
    struct t_gui_buffer *retval;

    API_FUNC_INIT(weechat_buffer_new);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SzSzS", &z_name, &z_input_callback,
                               &z_data_input, &z_close_callback,
                               &z_data_close) == FAILURE)
    {
        return;
    }
    name = ZSTR_VAL(z_name);
    weechat_php_get_function_name (z_input_callback, input_callback_name);
    data_input = ZSTR_VAL(z_data_input);
    weechat_php_get_function_name (z_close_callback, close_callback_name);
    data_close = ZSTR_VAL(z_data_close);
    retval = plugin_script_api_buffer_new (weechat_php_plugin,
                                           php_current_script,
                                           (const char *)name,
                                           weechat_php_buffer_new_input_callback,
                                           (const char *)input_callback_name,
                                           (const char *)data_input,
                                           weechat_php_buffer_new_close_callback,
                                           (const char *)close_callback_name,
                                           (const char *)data_close);
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

PHP_FUNCTION(weechat_buffer_search)
{
    zend_string *z_plugin, *z_name;
    char *plugin, *name;
    struct t_gui_buffer *retval;

    API_FUNC_INIT(weechat_buffer_search);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SS", &z_plugin, &z_name) == FAILURE)
    {
        return;
    }
    plugin = ZSTR_VAL(z_plugin);
    name = ZSTR_VAL(z_name);
    retval = weechat_buffer_search ((const char *)plugin, (const char *)name);
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

PHP_FUNCTION(weechat_buffer_search_main)
{
    struct t_gui_buffer *retval;

    API_FUNC_INIT(weechat_buffer_search_main);

    if (zend_parse_parameters_none () == FAILURE)
    {
        return;
    }
    retval = weechat_buffer_search_main ();
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

PHP_FUNCTION(weechat_current_buffer)
{
    struct t_gui_buffer *retval;

    API_FUNC_INIT(weechat_current_buffer);

    if (zend_parse_parameters_none () == FAILURE)
    {
        return;
    }
    retval = weechat_current_buffer ();
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

PHP_FUNCTION(weechat_buffer_clear)
{
    zend_string *z_buffer;
    struct t_gui_buffer *buffer;

    API_FUNC_INIT(weechat_buffer_clear);

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_buffer) == FAILURE)
    {
        return;
    }
    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));
    weechat_buffer_clear (buffer);
    RETURN_NULL();
}

PHP_FUNCTION(weechat_buffer_close)
{
    zend_string *z_buffer;
    struct t_gui_buffer *buffer;

    API_FUNC_INIT(weechat_buffer_close);

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_buffer) == FAILURE)
    {
        return;
    }
    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));
    weechat_buffer_close (buffer);
    RETURN_NULL();
}

PHP_FUNCTION(weechat_buffer_merge)
{
    zend_string *z_buffer;
    zend_string *z_target_buffer;
    struct t_gui_buffer *buffer;
    struct t_gui_buffer *target_buffer;

    API_FUNC_INIT(weechat_buffer_merge);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SS", &z_buffer, &z_target_buffer) == FAILURE)
    {
        return;
    }
    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));
    target_buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_target_buffer));
    weechat_buffer_merge (buffer, target_buffer);
    RETURN_NULL();
}

PHP_FUNCTION(weechat_buffer_unmerge)
{
    zend_string *z_buffer;
    zend_long z_number;
    struct t_gui_buffer *buffer;

    API_FUNC_INIT(weechat_buffer_unmerge);

    int number;
    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "Sl", &z_buffer, &z_number) == FAILURE)
    {
        return;
    }
    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));
    number = (int)z_number;
    weechat_buffer_unmerge (buffer, number);
    RETURN_NULL();
}

PHP_FUNCTION(weechat_buffer_get_integer)
{
    zend_string *z_buffer, *z_property;
    struct t_gui_buffer *buffer;
    char *property;
    int retval;

    API_FUNC_INIT(weechat_buffer_get_integer);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SS", &z_buffer, &z_property) == FAILURE)
    {
        return;
    }
    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));
    property = ZSTR_VAL(z_property);
    retval = weechat_buffer_get_integer (buffer, (const char *)property);
    RETURN_LONG(retval);
}

PHP_FUNCTION(weechat_buffer_get_string)
{
    zend_string *z_buffer, *z_property;
    const char *retval;
    struct t_gui_buffer *buffer;
    char *property;

    API_FUNC_INIT(weechat_buffer_get_string);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SS", &z_buffer, &z_property) == FAILURE)
    {
        return;
    }
    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));
    property = ZSTR_VAL(z_property);
    retval = weechat_buffer_get_string (buffer, (const char *)property);
    SAFE_RETURN_STRING(retval);
}

PHP_FUNCTION(weechat_buffer_get_pointer)
{
    zend_string *z_buffer, *z_property;
    void *retval;
    struct t_gui_buffer *buffer;
    char *property;

    API_FUNC_INIT(weechat_buffer_get_pointer);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SS", &z_buffer, &z_property) == FAILURE)
    {
        return;
    }
    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));
    property = ZSTR_VAL(z_property);
    retval = weechat_buffer_get_pointer (buffer, (const char *)property);
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

PHP_FUNCTION(weechat_buffer_set)
{
    zend_string *z_buffer, *z_property, *z_value;
    struct t_gui_buffer *buffer;
    char *property, *value;

    API_FUNC_INIT(weechat_buffer_set);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SSS", &z_buffer, &z_property,
                               &z_value) == FAILURE)
    {
        return;
    }
    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));
    property = ZSTR_VAL(z_property);
    value = ZSTR_VAL(z_value);
    weechat_buffer_set (buffer, (const char *)property, (const char *)value);
    RETURN_NULL();
}

PHP_FUNCTION(weechat_buffer_string_replace_local_var)
{
    zend_string *z_buffer, *z_string;
    struct t_gui_buffer *buffer;
    char *string, *retval;

    API_FUNC_INIT(weechat_buffer_string_replace_local_var);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SS", &z_buffer, &z_string) == FAILURE)
    {
        return;
    }
    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));
    string = ZSTR_VAL(z_string);
    retval = weechat_buffer_string_replace_local_var (buffer,
                                                      (const char *)string);
    SAFE_RETURN_STRING(retval);
}

PHP_FUNCTION(weechat_buffer_match_list)
{
    zend_string *z_buffer, *z_string;
    struct t_gui_buffer *buffer;
    char *string;
    int retval;

    API_FUNC_INIT(weechat_buffer_match_list);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SS", &z_buffer, &z_string) == FAILURE)
    {
        return;
    }
    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));
    string = ZSTR_VAL(z_string);
    retval = weechat_buffer_match_list (buffer, (const char *)string);
    RETURN_LONG(retval);
}

PHP_FUNCTION(weechat_current_window)
{
    struct t_gui_window *retval;

    API_FUNC_INIT(weechat_current_window);

    if (zend_parse_parameters_none () == FAILURE)
    {
        return;
    }
    retval = weechat_current_window ();
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

PHP_FUNCTION(weechat_window_search_with_buffer)
{
    zend_string *z_buffer;
    struct t_gui_buffer *buffer;
    struct t_gui_window *retval;

    API_FUNC_INIT(weechat_window_search_with_buffer);

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_buffer) == FAILURE)
    {
        return;
    }
    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));
    retval = weechat_window_search_with_buffer (buffer);
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

PHP_FUNCTION(weechat_window_get_integer)
{
    zend_string *z_window, *z_property;
    struct t_gui_window *window;
    char *property;
    int retval;

    API_FUNC_INIT(weechat_window_get_integer);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SS", &z_window, &z_property) == FAILURE)
    {
        return;
    }
    window = (struct t_gui_window *)API_STR2PTR(ZSTR_VAL(z_window));
    property = ZSTR_VAL(z_property);
    retval = weechat_window_get_integer (window, (const char *)property);
    RETURN_LONG(retval);
}

PHP_FUNCTION(weechat_window_get_string)
{
    zend_string *z_window, *z_property;
    struct t_gui_window *window;
    char *property;
    const char *retval;

    API_FUNC_INIT(weechat_window_get_string);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SS", &z_window, &z_property) == FAILURE)
    {
        return;
    }
    window = (struct t_gui_window *)API_STR2PTR(ZSTR_VAL(z_window));
    property = ZSTR_VAL(z_property);
    retval = weechat_window_get_string (window, (const char *)property);
    SAFE_RETURN_STRING(retval);
}

PHP_FUNCTION(weechat_window_get_pointer)
{
    zend_string *z_window, *z_property;
    struct t_gui_window *window;
    char *property;
    void *retval;

    API_FUNC_INIT(weechat_window_get_pointer);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SS", &z_window, &z_property) == FAILURE)
    {
        return;
    }
    window = (struct t_gui_window *)API_STR2PTR(ZSTR_VAL(z_window));
    property = ZSTR_VAL(z_property);
    retval = weechat_window_get_pointer (window, (const char *)property);
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

PHP_FUNCTION(weechat_window_set_title)
{
    zend_string *z_title;
    char *title;

    API_FUNC_INIT(weechat_window_set_title);

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_title) == FAILURE)
    {
        return;
    }
    title = ZSTR_VAL(z_title);
    weechat_window_set_title ((const char *)title);
    RETURN_NULL();
}

PHP_FUNCTION(weechat_nicklist_add_group)
{
    zend_string *z_buffer, *z_parent_group, *z_name, *z_color;
    zend_long z_visible;
    struct t_gui_buffer *buffer;
    struct t_gui_nick_group *parent_group, *retval;
    char *name, *color;
    int visible;

    API_FUNC_INIT(weechat_nicklist_add_group);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SSSSl", &z_buffer, &z_parent_group, &z_name,
                               &z_color, &z_visible) == FAILURE)
    {
        return;
    }
    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));
    parent_group = (struct t_gui_nick_group *)API_STR2PTR(ZSTR_VAL(z_parent_group));
    name = ZSTR_VAL(z_name);
    color = ZSTR_VAL(z_color);
    visible = (int)z_visible;
    retval = weechat_nicklist_add_group (buffer,
                                         parent_group,
                                         (const char *)name,
                                         (const char *)color,
                                         visible);
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

PHP_FUNCTION(weechat_nicklist_search_group)
{
    zend_string *z_buffer, *z_from_group, *z_name;
    struct t_gui_nick_group *retval;
    struct t_gui_buffer *buffer;
    struct t_gui_nick_group *from_group;
    char *name;

    API_FUNC_INIT(weechat_nicklist_search_group);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SSS", &z_buffer, &z_from_group,
                               &z_name) == FAILURE)
    {
        return;
    }
    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));
    from_group = (struct t_gui_nick_group *)API_STR2PTR(ZSTR_VAL(z_from_group));
    name = ZSTR_VAL(z_name);
    retval = weechat_nicklist_search_group (buffer,
                                            from_group,
                                            (const char *)name);
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

PHP_FUNCTION(weechat_nicklist_add_nick)
{
    zend_string *z_buffer, *z_group, *z_name, *z_color, *z_prefix;
    zend_string *z_prefix_color;
    zend_long z_visible;
    struct t_gui_buffer *buffer;
    struct t_gui_nick_group *group;
    char *name, *color, *prefix, *prefix_color;
    int visible;
    struct t_gui_nick *retval;

    API_FUNC_INIT(weechat_nicklist_add_nick);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SSSSSSl", &z_buffer, &z_group, &z_name,
                               &z_color, &z_prefix, &z_prefix_color,
                               &z_visible) == FAILURE)
    {
        return;
    }
    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));
    group = (struct t_gui_nick_group *)API_STR2PTR(ZSTR_VAL(z_group));
    name = ZSTR_VAL(z_name);
    color = ZSTR_VAL(z_color);
    prefix = ZSTR_VAL(z_prefix);
    prefix_color = ZSTR_VAL(z_prefix_color);
    visible = (int)z_visible;
    retval = weechat_nicklist_add_nick (buffer,
                                        group,
                                        (const char *)name,
                                        (const char *)color,
                                        (const char *)prefix,
                                        (const char *)prefix_color,
                                        visible);
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

PHP_FUNCTION(weechat_nicklist_search_nick)
{
    zend_string *z_buffer, *z_from_group, *z_name;
    struct t_gui_buffer *buffer;
    struct t_gui_nick_group *from_group;
    struct t_gui_nick *retval;
    char *name;

    API_FUNC_INIT(weechat_nicklist_search_nick);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SSS", &z_buffer, &z_from_group,
                               &z_name) == FAILURE)
    {
        return;
    }
    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));
    from_group = (struct t_gui_nick_group *)API_STR2PTR(ZSTR_VAL(z_from_group));
    name = ZSTR_VAL(z_name);
    retval = weechat_nicklist_search_nick (buffer,
                                           from_group,
                                           (const char *)name);
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

PHP_FUNCTION(weechat_nicklist_remove_group)
{
    zend_string *z_buffer, *z_group;
    struct t_gui_buffer *buffer;
    struct t_gui_nick_group *group;

    API_FUNC_INIT(weechat_nicklist_remove_group);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SS", &z_buffer, &z_group) == FAILURE)
    {
        return;
    }
    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));
    group = (struct t_gui_nick_group *)API_STR2PTR(ZSTR_VAL(z_group));
    weechat_nicklist_remove_group (buffer, group);
    RETURN_NULL();
}

PHP_FUNCTION(weechat_nicklist_remove_nick)
{
    zend_string *z_buffer, *z_nick;
    struct t_gui_buffer *buffer;
    struct t_gui_nick *nick;

    API_FUNC_INIT(weechat_nicklist_remove_nick);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SS", &z_buffer, &z_nick) == FAILURE)
    {
        return;
    }
    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));
    nick = (struct t_gui_nick *)API_STR2PTR(ZSTR_VAL(z_nick));
    weechat_nicklist_remove_nick (buffer, nick);
    RETURN_NULL();
}

PHP_FUNCTION(weechat_nicklist_remove_all)
{
    zend_string *z_buffer;
    struct t_gui_buffer *buffer;

    API_FUNC_INIT(weechat_nicklist_remove_all);

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_buffer) == FAILURE)
    {
        return;
    }
    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));
    weechat_nicklist_remove_all (buffer);
    RETURN_NULL();
}

PHP_FUNCTION(weechat_nicklist_group_get_integer)
{
    zend_string *z_buffer, *z_group, *z_property;
    struct t_gui_buffer *buffer;
    struct t_gui_nick_group *group;
    char *property;
    int retval;

    API_FUNC_INIT(weechat_nicklist_group_get_integer);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SSS", &z_buffer, &z_group,
                               &z_property) == FAILURE)
    {
        return;
    }
    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));
    group = (struct t_gui_nick_group *)API_STR2PTR(ZSTR_VAL(z_group));
    property = ZSTR_VAL(z_property);
    retval = weechat_nicklist_group_get_integer (buffer,
                                                 group,
                                                 (const char *)property);
    RETURN_LONG(retval);
}

PHP_FUNCTION(weechat_nicklist_group_get_string)
{
    zend_string *z_buffer, *z_group, *z_property;
    struct t_gui_buffer *buffer;
    struct t_gui_nick_group *group;
    char *property;
    const char *retval;

    API_FUNC_INIT(weechat_nicklist_group_get_string);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SSS", &z_buffer, &z_group,
                               &z_property) == FAILURE)
    {
        return;
    }
    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));
    group = (struct t_gui_nick_group *)API_STR2PTR(ZSTR_VAL(z_group));
    property = ZSTR_VAL(z_property);
    retval = weechat_nicklist_group_get_string (buffer,
                                                group,
                                                (const char *)property);
    SAFE_RETURN_STRING(retval);
}

PHP_FUNCTION(weechat_nicklist_group_get_pointer)
{
    zend_string *z_buffer, *z_group, *z_property;
    struct t_gui_buffer *buffer;
    struct t_gui_nick_group *group;
    char *property;
    void *retval;

    API_FUNC_INIT(weechat_nicklist_group_get_pointer);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SSS", &z_buffer, &z_group,
                               &z_property) == FAILURE)
    {
        return;
    }
    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));
    group = (struct t_gui_nick_group *)API_STR2PTR(ZSTR_VAL(z_group));
    property = ZSTR_VAL(z_property);
    retval = weechat_nicklist_group_get_pointer (buffer,
                                                 group,
                                                 (const char *)property);
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

PHP_FUNCTION(weechat_nicklist_group_set)
{
    zend_string *z_buffer, *z_group, *z_property, *z_value;
    struct t_gui_buffer *buffer;
    struct t_gui_nick_group *group;
    char *property, *value;

    API_FUNC_INIT(weechat_nicklist_group_set);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SSSS", &z_buffer, &z_group, &z_property,
                               &z_value) == FAILURE)
    {
        return;
    }
    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));
    group = (struct t_gui_nick_group *)API_STR2PTR(ZSTR_VAL(z_group));
    property = ZSTR_VAL(z_property);
    value = ZSTR_VAL(z_value);
    weechat_nicklist_group_set (buffer,
                                group,
                                (const char *)property,
                                (const char *)value);
    RETURN_NULL();
}

PHP_FUNCTION(weechat_nicklist_nick_get_integer)
{
    zend_string *z_buffer, *z_nick, *z_property;
    struct t_gui_buffer *buffer;
    struct t_gui_nick *nick;
    char *property;
    int retval;

    API_FUNC_INIT(weechat_nicklist_nick_get_integer);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SSS", &z_buffer, &z_nick,
                               &z_property) == FAILURE)
    {
        return;
    }
    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));
    nick = (struct t_gui_nick *)API_STR2PTR(ZSTR_VAL(z_nick));
    property = ZSTR_VAL(z_property);
    retval = weechat_nicklist_nick_get_integer (buffer,
                                                nick,
                                                (const char *)property);
    RETURN_LONG(retval);
}

PHP_FUNCTION(weechat_nicklist_nick_get_string)
{
    zend_string *z_buffer, *z_nick, *z_property;
    struct t_gui_buffer *buffer;
    struct t_gui_nick *nick;
    char *property;
    const char *retval;

    API_FUNC_INIT(weechat_nicklist_nick_get_string);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SSS", &z_buffer, &z_nick,
                               &z_property) == FAILURE)
    {
        return;
    }
    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));
    nick = (struct t_gui_nick *)API_STR2PTR(ZSTR_VAL(z_nick));
    property = ZSTR_VAL(z_property);
    retval = weechat_nicklist_nick_get_string (buffer,
                                               nick,
                                               (const char *)property);
    SAFE_RETURN_STRING(retval);
}

PHP_FUNCTION(weechat_nicklist_nick_get_pointer)
{
    zend_string *z_buffer, *z_nick, *z_property;
    struct t_gui_buffer *buffer;
    struct t_gui_nick *nick;
    char *property;
    void *retval;

    API_FUNC_INIT(weechat_nicklist_nick_get_pointer);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SSS", &z_buffer, &z_nick,
                               &z_property) == FAILURE)
    {
        return;
    }
    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));
    nick = (struct t_gui_nick *)API_STR2PTR(ZSTR_VAL(z_nick));
    property = ZSTR_VAL(z_property);
    retval = weechat_nicklist_nick_get_pointer (buffer,
                                                nick,
                                                (const char *)property);
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

PHP_FUNCTION(weechat_nicklist_nick_set)
{
    zend_string *z_buffer, *z_nick, *z_property, *z_value;
    struct t_gui_buffer *buffer;
    struct t_gui_nick *nick;
    char *property, *value;

    API_FUNC_INIT(weechat_nicklist_nick_set);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SSSS", &z_buffer, &z_nick, &z_property,
                               &z_value) == FAILURE)
    {
        return;
    }
    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));
    nick = (struct t_gui_nick *)API_STR2PTR(ZSTR_VAL(z_nick));
    property = ZSTR_VAL(z_property);
    value = ZSTR_VAL(z_value);
    weechat_nicklist_nick_set (buffer,
                               nick,
                               (const char *)property,
                               (const char *)value);
    RETURN_NULL();
}

PHP_FUNCTION(weechat_bar_item_search)
{
    zend_string *z_name;
    struct t_gui_bar_item *retval;
    char *name;

    API_FUNC_INIT(weechat_bar_item_search);

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_name) == FAILURE)
    {
        return;
    }
    name = ZSTR_VAL(z_name);
    retval = weechat_bar_item_search ((const char *)name);
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

static char *
weechat_php_bar_item_new_build_callback (const void *pointer, void *data,
                                         struct t_gui_bar_item *item,
                                         struct t_gui_window *window,
                                         struct t_gui_buffer *buffer,
                                         struct t_hashtable *extra_info)
{
    char *rc;
    void *func_argv[5];

    func_argv[1] = API_PTR2STR(item);
    func_argv[2] = API_PTR2STR(window);
    func_argv[3] = API_PTR2STR(buffer);
    func_argv[4] = extra_info;

    weechat_php_cb (pointer, data, func_argv, "ssssh",
                    WEECHAT_SCRIPT_EXEC_STRING, &rc);

    if (func_argv[1])
        free (func_argv[1]);
    if (func_argv[2])
        free (func_argv[2]);
    if (func_argv[3])
        free (func_argv[3]);

    return rc;
}

PHP_FUNCTION(weechat_bar_item_new)
{
    zend_string *z_name, *z_data;
    zval *z_build_callback;
    struct t_gui_bar_item *retval;
    char *name, *data;

    API_FUNC_INIT(weechat_bar_item_new);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SzS", &z_name, &z_build_callback,
                               &z_data) == FAILURE)
    {
        return;
    }
    name = ZSTR_VAL(z_name);
    weechat_php_get_function_name (z_build_callback, build_callback_name);
    data = ZSTR_VAL(z_data);
    retval = plugin_script_api_bar_item_new (weechat_php_plugin,
                                             php_current_script,
                                             (const char *)name,
                                             weechat_php_bar_item_new_build_callback,
                                             (const char *)build_callback_name,
                                             (const char *)data);
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

PHP_FUNCTION(weechat_bar_item_update)
{
    zend_string *z_name;
    char *name;

    API_FUNC_INIT(weechat_bar_item_update);

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_name) == FAILURE)
    {
        return;
    }
    name = ZSTR_VAL(z_name);
    weechat_bar_item_update ((const char *)name);
    RETURN_NULL();
}

PHP_FUNCTION(weechat_bar_item_remove)
{
    zend_string *z_item;
    struct t_gui_bar_item *item;

    API_FUNC_INIT(weechat_bar_item_remove);

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_item) == FAILURE)
    {
        return;
    }
    item = (struct t_gui_bar_item *)API_STR2PTR(ZSTR_VAL(z_item));
    weechat_bar_item_remove (item);
    RETURN_NULL();
}

PHP_FUNCTION(weechat_bar_search)
{
    zend_string *z_name;
    struct t_gui_bar *retval;
    char *name;

    API_FUNC_INIT(weechat_bar_search);

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_name) == FAILURE)
    {
        return;
    }
    name = ZSTR_VAL(z_name);
    retval = weechat_bar_search ((const char *)name);
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

PHP_FUNCTION(weechat_bar_new)
{
    zend_string *z_name, *z_hidden, *z_priority, *z_type, *z_condition;
    zend_string *z_position, *z_filling_top_bottom, *z_filling_left_right;
    zend_string *z_size, *z_size_max, *z_color_fg, *z_color_delim, *z_color_bg;
    zend_string *z_separator, *z_items;
    char *name, *hidden, *priority, *type, *condition, *position;
    char *filling_top_bottom, *filling_left_right, *size, *size_max, *color_fg;
    char *color_delim, *color_bg, *separator, *items;
    struct t_gui_bar *retval;

    API_FUNC_INIT(weechat_bar_new);

    if (zend_parse_parameters (
            ZEND_NUM_ARGS(), "SSSSSSSSSSSSSSS", &z_name, &z_hidden,
            &z_priority, &z_type, &z_condition, &z_position,
            &z_filling_top_bottom, &z_filling_left_right, &z_size, &z_size_max,
            &z_color_fg, &z_color_delim, &z_color_bg, &z_separator,
            &z_items) == FAILURE)
    {
        return;
    }
    name = ZSTR_VAL(z_name);
    hidden = ZSTR_VAL(z_hidden);
    priority = ZSTR_VAL(z_priority);
    type = ZSTR_VAL(z_type);
    condition = ZSTR_VAL(z_condition);
    position = ZSTR_VAL(z_position);
    filling_top_bottom = ZSTR_VAL(z_filling_top_bottom);
    filling_left_right = ZSTR_VAL(z_filling_left_right);
    size = ZSTR_VAL(z_size);
    size_max = ZSTR_VAL(z_size_max);
    color_fg = ZSTR_VAL(z_color_fg);
    color_delim = ZSTR_VAL(z_color_delim);
    color_bg = ZSTR_VAL(z_color_bg);
    separator = ZSTR_VAL(z_separator);
    items = ZSTR_VAL(z_items);
    retval = weechat_bar_new (
        (const char *)name,
        (const char *)hidden,
        (const char *)priority,
        (const char *)type,
        (const char *)condition,
        (const char *)position,
        (const char *)filling_top_bottom,
        (const char *)filling_left_right,
        (const char *)size,
        (const char *)size_max,
        (const char *)color_fg,
        (const char *)color_delim,
        (const char *)color_bg,
        (const char *)separator,
        (const char *)items);
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

PHP_FUNCTION(weechat_bar_set)
{
    zend_string *z_bar, *z_property, *z_value;
    struct t_gui_bar *bar;
    char *property, *value;
    int retval;

    API_FUNC_INIT(weechat_bar_set);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SSS", &z_bar, &z_property, &z_value) == FAILURE)
    {
        return;
    }
    bar = (struct t_gui_bar *)API_STR2PTR(ZSTR_VAL(z_bar));
    property = ZSTR_VAL(z_property);
    value = ZSTR_VAL(z_value);
    retval = weechat_bar_set (bar, (const char *)property, (const char *)value);
    RETURN_LONG(retval);
}

PHP_FUNCTION(weechat_bar_update)
{
    zend_string *z_name;
    char *name;

    API_FUNC_INIT(weechat_bar_update);

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_name) == FAILURE)
    {
        return;
    }
    name = ZSTR_VAL(z_name);
    weechat_bar_update ((const char *)name);
    RETURN_NULL();
}

PHP_FUNCTION(weechat_bar_remove)
{
    zend_string *z_bar;
    struct t_gui_bar *bar;

    API_FUNC_INIT(weechat_bar_remove);

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_bar) == FAILURE)
    {
        return;
    }
    bar = (struct t_gui_bar *)API_STR2PTR(ZSTR_VAL(z_bar));
    weechat_bar_remove (bar);
    RETURN_NULL();
}

PHP_FUNCTION(weechat_command)
{
    zend_string *z_buffer, *z_command;
    struct t_gui_buffer *buffer;
    char *command;
    int retval;

    API_FUNC_INIT(weechat_command);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SS", &z_buffer, &z_command) == FAILURE)
    {
        return;
    }
    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));
    command = ZSTR_VAL(z_command);
    retval = plugin_script_api_command (weechat_php_plugin,
                                        php_current_script,
                                        buffer,
                                        (const char *)command);
    RETURN_LONG(retval);
}

PHP_FUNCTION(weechat_info_get)
{
    zend_string *z_info_name, *z_arguments;
    char *info_name, *arguments;
    const char *retval;

    API_FUNC_INIT(weechat_info_get);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SS", &z_info_name, &z_arguments) == FAILURE)
    {
        return;
    }
    info_name = ZSTR_VAL(z_info_name);
    arguments = ZSTR_VAL(z_arguments);
    retval = weechat_info_get ((const char *)info_name,
                               (const char *)arguments);
    SAFE_RETURN_STRING(retval);
}

PHP_FUNCTION(weechat_info_get_hashtable)
{
    zend_string *z_info_name;
    zval *z_hashtable;
    char *info_name;
    struct t_hashtable *hashtable, *retval;

    API_FUNC_INIT(weechat_info_get_hashtable);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "Sa", &z_info_name, &z_hashtable) == FAILURE)
    {
        return;
    }
    info_name = ZSTR_VAL(z_info_name);
    hashtable = weechat_php_array_to_hashtable (
        z_hashtable,
        WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING);
    retval = weechat_info_get_hashtable ((const char *)info_name, hashtable);
    weechat_php_hashtable_to_array (retval, return_value);
}

PHP_FUNCTION(weechat_infolist_new)
{
    struct t_infolist *retval;

    API_FUNC_INIT(weechat_infolist_new);

    if (zend_parse_parameters_none () == FAILURE)
    {
        return;
    }
    retval = weechat_infolist_new ();
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

PHP_FUNCTION(weechat_infolist_new_item)
{
    zend_string *z_infolist;
    struct t_infolist *infolist;
    struct t_infolist_item *retval;

    API_FUNC_INIT(weechat_infolist_new_item);

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_infolist) == FAILURE)
    {
        return;
    }
    infolist = (struct t_infolist *)API_STR2PTR(ZSTR_VAL(z_infolist));
    retval = weechat_infolist_new_item (infolist);
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

PHP_FUNCTION(weechat_infolist_new_var_integer)
{
    zend_string *z_item, *z_name;
    zend_long z_value;
    struct t_infolist_item *item;
    char *name;
    int value;
    struct t_infolist_var *retval;

    API_FUNC_INIT(weechat_infolist_new_var_integer);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SSl", &z_item, &z_name, &z_value) == FAILURE)
    {
        return;
    }
    item = (struct t_infolist_item *)API_STR2PTR(ZSTR_VAL(z_item));
    name = ZSTR_VAL(z_name);
    value = (int)z_value;
    retval = weechat_infolist_new_var_integer (item,
                                               (const char *)name,
                                               value);
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

PHP_FUNCTION(weechat_infolist_new_var_string)
{
    zend_string *z_item, *z_name, *z_value;
    struct t_infolist_item *item;
    char *name, *value;
    struct t_infolist_var *retval;

    API_FUNC_INIT(weechat_infolist_new_var_string);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SSS", &z_item, &z_name, &z_value) == FAILURE)
    {
        return;
    }
    item = (struct t_infolist_item *)API_STR2PTR(ZSTR_VAL(z_item));
    name = ZSTR_VAL(z_name);
    value = ZSTR_VAL(z_value);
    retval = weechat_infolist_new_var_string (item,
                                              (const char *)name,
                                              (const char *)value);
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

PHP_FUNCTION(weechat_infolist_new_var_pointer)
{
    zend_string *z_item, *z_name, *z_pointer;
    struct t_infolist_item *item;
    char *name;
    void *pointer;
    struct t_infolist_var *retval;

    API_FUNC_INIT(weechat_infolist_new_var_pointer);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SSS", &z_item, &z_name, &z_pointer) == FAILURE)
    {
        return;
    }
    item = (struct t_infolist_item *)API_STR2PTR(ZSTR_VAL(z_item));
    name = ZSTR_VAL(z_name);
    pointer = (void *)API_STR2PTR(ZSTR_VAL(z_pointer));
    retval = weechat_infolist_new_var_pointer (item,
                                               (const char *)name,
                                               pointer);
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

PHP_FUNCTION(weechat_infolist_new_var_time)
{
    zend_string *z_item, *z_name;
    zend_long z_time;
    struct t_infolist_item *item;
    char *name;
    time_t time;
    struct t_infolist_var *retval;

    API_FUNC_INIT(weechat_infolist_new_var_time);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SSl", &z_item, &z_name, &z_time) == FAILURE)
    {
        return;
    }
    item = (struct t_infolist_item *)API_STR2PTR(ZSTR_VAL(z_item));
    name = ZSTR_VAL(z_name);
    time = (time_t)z_time;
    retval = weechat_infolist_new_var_time (item, (const char *)name, time);
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

PHP_FUNCTION(weechat_infolist_search_var)
{
    zend_string *z_infolist, *z_name;
    struct t_infolist *infolist;
    char *name;
    struct t_infolist_var *retval;

    API_FUNC_INIT(weechat_infolist_search_var);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SS", &z_infolist, &z_name) == FAILURE)
    {
        return;
    }
    infolist = (struct t_infolist *)API_STR2PTR(ZSTR_VAL(z_infolist));
    name = ZSTR_VAL(z_name);
    retval = weechat_infolist_search_var (infolist, (const char *)name);
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

PHP_FUNCTION(weechat_infolist_get)
{
    zend_string *z_infolist_name, *z_pointer, *z_arguments;
    struct t_infolist *retval;
    char *infolist_name, *arguments;
    void *pointer;

    API_FUNC_INIT(weechat_infolist_get);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SSS", &z_infolist_name, &z_pointer,
                               &z_arguments) == FAILURE)
    {
        return;
    }
    infolist_name = ZSTR_VAL(z_infolist_name);
    pointer = (void *)API_STR2PTR(ZSTR_VAL(z_pointer));
    arguments = ZSTR_VAL(z_arguments);
    retval = weechat_infolist_get ((const char *)infolist_name,
                                   pointer,
                                   (const char *)arguments);
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

PHP_FUNCTION(weechat_infolist_next)
{
    zend_string *z_infolist;
    struct t_infolist *infolist;
    int retval;

    API_FUNC_INIT(weechat_infolist_next);

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_infolist) == FAILURE)
    {
        return;
    }
    infolist = (struct t_infolist *)API_STR2PTR(ZSTR_VAL(z_infolist));
    retval = weechat_infolist_next (infolist);
    RETURN_LONG(retval);
}

PHP_FUNCTION(weechat_infolist_prev)
{
    zend_string *z_infolist;
    struct t_infolist *infolist;
    int retval;

    API_FUNC_INIT(weechat_infolist_prev);

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_infolist) == FAILURE)
    {
        return;
    }
    infolist = (struct t_infolist *)API_STR2PTR(ZSTR_VAL(z_infolist));
    retval = weechat_infolist_prev (infolist);
    RETURN_LONG(retval);
}

PHP_FUNCTION(weechat_infolist_reset_item_cursor)
{
    zend_string *z_infolist;
    struct t_infolist *infolist;

    API_FUNC_INIT(weechat_infolist_reset_item_cursor);

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_infolist) == FAILURE)
    {
        return;
    }
    infolist = (struct t_infolist *)API_STR2PTR(ZSTR_VAL(z_infolist));
    weechat_infolist_reset_item_cursor (infolist);
    RETURN_NULL();
}

PHP_FUNCTION(weechat_infolist_fields)
{
    zend_string *z_infolist;
    struct t_infolist *infolist;
    const char *retval;

    API_FUNC_INIT(weechat_infolist_fields);

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_infolist) == FAILURE)
    {
        return;
    }
    infolist = (struct t_infolist *)API_STR2PTR(ZSTR_VAL(z_infolist));
    retval = weechat_infolist_fields (infolist);
    SAFE_RETURN_STRING(retval);
}

PHP_FUNCTION(weechat_infolist_integer)
{
    zend_string *z_infolist, *z_var;
    struct t_infolist *infolist;
    char *var;
    int retval;

    API_FUNC_INIT(weechat_infolist_integer);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SS", &z_infolist, &z_var) == FAILURE)
    {
        return;
    }
    infolist = (struct t_infolist *)API_STR2PTR(ZSTR_VAL(z_infolist));
    var = ZSTR_VAL(z_var);
    retval = weechat_infolist_integer (infolist, (const char *)var);
    RETURN_LONG(retval);
}

PHP_FUNCTION(weechat_infolist_string)
{
    zend_string *z_infolist, *z_var;
    struct t_infolist *infolist;
    char *var;
    const char *retval;

    API_FUNC_INIT(weechat_infolist_string);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SS", &z_infolist, &z_var) == FAILURE)
    {
        return;
    }
    infolist = (struct t_infolist *)API_STR2PTR(ZSTR_VAL(z_infolist));
    var = ZSTR_VAL(z_var);
    retval = weechat_infolist_string (infolist, (const char *)var);
    SAFE_RETURN_STRING(retval);
}

PHP_FUNCTION(weechat_infolist_pointer)
{
    zend_string *z_infolist, *z_var;
    struct t_infolist *infolist;
    char *var;
    void *retval;

    API_FUNC_INIT(weechat_infolist_pointer);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SS", &z_infolist, &z_var) == FAILURE)
    {
        return;
    }
    infolist = (struct t_infolist *)API_STR2PTR(ZSTR_VAL(z_infolist));
    var = ZSTR_VAL(z_var);
    retval = weechat_infolist_pointer (infolist, (const char *)var);
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

PHP_FUNCTION(weechat_infolist_time)
{
    zend_string *z_infolist, *z_var;
    struct t_infolist *infolist;
    char *var;
    time_t retval;

    API_FUNC_INIT(weechat_infolist_time);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SS", &z_infolist, &z_var) == FAILURE)
    {
        return;
    }
    infolist = (struct t_infolist *)API_STR2PTR(ZSTR_VAL(z_infolist));
    var = ZSTR_VAL(z_var);
    retval = weechat_infolist_time (infolist, (const char *)var);
    RETURN_LONG(retval);
}

PHP_FUNCTION(weechat_infolist_free)
{
    zend_string *z_infolist;
    struct t_infolist *infolist;

    API_FUNC_INIT(weechat_infolist_free);

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_infolist) == FAILURE)
    {
        return;
    }
    infolist = (struct t_infolist *)API_STR2PTR(ZSTR_VAL(z_infolist));
    weechat_infolist_free (infolist);
    RETURN_NULL();
}

PHP_FUNCTION(weechat_hdata_get)
{
    zend_string *z_hdata_name;
    char *hdata_name;
    struct t_hdata *retval;

    API_FUNC_INIT(weechat_hdata_get);

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_hdata_name) == FAILURE)
    {
        return;
    }
    hdata_name = ZSTR_VAL(z_hdata_name);
    retval = weechat_hdata_get ((const char *)hdata_name);
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

PHP_FUNCTION(weechat_hdata_get_var_offset)
{
    zend_string *z_hdata, *z_name;
    struct t_hdata *hdata;
    char *name;
    int retval;

    API_FUNC_INIT(weechat_hdata_get_var_offset);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SS", &z_hdata, &z_name) == FAILURE)
    {
        return;
    }
    hdata = (struct t_hdata *)API_STR2PTR(ZSTR_VAL(z_hdata));
    name = ZSTR_VAL(z_name);
    retval = weechat_hdata_get_var_offset (hdata, (const char *)name);
    RETURN_LONG(retval);
}

PHP_FUNCTION(weechat_hdata_get_var_type_string)
{
    zend_string *z_hdata, *z_name;
    struct t_hdata *hdata;
    char *name;
    const char *retval;

    API_FUNC_INIT(weechat_hdata_get_var_type_string);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SS", &z_hdata, &z_name) == FAILURE)
    {
        return;
    }
    hdata = (struct t_hdata *)API_STR2PTR(ZSTR_VAL(z_hdata));
    name = ZSTR_VAL(z_name);
    retval = weechat_hdata_get_var_type_string (hdata, (const char *)name);
    SAFE_RETURN_STRING(retval);
}

PHP_FUNCTION(weechat_hdata_get_var_array_size)
{
    zend_string *z_hdata, *z_pointer, *z_name;
    struct t_hdata *hdata;
    void *pointer;
    char *name;
    int retval;

    API_FUNC_INIT(weechat_hdata_get_var_array_size);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SSS", &z_hdata, &z_pointer, &z_name) == FAILURE)
    {
        return;
    }
    hdata = (struct t_hdata *)API_STR2PTR(ZSTR_VAL(z_hdata));
    pointer = (void *)API_STR2PTR(ZSTR_VAL(z_pointer));
    name = ZSTR_VAL(z_name);
    retval = weechat_hdata_get_var_array_size (hdata,
                                               pointer,
                                               (const char *)name);
    RETURN_LONG(retval);
}

PHP_FUNCTION(weechat_hdata_get_var_array_size_string)
{
    zend_string *z_hdata, *z_pointer, *z_name;
    struct t_hdata *hdata;
    void *pointer;
    char *name;
    const char *retval;

    API_FUNC_INIT(weechat_hdata_get_var_array_size_string);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SSS", &z_hdata, &z_pointer, &z_name) == FAILURE)
    {
        return;
    }
    hdata = (struct t_hdata *)API_STR2PTR(ZSTR_VAL(z_hdata));
    pointer = (void *)API_STR2PTR(ZSTR_VAL(z_pointer));
    name = ZSTR_VAL(z_name);
    retval = weechat_hdata_get_var_array_size_string (hdata,
                                                      pointer,
                                                      (const char *)name);
    SAFE_RETURN_STRING(retval);
}

PHP_FUNCTION(weechat_hdata_get_var_hdata)
{
    zend_string *z_hdata, *z_name;
    struct t_hdata *hdata;
    char *name;
    const char *retval;

    API_FUNC_INIT(weechat_hdata_get_var_hdata);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SS", &z_hdata, &z_name) == FAILURE)
    {
        return;
    }
    hdata = (struct t_hdata *)API_STR2PTR(ZSTR_VAL(z_hdata));
    name = ZSTR_VAL(z_name);
    retval = weechat_hdata_get_var_hdata (hdata, (const char *)name);
    SAFE_RETURN_STRING(retval);
}

PHP_FUNCTION(weechat_hdata_get_list)
{
    zend_string *z_hdata, *z_name;
    struct t_hdata *hdata;
    char *name;
    void *retval;

    API_FUNC_INIT(weechat_hdata_get_list);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SS", &z_hdata, &z_name) == FAILURE)
    {
        return;
    }
    hdata = (struct t_hdata *)API_STR2PTR(ZSTR_VAL(z_hdata));
    name = ZSTR_VAL(z_name);
    retval = weechat_hdata_get_list (hdata, (const char *)name);
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

PHP_FUNCTION(weechat_hdata_check_pointer)
{
    zend_string *z_hdata, *z_list, *z_pointer;
    struct t_hdata *hdata;
    void *list, *pointer;
    int retval;

    API_FUNC_INIT(weechat_hdata_check_pointer);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SSS", &z_hdata, &z_list, &z_pointer) == FAILURE)
    {
        return;
    }
    hdata = (struct t_hdata *)API_STR2PTR(ZSTR_VAL(z_hdata));
    list = (void *)API_STR2PTR(ZSTR_VAL(z_list));
    pointer = (void *)API_STR2PTR(ZSTR_VAL(z_pointer));
    retval = weechat_hdata_check_pointer (hdata, list, pointer);
    RETURN_LONG(retval);
}

PHP_FUNCTION(weechat_hdata_move)
{
    zend_string *z_hdata, *z_pointer;
    zend_long z_count;
    struct t_hdata *hdata;
    void *pointer;
    int count;
    void *retval;

    API_FUNC_INIT(weechat_hdata_move);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SSl", &z_hdata, &z_pointer, &z_count) == FAILURE)
    {
        return;
    }
    hdata = (struct t_hdata *)API_STR2PTR(ZSTR_VAL(z_hdata));
    pointer = (void *)API_STR2PTR(ZSTR_VAL(z_pointer));
    count = (int)z_count;
    retval = weechat_hdata_move (hdata, pointer, count);
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

PHP_FUNCTION(weechat_hdata_search)
{
    zend_string *z_hdata, *z_pointer, *z_search;
    zend_long z_move;
    struct t_hdata *hdata;
    void *pointer;
    char *search;
    int move;
    void *retval;

    API_FUNC_INIT(weechat_hdata_search);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SSSl", &z_hdata, &z_pointer, &z_search,
                               &z_move) == FAILURE)
    {
        return;
    }
    hdata = (struct t_hdata *)API_STR2PTR(ZSTR_VAL(z_hdata));
    pointer = (void *)API_STR2PTR(ZSTR_VAL(z_pointer));
    search = ZSTR_VAL(z_search);
    move = (int)z_move;
    retval = weechat_hdata_search (hdata, pointer, (const char *)search, move);
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

PHP_FUNCTION(weechat_hdata_char)
{
    zend_string *z_hdata, *z_pointer, *z_name;
    struct t_hdata *hdata;
    void *pointer;
    char *name, retval;

    API_FUNC_INIT(weechat_hdata_char);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SSS", &z_hdata, &z_pointer, &z_name) == FAILURE)
    {
        return;
    }
    hdata = (struct t_hdata *)API_STR2PTR(ZSTR_VAL(z_hdata));
    pointer = (void *)API_STR2PTR(ZSTR_VAL(z_pointer));
    name = ZSTR_VAL(z_name);
    retval = weechat_hdata_char (hdata, pointer, (const char *)name);
    RETURN_LONG((int)retval);
}

PHP_FUNCTION(weechat_hdata_integer)
{
    zend_string *z_hdata, *z_pointer, *z_name;
    struct t_hdata *hdata;
    void *pointer;
    char *name;
    int retval;

    API_FUNC_INIT(weechat_hdata_integer);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SSS", &z_hdata, &z_pointer, &z_name) == FAILURE)
    {
        return;
    }
    hdata = (struct t_hdata *)API_STR2PTR(ZSTR_VAL(z_hdata));
    pointer = (void *)API_STR2PTR(ZSTR_VAL(z_pointer));
    name = ZSTR_VAL(z_name);
    retval = weechat_hdata_integer (hdata, pointer, (const char *)name);
    RETURN_LONG(retval);
}

PHP_FUNCTION(weechat_hdata_long)
{
    zend_string *z_hdata, *z_pointer, *z_name;
    struct t_hdata *hdata;
    void *pointer;
    char *name;
    long retval;

    API_FUNC_INIT(weechat_hdata_long);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SSS", &z_hdata, &z_pointer, &z_name) == FAILURE)
    {
        return;
    }
    hdata = (struct t_hdata *)API_STR2PTR(ZSTR_VAL(z_hdata));
    pointer = (void *)API_STR2PTR(ZSTR_VAL(z_pointer));
    name = ZSTR_VAL(z_name);
    retval = weechat_hdata_long (hdata, pointer, (const char *)name);
    RETURN_LONG(retval);
}

PHP_FUNCTION(weechat_hdata_string)
{
    zend_string *z_hdata, *z_pointer, *z_name;
    struct t_hdata *hdata;
    void *pointer;
    char *name;
    const char *retval;

    API_FUNC_INIT(weechat_hdata_string);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SSS", &z_hdata, &z_pointer, &z_name) == FAILURE)
    {
        return;
    }
    hdata = (struct t_hdata *)API_STR2PTR(ZSTR_VAL(z_hdata));
    pointer = (void *)API_STR2PTR(ZSTR_VAL(z_pointer));
    name = ZSTR_VAL(z_name);
    retval = weechat_hdata_string (hdata, pointer, (const char *)name);
    SAFE_RETURN_STRING(retval);
}

PHP_FUNCTION(weechat_hdata_pointer)
{
    zend_string *z_hdata, *z_pointer, *z_name;
    struct t_hdata *hdata;
    void *pointer, *retval;
    char *name;

    API_FUNC_INIT(weechat_hdata_pointer);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SSS", &z_hdata, &z_pointer, &z_name) == FAILURE)
    {
        return;
    }
    hdata = (struct t_hdata *)API_STR2PTR(ZSTR_VAL(z_hdata));
    pointer = (void *)API_STR2PTR(ZSTR_VAL(z_pointer));
    name = ZSTR_VAL(z_name);
    retval = weechat_hdata_pointer (hdata, pointer, (const char *)name);
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

PHP_FUNCTION(weechat_hdata_time)
{
    zend_string *z_hdata, *z_pointer, *z_name;
    struct t_hdata *hdata;
    void *pointer;
    char *name;
    time_t retval;

    API_FUNC_INIT(weechat_hdata_time);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SSS", &z_hdata, &z_pointer, &z_name) == FAILURE)
    {
        return;
    }
    hdata = (struct t_hdata *)API_STR2PTR(ZSTR_VAL(z_hdata));
    pointer = (void *)API_STR2PTR(ZSTR_VAL(z_pointer));
    name = ZSTR_VAL(z_name);
    retval = weechat_hdata_time (hdata, pointer, (const char *)name);
    RETURN_LONG(retval);
}

PHP_FUNCTION(weechat_hdata_hashtable)
{
    zend_string *z_hdata, *z_pointer, *z_name;
    struct t_hdata *hdata;
    void *pointer;
    char *name;
    struct t_hashtable *retval;

    API_FUNC_INIT(weechat_hdata_hashtable);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SSS", &z_hdata, &z_pointer, &z_name) == FAILURE)
    {
        return;
    }
    hdata = (struct t_hdata *)API_STR2PTR(ZSTR_VAL(z_hdata));
    pointer = (void *)API_STR2PTR(ZSTR_VAL(z_pointer));
    name = ZSTR_VAL(z_name);
    retval = weechat_hdata_hashtable (hdata, pointer, (const char *)name);
    weechat_php_hashtable_to_array (retval, return_value);
}

PHP_FUNCTION(weechat_hdata_compare)
{
    zend_string *z_hdata, *z_pointer1, *z_pointer2, *z_name;
    zend_long z_case_sensitive;
    struct t_hdata *hdata;
    void *pointer1, *pointer2;
    char *name;
    int case_sensitive, retval;

    API_FUNC_INIT(weechat_hdata_compare);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SSSSl", &z_hdata, &z_pointer1, &z_pointer2,
                               &z_name, &z_case_sensitive) == FAILURE)
    {
        return;
    }
    hdata = (struct t_hdata *)API_STR2PTR(ZSTR_VAL(z_hdata));
    pointer1 = (void *)API_STR2PTR(ZSTR_VAL(z_pointer1));
    pointer2 = (void *)API_STR2PTR(ZSTR_VAL(z_pointer2));
    name = (void *)API_STR2PTR(ZSTR_VAL(z_name));
    case_sensitive = (int)z_case_sensitive;
    retval = weechat_hdata_compare (hdata, pointer1, pointer2, name,
                                    case_sensitive);
    RETURN_LONG(retval);
}

PHP_FUNCTION(weechat_hdata_update)
{
    zend_string *z_hdata, *z_pointer;
    zval *z_hashtable;
    int retval;
    struct t_hdata *hdata;
    void *pointer;
    struct t_hashtable *hashtable;

    API_FUNC_INIT(weechat_hdata_update);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SSa", &z_hdata, &z_pointer,
                               &z_hashtable) == FAILURE)
    {
        return;
    }
    hdata = (struct t_hdata *)API_STR2PTR(ZSTR_VAL(z_hdata));
    pointer = (void *)API_STR2PTR(ZSTR_VAL(z_pointer));
    hashtable = weechat_php_array_to_hashtable (
        z_hashtable,
        WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING);
    retval = weechat_hdata_update (hdata, pointer, hashtable);
    RETURN_LONG(retval);
}

PHP_FUNCTION(weechat_hdata_get_string)
{
    zend_string *z_hdata, *z_property;
    struct t_hdata *hdata;
    char *property;
    const char *retval;

    API_FUNC_INIT(weechat_hdata_get_string);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SS", &z_hdata, &z_property) == FAILURE)
    {
        return;
    }
    hdata = (struct t_hdata *)API_STR2PTR(ZSTR_VAL(z_hdata));
    property = ZSTR_VAL(z_property);
    retval = weechat_hdata_get_string (hdata, (const char *)property);
    SAFE_RETURN_STRING(retval);
}

static int
weechat_php_upgrade_new_callback_read (const void *pointer, void *data,
                                       struct t_upgrade_file *upgrade_file,
                                       int object_id,
                                       struct t_infolist *infolist)
{
    int rc;
    void *func_argv[4];

    func_argv[1] = API_PTR2STR(upgrade_file);
    func_argv[2] = &object_id;
    func_argv[3] = API_PTR2STR(infolist);

    weechat_php_cb (pointer, data, func_argv, "ssis",
                    WEECHAT_SCRIPT_EXEC_INT, &rc);

    if (func_argv[1])
        free (func_argv[1]);
    if (func_argv[3])
        free (func_argv[3]);

    return rc;
}

PHP_FUNCTION(weechat_upgrade_new)
{
    zend_string *z_filename, *z_data;
    zval *z_callback_read;
    char *filename;
    char *data;
    struct t_upgrade_file *retval;

    API_FUNC_INIT(weechat_upgrade_new);

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SzS", &z_filename,
                               &z_callback_read, &z_data) == FAILURE)
    {
        return;
    }
    filename = ZSTR_VAL(z_filename);
    weechat_php_get_function_name (z_callback_read, callback_read_name);
    data = ZSTR_VAL(z_data);
    retval = plugin_script_api_upgrade_new (weechat_php_plugin,
                                            php_current_script,
                                            (const char *)filename,
                                            weechat_php_upgrade_new_callback_read,
                                            (const char *)callback_read_name,
                                            (const char *)data);
    char *__retstr = API_PTR2STR(retval); SAFE_RETURN_STRING(__retstr);
}

PHP_FUNCTION(weechat_upgrade_write_object)
{
    zend_string *z_upgrade_file, *z_infolist;
    zend_long z_object_id;
    struct t_upgrade_file *upgrade_file;
    int object_id;
    struct t_infolist *infolist;
    int retval;

    API_FUNC_INIT(weechat_upgrade_write_object);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "SlS", &z_upgrade_file, &z_object_id,
                               &z_infolist) == FAILURE)
    {
        return;
    }
    upgrade_file = (struct t_upgrade_file *)API_STR2PTR(ZSTR_VAL(z_upgrade_file));
    object_id = (int)z_object_id;
    infolist = (struct t_infolist *)API_STR2PTR(ZSTR_VAL(z_infolist));
    retval = weechat_upgrade_write_object (upgrade_file, object_id, infolist);
    RETURN_LONG(retval);
}

PHP_FUNCTION(weechat_upgrade_read)
{
    zend_string *z_upgrade_file;
    struct t_upgrade_file *upgrade_file;
    int retval;

    API_FUNC_INIT(weechat_upgrade_read);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "S", &z_upgrade_file) == FAILURE)
    {
        return;
    }
    upgrade_file = (struct t_upgrade_file *)API_STR2PTR(ZSTR_VAL(z_upgrade_file));
    retval = weechat_upgrade_read (upgrade_file);
    RETURN_LONG(retval);
}

PHP_FUNCTION(weechat_upgrade_close)
{
    zend_string *z_upgrade_file;
    struct t_upgrade_file *upgrade_file;

    API_FUNC_INIT(weechat_upgrade_close);

    if (zend_parse_parameters (ZEND_NUM_ARGS(),
                               "S", &z_upgrade_file) == FAILURE)
    {
        return;
    }
    upgrade_file = (struct t_upgrade_file *)API_STR2PTR(ZSTR_VAL(z_upgrade_file));
    weechat_upgrade_close (upgrade_file);
    RETURN_NULL();
}

static void forget_hash_entry (HashTable *ht, INTERNAL_FUNCTION_PARAMETERS)
{
    zend_string *class_name;
    zend_string *lc_name;
    int re;
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &class_name) == FAILURE)
    {
        return;
    }
    if (ZSTR_VAL(class_name)[0] == '\\')
    {
        lc_name = zend_string_alloc (ZSTR_LEN(class_name) - 1, 0);
        zend_str_tolower_copy (ZSTR_VAL(lc_name), ZSTR_VAL(class_name) + 1, ZSTR_LEN(class_name) - 1);
    }
    else
    {
        lc_name = zend_string_tolower (class_name);
    }
    re = zend_hash_del (ht, lc_name);
    zend_string_release (lc_name);
    if (re == SUCCESS)
    {
        RETURN_TRUE;
    }
    RETURN_FALSE;
}

PHP_FUNCTION(forget_class)
{
    forget_hash_entry (EG(class_table), INTERNAL_FUNCTION_PARAM_PASSTHRU);
}

PHP_FUNCTION(forget_function)
{
    forget_hash_entry (EG(function_table), INTERNAL_FUNCTION_PARAM_PASSTHRU);
}
