/*
 * weechat-php-api.c - PHP API functions
 *
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

#include <sapi/embed/php_embed.h>
#include <php.h>
#include <php_ini.h>
#include <ext/standard/info.h>

#include "../weechat-plugin.h"
#include "../plugin-script.h"
#include "../plugin-script-api.h"
#include "weechat-php.h"

#define API_FUNC(__name)                                                \
    PHP_FUNCTION(weechat_##__name)
#define API_INIT_FUNC(__init, __name, __ret)                            \
    char *php_function_name = __name;                                   \
    if (__init                                                          \
        && (!php_current_script || !php_current_script->name))          \
    {                                                                   \
        WEECHAT_SCRIPT_MSG_NOT_INIT(PHP_CURRENT_SCRIPT_NAME,            \
                                    php_function_name);                 \
        __ret;                                                          \
    }
#define API_WRONG_ARGS(__ret)                                           \
    {                                                                   \
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PHP_CURRENT_SCRIPT_NAME,          \
                                      php_function_name);               \
        __ret;                                                          \
    }
#define API_PTR2STR(__pointer)                                          \
    plugin_script_ptr2str (__pointer)
#define API_STR2PTR(__string)                                           \
    plugin_script_str2ptr (weechat_php_plugin,                          \
                           PHP_CURRENT_SCRIPT_NAME,                     \
                           php_function_name, __string)
#define API_RETURN_OK RETURN_LONG((long)1)
#define API_RETURN_ERROR RETURN_LONG((long)0)
#define API_RETURN_EMPTY RETURN_NULL()
#define API_RETURN_STRING(__string)                                     \
    RETURN_STRING((__string) ? (__string) : "")
#define API_RETURN_STRING_FREE(__string)                                \
    if (__string)                                                       \
    {                                                                   \
        RETVAL_STRING(__string);                                        \
        free (__string);                                                \
        return;                                                         \
    }                                                                   \
    RETURN_STRING("");
#define API_RETURN_INT(__int) RETURN_LONG(__int)
#define API_RETURN_LONG(__long) RETURN_LONG(__long)
#define weechat_php_get_function_name(__zfunc, __str)                   \
    const char *(__str);                                                \
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

API_FUNC(register)
{
    zend_string *name, *author, *version, *license, *description, *charset;
    zval *shutdown_func;
    const char *shutdown_func_name;

    API_INIT_FUNC(0, "register", API_RETURN_ERROR);
    if (php_registered_script)
    {
        /* script already registered */
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: script \"%s\" already "
                                         "registered (register ignored)"),
                        weechat_prefix ("error"), PHP_PLUGIN_NAME,
                        php_registered_script->name);
        API_RETURN_ERROR;
    }
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SSSSSzS",
                               &name, &author, &version, &license, &description,
                               &shutdown_func, &charset) == FAILURE)
    {
        API_WRONG_ARGS(API_RETURN_ERROR);
    }

    php_current_script = NULL;
    php_registered_script = NULL;

    if (plugin_script_search (php_scripts, ZSTR_VAL(name)))
    {
        /* another script already exists with same name */
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to register script "
                                         "\"%s\" (another script already "
                                         "exists with this name)"),
                        weechat_prefix ("error"), PHP_PLUGIN_NAME,
                        ZSTR_VAL(name));
        API_RETURN_ERROR;
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
                                            &php_data,
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
        API_RETURN_ERROR;
    }

    API_RETURN_OK;
}

/*
 * Generic PHP callback function.
 */

static void
weechat_php_cb (const void *pointer, void *data, void **func_argv,
                const char *func_types, int ret_type, void *rc)
{
    struct t_plugin_script *script;
    const char *ptr_function, *ptr_data;
    void *ret;

    ret = NULL;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    func_argv[0] = ptr_data ? (char *)ptr_data : weechat_php_empty_arg;

    if (!ptr_function || !ptr_function[0])
    {
        goto weechat_php_cb_err;
    }

    ret = weechat_php_exec (script, ret_type, ptr_function,
                            func_types, func_argv);

    if ((ret_type != WEECHAT_SCRIPT_EXEC_IGNORE) && !ret)
    {
        goto weechat_php_cb_err;
    }

    if (ret_type == WEECHAT_SCRIPT_EXEC_IGNORE)
    {
        if (ret)
            free (ret);
    }
    else if (ret_type == WEECHAT_SCRIPT_EXEC_INT)
    {
        *((int *)rc) = *((int *)ret);
        free (ret);
    }
    else if (ret_type == WEECHAT_SCRIPT_EXEC_HASHTABLE)
    {
        *((struct t_hashtable **)rc) = (struct t_hashtable *)ret;
    }
    else
    {
        *((char **)rc) = (char *)ret;
    }
    return;

weechat_php_cb_err:
    if (ret_type == WEECHAT_SCRIPT_EXEC_IGNORE)
    {
        if (ret)
            free (ret);
    }
    else if (ret_type == WEECHAT_SCRIPT_EXEC_INT)
    {
        *((int *)rc) = WEECHAT_RC_ERROR;
    }
    else if (ret_type == WEECHAT_SCRIPT_EXEC_HASHTABLE)
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

API_FUNC(plugin_get_name)
{
    zend_string *z_plugin;
    struct t_weechat_plugin *plugin;
    const char *result;

    API_INIT_FUNC(1, "plugin_get_name", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_plugin) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    plugin = API_STR2PTR(ZSTR_VAL(z_plugin));

    result = weechat_plugin_get_name (plugin);

    API_RETURN_STRING(result);
}

API_FUNC(charset_set)
{
    zend_string *z_charset;
    char *charset;

    API_INIT_FUNC(1, "charset_set", API_RETURN_ERROR);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_charset) == FAILURE)
        API_WRONG_ARGS(API_RETURN_ERROR);

    charset = ZSTR_VAL(z_charset);

    plugin_script_api_charset_set (php_current_script, (const char *)charset);

    API_RETURN_OK;
}

API_FUNC(iconv_to_internal)
{
    zend_string *z_charset, *z_string;
    char *charset, *string, *result;

    API_INIT_FUNC(1, "iconv_to_internal", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SS",
                               &z_charset, &z_string) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    charset = ZSTR_VAL(z_charset);
    string = ZSTR_VAL(z_string);

    result = weechat_iconv_to_internal ((const char *)charset,
                                        (const char *)string);

    API_RETURN_STRING_FREE(result);
}

API_FUNC(iconv_from_internal)
{
    zend_string *z_charset, *z_string;
    char *charset, *string, *result;

    API_INIT_FUNC(1, "iconv_from_internal", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SS",
                               &z_charset, &z_string) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    charset = ZSTR_VAL(z_charset);
    string = ZSTR_VAL(z_string);

    result = weechat_iconv_from_internal ((const char *)charset,
                                          (const char *)string);

    API_RETURN_STRING_FREE(result);
}

API_FUNC(gettext)
{
    zend_string *z_string;
    char *string;
    const char *result;

    API_INIT_FUNC(1, "gettext", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_string) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    string = ZSTR_VAL(z_string);

    result = weechat_gettext ((const char *)string);

    API_RETURN_STRING(result);
}

API_FUNC(ngettext)
{
    zend_string *z_single, *z_plural;
    zend_long z_count;
    char *single, *plural;
    int count;
    const char *result;

    API_INIT_FUNC(1, "ngettext", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SSl",
                               &z_single, &z_plural, &z_count) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    single = ZSTR_VAL(z_single);
    plural = ZSTR_VAL(z_plural);
    count = (int)z_count;

    result = weechat_ngettext ((const char *)single,
                               (const char *)plural,
                               count);

    API_RETURN_STRING(result);
}

API_FUNC(strlen_screen)
{
    zend_string *z_string;
    char *string;
    int result;

    API_INIT_FUNC(1, "strlen_screen", API_RETURN_INT(0));

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_string) == FAILURE)
        API_WRONG_ARGS(API_RETURN_INT(0));

    string = ZSTR_VAL(z_string);

    result = weechat_strlen_screen ((const char *)string);

    API_RETURN_INT(result);
}

API_FUNC(string_match)
{
    zend_string *z_string, *z_mask;
    zend_long z_case_sensitive;
    int case_sensitive, result;
    char *string, *mask;

    API_INIT_FUNC(1, "string_match", API_RETURN_INT(0));
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SSl", &z_string, &z_mask,
                               &z_case_sensitive) == FAILURE)
        API_WRONG_ARGS(API_RETURN_INT(0));

    string = ZSTR_VAL(z_string);
    mask = ZSTR_VAL(z_mask);
    case_sensitive = (int)z_case_sensitive;

    result = weechat_string_match ((const char *)string,
                                   (const char *)mask,
                                   case_sensitive);

    API_RETURN_INT(result);
}

API_FUNC(string_match_list)
{
    zend_string *z_string, *z_masks;
    zend_long z_case_sensitive;
    int case_sensitive, result;
    char *string, *masks;

    API_INIT_FUNC(1, "string_match_list", API_RETURN_INT(0));
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SSl", &z_string, &z_masks,
                               &z_case_sensitive) == FAILURE)
        API_WRONG_ARGS(API_RETURN_INT(0));

    string = ZSTR_VAL(z_string);
    masks = ZSTR_VAL(z_masks);
    case_sensitive = (int)z_case_sensitive;

    result = plugin_script_api_string_match_list (weechat_php_plugin,
                                                  (const char *)string,
                                                  (const char *)masks,
                                                  case_sensitive);

    API_RETURN_INT(result);
}

API_FUNC(string_has_highlight)
{
    zend_string *z_string, *z_highlight_words;
    char *string, *highlight_words;
    int result;

    API_INIT_FUNC(1, "string_has_highlight", API_RETURN_INT(0));

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SS", &z_string,
                               &z_highlight_words) == FAILURE)
        API_WRONG_ARGS(API_RETURN_INT(0));

    string = ZSTR_VAL(z_string);
    highlight_words = ZSTR_VAL(z_highlight_words);

    result = weechat_string_has_highlight ((const char *)string,
                                           (const char *)highlight_words);

    API_RETURN_INT(result);
}

API_FUNC(string_has_highlight_regex)
{
    zend_string *z_string, *z_regex;
    char *string, *regex;
    int result;

    API_INIT_FUNC(1, "string_has_highlight_regex", API_RETURN_INT(0));
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SS", &z_string,
                               &z_regex) == FAILURE)
        API_WRONG_ARGS(API_RETURN_INT(0));

    string = ZSTR_VAL(z_string);
    regex = ZSTR_VAL(z_regex);

    result = weechat_string_has_highlight_regex ((const char *)string,
                                                 (const char *)regex);

    API_RETURN_INT(result);
}

API_FUNC(string_mask_to_regex)
{
    zend_string *z_mask;
    char *mask, *result;

    API_INIT_FUNC(1, "string_mask_to_regex", API_RETURN_EMPTY);

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_mask) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    mask = ZSTR_VAL(z_mask);

    result = weechat_string_mask_to_regex ((const char *)mask);

    API_RETURN_STRING_FREE(result);
}

API_FUNC(string_format_size)
{
    zend_long z_size;
    char *result;

    API_INIT_FUNC(1, "string_format_size", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "l", &z_size) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_string_format_size ((unsigned long long)z_size);

    API_RETURN_STRING_FREE(result);
}

API_FUNC(string_parse_size)
{
    zend_string *z_size;
    char *size;
    unsigned long long value;

    API_INIT_FUNC(1, "string_parse_size", API_RETURN_LONG(0));
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_size) == FAILURE)
        API_WRONG_ARGS(API_RETURN_LONG(0));

    size = ZSTR_VAL(z_size);

    value = weechat_string_parse_size ((const char *)size);

    API_RETURN_LONG(value);
}

API_FUNC(string_color_code_size)
{
    zend_string *z_string;
    char *string;
    int result;

    API_INIT_FUNC(1, "string_color_code_size", API_RETURN_INT(0));
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_string) == FAILURE)
        API_WRONG_ARGS(API_RETURN_INT(0));

    string = ZSTR_VAL(z_string);

    result = weechat_string_color_code_size ((const char *)string);

    API_RETURN_INT(result);
}

API_FUNC(string_remove_color)
{
    zend_string *z_string, *z_replacement;
    char *string, *replacement, *result;

    API_INIT_FUNC(1, "string_remove_color", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SS", &z_string,
                               &z_replacement) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    string = ZSTR_VAL(z_string);
    replacement = ZSTR_VAL(z_replacement);

    result = weechat_string_remove_color ((const char *)string,
                                          (const char *)replacement);

    API_RETURN_STRING_FREE(result);
}

API_FUNC(string_is_command_char)
{
    zend_string *z_string;
    char *string;
    int result;

    API_INIT_FUNC(1, "string_is_command_char", API_RETURN_INT(0));
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_string) == FAILURE)
        API_WRONG_ARGS(API_RETURN_INT(0));

    string = ZSTR_VAL(z_string);

    result = weechat_string_is_command_char ((const char *)string);

    API_RETURN_INT(result);
}

API_FUNC(string_input_for_buffer)
{
    zend_string *z_string;
    char *string;
    const char *result;

    API_INIT_FUNC(1, "string_input_for_buffer", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_string) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    string = ZSTR_VAL(z_string);

    result = weechat_string_input_for_buffer ((const char *)string);

    API_RETURN_STRING(result);
}

API_FUNC(string_eval_expression)
{
    zend_string *z_expr;
    zval *z_pointers, *z_extra_vars, *z_options;
    char *expr, *result;
    struct t_hashtable *pointers, *extra_vars, *options;

    API_INIT_FUNC(1, "string_eval_expression", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "Saaa", &z_expr, &z_pointers,
                               &z_extra_vars, &z_options) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    expr = ZSTR_VAL(z_expr);
    pointers = weechat_php_array_to_hashtable (
        z_pointers,
        WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_POINTER);
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

    result = weechat_string_eval_expression ((const char *)expr,
                                             pointers,
                                             extra_vars,
                                             options);

    if (pointers)
        weechat_hashtable_free (pointers);
    if (extra_vars)
        weechat_hashtable_free (extra_vars);
    if (options)
        weechat_hashtable_free (options);

    API_RETURN_STRING_FREE(result);
}

API_FUNC(string_eval_path_home)
{
    zend_string *z_path;
    zval *z_pointers, *z_extra_vars, *z_options;
    char *path, *result;
    struct t_hashtable *pointers, *extra_vars, *options;

    API_INIT_FUNC(1, "string_eval_path_home", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "Saaa",
                               &z_path, &z_pointers, &z_extra_vars,
                               &z_options) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    path = ZSTR_VAL(z_path);
    pointers = weechat_php_array_to_hashtable (
        z_pointers,
        WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_POINTER);
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

    result = weechat_string_eval_path_home ((const char *)path,
                                            pointers,
                                            extra_vars,
                                            options);

    if (pointers)
        weechat_hashtable_free (pointers);
    if (extra_vars)
        weechat_hashtable_free (extra_vars);
    if (options)
        weechat_hashtable_free (options);

    API_RETURN_STRING_FREE(result);
}

API_FUNC(mkdir_home)
{
    zend_string *z_directory;
    zend_long z_mode;
    char *directory;
    int mode;

    API_INIT_FUNC(1, "mkdir_home", API_RETURN_ERROR);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "Sl", &z_directory,
                               &z_mode) == FAILURE)
        API_WRONG_ARGS(API_RETURN_ERROR);

    directory = ZSTR_VAL(z_directory);
    mode = (int)z_mode;

    if (weechat_mkdir_home ((const char *)directory, mode))
        API_RETURN_OK;

    API_RETURN_ERROR;
}

API_FUNC(mkdir)
{
    zend_string *z_directory;
    zend_long z_mode;
    char *directory;
    int mode;

    API_INIT_FUNC(1, "mkdir", API_RETURN_ERROR);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "Sl", &z_directory,
                               &z_mode) == FAILURE)
        API_WRONG_ARGS(API_RETURN_ERROR);

    directory = ZSTR_VAL(z_directory);
    mode = (int)z_mode;

    if (weechat_mkdir ((const char *)directory, mode))
        API_RETURN_OK;

    API_RETURN_ERROR;
}

API_FUNC(mkdir_parents)
{
    zend_string *z_directory;
    zend_long z_mode;
    char *directory;
    int mode;

    API_INIT_FUNC(1, "mkdir_parents", API_RETURN_ERROR);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "Sl", &z_directory,
                               &z_mode) == FAILURE)
        API_WRONG_ARGS(API_RETURN_ERROR);

    directory = ZSTR_VAL(z_directory);
    mode = (int)z_mode;

    if (weechat_mkdir_parents ((const char *)directory, mode))
        API_RETURN_OK;

    API_RETURN_ERROR;
}

API_FUNC(list_new)
{
    const char *result;

    API_INIT_FUNC(1, "list_new", API_RETURN_EMPTY);
    if (zend_parse_parameters_none () == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_list_new ());

    API_RETURN_STRING(result);
}

API_FUNC(list_add)
{
    zend_string *z_weelist, *z_data, *z_where, *z_user_data;
    struct t_weelist *weelist;
    char *data, *where;
    void *user_data;
    const char *result;

    API_INIT_FUNC(1, "list_add", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SSSS", &z_weelist, &z_data,
                               &z_where, &z_user_data) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    weelist = (struct t_weelist *)API_STR2PTR(ZSTR_VAL(z_weelist));
    data = ZSTR_VAL(z_data);
    where = ZSTR_VAL(z_where);
    user_data = (void *)API_STR2PTR(ZSTR_VAL(z_user_data));

    result = API_PTR2STR(weechat_list_add (weelist,
                                           (const char *)data,
                                           (const char *)where,
                                           user_data));

    API_RETURN_STRING(result);
}

API_FUNC(list_search)
{
    zend_string *z_weelist, *z_data;
    struct t_weelist *weelist;
    char *data;
    const char *result;

    API_INIT_FUNC(1, "list_search", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SS", &z_weelist,
                               &z_data) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    weelist = (struct t_weelist *)API_STR2PTR(ZSTR_VAL(z_weelist));
    data = ZSTR_VAL(z_data);

    result = API_PTR2STR(weechat_list_search (weelist, (const char *)data));

    API_RETURN_STRING(result);
}

API_FUNC(list_search_pos)
{
    zend_string *z_weelist, *z_data;
    struct t_weelist *weelist;
    char *data;
    int result;

    API_INIT_FUNC(1, "list_search_pos", API_RETURN_INT(-1));
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SS", &z_weelist,
                               &z_data) == FAILURE)
        API_WRONG_ARGS(API_RETURN_INT(-1));

    weelist = (struct t_weelist *)API_STR2PTR(ZSTR_VAL(z_weelist));
    data = ZSTR_VAL(z_data);

    result = weechat_list_search_pos (weelist, (const char *)data);

    API_RETURN_INT(result);
}

API_FUNC(list_casesearch)
{
    zend_string *z_weelist, *z_data;
    struct t_weelist *weelist;
    char *data;
    const char *result;

    API_INIT_FUNC(1, "list_casesearch", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SS", &z_weelist,
                               &z_data) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    weelist = (struct t_weelist *)API_STR2PTR(ZSTR_VAL(z_weelist));
    data = ZSTR_VAL(z_data);

    result = API_PTR2STR(
        weechat_list_casesearch (weelist, (const char *)data));

    API_RETURN_STRING(result);
}

API_FUNC(list_casesearch_pos)
{
    zend_string *z_weelist, *z_data;
    struct t_weelist *weelist;
    char *data;
    int result;

    API_INIT_FUNC(1, "list_casesearch_pos", API_RETURN_INT(-1));
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SS", &z_weelist,
                               &z_data) == FAILURE)
        API_WRONG_ARGS(API_RETURN_INT(-1));

    weelist = (struct t_weelist *)API_STR2PTR(ZSTR_VAL(z_weelist));
    data = ZSTR_VAL(z_data);

    result = weechat_list_casesearch_pos (weelist, (const char *)data);

    API_RETURN_INT(result);
}

API_FUNC(list_get)
{
    zend_string *z_weelist;
    zend_long z_position;
    struct t_weelist *weelist;
    int position;
    const char *result;

    API_INIT_FUNC(1, "list_get", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "Sl", &z_weelist,
                               &z_position) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    weelist = (struct t_weelist *)API_STR2PTR(ZSTR_VAL(z_weelist));
    position = (int)z_position;

    result = API_PTR2STR(weechat_list_get (weelist, position));

    API_RETURN_STRING(result);
}

API_FUNC(list_set)
{
    zend_string *z_item, *z_value;
    struct t_weelist_item *item;
    char *value;

    API_INIT_FUNC(1, "list_set", API_RETURN_ERROR);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SS", &z_item,
                               &z_value) == FAILURE)
        API_WRONG_ARGS(API_RETURN_ERROR);

    item = (struct t_weelist_item *)API_STR2PTR(ZSTR_VAL(z_item));
    value = ZSTR_VAL(z_value);

    weechat_list_set (item, (const char *)value);

    API_RETURN_OK;
}

API_FUNC(list_next)
{
    zend_string *z_item;
    struct t_weelist_item *item;
    const char *result;

    API_INIT_FUNC(1, "list_next", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_item) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    item = (struct t_weelist_item *)API_STR2PTR(ZSTR_VAL(z_item));

    result = API_PTR2STR(weechat_list_next (item));

    API_RETURN_STRING(result);
}

API_FUNC(list_prev)
{
    zend_string *z_item;
    struct t_weelist_item *item;
    const char *result;

    API_INIT_FUNC(1, "list_prev", API_RETURN_EMPTY);

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_item) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    item = (struct t_weelist_item *)API_STR2PTR(ZSTR_VAL(z_item));

    result = API_PTR2STR(weechat_list_prev (item));

    API_RETURN_STRING(result);
}

API_FUNC(list_string)
{
    zend_string *z_item;
    const char *result;
    struct t_weelist_item *item;

    API_INIT_FUNC(1, "list_string", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_item) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    item = (struct t_weelist_item *)API_STR2PTR(ZSTR_VAL(z_item));

    result = weechat_list_string (item);

    API_RETURN_STRING(result);
}

API_FUNC(list_size)
{
    zend_string *z_weelist;
    struct t_weelist *weelist;
    int result;

    API_INIT_FUNC(1, "list_size", API_RETURN_INT(0));
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_weelist) == FAILURE)
        API_WRONG_ARGS(API_RETURN_INT(0));

    weelist = (struct t_weelist *)API_STR2PTR(ZSTR_VAL(z_weelist));

    result = weechat_list_size (weelist);

    API_RETURN_INT(result);
}

API_FUNC(list_remove)
{
    zend_string *z_weelist, *z_item;
    struct t_weelist *weelist;
    struct t_weelist_item *item;

    API_INIT_FUNC(1, "list_remove", API_RETURN_ERROR);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SS", &z_weelist,
                               &z_item) == FAILURE)
        API_WRONG_ARGS(API_RETURN_ERROR);

    weelist = (struct t_weelist *)API_STR2PTR(ZSTR_VAL(z_weelist));
    item = (struct t_weelist_item *)API_STR2PTR(ZSTR_VAL(z_item));

    weechat_list_remove (weelist, item);

    API_RETURN_OK;
}

API_FUNC(list_remove_all)
{
    zend_string *z_weelist;
    struct t_weelist *weelist;

    API_INIT_FUNC(1, "list_remove_all", API_RETURN_ERROR);

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_weelist) == FAILURE)
        API_WRONG_ARGS(API_RETURN_ERROR);

    weelist = (struct t_weelist *)API_STR2PTR(ZSTR_VAL(z_weelist));

    weechat_list_remove_all (weelist);

    API_RETURN_OK;
}

API_FUNC(list_free)
{
    zend_string *z_weelist;
    struct t_weelist *weelist;

    API_INIT_FUNC(1, "list_free", API_RETURN_ERROR);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_weelist) == FAILURE)
        API_WRONG_ARGS(API_RETURN_ERROR);

    weelist = (struct t_weelist *)API_STR2PTR(ZSTR_VAL(z_weelist));

    weechat_list_free (weelist);

    API_RETURN_OK;
}

static int
weechat_php_api_config_reload_cb (const void *pointer, void *data,
                                  struct t_config_file *config_file)
{
    int rc;
    void *func_argv[2];

    func_argv[1] = (char *)API_PTR2STR(config_file);

    weechat_php_cb (pointer, data, func_argv, "ss",
                    WEECHAT_SCRIPT_EXEC_INT, &rc);

    return rc;
}

API_FUNC(config_new)
{
    zend_string *z_name;
    zval *z_callback_reload;
    zend_string *z_data;
    char *name, *data;
    const char *result;

    API_INIT_FUNC(1, "config_new", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SzS", &z_name,
                               &z_callback_reload, &z_data) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    name = ZSTR_VAL(z_name);
    weechat_php_get_function_name (z_callback_reload, callback_reload_name);
    data = ZSTR_VAL(z_data);

    result = API_PTR2STR(
        plugin_script_api_config_new (
            weechat_php_plugin,
            php_current_script,
            (const char *)name,
            &weechat_php_api_config_reload_cb,
            (const char *)callback_reload_name,
            (const char *)data));

    API_RETURN_STRING(result);
}

struct t_hashtable *
weechat_php_api_config_update_cb (const void *pointer, void *data,
                                  struct t_config_file *config_file,
                                  int version_read,
                                  struct t_hashtable *data_read)
{
    struct t_hashtable *rc;
    void *func_argv[4];

    func_argv[1] = (char *)API_PTR2STR(config_file);
    func_argv[2] = &version_read;
    func_argv[3] = data_read;

    weechat_php_cb (pointer, data, func_argv, "ssih",
                    WEECHAT_SCRIPT_EXEC_HASHTABLE, &rc);

    return rc;
}

API_FUNC(config_set_version)
{
    zend_string *z_config_file;
    zend_long z_version;
    zval *z_callback_update;
    zend_string *z_data;
    struct t_config_file *config_file;
    char *data;
    int rc, version;

    API_INIT_FUNC(1, "config_set_version", API_RETURN_INT(0));
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SlzS", &z_config_file,
                               &z_version, &z_callback_update,
                               &z_data) == FAILURE)
        API_WRONG_ARGS(API_RETURN_INT(0));

    config_file = (struct t_config_file *)API_STR2PTR(ZSTR_VAL(z_config_file));
    version = (int)z_version;
    weechat_php_get_function_name (z_callback_update, callback_update_name);
    data = ZSTR_VAL(z_data);

    rc = plugin_script_api_config_set_version (
        weechat_php_plugin,
        php_current_script,
        config_file,
        version,
        &weechat_php_api_config_update_cb,
        (const char *)callback_update_name,
        (const char *)data);

    API_RETURN_INT(rc);
}

static int
weechat_php_api_config_section_read_cb (const void *pointer, void *data,
                                        struct t_config_file *config_file,
                                        struct t_config_section *section,
                                        const char *option_name,
                                        const char *value)
{
    int rc;
    void *func_argv[5];

    func_argv[1] = (char *)API_PTR2STR(config_file);
    func_argv[2] = (char *)API_PTR2STR(section);
    func_argv[3] = option_name ? (char *)option_name : weechat_php_empty_arg;
    func_argv[4] = value ? (char *)value : NULL;

    weechat_php_cb (pointer, data, func_argv, "sssss",
                    WEECHAT_SCRIPT_EXEC_INT, &rc);

    return rc;
}

static int
weechat_php_api_config_section_write_cb (const void *pointer, void *data,
                                         struct t_config_file *config_file,
                                         const char *section_name)
{
    int rc;
    void *func_argv[3];

    func_argv[1] = (char *)API_PTR2STR(config_file);
    func_argv[2] = section_name ? (char *)section_name : weechat_php_empty_arg;

    weechat_php_cb (pointer, data, func_argv, "sss",
                    WEECHAT_SCRIPT_EXEC_INT, &rc);

    return rc;
}

static int
weechat_php_api_config_section_write_default_cb (const void *pointer,
                                                 void *data,
                                                 struct t_config_file *config_file,
                                                 const char *section_name)
{
    int rc;
    void *func_argv[3];

    func_argv[1] = (char *)API_PTR2STR(config_file);
    func_argv[2] = section_name ? (char *)section_name : weechat_php_empty_arg;

    weechat_php_cb (pointer, data, func_argv, "sss",
                    WEECHAT_SCRIPT_EXEC_INT, &rc);

    return rc;
}

static int
weechat_php_api_config_section_create_option_cb (const void *pointer,
                                                 void *data,
                                                 struct t_config_file *config_file,
                                                 struct t_config_section *section,
                                                 const char *option_name,
                                                 const char *value)
{
    int rc;
    void *func_argv[5];

    func_argv[1] = (char *)API_PTR2STR(config_file);
    func_argv[2] = (char *)API_PTR2STR(section);
    func_argv[3] = option_name ? (char *)option_name : weechat_php_empty_arg;
    func_argv[4] = value ? (char *)value : NULL;

    weechat_php_cb (pointer, data, func_argv, "sssss",
                    WEECHAT_SCRIPT_EXEC_INT, &rc);

    return rc;
}

static int
weechat_php_api_config_section_delete_option_cb (const void *pointer,
                                                 void *data,
                                                 struct t_config_file *config_file,
                                                 struct t_config_section *section,
                                                 struct t_config_option *option)
{
    int rc;
    void *func_argv[4];

    func_argv[1] = (char *)API_PTR2STR(config_file);
    func_argv[2] = (char *)API_PTR2STR(section);
    func_argv[3] = (char *)API_PTR2STR(option);

    weechat_php_cb (pointer, data, func_argv, "ssss",
                    WEECHAT_SCRIPT_EXEC_INT, &rc);

    return rc;
}

API_FUNC(config_new_section)
{
    zend_string *z_config_file, *z_name;
    zend_long z_user_can_add_options, z_user_can_delete_options;
    zval *z_callback_read, *z_callback_write, *z_callback_write_default;
    zval *z_callback_create_option, *z_callback_delete_option;
    zend_string *z_data_read, *z_data_write, *z_data_write_default;
    zend_string *z_data_create_option, *z_data_delete_option;
    struct t_config_file *config_file;
    char *name;
    int user_can_add_options, user_can_delete_options;
    char *data_read, *data_write, *data_write_default;
    char *data_create_option, *data_delete_option;
    const char *result;

    API_INIT_FUNC(1, "config_new_section", API_RETURN_EMPTY);
    if (zend_parse_parameters (
            ZEND_NUM_ARGS(), "SSllzSzSzSzSzS", &z_config_file, &z_name,
            &z_user_can_add_options, &z_user_can_delete_options,
            &z_callback_read, &z_data_read, &z_callback_write, &z_data_write,
            &z_callback_write_default, &z_data_write_default,
            &z_callback_create_option, &z_data_create_option,
            &z_callback_delete_option, &z_data_delete_option) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

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

    result = API_PTR2STR(
        plugin_script_api_config_new_section (
            weechat_php_plugin,
            php_current_script,
            config_file,
            (const char *)name,
            user_can_add_options,
            user_can_delete_options,
            &weechat_php_api_config_section_read_cb,
            (const char *)callback_read_name,
            (const char *)data_read,
            &weechat_php_api_config_section_write_cb,
            (const char *)callback_write_name,
            (const char *)data_write,
            &weechat_php_api_config_section_write_default_cb,
            (const char *)callback_write_default_name,
            (const char *)data_write_default,
            &weechat_php_api_config_section_create_option_cb,
            (const char *)callback_create_option_name,
            (const char *)data_create_option,
            &weechat_php_api_config_section_delete_option_cb,
            (const char *)callback_delete_option_name,
            (const char *)data_delete_option));

    API_RETURN_STRING(result);
}

API_FUNC(config_search_section)
{
    zend_string *z_config_file, *z_section_name;
    struct t_config_file *config_file;
    char *section_name;
    const char *result;

    API_INIT_FUNC(1, "config_search_section", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SS", &z_config_file,
                               &z_section_name) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    config_file = (struct t_config_file *)API_STR2PTR(ZSTR_VAL(z_config_file));
    section_name = ZSTR_VAL(z_section_name);

    result = API_PTR2STR(
        weechat_config_search_section (config_file,
                                       (const char *)section_name));

    API_RETURN_STRING(result);
}

static int
weechat_php_api_config_option_check_value_cb (const void *pointer,
                                              void *data,
                                              struct t_config_option *option,
                                              const char *value)
{
    int rc;
    void *func_argv[3];

    func_argv[1] = (char *)API_PTR2STR(option);
    func_argv[2] = value ? (char *)value : weechat_php_empty_arg;

    weechat_php_cb (pointer, data, func_argv, "sss",
                    WEECHAT_SCRIPT_EXEC_INT, &rc);

    return rc;
}

static void
weechat_php_api_config_option_change_cb (const void *pointer,
                                         void *data,
                                         struct t_config_option *option)
{
    void *func_argv[2];

    func_argv[1] = (char *)API_PTR2STR(option);

    weechat_php_cb (pointer, data, func_argv, "ss",
                    WEECHAT_SCRIPT_EXEC_IGNORE, NULL);
}

static void
weechat_php_api_config_option_delete_cb (const void *pointer,
                                         void *data,
                                         struct t_config_option *option)
{
    void *func_argv[2];

    func_argv[1] = (char *)API_PTR2STR(option);

    weechat_php_cb (pointer, data, func_argv, "ss",
                    WEECHAT_SCRIPT_EXEC_IGNORE, NULL);
}

API_FUNC(config_new_option)
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
    const char *result;

    API_INIT_FUNC(1, "config_new_option", API_RETURN_EMPTY);
    if (zend_parse_parameters (
            ZEND_NUM_ARGS(), "SSSSSSllS!S!lzSzSzS", &z_config_file, &z_section,
            &z_name, &z_type, &z_description, &z_string_values, &z_min, &z_max,
            &z_default_value, &z_value, &z_null_value_allowed,
            &z_callback_check_value, &z_data_check_value, &z_callback_change,
            &z_data_change, &z_callback_delete, &z_data_delete) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    config_file = (struct t_config_file *)API_STR2PTR(ZSTR_VAL(z_config_file));
    section = (struct t_config_section *)API_STR2PTR(ZSTR_VAL(z_section));
    name = ZSTR_VAL(z_name);
    type = ZSTR_VAL(z_type);
    description = ZSTR_VAL(z_description);
    string_values = ZSTR_VAL(z_string_values);
    min = (int)z_min;
    max = (int)z_max;
    default_value = z_default_value ? ZSTR_VAL(z_default_value) : NULL;
    value = z_value ? ZSTR_VAL(z_value) : NULL;
    null_value_allowed = (int)z_null_value_allowed;
    weechat_php_get_function_name (z_callback_check_value,
                                   callback_check_value_name);
    data_check_value = ZSTR_VAL(z_data_check_value);
    weechat_php_get_function_name (z_callback_change, callback_change_name);
    data_change = ZSTR_VAL(z_data_change);
    weechat_php_get_function_name (z_callback_delete, callback_delete_name);
    data_delete = ZSTR_VAL(z_data_delete);

    result = API_PTR2STR(
        plugin_script_api_config_new_option (
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
            &weechat_php_api_config_option_check_value_cb,
            (const char *)callback_check_value_name,
            (const char *)data_check_value,
            &weechat_php_api_config_option_change_cb,
            (const char *)callback_change_name,
            (const char *)data_change,
            &weechat_php_api_config_option_delete_cb,
            (const char *)callback_delete_name,
            (const char *)data_delete));

    API_RETURN_STRING(result);
}

API_FUNC(config_search_option)
{
    zend_string *z_config_file, *z_section, *z_option_name;
    struct t_config_file *config_file;
    struct t_config_section *section;
    char *option_name;
    const char *result;

    API_INIT_FUNC(1, "config_search_option", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SSS",
                               &z_config_file, &z_section,
                               &z_option_name) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    config_file = (struct t_config_file *)API_STR2PTR(ZSTR_VAL(z_config_file));
    section = (struct t_config_section *)API_STR2PTR(ZSTR_VAL(z_section));
    option_name = ZSTR_VAL(z_option_name);

    result = API_PTR2STR(
        weechat_config_search_option (config_file, section,
                                      (const char *)option_name));

    API_RETURN_STRING(result);
}

API_FUNC(config_string_to_boolean)
{
    zend_string *z_text;
    char *text;
    int result;

    API_INIT_FUNC(1, "config_string_to_boolean", API_RETURN_INT(0));
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_text) == FAILURE)
        API_WRONG_ARGS(API_RETURN_INT(0));

    text = ZSTR_VAL(z_text);

    result = weechat_config_string_to_boolean ((const char *)text);

    API_RETURN_INT(result);
}

API_FUNC(config_option_reset)
{
    zend_string *z_option;
    zend_long z_run_callback;
    struct t_config_option *option;
    int run_callback, result;

    API_INIT_FUNC(1, "config_option_reset", API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "Sl",
                               &z_option, &z_run_callback) == FAILURE)
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));

    option = (struct t_config_option *)API_STR2PTR(ZSTR_VAL(z_option));
    run_callback = (int)z_run_callback;

    result = weechat_config_option_reset (option, run_callback);

    API_RETURN_INT(result);
}

API_FUNC(config_option_set)
{
    zend_string *z_option, *z_value;
    zend_long z_run_callback;
    struct t_config_option *option;
    char *value;
    int run_callback, result;

    API_INIT_FUNC(1, "config_option_set",  API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SSl",
                               &z_option, &z_value,
                               &z_run_callback) == FAILURE)
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));

    option = (struct t_config_option *)API_STR2PTR(ZSTR_VAL(z_option));
    value = ZSTR_VAL(z_value);
    run_callback = (int)z_run_callback;

    result = weechat_config_option_set (option, (const char *)value,
                                        run_callback);

    API_RETURN_INT(result);
}

API_FUNC(config_option_set_null)
{
    zend_string *z_option;
    zend_long z_run_callback;
    struct t_config_option *option;
    int run_callback, result;

    API_INIT_FUNC(1, "config_option_set_null", API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "Sl",
                               &z_option, &z_run_callback) == FAILURE)
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));

    option = (struct t_config_option *)API_STR2PTR(ZSTR_VAL(z_option));
    run_callback = (int)z_run_callback;

    result = weechat_config_option_set_null (option, run_callback);

    API_RETURN_INT(result);
}

API_FUNC(config_option_unset)
{
    zend_string *z_option;
    struct t_config_option *option;
    int result;

    API_INIT_FUNC(1, "config_option_unset", API_RETURN_INT(WEECHAT_CONFIG_OPTION_UNSET_ERROR));
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_option) == FAILURE)
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_OPTION_UNSET_ERROR));

    option = (struct t_config_option *)API_STR2PTR(ZSTR_VAL(z_option));

    result = weechat_config_option_unset (option);

    API_RETURN_INT(result);
}

API_FUNC(config_option_rename)
{
    zend_string *z_option, *z_new_name;
    struct t_config_option *option;
    char *new_name;

    API_INIT_FUNC(1, "config_option_rename", API_RETURN_ERROR);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SS",
                               &z_option, &z_new_name) == FAILURE)
        API_WRONG_ARGS(API_RETURN_ERROR);

    option = (struct t_config_option *)API_STR2PTR(ZSTR_VAL(z_option));
    new_name = ZSTR_VAL(z_new_name);

    weechat_config_option_rename (option, (const char *)new_name);

    API_RETURN_OK;
}

API_FUNC(config_option_is_null)
{
    zend_string *z_option;
    struct t_config_option *option;
    int result;

    API_INIT_FUNC(1, "config_option_is_null", API_RETURN_INT(1));
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_option) == FAILURE)
        API_WRONG_ARGS(API_RETURN_INT(1));

    option = (struct t_config_option *)API_STR2PTR(ZSTR_VAL(z_option));

    result = weechat_config_option_is_null (option);

    API_RETURN_INT(result);
}

API_FUNC(config_option_default_is_null)
{
    zend_string *z_option;
    struct t_config_option *option;
    int result;

    API_INIT_FUNC(1, "config_option_default_is_null", API_RETURN_INT(1));
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_option) == FAILURE)
        API_WRONG_ARGS(API_RETURN_INT(1));

    option = (struct t_config_option *)API_STR2PTR(ZSTR_VAL(z_option));

    result = weechat_config_option_default_is_null (option);

    API_RETURN_INT(result);
}

API_FUNC(config_boolean)
{
    zend_string *z_option;
    struct t_config_option *option;
    int result;

    API_INIT_FUNC(1, "config_boolean", API_RETURN_INT(0));
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_option) == FAILURE)
        API_WRONG_ARGS(API_RETURN_INT(0));

    option = (struct t_config_option *)API_STR2PTR(ZSTR_VAL(z_option));

    result = weechat_config_boolean (option);

    API_RETURN_INT(result);
}

API_FUNC(config_boolean_default)
{
    zend_string *z_option;
    struct t_config_option *option;
    int result;

    API_INIT_FUNC(1, "config_boolean_default", API_RETURN_INT(0));
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_option) == FAILURE)
        API_WRONG_ARGS(API_RETURN_INT(0));

    option = (struct t_config_option *)API_STR2PTR(ZSTR_VAL(z_option));

    result = weechat_config_boolean_default (option);

    API_RETURN_INT(result);
}

API_FUNC(config_integer)
{
    zend_string *z_option;
    struct t_config_option *option;
    int result;

    API_INIT_FUNC(1, "config_integer", API_RETURN_INT(0));
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_option) == FAILURE)
        API_WRONG_ARGS(API_RETURN_INT(0));

    option = (struct t_config_option *)API_STR2PTR(ZSTR_VAL(z_option));

    result = weechat_config_integer (option);

    API_RETURN_INT(result);
}

API_FUNC(config_integer_default)
{
    zend_string *z_option;
    struct t_config_option *option;
    int result;

    API_INIT_FUNC(1, "config_integer_default", API_RETURN_INT(0));
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_option) == FAILURE)
        API_WRONG_ARGS(API_RETURN_INT(0));

    option = (struct t_config_option *)API_STR2PTR(ZSTR_VAL(z_option));

    result = weechat_config_integer_default (option);

    API_RETURN_INT(result);
}

API_FUNC(config_string)
{
    zend_string *z_option;
    struct t_config_option *option;
    const char *result;

    API_INIT_FUNC(1, "config_string", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_option) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    option = (struct t_config_option *)API_STR2PTR(ZSTR_VAL(z_option));

    result = weechat_config_string (option);

    API_RETURN_STRING(result);
}

API_FUNC(config_string_default)
{
    zend_string *z_option;
    struct t_config_option *option;
    const char *result;

    API_INIT_FUNC(1, "config_string_default", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_option) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    option = (struct t_config_option *)API_STR2PTR(ZSTR_VAL(z_option));

    result = weechat_config_string_default (option);

    API_RETURN_STRING(result);
}

API_FUNC(config_color)
{
    zend_string *z_option;
    struct t_config_option *option;
    const char *result;

    API_INIT_FUNC(1, "config_color", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_option) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    option = (struct t_config_option *)API_STR2PTR(ZSTR_VAL(z_option));

    result = weechat_config_color (option);

    API_RETURN_STRING(result);
}

API_FUNC(config_color_default)
{
    zend_string *z_option;
    struct t_config_option *option;
    const char *result;

    API_INIT_FUNC(1, "config_color_default", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_option) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    option = (struct t_config_option *)API_STR2PTR(ZSTR_VAL(z_option));

    result = weechat_config_color_default (option);

    API_RETURN_STRING(result);
}

API_FUNC(config_write_option)
{
    zend_string *z_config_file, *z_option;
    struct t_config_file *config_file;
    struct t_config_option *option;

    API_INIT_FUNC(1, "config_write_option", API_RETURN_ERROR);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SS", &z_config_file,
                               &z_option) == FAILURE)
        API_WRONG_ARGS(API_RETURN_ERROR);

    config_file = (struct t_config_file *)API_STR2PTR(ZSTR_VAL(z_config_file));
    option = (struct t_config_option *)API_STR2PTR(ZSTR_VAL(z_option));

    weechat_config_write_option (config_file, option);

    API_RETURN_OK;
}

API_FUNC(config_write_line)
{
    zend_string *z_config_file, *z_option_name, *z_value;
    struct t_config_file *config_file;
    char *option_name, *value;

    API_INIT_FUNC(1, "config_write_line", API_RETURN_ERROR);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SSS", &z_config_file,
                               &z_option_name, &z_value) == FAILURE)
        API_WRONG_ARGS(API_RETURN_ERROR);

    config_file = (struct t_config_file *)API_STR2PTR(ZSTR_VAL(z_config_file));
    option_name = ZSTR_VAL(z_option_name);
    value = ZSTR_VAL(z_value);

    weechat_config_write_line (config_file,
                               (const char *)option_name,
                               (const char *)value);

    API_RETURN_OK;
}

API_FUNC(config_write)
{
    zend_string *z_config_file;
    struct t_config_file *config_file;
    int result;

    API_INIT_FUNC(1, "config_write", API_RETURN_INT(WEECHAT_CONFIG_WRITE_ERROR));
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S",
                               &z_config_file) == FAILURE)
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_WRITE_ERROR));

    config_file = (struct t_config_file *)API_STR2PTR(ZSTR_VAL(z_config_file));

    result = weechat_config_write (config_file);

    API_RETURN_INT(result);
}

API_FUNC(config_read)
{
    zend_string *z_config_file;
    struct t_config_file *config_file;
    int result;

    API_INIT_FUNC(1, "config_read", API_RETURN_INT(WEECHAT_CONFIG_READ_FILE_NOT_FOUND));
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S",
                               &z_config_file) == FAILURE)
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_READ_FILE_NOT_FOUND));

    config_file = (struct t_config_file *)API_STR2PTR(ZSTR_VAL(z_config_file));

    result = weechat_config_read (config_file);

    API_RETURN_INT(result);
}

API_FUNC(config_reload)
{
    zend_string *z_config_file;
    struct t_config_file *config_file;
    int result;

    API_INIT_FUNC(1, "config_reload", API_RETURN_INT(WEECHAT_CONFIG_READ_FILE_NOT_FOUND));
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S",
                               &z_config_file) == FAILURE)
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_READ_FILE_NOT_FOUND));

    config_file = (struct t_config_file *)API_STR2PTR(ZSTR_VAL(z_config_file));

    result = weechat_config_reload (config_file);

    API_RETURN_INT(result);
}

API_FUNC(config_option_free)
{
    zend_string *z_option;
    struct t_config_option *option;

    API_INIT_FUNC(1, "config_option_free", API_RETURN_ERROR);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_option) == FAILURE)
        API_WRONG_ARGS(API_RETURN_ERROR);

    option = (struct t_config_option *)API_STR2PTR(ZSTR_VAL(z_option));

    weechat_config_option_free (option);

    API_RETURN_OK;
}

API_FUNC(config_section_free_options)
{
    zend_string *z_section;
    struct t_config_section *section;

    API_INIT_FUNC(1, "config_section_free_options", API_RETURN_ERROR);

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_section) == FAILURE)
        API_WRONG_ARGS(API_RETURN_ERROR);

    section = (struct t_config_section *)API_STR2PTR(ZSTR_VAL(z_section));

    weechat_config_section_free_options (section);

    API_RETURN_OK;
}

API_FUNC(config_section_free)
{
    zend_string *z_section;
    struct t_config_section *section;

    API_INIT_FUNC(1, "config_section_free", API_RETURN_ERROR);

    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_section) == FAILURE)
        API_WRONG_ARGS(API_RETURN_ERROR);

    section = (struct t_config_section *)API_STR2PTR(ZSTR_VAL(z_section));

    weechat_config_section_free (section);

    API_RETURN_OK;
}

API_FUNC(config_free)
{
    zend_string *z_config_file;
    struct t_config_file *config_file;

    API_INIT_FUNC(1, "config_free", API_RETURN_ERROR);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S",
                               &z_config_file) == FAILURE)
        API_WRONG_ARGS(API_RETURN_ERROR);

    config_file = (struct t_config_file *)API_STR2PTR(ZSTR_VAL(z_config_file));

    weechat_config_free (config_file);

    API_RETURN_OK;
}

API_FUNC(config_get)
{
    zend_string *z_option_name;
    char *option_name;
    const char *result;

    API_INIT_FUNC(1, "config_get", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S",
                               &z_option_name) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    option_name = ZSTR_VAL(z_option_name);

    result = API_PTR2STR(weechat_config_get ((const char *)option_name));

    API_RETURN_STRING(result);
}

API_FUNC(config_get_plugin)
{
    zend_string *z_option;
    char *option;
    const char *result;

    API_INIT_FUNC(1, "config_get_plugin", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_option) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    option = ZSTR_VAL(z_option);

    result = plugin_script_api_config_get_plugin (weechat_php_plugin,
                                                  php_current_script,
                                                  (const char *)option);

    API_RETURN_STRING(result);
}

API_FUNC(config_is_set_plugin)
{
    zend_string *z_option;
    char *option;
    int result;

    API_INIT_FUNC(1, "config_is_set_plugin", API_RETURN_INT(0));
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_option) == FAILURE)
        API_WRONG_ARGS(API_RETURN_INT(0));

    option = ZSTR_VAL(z_option);

    result = plugin_script_api_config_is_set_plugin (weechat_php_plugin,
                                                     php_current_script,
                                                     (const char *)option);

    API_RETURN_INT(result);
}

API_FUNC(config_set_plugin)
{
    zend_string *z_option, *z_value;
    char *option, *value;
    int result;

    API_INIT_FUNC(1, "config_set_plugin", API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SS", &z_option,
                               &z_value) == FAILURE)
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));

    option = ZSTR_VAL(z_option);
    value = ZSTR_VAL(z_value);

    result = plugin_script_api_config_set_plugin (weechat_php_plugin,
                                                  php_current_script,
                                                  (const char *)option,
                                                  (const char *)value);

    API_RETURN_INT(result);
}

API_FUNC(config_set_desc_plugin)
{
    zend_string *z_option, *z_description;
    char *option, *description;

    API_INIT_FUNC(1, "config_set_desc_plugin", API_RETURN_ERROR);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SS", &z_option,
                               &z_description) == FAILURE)
        API_WRONG_ARGS(API_RETURN_ERROR);

    option = ZSTR_VAL(z_option);
    description = ZSTR_VAL(z_description);

    plugin_script_api_config_set_desc_plugin (weechat_php_plugin,
                                              php_current_script,
                                              (const char *)option,
                                              (const char *)description);

    API_RETURN_OK;
}

API_FUNC(config_unset_plugin)
{
    zend_string *z_option;
    char *option;
    int result;

    API_INIT_FUNC(1, "config_unset_plugin", API_RETURN_INT(WEECHAT_CONFIG_OPTION_UNSET_ERROR));
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_option) == FAILURE)
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_OPTION_UNSET_ERROR));

    option = ZSTR_VAL(z_option);

    result = plugin_script_api_config_unset_plugin (weechat_php_plugin,
                                                    php_current_script,
                                                    (const char *)option);

    API_RETURN_INT(result);
}

API_FUNC(key_bind)
{
    zend_string *z_context;
    zval *z_keys;
    char *context;
    struct t_hashtable *keys;
    int result;

    API_INIT_FUNC(1, "key_bind", API_RETURN_INT(0));
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "Sa", &z_context,
                               &z_keys) == FAILURE)
        API_WRONG_ARGS(API_RETURN_INT(0));

    context = ZSTR_VAL(z_context);
    keys = weechat_php_array_to_hashtable (z_keys,
                                           WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                           WEECHAT_HASHTABLE_STRING,
                                           WEECHAT_HASHTABLE_STRING);

    result = weechat_key_bind ((const char *)context, keys);

    if (keys)
        weechat_hashtable_free (keys);

    API_RETURN_INT(result);
}

API_FUNC(key_unbind)
{
    zend_string *z_context, *z_key;
    int result;
    char *context, *key;

    API_INIT_FUNC(1, "key_unbind", API_RETURN_INT(0));
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SS", &z_context,
                               &z_key) == FAILURE)
        API_WRONG_ARGS(API_RETURN_INT(0));

    context = ZSTR_VAL(z_context);
    key = ZSTR_VAL(z_key);

    result = weechat_key_unbind ((const char *)context, (const char *)key);

    API_RETURN_INT(result);
}

API_FUNC(prefix)
{
    zend_string *z_prefix;
    char *prefix;
    const char *result;

    API_INIT_FUNC(0, "prefix", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_prefix) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    prefix = ZSTR_VAL(z_prefix);

    result = weechat_prefix ((const char *)prefix);

    API_RETURN_STRING(result);
}

API_FUNC(color)
{
    zend_string *z_color_name;
    char *color_name;
    const char *result;

    API_INIT_FUNC(0, "color", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_color_name) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    color_name = ZSTR_VAL(z_color_name);

    result = weechat_color ((const char *)color_name);

    API_RETURN_STRING(result);
}

API_FUNC(print)
{
    zend_string *z_buffer, *z_message;
    struct t_gui_buffer *buffer;
    char *message;

    API_INIT_FUNC(0, "print", API_RETURN_ERROR);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SS", &z_buffer,
                               &z_message) == FAILURE)
        API_WRONG_ARGS(API_RETURN_ERROR);

    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));
    message = ZSTR_VAL(z_message);

    plugin_script_api_printf (weechat_php_plugin, php_current_script, buffer,
                              "%s", message);

    API_RETURN_OK;
}

API_FUNC(print_date_tags)
{
    zend_string *z_buffer, *z_tags, *z_message;
    zend_long z_date;
    struct t_gui_buffer *buffer;
    time_t date;
    char *tags, *message;

    API_INIT_FUNC(1, "print_date_tags", API_RETURN_ERROR);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SlSS", &z_buffer, &z_date,
                               &z_tags, &z_message) == FAILURE)
        API_WRONG_ARGS(API_RETURN_ERROR);

    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));
    date = (time_t)z_date;
    tags = ZSTR_VAL(z_tags);
    message = ZSTR_VAL(z_message);

    plugin_script_api_printf_date_tags (weechat_php_plugin,
                                        php_current_script,
                                        buffer,
                                        date,
                                        (const char *)tags,
                                        "%s",
                                        message);

    API_RETURN_OK;
}

API_FUNC(print_y)
{
    zend_string *z_buffer, *z_message;
    zend_long z_y;
    struct t_gui_buffer *buffer;
    int y;
    char *message;

    API_INIT_FUNC(1, "print_y", API_RETURN_ERROR);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SlS", &z_buffer, &z_y,
                               &z_message) == FAILURE)
        API_WRONG_ARGS(API_RETURN_ERROR);

    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));
    y = (int)z_y;
    message = ZSTR_VAL(z_message);

    plugin_script_api_printf_y (weechat_php_plugin,
                                php_current_script,
                                buffer,
                                y,
                                "%s",
                                message);

    API_RETURN_OK;
}

API_FUNC(print_y_date_tags)
{
    zend_string *z_buffer, *z_tags, *z_message;
    zend_long z_y, z_date;
    struct t_gui_buffer *buffer;
    int y;
    time_t date;
    char *tags, *message;

    API_INIT_FUNC(1, "print_y_date_tags", API_RETURN_ERROR);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SllSS", &z_buffer, &z_y,
                               &z_date, &z_tags, &z_message) == FAILURE)
        API_WRONG_ARGS(API_RETURN_ERROR);

    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));
    y = (int)z_y;
    date = (time_t)z_date;
    tags = ZSTR_VAL(z_tags);
    message = ZSTR_VAL(z_message);

    plugin_script_api_printf_y_date_tags (weechat_php_plugin,
                                          php_current_script,
                                          buffer,
                                          y,
                                          date,
                                          (const char *)tags,
                                          "%s",
                                          message);

    API_RETURN_OK;
}

API_FUNC(log_print)
{
    zend_string *z_message;
    char *message;

    API_INIT_FUNC(1, "log_print", API_RETURN_ERROR);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_message) == FAILURE)
        API_WRONG_ARGS(API_RETURN_ERROR);

    message = ZSTR_VAL(z_message);

    plugin_script_api_log_printf (weechat_php_plugin, php_current_script,
                                  "%s", message);

    API_RETURN_OK;
}

static int
weechat_php_api_hook_command_cb (const void *pointer, void *data,
                                 struct t_gui_buffer *buffer,
                                 int argc, char **argv, char **argv_eol)
{
    int rc;
    void *func_argv[3];

    /* make C compiler happy */
    (void) argv;

    func_argv[1] = (char *)API_PTR2STR(buffer);
    func_argv[2] = (argc > 1) ? argv_eol[1] : weechat_php_empty_arg;

    weechat_php_cb (pointer, data, func_argv, "sss",
                    WEECHAT_SCRIPT_EXEC_INT, &rc);

    return rc;
}

API_FUNC(hook_command)
{
    zend_string *z_command, *z_description, *z_args, *z_args_description;
    zend_string *z_completion, *z_data;
    zval *z_callback;
    char *command, *description, *args, *args_description, *completion, *data;
    const char *result;

    API_INIT_FUNC(1, "hook_command", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SSSSSzS", &z_command,
                               &z_description, &z_args, &z_args_description,
                               &z_completion, &z_callback, &z_data) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    command = ZSTR_VAL(z_command);
    description = ZSTR_VAL(z_description);
    args = ZSTR_VAL(z_args);
    args_description = ZSTR_VAL(z_args_description);
    completion = ZSTR_VAL(z_completion);
    weechat_php_get_function_name (z_callback, callback_name);
    data = ZSTR_VAL(z_data);

    result = API_PTR2STR(
        plugin_script_api_hook_command (
            weechat_php_plugin,
            php_current_script,
            (const char *)command,
            (const char *)description,
            (const char *)args,
            (const char *)args_description,
            (const char *)completion,
            &weechat_php_api_hook_command_cb,
            (const char *)callback_name,
            (const char *)data));

    API_RETURN_STRING(result);
}

static int
weechat_php_api_hook_completion_cb (const void *pointer, void *data,
                                    const char *completion_item,
                                    struct t_gui_buffer *buffer,
                                    struct t_gui_completion *completion)
{
    int rc;
    void *func_argv[4];

    func_argv[1] = completion_item ? (char *)completion_item : weechat_php_empty_arg;
    func_argv[2] = (char *)API_PTR2STR(buffer);
    func_argv[3] = (char *)API_PTR2STR(completion);

    weechat_php_cb (pointer, data, func_argv, "ssss",
                    WEECHAT_SCRIPT_EXEC_INT, &rc);

    return rc;
}

API_FUNC(hook_completion)
{
    zend_string *z_completion, *z_description, *z_data;
    zval *z_callback;
    char *completion, *description, *data;
    const char *result;

    API_INIT_FUNC(1, "hook_completion", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SSzS",
                               &z_completion, &z_description, &z_callback,
                               &z_data) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    completion = ZSTR_VAL(z_completion);
    description = ZSTR_VAL(z_description);
    weechat_php_get_function_name (z_callback, callback_name);
    data = ZSTR_VAL(z_data);

    result = API_PTR2STR(
        plugin_script_api_hook_completion (
            weechat_php_plugin,
            php_current_script,
            (const char *)completion,
            (const char *)description,
            &weechat_php_api_hook_completion_cb,
            (const char *)callback_name,
            (const char *)data));

    API_RETURN_STRING(result);
}

/*
 * This function deprecated since WeeChat 2.9, kept for compatibility.
 * It is replaced by completion_get_string.
 */

API_FUNC(hook_completion_get_string)
{
    zend_string *z_completion, *z_property;
    struct t_gui_completion *completion;
    char *property;
    const char *result;

    API_INIT_FUNC(1, "hook_completion_get_string", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SS", &z_completion,
                               &z_property) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    completion = (struct t_gui_completion *)API_STR2PTR(ZSTR_VAL(z_completion));
    property = ZSTR_VAL(z_property);

    result = weechat_hook_completion_get_string (completion,
                                                 (const char *)property);

    API_RETURN_STRING(result);
}

/*
 * This function deprecated since WeeChat 2.9, kept for compatibility.
 * It is replaced by completion_list_add.
 */

API_FUNC(hook_completion_list_add)
{
    zend_string *z_completion, *z_word, *z_where;
    zend_long z_nick_completion;
    struct t_gui_completion *completion;
    char *word, *where;
    int nick_completion;

    API_INIT_FUNC(1, "hook_completion_list_add", API_RETURN_ERROR);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SSlS", &z_completion,
                               &z_word, &z_nick_completion, &z_where) == FAILURE)
        API_WRONG_ARGS(API_RETURN_ERROR);

    completion = (struct t_gui_completion *)API_STR2PTR(ZSTR_VAL(z_completion));
    word = ZSTR_VAL(z_word);
    nick_completion = (int)z_nick_completion;
    where = ZSTR_VAL(z_where);

    weechat_hook_completion_list_add (completion,
                                      (const char *)word,
                                      nick_completion,
                                      (const char *)where);

    API_RETURN_OK;
}

static int
weechat_php_api_hook_command_run_cb (const void *pointer, void *data,
                                     struct t_gui_buffer *buffer,
                                     const char *command)
{
    int rc;
    void *func_argv[3];

    func_argv[1] = (char *)API_PTR2STR(buffer);
    func_argv[2] = command ? (char *)command : weechat_php_empty_arg;

    weechat_php_cb (pointer, data, func_argv, "sss",
                    WEECHAT_SCRIPT_EXEC_INT, &rc);

    return rc;
}

API_FUNC(hook_command_run)
{
    zend_string *z_command, *z_data;
    zval *z_callback;
    char *command, *data;
    const char *result;

    API_INIT_FUNC(1, "hook_command_run", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SzS", &z_command, &z_callback,
                               &z_data) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    command = ZSTR_VAL(z_command);
    weechat_php_get_function_name (z_callback, callback_name);
    data = ZSTR_VAL(z_data);

    result = API_PTR2STR(
        plugin_script_api_hook_command_run (
            weechat_php_plugin,
            php_current_script,
            (const char *)command,
            &weechat_php_api_hook_command_run_cb,
            (const char *)callback_name,
            (const char *)data));

    API_RETURN_STRING(result);
}

static int
weechat_php_api_hook_timer_cb (const void *pointer, void *data,
                               int remaining_calls)
{
    int rc;
    void *func_argv[2];

    func_argv[1] = &remaining_calls;

    weechat_php_cb (pointer, data, func_argv, "si",
                    WEECHAT_SCRIPT_EXEC_INT, &rc);

    return rc;
}

API_FUNC(hook_timer)
{
    zend_long z_interval, z_align_second, z_max_calls;
    zval *z_callback;
    zend_string *z_data;
    long interval;
    int align_second, max_calls;
    char *data;
    const char *result;

    API_INIT_FUNC(1, "hook_timer", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "lllzS", &z_interval,
                               &z_align_second, &z_max_calls, &z_callback,
                               &z_data) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    interval = (long)z_interval;
    align_second = (int)z_align_second;
    max_calls = (int)z_max_calls;
    weechat_php_get_function_name (z_callback, callback_name);
    data = ZSTR_VAL(z_data);

    result = API_PTR2STR(
        plugin_script_api_hook_timer (
            weechat_php_plugin,
            php_current_script,
            interval,
            align_second,
            max_calls,
            &weechat_php_api_hook_timer_cb,
            (const char *)callback_name,
            (const char *)data));

    API_RETURN_STRING(result);
}

static int
weechat_php_api_hook_fd_cb (const void *pointer, void *data,
                            int fd)
{
    int rc;
    void *func_argv[2];

    func_argv[1] = &fd;

    weechat_php_cb (pointer, data, func_argv, "si",
                    WEECHAT_SCRIPT_EXEC_INT, &rc);

    return rc;
}

API_FUNC(hook_fd)
{
    zend_long z_fd, z_flag_read, z_flag_write, z_flag_exception;
    zval *z_callback;
    zend_string *z_data;
    int fd, flag_read, flag_write, flag_exception;
    char *data;
    const char *result;

    API_INIT_FUNC(1, "hook_fd", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "llllzS", &z_fd, &z_flag_read,
                               &z_flag_write, &z_flag_exception, &z_callback,
                               &z_data) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    fd = (int)z_fd;
    flag_read = (int)z_flag_read;
    flag_write = (int)z_flag_write;
    flag_exception = (int)z_flag_exception;
    weechat_php_get_function_name (z_callback, callback_name);
    data = ZSTR_VAL(z_data);

    result = API_PTR2STR(
        plugin_script_api_hook_fd (
            weechat_php_plugin,
            php_current_script, fd,
            flag_read,
            flag_write,
            flag_exception,
            &weechat_php_api_hook_fd_cb,
            (const char *)callback_name,
            (const char *)data));

    API_RETURN_STRING(result);
}

static int
weechat_php_api_hook_process_cb (const void *pointer, void *data,
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

API_FUNC(hook_process)
{
    zend_string *z_command, *z_data;
    zend_long z_timeout;
    zval *z_callback;
    char *command, *data;
    int timeout;
    const char *result;

    API_INIT_FUNC(1, "hook_process", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SlzS", &z_command, &z_timeout,
                               &z_callback, &z_data) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    command = ZSTR_VAL(z_command);
    timeout = (int)z_timeout;
    weechat_php_get_function_name (z_callback, callback_name);
    data = ZSTR_VAL(z_data);

    result = API_PTR2STR(
        plugin_script_api_hook_process (
            weechat_php_plugin,
            php_current_script,
            (const char *)command,
            timeout,
            &weechat_php_api_hook_process_cb,
            (const char *)callback_name,
            (const char *)data));

    API_RETURN_STRING(result);
}

static int
weechat_php_api_hook_process_hashtable_cb (const void *pointer, void *data,
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

API_FUNC(hook_process_hashtable)
{
    zend_string *z_command, *z_data;
    zval *z_options, *z_callback;
    zend_long z_timeout;
    char *command, *data;
    struct t_hashtable *options;
    int timeout;
    const char *result;

    API_INIT_FUNC(1, "hook_process_hashtable", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SalzS", &z_command,
                               &z_options, &z_timeout, &z_callback,
                               &z_data) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    command = ZSTR_VAL(z_command);
    options = weechat_php_array_to_hashtable (
        z_options,
        WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING);
    timeout = (int)z_timeout;
    weechat_php_get_function_name (z_callback, callback_name);
    data = ZSTR_VAL(z_data);

    result = API_PTR2STR(
        plugin_script_api_hook_process_hashtable (
            weechat_php_plugin,
            php_current_script,
            (const char *)command,
            options,
            timeout,
            &weechat_php_api_hook_process_hashtable_cb,
            (const char *)callback_name,
            (const char *)data));

    if (options)
        weechat_hashtable_free (options);

    API_RETURN_STRING(result);
}

static int
weechat_php_api_hook_connect_cb (const void *pointer, void *data, int status,
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

API_FUNC(hook_connect)
{
    zend_string *z_proxy, *z_address, *z_gnutls_sess, *z_gnutls_cb;
    zend_string *z_gnutls_priorities, *z_local_hostname, *z_data;
    zend_long z_port, z_ipv6, z_retry, z_gnutls_dhkey_size;
    zval *z_callback;
    char *proxy, *address, *gnutls_priorities, *local_hostname, *data;
    int port, ipv6, retry;
    void *gnutls_sess, *gnutls_cb;
    int gnutls_dhkey_size;
    const char *result;

    API_INIT_FUNC(1, "hook_connect", API_RETURN_EMPTY);
    if (zend_parse_parameters (
            ZEND_NUM_ARGS(), "SSlllSSlSSzS", &z_proxy, &z_address, &z_port,
            &z_ipv6, &z_retry, &z_gnutls_sess, &z_gnutls_cb,
            &z_gnutls_dhkey_size, &z_gnutls_priorities, &z_local_hostname,
            &z_callback, &z_data) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

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

    result = API_PTR2STR(
        plugin_script_api_hook_connect (
            weechat_php_plugin,
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
            &weechat_php_api_hook_connect_cb,
            (const char *)callback_name,
            (const char *)data));

    API_RETURN_STRING(result);
}

struct t_hashtable *
weechat_php_api_hook_line_cb (const void *pointer, void *data,
                              struct t_hashtable *line)
{
    struct t_hashtable *rc;
    void *func_argv[2];

    func_argv[1] = line;

    weechat_php_cb (pointer, data, func_argv, "sh",
                    WEECHAT_SCRIPT_EXEC_HASHTABLE, &rc);

    return rc;
}

API_FUNC(hook_line)
{
    zend_string *z_buffer_type, *z_buffer_name, *z_tags, *z_data;
    zval *z_callback;
    char *buffer_type, *buffer_name, *tags, *data;
    const char *result;

    API_INIT_FUNC(1, "hook_line", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SSSzS",
                               &z_buffer_type, &z_buffer_name, &z_tags,
                               &z_callback, &z_data) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    weechat_php_get_function_name (z_callback, callback_name);
    buffer_type = ZSTR_VAL(z_buffer_type);
    buffer_name = ZSTR_VAL(z_buffer_name);
    tags = ZSTR_VAL(z_tags);
    data = ZSTR_VAL(z_data);

    result = API_PTR2STR(
        plugin_script_api_hook_line (
            weechat_php_plugin,
            php_current_script,
            (const char *)buffer_type,
            (const char *)buffer_name,
            (const char *)tags,
            &weechat_php_api_hook_line_cb,
            (const char *)callback_name,
            (const char *)data));

    API_RETURN_STRING(result);
}

static int
weechat_php_api_hook_print_cb (const void *pointer, void *data,
                               struct t_gui_buffer *buffer, time_t date,
                               int tags_count, const char **tags,
                               int displayed, int highlight,
                               const char *prefix, const char *message)
{
    int rc;
    void *func_argv[9];

    func_argv[1] = (char *)API_PTR2STR(buffer);
    func_argv[2] = &date;
    func_argv[3] = &tags_count;
    func_argv[4] = tags ? (char *)tags : weechat_php_empty_arg;
    func_argv[5] = &displayed;
    func_argv[6] = &highlight;
    func_argv[7] = prefix ? (char *)prefix : weechat_php_empty_arg;
    func_argv[8] = message ? (char *)message : weechat_php_empty_arg;

    weechat_php_cb (pointer, data, func_argv, "ssiisiiss",
                    WEECHAT_SCRIPT_EXEC_INT, &rc);

    return rc;
}

API_FUNC(hook_print)
{
    zend_string *z_buffer, *z_tags, *z_message, *z_data;
    zend_long z_strip_colors;
    zval *z_callback;
    struct t_gui_buffer *buffer;
    char *tags, *message, *data;
    int strip_colors;
    const char *result;

    API_INIT_FUNC(1, "hook_print", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SSSlzS", &z_buffer, &z_tags,
                               &z_message, &z_strip_colors, &z_callback,
                               &z_data) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));
    tags = ZSTR_VAL(z_tags);
    message = ZSTR_VAL(z_message);
    strip_colors = (int)z_strip_colors;
    weechat_php_get_function_name (z_callback, callback_name);
    data = ZSTR_VAL(z_data);

    result = API_PTR2STR(
        plugin_script_api_hook_print (
            weechat_php_plugin,
            php_current_script,
            buffer,
            (const char *)tags,
            (const char *)message,
            strip_colors,
            &weechat_php_api_hook_print_cb,
            (const char *)callback_name,
            (const char *)data));

    API_RETURN_STRING(result);
}

static int
weechat_php_api_hook_signal_cb (const void *pointer, void *data,
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

API_FUNC(hook_signal)
{
    zend_string *z_signal, *z_data;
    zval *z_callback;
    char *signal, *data;
    const char *result;

    API_INIT_FUNC(1, "hook_signal", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SzS", &z_signal, &z_callback,
                               &z_data) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    signal = ZSTR_VAL(z_signal);
    weechat_php_get_function_name (z_callback, callback_name);
    data = ZSTR_VAL(z_data);

    result = API_PTR2STR(
        plugin_script_api_hook_signal (
            weechat_php_plugin,
            php_current_script,
            (const char *)signal,
            &weechat_php_api_hook_signal_cb,
            (const char *)callback_name,
            (const char *)data));

    API_RETURN_STRING(result);
}

API_FUNC(hook_signal_send)
{
    zend_string *z_signal, *z_type_data, *z_signal_data;
    char *signal, *type_data;
    void *signal_data;
    int result;

    API_INIT_FUNC(1, "hook_signal_send", API_RETURN_INT(WEECHAT_RC_ERROR));
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SSS", &z_signal, &z_type_data,
                               &z_signal_data) == FAILURE)
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_RC_ERROR));

    signal = ZSTR_VAL(z_signal);
    type_data = ZSTR_VAL(z_type_data);
    signal_data = (void *)API_STR2PTR(ZSTR_VAL(z_signal_data));
    result = weechat_hook_signal_send ((const char *)signal,
                                       (const char *)type_data, signal_data);

    API_RETURN_INT(result);
}

static int
weechat_php_api_hook_hsignal_cb (const void *pointer, void *data,
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

API_FUNC(hook_hsignal)
{
    zend_string *z_signal, *z_data;
    zval *z_callback;
    char *signal, *data;
    const char *result;

    API_INIT_FUNC(1, "hook_hsignal", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SzS", &z_signal, &z_callback,
                               &z_data) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    signal = ZSTR_VAL(z_signal);
    weechat_php_get_function_name (z_callback, callback_name);
    data = ZSTR_VAL(z_data);

    result = API_PTR2STR(
        plugin_script_api_hook_hsignal (
            weechat_php_plugin,
            php_current_script,
            (const char *)signal,
            &weechat_php_api_hook_hsignal_cb,
            (const char *)callback_name,
            (const char *)data));

    API_RETURN_STRING(result);
}

API_FUNC(hook_hsignal_send)
{
    zend_string *z_signal;
    zval *z_hashtable;
    char *signal;
    struct t_hashtable *hashtable;
    int result;

    API_INIT_FUNC(1, "hook_hsignal_send", API_RETURN_INT(WEECHAT_RC_ERROR));
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "Sa", &z_signal,
                               &z_hashtable) == FAILURE)
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_RC_ERROR));

    signal = ZSTR_VAL(z_signal);
    hashtable = weechat_php_array_to_hashtable (
        z_hashtable,
        WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING);

    result = weechat_hook_hsignal_send ((const char *)signal, hashtable);

    if (hashtable)
        weechat_hashtable_free (hashtable);

    API_RETURN_INT(result);
}

static int
weechat_php_api_hook_config_cb (const void *pointer, void *data,
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

API_FUNC(hook_config)
{
    zend_string *z_option, *z_data;
    zval *z_callback;
    char *option, *data;
    const char *result;

    API_INIT_FUNC(1, "hook_config", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SzS", &z_option, &z_callback,
                               &z_data) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    option = ZSTR_VAL(z_option);
    weechat_php_get_function_name (z_callback, callback_name);
    data = ZSTR_VAL(z_data);

    result = API_PTR2STR(
        plugin_script_api_hook_config (
            weechat_php_plugin,
            php_current_script,
            (const char *)option,
            &weechat_php_api_hook_config_cb,
            (const char *)callback_name,
            (const char *)data));

    API_RETURN_STRING(result);
}

static char *
weechat_php_api_hook_modifier_cb (const void *pointer, void *data,
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

API_FUNC(hook_modifier)
{
    zend_string *z_modifier, *z_data;
    zval *z_callback;
    char *modifier, *data;
    const char *result;

    API_INIT_FUNC(1, "hook_modifier", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SzS", &z_modifier,
                               &z_callback, &z_data) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    modifier = ZSTR_VAL(z_modifier);
    weechat_php_get_function_name (z_callback, callback_name);
    data = ZSTR_VAL(z_data);

    result = API_PTR2STR(
        plugin_script_api_hook_modifier (
            weechat_php_plugin,
            php_current_script,
            (const char *)modifier,
            &weechat_php_api_hook_modifier_cb,
            (const char *)callback_name,
            (const char *)data));

    API_RETURN_STRING(result);
}

API_FUNC(hook_modifier_exec)
{
    zend_string *z_modifier, *z_modifier_data, *z_string;
    char *modifier, *modifier_data, *string, *result;

    API_INIT_FUNC(1, "hook_modifier_exec", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SSS", &z_modifier,
                               &z_modifier_data, &z_string) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    modifier = ZSTR_VAL(z_modifier);
    modifier_data = ZSTR_VAL(z_modifier_data);
    string = ZSTR_VAL(z_string);

    result = weechat_hook_modifier_exec ((const char *)modifier,
                                         (const char *)modifier_data,
                                         (const char *)string);

    API_RETURN_STRING_FREE(result);
}

static char *
weechat_php_api_hook_info_cb (const void *pointer,
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

API_FUNC(hook_info)
{
    zend_string *z_info_name, *z_description, *z_args_description, *z_data;
    zval *z_callback;
    char *info_name, *description, *args_description, *data;
    const char *result;

    API_INIT_FUNC(1, "hook_info", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SSSzS", &z_info_name,
                               &z_description, &z_args_description,
                               &z_callback, &z_data) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    info_name = ZSTR_VAL(z_info_name);
    description = ZSTR_VAL(z_description);
    args_description = ZSTR_VAL(z_args_description);
    weechat_php_get_function_name (z_callback, callback_name);
    data = ZSTR_VAL(z_data);

    result = API_PTR2STR(
        plugin_script_api_hook_info (
            weechat_php_plugin,
            php_current_script,
            (const char *)info_name,
            (const char *)description,
            (const char *)args_description,
            &weechat_php_api_hook_info_cb,
            (const char *)callback_name,
            (const char *)data));

    API_RETURN_STRING(result);
}

struct t_hashtable *
weechat_php_api_hook_info_hashtable_cb (const void *pointer, void *data,
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

API_FUNC(hook_info_hashtable)
{
    zend_string *z_info_name, *z_description, *z_args_description;
    zend_string *z_output_description, *z_data;
    zval *z_callback;
    char *info_name, *description, *args_description, *output_description;
    char *data;
    const char *result;

    API_INIT_FUNC(1, "hook_info_hashtable", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SSSSSzS", &z_info_name,
                               &z_description, &z_args_description,
                               &z_output_description, &z_callback,
                               &z_data) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    info_name = ZSTR_VAL(z_info_name);
    description = ZSTR_VAL(z_description);
    args_description = ZSTR_VAL(z_args_description);
    output_description = ZSTR_VAL(z_output_description);
    data = ZSTR_VAL(z_data);
    weechat_php_get_function_name (z_callback, callback_name);

    result = API_PTR2STR(
        plugin_script_api_hook_info_hashtable (
            weechat_php_plugin,
            php_current_script,
            info_name,
            description,
            args_description,
            output_description,
            &weechat_php_api_hook_info_hashtable_cb,
            callback_name,
            data));

    API_RETURN_STRING(result);
}

struct t_infolist *
weechat_php_api_hook_infolist_cb (const void *pointer, void *data,
                                  const char *info_name,
                                  void *obj_pointer,
                                  const char *arguments)
{
    struct t_infolist *rc;
    void *func_argv[4];

    func_argv[1] = (info_name) ? (char *)info_name : weechat_php_empty_arg;
    func_argv[2] = (char *)API_PTR2STR(obj_pointer);
    func_argv[3] = (arguments) ? (char *)arguments : weechat_php_empty_arg;

    weechat_php_cb (pointer, data, func_argv, "ssss",
                    WEECHAT_SCRIPT_EXEC_POINTER, &rc);

    return rc;
}

API_FUNC(hook_infolist)
{
    zend_string *z_infolist_name, *z_description, *z_pointer_description;
    zend_string *z_args_description, *z_data;
    zval *z_callback;
    char *infolist_name, *description, *pointer_description, *args_description;
    char *data;
    const char *result;

    API_INIT_FUNC(1, "hook_infolist", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SSSSzS", &z_infolist_name,
                               &z_description, &z_pointer_description,
                               &z_args_description, &z_callback,
                               &z_data) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    infolist_name = ZSTR_VAL(z_infolist_name);
    description = ZSTR_VAL(z_description);
    pointer_description = ZSTR_VAL(z_pointer_description);
    args_description = ZSTR_VAL(z_args_description);
    data = ZSTR_VAL(z_data);
    weechat_php_get_function_name (z_callback, callback_name);

    result = API_PTR2STR(
        plugin_script_api_hook_infolist (
            weechat_php_plugin,
            php_current_script,
            infolist_name,
            description,
            pointer_description,
            args_description,
            &weechat_php_api_hook_infolist_cb,
            callback_name,
            data));

    API_RETURN_STRING(result);
}

struct t_hashtable *
weechat_php_api_hook_focus_cb (const void *pointer, void *data,
                               struct t_hashtable *info)
{
    struct t_hashtable *rc;
    void *func_argv[2];

    func_argv[1] = info;

    weechat_php_cb (pointer, data, func_argv, "sh",
                    WEECHAT_SCRIPT_EXEC_HASHTABLE, &rc);

    return rc;
}

API_FUNC(hook_focus)
{
    zend_string *z_area, *z_data;
    zval *z_callback;
    char *area, *data;
    const char *result;

    API_INIT_FUNC(1, "hook_focus", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SzS", &z_area, &z_callback,
                               &z_data) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    area = ZSTR_VAL(z_area);
    data = ZSTR_VAL(z_data);
    weechat_php_get_function_name (z_callback, callback_name);

    result = API_PTR2STR(
        plugin_script_api_hook_focus (
            weechat_php_plugin,
            php_current_script,
            area,
            &weechat_php_api_hook_focus_cb,
            callback_name,
            data));

    API_RETURN_STRING(result);
}

API_FUNC(hook_set)
{
    zend_string *z_hook, *z_property, *z_value;
    struct t_hook *hook;
    char *property, *value;

    API_INIT_FUNC(1, "hook_set", API_RETURN_ERROR);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SSS", &z_hook, &z_property,
                               &z_value) == FAILURE)
        API_WRONG_ARGS(API_RETURN_ERROR);

    hook = (struct t_hook *)API_STR2PTR(ZSTR_VAL(z_hook));
    property = ZSTR_VAL(z_property);
    value = ZSTR_VAL(z_value);

    weechat_hook_set (hook, (const char *)property, (const char *)value);

    API_RETURN_OK;
}

API_FUNC(unhook)
{
    zend_string *z_hook;
    struct t_hook *hook;

    API_INIT_FUNC(1, "unhook", API_RETURN_ERROR);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_hook) == FAILURE)
        API_WRONG_ARGS(API_RETURN_ERROR);

    hook = (struct t_hook *)API_STR2PTR(ZSTR_VAL(z_hook));

    weechat_unhook (hook);

    API_RETURN_OK;
}

API_FUNC(unhook_all)
{
    zend_string *z_subplugin;
    char *subplugin;

    API_INIT_FUNC(1, "unhook_all", API_RETURN_ERROR);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_subplugin) == FAILURE)
        API_WRONG_ARGS(API_RETURN_ERROR);

    subplugin = ZSTR_VAL(z_subplugin);

    weechat_unhook_all ((const char *)subplugin);

    API_RETURN_OK;
}

int
weechat_php_api_buffer_input_data_cb (const void *pointer, void *data,
                                      struct t_gui_buffer *buffer,
                                      const char *input_data)
{
    int rc;
    void *func_argv[3];

    func_argv[1] = (char *)API_PTR2STR(buffer);
    func_argv[2] = input_data ? (char *)input_data : weechat_php_empty_arg;

    weechat_php_cb (pointer, data, func_argv, "sss",
                    WEECHAT_SCRIPT_EXEC_INT, &rc);

    return rc;
}

int
weechat_php_api_buffer_close_cb (const void *pointer, void *data,
                                 struct t_gui_buffer *buffer)
{
    int rc;
    void *func_argv[2];

    func_argv[1] = (char *)API_PTR2STR(buffer);

    weechat_php_cb (pointer, data, func_argv, "ss",
                    WEECHAT_SCRIPT_EXEC_INT, &rc);

    return rc;
}

API_FUNC(buffer_new)
{
    zend_string *z_name, *z_data_input, *z_data_close;
    zval *z_input_callback, *z_close_callback;
    char *name, *data_input, *data_close;
    const char *result;

    API_INIT_FUNC(1, "buffer_new", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SzSzS", &z_name,
                               &z_input_callback, &z_data_input,
                               &z_close_callback, &z_data_close) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    name = ZSTR_VAL(z_name);
    weechat_php_get_function_name (z_input_callback, input_callback_name);
    data_input = ZSTR_VAL(z_data_input);
    weechat_php_get_function_name (z_close_callback, close_callback_name);
    data_close = ZSTR_VAL(z_data_close);

    result = API_PTR2STR(
        plugin_script_api_buffer_new (
            weechat_php_plugin,
            php_current_script,
            (const char *)name,
            &weechat_php_api_buffer_input_data_cb,
            (const char *)input_callback_name,
            (const char *)data_input,
            &weechat_php_api_buffer_close_cb,
            (const char *)close_callback_name,
            (const char *)data_close));

    API_RETURN_STRING(result);
}

API_FUNC(buffer_new_props)
{
    zend_string *z_name, *z_data_input, *z_data_close;
    zval *z_properties, *z_input_callback, *z_close_callback;
    char *name, *data_input, *data_close;
    struct t_hashtable *properties;
    const char *result;

    API_INIT_FUNC(1, "buffer_new_props", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SazSzS", &z_name,
                               &z_properties, &z_input_callback, &z_data_input,
                               &z_close_callback, &z_data_close) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    name = ZSTR_VAL(z_name);
    properties = weechat_php_array_to_hashtable (
        z_properties,
        WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING);
    weechat_php_get_function_name (z_input_callback, input_callback_name);
    data_input = ZSTR_VAL(z_data_input);
    weechat_php_get_function_name (z_close_callback, close_callback_name);
    data_close = ZSTR_VAL(z_data_close);

    result = API_PTR2STR(
        plugin_script_api_buffer_new_props (
            weechat_php_plugin,
            php_current_script,
            (const char *)name,
            properties,
            &weechat_php_api_buffer_input_data_cb,
            (const char *)input_callback_name,
            (const char *)data_input,
            &weechat_php_api_buffer_close_cb,
            (const char *)close_callback_name,
            (const char *)data_close));

    if (properties)
        weechat_hashtable_free (properties);

    API_RETURN_STRING(result);
}

API_FUNC(buffer_search)
{
    zend_string *z_plugin, *z_name;
    char *plugin, *name;
    const char *result;

    API_INIT_FUNC(1, "buffer_search", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SS", &z_plugin,
                               &z_name) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    plugin = ZSTR_VAL(z_plugin);
    name = ZSTR_VAL(z_name);

    result = API_PTR2STR(
        weechat_buffer_search ((const char *)plugin, (const char *)name));

    API_RETURN_STRING(result);
}

API_FUNC(buffer_search_main)
{
    const char *result;

    API_INIT_FUNC(1, "buffer_search_main", API_RETURN_EMPTY);
    if (zend_parse_parameters_none () == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_buffer_search_main ());

    API_RETURN_STRING(result);
}

API_FUNC(current_buffer)
{
    const char *result;

    API_INIT_FUNC(1, "current_buffer", API_RETURN_EMPTY);
    if (zend_parse_parameters_none () == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_current_buffer ());

    API_RETURN_STRING(result);
}

API_FUNC(buffer_clear)
{
    zend_string *z_buffer;
    struct t_gui_buffer *buffer;

    API_INIT_FUNC(1, "buffer_clear", API_RETURN_ERROR);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_buffer) == FAILURE)
        API_WRONG_ARGS(API_RETURN_ERROR);

    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));

    weechat_buffer_clear (buffer);

    API_RETURN_OK;
}

API_FUNC(buffer_close)
{
    zend_string *z_buffer;
    struct t_gui_buffer *buffer;

    API_INIT_FUNC(1, "buffer_close", API_RETURN_ERROR);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_buffer) == FAILURE)
        API_WRONG_ARGS(API_RETURN_ERROR);

    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));

    weechat_buffer_close (buffer);

    API_RETURN_OK;
}

API_FUNC(buffer_merge)
{
    zend_string *z_buffer;
    zend_string *z_target_buffer;
    struct t_gui_buffer *buffer;
    struct t_gui_buffer *target_buffer;

    API_INIT_FUNC(1, "buffer_merge", API_RETURN_ERROR);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SS", &z_buffer,
                               &z_target_buffer) == FAILURE)
        API_WRONG_ARGS(API_RETURN_ERROR);

    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));
    target_buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_target_buffer));

    weechat_buffer_merge (buffer, target_buffer);

    API_RETURN_OK;
}

API_FUNC(buffer_unmerge)
{
    zend_string *z_buffer;
    zend_long z_number;
    struct t_gui_buffer *buffer;
    int number;

    API_INIT_FUNC(1, "buffer_unmerge", API_RETURN_ERROR);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "Sl", &z_buffer,
                               &z_number) == FAILURE)
        API_WRONG_ARGS(API_RETURN_ERROR);

    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));
    number = (int)z_number;

    weechat_buffer_unmerge (buffer, number);

    API_RETURN_OK;
}

API_FUNC(buffer_get_integer)
{
    zend_string *z_buffer, *z_property;
    struct t_gui_buffer *buffer;
    char *property;
    int result;

    API_INIT_FUNC(1, "buffer_get_integer", API_RETURN_INT(-1));
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SS", &z_buffer,
                               &z_property) == FAILURE)
        API_WRONG_ARGS(API_RETURN_INT(-1));

    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));
    property = ZSTR_VAL(z_property);

    result = weechat_buffer_get_integer (buffer, (const char *)property);

    API_RETURN_INT(result);
}

API_FUNC(buffer_get_string)
{
    zend_string *z_buffer, *z_property;
    const char *result;
    struct t_gui_buffer *buffer;
    char *property;

    API_INIT_FUNC(1, "buffer_get_string", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SS", &z_buffer,
                               &z_property) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));
    property = ZSTR_VAL(z_property);

    result = weechat_buffer_get_string (buffer, (const char *)property);

    API_RETURN_STRING(result);
}

API_FUNC(buffer_get_pointer)
{
    zend_string *z_buffer, *z_property;
    struct t_gui_buffer *buffer;
    char *property;
    const char *result;

    API_INIT_FUNC(1, "buffer_get_pointer", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SS", &z_buffer,
                               &z_property) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));
    property = ZSTR_VAL(z_property);

    result = API_PTR2STR(
        weechat_buffer_get_pointer (buffer, (const char *)property));

    API_RETURN_STRING(result);
}

API_FUNC(buffer_set)
{
    zend_string *z_buffer, *z_property, *z_value;
    struct t_gui_buffer *buffer;
    char *property, *value;

    API_INIT_FUNC(1, "buffer_set", API_RETURN_ERROR);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SSS", &z_buffer, &z_property,
                               &z_value) == FAILURE)
        API_WRONG_ARGS(API_RETURN_ERROR);

    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));
    property = ZSTR_VAL(z_property);
    value = ZSTR_VAL(z_value);

    weechat_buffer_set (buffer, (const char *)property, (const char *)value);

    API_RETURN_OK;
}

API_FUNC(buffer_string_replace_local_var)
{
    zend_string *z_buffer, *z_string;
    struct t_gui_buffer *buffer;
    char *string, *result;

    API_INIT_FUNC(1, "buffer_string_replace_local_var", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SS", &z_buffer,
                               &z_string) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));
    string = ZSTR_VAL(z_string);

    result = weechat_buffer_string_replace_local_var (buffer,
                                                      (const char *)string);

    API_RETURN_STRING_FREE(result);
}

API_FUNC(buffer_match_list)
{
    zend_string *z_buffer, *z_string;
    struct t_gui_buffer *buffer;
    char *string;
    int result;

    API_INIT_FUNC(1, "buffer_match_list", API_RETURN_INT(0));
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SS", &z_buffer,
                               &z_string) == FAILURE)
        API_WRONG_ARGS(API_RETURN_INT(0));

    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));
    string = ZSTR_VAL(z_string);

    result = weechat_buffer_match_list (buffer, (const char *)string);

    API_RETURN_INT(result);
}

API_FUNC(current_window)
{
    const char *result;

    API_INIT_FUNC(1, "current_window", API_RETURN_EMPTY);
    if (zend_parse_parameters_none () == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_current_window ());

    API_RETURN_STRING(result);
}

API_FUNC(window_search_with_buffer)
{
    zend_string *z_buffer;
    struct t_gui_buffer *buffer;
    const char *result;

    API_INIT_FUNC(1, "window_search_with_buffer", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_buffer) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));

    result = API_PTR2STR(weechat_window_search_with_buffer (buffer));

    API_RETURN_STRING(result);
}

API_FUNC(window_get_integer)
{
    zend_string *z_window, *z_property;
    struct t_gui_window *window;
    char *property;
    int result;

    API_INIT_FUNC(1, "window_get_integer", API_RETURN_INT(-1));
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SS", &z_window,
                               &z_property) == FAILURE)
        API_WRONG_ARGS(API_RETURN_INT(-1));

    window = (struct t_gui_window *)API_STR2PTR(ZSTR_VAL(z_window));
    property = ZSTR_VAL(z_property);

    result = weechat_window_get_integer (window, (const char *)property);

    API_RETURN_INT(result);
}

API_FUNC(window_get_string)
{
    zend_string *z_window, *z_property;
    struct t_gui_window *window;
    char *property;
    const char *result;

    API_INIT_FUNC(1, "window_get_string", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SS", &z_window,
                               &z_property) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    window = (struct t_gui_window *)API_STR2PTR(ZSTR_VAL(z_window));
    property = ZSTR_VAL(z_property);

    result = weechat_window_get_string (window, (const char *)property);

    API_RETURN_STRING(result);
}

API_FUNC(window_get_pointer)
{
    zend_string *z_window, *z_property;
    struct t_gui_window *window;
    char *property;
    const char *result;

    API_INIT_FUNC(1, "window_get_pointer", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SS", &z_window,
                               &z_property) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    window = (struct t_gui_window *)API_STR2PTR(ZSTR_VAL(z_window));
    property = ZSTR_VAL(z_property);

    result = API_PTR2STR(
        weechat_window_get_pointer (window, (const char *)property));

    API_RETURN_STRING(result);
}

API_FUNC(window_set_title)
{
    zend_string *z_title;
    char *title;

    API_INIT_FUNC(1, "window_set_title", API_RETURN_ERROR);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_title) == FAILURE)
        API_WRONG_ARGS(API_RETURN_ERROR);

    title = ZSTR_VAL(z_title);

    weechat_window_set_title ((const char *)title);

    API_RETURN_OK;
}

API_FUNC(nicklist_add_group)
{
    zend_string *z_buffer, *z_parent_group, *z_name, *z_color;
    zend_long z_visible;
    struct t_gui_buffer *buffer;
    struct t_gui_nick_group *parent_group;
    char *name, *color;
    int visible;
    const char *result;

    API_INIT_FUNC(1, "nicklist_add_group", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SSSSl", &z_buffer,
                               &z_parent_group, &z_name, &z_color,
                               &z_visible) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));
    parent_group = (struct t_gui_nick_group *)API_STR2PTR(ZSTR_VAL(z_parent_group));
    name = ZSTR_VAL(z_name);
    color = ZSTR_VAL(z_color);
    visible = (int)z_visible;

    result = API_PTR2STR(weechat_nicklist_add_group (buffer,
                                                     parent_group,
                                                     (const char *)name,
                                                     (const char *)color,
                                                     visible));

    API_RETURN_STRING(result);
}

API_FUNC(nicklist_search_group)
{
    zend_string *z_buffer, *z_from_group, *z_name;
    struct t_gui_buffer *buffer;
    struct t_gui_nick_group *from_group;
    char *name;
    const char *result;

    API_INIT_FUNC(1, "nicklist_search_group", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SSS", &z_buffer,
                               &z_from_group, &z_name) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));
    from_group = (struct t_gui_nick_group *)API_STR2PTR(ZSTR_VAL(z_from_group));
    name = ZSTR_VAL(z_name);

    result = API_PTR2STR(weechat_nicklist_search_group (buffer,
                                                        from_group,
                                                        (const char *)name));

    API_RETURN_STRING(result);
}

API_FUNC(nicklist_add_nick)
{
    zend_string *z_buffer, *z_group, *z_name, *z_color, *z_prefix;
    zend_string *z_prefix_color;
    zend_long z_visible;
    struct t_gui_buffer *buffer;
    struct t_gui_nick_group *group;
    char *name, *color, *prefix, *prefix_color;
    int visible;
    const char *result;

    API_INIT_FUNC(1, "nicklist_add_nick", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SSSSSSl", &z_buffer,
                               &z_group, &z_name, &z_color, &z_prefix,
                               &z_prefix_color, &z_visible) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));
    group = (struct t_gui_nick_group *)API_STR2PTR(ZSTR_VAL(z_group));
    name = ZSTR_VAL(z_name);
    color = ZSTR_VAL(z_color);
    prefix = ZSTR_VAL(z_prefix);
    prefix_color = ZSTR_VAL(z_prefix_color);
    visible = (int)z_visible;

    result = API_PTR2STR(
        weechat_nicklist_add_nick (
            buffer,
            group,
            (const char *)name,
            (const char *)color,
            (const char *)prefix,
            (const char *)prefix_color,
            visible));

    API_RETURN_STRING(result);
}

API_FUNC(nicklist_search_nick)
{
    zend_string *z_buffer, *z_from_group, *z_name;
    struct t_gui_buffer *buffer;
    struct t_gui_nick_group *from_group;
    char *name;
    const char *result;

    API_INIT_FUNC(1, "nicklist_search_nick", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SSS", &z_buffer,
                               &z_from_group, &z_name) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));
    from_group = (struct t_gui_nick_group *)API_STR2PTR(ZSTR_VAL(z_from_group));
    name = ZSTR_VAL(z_name);

    result = API_PTR2STR(weechat_nicklist_search_nick (buffer,
                                                       from_group,
                                                       (const char *)name));

    API_RETURN_STRING(result);
}

API_FUNC(nicklist_remove_group)
{
    zend_string *z_buffer, *z_group;
    struct t_gui_buffer *buffer;
    struct t_gui_nick_group *group;

    API_INIT_FUNC(1, "nicklist_remove_group", API_RETURN_ERROR);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SS", &z_buffer,
                               &z_group) == FAILURE)
        API_WRONG_ARGS(API_RETURN_ERROR);

    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));
    group = (struct t_gui_nick_group *)API_STR2PTR(ZSTR_VAL(z_group));

    weechat_nicklist_remove_group (buffer, group);

    API_RETURN_OK;
}

API_FUNC(nicklist_remove_nick)
{
    zend_string *z_buffer, *z_nick;
    struct t_gui_buffer *buffer;
    struct t_gui_nick *nick;

    API_INIT_FUNC(1, "nicklist_remove_nick", API_RETURN_ERROR);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SS", &z_buffer,
                               &z_nick) == FAILURE)
        API_WRONG_ARGS(API_RETURN_ERROR);

    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));
    nick = (struct t_gui_nick *)API_STR2PTR(ZSTR_VAL(z_nick));

    weechat_nicklist_remove_nick (buffer, nick);

    API_RETURN_OK;
}

API_FUNC(nicklist_remove_all)
{
    zend_string *z_buffer;
    struct t_gui_buffer *buffer;

    API_INIT_FUNC(1, "nicklist_remove_all", API_RETURN_ERROR);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_buffer) == FAILURE)
        API_WRONG_ARGS(API_RETURN_ERROR);

    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));

    weechat_nicklist_remove_all (buffer);

    API_RETURN_OK;
}

API_FUNC(nicklist_group_get_integer)
{
    zend_string *z_buffer, *z_group, *z_property;
    struct t_gui_buffer *buffer;
    struct t_gui_nick_group *group;
    char *property;
    int result;

    API_INIT_FUNC(1, "nicklist_group_get_integer", API_RETURN_INT(-1));
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SSS", &z_buffer, &z_group,
                               &z_property) == FAILURE)
        API_WRONG_ARGS(API_RETURN_INT(-1));

    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));
    group = (struct t_gui_nick_group *)API_STR2PTR(ZSTR_VAL(z_group));
    property = ZSTR_VAL(z_property);

    result = weechat_nicklist_group_get_integer (buffer,
                                                 group,
                                                 (const char *)property);

    API_RETURN_INT(result);
}

API_FUNC(nicklist_group_get_string)
{
    zend_string *z_buffer, *z_group, *z_property;
    struct t_gui_buffer *buffer;
    struct t_gui_nick_group *group;
    char *property;
    const char *result;

    API_INIT_FUNC(1, "nicklist_group_get_string", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SSS", &z_buffer, &z_group,
                               &z_property) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));
    group = (struct t_gui_nick_group *)API_STR2PTR(ZSTR_VAL(z_group));
    property = ZSTR_VAL(z_property);

    result = weechat_nicklist_group_get_string (buffer,
                                                group,
                                                (const char *)property);

    API_RETURN_STRING(result);
}

API_FUNC(nicklist_group_get_pointer)
{
    zend_string *z_buffer, *z_group, *z_property;
    struct t_gui_buffer *buffer;
    struct t_gui_nick_group *group;
    char *property;
    const char *result;

    API_INIT_FUNC(1, "nicklist_group_get_pointer", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SSS", &z_buffer, &z_group,
                               &z_property) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));
    group = (struct t_gui_nick_group *)API_STR2PTR(ZSTR_VAL(z_group));
    property = ZSTR_VAL(z_property);

    result = API_PTR2STR(
        weechat_nicklist_group_get_pointer (buffer,
                                            group,
                                            (const char *)property));

    API_RETURN_STRING(result);
}

API_FUNC(nicklist_group_set)
{
    zend_string *z_buffer, *z_group, *z_property, *z_value;
    struct t_gui_buffer *buffer;
    struct t_gui_nick_group *group;
    char *property, *value;

    API_INIT_FUNC(1, "nicklist_group_set", API_RETURN_ERROR);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SSSS", &z_buffer, &z_group,
                               &z_property, &z_value) == FAILURE)
        API_WRONG_ARGS(API_RETURN_ERROR);

    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));
    group = (struct t_gui_nick_group *)API_STR2PTR(ZSTR_VAL(z_group));
    property = ZSTR_VAL(z_property);
    value = ZSTR_VAL(z_value);

    weechat_nicklist_group_set (buffer,
                                group,
                                (const char *)property,
                                (const char *)value);

    API_RETURN_OK;
}

API_FUNC(nicklist_nick_get_integer)
{
    zend_string *z_buffer, *z_nick, *z_property;
    struct t_gui_buffer *buffer;
    struct t_gui_nick *nick;
    char *property;
    int result;

    API_INIT_FUNC(1, "nicklist_nick_get_integer", API_RETURN_INT(-1));
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SSS", &z_buffer, &z_nick,
                               &z_property) == FAILURE)
        API_WRONG_ARGS(API_RETURN_INT(-1));

    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));
    nick = (struct t_gui_nick *)API_STR2PTR(ZSTR_VAL(z_nick));
    property = ZSTR_VAL(z_property);

    result = weechat_nicklist_nick_get_integer (buffer,
                                                nick,
                                                (const char *)property);

    API_RETURN_INT(result);
}

API_FUNC(nicklist_nick_get_string)
{
    zend_string *z_buffer, *z_nick, *z_property;
    struct t_gui_buffer *buffer;
    struct t_gui_nick *nick;
    char *property;
    const char *result;

    API_INIT_FUNC(1, "nicklist_nick_get_string", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SSS", &z_buffer, &z_nick,
                               &z_property) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));
    nick = (struct t_gui_nick *)API_STR2PTR(ZSTR_VAL(z_nick));
    property = ZSTR_VAL(z_property);

    result = weechat_nicklist_nick_get_string (buffer,
                                               nick,
                                               (const char *)property);

    API_RETURN_STRING(result);
}

API_FUNC(nicklist_nick_get_pointer)
{
    zend_string *z_buffer, *z_nick, *z_property;
    struct t_gui_buffer *buffer;
    struct t_gui_nick *nick;
    char *property;
    const char *result;

    API_INIT_FUNC(1, "nicklist_nick_get_pointer", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SSS", &z_buffer, &z_nick,
                               &z_property) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));
    nick = (struct t_gui_nick *)API_STR2PTR(ZSTR_VAL(z_nick));
    property = ZSTR_VAL(z_property);

    result = API_PTR2STR(
        weechat_nicklist_nick_get_pointer (buffer,
                                           nick,
                                           (const char *)property));

    API_RETURN_STRING(result);
}

API_FUNC(nicklist_nick_set)
{
    zend_string *z_buffer, *z_nick, *z_property, *z_value;
    struct t_gui_buffer *buffer;
    struct t_gui_nick *nick;
    char *property, *value;

    API_INIT_FUNC(1, "nicklist_nick_set", API_RETURN_ERROR);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SSSS", &z_buffer, &z_nick,
                               &z_property, &z_value) == FAILURE)
        API_WRONG_ARGS(API_RETURN_ERROR);

    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));
    nick = (struct t_gui_nick *)API_STR2PTR(ZSTR_VAL(z_nick));
    property = ZSTR_VAL(z_property);
    value = ZSTR_VAL(z_value);

    weechat_nicklist_nick_set (buffer,
                               nick,
                               (const char *)property,
                               (const char *)value);

    API_RETURN_OK;
}

API_FUNC(bar_item_search)
{
    zend_string *z_name;
    char *name;
    const char *result;

    API_INIT_FUNC(1, "bar_item_search", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_name) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    name = ZSTR_VAL(z_name);

    result = API_PTR2STR(weechat_bar_item_search ((const char *)name));

    API_RETURN_STRING(result);
}

static char *
weechat_php_api_bar_item_new_build_cb (const void *pointer, void *data,
                                       struct t_gui_bar_item *item,
                                       struct t_gui_window *window,
                                       struct t_gui_buffer *buffer,
                                       struct t_hashtable *extra_info)
{
    char *rc;
    void *func_argv[5];

    func_argv[1] = (char *)API_PTR2STR(item);
    func_argv[2] = (char *)API_PTR2STR(window);
    func_argv[3] = (char *)API_PTR2STR(buffer);
    func_argv[4] = extra_info;

    weechat_php_cb (pointer, data, func_argv, "ssssh",
                    WEECHAT_SCRIPT_EXEC_STRING, &rc);

    return rc;
}

API_FUNC(bar_item_new)
{
    zend_string *z_name, *z_data;
    zval *z_build_callback;
    char *name, *data;
    const char *result;

    API_INIT_FUNC(1, "bar_item_new", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SzS", &z_name,
                               &z_build_callback, &z_data) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    name = ZSTR_VAL(z_name);
    weechat_php_get_function_name (z_build_callback, build_callback_name);
    data = ZSTR_VAL(z_data);

    result = API_PTR2STR(
        plugin_script_api_bar_item_new (
            weechat_php_plugin,
            php_current_script,
            (const char *)name,
            &weechat_php_api_bar_item_new_build_cb,
            (const char *)build_callback_name,
            (const char *)data));

    API_RETURN_STRING(result);
}

API_FUNC(bar_item_update)
{
    zend_string *z_name;
    char *name;

    API_INIT_FUNC(1, "bar_item_update", API_RETURN_ERROR);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_name) == FAILURE)
        API_WRONG_ARGS(API_RETURN_ERROR);

    name = ZSTR_VAL(z_name);

    weechat_bar_item_update ((const char *)name);

    API_RETURN_OK;
}

API_FUNC(bar_item_remove)
{
    zend_string *z_item;
    struct t_gui_bar_item *item;

    API_INIT_FUNC(1, "bar_item_remove", API_RETURN_ERROR);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_item) == FAILURE)
        API_WRONG_ARGS(API_RETURN_ERROR);

    item = (struct t_gui_bar_item *)API_STR2PTR(ZSTR_VAL(z_item));

    weechat_bar_item_remove (item);

    API_RETURN_OK;
}

API_FUNC(bar_search)
{
    zend_string *z_name;
    char *name;
    const char *result;

    API_INIT_FUNC(1, "bar_search", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_name) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    name = ZSTR_VAL(z_name);

    result = API_PTR2STR(weechat_bar_search ((const char *)name));

    API_RETURN_STRING(result);
}

API_FUNC(bar_new)
{
    zend_string *z_name, *z_hidden, *z_priority, *z_type, *z_condition;
    zend_string *z_position, *z_filling_top_bottom, *z_filling_left_right;
    zend_string *z_size, *z_size_max, *z_color_fg, *z_color_delim, *z_color_bg;
    zend_string *z_color_bg_inactive, *z_separator, *z_items;
    char *name, *hidden, *priority, *type, *condition, *position;
    char *filling_top_bottom, *filling_left_right, *size, *size_max, *color_fg;
    char *color_delim, *color_bg, *color_bg_inactive, *separator, *items;
    const char *result;

    API_INIT_FUNC(1, "bar_new", API_RETURN_EMPTY);
    if (zend_parse_parameters (
            ZEND_NUM_ARGS(), "SSSSSSSSSSSSSSSS", &z_name, &z_hidden,
            &z_priority, &z_type, &z_condition, &z_position,
            &z_filling_top_bottom, &z_filling_left_right, &z_size, &z_size_max,
            &z_color_fg, &z_color_delim, &z_color_bg, &z_color_bg_inactive,
            &z_separator, &z_items) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

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
    color_bg_inactive = ZSTR_VAL(z_color_bg_inactive);
    separator = ZSTR_VAL(z_separator);
    items = ZSTR_VAL(z_items);

    result = API_PTR2STR(
        weechat_bar_new (
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
            (const char *)color_bg_inactive,
            (const char *)separator,
            (const char *)items));

    API_RETURN_STRING(result);
}

API_FUNC(bar_set)
{
    zend_string *z_bar, *z_property, *z_value;
    struct t_gui_bar *bar;
    char *property, *value;
    int result;

    API_INIT_FUNC(1, "bar_set", API_RETURN_INT(0));
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SSS", &z_bar, &z_property,
                               &z_value) == FAILURE)
        API_WRONG_ARGS(API_RETURN_INT(0));

    bar = (struct t_gui_bar *)API_STR2PTR(ZSTR_VAL(z_bar));
    property = ZSTR_VAL(z_property);
    value = ZSTR_VAL(z_value);

    result = weechat_bar_set (bar, (const char *)property, (const char *)value);

    API_RETURN_INT(result);
}

API_FUNC(bar_update)
{
    zend_string *z_name;
    char *name;

    API_INIT_FUNC(1, "bar_update", API_RETURN_ERROR);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_name) == FAILURE)
        API_WRONG_ARGS(API_RETURN_ERROR);

    name = ZSTR_VAL(z_name);
    weechat_bar_update ((const char *)name);

    API_RETURN_OK;
}

API_FUNC(bar_remove)
{
    zend_string *z_bar;
    struct t_gui_bar *bar;

    API_INIT_FUNC(1, "bar_remove", API_RETURN_ERROR);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_bar) == FAILURE)
        API_WRONG_ARGS(API_RETURN_ERROR);

    bar = (struct t_gui_bar *)API_STR2PTR(ZSTR_VAL(z_bar));

    weechat_bar_remove (bar);

    API_RETURN_OK;
}

API_FUNC(command)
{
    zend_string *z_buffer, *z_command;
    struct t_gui_buffer *buffer;
    char *command;
    int result;

    API_INIT_FUNC(1, "command", API_RETURN_INT(WEECHAT_RC_ERROR));
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SS", &z_buffer,
                               &z_command) == FAILURE)
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_RC_ERROR));

    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));
    command = ZSTR_VAL(z_command);

    result = plugin_script_api_command (weechat_php_plugin,
                                        php_current_script,
                                        buffer,
                                        (const char *)command);

    API_RETURN_INT(result);
}

API_FUNC(command_options)
{
    zend_string *z_buffer, *z_command;
    zval *z_options;
    struct t_gui_buffer *buffer;
    char *command;
    struct t_hashtable *options;
    int result;

    API_INIT_FUNC(1, "command", API_RETURN_INT(WEECHAT_RC_ERROR));
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SSa", &z_buffer,
                               &z_command, &z_options) == FAILURE)
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_RC_ERROR));

    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));
    command = ZSTR_VAL(z_command);
    options = weechat_php_array_to_hashtable (
        z_options,
        WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING);

    result = plugin_script_api_command_options (weechat_php_plugin,
                                                php_current_script,
                                                buffer,
                                                (const char *)command,
                                                options);

    if (options)
        weechat_hashtable_free (options);

    API_RETURN_INT(result);
}

API_FUNC(completion_new)
{
    zend_string *z_buffer;
    struct t_gui_buffer *buffer;
    const char *result;

    API_INIT_FUNC(1, "completion_new", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_buffer) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = (struct t_gui_buffer *)API_STR2PTR(ZSTR_VAL(z_buffer));

    result = API_PTR2STR(weechat_completion_new (buffer));

    API_RETURN_STRING(result);
}

API_FUNC(completion_search)
{
    zend_string *z_completion, *z_data;
    zend_long z_position, z_direction;
    char *data;
    struct t_gui_completion *completion;
    int position, direction, rc;

    API_INIT_FUNC(1, "completion_search", API_RETURN_INT(0));
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SSll", &z_completion, &z_data,
                               &z_position, &z_direction) == FAILURE)
        API_WRONG_ARGS(API_RETURN_INT(0));

    completion = (struct t_gui_completion *)API_STR2PTR(ZSTR_VAL(z_completion));
    data = ZSTR_VAL(z_data);
    position = (int)z_position;
    direction = (int)z_direction;

    rc = weechat_completion_search (completion, data, position, direction);

    API_RETURN_INT(rc);
}

API_FUNC(completion_get_string)
{
    zend_string *z_completion, *z_property;
    struct t_gui_completion *completion;
    char *property;
    const char *result;

    API_INIT_FUNC(1, "completion_get_string", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SS", &z_completion,
                               &z_property) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    completion = (struct t_gui_completion *)API_STR2PTR(ZSTR_VAL(z_completion));
    property = ZSTR_VAL(z_property);

    result = weechat_completion_get_string (completion,
                                            (const char *)property);

    API_RETURN_STRING(result);
}

API_FUNC(completion_list_add)
{
    zend_string *z_completion, *z_word, *z_where;
    zend_long z_nick_completion;
    struct t_gui_completion *completion;
    char *word, *where;
    int nick_completion;

    API_INIT_FUNC(1, "completion_list_add", API_RETURN_ERROR);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SSlS", &z_completion,
                               &z_word, &z_nick_completion, &z_where) == FAILURE)
        API_WRONG_ARGS(API_RETURN_ERROR);

    completion = (struct t_gui_completion *)API_STR2PTR(ZSTR_VAL(z_completion));
    word = ZSTR_VAL(z_word);
    nick_completion = (int)z_nick_completion;
    where = ZSTR_VAL(z_where);

    weechat_completion_list_add (completion,
                                 (const char *)word,
                                 nick_completion,
                                 (const char *)where);

    API_RETURN_OK;
}

API_FUNC(completion_free)
{
    zend_string *z_completion;
    struct t_gui_completion *completion;

    API_INIT_FUNC(1, "completion_free", API_RETURN_ERROR);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_completion) == FAILURE)
        API_WRONG_ARGS(API_RETURN_ERROR);

    completion = (struct t_gui_completion *)API_STR2PTR(ZSTR_VAL(z_completion));

    weechat_completion_free (completion);

    API_RETURN_OK;
}

API_FUNC(info_get)
{
    zend_string *z_info_name, *z_arguments;
    char *info_name, *arguments, *result;

    API_INIT_FUNC(1, "info_get", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SS", &z_info_name,
                               &z_arguments) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    info_name = ZSTR_VAL(z_info_name);
    arguments = ZSTR_VAL(z_arguments);

    result = weechat_info_get ((const char *)info_name,
                               (const char *)arguments);

    API_RETURN_STRING_FREE(result);
}

API_FUNC(info_get_hashtable)
{
    zend_string *z_info_name;
    zval *z_hashtable;
    char *info_name;
    struct t_hashtable *hashtable, *result;

    API_INIT_FUNC(1, "info_get_hashtable", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "Sa", &z_info_name,
                               &z_hashtable) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    info_name = ZSTR_VAL(z_info_name);
    hashtable = weechat_php_array_to_hashtable (
        z_hashtable,
        WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING);

    result = weechat_info_get_hashtable ((const char *)info_name, hashtable);

    if (hashtable)
        weechat_hashtable_free (hashtable);

    weechat_php_hashtable_to_array (result, return_value);
}

API_FUNC(infolist_new)
{
    const char *result;

    API_INIT_FUNC(1, "infolist_new", API_RETURN_EMPTY);
    if (zend_parse_parameters_none () == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_infolist_new ());

    API_RETURN_STRING(result);
}

API_FUNC(infolist_new_item)
{
    zend_string *z_infolist;
    struct t_infolist *infolist;
    const char *result;

    API_INIT_FUNC(1, "infolist_new_item", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_infolist) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    infolist = (struct t_infolist *)API_STR2PTR(ZSTR_VAL(z_infolist));

    result = API_PTR2STR(weechat_infolist_new_item (infolist));

    API_RETURN_STRING(result);
}

API_FUNC(infolist_new_var_integer)
{
    zend_string *z_item, *z_name;
    zend_long z_value;
    struct t_infolist_item *item;
    char *name;
    int value;
    const char *result;

    API_INIT_FUNC(1, "infolist_new_var_integer", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SSl", &z_item, &z_name,
                               &z_value) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    item = (struct t_infolist_item *)API_STR2PTR(ZSTR_VAL(z_item));
    name = ZSTR_VAL(z_name);
    value = (int)z_value;

    result = API_PTR2STR(weechat_infolist_new_var_integer (item,
                                                           (const char *)name,
                                                           value));

    API_RETURN_STRING(result);
}

API_FUNC(infolist_new_var_string)
{
    zend_string *z_item, *z_name, *z_value;
    struct t_infolist_item *item;
    char *name, *value;
    const char *result;

    API_INIT_FUNC(1, "infolist_new_var_string", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SSS", &z_item, &z_name,
                               &z_value) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    item = (struct t_infolist_item *)API_STR2PTR(ZSTR_VAL(z_item));
    name = ZSTR_VAL(z_name);
    value = ZSTR_VAL(z_value);

    result = API_PTR2STR(
        weechat_infolist_new_var_string (item,
                                         (const char *)name,
                                         (const char *)value));

    API_RETURN_STRING(result);
}

API_FUNC(infolist_new_var_pointer)
{
    zend_string *z_item, *z_name, *z_pointer;
    struct t_infolist_item *item;
    char *name;
    void *pointer;
    const char *result;

    API_INIT_FUNC(1, "infolist_new_var_pointer", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SSS", &z_item, &z_name,
                               &z_pointer) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    item = (struct t_infolist_item *)API_STR2PTR(ZSTR_VAL(z_item));
    name = ZSTR_VAL(z_name);
    pointer = (void *)API_STR2PTR(ZSTR_VAL(z_pointer));

    result = API_PTR2STR(
        weechat_infolist_new_var_pointer (item,
                                          (const char *)name,
                                          pointer));

    API_RETURN_STRING(result);
}

API_FUNC(infolist_new_var_time)
{
    zend_string *z_item, *z_name;
    zend_long z_time;
    struct t_infolist_item *item;
    char *name;
    time_t time;
    const char *result;

    API_INIT_FUNC(1, "infolist_new_var_time", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SSl", &z_item, &z_name,
                               &z_time) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    item = (struct t_infolist_item *)API_STR2PTR(ZSTR_VAL(z_item));
    name = ZSTR_VAL(z_name);
    time = (time_t)z_time;

    result = API_PTR2STR(
        weechat_infolist_new_var_time (item, (const char *)name, time));

    API_RETURN_STRING(result);
}

API_FUNC(infolist_search_var)
{
    zend_string *z_infolist, *z_name;
    struct t_infolist *infolist;
    char *name;
    const char *result;

    API_INIT_FUNC(1, "infolist_search_var", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SS", &z_infolist,
                               &z_name) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    infolist = (struct t_infolist *)API_STR2PTR(ZSTR_VAL(z_infolist));
    name = ZSTR_VAL(z_name);

    result = API_PTR2STR(
        weechat_infolist_search_var (infolist, (const char *)name));

    API_RETURN_STRING(result);
}

API_FUNC(infolist_get)
{
    zend_string *z_infolist_name, *z_pointer, *z_arguments;
    char *infolist_name, *arguments;
    void *pointer;
    const char *result;

    API_INIT_FUNC(1, "infolist_get", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SSS", &z_infolist_name,
                               &z_pointer, &z_arguments) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    infolist_name = ZSTR_VAL(z_infolist_name);
    pointer = (void *)API_STR2PTR(ZSTR_VAL(z_pointer));
    arguments = ZSTR_VAL(z_arguments);

    result = API_PTR2STR(
        weechat_infolist_get ((const char *)infolist_name,
                              pointer,
                              (const char *)arguments));

    API_RETURN_STRING(result);
}

API_FUNC(infolist_next)
{
    zend_string *z_infolist;
    struct t_infolist *infolist;
    int result;

    API_INIT_FUNC(1, "infolist_next", API_RETURN_INT(0));
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_infolist) == FAILURE)
        API_WRONG_ARGS(API_RETURN_INT(0));

    infolist = (struct t_infolist *)API_STR2PTR(ZSTR_VAL(z_infolist));

    result = weechat_infolist_next (infolist);

    API_RETURN_INT(result);
}

API_FUNC(infolist_prev)
{
    zend_string *z_infolist;
    struct t_infolist *infolist;
    int result;

    API_INIT_FUNC(1, "infolist_prev", API_RETURN_INT(0));
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_infolist) == FAILURE)
        API_WRONG_ARGS(API_RETURN_INT(0));

    infolist = (struct t_infolist *)API_STR2PTR(ZSTR_VAL(z_infolist));

    result = weechat_infolist_prev (infolist);

    API_RETURN_INT(result);
}

API_FUNC(infolist_reset_item_cursor)
{
    zend_string *z_infolist;
    struct t_infolist *infolist;

    API_INIT_FUNC(1, "infolist_reset_item_cursor", API_RETURN_ERROR);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_infolist) == FAILURE)
        API_WRONG_ARGS(API_RETURN_ERROR);

    infolist = (struct t_infolist *)API_STR2PTR(ZSTR_VAL(z_infolist));

    weechat_infolist_reset_item_cursor (infolist);

    API_RETURN_OK;
}

API_FUNC(infolist_fields)
{
    zend_string *z_infolist;
    struct t_infolist *infolist;
    const char *result;

    API_INIT_FUNC(1, "infolist_fields", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_infolist) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    infolist = (struct t_infolist *)API_STR2PTR(ZSTR_VAL(z_infolist));

    result = weechat_infolist_fields (infolist);

    API_RETURN_STRING(result);
}

API_FUNC(infolist_integer)
{
    zend_string *z_infolist, *z_var;
    struct t_infolist *infolist;
    char *var;
    int result;

    API_INIT_FUNC(1, "infolist_integer", API_RETURN_INT(0));
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SS", &z_infolist,
                               &z_var) == FAILURE)
        API_WRONG_ARGS(API_RETURN_INT(0));

    infolist = (struct t_infolist *)API_STR2PTR(ZSTR_VAL(z_infolist));
    var = ZSTR_VAL(z_var);

    result = weechat_infolist_integer (infolist, (const char *)var);

    API_RETURN_INT(result);
}

API_FUNC(infolist_string)
{
    zend_string *z_infolist, *z_var;
    struct t_infolist *infolist;
    char *var;
    const char *result;

    API_INIT_FUNC(1, "infolist_string", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SS", &z_infolist,
                               &z_var) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    infolist = (struct t_infolist *)API_STR2PTR(ZSTR_VAL(z_infolist));
    var = ZSTR_VAL(z_var);

    result = weechat_infolist_string (infolist, (const char *)var);

    API_RETURN_STRING(result);
}

API_FUNC(infolist_pointer)
{
    zend_string *z_infolist, *z_var;
    struct t_infolist *infolist;
    char *var;
    const char *result;

    API_INIT_FUNC(1, "infolist_pointer", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SS", &z_infolist,
                               &z_var) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    infolist = (struct t_infolist *)API_STR2PTR(ZSTR_VAL(z_infolist));
    var = ZSTR_VAL(z_var);

    result = API_PTR2STR(
        weechat_infolist_pointer (infolist, (const char *)var));

    API_RETURN_STRING(result);
}

API_FUNC(infolist_time)
{
    zend_string *z_infolist, *z_var;
    struct t_infolist *infolist;
    char *var;
    time_t time;

    API_INIT_FUNC(1, "infolist_time", API_RETURN_LONG(0));
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SS", &z_infolist,
                               &z_var) == FAILURE)
        API_WRONG_ARGS(API_RETURN_LONG(0));

    infolist = (struct t_infolist *)API_STR2PTR(ZSTR_VAL(z_infolist));
    var = ZSTR_VAL(z_var);

    time = weechat_infolist_time (infolist, (const char *)var);

    API_RETURN_LONG(time);
}

API_FUNC(infolist_free)
{
    zend_string *z_infolist;
    struct t_infolist *infolist;

    API_INIT_FUNC(1, "infolist_free", API_RETURN_ERROR);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_infolist) == FAILURE)
        API_WRONG_ARGS(API_RETURN_ERROR);

    infolist = (struct t_infolist *)API_STR2PTR(ZSTR_VAL(z_infolist));

    weechat_infolist_free (infolist);

    API_RETURN_OK;
}

API_FUNC(hdata_get)
{
    zend_string *z_hdata_name;
    char *hdata_name;
    const char *result;

    API_INIT_FUNC(1, "hdata_get", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S", &z_hdata_name) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    hdata_name = ZSTR_VAL(z_hdata_name);

    result = API_PTR2STR(weechat_hdata_get ((const char *)hdata_name));

    API_RETURN_STRING(result);
}

API_FUNC(hdata_get_var_offset)
{
    zend_string *z_hdata, *z_name;
    struct t_hdata *hdata;
    char *name;
    int result;

    API_INIT_FUNC(1, "hdata_get_var_offset", API_RETURN_INT(0));
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SS", &z_hdata,
                               &z_name) == FAILURE)
        API_WRONG_ARGS(API_RETURN_INT(0));

    hdata = (struct t_hdata *)API_STR2PTR(ZSTR_VAL(z_hdata));
    name = ZSTR_VAL(z_name);

    result = weechat_hdata_get_var_offset (hdata, (const char *)name);

    API_RETURN_INT(result);
}

API_FUNC(hdata_get_var_type_string)
{
    zend_string *z_hdata, *z_name;
    struct t_hdata *hdata;
    char *name;
    const char *result;

    API_INIT_FUNC(1, "hdata_get_var_type_string", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SS", &z_hdata,
                               &z_name) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    hdata = (struct t_hdata *)API_STR2PTR(ZSTR_VAL(z_hdata));
    name = ZSTR_VAL(z_name);

    result = weechat_hdata_get_var_type_string (hdata, (const char *)name);

    API_RETURN_STRING(result);
}

API_FUNC(hdata_get_var_array_size)
{
    zend_string *z_hdata, *z_pointer, *z_name;
    struct t_hdata *hdata;
    void *pointer;
    char *name;
    int result;

    API_INIT_FUNC(1, "hdata_get_var_array_size", API_RETURN_INT(-1));
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SSS", &z_hdata, &z_pointer,
                               &z_name) == FAILURE)
        API_WRONG_ARGS(API_RETURN_INT(-1));

    hdata = (struct t_hdata *)API_STR2PTR(ZSTR_VAL(z_hdata));
    pointer = (void *)API_STR2PTR(ZSTR_VAL(z_pointer));
    name = ZSTR_VAL(z_name);

    result = weechat_hdata_get_var_array_size (hdata,
                                               pointer,
                                               (const char *)name);

    API_RETURN_INT(result);
}

API_FUNC(hdata_get_var_array_size_string)
{
    zend_string *z_hdata, *z_pointer, *z_name;
    struct t_hdata *hdata;
    void *pointer;
    char *name;
    const char *result;

    API_INIT_FUNC(1, "hdata_get_var_array_size_string", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SSS", &z_hdata, &z_pointer,
                               &z_name) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    hdata = (struct t_hdata *)API_STR2PTR(ZSTR_VAL(z_hdata));
    pointer = (void *)API_STR2PTR(ZSTR_VAL(z_pointer));
    name = ZSTR_VAL(z_name);

    result = weechat_hdata_get_var_array_size_string (hdata,
                                                      pointer,
                                                      (const char *)name);

    API_RETURN_STRING(result);
}

API_FUNC(hdata_get_var_hdata)
{
    zend_string *z_hdata, *z_name;
    struct t_hdata *hdata;
    char *name;
    const char *result;

    API_INIT_FUNC(1, "hdata_get_var_hdata", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SS", &z_hdata,
                               &z_name) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    hdata = (struct t_hdata *)API_STR2PTR(ZSTR_VAL(z_hdata));
    name = ZSTR_VAL(z_name);

    result = weechat_hdata_get_var_hdata (hdata, (const char *)name);

    API_RETURN_STRING(result);
}

API_FUNC(hdata_get_list)
{
    zend_string *z_hdata, *z_name;
    struct t_hdata *hdata;
    char *name;
    const char *result;

    API_INIT_FUNC(1, "hdata_get_list", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SS", &z_hdata,
                               &z_name) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    hdata = (struct t_hdata *)API_STR2PTR(ZSTR_VAL(z_hdata));
    name = ZSTR_VAL(z_name);

    result = API_PTR2STR(
        weechat_hdata_get_list (hdata, (const char *)name));

    API_RETURN_STRING(result);
}

API_FUNC(hdata_check_pointer)
{
    zend_string *z_hdata, *z_list, *z_pointer;
    struct t_hdata *hdata;
    void *list, *pointer;
    int result;

    API_INIT_FUNC(1, "hdata_check_pointer", API_RETURN_INT(0));
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SSS", &z_hdata, &z_list,
                               &z_pointer) == FAILURE)
        API_WRONG_ARGS(API_RETURN_INT(0));

    hdata = (struct t_hdata *)API_STR2PTR(ZSTR_VAL(z_hdata));
    list = (void *)API_STR2PTR(ZSTR_VAL(z_list));
    pointer = (void *)API_STR2PTR(ZSTR_VAL(z_pointer));

    result = weechat_hdata_check_pointer (hdata, list, pointer);

    API_RETURN_INT(result);
}

API_FUNC(hdata_move)
{
    zend_string *z_hdata, *z_pointer;
    zend_long z_count;
    struct t_hdata *hdata;
    void *pointer;
    int count;
    const char *result;

    API_INIT_FUNC(1, "hdata_move", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SSl", &z_hdata, &z_pointer,
                               &z_count) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    hdata = (struct t_hdata *)API_STR2PTR(ZSTR_VAL(z_hdata));
    pointer = (void *)API_STR2PTR(ZSTR_VAL(z_pointer));
    count = (int)z_count;

    result = API_PTR2STR(weechat_hdata_move (hdata, pointer, count));

    API_RETURN_STRING(result);
}

API_FUNC(hdata_search)
{
    zend_string *z_hdata, *z_pointer, *z_search;
    zval *z_pointers, *z_extra_vars, *z_options;
    zend_long z_move;
    struct t_hdata *hdata;
    void *pointer;
    char *search;
    int move;
    const char *result;
    struct t_hashtable *pointers, *extra_vars, *options;

    API_INIT_FUNC(1, "hdata_search", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SSSaaal", &z_hdata,
                               &z_pointer, &z_search, &z_pointers,
                               &z_extra_vars, &z_options, &z_move) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    hdata = (struct t_hdata *)API_STR2PTR(ZSTR_VAL(z_hdata));
    pointer = (void *)API_STR2PTR(ZSTR_VAL(z_pointer));
    search = ZSTR_VAL(z_search);
    pointers = weechat_php_array_to_hashtable (
        z_pointers,
        WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_POINTER);
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
    move = (int)z_move;

    result = API_PTR2STR(
        weechat_hdata_search (
            hdata,
            pointer,
            (const char *)search,
            pointers,
            extra_vars,
            options,
            move));

    if (pointers)
        weechat_hashtable_free (pointers);
    if (extra_vars)
        weechat_hashtable_free (extra_vars);
    if (options)
        weechat_hashtable_free (options);

    API_RETURN_STRING(result);
}

API_FUNC(hdata_char)
{
    zend_string *z_hdata, *z_pointer, *z_name;
    struct t_hdata *hdata;
    void *pointer;
    char *name, result;

    API_INIT_FUNC(1, "hdata_char", API_RETURN_INT(0));
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SSS", &z_hdata, &z_pointer,
                               &z_name) == FAILURE)
        API_WRONG_ARGS(API_RETURN_INT(0));

    hdata = (struct t_hdata *)API_STR2PTR(ZSTR_VAL(z_hdata));
    pointer = (void *)API_STR2PTR(ZSTR_VAL(z_pointer));
    name = ZSTR_VAL(z_name);

    result = weechat_hdata_char (hdata, pointer, (const char *)name);

    API_RETURN_INT((int)result);
}

API_FUNC(hdata_integer)
{
    zend_string *z_hdata, *z_pointer, *z_name;
    struct t_hdata *hdata;
    void *pointer;
    char *name;
    int result;

    API_INIT_FUNC(1, "hdata_integer", API_RETURN_INT(0));
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SSS", &z_hdata, &z_pointer,
                               &z_name) == FAILURE)
        API_WRONG_ARGS(API_RETURN_INT(0));

    hdata = (struct t_hdata *)API_STR2PTR(ZSTR_VAL(z_hdata));
    pointer = (void *)API_STR2PTR(ZSTR_VAL(z_pointer));
    name = ZSTR_VAL(z_name);

    result = weechat_hdata_integer (hdata, pointer, (const char *)name);

    API_RETURN_INT(result);
}

API_FUNC(hdata_long)
{
    zend_string *z_hdata, *z_pointer, *z_name;
    struct t_hdata *hdata;
    void *pointer;
    char *name;
    long result;

    API_INIT_FUNC(1, "hdata_long", API_RETURN_LONG(0));
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SSS", &z_hdata, &z_pointer,
                               &z_name) == FAILURE)
        API_WRONG_ARGS(API_RETURN_LONG(0));

    hdata = (struct t_hdata *)API_STR2PTR(ZSTR_VAL(z_hdata));
    pointer = (void *)API_STR2PTR(ZSTR_VAL(z_pointer));
    name = ZSTR_VAL(z_name);

    result = weechat_hdata_long (hdata, pointer, (const char *)name);

    API_RETURN_LONG(result);
}

API_FUNC(hdata_string)
{
    zend_string *z_hdata, *z_pointer, *z_name;
    struct t_hdata *hdata;
    void *pointer;
    char *name;
    const char *result;

    API_INIT_FUNC(1, "hdata_string", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SSS", &z_hdata, &z_pointer,
                               &z_name) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    hdata = (struct t_hdata *)API_STR2PTR(ZSTR_VAL(z_hdata));
    pointer = (void *)API_STR2PTR(ZSTR_VAL(z_pointer));
    name = ZSTR_VAL(z_name);

    result = weechat_hdata_string (hdata, pointer, (const char *)name);

    API_RETURN_STRING(result);
}

API_FUNC(hdata_pointer)
{
    zend_string *z_hdata, *z_pointer, *z_name;
    struct t_hdata *hdata;
    void *pointer;
    char *name;
    const char *result;

    API_INIT_FUNC(1, "hdata_pointer", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SSS", &z_hdata, &z_pointer,
                               &z_name) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    hdata = (struct t_hdata *)API_STR2PTR(ZSTR_VAL(z_hdata));
    pointer = (void *)API_STR2PTR(ZSTR_VAL(z_pointer));
    name = ZSTR_VAL(z_name);

    result = API_PTR2STR(
        weechat_hdata_pointer (hdata, pointer, (const char *)name));

    API_RETURN_STRING(result);
}

API_FUNC(hdata_time)
{
    zend_string *z_hdata, *z_pointer, *z_name;
    struct t_hdata *hdata;
    void *pointer;
    char *name;
    time_t result;

    API_INIT_FUNC(1, "hdata_time", API_RETURN_LONG(0));
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SSS", &z_hdata, &z_pointer,
                               &z_name) == FAILURE)
        API_WRONG_ARGS(API_RETURN_LONG(0));

    hdata = (struct t_hdata *)API_STR2PTR(ZSTR_VAL(z_hdata));
    pointer = (void *)API_STR2PTR(ZSTR_VAL(z_pointer));
    name = ZSTR_VAL(z_name);

    result = weechat_hdata_time (hdata, pointer, (const char *)name);

    API_RETURN_LONG(result);
}

API_FUNC(hdata_hashtable)
{
    zend_string *z_hdata, *z_pointer, *z_name;
    struct t_hdata *hdata;
    void *pointer;
    char *name;
    struct t_hashtable *result;

    API_INIT_FUNC(1, "hdata_hashtable", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SSS", &z_hdata, &z_pointer,
                               &z_name) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    hdata = (struct t_hdata *)API_STR2PTR(ZSTR_VAL(z_hdata));
    pointer = (void *)API_STR2PTR(ZSTR_VAL(z_pointer));
    name = ZSTR_VAL(z_name);

    result = weechat_hdata_hashtable (hdata, pointer, (const char *)name);

    weechat_php_hashtable_to_array (result, return_value);
}

API_FUNC(hdata_compare)
{
    zend_string *z_hdata, *z_pointer1, *z_pointer2, *z_name;
    zend_long z_case_sensitive;
    struct t_hdata *hdata;
    void *pointer1, *pointer2;
    char *name;
    int case_sensitive, result;

    API_INIT_FUNC(1, "hdata_compare", API_RETURN_INT(0));
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SSSSl", &z_hdata, &z_pointer1,
                               &z_pointer2, &z_name,
                               &z_case_sensitive) == FAILURE)
        API_WRONG_ARGS(API_RETURN_INT(0));

    hdata = (struct t_hdata *)API_STR2PTR(ZSTR_VAL(z_hdata));
    pointer1 = (void *)API_STR2PTR(ZSTR_VAL(z_pointer1));
    pointer2 = (void *)API_STR2PTR(ZSTR_VAL(z_pointer2));
    name = ZSTR_VAL(z_name);
    case_sensitive = (int)z_case_sensitive;

    result = weechat_hdata_compare (hdata, pointer1, pointer2, name,
                                    case_sensitive);

    API_RETURN_INT(result);
}

API_FUNC(hdata_update)
{
    zend_string *z_hdata, *z_pointer;
    zval *z_hashtable;
    int result;
    struct t_hdata *hdata;
    void *pointer;
    struct t_hashtable *hashtable;

    API_INIT_FUNC(1, "hdata_update", API_RETURN_INT(0));
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SSa", &z_hdata, &z_pointer,
                               &z_hashtable) == FAILURE)
        API_WRONG_ARGS(API_RETURN_INT(0));

    hdata = (struct t_hdata *)API_STR2PTR(ZSTR_VAL(z_hdata));
    pointer = (void *)API_STR2PTR(ZSTR_VAL(z_pointer));
    hashtable = weechat_php_array_to_hashtable (
        z_hashtable,
        WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING);

    result = weechat_hdata_update (hdata, pointer, hashtable);

    if (hashtable)
        weechat_hashtable_free (hashtable);

    API_RETURN_INT(result);
}

API_FUNC(hdata_get_string)
{
    zend_string *z_hdata, *z_property;
    struct t_hdata *hdata;
    char *property;
    const char *result;

    API_INIT_FUNC(1, "hdata_get_string", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SS", &z_hdata,
                               &z_property) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    hdata = (struct t_hdata *)API_STR2PTR(ZSTR_VAL(z_hdata));
    property = ZSTR_VAL(z_property);

    result = weechat_hdata_get_string (hdata, (const char *)property);

    API_RETURN_STRING(result);
}

static int
weechat_php_api_upgrade_read_cb (const void *pointer, void *data,
                                 struct t_upgrade_file *upgrade_file,
                                 int object_id,
                                 struct t_infolist *infolist)
{
    int rc;
    void *func_argv[4];

    func_argv[1] = (char *)API_PTR2STR(upgrade_file);
    func_argv[2] = &object_id;
    func_argv[3] = (char *)API_PTR2STR(infolist);

    weechat_php_cb (pointer, data, func_argv, "ssis",
                    WEECHAT_SCRIPT_EXEC_INT, &rc);

    return rc;
}

API_FUNC(upgrade_new)
{
    zend_string *z_filename, *z_data;
    zval *z_callback_read;
    char *filename;
    char *data;
    const char *result;

    API_INIT_FUNC(1, "upgrade_new", API_RETURN_EMPTY);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SzS", &z_filename,
                               &z_callback_read, &z_data) == FAILURE)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    filename = ZSTR_VAL(z_filename);
    weechat_php_get_function_name (z_callback_read, callback_read_name);
    data = ZSTR_VAL(z_data);

    result = API_PTR2STR(
        plugin_script_api_upgrade_new (
            weechat_php_plugin,
            php_current_script,
            (const char *)filename,
            &weechat_php_api_upgrade_read_cb,
            (const char *)callback_read_name,
            (const char *)data));

    API_RETURN_STRING(result);
}

API_FUNC(upgrade_write_object)
{
    zend_string *z_upgrade_file, *z_infolist;
    zend_long z_object_id;
    struct t_upgrade_file *upgrade_file;
    int object_id;
    struct t_infolist *infolist;
    int result;

    API_INIT_FUNC(1, "upgrade_write_object", API_RETURN_INT(0));
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "SlS", &z_upgrade_file,
                               &z_object_id, &z_infolist) == FAILURE)
        API_WRONG_ARGS(API_RETURN_INT(0));

    upgrade_file = (struct t_upgrade_file *)API_STR2PTR(ZSTR_VAL(z_upgrade_file));
    object_id = (int)z_object_id;
    infolist = (struct t_infolist *)API_STR2PTR(ZSTR_VAL(z_infolist));

    result = weechat_upgrade_write_object (upgrade_file, object_id, infolist);

    API_RETURN_INT(result);
}

API_FUNC(upgrade_read)
{
    zend_string *z_upgrade_file;
    struct t_upgrade_file *upgrade_file;
    int result;

    API_INIT_FUNC(1, "upgrade_read", API_RETURN_INT(0));
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S",
                               &z_upgrade_file) == FAILURE)
        API_WRONG_ARGS(API_RETURN_INT(0));

    upgrade_file = (struct t_upgrade_file *)API_STR2PTR(ZSTR_VAL(z_upgrade_file));

    result = weechat_upgrade_read (upgrade_file);

    API_RETURN_INT(result);
}

API_FUNC(upgrade_close)
{
    zend_string *z_upgrade_file;
    struct t_upgrade_file *upgrade_file;

    API_INIT_FUNC(1, "upgrade_close", API_RETURN_ERROR);
    if (zend_parse_parameters (ZEND_NUM_ARGS(), "S",
                               &z_upgrade_file) == FAILURE)
        API_WRONG_ARGS(API_RETURN_ERROR);

    upgrade_file = (struct t_upgrade_file *)API_STR2PTR(ZSTR_VAL(z_upgrade_file));

    weechat_upgrade_close (upgrade_file);

    API_RETURN_OK;
}

static void
forget_hash_entry (HashTable *ht, INTERNAL_FUNCTION_PARAMETERS)
{
#if PHP_VERSION_ID >= 80000
    /* make C compiler happy */
    (void) ht;
    (void) execute_data;

    RETURN_FALSE;
#else
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
#endif
}

PHP_FUNCTION(forget_class)
{
    forget_hash_entry (EG(class_table), INTERNAL_FUNCTION_PARAM_PASSTHRU);
}

PHP_FUNCTION(forget_function)
{
    forget_hash_entry (EG(function_table), INTERNAL_FUNCTION_PARAM_PASSTHRU);
}
