/*
 * weechat-guile-api.c - guile API functions
 *
 * Copyright (C) 2011-2024 Sébastien Helleu <flashcode@flashtux.org>
 * Copyright (C) 2012 Simon Arlott
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

#undef _

#include <libguile.h>
#include <time.h>
#include <stdio.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "../plugin-script.h"
#include "../plugin-script-api.h"
#include "weechat-guile.h"


/* max strings created by an API function */
#define GUILE_MAX_STRINGS 64

#define API_INIT_FUNC(__init, __name, __ret)                            \
    char *guile_function_name = __name;                                 \
    char *guile_strings[GUILE_MAX_STRINGS];                             \
    int guile_num_strings = 0;                                          \
    if (__init                                                          \
        && (!guile_current_script || !guile_current_script->name))      \
    {                                                                   \
        WEECHAT_SCRIPT_MSG_NOT_INIT(GUILE_CURRENT_SCRIPT_NAME,          \
                                    guile_function_name);               \
        __ret;                                                          \
    }
#define API_WRONG_ARGS(__ret)                                           \
    {                                                                   \
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(GUILE_CURRENT_SCRIPT_NAME,        \
                                      guile_function_name);             \
        __ret;                                                          \
    }
#define API_PTR2STR(__pointer)                                          \
    plugin_script_ptr2str (__pointer)
#define API_STR2PTR(__string)                                           \
    plugin_script_str2ptr (weechat_guile_plugin,                        \
                           GUILE_CURRENT_SCRIPT_NAME,                   \
                           guile_function_name, __string)
#define API_SCM_TO_STRING(__str)                                        \
    weechat_guile_api_scm_to_string (__str,                             \
                                     guile_strings, &guile_num_strings)
#define API_FREE_STRINGS                                                \
    if (guile_num_strings > 0)                                          \
    {                                                                   \
        weechat_guile_api_free_strings (guile_strings,                  \
                                        &guile_num_strings);            \
    }
#define API_RETURN_OK                                                   \
    API_FREE_STRINGS;                                                   \
    return scm_from_int (1)
#define API_RETURN_ERROR                                                \
    API_FREE_STRINGS                                                    \
    return scm_from_int (0)
#define API_RETURN_EMPTY                                                \
    API_FREE_STRINGS;                                                   \
    return scm_from_locale_string ("")
#define API_RETURN_STRING(__string)                                     \
    return_value = scm_from_locale_string ((__string) ? __string : ""); \
    API_FREE_STRINGS;                                                   \
    return return_value
#define API_RETURN_STRING_FREE(__string)                                \
    return_value = scm_from_locale_string ((__string) ? __string : ""); \
    free (__string);                                                    \
    API_FREE_STRINGS;                                                   \
    return return_value
#define API_RETURN_INT(__int)                                           \
    API_FREE_STRINGS;                                                   \
    return scm_from_int (__int)
#define API_RETURN_LONG(__long)                                         \
    API_FREE_STRINGS;                                                   \
    return scm_from_long (__long)
#define API_RETURN_LONGLONG(__long)                                     \
    API_FREE_STRINGS;                                                   \
    return scm_from_long_long (__long)
#define API_RETURN_OTHER(__scm)                                         \
    API_FREE_STRINGS;                                                   \
    return __scm

#define API_DEF_FUNC(__name, __argc)                                    \
    scm_c_define_gsubr ("weechat:" #__name, __argc, 0, 0,               \
                        &weechat_guile_api_##__name);                   \
    scm_c_export ("weechat:" #__name, NULL);


/*
 * Converts a guile string into a C string.
 *
 * The result is stored in an array, and will be freed later by a call to
 * weechat_guile_free_strings().
 */

char *
weechat_guile_api_scm_to_string (SCM str,
                                 char *guile_strings[], int *guile_num_strings)
{
    if (scm_is_null (str))
        return NULL;

    /*
     * if array is full, just return string without using length
     * (this should never happen, the array should be large enough for any API
     * function!)
     */
    if (*guile_num_strings + 1 >= GUILE_MAX_STRINGS)
        return (char *)scm_i_string_chars (str);

    guile_strings[*guile_num_strings] = scm_to_locale_string (str);
    (*guile_num_strings)++;

    return guile_strings[*guile_num_strings - 1];
}

/*
 * Frees all allocated strings in "guile_strings".
 */

void
weechat_guile_api_free_strings (char *guile_strings[], int *guile_num_strings)
{
    int i;

    for (i = 0; i < *guile_num_strings; i++)
    {
        free (guile_strings[i]);
    }

    *guile_num_strings = 0;
}

/*
 * Registers a guile script.
 */

SCM
weechat_guile_api_register (SCM name, SCM author, SCM version, SCM license,
                            SCM description, SCM shutdown_func, SCM charset)
{
    API_INIT_FUNC(0, "register", API_RETURN_ERROR);
    if (guile_registered_script)
    {
        /* script already registered */
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: script \"%s\" already "
                                         "registered (register ignored)"),
                        weechat_prefix ("error"), GUILE_PLUGIN_NAME,
                        guile_registered_script->name);
        API_RETURN_ERROR;
    }
    guile_current_script = NULL;
    guile_registered_script = NULL;
    if (!scm_is_string (name) || !scm_is_string (author)
        || !scm_is_string (version) || !scm_is_string (license)
        || !scm_is_string (description) || !scm_is_string (shutdown_func)
        || !scm_is_string (charset))
        API_WRONG_ARGS(API_RETURN_ERROR);

    if (plugin_script_search (guile_scripts, API_SCM_TO_STRING(name)))
    {
        /* another script already exists with same name */
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to register script "
                                         "\"%s\" (another script already "
                                         "exists with this name)"),
                        weechat_prefix ("error"), GUILE_PLUGIN_NAME, name);
        API_RETURN_ERROR;
    }

    /* register script */
    guile_current_script = plugin_script_add (weechat_guile_plugin,
                                              &guile_data,
                                              (guile_current_script_filename) ?
                                              guile_current_script_filename : "",
                                              API_SCM_TO_STRING(name),
                                              API_SCM_TO_STRING(author),
                                              API_SCM_TO_STRING(version),
                                              API_SCM_TO_STRING(license),
                                              API_SCM_TO_STRING(description),
                                              API_SCM_TO_STRING(shutdown_func),
                                              API_SCM_TO_STRING(charset));
    if (guile_current_script)
    {
        guile_registered_script = guile_current_script;
        if ((weechat_guile_plugin->debug >= 2) || !guile_quiet)
        {
            weechat_printf (NULL,
                            weechat_gettext ("%s: registered script \"%s\", "
                                             "version %s (%s)"),
                            GUILE_PLUGIN_NAME,
                            API_SCM_TO_STRING(name),
                            API_SCM_TO_STRING(version),
                            API_SCM_TO_STRING(description));
        }
        guile_current_script->interpreter = scm_current_module ();
    }
    else
    {
        API_RETURN_ERROR;
    }

    API_RETURN_OK;
}

/*
 * Wrappers for functions in scripting API.
 *
 * For more info about these functions, look at their implementation in WeeChat
 * core.
 */

SCM
weechat_guile_api_plugin_get_name (SCM plugin)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "plugin_get_name", API_RETURN_EMPTY);
    if (!scm_is_string (plugin))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_plugin_get_name (API_STR2PTR(API_SCM_TO_STRING(plugin)));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_charset_set (SCM charset)
{
    API_INIT_FUNC(1, "charset_set", API_RETURN_ERROR);
    if (!scm_is_string (charset))
        API_WRONG_ARGS(API_RETURN_ERROR);

    plugin_script_api_charset_set (guile_current_script, API_SCM_TO_STRING(charset));

    API_RETURN_OK;
}

SCM
weechat_guile_api_iconv_to_internal (SCM charset, SCM string)
{
    char *result;
    SCM return_value;

    API_INIT_FUNC(1, "iconv_to_internal", API_RETURN_EMPTY);
    if (!scm_is_string (charset) || !scm_is_string (string))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_iconv_to_internal (API_SCM_TO_STRING(charset),
                                        API_SCM_TO_STRING(string));

    API_RETURN_STRING_FREE(result);
}

SCM
weechat_guile_api_iconv_from_internal (SCM charset, SCM string)
{
    char *result;
    SCM return_value;

    API_INIT_FUNC(1, "iconv_from_internal", API_RETURN_EMPTY);
    if (!scm_is_string (charset) || !scm_is_string (string))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_iconv_from_internal (API_SCM_TO_STRING(charset),
                                          API_SCM_TO_STRING(string));

    API_RETURN_STRING_FREE(result);
}

SCM
weechat_guile_api_gettext (SCM string)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "gettext", API_RETURN_EMPTY);
    if (!scm_is_string (string))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_gettext (API_SCM_TO_STRING(string));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_ngettext (SCM single, SCM plural, SCM count)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "ngettext", API_RETURN_EMPTY);
    if (!scm_is_string (single) || !scm_is_string (plural)
        || !scm_is_integer (count))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_ngettext (API_SCM_TO_STRING(single),
                               API_SCM_TO_STRING(plural),
                               scm_to_int (count));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_strlen_screen (SCM string)
{
    int value;

    API_INIT_FUNC(1, "strlen_screen", API_RETURN_INT(0));
    if (!scm_is_string (string))
        API_WRONG_ARGS(API_RETURN_INT(0));

    value = weechat_strlen_screen (API_SCM_TO_STRING(string));

    API_RETURN_INT(value);
}

SCM
weechat_guile_api_string_match (SCM string, SCM mask, SCM case_sensitive)
{
    int value;

    API_INIT_FUNC(1, "string_match", API_RETURN_INT(0));
    if (!scm_is_string (string) || !scm_is_string (mask)
        || !scm_is_integer (case_sensitive))
        API_WRONG_ARGS(API_RETURN_INT(0));

    value = weechat_string_match (API_SCM_TO_STRING(string),
                                  API_SCM_TO_STRING(mask),
                                  scm_to_int (case_sensitive));

    API_RETURN_INT(value);
}

SCM
weechat_guile_api_string_match_list (SCM string, SCM masks, SCM case_sensitive)
{
    int value;

    API_INIT_FUNC(1, "string_match_list", API_RETURN_INT(0));
    if (!scm_is_string (string) || !scm_is_string (masks)
        || !scm_is_integer (case_sensitive))
        API_WRONG_ARGS(API_RETURN_INT(0));

    value = plugin_script_api_string_match_list (weechat_guile_plugin,
                                                 API_SCM_TO_STRING(string),
                                                 API_SCM_TO_STRING(masks),
                                                 scm_to_int (case_sensitive));

    API_RETURN_INT(value);
}

SCM
weechat_guile_api_string_has_highlight (SCM string, SCM highlight_words)
{
    int value;

    API_INIT_FUNC(1, "string_has_highlight", API_RETURN_INT(0));
    if (!scm_is_string (string) || !scm_is_string (highlight_words))
        API_WRONG_ARGS(API_RETURN_INT(0));

    value = weechat_string_has_highlight (API_SCM_TO_STRING(string),
                                          API_SCM_TO_STRING(highlight_words));

    API_RETURN_INT(value);
}

SCM
weechat_guile_api_string_has_highlight_regex (SCM string, SCM regex)
{
    int value;

    API_INIT_FUNC(1, "string_has_highlight_regex", API_RETURN_INT(0));
    if (!scm_is_string (string) || !scm_is_string (regex))
        API_WRONG_ARGS(API_RETURN_INT(0));

    value = weechat_string_has_highlight_regex (API_SCM_TO_STRING(string),
                                                API_SCM_TO_STRING(regex));

    API_RETURN_INT(value);
}

SCM
weechat_guile_api_string_mask_to_regex (SCM mask)
{
    char *result;
    SCM return_value;

    API_INIT_FUNC(1, "string_mask_to_regex", API_RETURN_EMPTY);
    if (!scm_is_string (mask))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_string_mask_to_regex (API_SCM_TO_STRING(mask));

    API_RETURN_STRING_FREE(result);
}

SCM
weechat_guile_api_string_format_size (SCM size)
{
    char *result;
    SCM return_value;

    API_INIT_FUNC(1, "string_format_size", API_RETURN_EMPTY);
    if (!scm_is_integer (size))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_string_format_size (scm_to_ulong_long (size));

    API_RETURN_STRING_FREE(result);
}

SCM
weechat_guile_api_string_parse_size (SCM size)
{
    unsigned long long value;

    API_INIT_FUNC(1, "string_parse_size", API_RETURN_LONG(0));
    if (!scm_is_string (size))
        API_WRONG_ARGS(API_RETURN_LONG(0));

    value = weechat_string_parse_size (API_SCM_TO_STRING(size));

    API_RETURN_LONG(value);
}

SCM
weechat_guile_api_string_color_code_size (SCM string)
{
    int size;

    API_INIT_FUNC(1, "string_color_code_size", API_RETURN_INT(0));
    if (!scm_is_string (string))
        API_WRONG_ARGS(API_RETURN_INT(0));

    size = weechat_string_color_code_size (API_SCM_TO_STRING(string));

    API_RETURN_INT(size);
}

SCM
weechat_guile_api_string_remove_color (SCM string, SCM replacement)
{
    char *result;
    SCM return_value;

    API_INIT_FUNC(1, "string_remove_color", API_RETURN_EMPTY);
    if (!scm_is_string (string) || !scm_is_string (replacement))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_string_remove_color (API_SCM_TO_STRING(string),
                                          API_SCM_TO_STRING(replacement));

    API_RETURN_STRING_FREE(result);
}

SCM
weechat_guile_api_string_is_command_char (SCM string)
{
    int value;

    API_INIT_FUNC(1, "string_is_command_char", API_RETURN_INT(0));
    if (!scm_is_string (string))
        API_WRONG_ARGS(API_RETURN_INT(0));

    value = weechat_string_is_command_char (API_SCM_TO_STRING(string));

    API_RETURN_INT(value);
}

SCM
weechat_guile_api_string_input_for_buffer (SCM string)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "string_input_for_buffer", API_RETURN_EMPTY);
    if (!scm_is_string (string))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_string_input_for_buffer (API_SCM_TO_STRING(string));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_string_eval_expression (SCM expr, SCM pointers,
                                          SCM extra_vars, SCM options)
{
    char *result;
    SCM return_value;
    struct t_hashtable *c_pointers, *c_extra_vars, *c_options;

    API_INIT_FUNC(1, "string_eval_expression", API_RETURN_EMPTY);
    if (!scm_is_string (expr) || !scm_list_p (pointers)
        || !scm_list_p (extra_vars) || !scm_list_p (options))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    c_pointers = weechat_guile_alist_to_hashtable (pointers,
                                                   WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                                   WEECHAT_HASHTABLE_STRING,
                                                   WEECHAT_HASHTABLE_POINTER);
    c_extra_vars = weechat_guile_alist_to_hashtable (extra_vars,
                                                     WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                                     WEECHAT_HASHTABLE_STRING,
                                                     WEECHAT_HASHTABLE_STRING);
    c_options = weechat_guile_alist_to_hashtable (options,
                                                  WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                                  WEECHAT_HASHTABLE_STRING,
                                                  WEECHAT_HASHTABLE_STRING);

    result = weechat_string_eval_expression (API_SCM_TO_STRING(expr),
                                             c_pointers, c_extra_vars,
                                             c_options);

    weechat_hashtable_free (c_pointers);
    weechat_hashtable_free (c_extra_vars);
    weechat_hashtable_free (c_options);

    API_RETURN_STRING_FREE(result);
}

SCM
weechat_guile_api_string_eval_path_home (SCM path, SCM pointers,
                                         SCM extra_vars, SCM options)
{
    char *result;
    SCM return_value;
    struct t_hashtable *c_pointers, *c_extra_vars, *c_options;

    API_INIT_FUNC(1, "string_eval_path_home", API_RETURN_EMPTY);
    if (!scm_is_string (path) || !scm_list_p (pointers)
        || !scm_list_p (extra_vars) || !scm_list_p (options))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    c_pointers = weechat_guile_alist_to_hashtable (
        pointers,
        WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_POINTER);
    c_extra_vars = weechat_guile_alist_to_hashtable (
        extra_vars,
        WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING);
    c_options = weechat_guile_alist_to_hashtable (
        options,
        WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING);

    result = weechat_string_eval_path_home (API_SCM_TO_STRING(path),
                                            c_pointers, c_extra_vars,
                                            c_options);

    weechat_hashtable_free (c_pointers);
    weechat_hashtable_free (c_extra_vars);
    weechat_hashtable_free (c_options);

    API_RETURN_STRING_FREE(result);
}

SCM
weechat_guile_api_mkdir_home (SCM directory, SCM mode)
{
    API_INIT_FUNC(1, "mkdir_home", API_RETURN_ERROR);
    if (!scm_is_string (directory) || !scm_is_integer (mode))
        API_WRONG_ARGS(API_RETURN_ERROR);

    if (weechat_mkdir_home (API_SCM_TO_STRING(directory), scm_to_int (mode)))
        API_RETURN_OK;

    API_RETURN_ERROR;
}

SCM
weechat_guile_api_mkdir (SCM directory, SCM mode)
{
    API_INIT_FUNC(1, "mkdir", API_RETURN_ERROR);
    if (!scm_is_string (directory) || !scm_is_integer (mode))
        API_WRONG_ARGS(API_RETURN_ERROR);

    if (weechat_mkdir (API_SCM_TO_STRING(directory), scm_to_int (mode)))
        API_RETURN_OK;

    API_RETURN_ERROR;
}

SCM
weechat_guile_api_mkdir_parents (SCM directory, SCM mode)
{
    API_INIT_FUNC(1, "mkdir_parents", API_RETURN_ERROR);
    if (!scm_is_string (directory) || !scm_is_integer (mode))
        API_WRONG_ARGS(API_RETURN_ERROR);

    if (weechat_mkdir_parents (API_SCM_TO_STRING(directory), scm_to_int (mode)))
        API_RETURN_OK;

    API_RETURN_ERROR;
}

SCM
weechat_guile_api_list_new ()
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "list_new", API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_list_new ());

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_list_add (SCM weelist, SCM data, SCM where, SCM user_data)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "list_add", API_RETURN_EMPTY);
    if (!scm_is_string (weelist) || !scm_is_string (data)
        || !scm_is_string (where) || !scm_is_string (user_data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_list_add (API_STR2PTR(API_SCM_TO_STRING(weelist)),
                                           API_SCM_TO_STRING(data),
                                           API_SCM_TO_STRING(where),
                                           API_STR2PTR(API_SCM_TO_STRING(user_data))));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_list_search (SCM weelist, SCM data)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "list_search", API_RETURN_EMPTY);
    if (!scm_is_string (weelist) || !scm_is_string (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_list_search (API_STR2PTR(API_SCM_TO_STRING(weelist)),
                                              API_SCM_TO_STRING(data)));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_list_search_pos (SCM weelist, SCM data)
{
    int pos;

    API_INIT_FUNC(1, "list_search_pos", API_RETURN_INT(-1));
    if (!scm_is_string (weelist) || !scm_is_string (data))
        API_WRONG_ARGS(API_RETURN_INT(-1));

    pos = weechat_list_search_pos (API_STR2PTR(API_SCM_TO_STRING(weelist)),
                                   API_SCM_TO_STRING(data));

    API_RETURN_INT(pos);
}

SCM
weechat_guile_api_list_casesearch (SCM weelist, SCM data)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "list_casesearch", API_RETURN_EMPTY);
    if (!scm_is_string (weelist) || !scm_is_string (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_list_casesearch (API_STR2PTR(API_SCM_TO_STRING(weelist)),
                                                  API_SCM_TO_STRING(data)));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_list_casesearch_pos (SCM weelist, SCM data)
{
    int pos;

    API_INIT_FUNC(1, "list_casesearch_pos", API_RETURN_INT(-1));
    if (!scm_is_string (weelist) || !scm_is_string (data))
        API_WRONG_ARGS(API_RETURN_INT(-1));

    pos = weechat_list_casesearch_pos (API_STR2PTR(API_SCM_TO_STRING(weelist)),
                                       API_SCM_TO_STRING(data));

    API_RETURN_INT(pos);
}

SCM
weechat_guile_api_list_get (SCM weelist, SCM position)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "list_get", API_RETURN_EMPTY);
    if (!scm_is_string (weelist) || !scm_is_integer (position))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_list_get (API_STR2PTR(API_SCM_TO_STRING(weelist)),
                                           scm_to_int (position)));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_list_set (SCM item, SCM new_value)
{
    API_INIT_FUNC(1, "list_set", API_RETURN_ERROR);
    if (!scm_is_string (item) || !scm_is_string (new_value))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_list_set (API_STR2PTR(API_SCM_TO_STRING(item)),
                      API_SCM_TO_STRING(new_value));

    API_RETURN_OK;
}

SCM
weechat_guile_api_list_next (SCM item)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "list_next", API_RETURN_EMPTY);
    if (!scm_is_string (item))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_list_next (API_STR2PTR(API_SCM_TO_STRING(item))));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_list_prev (SCM item)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "list_prev", API_RETURN_EMPTY);
    if (!scm_is_string (item))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_list_prev (API_STR2PTR(API_SCM_TO_STRING(item))));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_list_string (SCM item)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "list_string", API_RETURN_EMPTY);
    if (!scm_is_string (item))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_list_string (API_STR2PTR(API_SCM_TO_STRING(item)));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_list_size (SCM weelist)
{
    int size;

    API_INIT_FUNC(1, "list_size", API_RETURN_INT(0));
    if (!scm_is_string (weelist))
        API_WRONG_ARGS(API_RETURN_INT(0));

    size = weechat_list_size (API_STR2PTR(API_SCM_TO_STRING(weelist)));

    API_RETURN_INT(size);
}

SCM
weechat_guile_api_list_remove (SCM weelist, SCM item)
{
    API_INIT_FUNC(1, "list_remove", API_RETURN_ERROR);
    if (!scm_is_string (weelist) || !scm_is_string (item))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_list_remove (API_STR2PTR(API_SCM_TO_STRING(weelist)),
                         API_STR2PTR(API_SCM_TO_STRING(item)));

    API_RETURN_OK;
}

SCM
weechat_guile_api_list_remove_all (SCM weelist)
{
    API_INIT_FUNC(1, "list_remove_all", API_RETURN_ERROR);
    if (!scm_is_string (weelist))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_list_remove_all (API_STR2PTR(API_SCM_TO_STRING(weelist)));

    API_RETURN_OK;
}

SCM
weechat_guile_api_list_free (SCM weelist)
{
    API_INIT_FUNC(1, "list_free", API_RETURN_ERROR);
    if (!scm_is_string (weelist))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_list_free (API_STR2PTR(API_SCM_TO_STRING(weelist)));

    API_RETURN_OK;
}

int
weechat_guile_api_config_reload_cb (const void *pointer, void *data,
                                    struct t_config_file *config_file)
{
    struct t_plugin_script *script;
    void *func_argv[2];
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;
    int *rc, ret;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = (char *)API_PTR2STR(config_file);

        rc = (int *) weechat_guile_exec (script,
                                         WEECHAT_SCRIPT_EXEC_INT,
                                         ptr_function,
                                         "ss", func_argv);

        if (!rc)
            ret = WEECHAT_CONFIG_READ_FILE_NOT_FOUND;
        else
        {
            ret = *rc;
            free (rc);
        }

        return ret;
    }

    return WEECHAT_CONFIG_READ_FILE_NOT_FOUND;
}

SCM
weechat_guile_api_config_new (SCM name, SCM function, SCM data)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "config_new", API_RETURN_EMPTY);
    if (!scm_is_string (name) || !scm_is_string (function)
        || !scm_is_string (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(plugin_script_api_config_new (weechat_guile_plugin,
                                                       guile_current_script,
                                                       API_SCM_TO_STRING(name),
                                                       &weechat_guile_api_config_reload_cb,
                                                       API_SCM_TO_STRING(function),
                                                       API_SCM_TO_STRING(data)));

    API_RETURN_STRING(result);
}

struct t_hashtable *
weechat_guile_api_config_update_cb (const void *pointer, void *data,
                                    struct t_config_file *config_file,
                                    int version_read,
                                    struct t_hashtable *data_read)
{
    struct t_plugin_script *script;
    void *func_argv[4];
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;
    struct t_hashtable *ret_hashtable;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = (char *)API_PTR2STR(config_file);
        func_argv[2] = &version_read;
        func_argv[3] = data_read;

        ret_hashtable = weechat_guile_exec (script,
                                            WEECHAT_SCRIPT_EXEC_HASHTABLE,
                                            ptr_function,
                                            "ssih", func_argv);

        return ret_hashtable;
    }

    return NULL;
}

SCM
weechat_guile_api_config_set_version (SCM config_file, SCM version,
                                      SCM function, SCM data)
{
    int rc;

    API_INIT_FUNC(1, "config_set_version", API_RETURN_INT(0));
    if (!scm_is_string (config_file) || !scm_is_integer (version)
        || !scm_is_string (function) || !scm_is_string (data))
        API_WRONG_ARGS(API_RETURN_INT(0));

    rc = plugin_script_api_config_set_version (
        weechat_guile_plugin,
        guile_current_script,
        API_STR2PTR(API_SCM_TO_STRING(config_file)),
        scm_to_int (version),
        &weechat_guile_api_config_update_cb,
        API_SCM_TO_STRING(function),
        API_SCM_TO_STRING(data));

    API_RETURN_INT(rc);
}

int
weechat_guile_api_config_read_cb (const void *pointer, void *data,
                                  struct t_config_file *config_file,
                                  struct t_config_section *section,
                                  const char *option_name, const char *value)
{
    struct t_plugin_script *script;
    void *func_argv[5];
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;
    int *rc, ret;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = (char *)API_PTR2STR(config_file);
        func_argv[2] = (char *)API_PTR2STR(section);
        func_argv[3] = (option_name) ? (char *)option_name : empty_arg;
        func_argv[4] = (value) ? (char *)value : NULL;

        rc = (int *) weechat_guile_exec (script,
                                         WEECHAT_SCRIPT_EXEC_INT,
                                         ptr_function,
                                         "sssss", func_argv);

        if (!rc)
            ret = WEECHAT_CONFIG_OPTION_SET_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }

        return ret;
    }

    return WEECHAT_CONFIG_OPTION_SET_ERROR;
}

int
weechat_guile_api_config_section_write_cb (const void *pointer, void *data,
                                           struct t_config_file *config_file,
                                           const char *section_name)
{
    struct t_plugin_script *script;
    void *func_argv[3];
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;
    int *rc, ret;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = (char *)API_PTR2STR(config_file);
        func_argv[2] = (section_name) ? (char *)section_name : empty_arg;

        rc = (int *) weechat_guile_exec (script,
                                         WEECHAT_SCRIPT_EXEC_INT,
                                         ptr_function,
                                         "sss", func_argv);

        if (!rc)
            ret = WEECHAT_CONFIG_WRITE_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }

        return ret;
    }

    return WEECHAT_CONFIG_WRITE_ERROR;
}

int
weechat_guile_api_config_section_write_default_cb (const void *pointer, void *data,
                                                   struct t_config_file *config_file,
                                                   const char *section_name)
{
    struct t_plugin_script *script;
    void *func_argv[3];
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;
    int *rc, ret;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = (char *)API_PTR2STR(config_file);
        func_argv[2] = (section_name) ? (char *)section_name : empty_arg;

        rc = (int *) weechat_guile_exec (script,
                                         WEECHAT_SCRIPT_EXEC_INT,
                                         ptr_function,
                                         "sss", func_argv);

        if (!rc)
            ret = WEECHAT_CONFIG_WRITE_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }

        return ret;
    }

    return WEECHAT_CONFIG_WRITE_ERROR;
}

int
weechat_guile_api_config_section_create_option_cb (const void *pointer, void *data,
                                                   struct t_config_file *config_file,
                                                   struct t_config_section *section,
                                                   const char *option_name,
                                                   const char *value)
{
    struct t_plugin_script *script;
    void *func_argv[5];
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;
    int *rc, ret;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = (char *)API_PTR2STR(config_file);
        func_argv[2] = (char *)API_PTR2STR(section);
        func_argv[3] = (option_name) ? (char *)option_name : empty_arg;
        func_argv[4] = (value) ? (char *)value : NULL;

        rc = (int *) weechat_guile_exec (script,
                                         WEECHAT_SCRIPT_EXEC_INT,
                                         ptr_function,
                                         "sssss", func_argv);

        if (!rc)
            ret = WEECHAT_CONFIG_OPTION_SET_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }

        return ret;
    }

    return WEECHAT_CONFIG_OPTION_SET_ERROR;
}

int
weechat_guile_api_config_section_delete_option_cb (const void *pointer, void *data,
                                                   struct t_config_file *config_file,
                                                   struct t_config_section *section,
                                                   struct t_config_option *option)
{
    struct t_plugin_script *script;
    void *func_argv[4];
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;
    int *rc, ret;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = (char *)API_PTR2STR(config_file);
        func_argv[2] = (char *)API_PTR2STR(section);
        func_argv[3] = (char *)API_PTR2STR(option);

        rc = (int *) weechat_guile_exec (script,
                                         WEECHAT_SCRIPT_EXEC_INT,
                                         ptr_function,
                                         "ssss", func_argv);

        if (!rc)
            ret = WEECHAT_CONFIG_OPTION_UNSET_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }

        return ret;
    }

    return WEECHAT_CONFIG_OPTION_UNSET_ERROR;
}

SCM
weechat_guile_api_config_new_section (SCM args)
{
    SCM config_file, name, user_can_add_options, user_can_delete_options;
    SCM function_read, data_read, function_write, data_write;
    SCM function_write_default, data_write_default, function_create_option;
    SCM data_create_option, function_delete_option, data_delete_option;
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "config_new_section", API_RETURN_EMPTY);
    if (!scm_list_p (args) || (scm_to_int (scm_length (args)) != 14))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    config_file = scm_list_ref (args, scm_from_int (0));
    name = scm_list_ref (args, scm_from_int (1));
    user_can_add_options = scm_list_ref (args, scm_from_int (2));
    user_can_delete_options = scm_list_ref (args, scm_from_int (3));
    function_read = scm_list_ref (args, scm_from_int (4));
    data_read = scm_list_ref (args, scm_from_int (5));
    function_write = scm_list_ref (args, scm_from_int (6));
    data_write = scm_list_ref (args, scm_from_int (7));
    function_write_default = scm_list_ref (args, scm_from_int (8));
    data_write_default = scm_list_ref (args, scm_from_int (9));
    function_create_option = scm_list_ref (args, scm_from_int (10));
    data_create_option = scm_list_ref (args, scm_from_int (11));
    function_delete_option = scm_list_ref (args, scm_from_int (12));
    data_delete_option = scm_list_ref (args, scm_from_int (13));

    if (!scm_is_string (config_file) || !scm_is_string (name)
        || !scm_is_integer (user_can_add_options) || !scm_is_integer (user_can_delete_options)
        || !scm_is_string (function_read) || !scm_is_string (data_read)
        || !scm_is_string (function_write) || !scm_is_string (data_write)
        || !scm_is_string (function_write_default) || !scm_is_string (data_write_default)
        || !scm_is_string (function_create_option) || !scm_is_string (data_create_option)
        || !scm_is_string (function_delete_option) || !scm_is_string (data_delete_option))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(
        plugin_script_api_config_new_section (
            weechat_guile_plugin,
            guile_current_script,
            API_STR2PTR(API_SCM_TO_STRING(config_file)),
            API_SCM_TO_STRING(name),
            scm_to_int (user_can_add_options),
            scm_to_int (user_can_delete_options),
            &weechat_guile_api_config_read_cb,
            API_SCM_TO_STRING(function_read),
            API_SCM_TO_STRING(data_read),
            &weechat_guile_api_config_section_write_cb,
            API_SCM_TO_STRING(function_write),
            API_SCM_TO_STRING(data_write),
            &weechat_guile_api_config_section_write_default_cb,
            API_SCM_TO_STRING(function_write_default),
            API_SCM_TO_STRING(data_write_default),
            &weechat_guile_api_config_section_create_option_cb,
            API_SCM_TO_STRING(function_create_option),
            API_SCM_TO_STRING(data_create_option),
            &weechat_guile_api_config_section_delete_option_cb,
            API_SCM_TO_STRING(function_delete_option),
            API_SCM_TO_STRING(data_delete_option)));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_config_search_section (SCM config_file, SCM section_name)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "config_search_section", API_RETURN_EMPTY);
    if (!scm_is_string (config_file) || !scm_is_string (section_name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_config_search_section (API_STR2PTR(API_SCM_TO_STRING(config_file)),
                                                        API_SCM_TO_STRING(section_name)));

    API_RETURN_STRING(result);
}

int
weechat_guile_api_config_option_check_value_cb (const void *pointer, void *data,
                                                struct t_config_option *option,
                                                const char *value)
{
    struct t_plugin_script *script;
    void *func_argv[3];
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;
    int *rc, ret;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = (char *)API_PTR2STR(option);
        func_argv[2] = (value) ? (char *)value : empty_arg;

        rc = (int *) weechat_guile_exec (script,
                                         WEECHAT_SCRIPT_EXEC_INT,
                                         ptr_function,
                                         "sss", func_argv);

        if (!rc)
            ret = 0;
        else
        {
            ret = *rc;
            free (rc);
        }

        return ret;
    }

    return 0;
}

void
weechat_guile_api_config_option_change_cb (const void *pointer, void *data,
                                           struct t_config_option *option)
{
    struct t_plugin_script *script;
    void *func_argv[2], *rc;
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = (char *)API_PTR2STR(option);

        rc = weechat_guile_exec (script,
                                 WEECHAT_SCRIPT_EXEC_IGNORE,
                                 ptr_function,
                                 "ss", func_argv);
        free (rc);
    }
}

void
weechat_guile_api_config_option_delete_cb (const void *pointer, void *data,
                                           struct t_config_option *option)
{
    struct t_plugin_script *script;
    void *func_argv[2], *rc;
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = (char *)API_PTR2STR(option);

        rc = weechat_guile_exec (script,
                                 WEECHAT_SCRIPT_EXEC_IGNORE,
                                 ptr_function,
                                 "ss", func_argv);
        free (rc);
    }
}

SCM
weechat_guile_api_config_new_option (SCM args)
{
    SCM config_file, section, name, type, description, string_values, min;
    SCM max, default_value, value, null_value_allowed, function_check_value;
    SCM data_check_value, function_change, data_change, function_delete;
    SCM data_delete;
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "config_new_option", API_RETURN_EMPTY);
    if (!scm_list_p (args) || (scm_to_int (scm_length (args)) != 17))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    config_file = scm_list_ref (args, scm_from_int (0));
    section = scm_list_ref (args, scm_from_int (1));
    name = scm_list_ref (args, scm_from_int (2));
    type = scm_list_ref (args, scm_from_int (3));
    description = scm_list_ref (args, scm_from_int (4));
    string_values = scm_list_ref (args, scm_from_int (5));
    min = scm_list_ref (args, scm_from_int (6));
    max = scm_list_ref (args, scm_from_int (7));
    default_value = scm_list_ref (args, scm_from_int (8));
    value = scm_list_ref (args, scm_from_int (9));
    null_value_allowed = scm_list_ref (args, scm_from_int (10));
    function_check_value = scm_list_ref (args, scm_from_int (11));
    data_check_value = scm_list_ref (args, scm_from_int (12));
    function_change = scm_list_ref (args, scm_from_int (13));
    data_change = scm_list_ref (args, scm_from_int (14));
    function_delete = scm_list_ref (args, scm_from_int (15));
    data_delete = scm_list_ref (args, scm_from_int (16));

    if (!scm_is_string (config_file) || !scm_is_string (section)
        || !scm_is_string (name) || !scm_is_string (type)
        || !scm_is_string (description) || !scm_is_string (string_values)
        || !scm_is_integer (min) || !scm_is_integer (max)
        || !(scm_is_null (default_value) || scm_is_string (default_value))
        || !(scm_is_null (value) || scm_is_string (value))
        || !scm_is_integer (null_value_allowed)
        || !scm_is_string (function_check_value)
        || !scm_is_string (data_check_value)
        || !scm_is_string (function_change) || !scm_is_string (data_change)
        || !scm_is_string (function_delete) || !scm_is_string (data_delete))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(plugin_script_api_config_new_option (weechat_guile_plugin,
                                                              guile_current_script,
                                                              API_STR2PTR(API_SCM_TO_STRING(config_file)),
                                                              API_STR2PTR(API_SCM_TO_STRING(section)),
                                                              API_SCM_TO_STRING(name),
                                                              API_SCM_TO_STRING(type),
                                                              API_SCM_TO_STRING(description),
                                                              API_SCM_TO_STRING(string_values),
                                                              scm_to_int (min),
                                                              scm_to_int (max),
                                                              API_SCM_TO_STRING(default_value),
                                                              API_SCM_TO_STRING(value),
                                                              scm_to_int (null_value_allowed),
                                                              &weechat_guile_api_config_option_check_value_cb,
                                                              API_SCM_TO_STRING(function_check_value),
                                                              API_SCM_TO_STRING(data_check_value),
                                                              &weechat_guile_api_config_option_change_cb,
                                                              API_SCM_TO_STRING(function_change),
                                                              API_SCM_TO_STRING(data_change),
                                                              &weechat_guile_api_config_option_delete_cb,
                                                              API_SCM_TO_STRING(function_delete),
                                                              API_SCM_TO_STRING(data_delete)));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_config_search_option (SCM config_file, SCM section,
                                        SCM option_name)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "config_search_option", API_RETURN_EMPTY);
    if (!scm_is_string (config_file) || !scm_is_string (section)
        || !scm_is_string (option_name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_config_search_option (API_STR2PTR(API_SCM_TO_STRING(config_file)),
                                                       API_STR2PTR(API_SCM_TO_STRING(section)),
                                                       API_SCM_TO_STRING(option_name)));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_config_string_to_boolean (SCM text)
{
    int value;

    API_INIT_FUNC(1, "config_string_to_boolean", API_RETURN_INT(0));
    if (!scm_is_string (text))
        API_WRONG_ARGS(API_RETURN_INT(0));

    value = weechat_config_string_to_boolean (API_SCM_TO_STRING(text));

    API_RETURN_INT(value);
}

SCM
weechat_guile_api_config_option_reset (SCM option, SCM run_callback)
{
    int rc;

    API_INIT_FUNC(1, "config_option_reset", API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));
    if (!scm_is_string (option) || !scm_is_integer (run_callback))
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));

    rc = weechat_config_option_reset (API_STR2PTR(API_SCM_TO_STRING(option)),
                                      scm_to_int (run_callback));

    API_RETURN_INT(rc);
}

SCM
weechat_guile_api_config_option_set (SCM option, SCM new_value,
                                     SCM run_callback)
{
    int rc;

    API_INIT_FUNC(1, "config_option_set", API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));
    if (!scm_is_string (option) || !scm_is_string (new_value)
        || !scm_is_integer (run_callback))
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));

    rc = weechat_config_option_set (API_STR2PTR(API_SCM_TO_STRING(option)),
                                    API_SCM_TO_STRING(new_value),
                                    scm_to_int (run_callback));

    API_RETURN_INT(rc);
}

SCM
weechat_guile_api_config_option_set_null (SCM option, SCM run_callback)
{
    int rc;

    API_INIT_FUNC(1, "config_option_set_null", API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));
    if (!scm_is_string (option) || !scm_is_integer (run_callback))
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));

    rc = weechat_config_option_set_null (API_STR2PTR(API_SCM_TO_STRING(option)),
                                         scm_to_int (run_callback));

    API_RETURN_INT(rc);
}

SCM
weechat_guile_api_config_option_unset (SCM option)
{
    int rc;

    API_INIT_FUNC(1, "config_option_unset", API_RETURN_INT(WEECHAT_CONFIG_OPTION_UNSET_ERROR));
    if (!scm_is_string (option))
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_OPTION_UNSET_ERROR));

    rc = weechat_config_option_unset (API_STR2PTR(API_SCM_TO_STRING(option)));

    API_RETURN_INT(rc);
}

SCM
weechat_guile_api_config_option_rename (SCM option, SCM new_name)
{
    API_INIT_FUNC(1, "config_option_rename", API_RETURN_ERROR);
    if (!scm_is_string (option) || !scm_is_string (new_name))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_config_option_rename (API_STR2PTR(API_SCM_TO_STRING(option)),
                                  API_SCM_TO_STRING(new_name));

    API_RETURN_OK;
}

SCM
weechat_guile_api_config_option_get_string (SCM option, SCM property)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "config_option_get_string", API_RETURN_EMPTY);
    if (!scm_is_string (option) || !scm_is_string (property))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_config_option_get_string (
        API_STR2PTR(API_SCM_TO_STRING(option)),
        API_SCM_TO_STRING(property));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_config_option_get_pointer (SCM option, SCM property)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "config_option_get_pointer", API_RETURN_EMPTY);
    if (!scm_is_string (option) || !scm_is_string (property))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(
        weechat_config_option_get_pointer (
            API_STR2PTR(API_SCM_TO_STRING(option)),
            API_SCM_TO_STRING(property)));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_config_option_is_null (SCM option)
{
    int value;

    API_INIT_FUNC(1, "config_option_is_null", API_RETURN_INT(1));
    if (!scm_is_string (option))
        API_WRONG_ARGS(API_RETURN_INT(1));

    value = weechat_config_option_is_null (API_STR2PTR(API_SCM_TO_STRING(option)));

    API_RETURN_INT(value);
}

SCM
weechat_guile_api_config_option_default_is_null (SCM option)
{
    int value;

    API_INIT_FUNC(1, "config_option_default_is_null", API_RETURN_INT(1));
    if (!scm_is_string (option))
        API_WRONG_ARGS(API_RETURN_INT(1));

    value = weechat_config_option_default_is_null (API_STR2PTR(API_SCM_TO_STRING(option)));

    API_RETURN_INT(value);
}

SCM
weechat_guile_api_config_boolean (SCM option)
{
    int value;

    API_INIT_FUNC(1, "config_boolean", API_RETURN_INT(0));
    if (!scm_is_string (option))
        API_WRONG_ARGS(API_RETURN_INT(0));

    value = weechat_config_boolean (API_STR2PTR(API_SCM_TO_STRING(option)));

    API_RETURN_INT(value);
}

SCM
weechat_guile_api_config_boolean_default (SCM option)
{
    int value;

    API_INIT_FUNC(1, "config_boolean_default", API_RETURN_INT(0));
    if (!scm_is_string (option))
        API_WRONG_ARGS(API_RETURN_INT(0));

    value = weechat_config_boolean_default (API_STR2PTR(API_SCM_TO_STRING(option)));

    API_RETURN_INT(value);
}

SCM
weechat_guile_api_config_boolean_inherited (SCM option)
{
    int value;

    API_INIT_FUNC(1, "config_boolean_inherited", API_RETURN_INT(0));
    if (!scm_is_string (option))
        API_WRONG_ARGS(API_RETURN_INT(0));

    value = weechat_config_boolean_inherited (API_STR2PTR(API_SCM_TO_STRING(option)));

    API_RETURN_INT(value);
}

SCM
weechat_guile_api_config_integer (SCM option)
{
    int value;

    API_INIT_FUNC(1, "config_integer", API_RETURN_INT(0));
    if (!scm_is_string (option))
        API_WRONG_ARGS(API_RETURN_INT(0));

    value = weechat_config_integer (API_STR2PTR(API_SCM_TO_STRING(option)));

    API_RETURN_INT(value);
}

SCM
weechat_guile_api_config_integer_default (SCM option)
{
    int value;

    API_INIT_FUNC(1, "config_integer_default", API_RETURN_INT(0));
    if (!scm_is_string (option))
        API_WRONG_ARGS(API_RETURN_INT(0));

    value = weechat_config_integer_default (API_STR2PTR(API_SCM_TO_STRING(option)));

    API_RETURN_INT(value);
}

SCM
weechat_guile_api_config_integer_inherited (SCM option)
{
    int value;

    API_INIT_FUNC(1, "config_integer_inherited", API_RETURN_INT(0));
    if (!scm_is_string (option))
        API_WRONG_ARGS(API_RETURN_INT(0));

    value = weechat_config_integer_inherited (API_STR2PTR(API_SCM_TO_STRING(option)));

    API_RETURN_INT(value);
}

SCM
weechat_guile_api_config_string (SCM option)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "config_string", API_RETURN_EMPTY);
    if (!scm_is_string (option))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_config_string (API_STR2PTR(API_SCM_TO_STRING(option)));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_config_string_default (SCM option)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "config_string_default", API_RETURN_EMPTY);
    if (!scm_is_string (option))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_config_string_default (API_STR2PTR(API_SCM_TO_STRING(option)));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_config_string_inherited (SCM option)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "config_string_inherited", API_RETURN_EMPTY);
    if (!scm_is_string (option))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_config_string_inherited (API_STR2PTR(API_SCM_TO_STRING(option)));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_config_color (SCM option)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "config_color", API_RETURN_EMPTY);
    if (!scm_is_string (option))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_config_color (API_STR2PTR(API_SCM_TO_STRING(option)));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_config_color_default (SCM option)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "config_color_default", API_RETURN_EMPTY);
    if (!scm_is_string (option))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_config_color_default (API_STR2PTR(API_SCM_TO_STRING(option)));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_config_color_inherited (SCM option)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "config_color_inherited", API_RETURN_EMPTY);
    if (!scm_is_string (option))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_config_color_inherited (API_STR2PTR(API_SCM_TO_STRING(option)));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_config_enum (SCM option)
{
    int value;

    API_INIT_FUNC(1, "config_enum", API_RETURN_INT(0));
    if (!scm_is_string (option))
        API_WRONG_ARGS(API_RETURN_INT(0));

    value = weechat_config_enum (API_STR2PTR(API_SCM_TO_STRING(option)));

    API_RETURN_INT(value);
}

SCM
weechat_guile_api_config_enum_default (SCM option)
{
    int value;

    API_INIT_FUNC(1, "config_enum_default", API_RETURN_INT(0));
    if (!scm_is_string (option))
        API_WRONG_ARGS(API_RETURN_INT(0));

    value = weechat_config_enum_default (API_STR2PTR(API_SCM_TO_STRING(option)));

    API_RETURN_INT(value);
}

SCM
weechat_guile_api_config_enum_inherited (SCM option)
{
    int value;

    API_INIT_FUNC(1, "config_enum_inherited", API_RETURN_INT(0));
    if (!scm_is_string (option))
        API_WRONG_ARGS(API_RETURN_INT(0));

    value = weechat_config_enum_inherited (API_STR2PTR(API_SCM_TO_STRING(option)));

    API_RETURN_INT(value);
}

SCM
weechat_guile_api_config_write_option (SCM config_file, SCM option)
{
    API_INIT_FUNC(1, "config_write_option", API_RETURN_ERROR);
    if (!scm_is_string (config_file) || !scm_is_string (option))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_config_write_option (API_STR2PTR(API_SCM_TO_STRING(config_file)),
                                 API_STR2PTR(API_SCM_TO_STRING(option)));

    API_RETURN_OK;
}

SCM
weechat_guile_api_config_write_line (SCM config_file,
                                    SCM option_name, SCM value)
{
    API_INIT_FUNC(1, "config_write_line", API_RETURN_ERROR);
    if (!scm_is_string (config_file) || !scm_is_string (option_name) || !scm_is_string (value))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_config_write_line (API_STR2PTR(API_SCM_TO_STRING(config_file)),
                               API_SCM_TO_STRING(option_name),
                               "%s",
                               API_SCM_TO_STRING(value));

    API_RETURN_OK;
}

SCM
weechat_guile_api_config_write (SCM config_file)
{
    int rc;

    API_INIT_FUNC(1, "config_write", API_RETURN_INT(WEECHAT_CONFIG_WRITE_ERROR));
    if (!scm_is_string (config_file))
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_WRITE_ERROR));

    rc = weechat_config_write (API_STR2PTR(API_SCM_TO_STRING(config_file)));

    API_RETURN_INT(rc);
}

SCM
weechat_guile_api_config_read (SCM config_file)
{
    int rc;

    API_INIT_FUNC(1, "config_read", API_RETURN_INT(WEECHAT_CONFIG_READ_FILE_NOT_FOUND));
    if (!scm_is_string (config_file))
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_READ_FILE_NOT_FOUND));

    rc = weechat_config_read (API_STR2PTR(API_SCM_TO_STRING(config_file)));

    API_RETURN_INT(rc);
}

SCM
weechat_guile_api_config_reload (SCM config_file)
{
    int rc;

    API_INIT_FUNC(1, "config_reload", API_RETURN_INT(WEECHAT_CONFIG_READ_FILE_NOT_FOUND));
    if (!scm_is_string (config_file))
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_READ_FILE_NOT_FOUND));

    rc = weechat_config_reload (API_STR2PTR(API_SCM_TO_STRING(config_file)));

    API_RETURN_INT(rc);
}

SCM
weechat_guile_api_config_option_free (SCM option)
{
    API_INIT_FUNC(1, "config_option_free", API_RETURN_ERROR);
    if (!scm_is_string (option))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_config_option_free (API_STR2PTR(API_SCM_TO_STRING(option)));

    API_RETURN_OK;
}

SCM
weechat_guile_api_config_section_free_options (SCM section)
{
    API_INIT_FUNC(1, "config_section_free_options", API_RETURN_ERROR);
    if (!scm_is_string (section))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_config_section_free_options (
        API_STR2PTR(API_SCM_TO_STRING(section)));

    API_RETURN_OK;
}

SCM
weechat_guile_api_config_section_free (SCM section)
{
    API_INIT_FUNC(1, "config_section_free", API_RETURN_ERROR);
    if (!scm_is_string (section))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_config_section_free (API_STR2PTR(API_SCM_TO_STRING(section)));

    API_RETURN_OK;
}

SCM
weechat_guile_api_config_free (SCM config_file)
{
    API_INIT_FUNC(1, "config_free", API_RETURN_ERROR);
    if (!scm_is_string (config_file))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_config_free (API_STR2PTR(API_SCM_TO_STRING(config_file)));

    API_RETURN_OK;
}

SCM
weechat_guile_api_config_get (SCM option)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "config_get", API_RETURN_EMPTY);
    if (!scm_is_string (option))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_config_get (API_SCM_TO_STRING(option)));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_config_get_plugin (SCM option)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "config_get_plugin", API_RETURN_EMPTY);
    if (!scm_is_string (option))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = plugin_script_api_config_get_plugin (weechat_guile_plugin,
                                                  guile_current_script,
                                                  API_SCM_TO_STRING(option));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_config_is_set_plugin (SCM option)
{
    int rc;

    API_INIT_FUNC(1, "config_is_set_plugin", API_RETURN_INT(0));
    if (!scm_is_string (option))
        API_WRONG_ARGS(API_RETURN_INT(0));

    rc = plugin_script_api_config_is_set_plugin (weechat_guile_plugin,
                                                 guile_current_script,
                                                 API_SCM_TO_STRING(option));

    API_RETURN_INT(rc);
}

SCM
weechat_guile_api_config_set_plugin (SCM option, SCM value)
{
    int rc;

    API_INIT_FUNC(1, "config_set_plugin", API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));
    if (!scm_is_string (option) || !scm_is_string (value))
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));

    rc = plugin_script_api_config_set_plugin (weechat_guile_plugin,
                                              guile_current_script,
                                              API_SCM_TO_STRING(option),
                                              API_SCM_TO_STRING(value));

    API_RETURN_INT(rc);
}

SCM
weechat_guile_api_config_set_desc_plugin (SCM option, SCM description)
{
    API_INIT_FUNC(1, "config_set_desc_plugin", API_RETURN_ERROR);
    if (!scm_is_string (option) || !scm_is_string (description))
        API_WRONG_ARGS(API_RETURN_ERROR);

    plugin_script_api_config_set_desc_plugin (weechat_guile_plugin,
                                              guile_current_script,
                                              API_SCM_TO_STRING(option),
                                              API_SCM_TO_STRING(description));

    API_RETURN_OK;
}

SCM
weechat_guile_api_config_unset_plugin (SCM option)
{
    int rc;

    API_INIT_FUNC(1, "config_unset_plugin", API_RETURN_INT(WEECHAT_CONFIG_OPTION_UNSET_ERROR));
    if (!scm_is_string (option))
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_OPTION_UNSET_ERROR));

    rc = plugin_script_api_config_unset_plugin (weechat_guile_plugin,
                                                guile_current_script,
                                                API_SCM_TO_STRING(option));

    API_RETURN_INT(rc);
}

SCM
weechat_guile_api_key_bind (SCM context, SCM keys)
{
    struct t_hashtable *c_keys;
    int num_keys;

    API_INIT_FUNC(1, "key_bind", API_RETURN_INT(0));
    if (!scm_is_string (context) || !scm_list_p (keys))
        API_WRONG_ARGS(API_RETURN_INT(0));

    c_keys = weechat_guile_alist_to_hashtable (keys,
                                               WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                               WEECHAT_HASHTABLE_STRING,
                                               WEECHAT_HASHTABLE_STRING);

    num_keys = weechat_key_bind (API_SCM_TO_STRING(context), c_keys);

    weechat_hashtable_free (c_keys);

    API_RETURN_INT(num_keys);
}

SCM
weechat_guile_api_key_unbind (SCM context, SCM key)
{
    int num_keys;

    API_INIT_FUNC(1, "key_unbind", API_RETURN_INT(0));
    if (!scm_is_string (context) || !scm_is_string (key))
        API_WRONG_ARGS(API_RETURN_INT(0));

    num_keys = weechat_key_unbind (API_SCM_TO_STRING(context),
                                   API_SCM_TO_STRING(key));

    API_RETURN_INT(num_keys);
}

SCM
weechat_guile_api_prefix (SCM prefix)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(0, "prefix", API_RETURN_EMPTY);
    if (!scm_is_string (prefix))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_prefix (API_SCM_TO_STRING(prefix));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_color (SCM color)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(0, "color", API_RETURN_EMPTY);
    if (!scm_is_string (color))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_color (API_SCM_TO_STRING(color));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_print (SCM buffer, SCM message)
{
    API_INIT_FUNC(0, "print", API_RETURN_ERROR);
    if (!scm_is_string (buffer) || !scm_is_string (message))
        API_WRONG_ARGS(API_RETURN_ERROR);

    plugin_script_api_printf (weechat_guile_plugin,
                              guile_current_script,
                              API_STR2PTR(API_SCM_TO_STRING(buffer)),
                              "%s", API_SCM_TO_STRING(message));

    API_RETURN_OK;
}

SCM
weechat_guile_api_print_date_tags (SCM buffer, SCM date, SCM tags, SCM message)
{
    API_INIT_FUNC(1, "print_date_tags", API_RETURN_ERROR);
    if (!scm_is_string (buffer) || !scm_is_integer (date)
        || !scm_is_string (tags) || !scm_is_string (message))
        API_WRONG_ARGS(API_RETURN_ERROR);

    plugin_script_api_printf_date_tags (weechat_guile_plugin,
                                        guile_current_script,
                                        API_STR2PTR(API_SCM_TO_STRING(buffer)),
                                        (time_t)scm_to_long (date),
                                        API_SCM_TO_STRING(tags),
                                        "%s", API_SCM_TO_STRING(message));

    API_RETURN_OK;
}

SCM
weechat_guile_api_print_datetime_tags (SCM buffer, SCM date, SCM date_usec,
                                       SCM tags, SCM message)
{
    API_INIT_FUNC(1, "print_datetime_tags", API_RETURN_ERROR);
    if (!scm_is_string (buffer) || !scm_is_integer (date)
        || !scm_is_integer (date_usec) || !scm_is_string (tags)
        || !scm_is_string (message))
        API_WRONG_ARGS(API_RETURN_ERROR);

    plugin_script_api_printf_datetime_tags (
        weechat_guile_plugin,
        guile_current_script,
        API_STR2PTR(API_SCM_TO_STRING(buffer)),
        (time_t)scm_to_long (date),
        scm_to_int (date_usec),
        API_SCM_TO_STRING(tags),
        "%s", API_SCM_TO_STRING(message));

    API_RETURN_OK;
}

SCM
weechat_guile_api_print_y (SCM buffer, SCM y, SCM message)
{
    API_INIT_FUNC(1, "print_y", API_RETURN_ERROR);
    if (!scm_is_string (buffer) || !scm_is_integer (y)
        || !scm_is_string (message))
        API_WRONG_ARGS(API_RETURN_ERROR);

    plugin_script_api_printf_y (weechat_guile_plugin,
                                guile_current_script,
                                API_STR2PTR(API_SCM_TO_STRING(buffer)),
                                scm_to_int (y),
                                "%s", API_SCM_TO_STRING(message));

    API_RETURN_OK;
}

SCM
weechat_guile_api_print_y_date_tags (SCM buffer, SCM y, SCM date, SCM tags,
                                     SCM message)
{
    API_INIT_FUNC(1, "print_y_date_tags", API_RETURN_ERROR);
    if (!scm_is_string (buffer) || !scm_is_integer (y)
        || !scm_is_integer (date) || !scm_is_string (tags)
        || !scm_is_string (message))
        API_WRONG_ARGS(API_RETURN_ERROR);

    plugin_script_api_printf_y_date_tags (weechat_guile_plugin,
                                          guile_current_script,
                                          API_STR2PTR(API_SCM_TO_STRING(buffer)),
                                          scm_to_int (y),
                                          (time_t)scm_to_long (date),
                                          API_SCM_TO_STRING(tags),
                                          "%s", API_SCM_TO_STRING(message));

    API_RETURN_OK;
}

SCM
weechat_guile_api_print_y_datetime_tags (SCM buffer, SCM y, SCM date,
                                         SCM date_usec, SCM tags, SCM message)
{
    API_INIT_FUNC(1, "print_y_datetime_tags", API_RETURN_ERROR);
    if (!scm_is_string (buffer) || !scm_is_integer (y)
        || !scm_is_integer (date) || !scm_is_integer (date_usec)
        || !scm_is_string (tags) || !scm_is_string (message))
        API_WRONG_ARGS(API_RETURN_ERROR);

    plugin_script_api_printf_y_datetime_tags (
        weechat_guile_plugin,
        guile_current_script,
        API_STR2PTR(API_SCM_TO_STRING(buffer)),
        scm_to_int (y),
        (time_t)scm_to_long (date),
        scm_to_int (date_usec),
        API_SCM_TO_STRING(tags),
        "%s", API_SCM_TO_STRING(message));

    API_RETURN_OK;
}

SCM
weechat_guile_api_log_print (SCM message)
{
    API_INIT_FUNC(1, "log_print", API_RETURN_ERROR);
    if (!scm_is_string (message))
        API_WRONG_ARGS(API_RETURN_ERROR);

    plugin_script_api_log_printf (weechat_guile_plugin,
                                  guile_current_script,
                                  "%s", API_SCM_TO_STRING(message));

    API_RETURN_OK;
}

int
weechat_guile_api_hook_command_cb (const void *pointer, void *data,
                                   struct t_gui_buffer *buffer,
                                   int argc, char **argv, char **argv_eol)
{
    struct t_plugin_script *script;
    void *func_argv[3];
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;
    int *rc, ret;

    /* make C compiler happy */
    (void) argv;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = (char *)API_PTR2STR(buffer);
        func_argv[2] = (argc > 1) ? argv_eol[1] : empty_arg;

        rc = (int *) weechat_guile_exec (script,
                                         WEECHAT_SCRIPT_EXEC_INT,
                                         ptr_function,
                                         "sss", func_argv);

        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }

        return ret;
    }

    return WEECHAT_RC_ERROR;
}

SCM
weechat_guile_api_hook_command (SCM command, SCM description, SCM args,
                                SCM args_description, SCM completion,
                                SCM function, SCM data)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "hook_command", API_RETURN_EMPTY);
    if (!scm_is_string (command) || !scm_is_string (description)
        || !scm_is_string (args) || !scm_is_string (args_description)
        || !scm_is_string (completion) || !scm_is_string (function)
        || !scm_is_string (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(plugin_script_api_hook_command (weechat_guile_plugin,
                                                         guile_current_script,
                                                         API_SCM_TO_STRING(command),
                                                         API_SCM_TO_STRING(description),
                                                         API_SCM_TO_STRING(args),
                                                         API_SCM_TO_STRING(args_description),
                                                         API_SCM_TO_STRING(completion),
                                                         &weechat_guile_api_hook_command_cb,
                                                         API_SCM_TO_STRING(function),
                                                         API_SCM_TO_STRING(data)));

    API_RETURN_STRING(result);
}

int
weechat_guile_api_hook_completion_cb (const void *pointer, void *data,
                                      const char *completion_item,
                                      struct t_gui_buffer *buffer,
                                      struct t_gui_completion *completion)
{
    struct t_plugin_script *script;
    void *func_argv[4];
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;
    int *rc, ret;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = (completion_item) ? (char *)completion_item : empty_arg;
        func_argv[2] = (char *)API_PTR2STR(buffer);
        func_argv[3] = (char *)API_PTR2STR(completion);

        rc = (int *) weechat_guile_exec (script,
                                         WEECHAT_SCRIPT_EXEC_INT,
                                         ptr_function,
                                         "ssss", func_argv);

        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }

        return ret;
    }

    return WEECHAT_RC_ERROR;
}

SCM
weechat_guile_api_hook_completion (SCM completion, SCM description,
                                   SCM function, SCM data)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "hook_completion", API_RETURN_EMPTY);
    if (!scm_is_string (completion) || !scm_is_string (description)
        || !scm_is_string (function) || !scm_is_string (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(plugin_script_api_hook_completion (weechat_guile_plugin,
                                                            guile_current_script,
                                                            API_SCM_TO_STRING(completion),
                                                            API_SCM_TO_STRING(description),
                                                            &weechat_guile_api_hook_completion_cb,
                                                            API_SCM_TO_STRING(function),
                                                            API_SCM_TO_STRING(data)));

    API_RETURN_STRING(result);
}

/*
 * This function deprecated since WeeChat 2.9, kept for compatibility.
 * It is replaced by completion_get_string.
 */

SCM
weechat_guile_api_hook_completion_get_string (SCM completion, SCM property)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "hook_completion_get_string", API_RETURN_EMPTY);
    if (!scm_is_string (completion) || !scm_is_string (property))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_hook_completion_get_string (
        API_STR2PTR(API_SCM_TO_STRING(completion)),
        API_SCM_TO_STRING(property));

    API_RETURN_STRING(result);
}

/*
 * This function deprecated since WeeChat 2.9, kept for compatibility.
 * It is replaced by completion_list_add.
 */

SCM
weechat_guile_api_hook_completion_list_add (SCM completion, SCM word,
                                            SCM nick_completion, SCM where)
{
    API_INIT_FUNC(1, "hook_completion_list_add", API_RETURN_ERROR);
    if (!scm_is_string (completion) || !scm_is_string (word)
        || !scm_is_integer (nick_completion) || !scm_is_string (where))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_hook_completion_list_add (API_STR2PTR(API_SCM_TO_STRING(completion)),
                                      API_SCM_TO_STRING(word),
                                      scm_to_int (nick_completion),
                                      API_SCM_TO_STRING(where));

    API_RETURN_OK;
}

int
weechat_guile_api_hook_command_run_cb (const void *pointer, void *data,
                                       struct t_gui_buffer *buffer,
                                       const char *command)
{
    struct t_plugin_script *script;
    void *func_argv[3];
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;
    int *rc, ret;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = (char *)API_PTR2STR(buffer);
        func_argv[2] = (command) ? (char *)command : empty_arg;

        rc = (int *) weechat_guile_exec (script,
                                         WEECHAT_SCRIPT_EXEC_INT,
                                         ptr_function,
                                         "sss", func_argv);

        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }

        return ret;
    }

    return WEECHAT_RC_ERROR;
}

SCM
weechat_guile_api_hook_command_run (SCM command, SCM function, SCM data)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "hook_command_run", API_RETURN_EMPTY);
    if (!scm_is_string (command) || !scm_is_string (function)
        || !scm_is_string (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(plugin_script_api_hook_command_run (weechat_guile_plugin,
                                                             guile_current_script,
                                                             API_SCM_TO_STRING(command),
                                                             &weechat_guile_api_hook_command_run_cb,
                                                             API_SCM_TO_STRING(function),
                                                             API_SCM_TO_STRING(data)));

    API_RETURN_STRING(result);
}

int
weechat_guile_api_hook_timer_cb (const void *pointer, void *data,
                                 int remaining_calls)
{
    struct t_plugin_script *script;
    void *func_argv[2];
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;
    int *rc, ret;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = &remaining_calls;

        rc = (int *) weechat_guile_exec (script,
                                         WEECHAT_SCRIPT_EXEC_INT,
                                         ptr_function,
                                         "si", func_argv);

        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }

        return ret;
    }

    return WEECHAT_RC_ERROR;
}

SCM
weechat_guile_api_hook_timer (SCM interval, SCM align_second, SCM max_calls,
                              SCM function, SCM data)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "hook_timer", API_RETURN_EMPTY);
    if (!scm_is_integer (interval) || !scm_is_integer (align_second)
        || !scm_is_integer (max_calls) || !scm_is_string (function)
        || !scm_is_string (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(plugin_script_api_hook_timer (weechat_guile_plugin,
                                                       guile_current_script,
                                                       scm_to_long (interval),
                                                       scm_to_int (align_second),
                                                       scm_to_int (max_calls),
                                                       &weechat_guile_api_hook_timer_cb,
                                                       API_SCM_TO_STRING(function),
                                                       API_SCM_TO_STRING(data)));

    API_RETURN_STRING(result);
}

int
weechat_guile_api_hook_fd_cb (const void *pointer, void *data, int fd)
{
    struct t_plugin_script *script;
    void *func_argv[2];
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;
    int *rc, ret;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = &fd;

        rc = (int *) weechat_guile_exec (script,
                                         WEECHAT_SCRIPT_EXEC_INT,
                                         ptr_function,
                                         "si", func_argv);

        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }

        return ret;
    }

    return WEECHAT_RC_ERROR;
}

SCM
weechat_guile_api_hook_fd (SCM fd, SCM read, SCM write, SCM exception,
                           SCM function, SCM data)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "hook_fd", API_RETURN_EMPTY);
    if (!scm_is_integer (fd) || !scm_is_integer (read)
        || !scm_is_integer (write) || !scm_is_integer (exception)
        || !scm_is_string (function) || !scm_is_string (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(plugin_script_api_hook_fd (weechat_guile_plugin,
                                                    guile_current_script,
                                                    scm_to_int (fd),
                                                    scm_to_int (read),
                                                    scm_to_int (write),
                                                    scm_to_int (exception),
                                                    &weechat_guile_api_hook_fd_cb,
                                                    API_SCM_TO_STRING(function),
                                                    API_SCM_TO_STRING(data)));

    API_RETURN_STRING(result);
}

int
weechat_guile_api_hook_process_cb (const void *pointer, void *data,
                                   const char *command, int return_code,
                                   const char *out, const char *err)
{
    struct t_plugin_script *script;
    void *func_argv[5];
    char empty_arg[1] = { '\0' }, *result;
    const char *ptr_function, *ptr_data;
    int *rc, ret;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (return_code == WEECHAT_HOOK_PROCESS_CHILD)
    {
        if (strncmp (command, "func:", 5) == 0)
        {
            func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;

            result = (char *) weechat_guile_exec (script,
                                                  WEECHAT_SCRIPT_EXEC_STRING,
                                                  command + 5,
                                                  "s", func_argv);
            if (result)
            {
                printf ("%s", result);
                free (result);
                return 0;
            }
        }
        return 1;
    }
    else if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = (command) ? (char *)command : empty_arg;
        func_argv[2] = &return_code;
        func_argv[3] = (out) ? (char *)out : empty_arg;
        func_argv[4] = (err) ? (char *)err : empty_arg;

        rc = (int *) weechat_guile_exec (script,
                                         WEECHAT_SCRIPT_EXEC_INT,
                                         ptr_function,
                                         "ssiss", func_argv);

        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }

        return ret;
    }

    return WEECHAT_RC_ERROR;
}

SCM
weechat_guile_api_hook_process (SCM command, SCM timeout, SCM function,
                                SCM data)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "hook_process", API_RETURN_EMPTY);
    if (!scm_is_string (command) || !scm_is_integer (timeout)
        || !scm_is_string (function) || !scm_is_string (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(plugin_script_api_hook_process (weechat_guile_plugin,
                                                         guile_current_script,
                                                         API_SCM_TO_STRING(command),
                                                         scm_to_int (timeout),
                                                         &weechat_guile_api_hook_process_cb,
                                                         API_SCM_TO_STRING(function),
                                                         API_SCM_TO_STRING(data)));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_hook_process_hashtable (SCM command, SCM options, SCM timeout,
                                          SCM function, SCM data)
{
    const char *result;
    SCM return_value;
    struct t_hashtable *c_options;

    API_INIT_FUNC(1, "hook_process_hashtable", API_RETURN_EMPTY);
    if (!scm_is_string (command) || !scm_list_p (options)
        || !scm_is_integer (timeout) || !scm_is_string (function)
        || !scm_is_string (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    c_options = weechat_guile_alist_to_hashtable (options,
                                                  WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                                  WEECHAT_HASHTABLE_STRING,
                                                  WEECHAT_HASHTABLE_STRING);

    result = API_PTR2STR(plugin_script_api_hook_process_hashtable (weechat_guile_plugin,
                                                                   guile_current_script,
                                                                   API_SCM_TO_STRING(command),
                                                                   c_options,
                                                                   scm_to_int (timeout),
                                                                   &weechat_guile_api_hook_process_cb,
                                                                   API_SCM_TO_STRING(function),
                                                                   API_SCM_TO_STRING(data)));

    weechat_hashtable_free (c_options);

    API_RETURN_STRING(result);
}

int
weechat_guile_api_hook_url_cb (const void *pointer, void *data,
                               const char *url,
                               struct t_hashtable *options,
                               struct t_hashtable *output)
{
    struct t_plugin_script *script;
    void *func_argv[4];
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;
    int *rc, ret;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = (url) ? (char *)url : empty_arg;
        func_argv[2] = options;
        func_argv[3] = output;

        rc = (int *) weechat_guile_exec (script,
                                         WEECHAT_SCRIPT_EXEC_INT,
                                         ptr_function,
                                         "sshh", func_argv);

        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }

        return ret;
    }

    return WEECHAT_RC_ERROR;
}

SCM
weechat_guile_api_hook_url (SCM url, SCM options, SCM timeout,
                            SCM function, SCM data)
{
    const char *result;
    SCM return_value;
    struct t_hashtable *c_options;

    API_INIT_FUNC(1, "hook_url", API_RETURN_EMPTY);
    if (!scm_is_string (url) || !scm_list_p (options)
        || !scm_is_integer (timeout) || !scm_is_string (function)
        || !scm_is_string (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    c_options = weechat_guile_alist_to_hashtable (options,
                                                  WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                                  WEECHAT_HASHTABLE_STRING,
                                                  WEECHAT_HASHTABLE_STRING);

    result = API_PTR2STR(plugin_script_api_hook_url (weechat_guile_plugin,
                                                     guile_current_script,
                                                     API_SCM_TO_STRING(url),
                                                     c_options,
                                                     scm_to_int (timeout),
                                                     &weechat_guile_api_hook_url_cb,
                                                     API_SCM_TO_STRING(function),
                                                     API_SCM_TO_STRING(data)));

    weechat_hashtable_free (c_options);

    API_RETURN_STRING(result);
}

int
weechat_guile_api_hook_connect_cb (const void *pointer, void *data,
                                   int status, int gnutls_rc,
                                   int sock, const char *error,
                                   const char *ip_address)
{
    struct t_plugin_script *script;
    void *func_argv[6];
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;
    int *rc, ret;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = &status;
        func_argv[2] = &gnutls_rc;
        func_argv[3] = &sock;
        func_argv[4] = (ip_address) ? (char *)ip_address : empty_arg;
        func_argv[5] = (error) ? (char *)error : empty_arg;

        rc = (int *) weechat_guile_exec (script,
                                         WEECHAT_SCRIPT_EXEC_INT,
                                         ptr_function,
                                         "siiiss", func_argv);

        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }

        return ret;
    }

    return WEECHAT_RC_ERROR;
}

SCM
weechat_guile_api_hook_connect (SCM proxy, SCM address, SCM port, SCM ipv6,
                                SCM retry, SCM local_hostname, SCM function,
                                SCM data)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "hook_connect", API_RETURN_EMPTY);
    if (!scm_is_string (proxy) || !scm_is_string (address)
        || !scm_is_integer (port) || !scm_is_integer (ipv6)
        || !scm_is_integer (retry) || !scm_is_string (local_hostname)
        || !scm_is_string (function) || !scm_is_string (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(plugin_script_api_hook_connect (weechat_guile_plugin,
                                                         guile_current_script,
                                                         API_SCM_TO_STRING(proxy),
                                                         API_SCM_TO_STRING(address),
                                                         scm_to_int (port),
                                                         scm_to_int (ipv6),
                                                         scm_to_int (retry),
                                                         NULL, /* gnutls session */
                                                         NULL, /* gnutls callback */
                                                         0,    /* gnutls DH key size */
                                                         NULL, /* gnutls priorities */
                                                         API_SCM_TO_STRING(local_hostname),
                                                         &weechat_guile_api_hook_connect_cb,
                                                         API_SCM_TO_STRING(function),
                                                         API_SCM_TO_STRING(data)));

    API_RETURN_STRING(result);
}

struct t_hashtable *
weechat_guile_api_hook_line_cb (const void *pointer, void *data,
                                struct t_hashtable *line)
{
    struct t_plugin_script *script;
    void *func_argv[2];
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = line;

        return (struct t_hashtable *)weechat_guile_exec (
            script,
            WEECHAT_SCRIPT_EXEC_HASHTABLE,
            ptr_function,
            "sh", func_argv);
    }

    return NULL;
}

SCM
weechat_guile_api_hook_line (SCM buffer_type, SCM buffer_name, SCM tags,
                             SCM function, SCM data)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "hook_line", API_RETURN_EMPTY);
    if (!scm_is_string (buffer_type) || !scm_is_string (buffer_name)
        || !scm_is_string (tags) || !scm_is_string (function)
        || !scm_is_string (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(plugin_script_api_hook_line (weechat_guile_plugin,
                                                      guile_current_script,
                                                      API_SCM_TO_STRING(buffer_type),
                                                      API_SCM_TO_STRING(buffer_name),
                                                      API_SCM_TO_STRING(tags),
                                                      &weechat_guile_api_hook_line_cb,
                                                      API_SCM_TO_STRING(function),
                                                      API_SCM_TO_STRING(data)));

    API_RETURN_STRING(result);
}

int
weechat_guile_api_hook_print_cb (const void *pointer, void *data,
                                 struct t_gui_buffer *buffer,
                                 time_t date, int date_usec,
                                 int tags_count, const char **tags,
                                 int displayed, int highlight,
                                 const char *prefix, const char *message)
{
    struct t_plugin_script *script;
    void *func_argv[8];
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;
    static char timebuffer[64];
    int *rc, ret;

    /* make C compiler happy */
    (void) date_usec;
    (void) tags_count;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        snprintf (timebuffer, sizeof (timebuffer), "%lld", (long long)date);

        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = (char *)API_PTR2STR(buffer);
        func_argv[2] = timebuffer;
        func_argv[3] = weechat_string_rebuild_split_string (tags, ",", 0, -1);
        if (!func_argv[3])
            func_argv[3] = strdup ("");
        func_argv[4] = &displayed;
        func_argv[5] = &highlight;
        func_argv[6] = (prefix) ? (char *)prefix : empty_arg;
        func_argv[7] = (message) ? (char *)message : empty_arg;

        rc = (int *) weechat_guile_exec (script,
                                         WEECHAT_SCRIPT_EXEC_INT,
                                         ptr_function,
                                         "ssssiiss", func_argv);

        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        free (func_argv[3]);

        return ret;
    }

    return WEECHAT_RC_ERROR;
}

SCM
weechat_guile_api_hook_print (SCM buffer, SCM tags, SCM message,
                              SCM strip_colors, SCM function, SCM data)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "hook_print", API_RETURN_EMPTY);
    if (!scm_is_string (buffer) || !scm_is_string (tags)
        || !scm_is_string (message) || !scm_is_integer (strip_colors)
        || !scm_is_string (function) || !scm_is_string (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(plugin_script_api_hook_print (weechat_guile_plugin,
                                                       guile_current_script,
                                                       API_STR2PTR(API_SCM_TO_STRING(buffer)),
                                                       API_SCM_TO_STRING(tags),
                                                       API_SCM_TO_STRING(message),
                                                       scm_to_int (strip_colors),
                                                       &weechat_guile_api_hook_print_cb,
                                                       API_SCM_TO_STRING(function),
                                                       API_SCM_TO_STRING(data)));

    API_RETURN_STRING(result);
}

int
weechat_guile_api_hook_signal_cb (const void *pointer, void *data,
                                  const char *signal,
                                 const char *type_data, void *signal_data)
{
    struct t_plugin_script *script;
    void *func_argv[3];
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;
    static char str_value[64];
    int *rc, ret;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = (signal) ? (char *)signal : empty_arg;
        if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_STRING) == 0)
        {
            func_argv[2] = (signal_data) ? (char *)signal_data : empty_arg;
        }
        else if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_INT) == 0)
        {
            str_value[0] = '\0';
            if (signal_data)
            {
                snprintf (str_value, sizeof (str_value),
                          "%d", *((int *)signal_data));
            }
            func_argv[2] = str_value;
        }
        else if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_POINTER) == 0)
        {
            func_argv[2] = (char *)API_PTR2STR(signal_data);
        }
        else
            func_argv[2] = empty_arg;

        rc = (int *) weechat_guile_exec (script,
                                         WEECHAT_SCRIPT_EXEC_INT,
                                         ptr_function,
                                         "sss", func_argv);

        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }

        return ret;
    }

    return WEECHAT_RC_ERROR;
}

SCM
weechat_guile_api_hook_signal (SCM signal, SCM function, SCM data)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "hook_signal", API_RETURN_EMPTY);
    if (!scm_is_string (signal) || !scm_is_string (function) || !scm_is_string (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(plugin_script_api_hook_signal (weechat_guile_plugin,
                                                        guile_current_script,
                                                        API_SCM_TO_STRING(signal),
                                                        &weechat_guile_api_hook_signal_cb,
                                                        API_SCM_TO_STRING(function),
                                                        API_SCM_TO_STRING(data)));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_hook_signal_send (SCM signal, SCM type_data,
                                    SCM signal_data)
{
    int number, rc;

    API_INIT_FUNC(1, "hook_signal_send", API_RETURN_INT(WEECHAT_RC_ERROR));
    if (!scm_is_string (signal) || !scm_is_string (type_data))
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_RC_ERROR));

    if (strcmp (API_SCM_TO_STRING(type_data), WEECHAT_HOOK_SIGNAL_STRING) == 0)
    {
        if (!scm_is_string (signal_data))
            API_WRONG_ARGS(API_RETURN_INT(WEECHAT_RC_ERROR));
        rc = weechat_hook_signal_send (API_SCM_TO_STRING(signal),
                                       API_SCM_TO_STRING(type_data),
                                       (void *)API_SCM_TO_STRING(signal_data));
        API_RETURN_INT(rc);
    }
    else if (strcmp (API_SCM_TO_STRING(type_data), WEECHAT_HOOK_SIGNAL_INT) == 0)
    {
        if (!scm_is_integer (signal_data))
            API_WRONG_ARGS(API_RETURN_INT(WEECHAT_RC_ERROR));
        number = scm_to_int (signal_data);
        rc = weechat_hook_signal_send (API_SCM_TO_STRING(signal),
                                       API_SCM_TO_STRING(type_data),
                                       &number);
        API_RETURN_INT(rc);
    }
    else if (strcmp (API_SCM_TO_STRING(type_data), WEECHAT_HOOK_SIGNAL_POINTER) == 0)
    {
        if (!scm_is_string (signal_data))
            API_WRONG_ARGS(API_RETURN_INT(WEECHAT_RC_ERROR));
        rc = weechat_hook_signal_send (API_SCM_TO_STRING(signal),
                                       API_SCM_TO_STRING(type_data),
                                       API_STR2PTR(API_SCM_TO_STRING(signal_data)));
        API_RETURN_INT(rc);
    }

    API_RETURN_INT(WEECHAT_RC_ERROR);
}

int
weechat_guile_api_hook_hsignal_cb (const void *pointer, void *data,
                                   const char *signal,
                                   struct t_hashtable *hashtable)
{
    struct t_plugin_script *script;
    void *func_argv[3];
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;
    int *rc, ret;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = (signal) ? (char *)signal : empty_arg;
        func_argv[2] = hashtable;

        rc = (int *) weechat_guile_exec (script,
                                         WEECHAT_SCRIPT_EXEC_INT,
                                         ptr_function,
                                         "ssh", func_argv);

        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }

        return ret;
    }

    return WEECHAT_RC_ERROR;
}

SCM
weechat_guile_api_hook_hsignal (SCM signal, SCM function, SCM data)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "hook_hsignal", API_RETURN_EMPTY);
    if (!scm_is_string (signal) || !scm_is_string (function)
        || !scm_is_string (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(plugin_script_api_hook_hsignal (weechat_guile_plugin,
                                                         guile_current_script,
                                                         API_SCM_TO_STRING(signal),
                                                         &weechat_guile_api_hook_hsignal_cb,
                                                         API_SCM_TO_STRING(function),
                                                         API_SCM_TO_STRING(data)));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_hook_hsignal_send (SCM signal, SCM hashtable)
{
    struct t_hashtable *c_hashtable;
    int rc;

    API_INIT_FUNC(1, "hook_hsignal_send", API_RETURN_INT(WEECHAT_RC_ERROR));
    if (!scm_is_string (signal) || !scm_list_p (hashtable))
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_RC_ERROR));

    c_hashtable = weechat_guile_alist_to_hashtable (hashtable,
                                                    WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                                    WEECHAT_HASHTABLE_STRING,
                                                    WEECHAT_HASHTABLE_STRING);

    rc = weechat_hook_hsignal_send (API_SCM_TO_STRING(signal), c_hashtable);

    weechat_hashtable_free (c_hashtable);

    API_RETURN_INT(rc);
}

int
weechat_guile_api_hook_config_cb (const void *pointer, void *data,
                                  const char *option, const char *value)
{
    struct t_plugin_script *script;
    void *func_argv[3];
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;
    int *rc, ret;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = (option) ? (char *)option : empty_arg;
        func_argv[2] = (value) ? (char *)value : empty_arg;

        rc = (int *) weechat_guile_exec (script,
                                         WEECHAT_SCRIPT_EXEC_INT,
                                         ptr_function,
                                         "sss", func_argv);

        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }

        return ret;
    }

    return WEECHAT_RC_ERROR;
}

SCM
weechat_guile_api_hook_config (SCM option, SCM function, SCM data)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "hook_config", API_RETURN_EMPTY);
    if (!scm_is_string (option) || !scm_is_string (function)
        || !scm_is_string (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(plugin_script_api_hook_config (weechat_guile_plugin,
                                                        guile_current_script,
                                                        API_SCM_TO_STRING(option),
                                                        &weechat_guile_api_hook_config_cb,
                                                        API_SCM_TO_STRING(function),
                                                        API_SCM_TO_STRING(data)));

    API_RETURN_STRING(result);
}

char *
weechat_guile_api_hook_modifier_cb (const void *pointer, void *data,
                                    const char *modifier,
                                    const char *modifier_data,
                                    const char *string)
{
    struct t_plugin_script *script;
    void *func_argv[4];
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = (modifier) ? (char *)modifier : empty_arg;
        func_argv[2] = (modifier_data) ? (char *)modifier_data : empty_arg;
        func_argv[3] = (string) ? (char *)string : empty_arg;

        return (char *)weechat_guile_exec (script,
                                           WEECHAT_SCRIPT_EXEC_STRING,
                                           ptr_function,
                                           "ssss", func_argv);
    }

    return NULL;
}

SCM
weechat_guile_api_hook_modifier (SCM modifier, SCM function, SCM data)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "hook_modifier", API_RETURN_EMPTY);
    if (!scm_is_string (modifier) || !scm_is_string (function)
        || !scm_is_string (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(plugin_script_api_hook_modifier (weechat_guile_plugin,
                                                          guile_current_script,
                                                          API_SCM_TO_STRING(modifier),
                                                          &weechat_guile_api_hook_modifier_cb,
                                                          API_SCM_TO_STRING(function),
                                                          API_SCM_TO_STRING(data)));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_hook_modifier_exec (SCM modifier, SCM modifier_data,
                                      SCM string)
{
    char *result;
    SCM return_value;

    API_INIT_FUNC(1, "hook_modifier_exec", API_RETURN_EMPTY);
    if (!scm_is_string (modifier) || !scm_is_string (modifier_data)
        || !scm_is_string (string))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_hook_modifier_exec (API_SCM_TO_STRING(modifier),
                                         API_SCM_TO_STRING(modifier_data),
                                         API_SCM_TO_STRING(string));

    API_RETURN_STRING_FREE(result);
}

char *
weechat_guile_api_hook_info_cb (const void *pointer, void *data,
                                const char *info_name,
                                const char *arguments)
{
    struct t_plugin_script *script;
    void *func_argv[3];
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = (info_name) ? (char *)info_name : empty_arg;
        func_argv[2] = (arguments) ? (char *)arguments : empty_arg;

        return (char *)weechat_guile_exec (script,
                                           WEECHAT_SCRIPT_EXEC_STRING,
                                           ptr_function,
                                           "sss", func_argv);
    }

    return NULL;
}

SCM
weechat_guile_api_hook_info (SCM info_name, SCM description,
                             SCM args_description, SCM function, SCM data)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "hook_info", API_RETURN_EMPTY);
    if (!scm_is_string (info_name) || !scm_is_string (description)
        || !scm_is_string (args_description) || !scm_is_string (function)
        || !scm_is_string (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(plugin_script_api_hook_info (weechat_guile_plugin,
                                                      guile_current_script,
                                                      API_SCM_TO_STRING(info_name),
                                                      API_SCM_TO_STRING(description),
                                                      API_SCM_TO_STRING(args_description),
                                                      &weechat_guile_api_hook_info_cb,
                                                      API_SCM_TO_STRING(function),
                                                      API_SCM_TO_STRING(data)));

    API_RETURN_STRING(result);
}

struct t_hashtable *
weechat_guile_api_hook_info_hashtable_cb (const void *pointer, void *data,
                                          const char *info_name,
                                          struct t_hashtable *hashtable)
{
    struct t_plugin_script *script;
    void *func_argv[3];
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = (info_name) ? (char *)info_name : empty_arg;
        func_argv[2] = hashtable;

        return (struct t_hashtable *)weechat_guile_exec (
            script,
            WEECHAT_SCRIPT_EXEC_HASHTABLE,
            ptr_function,
            "ssh", func_argv);
    }

    return NULL;
}

SCM
weechat_guile_api_hook_info_hashtable (SCM info_name, SCM description,
                                       SCM args_description,
                                       SCM output_description, SCM function,
                                       SCM data)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "hook_info_hashtable", API_RETURN_EMPTY);
    if (!scm_is_string (info_name) || !scm_is_string (description)
        || !scm_is_string (args_description) || !scm_is_string (output_description)
        || !scm_is_string (function) || !scm_is_string (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(plugin_script_api_hook_info_hashtable (weechat_guile_plugin,
                                                                guile_current_script,
                                                                API_SCM_TO_STRING(info_name),
                                                                API_SCM_TO_STRING(description),
                                                                API_SCM_TO_STRING(args_description),
                                                                API_SCM_TO_STRING(output_description),
                                                                &weechat_guile_api_hook_info_hashtable_cb,
                                                                API_SCM_TO_STRING(function),
                                                                API_SCM_TO_STRING(data)));

    API_RETURN_STRING(result);
}

struct t_infolist *
weechat_guile_api_hook_infolist_cb (const void *pointer, void *data,
                                    const char *infolist_name,
                                    void *obj_pointer, const char *arguments)
{
    struct t_plugin_script *script;
    void *func_argv[4];
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;
    struct t_infolist *result;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = (infolist_name) ? (char *)infolist_name : empty_arg;
        func_argv[2] = (char *)API_PTR2STR(obj_pointer);
        func_argv[3] = (arguments) ? (char *)arguments : empty_arg;

        result = (struct t_infolist *)weechat_guile_exec (
            script,
            WEECHAT_SCRIPT_EXEC_POINTER,
            ptr_function,
            "ssss", func_argv);

        return result;
    }

    return NULL;
}

SCM
weechat_guile_api_hook_infolist (SCM infolist_name, SCM description,
                                 SCM pointer_description, SCM args_description,
                                 SCM function, SCM data)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "hook_infolist", API_RETURN_EMPTY);
    if (!scm_is_string (infolist_name) || !scm_is_string (description)
        || !scm_is_string (pointer_description) || !scm_is_string (args_description)
        || !scm_is_string (function) || !scm_is_string (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(plugin_script_api_hook_infolist (weechat_guile_plugin,
                                                          guile_current_script,
                                                          API_SCM_TO_STRING(infolist_name),
                                                          API_SCM_TO_STRING(description),
                                                          API_SCM_TO_STRING(pointer_description),
                                                          API_SCM_TO_STRING(args_description),
                                                          &weechat_guile_api_hook_infolist_cb,
                                                          API_SCM_TO_STRING(function),
                                                          API_SCM_TO_STRING(data)));

    API_RETURN_STRING(result);
}

struct t_hashtable *
weechat_guile_api_hook_focus_cb (const void *pointer, void *data,
                                 struct t_hashtable *info)
{
    struct t_plugin_script *script;
    void *func_argv[2];
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = info;

        return (struct t_hashtable *)weechat_guile_exec (
            script,
            WEECHAT_SCRIPT_EXEC_HASHTABLE,
            ptr_function,
            "sh", func_argv);
    }

    return NULL;
}

SCM
weechat_guile_api_hook_focus (SCM area, SCM function, SCM data)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "hook_focus", API_RETURN_EMPTY);
    if (!scm_is_string (area) || !scm_is_string (function)
        || !scm_is_string (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(plugin_script_api_hook_focus (weechat_guile_plugin,
                                                       guile_current_script,
                                                       API_SCM_TO_STRING(area),
                                                       &weechat_guile_api_hook_focus_cb,
                                                       API_SCM_TO_STRING(function),
                                                       API_SCM_TO_STRING(data)));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_hook_set (SCM hook, SCM property, SCM value)
{
    API_INIT_FUNC(1, "hook_set", API_RETURN_ERROR);
    if (!scm_is_string (hook) || !scm_is_string (property)
        || !scm_is_string (value))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_hook_set (API_STR2PTR(API_SCM_TO_STRING(hook)),
                      API_SCM_TO_STRING(property),
                      API_SCM_TO_STRING(value));

    API_RETURN_OK;
}

SCM
weechat_guile_api_unhook (SCM hook)
{
    API_INIT_FUNC(1, "unhook", API_RETURN_ERROR);
    if (!scm_is_string (hook))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_unhook (API_STR2PTR(API_SCM_TO_STRING(hook)));

    API_RETURN_OK;
}

SCM
weechat_guile_api_unhook_all ()
{
    API_INIT_FUNC(1, "unhook_all", API_RETURN_ERROR);

    weechat_unhook_all (guile_current_script->name);

    API_RETURN_OK;
}

int
weechat_guile_api_buffer_input_data_cb (const void *pointer, void *data,
                                        struct t_gui_buffer *buffer,
                                        const char *input_data)
{
    struct t_plugin_script *script;
    void *func_argv[3];
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;
    int *rc, ret;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = (char *)API_PTR2STR(buffer);
        func_argv[2] = (input_data) ? (char *)input_data : empty_arg;

        rc = (int *) weechat_guile_exec (script,
                                         WEECHAT_SCRIPT_EXEC_INT,
                                         ptr_function,
                                         "sss", func_argv);

        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }

        return ret;
    }

    return WEECHAT_RC_ERROR;
}

int
weechat_guile_api_buffer_close_cb (const void *pointer, void *data,
                                   struct t_gui_buffer *buffer)
{
    struct t_plugin_script *script;
    void *func_argv[2];
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;
    int *rc, ret;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = (char *)API_PTR2STR(buffer);

        rc = (int *) weechat_guile_exec (script,
                                         WEECHAT_SCRIPT_EXEC_INT,
                                         ptr_function,
                                         "ss", func_argv);

        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }

        return ret;
    }

    return WEECHAT_RC_ERROR;
}

SCM
weechat_guile_api_buffer_new (SCM name, SCM function_input, SCM data_input,
                              SCM function_close, SCM data_close)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "buffer_new", API_RETURN_EMPTY);
    if (!scm_is_string (name) || !scm_is_string (function_input)
        || !scm_is_string (data_input) || !scm_is_string (function_close)
        || !scm_is_string (data_close))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(plugin_script_api_buffer_new (weechat_guile_plugin,
                                                       guile_current_script,
                                                       API_SCM_TO_STRING(name),
                                                       &weechat_guile_api_buffer_input_data_cb,
                                                       API_SCM_TO_STRING(function_input),
                                                       API_SCM_TO_STRING(data_input),
                                                       &weechat_guile_api_buffer_close_cb,
                                                       API_SCM_TO_STRING(function_close),
                                                       API_SCM_TO_STRING(data_close)));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_buffer_new_props (SCM name, SCM properties,
                                    SCM function_input, SCM data_input,
                                    SCM function_close, SCM data_close)
{
    struct t_hashtable *c_properties;
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "buffer_new_props", API_RETURN_EMPTY);
    if (!scm_is_string (name) || !scm_list_p (properties)
        || !scm_is_string (function_input) || !scm_is_string (data_input)
        || !scm_is_string (function_close) || !scm_is_string (data_close))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    c_properties = weechat_guile_alist_to_hashtable (properties,
                                                     WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                                     WEECHAT_HASHTABLE_STRING,
                                                     WEECHAT_HASHTABLE_STRING);

    result = API_PTR2STR(
        plugin_script_api_buffer_new_props (
            weechat_guile_plugin,
            guile_current_script,
            API_SCM_TO_STRING(name),
            c_properties,
            &weechat_guile_api_buffer_input_data_cb,
            API_SCM_TO_STRING(function_input),
            API_SCM_TO_STRING(data_input),
            &weechat_guile_api_buffer_close_cb,
            API_SCM_TO_STRING(function_close),
            API_SCM_TO_STRING(data_close)));

    weechat_hashtable_free (c_properties);

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_buffer_search (SCM plugin, SCM name)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "buffer_search", API_RETURN_EMPTY);
    if (!scm_is_string (plugin) || !scm_is_string (name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_buffer_search (API_SCM_TO_STRING(plugin),
                                                API_SCM_TO_STRING(name)));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_buffer_search_main ()
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "buffer_search_main", API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_buffer_search_main ());

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_current_buffer ()
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "current_buffer", API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_current_buffer ());

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_buffer_clear (SCM buffer)
{
    API_INIT_FUNC(1, "buffer_clear", API_RETURN_ERROR);
    if (!scm_is_string (buffer))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_buffer_clear (API_STR2PTR(API_SCM_TO_STRING(buffer)));

    API_RETURN_OK;
}

SCM
weechat_guile_api_buffer_close (SCM buffer)
{
    API_INIT_FUNC(1, "buffer_close", API_RETURN_ERROR);
    if (!scm_is_string (buffer))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_buffer_close (API_STR2PTR(API_SCM_TO_STRING(buffer)));

    API_RETURN_OK;
}

SCM
weechat_guile_api_buffer_merge (SCM buffer, SCM target_buffer)
{
    API_INIT_FUNC(1, "buffer_merge", API_RETURN_ERROR);
    if (!scm_is_string (buffer) || !scm_is_string (target_buffer))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_buffer_merge (API_STR2PTR(API_SCM_TO_STRING(buffer)),
                          API_STR2PTR(API_SCM_TO_STRING(target_buffer)));

    API_RETURN_OK;
}

SCM
weechat_guile_api_buffer_unmerge (SCM buffer, SCM number)
{
    API_INIT_FUNC(1, "buffer_unmerge", API_RETURN_ERROR);
    if (!scm_is_string (buffer) || !scm_is_integer (number))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_buffer_unmerge (API_STR2PTR(API_SCM_TO_STRING(buffer)),
                            scm_to_int (number));

    API_RETURN_OK;
}

SCM
weechat_guile_api_buffer_get_integer (SCM buffer, SCM property)
{
    int value;

    API_INIT_FUNC(1, "buffer_get_integer", API_RETURN_INT(-1));
    if (!scm_is_string (buffer) || !scm_is_string (property))
        API_WRONG_ARGS(API_RETURN_INT(-1));

    value = weechat_buffer_get_integer (API_STR2PTR(API_SCM_TO_STRING(buffer)),
                                        API_SCM_TO_STRING(property));

    API_RETURN_INT(value);
}

SCM
weechat_guile_api_buffer_get_string (SCM buffer, SCM property)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "buffer_get_string", API_RETURN_EMPTY);
    if (!scm_is_string (buffer) || !scm_is_string (property))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_buffer_get_string (API_STR2PTR(API_SCM_TO_STRING(buffer)),
                                        API_SCM_TO_STRING(property));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_buffer_get_pointer (SCM buffer, SCM property)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "buffer_get_pointer", API_RETURN_EMPTY);
    if (!scm_is_string (buffer) || !scm_is_string (property))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_buffer_get_pointer (API_STR2PTR(API_SCM_TO_STRING(buffer)),
                                                     API_SCM_TO_STRING(property)));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_buffer_set (SCM buffer, SCM property, SCM value)
{
    API_INIT_FUNC(1, "buffer_set", API_RETURN_ERROR);
    if (!scm_is_string (buffer) || !scm_is_string (property)
        || !scm_is_string (value))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_buffer_set (API_STR2PTR(API_SCM_TO_STRING(buffer)),
                        API_SCM_TO_STRING(property),
                        API_SCM_TO_STRING(value));

    API_RETURN_OK;
}

SCM
weechat_guile_api_buffer_string_replace_local_var (SCM buffer, SCM string)
{
    char *result;
    SCM return_value;

    API_INIT_FUNC(1, "buffer_string_replace_local_var", API_RETURN_EMPTY);
    if (!scm_is_string (buffer) || !scm_is_string (string))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_buffer_string_replace_local_var (API_STR2PTR(API_SCM_TO_STRING(buffer)),
                                                      API_SCM_TO_STRING(string));

    API_RETURN_STRING_FREE(result);
}

SCM
weechat_guile_api_buffer_match_list (SCM buffer, SCM string)
{
    int value;

    API_INIT_FUNC(1, "buffer_match_list", API_RETURN_INT(0));
    if (!scm_is_string (buffer) || !scm_is_string (string))
        API_WRONG_ARGS(API_RETURN_INT(0));

    value = weechat_buffer_match_list (API_STR2PTR(API_SCM_TO_STRING(buffer)),
                                       API_SCM_TO_STRING(string));

    API_RETURN_INT(value);
}

SCM
weechat_guile_api_current_window ()
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "current_window", API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_current_window ());

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_window_search_with_buffer (SCM buffer)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "window_search_with_buffer", API_RETURN_EMPTY);
    if (!scm_is_string (buffer))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_window_search_with_buffer (API_STR2PTR(API_SCM_TO_STRING(buffer))));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_window_get_integer (SCM window, SCM property)
{
    int value;

    API_INIT_FUNC(1, "window_get_integer", API_RETURN_INT(-1));
    if (!scm_is_string (window) || !scm_is_string (property))
        API_WRONG_ARGS(API_RETURN_INT(-1));

    value = weechat_window_get_integer (API_STR2PTR(API_SCM_TO_STRING(window)),
                                        API_SCM_TO_STRING(property));

    API_RETURN_INT(value);
}

SCM
weechat_guile_api_window_get_string (SCM window, SCM property)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "window_get_string", API_RETURN_EMPTY);
    if (!scm_is_string (window) || !scm_is_string (property))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_window_get_string (API_STR2PTR(API_SCM_TO_STRING(window)),
                                        API_SCM_TO_STRING(property));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_window_get_pointer (SCM window, SCM property)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "window_get_pointer", API_RETURN_EMPTY);
    if (!scm_is_string (window) || !scm_is_string (property))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_window_get_pointer (API_STR2PTR(API_SCM_TO_STRING(window)),
                                                     API_SCM_TO_STRING(property)));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_window_set_title (SCM title)
{
    API_INIT_FUNC(1, "window_set_title", API_RETURN_ERROR);
    if (!scm_is_string (title))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_window_set_title (API_SCM_TO_STRING(title));

    API_RETURN_OK;
}

SCM
weechat_guile_api_nicklist_add_group (SCM buffer, SCM parent_group, SCM name,
                                      SCM color, SCM visible)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "nicklist_add_group", API_RETURN_EMPTY);
    if (!scm_is_string (buffer) || !scm_is_string (parent_group)
        || !scm_is_string (name) || !scm_is_string (color)
        || !scm_is_integer (visible))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_nicklist_add_group (API_STR2PTR(API_SCM_TO_STRING(buffer)),
                                                     API_STR2PTR(API_SCM_TO_STRING(parent_group)),
                                                     API_SCM_TO_STRING(name),
                                                     API_SCM_TO_STRING(color),
                                                     scm_to_int (visible)));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_nicklist_search_group (SCM buffer, SCM from_group, SCM name)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "nicklist_search_group", API_RETURN_EMPTY);
    if (!scm_is_string (buffer) || !scm_is_string (from_group)
        || !scm_is_string (name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_nicklist_search_group (API_STR2PTR(API_SCM_TO_STRING(buffer)),
                                                        API_STR2PTR(API_SCM_TO_STRING(from_group)),
                                                        API_SCM_TO_STRING(name)));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_nicklist_add_nick (SCM buffer, SCM group, SCM name,
                                     SCM color, SCM prefix, SCM prefix_color,
                                     SCM visible)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "nicklist_add_nick", API_RETURN_EMPTY);
    if (!scm_is_string (buffer) || !scm_is_string (group)
        || !scm_is_string (name) || !scm_is_string (color)
        || !scm_is_string (prefix) || !scm_is_string (prefix_color)
        || !scm_is_integer (visible))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_nicklist_add_nick (API_STR2PTR(API_SCM_TO_STRING(buffer)),
                                                    API_STR2PTR(API_SCM_TO_STRING(group)),
                                                    API_SCM_TO_STRING(name),
                                                    API_SCM_TO_STRING(color),
                                                    API_SCM_TO_STRING(prefix),
                                                    API_SCM_TO_STRING(prefix_color),
                                                    scm_to_int (visible)));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_nicklist_search_nick (SCM buffer, SCM from_group, SCM name)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "nicklist_search_nick", API_RETURN_EMPTY);
    if (!scm_is_string (buffer) || !scm_is_string (from_group)
        || !scm_is_string (name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_nicklist_search_nick (API_STR2PTR(API_SCM_TO_STRING(buffer)),
                                                       API_STR2PTR(API_SCM_TO_STRING(from_group)),
                                                       API_SCM_TO_STRING(name)));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_nicklist_remove_group (SCM buffer, SCM group)
{
    API_INIT_FUNC(1, "nicklist_remove_group", API_RETURN_ERROR);
    if (!scm_is_string (buffer) || !scm_is_string (group))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_nicklist_remove_group (API_STR2PTR(API_SCM_TO_STRING(buffer)),
                                   API_STR2PTR(API_SCM_TO_STRING(group)));

    API_RETURN_OK;
}

SCM
weechat_guile_api_nicklist_remove_nick (SCM buffer, SCM nick)
{
    API_INIT_FUNC(1, "nicklist_remove_nick", API_RETURN_ERROR);
    if (!scm_is_string (buffer) || !scm_is_string (nick))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_nicklist_remove_nick (API_STR2PTR(API_SCM_TO_STRING(buffer)),
                                  API_STR2PTR(API_SCM_TO_STRING(nick)));

    API_RETURN_OK;
}

SCM
weechat_guile_api_nicklist_remove_all (SCM buffer)
{
    API_INIT_FUNC(1, "nicklist_remove_all", API_RETURN_ERROR);
    if (!scm_is_string (buffer))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_nicklist_remove_all (API_STR2PTR(API_SCM_TO_STRING(buffer)));

    API_RETURN_OK;
}

SCM
weechat_guile_api_nicklist_group_get_integer (SCM buffer, SCM group,
                                              SCM property)
{
    int value;

    API_INIT_FUNC(1, "nicklist_group_get_integer", API_RETURN_INT(-1));
    if (!scm_is_string (buffer) || !scm_is_string (group)
        || !scm_is_string (property))
        API_WRONG_ARGS(API_RETURN_INT(-1));

    value = weechat_nicklist_group_get_integer (API_STR2PTR(API_SCM_TO_STRING(buffer)),
                                                API_STR2PTR(API_SCM_TO_STRING(group)),
                                                API_SCM_TO_STRING(property));

    API_RETURN_INT(value);
}

SCM
weechat_guile_api_nicklist_group_get_string (SCM buffer, SCM group,
                                             SCM property)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "nicklist_group_get_string", API_RETURN_EMPTY);
    if (!scm_is_string (buffer) || !scm_is_string (group)
        || !scm_is_string (property))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_nicklist_group_get_string (API_STR2PTR(API_SCM_TO_STRING(buffer)),
                                                API_STR2PTR(API_SCM_TO_STRING(group)),
                                                API_SCM_TO_STRING(property));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_nicklist_group_get_pointer (SCM buffer, SCM group,
                                              SCM property)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "nicklist_group_get_pointer", API_RETURN_EMPTY);
    if (!scm_is_string (buffer) || !scm_is_string (group)
        || !scm_is_string (property))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_nicklist_group_get_pointer (API_STR2PTR(API_SCM_TO_STRING(buffer)),
                                                             API_STR2PTR(API_SCM_TO_STRING(group)),
                                                             API_SCM_TO_STRING(property)));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_nicklist_group_set (SCM buffer, SCM group, SCM property,
                                      SCM value)
{
    API_INIT_FUNC(1, "nicklist_group_set", API_RETURN_ERROR);
    if (!scm_is_string (buffer) || !scm_is_string (group) || !scm_is_string (property) || !scm_is_string (value))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_nicklist_group_set (API_STR2PTR(API_SCM_TO_STRING(buffer)),
                                API_STR2PTR(API_SCM_TO_STRING(group)),
                                API_SCM_TO_STRING(property),
                                API_SCM_TO_STRING(value));

    API_RETURN_OK;
}

SCM
weechat_guile_api_nicklist_nick_get_integer (SCM buffer, SCM nick, SCM property)
{
    int value;

    API_INIT_FUNC(1, "nicklist_nick_get_integer", API_RETURN_INT(-1));
    if (!scm_is_string (buffer) || !scm_is_string (nick)
        || !scm_is_string (property))
        API_WRONG_ARGS(API_RETURN_INT(-1));

    value = weechat_nicklist_nick_get_integer (API_STR2PTR(API_SCM_TO_STRING(buffer)),
                                               API_STR2PTR(API_SCM_TO_STRING(nick)),
                                               API_SCM_TO_STRING(property));

    API_RETURN_INT(value);
}

SCM
weechat_guile_api_nicklist_nick_get_string (SCM buffer, SCM nick, SCM property)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "nicklist_nick_get_string", API_RETURN_EMPTY);
    if (!scm_is_string (buffer) || !scm_is_string (nick)
        || !scm_is_string (property))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_nicklist_nick_get_string (API_STR2PTR(API_SCM_TO_STRING(buffer)),
                                               API_STR2PTR(API_SCM_TO_STRING(nick)),
                                               API_SCM_TO_STRING(property));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_nicklist_nick_get_pointer (SCM buffer, SCM nick, SCM property)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "nicklist_nick_get_pointer", API_RETURN_EMPTY);
    if (!scm_is_string (buffer) || !scm_is_string (nick)
        || !scm_is_string (property))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_nicklist_nick_get_pointer (API_STR2PTR(API_SCM_TO_STRING(buffer)),
                                                            API_STR2PTR(API_SCM_TO_STRING(nick)),
                                                            API_SCM_TO_STRING(property)));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_nicklist_nick_set (SCM buffer, SCM nick, SCM property,
                                     SCM value)
{
    API_INIT_FUNC(1, "nicklist_nick_set", API_RETURN_ERROR);
    if (!scm_is_string (buffer) || !scm_is_string (nick)
        || !scm_is_string (property) || !scm_is_string (value))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_nicklist_nick_set (API_STR2PTR(API_SCM_TO_STRING(buffer)),
                               API_STR2PTR(API_SCM_TO_STRING(nick)),
                               API_SCM_TO_STRING(property),
                               API_SCM_TO_STRING(value));

    API_RETURN_OK;
}

SCM
weechat_guile_api_bar_item_search (SCM name)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "bar_item_search", API_RETURN_EMPTY);
    if (!scm_is_string (name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_bar_item_search (API_SCM_TO_STRING(name)));

    API_RETURN_STRING(result);
}

char *
weechat_guile_api_bar_item_build_cb (const void *pointer, void *data,
                                     struct t_gui_bar_item *item,
                                     struct t_gui_window *window,
                                     struct t_gui_buffer *buffer,
                                     struct t_hashtable *extra_info)
{
    struct t_plugin_script *script;
    void *func_argv[5];
    char empty_arg[1] = { '\0' }, *ret;
    const char *ptr_function, *ptr_data;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        if (strncmp (ptr_function, "(extra)", 7) == 0)
        {
            /* new callback: data, item, window, buffer, extra_info */
            func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
            func_argv[1] = (char *)API_PTR2STR(item);
            func_argv[2] = (char *)API_PTR2STR(window);
            func_argv[3] = (char *)API_PTR2STR(buffer);
            func_argv[4] = extra_info;

            ret = (char *)weechat_guile_exec (script,
                                              WEECHAT_SCRIPT_EXEC_STRING,
                                              ptr_function + 7,
                                              "ssssh", func_argv);
        }
        else
        {
            /* old callback: data, item, window */
            func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
            func_argv[1] = (char *)API_PTR2STR(item);
            func_argv[2] = (char *)API_PTR2STR(window);

            ret = (char *)weechat_guile_exec (script,
                                              WEECHAT_SCRIPT_EXEC_STRING,
                                              ptr_function,
                                              "sss", func_argv);
        }

        return ret;
    }

    return NULL;
}

SCM
weechat_guile_api_bar_item_new (SCM name, SCM function, SCM data)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "bar_item_new", API_RETURN_EMPTY);
    if (!scm_is_string (name) || !scm_is_string (function)
        || !scm_is_string (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(plugin_script_api_bar_item_new (weechat_guile_plugin,
                                                         guile_current_script,
                                                         API_SCM_TO_STRING(name),
                                                         &weechat_guile_api_bar_item_build_cb,
                                                         API_SCM_TO_STRING(function),
                                                         API_SCM_TO_STRING(data)));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_bar_item_update (SCM name)
{
    API_INIT_FUNC(1, "bar_item_update", API_RETURN_ERROR);
    if (!scm_is_string (name))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_bar_item_update (API_SCM_TO_STRING(name));

    API_RETURN_OK;
}

SCM
weechat_guile_api_bar_item_remove (SCM item)
{
    API_INIT_FUNC(1, "bar_item_remove", API_RETURN_ERROR);
    if (!scm_is_string (item))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_bar_item_remove (API_STR2PTR(API_SCM_TO_STRING(item)));

    API_RETURN_OK;
}

SCM
weechat_guile_api_bar_search (SCM name)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "bar_search", API_RETURN_EMPTY);
    if (!scm_is_string (name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_bar_search (API_SCM_TO_STRING(name)));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_bar_new (SCM args)
{
    SCM name, hidden, priority, type, conditions, position, filling_top_bottom;
    SCM filling_left_right, size, size_max, color_fg, color_delim, color_bg;
    SCM color_bg_inactive, separator, items;
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "bar_new", API_RETURN_EMPTY);
    if (!scm_list_p (args) || (scm_to_int (scm_length (args)) != 16))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    name = scm_list_ref (args, scm_from_int (0));
    hidden = scm_list_ref (args, scm_from_int (1));
    priority = scm_list_ref (args, scm_from_int (2));
    type = scm_list_ref (args, scm_from_int (3));
    conditions = scm_list_ref (args, scm_from_int (4));
    position = scm_list_ref (args, scm_from_int (5));
    filling_top_bottom = scm_list_ref (args, scm_from_int (6));
    filling_left_right = scm_list_ref (args, scm_from_int (7));
    size = scm_list_ref (args, scm_from_int (8));
    size_max = scm_list_ref (args, scm_from_int (9));
    color_fg = scm_list_ref (args, scm_from_int (10));
    color_delim = scm_list_ref (args, scm_from_int (11));
    color_bg = scm_list_ref (args, scm_from_int (12));
    color_bg_inactive = scm_list_ref (args, scm_from_int (13));
    separator = scm_list_ref (args, scm_from_int (14));
    items = scm_list_ref (args, scm_from_int (15));

    if (!scm_is_string (name) || !scm_is_string (hidden)
        || !scm_is_string (priority) || !scm_is_string (type)
        || !scm_is_string (conditions) || !scm_is_string (position)
        || !scm_is_string (filling_top_bottom) || !scm_is_string (filling_left_right)
        || !scm_is_string (size) || !scm_is_string (size_max)
        || !scm_is_string (color_fg) || !scm_is_string (color_delim)
        || !scm_is_string (color_bg) || !scm_is_string (color_bg_inactive)
        || !scm_is_string (separator) || !scm_is_string (items))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(
        weechat_bar_new (
            API_SCM_TO_STRING(name),
            API_SCM_TO_STRING(hidden),
            API_SCM_TO_STRING(priority),
            API_SCM_TO_STRING(type),
            API_SCM_TO_STRING(conditions),
            API_SCM_TO_STRING(position),
            API_SCM_TO_STRING(filling_top_bottom),
            API_SCM_TO_STRING(filling_left_right),
            API_SCM_TO_STRING(size),
            API_SCM_TO_STRING(size_max),
            API_SCM_TO_STRING(color_fg),
            API_SCM_TO_STRING(color_delim),
            API_SCM_TO_STRING(color_bg),
            API_SCM_TO_STRING(color_bg_inactive),
            API_SCM_TO_STRING(separator),
            API_SCM_TO_STRING(items)));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_bar_set (SCM bar, SCM property, SCM value)
{
    int rc;

    API_INIT_FUNC(1, "bar_set", API_RETURN_INT(0));
    if (!scm_is_string (bar) || !scm_is_string (property)
        || !scm_is_string (value))
        API_WRONG_ARGS(API_RETURN_INT(0));

    rc = weechat_bar_set (API_STR2PTR(API_SCM_TO_STRING(bar)),
                          API_SCM_TO_STRING(property),
                          API_SCM_TO_STRING(value));

    API_RETURN_INT(rc);
}

SCM
weechat_guile_api_bar_update (SCM name)
{
    API_INIT_FUNC(1, "bar_update", API_RETURN_ERROR);
    if (!scm_is_string (name))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_bar_update (API_SCM_TO_STRING(name));

    API_RETURN_OK;
}

SCM
weechat_guile_api_bar_remove (SCM bar)
{
    API_INIT_FUNC(1, "bar_remove", API_RETURN_ERROR);
    if (!scm_is_string (bar))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_bar_remove (API_STR2PTR(API_SCM_TO_STRING(bar)));

    API_RETURN_OK;
}

SCM
weechat_guile_api_command (SCM buffer, SCM command)
{
    int rc;

    API_INIT_FUNC(1, "command", API_RETURN_INT(WEECHAT_RC_ERROR));
    if (!scm_is_string (buffer) || !scm_is_string (command))
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_RC_ERROR));

    rc = plugin_script_api_command (weechat_guile_plugin,
                                    guile_current_script,
                                    API_STR2PTR(API_SCM_TO_STRING(buffer)),
                                    API_SCM_TO_STRING(command));

    API_RETURN_INT(rc);
}

SCM
weechat_guile_api_command_options (SCM buffer, SCM command, SCM options)
{
    struct t_hashtable *c_options;
    int rc;

    API_INIT_FUNC(1, "command_options", API_RETURN_INT(WEECHAT_RC_ERROR));
    if (!scm_is_string (buffer) || !scm_is_string (command)
        || !scm_list_p (options))
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_RC_ERROR));

    c_options = weechat_guile_alist_to_hashtable (options,
                                                  WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                                  WEECHAT_HASHTABLE_STRING,
                                                  WEECHAT_HASHTABLE_STRING);

    rc = plugin_script_api_command_options (weechat_guile_plugin,
                                            guile_current_script,
                                            API_STR2PTR(API_SCM_TO_STRING(buffer)),
                                            API_SCM_TO_STRING(command),
                                            c_options);

    weechat_hashtable_free (c_options);

    API_RETURN_INT(rc);
}

SCM
weechat_guile_api_completion_new (SCM buffer)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "completion_new", API_RETURN_EMPTY);
    if (!scm_is_string (buffer))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(
        weechat_completion_new (API_STR2PTR(API_SCM_TO_STRING(buffer))));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_completion_search (SCM completion, SCM data, SCM position,
                                     SCM direction)
{
    int rc;

    API_INIT_FUNC(1, "completion_search", API_RETURN_INT(0));
    if (!scm_is_string (completion) || !scm_is_string (data)
        || !scm_is_integer (position) || !scm_is_integer (direction))
        API_WRONG_ARGS(API_RETURN_INT(0));

    rc = weechat_completion_search (API_STR2PTR(API_SCM_TO_STRING(completion)),
                                    API_SCM_TO_STRING(data),
                                    scm_to_int (position),
                                    scm_to_int (direction));

    API_RETURN_INT(rc);
}

SCM
weechat_guile_api_completion_get_string (SCM completion, SCM property)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "completion_get_string", API_RETURN_EMPTY);
    if (!scm_is_string (completion) || !scm_is_string (property))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_completion_get_string (
        API_STR2PTR(API_SCM_TO_STRING(completion)),
        API_SCM_TO_STRING(property));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_completion_list_add (SCM completion, SCM word,
                                       SCM nick_completion, SCM where)
{
    API_INIT_FUNC(1, "completion_list_add", API_RETURN_ERROR);
    if (!scm_is_string (completion) || !scm_is_string (word)
        || !scm_is_integer (nick_completion) || !scm_is_string (where))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_completion_list_add (API_STR2PTR(API_SCM_TO_STRING(completion)),
                                 API_SCM_TO_STRING(word),
                                 scm_to_int (nick_completion),
                                 API_SCM_TO_STRING(where));

    API_RETURN_OK;
}

SCM
weechat_guile_api_completion_free (SCM completion)
{
    API_INIT_FUNC(1, "completion_free", API_RETURN_ERROR);
    if (!scm_is_string (completion))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_completion_free (API_STR2PTR(API_SCM_TO_STRING(completion)));

    API_RETURN_OK;
}

SCM
weechat_guile_api_info_get (SCM info_name, SCM arguments)
{
    char *result;
    SCM return_value;

    API_INIT_FUNC(1, "info_get", API_RETURN_EMPTY);
    if (!scm_is_string (info_name) || !scm_is_string (arguments))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_info_get (API_SCM_TO_STRING(info_name),
                               API_SCM_TO_STRING(arguments));

    API_RETURN_STRING_FREE(result);
}

SCM
weechat_guile_api_info_get_hashtable (SCM info_name, SCM hash)
{
    struct t_hashtable *c_hashtable, *result_hashtable;
    SCM result_alist;

    API_INIT_FUNC(1, "info_get_hashtable", API_RETURN_EMPTY);
    if (!scm_is_string (info_name) || !scm_list_p (hash))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    c_hashtable = weechat_guile_alist_to_hashtable (hash,
                                                    WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                                    WEECHAT_HASHTABLE_STRING,
                                                    WEECHAT_HASHTABLE_STRING);

    result_hashtable = weechat_info_get_hashtable (API_SCM_TO_STRING(info_name),
                                                   c_hashtable);
    result_alist = weechat_guile_hashtable_to_alist (result_hashtable);

    weechat_hashtable_free (c_hashtable);
    weechat_hashtable_free (result_hashtable);

    API_RETURN_OTHER(result_alist);
}

SCM
weechat_guile_api_infolist_new ()
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "infolist_new", API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_infolist_new ());

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_infolist_new_item (SCM infolist)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "infolist_new_item", API_RETURN_EMPTY);
    if (!scm_is_string (infolist))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_infolist_new_item (API_STR2PTR(API_SCM_TO_STRING(infolist))));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_infolist_new_var_integer (SCM item, SCM name, SCM value)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "infolist_new_var_integer", API_RETURN_EMPTY);
    if (!scm_is_string (item) || !scm_is_string (name)
        || !scm_is_integer (value))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_infolist_new_var_integer (API_STR2PTR(API_SCM_TO_STRING(item)),
                                                           API_SCM_TO_STRING(name),
                                                           scm_to_int (value)));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_infolist_new_var_string (SCM item, SCM name, SCM value)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "infolist_new_var_string", API_RETURN_EMPTY);
    if (!scm_is_string (item) || !scm_is_string (name)
        || !scm_is_string (value))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_infolist_new_var_string (API_STR2PTR(API_SCM_TO_STRING(item)),
                                                          API_SCM_TO_STRING(name),
                                                          API_SCM_TO_STRING(value)));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_infolist_new_var_pointer (SCM item, SCM name, SCM value)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "infolist_new_var_pointer", API_RETURN_EMPTY);
    if (!scm_is_string (item) || !scm_is_string (name)
        || !scm_is_string (value))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_infolist_new_var_pointer (API_STR2PTR(API_SCM_TO_STRING(item)),
                                                           API_SCM_TO_STRING(name),
                                                           API_STR2PTR(API_SCM_TO_STRING(value))));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_infolist_new_var_time (SCM item, SCM name, SCM value)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "infolist_new_var_time", API_RETURN_EMPTY);
    if (!scm_is_string (item) || !scm_is_string (name)
        || !scm_is_integer (value))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_infolist_new_var_time (API_STR2PTR(API_SCM_TO_STRING(item)),
                                                        API_SCM_TO_STRING(name),
                                                        (time_t)scm_to_long (value)));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_infolist_search_var (SCM infolist, SCM name)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "infolist_search_var", API_RETURN_EMPTY);
    if (!scm_is_string (infolist) || !scm_is_string (name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_infolist_search_var (API_STR2PTR(API_SCM_TO_STRING(infolist)),
                                                      API_SCM_TO_STRING(name)));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_infolist_get (SCM name, SCM pointer, SCM arguments)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "infolist_get", API_RETURN_EMPTY);
    if (!scm_is_string (name) || !scm_is_string (pointer)
        || !scm_is_string (arguments))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_infolist_get (API_SCM_TO_STRING(name),
                                               API_STR2PTR(API_SCM_TO_STRING(pointer)),
                                               API_SCM_TO_STRING(arguments)));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_infolist_next (SCM infolist)
{
    int value;

    API_INIT_FUNC(1, "infolist_next", API_RETURN_INT(0));
    if (!scm_is_string (infolist))
        API_WRONG_ARGS(API_RETURN_INT(0));

    value = weechat_infolist_next (API_STR2PTR(API_SCM_TO_STRING(infolist)));

    API_RETURN_INT(value);
}

SCM
weechat_guile_api_infolist_prev (SCM infolist)
{
    int value;

    API_INIT_FUNC(1, "infolist_prev", API_RETURN_INT(0));
    if (!scm_is_string (infolist))
        API_WRONG_ARGS(API_RETURN_INT(0));

    value = weechat_infolist_prev (API_STR2PTR(API_SCM_TO_STRING(infolist)));

    API_RETURN_INT(value);
}

SCM
weechat_guile_api_infolist_reset_item_cursor (SCM infolist)
{
    API_INIT_FUNC(1, "infolist_reset_item_cursor", API_RETURN_ERROR);
    if (!scm_is_string (infolist))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_infolist_reset_item_cursor (API_STR2PTR(API_SCM_TO_STRING(infolist)));

    API_RETURN_OK;
}

SCM
weechat_guile_api_infolist_fields (SCM infolist)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "infolist_fields", API_RETURN_EMPTY);
    if (!scm_is_string (infolist))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_infolist_fields (API_STR2PTR(API_SCM_TO_STRING(infolist)));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_infolist_integer (SCM infolist, SCM variable)
{
    int value;

    API_INIT_FUNC(1, "infolist_integer", API_RETURN_INT(0));
    if (!scm_is_string (infolist) || !scm_is_string (variable))
        API_WRONG_ARGS(API_RETURN_INT(0));

    value = weechat_infolist_integer (API_STR2PTR(API_SCM_TO_STRING(infolist)),
                                      API_SCM_TO_STRING(variable));

    API_RETURN_INT(value);
}

SCM
weechat_guile_api_infolist_string (SCM infolist, SCM variable)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "infolist_string", API_RETURN_EMPTY);
    if (!scm_is_string (infolist) || !scm_is_string (variable))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_infolist_string (API_STR2PTR(API_SCM_TO_STRING(infolist)),
                                      API_SCM_TO_STRING(variable));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_infolist_pointer (SCM infolist, SCM variable)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "infolist_pointer", API_RETURN_EMPTY);
    if (!scm_is_string (infolist) || !scm_is_string (variable))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_infolist_pointer (API_STR2PTR(API_SCM_TO_STRING(infolist)),
                                                   API_SCM_TO_STRING(variable)));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_infolist_time (SCM infolist, SCM variable)
{
    time_t time;

    API_INIT_FUNC(1, "infolist_time", API_RETURN_LONG(0));
    if (!scm_is_string (infolist) || !scm_is_string (variable))
        API_WRONG_ARGS(API_RETURN_LONG(0));

    time = weechat_infolist_time (API_STR2PTR(API_SCM_TO_STRING(infolist)),
                                  API_SCM_TO_STRING(variable));

    API_RETURN_LONG(time);
}

SCM
weechat_guile_api_infolist_free (SCM infolist)
{
    API_INIT_FUNC(1, "infolist_free", API_RETURN_ERROR);
    if (!scm_is_string (infolist))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_infolist_free (API_STR2PTR(API_SCM_TO_STRING(infolist)));

    API_RETURN_OK;
}

SCM
weechat_guile_api_hdata_get (SCM name)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "hdata_get", API_RETURN_EMPTY);
    if (!scm_is_string (name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_hdata_get (API_SCM_TO_STRING(name)));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_hdata_get_var_offset (SCM hdata, SCM name)
{
    int value;

    API_INIT_FUNC(1, "hdata_get_var_offset", API_RETURN_INT(0));
    if (!scm_is_string (hdata) || !scm_is_string (name))
        API_WRONG_ARGS(API_RETURN_INT(0));

    value = weechat_hdata_get_var_offset (API_STR2PTR(API_SCM_TO_STRING(hdata)),
                                          API_SCM_TO_STRING(name));

    API_RETURN_INT(value);
}

SCM
weechat_guile_api_hdata_get_var_type_string (SCM hdata, SCM name)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "hdata_get_var_type_string", API_RETURN_EMPTY);
    if (!scm_is_string (hdata) || !scm_is_string (name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_hdata_get_var_type_string (API_STR2PTR(API_SCM_TO_STRING(hdata)),
                                                API_SCM_TO_STRING(name));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_hdata_get_var_array_size (SCM hdata, SCM pointer, SCM name)
{
    int value;

    API_INIT_FUNC(1, "hdata_get_var_array_size", API_RETURN_INT(-1));
    if (!scm_is_string (hdata) || !scm_is_string (pointer)
        || !scm_is_string (name))
        API_WRONG_ARGS(API_RETURN_INT(-1));

    value = weechat_hdata_get_var_array_size (API_STR2PTR(API_SCM_TO_STRING(hdata)),
                                              API_STR2PTR(API_SCM_TO_STRING(pointer)),
                                              API_SCM_TO_STRING(name));

    API_RETURN_INT(value);
}

SCM
weechat_guile_api_hdata_get_var_array_size_string (SCM hdata, SCM pointer,
                                                    SCM name)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "hdata_get_var_array_size_string", API_RETURN_EMPTY);
    if (!scm_is_string (hdata) || !scm_is_string (pointer)
        || !scm_is_string (name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_hdata_get_var_array_size_string (API_STR2PTR(API_SCM_TO_STRING(hdata)),
                                                      API_STR2PTR(API_SCM_TO_STRING(pointer)),
                                                      API_SCM_TO_STRING(name));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_hdata_get_var_hdata (SCM hdata, SCM name)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "hdata_get_var_hdata", API_RETURN_EMPTY);
    if (!scm_is_string (hdata) || !scm_is_string (name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_hdata_get_var_hdata (API_STR2PTR(API_SCM_TO_STRING(hdata)),
                                          API_SCM_TO_STRING(name));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_hdata_get_list (SCM hdata, SCM name)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "hdata_get_list", API_RETURN_EMPTY);
    if (!scm_is_string (hdata) || !scm_is_string (name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_hdata_get_list (API_STR2PTR(API_SCM_TO_STRING(hdata)),
                                                 API_SCM_TO_STRING(name)));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_hdata_check_pointer (SCM hdata, SCM list, SCM pointer)
{
    int value;

    API_INIT_FUNC(1, "hdata_check_pointer", API_RETURN_INT(0));
    if (!scm_is_string (hdata) || !scm_is_string (list)
        || !scm_is_string (pointer))
        API_WRONG_ARGS(API_RETURN_INT(0));

    value = weechat_hdata_check_pointer (API_STR2PTR(API_SCM_TO_STRING(hdata)),
                                         API_STR2PTR(API_SCM_TO_STRING(list)),
                                         API_STR2PTR(API_SCM_TO_STRING(pointer)));

    API_RETURN_INT(value);
}

SCM
weechat_guile_api_hdata_move (SCM hdata, SCM pointer, SCM count)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "hdata_move", API_RETURN_EMPTY);
    if (!scm_is_string (hdata) || !scm_is_string (pointer)
        || !scm_is_integer (count))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_hdata_move (API_STR2PTR(API_SCM_TO_STRING(hdata)),
                                             API_STR2PTR(API_SCM_TO_STRING(pointer)),
                                             scm_to_int (count)));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_hdata_search (SCM hdata, SCM pointer, SCM search,
                                SCM pointers, SCM extra_vars, SCM options,
                                SCM move)
{
    const char *result;
    SCM return_value;
    struct t_hashtable *c_pointers, *c_extra_vars, *c_options;

    API_INIT_FUNC(1, "hdata_search", API_RETURN_EMPTY);
    if (!scm_is_string (hdata) || !scm_is_string (pointer)
        || !scm_is_string (search) || !scm_list_p (pointers)
        || !scm_list_p (extra_vars) || !scm_list_p (options)
        || !scm_is_integer (move))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    c_pointers = weechat_guile_alist_to_hashtable (pointers,
                                                   WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                                   WEECHAT_HASHTABLE_STRING,
                                                   WEECHAT_HASHTABLE_POINTER);
    c_extra_vars = weechat_guile_alist_to_hashtable (extra_vars,
                                                     WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                                     WEECHAT_HASHTABLE_STRING,
                                                     WEECHAT_HASHTABLE_STRING);
    c_options = weechat_guile_alist_to_hashtable (options,
                                                  WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                                  WEECHAT_HASHTABLE_STRING,
                                                  WEECHAT_HASHTABLE_STRING);

    result = API_PTR2STR(weechat_hdata_search (API_STR2PTR(API_SCM_TO_STRING(hdata)),
                                               API_STR2PTR(API_SCM_TO_STRING(pointer)),
                                               API_SCM_TO_STRING(search),
                                               c_pointers,
                                               c_extra_vars,
                                               c_options,
                                               scm_to_int (move)));

    weechat_hashtable_free (c_pointers);
    weechat_hashtable_free (c_extra_vars);
    weechat_hashtable_free (c_options);

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_hdata_char (SCM hdata, SCM pointer, SCM name)
{
    int value;

    API_INIT_FUNC(1, "hdata_char", API_RETURN_INT(0));
    if (!scm_is_string (hdata) || !scm_is_string (pointer)
        || !scm_is_string (name))
        API_WRONG_ARGS(API_RETURN_INT(0));

    value = (int)weechat_hdata_char (API_STR2PTR(API_SCM_TO_STRING(hdata)),
                                     API_STR2PTR(API_SCM_TO_STRING(pointer)),
                                     API_SCM_TO_STRING(name));

    API_RETURN_INT(value);
}

SCM
weechat_guile_api_hdata_integer (SCM hdata, SCM pointer, SCM name)
{
    int value;

    API_INIT_FUNC(1, "hdata_integer", API_RETURN_INT(0));
    if (!scm_is_string (hdata) || !scm_is_string (pointer)
        || !scm_is_string (name))
        API_WRONG_ARGS(API_RETURN_INT(0));

    value = weechat_hdata_integer (API_STR2PTR(API_SCM_TO_STRING(hdata)),
                                   API_STR2PTR(API_SCM_TO_STRING(pointer)),
                                   API_SCM_TO_STRING(name));

    API_RETURN_INT(value);
}

SCM
weechat_guile_api_hdata_long (SCM hdata, SCM pointer, SCM name)
{
    long value;

    API_INIT_FUNC(1, "hdata_long", API_RETURN_LONG(0));
    if (!scm_is_string (hdata) || !scm_is_string (pointer)
        || !scm_is_string (name))
        API_WRONG_ARGS(API_RETURN_LONG(0));

    value = weechat_hdata_long (API_STR2PTR(API_SCM_TO_STRING(hdata)),
                                API_STR2PTR(API_SCM_TO_STRING(pointer)),
                                API_SCM_TO_STRING(name));

    API_RETURN_LONG(value);
}

SCM
weechat_guile_api_hdata_longlong (SCM hdata, SCM pointer, SCM name)
{
    long long value;

    API_INIT_FUNC(1, "hdata_longlong", API_RETURN_LONGLONG(0));
    if (!scm_is_string (hdata) || !scm_is_string (pointer)
        || !scm_is_string (name))
        API_WRONG_ARGS(API_RETURN_LONGLONG(0));

    value = weechat_hdata_longlong (API_STR2PTR(API_SCM_TO_STRING(hdata)),
                                    API_STR2PTR(API_SCM_TO_STRING(pointer)),
                                    API_SCM_TO_STRING(name));

    API_RETURN_LONGLONG(value);
}

SCM
weechat_guile_api_hdata_string (SCM hdata, SCM pointer, SCM name)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "hdata_string", API_RETURN_EMPTY);
    if (!scm_is_string (hdata) || !scm_is_string (pointer)
        || !scm_is_string (name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_hdata_string (API_STR2PTR(API_SCM_TO_STRING(hdata)),
                                   API_STR2PTR(API_SCM_TO_STRING(pointer)),
                                   API_SCM_TO_STRING(name));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_hdata_pointer (SCM hdata, SCM pointer, SCM name)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "hdata_pointer", API_RETURN_EMPTY);
    if (!scm_is_string (hdata) || !scm_is_string (pointer)
        || !scm_is_string (name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_hdata_pointer (API_STR2PTR(API_SCM_TO_STRING(hdata)),
                                                API_STR2PTR(API_SCM_TO_STRING(pointer)),
                                                API_SCM_TO_STRING(name)));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_hdata_time (SCM hdata, SCM pointer, SCM name)
{
    time_t time;

    API_INIT_FUNC(1, "hdata_time", API_RETURN_LONG(0));
    if (!scm_is_string (hdata) || !scm_is_string (pointer) || !scm_is_string (name))
        API_WRONG_ARGS(API_RETURN_LONG(0));

    time = weechat_hdata_time (API_STR2PTR(API_SCM_TO_STRING(hdata)),
                               API_STR2PTR(API_SCM_TO_STRING(pointer)),
                               API_SCM_TO_STRING(name));

    API_RETURN_LONG(time);
}

SCM
weechat_guile_api_hdata_hashtable (SCM hdata, SCM pointer, SCM name)
{
    SCM result_alist;

    API_INIT_FUNC(1, "hdata_hashtable", API_RETURN_EMPTY);
    if (!scm_is_string (hdata) || !scm_is_string (pointer)
        || !scm_is_string (name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result_alist = weechat_guile_hashtable_to_alist (
        weechat_hdata_hashtable (API_STR2PTR(API_SCM_TO_STRING(hdata)),
                                 API_STR2PTR(API_SCM_TO_STRING(pointer)),
                                 API_SCM_TO_STRING(name)));

    API_RETURN_OTHER(result_alist);
}

SCM
weechat_guile_api_hdata_compare (SCM hdata, SCM pointer1, SCM pointer2,
                                 SCM name, SCM case_sensitive)
{
    int rc;

    API_INIT_FUNC(1, "hdata_compare", API_RETURN_INT(0));
    if (!scm_is_string (hdata) || !scm_is_string (pointer1)
        || !scm_is_string (pointer2) || !scm_is_string (name)
        || !scm_is_integer (case_sensitive))
        API_WRONG_ARGS(API_RETURN_INT(0));

    rc = weechat_hdata_compare (API_STR2PTR(API_SCM_TO_STRING(hdata)),
                                API_STR2PTR(API_SCM_TO_STRING(pointer1)),
                                API_STR2PTR(API_SCM_TO_STRING(pointer2)),
                                API_SCM_TO_STRING(name),
                                scm_to_int (case_sensitive));

    API_RETURN_INT(rc);
}

SCM
weechat_guile_api_hdata_update (SCM hdata, SCM pointer, SCM hashtable)
{
    struct t_hashtable *c_hashtable;
    int value;

    API_INIT_FUNC(1, "hdata_update", API_RETURN_INT(0));
    if (!scm_is_string (hdata) || !scm_is_string (pointer) || !scm_list_p (hashtable))
        API_WRONG_ARGS(API_RETURN_INT(0));

    c_hashtable = weechat_guile_alist_to_hashtable (hashtable,
                                                    WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                                    WEECHAT_HASHTABLE_STRING,
                                                    WEECHAT_HASHTABLE_STRING);

    value = weechat_hdata_update (API_STR2PTR(API_SCM_TO_STRING(hdata)),
                                  API_STR2PTR(API_SCM_TO_STRING(pointer)),
                                  c_hashtable);

    weechat_hashtable_free (c_hashtable);

    API_RETURN_INT(value);
}

SCM
weechat_guile_api_hdata_get_string (SCM hdata, SCM property)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "hdata_get_string", API_RETURN_EMPTY);
    if (!scm_is_string (hdata) || !scm_is_string (property))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_hdata_get_string (API_STR2PTR(API_SCM_TO_STRING(hdata)),
                                       API_SCM_TO_STRING(property));

    API_RETURN_STRING(result);
}

int
weechat_guile_api_upgrade_read_cb (const void *pointer, void *data,
                                   struct t_upgrade_file *upgrade_file,
                                   int object_id,
                                   struct t_infolist *infolist)
{
    struct t_plugin_script *script;
    void *func_argv[4];
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;
    int *rc, ret;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = (char *)API_PTR2STR(upgrade_file);
        func_argv[2] = &object_id;
        func_argv[3] = (char *)API_PTR2STR(infolist);

        rc = (int *) weechat_guile_exec (script,
                                         WEECHAT_SCRIPT_EXEC_INT,
                                         ptr_function,
                                         "ssis", func_argv);

        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }

        return ret;
    }

    return WEECHAT_RC_ERROR;
}

SCM
weechat_guile_api_upgrade_new (SCM filename, SCM function, SCM data)
{
    const char *result;
    SCM return_value;

    API_INIT_FUNC(1, "upgrade_new", API_RETURN_EMPTY);
    if (!scm_is_string (filename) || !scm_is_string (function)
        || !scm_is_string (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(
        plugin_script_api_upgrade_new (
            weechat_guile_plugin,
            guile_current_script,
            API_SCM_TO_STRING(filename),
            &weechat_guile_api_upgrade_read_cb,
            API_SCM_TO_STRING(function),
            API_SCM_TO_STRING(data)));

    API_RETURN_STRING(result);
}

SCM
weechat_guile_api_upgrade_write_object (SCM upgrade_file, SCM object_id,
                                        SCM infolist)
{
    int rc;

    API_INIT_FUNC(1, "upgrade_write_object", API_RETURN_INT(0));
    if (!scm_is_string (upgrade_file) || !scm_is_integer (object_id)
        || !scm_is_string (infolist))
        API_WRONG_ARGS(API_RETURN_INT(0));

    rc = weechat_upgrade_write_object (API_STR2PTR(API_SCM_TO_STRING(upgrade_file)),
                                       scm_to_int (object_id),
                                       API_STR2PTR(API_SCM_TO_STRING(infolist)));

    API_RETURN_INT(rc);
}

SCM
weechat_guile_api_upgrade_read (SCM upgrade_file)
{
    int rc;

    API_INIT_FUNC(1, "upgrade_read", API_RETURN_INT(0));
    if (!scm_is_string (upgrade_file))
        API_WRONG_ARGS(API_RETURN_INT(0));

    rc = weechat_upgrade_read (API_STR2PTR(API_SCM_TO_STRING(upgrade_file)));

    API_RETURN_INT(rc);
}

SCM
weechat_guile_api_upgrade_close (SCM upgrade_file)
{
    API_INIT_FUNC(1, "upgrade_close", API_RETURN_ERROR);
    if (!scm_is_string (upgrade_file))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_upgrade_close (API_STR2PTR(API_SCM_TO_STRING(upgrade_file)));

    API_RETURN_OK;
}

/*
 * Initializes guile functions and constants.
 */

void
weechat_guile_api_module_init (void *data)
{
    char str_const[256];
    int i;
#if SCM_MAJOR_VERSION >= 3 || (SCM_MAJOR_VERSION == 2 && SCM_MINOR_VERSION >= 2)
    /* Guile >= 2.2 */
    scm_t_port_type *port_type;

    port_type = scm_make_port_type ("weechat_stdout",
                                    &weechat_guile_port_fill_input,
                                    &weechat_guile_port_write);
    guile_port = scm_c_make_port (port_type, SCM_WRTNG | SCM_BUF0, 0);
    scm_set_current_output_port (guile_port);
    scm_set_current_error_port (guile_port);
#else
    /* Guile < 2.2 */
    scm_t_bits port_type;

    port_type = scm_make_port_type ("weechat_stdout",
                                    &weechat_guile_port_fill_input,
                                    &weechat_guile_port_write);
    guile_port = scm_new_port_table_entry (port_type);
    SCM_SET_CELL_TYPE (guile_port, port_type | SCM_OPN | SCM_WRTNG);
    scm_set_current_output_port (guile_port);
    scm_set_current_error_port (guile_port);
#endif

    /* make C compiler happy */
    (void) data;

    /* interface functions */
    API_DEF_FUNC(register, 7);
    API_DEF_FUNC(plugin_get_name, 1);
    API_DEF_FUNC(charset_set, 1);
    API_DEF_FUNC(iconv_to_internal, 2);
    API_DEF_FUNC(iconv_from_internal, 2);
    API_DEF_FUNC(gettext, 1);
    API_DEF_FUNC(ngettext, 3);
    API_DEF_FUNC(strlen_screen, 1);
    API_DEF_FUNC(string_match, 3);
    API_DEF_FUNC(string_match_list, 3);
    API_DEF_FUNC(string_has_highlight, 2);
    API_DEF_FUNC(string_has_highlight_regex, 2);
    API_DEF_FUNC(string_mask_to_regex, 1);
    API_DEF_FUNC(string_format_size, 1);
    API_DEF_FUNC(string_parse_size, 1);
    API_DEF_FUNC(string_color_code_size, 1);
    API_DEF_FUNC(string_remove_color, 2);
    API_DEF_FUNC(string_is_command_char, 1);
    API_DEF_FUNC(string_input_for_buffer, 1);
    API_DEF_FUNC(string_eval_expression, 4);
    API_DEF_FUNC(string_eval_path_home, 4);
    API_DEF_FUNC(mkdir_home, 2);
    API_DEF_FUNC(mkdir, 2);
    API_DEF_FUNC(mkdir_parents, 2);
    API_DEF_FUNC(list_new, 0);
    API_DEF_FUNC(list_add, 4);
    API_DEF_FUNC(list_search, 2);
    API_DEF_FUNC(list_search_pos, 2);
    API_DEF_FUNC(list_casesearch, 2);
    API_DEF_FUNC(list_casesearch_pos, 2);
    API_DEF_FUNC(list_get, 2);
    API_DEF_FUNC(list_set, 2);
    API_DEF_FUNC(list_next, 1);
    API_DEF_FUNC(list_prev, 1);
    API_DEF_FUNC(list_string, 1);
    API_DEF_FUNC(list_size, 1);
    API_DEF_FUNC(list_remove, 2);
    API_DEF_FUNC(list_remove_all, 1);
    API_DEF_FUNC(list_free, 1);
    API_DEF_FUNC(config_new, 3);
    API_DEF_FUNC(config_set_version, 4);
    API_DEF_FUNC(config_new_section, 1);
    API_DEF_FUNC(config_search_section, 2);
    API_DEF_FUNC(config_new_option, 1);
    API_DEF_FUNC(config_search_option, 3);
    API_DEF_FUNC(config_string_to_boolean, 1);
    API_DEF_FUNC(config_option_reset, 2);
    API_DEF_FUNC(config_option_set, 3);
    API_DEF_FUNC(config_option_set_null, 2);
    API_DEF_FUNC(config_option_unset, 1);
    API_DEF_FUNC(config_option_rename, 2);
    API_DEF_FUNC(config_option_get_string, 2);
    API_DEF_FUNC(config_option_get_pointer, 2);
    API_DEF_FUNC(config_option_is_null, 1);
    API_DEF_FUNC(config_option_default_is_null, 1);
    API_DEF_FUNC(config_boolean, 1);
    API_DEF_FUNC(config_boolean_default, 1);
    API_DEF_FUNC(config_boolean_inherited, 1);
    API_DEF_FUNC(config_integer, 1);
    API_DEF_FUNC(config_integer_default, 1);
    API_DEF_FUNC(config_integer_inherited, 1);
    API_DEF_FUNC(config_string, 1);
    API_DEF_FUNC(config_string_default, 1);
    API_DEF_FUNC(config_string_inherited, 1);
    API_DEF_FUNC(config_color, 1);
    API_DEF_FUNC(config_color_default, 1);
    API_DEF_FUNC(config_color_inherited, 1);
    API_DEF_FUNC(config_enum, 1);
    API_DEF_FUNC(config_enum_default, 1);
    API_DEF_FUNC(config_enum_inherited, 1);
    API_DEF_FUNC(config_write_option, 2);
    API_DEF_FUNC(config_write_line, 3);
    API_DEF_FUNC(config_write, 1);
    API_DEF_FUNC(config_read, 1);
    API_DEF_FUNC(config_reload, 1);
    API_DEF_FUNC(config_option_free, 1);
    API_DEF_FUNC(config_section_free_options, 1);
    API_DEF_FUNC(config_section_free, 1);
    API_DEF_FUNC(config_free, 1);
    API_DEF_FUNC(config_get, 1);
    API_DEF_FUNC(config_get_plugin, 1);
    API_DEF_FUNC(config_is_set_plugin, 1);
    API_DEF_FUNC(config_set_plugin, 2);
    API_DEF_FUNC(config_set_desc_plugin, 2);
    API_DEF_FUNC(config_unset_plugin, 1);
    API_DEF_FUNC(key_bind, 2);
    API_DEF_FUNC(key_unbind, 2);
    API_DEF_FUNC(prefix, 1);
    API_DEF_FUNC(color, 1);
    API_DEF_FUNC(print, 2);
    API_DEF_FUNC(print_date_tags, 4);
    API_DEF_FUNC(print_datetime_tags, 5);
    API_DEF_FUNC(print_y, 3);
    API_DEF_FUNC(print_y_date_tags, 5);
    API_DEF_FUNC(print_y_datetime_tags, 6);
    API_DEF_FUNC(log_print, 1);
    API_DEF_FUNC(hook_command, 7);
    API_DEF_FUNC(hook_completion, 4);
    API_DEF_FUNC(hook_completion_get_string, 2);
    API_DEF_FUNC(hook_completion_list_add, 4);
    API_DEF_FUNC(hook_command_run, 3);
    API_DEF_FUNC(hook_timer, 5);
    API_DEF_FUNC(hook_fd, 6);
    API_DEF_FUNC(hook_process, 4);
    API_DEF_FUNC(hook_process_hashtable, 5);
    API_DEF_FUNC(hook_url, 5);
    API_DEF_FUNC(hook_connect, 8);
    API_DEF_FUNC(hook_line, 5);
    API_DEF_FUNC(hook_print, 6);
    API_DEF_FUNC(hook_signal, 3);
    API_DEF_FUNC(hook_signal_send, 3);
    API_DEF_FUNC(hook_hsignal, 3);
    API_DEF_FUNC(hook_hsignal_send, 2);
    API_DEF_FUNC(hook_config, 3);
    API_DEF_FUNC(hook_modifier, 3);
    API_DEF_FUNC(hook_modifier_exec, 3);
    API_DEF_FUNC(hook_info, 5);
    API_DEF_FUNC(hook_info_hashtable, 6);
    API_DEF_FUNC(hook_infolist, 6);
    API_DEF_FUNC(hook_focus, 3);
    API_DEF_FUNC(hook_set, 3);
    API_DEF_FUNC(unhook, 1);
    API_DEF_FUNC(unhook_all, 0);
    API_DEF_FUNC(buffer_new, 5);
    API_DEF_FUNC(buffer_new_props, 6);
    API_DEF_FUNC(buffer_search, 2);
    API_DEF_FUNC(buffer_search_main, 0);
    API_DEF_FUNC(current_buffer, 0);
    API_DEF_FUNC(buffer_clear, 1);
    API_DEF_FUNC(buffer_close, 1);
    API_DEF_FUNC(buffer_merge, 2);
    API_DEF_FUNC(buffer_unmerge, 2);
    API_DEF_FUNC(buffer_get_integer, 2);
    API_DEF_FUNC(buffer_get_string, 2);
    API_DEF_FUNC(buffer_get_pointer, 2);
    API_DEF_FUNC(buffer_set, 3);
    API_DEF_FUNC(buffer_string_replace_local_var, 2);
    API_DEF_FUNC(buffer_match_list, 2);
    API_DEF_FUNC(current_window, 0);
    API_DEF_FUNC(window_search_with_buffer, 1);
    API_DEF_FUNC(window_get_integer, 2);
    API_DEF_FUNC(window_get_string, 2);
    API_DEF_FUNC(window_get_pointer, 2);
    API_DEF_FUNC(window_set_title, 1);
    API_DEF_FUNC(nicklist_add_group, 5);
    API_DEF_FUNC(nicklist_search_group, 3);
    API_DEF_FUNC(nicklist_add_nick, 7);
    API_DEF_FUNC(nicklist_search_nick, 3);
    API_DEF_FUNC(nicklist_remove_group, 2);
    API_DEF_FUNC(nicklist_remove_nick, 2);
    API_DEF_FUNC(nicklist_remove_all, 1);
    API_DEF_FUNC(nicklist_group_get_integer, 3);
    API_DEF_FUNC(nicklist_group_get_string, 3);
    API_DEF_FUNC(nicklist_group_get_pointer, 3);
    API_DEF_FUNC(nicklist_group_set, 4);
    API_DEF_FUNC(nicklist_nick_get_integer, 3);
    API_DEF_FUNC(nicklist_nick_get_string, 3);
    API_DEF_FUNC(nicklist_nick_get_pointer, 3);
    API_DEF_FUNC(nicklist_nick_set, 4);
    API_DEF_FUNC(bar_item_search, 1);
    API_DEF_FUNC(bar_item_new, 3);
    API_DEF_FUNC(bar_item_update, 1);
    API_DEF_FUNC(bar_item_remove, 1);
    API_DEF_FUNC(bar_search, 1);
    API_DEF_FUNC(bar_new, 1);
    API_DEF_FUNC(bar_set, 3);
    API_DEF_FUNC(bar_update, 1);
    API_DEF_FUNC(bar_remove, 1);
    API_DEF_FUNC(command, 2);
    API_DEF_FUNC(command_options, 3);
    API_DEF_FUNC(completion_new, 1);
    API_DEF_FUNC(completion_search, 4);
    API_DEF_FUNC(completion_get_string, 2);
    API_DEF_FUNC(completion_list_add, 4);
    API_DEF_FUNC(completion_free, 1);
    API_DEF_FUNC(info_get, 2);
    API_DEF_FUNC(info_get_hashtable, 2);
    API_DEF_FUNC(infolist_new, 0);
    API_DEF_FUNC(infolist_new_item, 1);
    API_DEF_FUNC(infolist_new_var_integer, 3);
    API_DEF_FUNC(infolist_new_var_string, 3);
    API_DEF_FUNC(infolist_new_var_pointer, 3);
    API_DEF_FUNC(infolist_new_var_time, 3);
    API_DEF_FUNC(infolist_search_var, 2);
    API_DEF_FUNC(infolist_get, 3);
    API_DEF_FUNC(infolist_next, 1);
    API_DEF_FUNC(infolist_prev, 1);
    API_DEF_FUNC(infolist_reset_item_cursor, 1);
    API_DEF_FUNC(infolist_fields, 1);
    API_DEF_FUNC(infolist_integer, 2);
    API_DEF_FUNC(infolist_string, 2);
    API_DEF_FUNC(infolist_pointer, 2);
    API_DEF_FUNC(infolist_time, 2);
    API_DEF_FUNC(infolist_free, 1);
    API_DEF_FUNC(hdata_get, 1);
    API_DEF_FUNC(hdata_get_var_offset, 2);
    API_DEF_FUNC(hdata_get_var_type_string, 2);
    API_DEF_FUNC(hdata_get_var_array_size, 3);
    API_DEF_FUNC(hdata_get_var_array_size_string, 3);
    API_DEF_FUNC(hdata_get_var_hdata, 2);
    API_DEF_FUNC(hdata_get_list, 2);
    API_DEF_FUNC(hdata_check_pointer, 3);
    API_DEF_FUNC(hdata_move, 3);
    API_DEF_FUNC(hdata_search, 7);
    API_DEF_FUNC(hdata_char, 3);
    API_DEF_FUNC(hdata_integer, 3);
    API_DEF_FUNC(hdata_long, 3);
    API_DEF_FUNC(hdata_longlong, 3);
    API_DEF_FUNC(hdata_string, 3);
    API_DEF_FUNC(hdata_pointer, 3);
    API_DEF_FUNC(hdata_time, 3);
    API_DEF_FUNC(hdata_hashtable, 3);
    API_DEF_FUNC(hdata_compare, 5);
    API_DEF_FUNC(hdata_update, 3);
    API_DEF_FUNC(hdata_get_string, 2);
    API_DEF_FUNC(upgrade_new, 3);
    API_DEF_FUNC(upgrade_write_object, 3);
    API_DEF_FUNC(upgrade_read, 1);
    API_DEF_FUNC(upgrade_close, 1);

    /* interface constants */
    for (i = 0; weechat_script_constants[i].name; i++)
    {
        snprintf (str_const, sizeof (str_const),
                  "weechat:%s", weechat_script_constants[i].name);
        scm_c_define (
            str_const,
            (weechat_script_constants[i].value_string) ?
            scm_from_locale_string (weechat_script_constants[i].value_string) :
            scm_from_int (weechat_script_constants[i].value_integer));
        scm_c_export (str_const, NULL);
    }
}
