/*
 * Copyright (C) 2011-2012 Sebastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * weechat-guile-api.c: guile API functions
 */

#undef _

#include <libguile.h>
#include <time.h>

#include "../weechat-plugin.h"
#include "../plugin-script.h"
#include "../plugin-script-api.h"
#include "../plugin-script-callback.h"
#include "weechat-guile.h"


#define API_FUNC(__init, __name, __ret)                                 \
    char *guile_function_name = __name;                                 \
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
#define API_RETURN_OK return SCM_BOOL_T;
#define API_RETURN_ERROR return SCM_BOOL_F;
#define API_RETURN_EMPTY                                                \
    return scm_from_locale_string("");
#define API_RETURN_STRING(__string)                                     \
    if (__string)                                                       \
        return scm_from_locale_string(__string);                        \
    return scm_from_locale_string("")
#define API_RETURN_STRING_FREE(__string)                                \
    if (__string)                                                       \
    {                                                                   \
        return_value = scm_from_locale_string (__string);               \
        free (__string);                                                \
        return return_value;                                            \
    }                                                                   \
    return scm_from_locale_string("")
#define API_RETURN_INT(__int)                                           \
    return scm_from_int (__int);
#define API_RETURN_LONG(__long)                                         \
    return scm_from_long (__long);

#define API_DEF_FUNC(__name, __argc)                                    \
    scm_c_define_gsubr ("weechat:" #__name, __argc, 0, 0,               \
                        &weechat_guile_api_##__name);                   \
    scm_c_export ("weechat:" #__name, NULL);


/*
 * weechat_guile_api_register: startup function for all WeeChat Guile scripts
 */

SCM
weechat_guile_api_register (SCM name, SCM author, SCM version, SCM license,
                            SCM description, SCM shutdown_func, SCM charset)
{
    API_FUNC(0, "register", API_RETURN_ERROR);
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

    if (plugin_script_search (weechat_guile_plugin, guile_scripts,
                              scm_i_string_chars (name)))
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
                                              &guile_scripts, &last_guile_script,
                                              (guile_current_script_filename) ?
                                              guile_current_script_filename : "",
                                              scm_i_string_chars (name),
                                              scm_i_string_chars (author),
                                              scm_i_string_chars (version),
                                              scm_i_string_chars (license),
                                              scm_i_string_chars (description),
                                              scm_i_string_chars (shutdown_func),
                                              scm_i_string_chars (charset));
    if (guile_current_script)
    {
        guile_registered_script = guile_current_script;
        if ((weechat_guile_plugin->debug >= 2) || !guile_quiet)
        {
            weechat_printf (NULL,
                            weechat_gettext ("%s: registered script \"%s\", "
                                             "version %s (%s)"),
                            GUILE_PLUGIN_NAME,
                            scm_i_string_chars (name),
                            scm_i_string_chars (version),
                            scm_i_string_chars (description));
        }
    }
    else
    {
        API_RETURN_ERROR;
    }

    API_RETURN_OK;
}

/*
 * weechat_guile_api_plugin_get_name: get name of plugin (return "core" for
 *                                    WeeChat core)
 */

SCM
weechat_guile_api_plugin_get_name (SCM plugin)
{
    const char *result;

    API_FUNC(1, "plugin_get_name", API_RETURN_EMPTY);
    if (!scm_is_string (plugin))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_plugin_get_name (API_STR2PTR(scm_i_string_chars (plugin)));

    API_RETURN_STRING(result);
}

/*
 * weechat_guile_api_charset_set: set script charset
 */

SCM
weechat_guile_api_charset_set (SCM charset)
{
    API_FUNC(1, "charset_set", API_RETURN_ERROR);
    if (!scm_is_string (charset))
        API_WRONG_ARGS(API_RETURN_ERROR);

    plugin_script_api_charset_set (guile_current_script, scm_i_string_chars (charset));

    API_RETURN_OK;
}

/*
 * weechat_guile_api_iconv_to_internal: convert string to internal WeeChat charset
 */

SCM
weechat_guile_api_iconv_to_internal (SCM charset, SCM string)
{
    char *result;
    SCM return_value;

    API_FUNC(1, "iconv_to_internal", API_RETURN_EMPTY);
    if (!scm_is_string (charset) || !scm_is_string (string))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_iconv_to_internal (scm_i_string_chars (charset),
                                        scm_i_string_chars (string));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_iconv_from_internal: convert string from WeeChat internal
 *                                        charset to another one
 */

SCM
weechat_guile_api_iconv_from_internal (SCM charset, SCM string)
{
    char *result;
    SCM return_value;

    API_FUNC(1, "iconv_from_internal", API_RETURN_EMPTY);
    if (!scm_is_string (charset) || !scm_is_string (string))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_iconv_from_internal (scm_i_string_chars (charset),
                                          scm_i_string_chars (string));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_gettext: get translated string
 */

SCM
weechat_guile_api_gettext (SCM string)
{
    const char *result;

    API_FUNC(1, "gettext", API_RETURN_EMPTY);
    if (!scm_is_string (string))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_gettext (scm_i_string_chars (string));

    API_RETURN_STRING(result);
}

/*
 * weechat_guile_api_ngettext: get translated string with plural form
 */

SCM
weechat_guile_api_ngettext (SCM single, SCM plural, SCM count)
{
    const char *result;

    API_FUNC(1, "ngettext", API_RETURN_EMPTY);
    if (!scm_is_string (single) || !scm_is_string (plural)
        || !scm_is_integer (count))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_ngettext (scm_i_string_chars (single),
                               scm_i_string_chars (plural),
                               scm_to_int (count));

    API_RETURN_STRING(result);
}

/*
 * weechat_guile_api_string_match: return 1 if string matches a mask
 *                                 mask can begin or end with "*", no other "*"
 *                                 are allowed inside mask
 */

SCM
weechat_guile_api_string_match (SCM string, SCM mask, SCM case_sensitive)
{
    int value;

    API_FUNC(1, "string_match", API_RETURN_INT(0));
    if (!scm_is_string (string) || !scm_is_string (mask)
        || !scm_is_integer (case_sensitive))
        API_WRONG_ARGS(API_RETURN_INT(0));

    value = weechat_string_match (scm_i_string_chars (string),
                                  scm_i_string_chars (mask),
                                  scm_to_int (case_sensitive));

    API_RETURN_INT(value);
}

/*
 * weechat_guile_api_string_has_highlight: return 1 if string contains a
 *                                         highlight (using list of words to
 *                                         highlight)
 *                                         return 0 if no highlight is found in
 *                                         string
 */

SCM
weechat_guile_api_string_has_highlight (SCM string, SCM highlight_words)
{
    int value;

    API_FUNC(1, "string_has_highlight", API_RETURN_INT(0));
    if (!scm_is_string (string) || !scm_is_string (highlight_words))
        API_WRONG_ARGS(API_RETURN_INT(0));

    value = weechat_string_has_highlight (scm_i_string_chars (string),
                                          scm_i_string_chars (highlight_words));

    API_RETURN_INT(value);
}

/*
 * weechat_guile_api_string_has_highlight_regex: return 1 if string contains a
 *                                               highlight (using regular
 *                                               expression)
 *                                               return 0 if no highlight is
 *                                               found in string
 */

SCM
weechat_guile_api_string_has_highlight_regex (SCM string, SCM regex)
{
    int value;

    API_FUNC(1, "string_has_highlight_regex", API_RETURN_INT(0));
    if (!scm_is_string (string) || !scm_is_string (regex))
        API_WRONG_ARGS(API_RETURN_INT(0));

    value = weechat_string_has_highlight_regex (scm_i_string_chars (string),
                                                scm_i_string_chars (regex));

    API_RETURN_INT(value);
}

/*
 * weechat_guile_api_string_mask_to_regex: convert a mask (string with only
 *                                         "*" as wildcard) to a regex, paying
 *                                         attention to special chars in a
 *                                         regex
 */

SCM
weechat_guile_api_string_mask_to_regex (SCM mask)
{
    char *result;
    SCM return_value;

    API_FUNC(1, "string_mask_to_regex", API_RETURN_EMPTY);
    if (!scm_is_string (mask))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_string_mask_to_regex (scm_i_string_chars (mask));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_string_remove_color: remove WeeChat color codes from string
 */

SCM
weechat_guile_api_string_remove_color (SCM string, SCM replacement)
{
    char *result;
    SCM return_value;

    API_FUNC(1, "string_remove_color", API_RETURN_EMPTY);
    if (!scm_is_string (string) || !scm_is_string (replacement))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_string_remove_color (scm_i_string_chars (string),
                                          scm_i_string_chars (replacement));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_string_is_command_char: check if first char of string is a
 *                                           command char
 */

SCM
weechat_guile_api_string_is_command_char (SCM string)
{
    int value;

    API_FUNC(1, "string_is_command_char", API_RETURN_INT(0));
    if (!scm_is_string (string))
        API_WRONG_ARGS(API_RETURN_INT(0));

    value = weechat_string_is_command_char (scm_i_string_chars (string));

    API_RETURN_INT(value);
}

/*
 * weechat_guile_api_string_input_for_buffer: return string with input text
 *                                           for buffer or empty string if
 *                                           it's a command
 */

SCM
weechat_guile_api_string_input_for_buffer (SCM string)
{
    const char *result;

    API_FUNC(1, "string_input_for_buffer", API_RETURN_EMPTY);
    if (!scm_is_string (string))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_string_input_for_buffer (scm_i_string_chars (string));

    API_RETURN_STRING(result);
}

/*
 * weechat_guile_api_mkdir_home: create a directory in WeeChat home
 */

SCM
weechat_guile_api_mkdir_home (SCM directory, SCM mode)
{
    API_FUNC(1, "mkdir_home", API_RETURN_ERROR);
    if (!scm_is_string (directory) || !scm_is_integer (mode))
        API_WRONG_ARGS(API_RETURN_ERROR);

    if (weechat_mkdir_home (scm_i_string_chars (directory), scm_to_int (mode)))
        API_RETURN_OK;

    API_RETURN_ERROR;
}

/*
 * weechat_guile_api_mkdir: create a directory
 */

SCM
weechat_guile_api_mkdir (SCM directory, SCM mode)
{
    API_FUNC(1, "mkdir", API_RETURN_ERROR);
    if (!scm_is_string (directory) || !scm_is_integer (mode))
        API_WRONG_ARGS(API_RETURN_ERROR);

    if (weechat_mkdir (scm_i_string_chars (directory), scm_to_int (mode)))
        API_RETURN_OK;

    API_RETURN_ERROR;
}

/*
 * weechat_guile_api_mkdir_parents: create a directory and make parent
 *                                  directories as needed
 */

SCM
weechat_guile_api_mkdir_parents (SCM directory, SCM mode)
{
    API_FUNC(1, "mkdir_parents", API_RETURN_ERROR);
    if (!scm_is_string (directory) || !scm_is_integer (mode))
        API_WRONG_ARGS(API_RETURN_ERROR);

    if (weechat_mkdir_parents (scm_i_string_chars (directory), scm_to_int (mode)))
        API_RETURN_OK;

    API_RETURN_ERROR;
}

/*
 * weechat_guile_api_list_new: create a new list
 */

SCM
weechat_guile_api_list_new ()
{
    char *result;

    API_FUNC(1, "list_new", API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_list_new ());

    API_RETURN_STRING(result);
}

/*
 * weechat_guile_api_list_add: add a string to list
 */

SCM
weechat_guile_api_list_add (SCM weelist, SCM data, SCM where, SCM user_data)
{
    char *result;

    API_FUNC(1, "list_add", API_RETURN_EMPTY);
    if (!scm_is_string (weelist) || !scm_is_string (data)
        || !scm_is_string (where) || !scm_is_string (user_data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_list_add (API_STR2PTR(scm_i_string_chars (weelist)),
                                           scm_i_string_chars (data),
                                           scm_i_string_chars (where),
                                           API_STR2PTR(scm_i_string_chars (user_data))));

    API_RETURN_STRING(result);
}

/*
 * weechat_guile_api_list_search: search a string in list
 */

SCM
weechat_guile_api_list_search (SCM weelist, SCM data)
{
    char *result;

    API_FUNC(1, "list_search", API_RETURN_EMPTY);
    if (!scm_is_string (weelist) || !scm_is_string (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_list_search (API_STR2PTR(scm_i_string_chars (weelist)),
                                              scm_i_string_chars (data)));

    API_RETURN_STRING(result);
}

/*
 * weechat_guile_api_list_search_pos: search position of a string in list
 */

SCM
weechat_guile_api_list_search_pos (SCM weelist, SCM data)
{
    int pos;

    API_FUNC(1, "list_search_pos", API_RETURN_INT(-1));
    if (!scm_is_string (weelist) || !scm_is_string (data))
        API_WRONG_ARGS(API_RETURN_INT(-1));

    pos = weechat_list_search_pos (API_STR2PTR(scm_i_string_chars (weelist)),
                                   scm_i_string_chars (data));

    API_RETURN_INT(pos);
}

/*
 * weechat_guile_api_list_casesearch: search a string in list (ignore case)
 */

SCM
weechat_guile_api_list_casesearch (SCM weelist, SCM data)
{
    char *result;

    API_FUNC(1, "list_casesearch", API_RETURN_EMPTY);
    if (!scm_is_string (weelist) || !scm_is_string (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_list_casesearch (API_STR2PTR(scm_i_string_chars (weelist)),
                                                  scm_i_string_chars (data)));

    API_RETURN_STRING(result);
}

/*
 * weechat_guile_api_list_casesearch_pos: search position of a string in list
 *                                        (ignore case)
 */

SCM
weechat_guile_api_list_casesearch_pos (SCM weelist, SCM data)
{
    int pos;

    API_FUNC(1, "list_casesearch_pos", API_RETURN_INT(-1));
    if (!scm_is_string (weelist) || !scm_is_string (data))
        API_WRONG_ARGS(API_RETURN_INT(-1));

    pos = weechat_list_casesearch_pos (API_STR2PTR(scm_i_string_chars (weelist)),
                                       scm_i_string_chars (data));

    API_RETURN_INT(pos);
}

/*
 * weechat_guile_api_list_get: get item by position
 */

SCM
weechat_guile_api_list_get (SCM weelist, SCM position)
{
    char *result;

    API_FUNC(1, "list_get", API_RETURN_EMPTY);
    if (!scm_is_string (weelist) || !scm_is_integer (position))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_list_get (API_STR2PTR(scm_i_string_chars (weelist)),
                                           scm_to_int (position)));

    API_RETURN_STRING(result);
}

/*
 * weechat_guile_api_list_set: set new value for item
 */

SCM
weechat_guile_api_list_set (SCM item, SCM new_value)
{
    API_FUNC(1, "list_set", API_RETURN_ERROR);
    if (!scm_is_string (item) || !scm_is_string (new_value))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_list_set (API_STR2PTR(scm_i_string_chars (item)),
                      scm_i_string_chars (new_value));

    API_RETURN_OK;
}

/*
 * weechat_guile_api_list_next: get next item
 */

SCM
weechat_guile_api_list_next (SCM item)
{
    char *result;

    API_FUNC(1, "list_next", API_RETURN_EMPTY);
    if (!scm_is_string (item))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_list_next (API_STR2PTR(scm_i_string_chars (item))));

    API_RETURN_STRING(result);
}

/*
 * weechat_guile_api_list_prev: get previous item
 */

SCM
weechat_guile_api_list_prev (SCM item)
{
    char *result;

    API_FUNC(1, "list_prev", API_RETURN_EMPTY);
    if (!scm_is_string (item))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_list_prev (API_STR2PTR(scm_i_string_chars (item))));

    API_RETURN_STRING(result);
}

/*
 * weechat_guile_api_list_string: get string value of item
 */

SCM
weechat_guile_api_list_string (SCM item)
{
    const char *result;

    API_FUNC(1, "list_string", API_RETURN_EMPTY);
    if (!scm_is_string (item))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_list_string (API_STR2PTR(scm_i_string_chars (item)));

    API_RETURN_STRING(result);
}

/*
 * weechat_guile_api_list_size: get number of elements in list
 */

SCM
weechat_guile_api_list_size (SCM weelist)
{
    int size;

    API_FUNC(1, "list_size", API_RETURN_INT(0));
    if (!scm_is_string (weelist))
        API_WRONG_ARGS(API_RETURN_INT(0));

    size = weechat_list_size (API_STR2PTR(scm_i_string_chars (weelist)));

    API_RETURN_INT(size);
}

/*
 * weechat_guile_api_list_remove: remove item from list
 */

SCM
weechat_guile_api_list_remove (SCM weelist, SCM item)
{
    API_FUNC(1, "list_remove", API_RETURN_ERROR);
    if (!scm_is_string (weelist) || !scm_is_string (item))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_list_remove (API_STR2PTR(scm_i_string_chars (weelist)),
                         API_STR2PTR(scm_i_string_chars (item)));

    API_RETURN_OK;
}

/*
 * weechat_guile_api_list_remove_all: remove all items from list
 */

SCM
weechat_guile_api_list_remove_all (SCM weelist)
{
    API_FUNC(1, "list_remove_all", API_RETURN_ERROR);
    if (!scm_is_string (weelist))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_list_remove_all (API_STR2PTR(scm_i_string_chars (weelist)));

    API_RETURN_OK;
}

/*
 * weechat_guile_api_list_free: free list
 */

SCM
weechat_guile_api_list_free (SCM weelist)
{
    API_FUNC(1, "list_free", API_RETURN_ERROR);
    if (!scm_is_string (weelist))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_list_free (API_STR2PTR(scm_i_string_chars (weelist)));

    API_RETURN_OK;
}

/*
 * weechat_guile_api_config_reload_cb: callback for config reload
 */

int
weechat_guile_api_config_reload_cb (void *data,
                                    struct t_config_file *config_file)
{
    struct t_plugin_script_cb *script_callback;
    void *func_argv[2];
    char empty_arg[1] = { '\0' };
    int *rc, ret;

    script_callback = (struct t_plugin_script_cb *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        func_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        func_argv[1] = API_PTR2STR(config_file);

        rc = (int *) weechat_guile_exec (script_callback->script,
                                         WEECHAT_SCRIPT_EXEC_INT,
                                         script_callback->function,
                                         "ss", func_argv);

        if (!rc)
            ret = WEECHAT_CONFIG_READ_FILE_NOT_FOUND;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (func_argv[1])
            free (func_argv[1]);

        return ret;
    }

    return WEECHAT_CONFIG_READ_FILE_NOT_FOUND;
}

/*
 * weechat_guile_api_config_new: create a new configuration file
 */

SCM
weechat_guile_api_config_new (SCM name, SCM function, SCM data)
{
    char *result;
    SCM return_value;

    API_FUNC(1, "config_new", API_RETURN_EMPTY);
    if (!scm_is_string (name) || !scm_is_string (function)
        || !scm_is_string (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(plugin_script_api_config_new (weechat_guile_plugin,
                                                       guile_current_script,
                                                       scm_i_string_chars (name),
                                                       &weechat_guile_api_config_reload_cb,
                                                       scm_i_string_chars (function),
                                                       scm_i_string_chars (data)));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_config_read_cb: callback for reading option in section
 */

int
weechat_guile_api_config_read_cb (void *data,
                                  struct t_config_file *config_file,
                                  struct t_config_section *section,
                                  const char *option_name, const char *value)
{
    struct t_plugin_script_cb *script_callback;
    void *func_argv[5];
    char empty_arg[1] = { '\0' };
    int *rc, ret;

    script_callback = (struct t_plugin_script_cb *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        func_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        func_argv[1] = API_PTR2STR(config_file);
        func_argv[2] = API_PTR2STR(section);
        func_argv[3] = (option_name) ? (char *)option_name : empty_arg;
        func_argv[4] = (value) ? (char *)value : empty_arg;

        rc = (int *) weechat_guile_exec (script_callback->script,
                                         WEECHAT_SCRIPT_EXEC_INT,
                                         script_callback->function,
                                         "sssss", func_argv);

        if (!rc)
            ret = WEECHAT_CONFIG_OPTION_SET_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (func_argv[1])
            free (func_argv[1]);
        if (func_argv[2])
            free (func_argv[2]);

        return ret;
    }

    return WEECHAT_CONFIG_OPTION_SET_ERROR;
}

/*
 * weechat_guile_api_config_section_write_cb: callback for writing section
 */

int
weechat_guile_api_config_section_write_cb (void *data,
                                           struct t_config_file *config_file,
                                           const char *section_name)
{
    struct t_plugin_script_cb *script_callback;
    void *func_argv[3];
    char empty_arg[1] = { '\0' };
    int *rc, ret;

    script_callback = (struct t_plugin_script_cb *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        func_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        func_argv[1] = API_PTR2STR(config_file);
        func_argv[2] = (section_name) ? (char *)section_name : empty_arg;

        rc = (int *) weechat_guile_exec (script_callback->script,
                                         WEECHAT_SCRIPT_EXEC_INT,
                                         script_callback->function,
                                         "sss", func_argv);

        if (!rc)
            ret = WEECHAT_CONFIG_WRITE_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (func_argv[1])
            free (func_argv[1]);

        return ret;
    }

    return WEECHAT_CONFIG_WRITE_ERROR;
}

/*
 * weechat_guile_api_config_section_write_default_cb: callback for writing
 *                                                    default values for section
 */

int
weechat_guile_api_config_section_write_default_cb (void *data,
                                                   struct t_config_file *config_file,
                                                   const char *section_name)
{
    struct t_plugin_script_cb *script_callback;
    void *func_argv[3];
    char empty_arg[1] = { '\0' };
    int *rc, ret;

    script_callback = (struct t_plugin_script_cb *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        func_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        func_argv[1] = API_PTR2STR(config_file);
        func_argv[2] = (section_name) ? (char *)section_name : empty_arg;

        rc = (int *) weechat_guile_exec (script_callback->script,
                                         WEECHAT_SCRIPT_EXEC_INT,
                                         script_callback->function,
                                         "sss", func_argv);

        if (!rc)
            ret = WEECHAT_CONFIG_WRITE_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (func_argv[1])
            free (func_argv[1]);

        return ret;
    }

    return WEECHAT_CONFIG_WRITE_ERROR;
}

/*
 * weechat_guile_api_config_section_create_option_cb: callback to create an
 *                                                    option
 */

int
weechat_guile_api_config_section_create_option_cb (void *data,
                                                   struct t_config_file *config_file,
                                                   struct t_config_section *section,
                                                   const char *option_name,
                                                   const char *value)
{
    struct t_plugin_script_cb *script_callback;
    void *func_argv[5];
    char empty_arg[1] = { '\0' };
    int *rc, ret;

    script_callback = (struct t_plugin_script_cb *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        func_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        func_argv[1] = API_PTR2STR(config_file);
        func_argv[2] = API_PTR2STR(section);
        func_argv[3] = (option_name) ? (char *)option_name : empty_arg;
        func_argv[4] = (value) ? (char *)value : empty_arg;

        rc = (int *) weechat_guile_exec (script_callback->script,
                                         WEECHAT_SCRIPT_EXEC_INT,
                                         script_callback->function,
                                         "sssss", func_argv);

        if (!rc)
            ret = WEECHAT_CONFIG_OPTION_SET_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (func_argv[1])
            free (func_argv[1]);
        if (func_argv[2])
            free (func_argv[2]);

        return ret;
    }

    return WEECHAT_CONFIG_OPTION_SET_ERROR;
}

/*
 * weechat_guile_api_config_section_delete_option_cb: callback to delete an
 *                                                    option
 */

int
weechat_guile_api_config_section_delete_option_cb (void *data,
                                                   struct t_config_file *config_file,
                                                   struct t_config_section *section,
                                                   struct t_config_option *option)
{
    struct t_plugin_script_cb *script_callback;
    void *func_argv[4];
    char empty_arg[1] = { '\0' };
    int *rc, ret;

    script_callback = (struct t_plugin_script_cb *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        func_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        func_argv[1] = API_PTR2STR(config_file);
        func_argv[2] = API_PTR2STR(section);
        func_argv[3] = API_PTR2STR(option);

        rc = (int *) weechat_guile_exec (script_callback->script,
                                         WEECHAT_SCRIPT_EXEC_INT,
                                         script_callback->function,
                                         "ssss", func_argv);

        if (!rc)
            ret = WEECHAT_CONFIG_OPTION_UNSET_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (func_argv[1])
            free (func_argv[1]);
        if (func_argv[2])
            free (func_argv[2]);
        if (func_argv[3])
            free (func_argv[3]);

        return ret;
    }

    return WEECHAT_CONFIG_OPTION_UNSET_ERROR;
}

/*
 * weechat_guile_api_config_new_section: create a new section in configuration file
 */

SCM
weechat_guile_api_config_new_section (SCM args)
{
    SCM config_file, name, user_can_add_options, user_can_delete_options;
    SCM function_read, data_read, function_write, data_write;
    SCM function_write_default, data_write_default, function_create_option;
    SCM data_create_option, function_delete_option, data_delete_option;
    char *result;
    SCM return_value;

    API_FUNC(1, "config_new_section", API_RETURN_EMPTY);
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

    result = API_PTR2STR(plugin_script_api_config_new_section (weechat_guile_plugin,
                                                               guile_current_script,
                                                               API_STR2PTR(scm_i_string_chars (config_file)),
                                                               scm_i_string_chars (name),
                                                               scm_to_int (user_can_add_options),
                                                               scm_to_int (user_can_delete_options),
                                                               &weechat_guile_api_config_read_cb,
                                                               scm_i_string_chars (function_read),
                                                               scm_i_string_chars (data_read),
                                                               &weechat_guile_api_config_section_write_cb,
                                                               scm_i_string_chars (function_write),
                                                               scm_i_string_chars (data_write),
                                                               &weechat_guile_api_config_section_write_default_cb,
                                                               scm_i_string_chars (function_write_default),
                                                               scm_i_string_chars (data_write_default),
                                                               &weechat_guile_api_config_section_create_option_cb,
                                                               scm_i_string_chars (function_create_option),
                                                               scm_i_string_chars (data_create_option),
                                                               &weechat_guile_api_config_section_delete_option_cb,
                                                               scm_i_string_chars (function_delete_option),
                                                               scm_i_string_chars (data_delete_option)));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_config_search_section: search section in configuration file
 */

SCM
weechat_guile_api_config_search_section (SCM config_file, SCM section_name)
{
    char *result;
    SCM return_value;

    API_FUNC(1, "config_search_section", API_RETURN_EMPTY);
    if (!scm_is_string (config_file) || !scm_is_string (section_name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_config_search_section (API_STR2PTR(scm_i_string_chars (config_file)),
                                                        scm_i_string_chars (section_name)));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_config_option_check_value_cb: callback for checking new
 *                                                 value for option
 */

int
weechat_guile_api_config_option_check_value_cb (void *data,
                                                struct t_config_option *option,
                                                const char *value)
{
    struct t_plugin_script_cb *script_callback;
    void *func_argv[3];
    char empty_arg[1] = { '\0' };
    int *rc, ret;

    script_callback = (struct t_plugin_script_cb *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        func_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        func_argv[1] = API_PTR2STR(option);
        func_argv[2] = (value) ? (char *)value : empty_arg;

        rc = (int *) weechat_guile_exec (script_callback->script,
                                         WEECHAT_SCRIPT_EXEC_INT,
                                         script_callback->function,
                                         "sss", func_argv);

        if (!rc)
            ret = 0;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (func_argv[1])
            free (func_argv[1]);

        return ret;
    }

    return 0;
}

/*
 * weechat_guile_api_config_option_change_cb: callback for option changed
 */

void
weechat_guile_api_config_option_change_cb (void *data,
                                           struct t_config_option *option)
{
    struct t_plugin_script_cb *script_callback;
    void *func_argv[2];
    char empty_arg[1] = { '\0' };
    int *rc;

    script_callback = (struct t_plugin_script_cb *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        func_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        func_argv[1] = API_PTR2STR(option);

        rc = (int *) weechat_guile_exec (script_callback->script,
                                         WEECHAT_SCRIPT_EXEC_INT,
                                         script_callback->function,
                                         "ss", func_argv);

        if (func_argv[1])
            free (func_argv[1]);

        if (rc)
            free (rc);
    }
}

/*
 * weechat_guile_api_config_option_delete_cb: callback when option is deleted
 */

void
weechat_guile_api_config_option_delete_cb (void *data,
                                           struct t_config_option *option)
{
    struct t_plugin_script_cb *script_callback;
    void *func_argv[2];
    char empty_arg[1] = { '\0' };
    int *rc;

    script_callback = (struct t_plugin_script_cb *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        func_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        func_argv[1] = API_PTR2STR(option);

        rc = (int *) weechat_guile_exec (script_callback->script,
                                         WEECHAT_SCRIPT_EXEC_INT,
                                         script_callback->function,
                                         "ss", func_argv);

        if (func_argv[1])
            free (func_argv[1]);

        if (rc)
            free (rc);
    }
}

/*
 * weechat_guile_api_config_new_option: create a new option in section
 */

SCM
weechat_guile_api_config_new_option (SCM args)
{
    SCM config_file, section, name, type, description, string_values, min;
    SCM max, default_value, value, null_value_allowed, function_check_value;
    SCM data_check_value, function_change, data_change, function_delete;
    SCM data_delete;
    char *result;
    SCM return_value;

    API_FUNC(1, "config_new_option", API_RETURN_EMPTY);
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
        || !scm_is_string (default_value) || !scm_is_string (value)
        || !scm_is_integer (null_value_allowed)
        || !scm_is_string (function_check_value)
        || !scm_is_string (data_check_value)
        || !scm_is_string (function_change) || !scm_is_string (data_change)
        || !scm_is_string (function_delete) || !scm_is_string (data_delete))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(plugin_script_api_config_new_option (weechat_guile_plugin,
                                                              guile_current_script,
                                                              API_STR2PTR(scm_i_string_chars (config_file)),
                                                              API_STR2PTR(scm_i_string_chars (section)),
                                                              scm_i_string_chars (name),
                                                              scm_i_string_chars (type),
                                                              scm_i_string_chars (description),
                                                              scm_i_string_chars (string_values),
                                                              scm_to_int (min),
                                                              scm_to_int (max),
                                                              scm_i_string_chars (default_value),
                                                              scm_i_string_chars (value),
                                                              scm_to_int (null_value_allowed),
                                                              &weechat_guile_api_config_option_check_value_cb,
                                                              scm_i_string_chars (function_check_value),
                                                              scm_i_string_chars (data_check_value),
                                                              &weechat_guile_api_config_option_change_cb,
                                                              scm_i_string_chars (function_change),
                                                              scm_i_string_chars (data_change),
                                                              &weechat_guile_api_config_option_delete_cb,
                                                              scm_i_string_chars (function_delete),
                                                              scm_i_string_chars (data_delete)));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_config_search_option: search option in configuration file
 *                                         or section
 */

SCM
weechat_guile_api_config_search_option (SCM config_file, SCM section,
                                        SCM option_name)
{
    char *result;
    SCM return_value;

    API_FUNC(1, "config_search_option", API_RETURN_EMPTY);
    if (!scm_is_string (config_file) || !scm_is_string (section)
        || !scm_is_string (option_name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_config_search_option (API_STR2PTR(scm_i_string_chars (config_file)),
                                                       API_STR2PTR(scm_i_string_chars (section)),
                                                       scm_i_string_chars (option_name)));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_config_string_to_boolean: return boolean value of a string
 */

SCM
weechat_guile_api_config_string_to_boolean (SCM text)
{
    int value;

    API_FUNC(1, "config_string_to_boolean", API_RETURN_INT(0));
    if (!scm_is_string (text))
        API_WRONG_ARGS(API_RETURN_INT(0));

    value = weechat_config_string_to_boolean (scm_i_string_chars (text));

    API_RETURN_INT(value);
}

/*
 * weechat_guile_api_config_option_reset: reset option with default value
 */

SCM
weechat_guile_api_config_option_reset (SCM option, SCM run_callback)
{
    int rc;

    API_FUNC(1, "config_option_reset", API_RETURN_INT(0));
    if (!scm_is_string (option) || !scm_is_integer (run_callback))
        API_WRONG_ARGS(API_RETURN_INT(0));

    rc = weechat_config_option_reset (API_STR2PTR(scm_i_string_chars (option)),
                                      scm_to_int (run_callback));

    API_RETURN_INT(rc);
}

/*
 * weechat_guile_api_config_option_set: set new value for option
 */

SCM
weechat_guile_api_config_option_set (SCM option, SCM new_value,
                                     SCM run_callback)
{
    int rc;

    API_FUNC(1, "config_option_set", API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));
    if (!scm_is_string (option) || !scm_is_string (new_value)
        || !scm_is_integer (run_callback))
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));

    rc = weechat_config_option_set (API_STR2PTR(scm_i_string_chars (option)),
                                    scm_i_string_chars (new_value),
                                    scm_to_int (run_callback));

    API_RETURN_INT(rc);
}

/*
 * weechat_guile_api_config_option_set_null: set null (undefined) value for
 *                                           option
 */

SCM
weechat_guile_api_config_option_set_null (SCM option, SCM run_callback)
{
    int rc;

    API_FUNC(1, "config_option_set_null", API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));
    if (!scm_is_string (option) || !scm_is_integer (run_callback))
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));

    rc = weechat_config_option_set_null (API_STR2PTR(scm_i_string_chars (option)),
                                         scm_to_int (run_callback));

    API_RETURN_INT(rc);
}

/*
 * weechat_guile_api_config_option_unset: unset an option
 */

SCM
weechat_guile_api_config_option_unset (SCM option)
{
    int rc;

    API_FUNC(1, "config_option_unset", API_RETURN_INT(WEECHAT_CONFIG_OPTION_UNSET_ERROR));
    if (!scm_is_string (option))
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_OPTION_UNSET_ERROR));

    rc = weechat_config_option_unset (API_STR2PTR(scm_i_string_chars (option)));

    API_RETURN_INT(rc);
}

/*
 * weechat_guile_api_config_option_rename: rename an option
 */

SCM
weechat_guile_api_config_option_rename (SCM option, SCM new_name)
{
    API_FUNC(1, "config_option_rename", API_RETURN_ERROR);
    if (!scm_is_string (option) || !scm_is_string (new_name))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_config_option_rename (API_STR2PTR(scm_i_string_chars (option)),
                                  scm_i_string_chars (new_name));

    API_RETURN_OK;
}

/*
 * weechat_guile_api_config_option_is_null: return 1 if value of option is null
 */

SCM
weechat_guile_api_config_option_is_null (SCM option)
{
    int value;

    API_FUNC(1, "config_option_is_null", API_RETURN_INT(1));
    if (!scm_is_string (option))
        API_WRONG_ARGS(API_RETURN_INT(1));

    value = weechat_config_option_is_null (API_STR2PTR(scm_i_string_chars (option)));

    API_RETURN_INT(value);
}

/*
 * weechat_guile_api_config_option_default_is_null: return 1 if value of option
 *                                                  is null
 */

SCM
weechat_guile_api_config_option_default_is_null (SCM option)
{
    int value;

    API_FUNC(1, "config_option_default_is_null", API_RETURN_INT(1));
    if (!scm_is_string (option))
        API_WRONG_ARGS(API_RETURN_INT(1));

    value = weechat_config_option_default_is_null (API_STR2PTR(scm_i_string_chars (option)));

    API_RETURN_INT(value);
}

/*
 * weechat_guile_api_config_boolean: return boolean value of option
 */

SCM
weechat_guile_api_config_boolean (SCM option)
{
    int value;

    API_FUNC(1, "config_boolean", API_RETURN_INT(0));
    if (!scm_is_string (option))
        API_WRONG_ARGS(API_RETURN_INT(0));

    value = weechat_config_boolean (API_STR2PTR(scm_i_string_chars (option)));

    API_RETURN_INT(value);
}

/*
 * weechat_guile_api_config_boolean_default: return default boolean value of option
 */

SCM
weechat_guile_api_config_boolean_default (SCM option)
{
    int value;

    API_FUNC(1, "config_boolean_default", API_RETURN_INT(0));
    if (!scm_is_string (option))
        API_WRONG_ARGS(API_RETURN_INT(0));

    value = weechat_config_boolean_default (API_STR2PTR(scm_i_string_chars (option)));

    API_RETURN_INT(value);
}

/*
 * weechat_guile_api_config_integer: return integer value of option
 */

SCM
weechat_guile_api_config_integer (SCM option)
{
    int value;

    API_FUNC(1, "config_integer", API_RETURN_INT(0));
    if (!scm_is_string (option))
        API_WRONG_ARGS(API_RETURN_INT(0));

    value = weechat_config_integer (API_STR2PTR(scm_i_string_chars (option)));

    API_RETURN_INT(value);
}

/*
 * weechat_guile_api_config_integer_default: return default integer value of option
 */

SCM
weechat_guile_api_config_integer_default (SCM option)
{
    int value;

    API_FUNC(1, "config_integer_default", API_RETURN_INT(0));
    if (!scm_is_string (option))
        API_WRONG_ARGS(API_RETURN_INT(0));

    value = weechat_config_integer_default (API_STR2PTR(scm_i_string_chars (option)));

    API_RETURN_INT(value);
}

/*
 * weechat_guile_api_config_string: return string value of option
 */

SCM
weechat_guile_api_config_string (SCM option)
{
    const char *result;

    API_FUNC(1, "config_string", API_RETURN_EMPTY);
    if (!scm_is_string (option))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_config_string (API_STR2PTR(scm_i_string_chars (option)));

    API_RETURN_STRING(result);
}

/*
 * weechat_guile_api_config_string_default: return default string value of option
 */

SCM
weechat_guile_api_config_string_default (SCM option)
{
    const char *result;

    API_FUNC(1, "config_string_default", API_RETURN_EMPTY);
    if (!scm_is_string (option))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_config_string_default (API_STR2PTR(scm_i_string_chars (option)));

    API_RETURN_STRING(result);
}

/*
 * weechat_guile_api_config_color: return color value of option
 */

SCM
weechat_guile_api_config_color (SCM option)
{
    const char *result;

    API_FUNC(1, "config_color", API_RETURN_INT(0));
    if (!scm_is_string (option))
        API_WRONG_ARGS(API_RETURN_INT(0));

    result = weechat_config_color (API_STR2PTR(scm_i_string_chars (option)));

    API_RETURN_STRING(result);
}

/*
 * weechat_guile_api_config_color_default: return default color value of option
 */

SCM
weechat_guile_api_config_color_default (SCM option)
{
    const char *result;

    API_FUNC(1, "config_color_default", API_RETURN_INT(0));
    if (!scm_is_string (option))
        API_WRONG_ARGS(API_RETURN_INT(0));

    result = weechat_config_color_default (API_STR2PTR(scm_i_string_chars (option)));

    API_RETURN_STRING(result);
}

/*
 * weechat_guile_api_config_write_option: write an option in configuration file
 */

SCM
weechat_guile_api_config_write_option (SCM config_file, SCM option)
{
    API_FUNC(1, "config_write_option", API_RETURN_ERROR);
    if (!scm_is_string (config_file) || !scm_is_string (option))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_config_write_option (API_STR2PTR(scm_i_string_chars (config_file)),
                                 API_STR2PTR(scm_i_string_chars (option)));

    API_RETURN_OK;
}

/*
 * weechat_guile_api_config_write_line: write a line in configuration file
 */

SCM
weechat_guile_api_config_write_line (SCM config_file,
                                    SCM option_name, SCM value)
{
    API_FUNC(1, "config_write_line", API_RETURN_ERROR);
    if (!scm_is_string (config_file) || !scm_is_string (option_name) || !scm_is_string (value))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_config_write_line (API_STR2PTR(scm_i_string_chars (config_file)),
                               scm_i_string_chars (option_name),
                               "%s",
                               scm_i_string_chars (value));

    API_RETURN_OK;
}

/*
 * weechat_guile_api_config_write: write configuration file
 */

SCM
weechat_guile_api_config_write (SCM config_file)
{
    int rc;

    API_FUNC(1, "config_write", API_RETURN_INT(-1));
    if (!scm_is_string (config_file))
        API_WRONG_ARGS(API_RETURN_INT(-1));

    rc = weechat_config_write (API_STR2PTR(scm_i_string_chars (config_file)));

    API_RETURN_INT(rc);
}

/*
 * weechat_guile_api_config_read: read configuration file
 */

SCM
weechat_guile_api_config_read (SCM config_file)
{
    int rc;

    API_FUNC(1, "config_read", API_RETURN_INT(-1));
    if (!scm_is_string (config_file))
        API_WRONG_ARGS(API_RETURN_INT(-1));

    rc = weechat_config_read (API_STR2PTR(scm_i_string_chars (config_file)));

    API_RETURN_INT(rc);
}

/*
 * weechat_guile_api_config_reload: reload configuration file
 */

SCM
weechat_guile_api_config_reload (SCM config_file)
{
    int rc;

    API_FUNC(1, "config_reload", API_RETURN_INT(-1));
    if (!scm_is_string (config_file))
        API_WRONG_ARGS(API_RETURN_INT(-1));

    rc = weechat_config_reload (API_STR2PTR(scm_i_string_chars (config_file)));

    API_RETURN_INT(rc);
}

/*
 * weechat_guile_api_config_option_free: free an option in configuration file
 */

SCM
weechat_guile_api_config_option_free (SCM option)
{
    API_FUNC(1, "config_option_free", API_RETURN_ERROR);
    if (!scm_is_string (option))
        API_WRONG_ARGS(API_RETURN_ERROR);

    plugin_script_api_config_option_free (weechat_guile_plugin,
                                          guile_current_script,
                                          API_STR2PTR(scm_i_string_chars (option)));

    API_RETURN_OK;
}

/*
 * weechat_guile_api_config_section_free_options: free all options of a section
 *                                               in configuration file
 */

SCM
weechat_guile_api_config_section_free_options (SCM section)
{
    API_FUNC(1, "config_section_free_options", API_RETURN_ERROR);
    if (!scm_is_string (section))
        API_WRONG_ARGS(API_RETURN_ERROR);

    plugin_script_api_config_section_free_options (weechat_guile_plugin,
                                                   guile_current_script,
                                                   API_STR2PTR(scm_i_string_chars (section)));

    API_RETURN_OK;
}

/*
 * weechat_guile_api_config_section_free: free section in configuration file
 */

SCM
weechat_guile_api_config_section_free (SCM section)
{
    API_FUNC(1, "config_section_free", API_RETURN_ERROR);
    if (!scm_is_string (section))
        API_WRONG_ARGS(API_RETURN_ERROR);

    plugin_script_api_config_section_free (weechat_guile_plugin,
                                           guile_current_script,
                                           API_STR2PTR(scm_i_string_chars (section)));

    API_RETURN_OK;
}

/*
 * weechat_guile_api_config_free: free configuration file
 */

SCM
weechat_guile_api_config_free (SCM config_file)
{
    API_FUNC(1, "config_free", API_RETURN_ERROR);
    if (!scm_is_string (config_file))
        API_WRONG_ARGS(API_RETURN_ERROR);

    plugin_script_api_config_free (weechat_guile_plugin,
                                   guile_current_script,
                                   API_STR2PTR(scm_i_string_chars (config_file)));

    API_RETURN_OK;
}

/*
 * weechat_guile_api_config_get: get config option
 */

SCM
weechat_guile_api_config_get (SCM option)
{
    char *result;
    SCM return_value;

    API_FUNC(1, "config_get", API_RETURN_EMPTY);
    if (!scm_is_string (option))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_config_get (scm_i_string_chars (option)));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_config_get_plugin: get value of a plugin option
 */

SCM
weechat_guile_api_config_get_plugin (SCM option)
{
    const char *result;

    API_FUNC(1, "config_get_plugin", API_RETURN_EMPTY);
    if (!scm_is_string (option))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = plugin_script_api_config_get_plugin (weechat_guile_plugin,
                                                  guile_current_script,
                                                  scm_i_string_chars (option));

    API_RETURN_STRING(result);
}

/*
 * weechat_guile_api_config_is_set_plugin: check if a plugin option is set
 */

SCM
weechat_guile_api_config_is_set_plugin (SCM option)
{
    int rc;

    API_FUNC(1, "config_is_set_plugin", API_RETURN_INT(0));
    if (!scm_is_string (option))
        API_WRONG_ARGS(API_RETURN_INT(0));

    rc = plugin_script_api_config_is_set_plugin (weechat_guile_plugin,
                                                 guile_current_script,
                                                 scm_i_string_chars (option));

    API_RETURN_INT(rc);
}

/*
 * weechat_guile_api_config_set_plugin: set value of a plugin option
 */

SCM
weechat_guile_api_config_set_plugin (SCM option, SCM value)
{
    int rc;

    API_FUNC(1, "config_set_plugin", API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));
    if (!scm_is_string (option) || !scm_is_string (value))
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));

    rc = plugin_script_api_config_set_plugin (weechat_guile_plugin,
                                              guile_current_script,
                                              scm_i_string_chars (option),
                                              scm_i_string_chars (value));

    API_RETURN_INT(rc);
}

/*
 * weechat_guile_api_config_set_desc_plugin: set description of a plugin option
 */

SCM
weechat_guile_api_config_set_desc_plugin (SCM option, SCM description)
{
    API_FUNC(1, "config_set_desc_plugin", API_RETURN_ERROR);
    if (!scm_is_string (option) || !scm_is_string (description))
        API_WRONG_ARGS(API_RETURN_ERROR);

    plugin_script_api_config_set_desc_plugin (weechat_guile_plugin,
                                              guile_current_script,
                                              scm_i_string_chars (option),
                                              scm_i_string_chars (description));

    API_RETURN_OK;
}

/*
 * weechat_guile_api_config_unset_plugin: unset plugin option
 */

SCM
weechat_guile_api_config_unset_plugin (SCM option)
{
    int rc;

    API_FUNC(1, "config_unset_plugin", API_RETURN_INT(WEECHAT_CONFIG_OPTION_UNSET_ERROR));
    if (!scm_is_string (option))
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_OPTION_UNSET_ERROR));

    rc = plugin_script_api_config_unset_plugin (weechat_guile_plugin,
                                                guile_current_script,
                                                scm_i_string_chars (option));

    API_RETURN_INT(rc);
}

/*
 * weechat_guile_api_key_bind: bind key(s)
 */

SCM
weechat_guile_api_key_bind (SCM context, SCM keys)
{
    struct t_hashtable *c_keys;
    int num_keys;

    API_FUNC(1, "key_bind", API_RETURN_INT(0));
    if (!scm_is_string (context) || !scm_list_p (keys))
        API_WRONG_ARGS(API_RETURN_INT(0));

    c_keys = weechat_guile_alist_to_hashtable (keys,
                                               WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE);

    num_keys = weechat_key_bind (scm_i_string_chars (context), c_keys);

    if (c_keys)
        weechat_hashtable_free (c_keys);

    API_RETURN_INT(num_keys);
}

/*
 * weechat_guile_api_key_unbind: unbind key(s)
 */

SCM
weechat_guile_api_key_unbind (SCM context, SCM key)
{
    int num_keys;

    API_FUNC(1, "key_unbind", API_RETURN_INT(0));
    if (!scm_is_string (context) || !scm_is_string (key))
        API_WRONG_ARGS(API_RETURN_INT(0));

    num_keys = weechat_key_unbind (scm_i_string_chars (context),
                                   scm_i_string_chars (key));

    API_RETURN_INT(num_keys);
}

/*
 * weechat_guile_api_prefix: get a prefix, used for display
 */

SCM
weechat_guile_api_prefix (SCM prefix)
{
    const char *result;

    API_FUNC(0, "prefix", API_RETURN_EMPTY);
    if (!scm_is_string (prefix))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_prefix (scm_i_string_chars (prefix));

    API_RETURN_STRING(result);
}

/*
 * weechat_guile_api_color: get a color code, used for display
 */

SCM
weechat_guile_api_color (SCM color)
{
    const char *result;

    API_FUNC(0, "color", API_RETURN_EMPTY);
    if (!scm_is_string (color))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_color (scm_i_string_chars (color));

    API_RETURN_STRING(result);
}

/*
 * weechat_guile_api_print: print message in a buffer
 */

SCM
weechat_guile_api_print (SCM buffer, SCM message)
{
    API_FUNC(0, "print", API_RETURN_ERROR);
    if (!scm_is_string (buffer) || !scm_is_string (message))
        API_WRONG_ARGS(API_RETURN_ERROR);

    plugin_script_api_printf (weechat_guile_plugin,
                              guile_current_script,
                              API_STR2PTR(scm_i_string_chars (buffer)),
                              "%s", scm_i_string_chars (message));

    API_RETURN_OK;
}

/*
 * weechat_guile_api_print_date_tags: print message in a buffer with optional
 *                                   date and tags
 */

SCM
weechat_guile_api_print_date_tags (SCM buffer, SCM date, SCM tags, SCM message)
{
    API_FUNC(1, "print_date_tags", API_RETURN_ERROR);
    if (!scm_is_string (buffer) || !scm_is_integer (date)
        || !scm_is_string (tags) || !scm_is_string (message))
        API_WRONG_ARGS(API_RETURN_ERROR);

    plugin_script_api_printf_date_tags (weechat_guile_plugin,
                                        guile_current_script,
                                        API_STR2PTR(scm_i_string_chars (buffer)),
                                        scm_to_int (date),
                                        scm_i_string_chars (tags),
                                        "%s", scm_i_string_chars (message));

    API_RETURN_OK;
}

/*
 * weechat_guile_api_print_y: print message in a buffer with free content
 */

SCM
weechat_guile_api_print_y (SCM buffer, SCM y, SCM message)
{
    API_FUNC(1, "print_y", API_RETURN_ERROR);
    if (!scm_is_string (buffer) || !scm_is_integer (y)
        || !scm_is_string (message))
        API_WRONG_ARGS(API_RETURN_ERROR);

    plugin_script_api_printf_y (weechat_guile_plugin,
                                guile_current_script,
                                API_STR2PTR(scm_i_string_chars (buffer)),
                                scm_to_int (y),
                                "%s", scm_i_string_chars (message));

    API_RETURN_OK;
}

/*
 * weechat_guile_api_log_print: print message in WeeChat log file
 */

SCM
weechat_guile_api_log_print (SCM message)
{
    API_FUNC(1, "log_print", API_RETURN_ERROR);
    if (!scm_is_string (message))
        API_WRONG_ARGS(API_RETURN_ERROR);

    plugin_script_api_log_printf (weechat_guile_plugin,
                                  guile_current_script,
                                  "%s", scm_i_string_chars (message));

    API_RETURN_OK;
}

/*
 * weechat_guile_api_hook_command_cb: callback for command hooked
 */

int
weechat_guile_api_hook_command_cb (void *data, struct t_gui_buffer *buffer,
                                   int argc, char **argv, char **argv_eol)
{
    struct t_plugin_script_cb *script_callback;
    void *func_argv[3];
    char empty_arg[1] = { '\0' };
    int *rc, ret;

    /* make C compiler happy */
    (void) argv;

    script_callback = (struct t_plugin_script_cb *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        func_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        func_argv[1] = API_PTR2STR(buffer);
        func_argv[2] = (argc > 1) ? argv_eol[1] : empty_arg;

        rc = (int *) weechat_guile_exec (script_callback->script,
                                         WEECHAT_SCRIPT_EXEC_INT,
                                         script_callback->function,
                                         "sss", func_argv);

        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (func_argv[1])
            free (func_argv[1]);

        return ret;
    }

    return WEECHAT_RC_ERROR;
}

/*
 * weechat_guile_api_hook_command: hook a command
 */

SCM
weechat_guile_api_hook_command (SCM command, SCM description, SCM args,
                                SCM args_description, SCM completion,
                                SCM function, SCM data)
{
    char *result;
    SCM return_value;

    API_FUNC(1, "hook_command", API_RETURN_EMPTY);
    if (!scm_is_string (command) || !scm_is_string (description)
        || !scm_is_string (args) || !scm_is_string (args_description)
        || !scm_is_string (completion) || !scm_is_string (function)
        || !scm_is_string (data))
        API_WRONG_ARGS(API_RETURN_ERROR);

    result = API_PTR2STR(plugin_script_api_hook_command (weechat_guile_plugin,
                                                         guile_current_script,
                                                         scm_i_string_chars (command),
                                                         scm_i_string_chars (description),
                                                         scm_i_string_chars (args),
                                                         scm_i_string_chars (args_description),
                                                         scm_i_string_chars (completion),
                                                         &weechat_guile_api_hook_command_cb,
                                                         scm_i_string_chars (function),
                                                         scm_i_string_chars (data)));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_hook_command_run_cb: callback for command_run hooked
 */

int
weechat_guile_api_hook_command_run_cb (void *data, struct t_gui_buffer *buffer,
                                       const char *command)
{
    struct t_plugin_script_cb *script_callback;
    void *func_argv[3];
    char empty_arg[1] = { '\0' };
    int *rc, ret;

    script_callback = (struct t_plugin_script_cb *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        func_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        func_argv[1] = API_PTR2STR(buffer);
        func_argv[2] = (command) ? (char *)command : empty_arg;

        rc = (int *) weechat_guile_exec (script_callback->script,
                                         WEECHAT_SCRIPT_EXEC_INT,
                                         script_callback->function,
                                         "sss", func_argv);

        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (func_argv[1])
            free (func_argv[1]);

        return ret;
    }

    return WEECHAT_RC_ERROR;
}

/*
 * weechat_guile_api_hook_command_run: hook a command_run
 */

SCM
weechat_guile_api_hook_command_run (SCM command, SCM function, SCM data)
{
    char *result;
    SCM return_value;

    API_FUNC(1, "hook_command_run", API_RETURN_EMPTY);
    if (!scm_is_string (command) || !scm_is_string (function)
        || !scm_is_string (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(plugin_script_api_hook_command_run (weechat_guile_plugin,
                                                             guile_current_script,
                                                             scm_i_string_chars (command),
                                                             &weechat_guile_api_hook_command_run_cb,
                                                             scm_i_string_chars (function),
                                                             scm_i_string_chars (data)));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_hook_timer_cb: callback for timer hooked
 */

int
weechat_guile_api_hook_timer_cb (void *data, int remaining_calls)
{
    struct t_plugin_script_cb *script_callback;
    void *func_argv[2];
    char str_remaining_calls[32], empty_arg[1] = { '\0' };
    int *rc, ret;

    script_callback = (struct t_plugin_script_cb *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        snprintf (str_remaining_calls, sizeof (str_remaining_calls),
                  "%d", remaining_calls);

        func_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        func_argv[1] = str_remaining_calls;

        rc = (int *) weechat_guile_exec (script_callback->script,
                                         WEECHAT_SCRIPT_EXEC_INT,
                                         script_callback->function,
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

/*
 * weechat_guile_api_hook_timer: hook a timer
 */

SCM
weechat_guile_api_hook_timer (SCM interval, SCM align_second, SCM max_calls,
                              SCM function, SCM data)
{
    char *result;
    SCM return_value;

    API_FUNC(1, "hook_timer", API_RETURN_EMPTY);
    if (!scm_is_integer (interval) || !scm_is_integer (align_second)
        || !scm_is_integer (max_calls) || !scm_is_string (function)
        || !scm_is_string (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(plugin_script_api_hook_timer (weechat_guile_plugin,
                                                       guile_current_script,
                                                       scm_to_int (interval),
                                                       scm_to_int (align_second),
                                                       scm_to_int (max_calls),
                                                       &weechat_guile_api_hook_timer_cb,
                                                       scm_i_string_chars (function),
                                                       scm_i_string_chars (data)));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_hook_fd_cb: callback for fd hooked
 */

int
weechat_guile_api_hook_fd_cb (void *data, int fd)
{
    struct t_plugin_script_cb *script_callback;
    void *func_argv[2];
    char str_fd[32], empty_arg[1] = { '\0' };
    int *rc, ret;

    script_callback = (struct t_plugin_script_cb *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        snprintf (str_fd, sizeof (str_fd), "%d", fd);

        func_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        func_argv[1] = str_fd;

        rc = (int *) weechat_guile_exec (script_callback->script,
                                         WEECHAT_SCRIPT_EXEC_INT,
                                         script_callback->function,
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

/*
 * weechat_guile_api_hook_fd: hook a fd
 */

SCM
weechat_guile_api_hook_fd (SCM fd, SCM read, SCM write, SCM exception,
                           SCM function, SCM data)
{
    char *result;
    SCM return_value;

    API_FUNC(1, "hook_fd", API_RETURN_EMPTY);
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
                                                    scm_i_string_chars (function),
                                                    scm_i_string_chars (data)));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_hook_process_cb: callback for process hooked
 */

int
weechat_guile_api_hook_process_cb (void *data,
                                   const char *command, int return_code,
                                   const char *out, const char *err)
{
    struct t_plugin_script_cb *script_callback;
    void *func_argv[5];
    char empty_arg[1] = { '\0' };
    int *rc, ret;

    script_callback = (struct t_plugin_script_cb *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        func_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        func_argv[1] = (command) ? (char *)command : empty_arg;
        func_argv[2] = &return_code;
        func_argv[3] = (out) ? (char *)out : empty_arg;
        func_argv[4] = (err) ? (char *)err : empty_arg;

        rc = (int *) weechat_guile_exec (script_callback->script,
                                         WEECHAT_SCRIPT_EXEC_INT,
                                         script_callback->function,
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

/*
 * weechat_guile_api_hook_process: hook a process
 */

SCM
weechat_guile_api_hook_process (SCM command, SCM timeout, SCM function,
                                SCM data)
{
    char *result;
    SCM return_value;

    API_FUNC(1, "hook_process", API_RETURN_EMPTY);
    if (!scm_is_string (command) || !scm_is_integer (timeout)
        || !scm_is_string (function) || !scm_is_string (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(plugin_script_api_hook_process (weechat_guile_plugin,
                                                         guile_current_script,
                                                         scm_i_string_chars (command),
                                                         scm_to_int (timeout),
                                                         &weechat_guile_api_hook_process_cb,
                                                         scm_i_string_chars (function),
                                                         scm_i_string_chars (data)));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_hook_process_hashtable: hook a process with options in a
 *                                           hashtable
 */

SCM
weechat_guile_api_hook_process_hashtable (SCM command, SCM options, SCM timeout,
                                          SCM function, SCM data)
{
    char *result;
    SCM return_value;
    struct t_hashtable *c_options;

    API_FUNC(1, "hook_process_hashtable", API_RETURN_EMPTY);
    if (!scm_is_string (command) || !scm_list_p (options)
        || !scm_is_integer (timeout) || !scm_is_string (function)
        || !scm_is_string (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    c_options = weechat_guile_alist_to_hashtable (options,
                                                  WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE);

    result = API_PTR2STR(plugin_script_api_hook_process_hashtable (weechat_guile_plugin,
                                                                   guile_current_script,
                                                                   scm_i_string_chars (command),
                                                                   c_options,
                                                                   scm_to_int (timeout),
                                                                   &weechat_guile_api_hook_process_cb,
                                                                   scm_i_string_chars (function),
                                                                   scm_i_string_chars (data)));

    if (c_options)
        weechat_hashtable_free (c_options);

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_hook_connect_cb: callback for connect hooked
 */

int
weechat_guile_api_hook_connect_cb (void *data, int status, int gnutls_rc,
                                   int sock, const char *error,
                                   const char *ip_address)
{
    struct t_plugin_script_cb *script_callback;
    void *func_argv[6];
    char str_status[32], str_gnutls_rc[32], str_sock[32];
    char empty_arg[1] = { '\0' };
    int *rc, ret;

    script_callback = (struct t_plugin_script_cb *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        snprintf (str_status, sizeof (str_status), "%d", status);
        snprintf (str_gnutls_rc, sizeof (str_gnutls_rc), "%d", gnutls_rc);
        snprintf (str_sock, sizeof (str_sock), "%d", sock);

        func_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        func_argv[1] = str_status;
        func_argv[2] = str_gnutls_rc;
        func_argv[3] = str_sock;
        func_argv[4] = (ip_address) ? (char *)ip_address : empty_arg;
        func_argv[5] = (error) ? (char *)error : empty_arg;

        rc = (int *) weechat_guile_exec (script_callback->script,
                                         WEECHAT_SCRIPT_EXEC_INT,
                                         script_callback->function,
                                         "ssssss", func_argv);

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

/*
 * weechat_guile_api_hook_connect: hook a connection
 */

SCM
weechat_guile_api_hook_connect (SCM proxy, SCM address, SCM port, SCM ipv6,
                                SCM retry, SCM local_hostname, SCM function,
                                SCM data)
{
    char *result;
    SCM return_value;

    API_FUNC(1, "hook_connect", API_RETURN_EMPTY);
    if (!scm_is_string (proxy) || !scm_is_string (address)
        || !scm_is_integer (port) || !scm_is_integer (ipv6)
        || !scm_is_integer (retry) || !scm_is_string (local_hostname)
        || !scm_is_string (function) || !scm_is_string (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(plugin_script_api_hook_connect (weechat_guile_plugin,
                                                         guile_current_script,
                                                         scm_i_string_chars (proxy),
                                                         scm_i_string_chars (address),
                                                         scm_to_int (port),
                                                         scm_to_int (ipv6),
                                                         scm_to_int (retry),
                                                         NULL, /* gnutls session */
                                                         NULL, /* gnutls callback */
                                                         0,    /* gnutls DH key size */
                                                         NULL, /* gnutls priorities */
                                                         scm_i_string_chars (local_hostname),
                                                         &weechat_guile_api_hook_connect_cb,
                                                         scm_i_string_chars (function),
                                                         scm_i_string_chars (data)));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_hook_print_cb: callback for print hooked
 */

int
weechat_guile_api_hook_print_cb (void *data, struct t_gui_buffer *buffer,
                                 time_t date,
                                 int tags_count, const char **tags,
                                 int displayed, int highlight,
                                 const char *prefix, const char *message)
{
    struct t_plugin_script_cb *script_callback;
    void *func_argv[8];
    char empty_arg[1] = { '\0' };
    static char timebuffer[64];
    int *rc, ret;

    /* make C compiler happy */
    (void) tags_count;

    script_callback = (struct t_plugin_script_cb *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        snprintf (timebuffer, sizeof (timebuffer) - 1, "%ld", (long int)date);

        func_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        func_argv[1] = API_PTR2STR(buffer);
        func_argv[2] = timebuffer;
        func_argv[3] = weechat_string_build_with_split_string (tags, ",");
        if (!func_argv[3])
            func_argv[3] = strdup ("");
        func_argv[4] = (displayed) ? strdup ("1") : strdup ("0");
        func_argv[5] = (highlight) ? strdup ("1") : strdup ("0");
        func_argv[6] = (prefix) ? (char *)prefix : empty_arg;
        func_argv[7] = (message) ? (char *)message : empty_arg;

        rc = (int *) weechat_guile_exec (script_callback->script,
                                         WEECHAT_SCRIPT_EXEC_INT,
                                         script_callback->function,
                                         "ssssssss", func_argv);

        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (func_argv[1])
            free (func_argv[1]);
        if (func_argv[3])
            free (func_argv[3]);
        if (func_argv[4])
            free (func_argv[4]);
        if (func_argv[5])
            free (func_argv[5]);

        return ret;
    }

    return WEECHAT_RC_ERROR;
}

/*
 * weechat_guile_api_hook_print: hook a print
 */

SCM
weechat_guile_api_hook_print (SCM buffer, SCM tags, SCM message,
                              SCM strip_colors, SCM function, SCM data)
{
    char *result;
    SCM return_value;

    API_FUNC(1, "hook_print", API_RETURN_EMPTY);
    if (!scm_is_string (buffer) || !scm_is_string (tags)
        || !scm_is_string (message) || !scm_is_integer (strip_colors)
        || !scm_is_string (function) || !scm_is_string (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(plugin_script_api_hook_print (weechat_guile_plugin,
                                                       guile_current_script,
                                                       API_STR2PTR(scm_i_string_chars (buffer)),
                                                       scm_i_string_chars (tags),
                                                       scm_i_string_chars (message),
                                                       scm_to_int (strip_colors),
                                                       &weechat_guile_api_hook_print_cb,
                                                       scm_i_string_chars (function),
                                                       scm_i_string_chars (data)));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_hook_signal_cb: callback for signal hooked
 */

int
weechat_guile_api_hook_signal_cb (void *data, const char *signal,
                                 const char *type_data, void *signal_data)
{
    struct t_plugin_script_cb *script_callback;
    void *func_argv[3];
    char empty_arg[1] = { '\0' };
    static char value_str[64];
    int *rc, ret, free_needed;

    script_callback = (struct t_plugin_script_cb *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        func_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        func_argv[1] = (signal) ? (char *)signal : empty_arg;
        free_needed = 0;
        if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_STRING) == 0)
        {
            func_argv[2] = (signal_data) ? (char *)signal_data : empty_arg;
        }
        else if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_INT) == 0)
        {
            snprintf (value_str, sizeof (value_str) - 1,
                      "%d", *((int *)signal_data));
            func_argv[2] = value_str;
        }
        else if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_POINTER) == 0)
        {
            func_argv[2] = API_PTR2STR(signal_data);
            free_needed = 1;
        }
        else
            func_argv[2] = empty_arg;

        rc = (int *) weechat_guile_exec (script_callback->script,
                                         WEECHAT_SCRIPT_EXEC_INT,
                                         script_callback->function,
                                         "sss", func_argv);

        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (free_needed && func_argv[2])
            free (func_argv[2]);

        return ret;
    }

    return WEECHAT_RC_ERROR;
}

/*
 * weechat_guile_api_hook_signal: hook a signal
 */

SCM
weechat_guile_api_hook_signal (SCM signal, SCM function, SCM data)
{
    char *result;
    SCM return_value;

    API_FUNC(1, "hook_signal", API_RETURN_EMPTY);
    if (!scm_is_string (signal) || !scm_is_string (function) || !scm_is_string (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(plugin_script_api_hook_signal (weechat_guile_plugin,
                                                        guile_current_script,
                                                        scm_i_string_chars (signal),
                                                        &weechat_guile_api_hook_signal_cb,
                                                        scm_i_string_chars (function),
                                                        scm_i_string_chars (data)));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_hook_signal_send: send a signal
 */

SCM
weechat_guile_api_hook_signal_send (SCM signal, SCM type_data,
                                   SCM signal_data)
{
    int number;

    API_FUNC(1, "hook_signal_send", API_RETURN_ERROR);
    if (!scm_is_string (signal) || !scm_is_string (type_data))
        API_WRONG_ARGS(API_RETURN_ERROR);

    if (strcmp (scm_i_string_chars (type_data), WEECHAT_HOOK_SIGNAL_STRING) == 0)
    {
        if (!scm_is_string (signal_data))
            API_WRONG_ARGS(API_RETURN_ERROR);
        weechat_hook_signal_send (scm_i_string_chars (signal),
                                  scm_i_string_chars (type_data),
                                  (void *)scm_i_string_chars (signal_data));
        API_RETURN_OK;
    }
    else if (strcmp (scm_i_string_chars (type_data), WEECHAT_HOOK_SIGNAL_INT) == 0)
    {
        if (!scm_is_integer (signal_data))
            API_WRONG_ARGS(API_RETURN_ERROR);
        number = scm_to_int (signal_data);
        weechat_hook_signal_send (scm_i_string_chars (signal),
                                  scm_i_string_chars (type_data),
                                  &number);
        API_RETURN_OK;
    }
    else if (strcmp (scm_i_string_chars (type_data), WEECHAT_HOOK_SIGNAL_POINTER) == 0)
    {
        if (!scm_is_string (signal_data))
            API_WRONG_ARGS(API_RETURN_ERROR);
        weechat_hook_signal_send (scm_i_string_chars (signal),
                                  scm_i_string_chars (type_data),
                                  API_STR2PTR(scm_i_string_chars (signal_data)));
        API_RETURN_OK;
    }

    API_RETURN_ERROR;
}

/*
 * weechat_guile_api_hook_hsignal_cb: callback for hsignal hooked
 */

int
weechat_guile_api_hook_hsignal_cb (void *data, const char *signal,
                                   struct t_hashtable *hashtable)
{
    struct t_plugin_script_cb *script_callback;
    void *func_argv[3];
    char empty_arg[1] = { '\0' };
    int *rc, ret;

    script_callback = (struct t_plugin_script_cb *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        func_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        func_argv[1] = (signal) ? (char *)signal : empty_arg;
        func_argv[2] = hashtable;

        rc = (int *) weechat_guile_exec (script_callback->script,
                                         WEECHAT_SCRIPT_EXEC_INT,
                                         script_callback->function,
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

/*
 * weechat_guile_api_hook_hsignal: hook a hsignal
 */

SCM
weechat_guile_api_hook_hsignal (SCM signal, SCM function, SCM data)
{
    char *result;
    SCM return_value;

    API_FUNC(1, "hook_hsignal", API_RETURN_EMPTY);
    if (!scm_is_string (signal) || !scm_is_string (function)
        || !scm_is_string (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(plugin_script_api_hook_hsignal (weechat_guile_plugin,
                                                         guile_current_script,
                                                         scm_i_string_chars (signal),
                                                         &weechat_guile_api_hook_hsignal_cb,
                                                         scm_i_string_chars (function),
                                                         scm_i_string_chars (data)));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_hook_hsignal_send: send a hsignal
 */

SCM
weechat_guile_api_hook_hsignal_send (SCM signal, SCM hashtable)
{
    struct t_hashtable *c_hashtable;

    API_FUNC(1, "hook_hsignal_send", API_RETURN_ERROR);
    if (!scm_is_string (signal) || !scm_list_p (hashtable))
        API_WRONG_ARGS(API_RETURN_ERROR);

    c_hashtable = weechat_guile_alist_to_hashtable (hashtable,
                                                    WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE);

    weechat_hook_hsignal_send (scm_i_string_chars (signal), c_hashtable);

    if (c_hashtable)
        weechat_hashtable_free (c_hashtable);

    API_RETURN_OK;
}

/*
 * weechat_guile_api_hook_config_cb: callback for config option hooked
 */

int
weechat_guile_api_hook_config_cb (void *data, const char *option, const char *value)
{
    struct t_plugin_script_cb *script_callback;
    void *func_argv[3];
    char empty_arg[1] = { '\0' };
    int *rc, ret;

    script_callback = (struct t_plugin_script_cb *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        func_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        func_argv[1] = (option) ? (char *)option : empty_arg;
        func_argv[2] = (value) ? (char *)value : empty_arg;

        rc = (int *) weechat_guile_exec (script_callback->script,
                                         WEECHAT_SCRIPT_EXEC_INT,
                                         script_callback->function,
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

/*
 * weechat_guile_api_hook_config: hook a config option
 */

SCM
weechat_guile_api_hook_config (SCM option, SCM function, SCM data)
{
    char *result;
    SCM return_value;

    API_FUNC(1, "hook_config", API_RETURN_EMPTY);
    if (!scm_is_string (option) || !scm_is_string (function)
        || !scm_is_string (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(plugin_script_api_hook_config (weechat_guile_plugin,
                                                        guile_current_script,
                                                        scm_i_string_chars (option),
                                                        &weechat_guile_api_hook_config_cb,
                                                        scm_i_string_chars (function),
                                                        scm_i_string_chars (data)));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_hook_completion_cb: callback for completion hooked
 */

int
weechat_guile_api_hook_completion_cb (void *data, const char *completion_item,
                                      struct t_gui_buffer *buffer,
                                      struct t_gui_completion *completion)
{
    struct t_plugin_script_cb *script_callback;
    void *func_argv[4];
    char empty_arg[1] = { '\0' };
    int *rc, ret;

    script_callback = (struct t_plugin_script_cb *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        func_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        func_argv[1] = (completion_item) ? (char *)completion_item : empty_arg;
        func_argv[2] = API_PTR2STR(buffer);
        func_argv[3] = API_PTR2STR(completion);

        rc = (int *) weechat_guile_exec (script_callback->script,
                                         WEECHAT_SCRIPT_EXEC_INT,
                                         script_callback->function,
                                         "ssss", func_argv);

        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (func_argv[2])
            free (func_argv[2]);
        if (func_argv[3])
            free (func_argv[3]);

        return ret;
    }

    return WEECHAT_RC_ERROR;
}

/*
 * weechat_guile_api_hook_completion: hook a completion
 */

SCM
weechat_guile_api_hook_completion (SCM completion, SCM description,
                                   SCM function, SCM data)
{
    char *result;
    SCM return_value;

    API_FUNC(1, "hook_completion", API_RETURN_EMPTY);
    if (!scm_is_string (completion) || !scm_is_string (description)
        || !scm_is_string (function) || !scm_is_string (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(plugin_script_api_hook_completion (weechat_guile_plugin,
                                                            guile_current_script,
                                                            scm_i_string_chars (completion),
                                                            scm_i_string_chars (description),
                                                            &weechat_guile_api_hook_completion_cb,
                                                            scm_i_string_chars (function),
                                                            scm_i_string_chars (data)));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_hook_completion_list_add: add a word to list for a completion
 */

SCM
weechat_guile_api_hook_completion_list_add (SCM completion, SCM word,
                                            SCM nick_completion, SCM where)
{
    API_FUNC(1, "hook_completion_list_add", API_RETURN_ERROR);
    if (!scm_is_string (completion) || !scm_is_string (word)
        || !scm_is_integer (nick_completion) || !scm_is_string (where))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_hook_completion_list_add (API_STR2PTR(scm_i_string_chars (completion)),
                                      scm_i_string_chars (word),
                                      scm_to_int (nick_completion),
                                      scm_i_string_chars (where));

    API_RETURN_OK;
}

/*
 * weechat_guile_api_hook_modifier_cb: callback for modifier hooked
 */

char *
weechat_guile_api_hook_modifier_cb (void *data, const char *modifier,
                                    const char *modifier_data,  const char *string)
{
    struct t_plugin_script_cb *script_callback;
    void *func_argv[4];
    char empty_arg[1] = { '\0' };

    script_callback = (struct t_plugin_script_cb *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        func_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        func_argv[1] = (modifier) ? (char *)modifier : empty_arg;
        func_argv[2] = (modifier_data) ? (char *)modifier_data : empty_arg;
        func_argv[3] = (string) ? (char *)string : empty_arg;

        return (char *)weechat_guile_exec (script_callback->script,
                                           WEECHAT_SCRIPT_EXEC_STRING,
                                           script_callback->function,
                                           "ssss", func_argv);
    }

    return NULL;
}

/*
 * weechat_guile_api_hook_modifier: hook a modifier
 */

SCM
weechat_guile_api_hook_modifier (SCM modifier, SCM function, SCM data)
{
    char *result;
    SCM return_value;

    API_FUNC(1, "hook_modifier", API_RETURN_EMPTY);
    if (!scm_is_string (modifier) || !scm_is_string (function)
        || !scm_is_string (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(plugin_script_api_hook_modifier (weechat_guile_plugin,
                                                          guile_current_script,
                                                          scm_i_string_chars (modifier),
                                                          &weechat_guile_api_hook_modifier_cb,
                                                          scm_i_string_chars (function),
                                                          scm_i_string_chars (data)));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_hook_modifier_exec: execute a modifier hook
 */

SCM
weechat_guile_api_hook_modifier_exec (SCM modifier, SCM modifier_data,
                                      SCM string)
{
    char *result;
    SCM return_value;

    API_FUNC(1, "hook_modifier_exec", API_RETURN_EMPTY);
    if (!scm_is_string (modifier) || !scm_is_string (modifier_data)
        || !scm_is_string (string))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_hook_modifier_exec (scm_i_string_chars (modifier),
                                         scm_i_string_chars (modifier_data),
                                         scm_i_string_chars (string));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_hook_info_cb: callback for info hooked
 */

const char *
weechat_guile_api_hook_info_cb (void *data, const char *info_name,
                                const char *arguments)
{
    struct t_plugin_script_cb *script_callback;
    void *func_argv[3];
    char empty_arg[1] = { '\0' };

    script_callback = (struct t_plugin_script_cb *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        func_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        func_argv[1] = (info_name) ? (char *)info_name : empty_arg;
        func_argv[2] = (arguments) ? (char *)arguments : empty_arg;

        return (const char *)weechat_guile_exec (script_callback->script,
                                                 WEECHAT_SCRIPT_EXEC_STRING,
                                                 script_callback->function,
                                                 "sss", func_argv);
    }

    return NULL;
}

/*
 * weechat_guile_api_hook_info: hook an info
 */

SCM
weechat_guile_api_hook_info (SCM info_name, SCM description,
                             SCM args_description, SCM function, SCM data)
{
    char *result;
    SCM return_value;

    API_FUNC(1, "hook_info", API_RETURN_EMPTY);
    if (!scm_is_string (info_name) || !scm_is_string (description)
        || !scm_is_string (args_description) || !scm_is_string (function)
        || !scm_is_string (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(plugin_script_api_hook_info (weechat_guile_plugin,
                                                      guile_current_script,
                                                      scm_i_string_chars (info_name),
                                                      scm_i_string_chars (description),
                                                      scm_i_string_chars (args_description),
                                                      &weechat_guile_api_hook_info_cb,
                                                      scm_i_string_chars (function),
                                                      scm_i_string_chars (data)));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_hook_info_hashtable_cb: callback for info_hashtable hooked
 */

struct t_hashtable *
weechat_guile_api_hook_info_hashtable_cb (void *data, const char *info_name,
                                          struct t_hashtable *hashtable)
{
    struct t_plugin_script_cb *script_callback;
    void *func_argv[3];
    char empty_arg[1] = { '\0' };

    script_callback = (struct t_plugin_script_cb *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        func_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        func_argv[1] = (info_name) ? (char *)info_name : empty_arg;
        func_argv[2] = hashtable;

        return (struct t_hashtable *)weechat_guile_exec (script_callback->script,
                                                         WEECHAT_SCRIPT_EXEC_HASHTABLE,
                                                         script_callback->function,
                                                         "ssh", func_argv);
    }

    return NULL;
}

/*
 * weechat_guile_api_hook_info_hashtable: hook an info_hashtable
 */

SCM
weechat_guile_api_hook_info_hashtable (SCM info_name, SCM description,
                                       SCM args_description,
                                       SCM output_description, SCM function,
                                       SCM data)
{
    char *result;
    SCM return_value;

    API_FUNC(1, "hook_info_hashtable", API_RETURN_EMPTY);
    if (!scm_is_string (info_name) || !scm_is_string (description)
        || !scm_is_string (args_description) || !scm_is_string (output_description)
        || !scm_is_string (function) || !scm_is_string (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(plugin_script_api_hook_info_hashtable (weechat_guile_plugin,
                                                                guile_current_script,
                                                                scm_i_string_chars (info_name),
                                                                scm_i_string_chars (description),
                                                                scm_i_string_chars (args_description),
                                                                scm_i_string_chars (output_description),
                                                                &weechat_guile_api_hook_info_hashtable_cb,
                                                                scm_i_string_chars (function),
                                                                scm_i_string_chars (data)));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_hook_infolist_cb: callback for infolist hooked
 */

struct t_infolist *
weechat_guile_api_hook_infolist_cb (void *data, const char *infolist_name,
                                    void *pointer, const char *arguments)
{
    struct t_plugin_script_cb *script_callback;
    void *func_argv[4];
    char empty_arg[1] = { '\0' };
    struct t_infolist *result;

    script_callback = (struct t_plugin_script_cb *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        func_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        func_argv[1] = (infolist_name) ? (char *)infolist_name : empty_arg;
        func_argv[2] = API_PTR2STR(pointer);
        func_argv[3] = (arguments) ? (char *)arguments : empty_arg;

        result = (struct t_infolist *)weechat_guile_exec (script_callback->script,
                                                          WEECHAT_SCRIPT_EXEC_STRING,
                                                          script_callback->function,
                                                          "ssss", func_argv);

        if (func_argv[2])
            free (func_argv[2]);

        return result;
    }

    return NULL;
}

/*
 * weechat_guile_api_hook_infolist: hook an infolist
 */

SCM
weechat_guile_api_hook_infolist (SCM infolist_name, SCM description,
                                 SCM pointer_description, SCM args_description,
                                 SCM function, SCM data)
{
    char *result;
    SCM return_value;

    API_FUNC(1, "hook_infolist", API_RETURN_EMPTY);
    if (!scm_is_string (infolist_name) || !scm_is_string (description)
        || !scm_is_string (pointer_description) || !scm_is_string (args_description)
        || !scm_is_string (function) || !scm_is_string (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(plugin_script_api_hook_infolist (weechat_guile_plugin,
                                                          guile_current_script,
                                                          scm_i_string_chars (infolist_name),
                                                          scm_i_string_chars (description),
                                                          scm_i_string_chars (pointer_description),
                                                          scm_i_string_chars (args_description),
                                                          &weechat_guile_api_hook_infolist_cb,
                                                          scm_i_string_chars (function),
                                                          scm_i_string_chars (data)));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_hook_focus_cb: callback for focus hooked
 */

struct t_hashtable *
weechat_guile_api_hook_focus_cb (void *data, struct t_hashtable *info)
{
    struct t_plugin_script_cb *script_callback;
    void *func_argv[2];
    char empty_arg[1] = { '\0' };

    script_callback = (struct t_plugin_script_cb *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        func_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        func_argv[1] = info;

        return (struct t_hashtable *)weechat_guile_exec (script_callback->script,
                                                         WEECHAT_SCRIPT_EXEC_HASHTABLE,
                                                         script_callback->function,
                                                         "sh", func_argv);
    }

    return NULL;
}

/*
 * weechat_guile_api_hook_focus: hook a focus
 */

SCM
weechat_guile_api_hook_focus (SCM area, SCM function, SCM data)
{
    char *result;
    SCM return_value;

    API_FUNC(1, "hook_focus", API_RETURN_EMPTY);
    if (!scm_is_string (area) || !scm_is_string (function)
        || !scm_is_string (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(plugin_script_api_hook_focus (weechat_guile_plugin,
                                                       guile_current_script,
                                                       scm_i_string_chars (area),
                                                       &weechat_guile_api_hook_focus_cb,
                                                       scm_i_string_chars (function),
                                                       scm_i_string_chars (data)));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_unhook: unhook something
 */

SCM
weechat_guile_api_unhook (SCM hook)
{
    API_FUNC(1, "unhook", API_RETURN_ERROR);
    if (!scm_is_string (hook))
        API_WRONG_ARGS(API_RETURN_ERROR);

    plugin_script_api_unhook (weechat_guile_plugin,
                              guile_current_script,
                              API_STR2PTR(scm_i_string_chars (hook)));

    API_RETURN_OK;
}

/*
 * weechat_guile_api_unhook_all: unhook all for script
 */

SCM
weechat_guile_api_unhook_all ()
{
    API_FUNC(1, "unhook_all", API_RETURN_ERROR);

    plugin_script_api_unhook_all (weechat_guile_plugin, guile_current_script);

    API_RETURN_OK;
}

/*
 * weechat_guile_api_buffer_input_data_cb: callback for input data in a buffer
 */

int
weechat_guile_api_buffer_input_data_cb (void *data, struct t_gui_buffer *buffer,
                                        const char *input_data)
{
    struct t_plugin_script_cb *script_callback;
    void *func_argv[3];
    char empty_arg[1] = { '\0' };
    int *rc, ret;

    script_callback = (struct t_plugin_script_cb *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        func_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        func_argv[1] = API_PTR2STR(buffer);
        func_argv[2] = (input_data) ? (char *)input_data : empty_arg;

        rc = (int *) weechat_guile_exec (script_callback->script,
                                         WEECHAT_SCRIPT_EXEC_INT,
                                         script_callback->function,
                                         "sss", func_argv);

        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (func_argv[1])
            free (func_argv[1]);

        return ret;
    }

    return WEECHAT_RC_ERROR;
}

/*
 * weechat_guile_api_buffer_close_cb: callback for closed buffer
 */

int
weechat_guile_api_buffer_close_cb (void *data, struct t_gui_buffer *buffer)
{
    struct t_plugin_script_cb *script_callback;
    void *func_argv[2];
    char empty_arg[1] = { '\0' };
    int *rc, ret;

    script_callback = (struct t_plugin_script_cb *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        func_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        func_argv[1] = API_PTR2STR(buffer);

        rc = (int *) weechat_guile_exec (script_callback->script,
                                         WEECHAT_SCRIPT_EXEC_INT,
                                         script_callback->function,
                                         "ss", func_argv);

        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (func_argv[1])
            free (func_argv[1]);

        return ret;
    }

    return WEECHAT_RC_ERROR;
}

/*
 * weechat_guile_api_buffer_new: create a new buffer
 */

SCM
weechat_guile_api_buffer_new (SCM name, SCM function_input, SCM data_input,
                              SCM function_close, SCM data_close)
{
    char *result;
    SCM return_value;

    API_FUNC(1, "buffer_new", API_RETURN_EMPTY);
    if (!scm_is_string (name) || !scm_is_string (function_input)
        || !scm_is_string (data_input) || !scm_is_string (function_close)
        || !scm_is_string (data_close))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(plugin_script_api_buffer_new (weechat_guile_plugin,
                                                       guile_current_script,
                                                       scm_i_string_chars (name),
                                                       &weechat_guile_api_buffer_input_data_cb,
                                                       scm_i_string_chars (function_input),
                                                       scm_i_string_chars (data_input),
                                                       &weechat_guile_api_buffer_close_cb,
                                                       scm_i_string_chars (function_close),
                                                       scm_i_string_chars (data_close)));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_buffer_search: search a buffer
 */

SCM
weechat_guile_api_buffer_search (SCM plugin, SCM name)
{
    char *result;
    SCM return_value;

    API_FUNC(1, "buffer_search", API_RETURN_EMPTY);
    if (!scm_is_string (plugin) || !scm_is_string (name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_buffer_search (scm_i_string_chars (plugin),
                                                scm_i_string_chars (name)));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_buffer_search_main: search main buffer (WeeChat core buffer)
 */

SCM
weechat_guile_api_buffer_search_main ()
{
    char *result;
    SCM return_value;

    API_FUNC(1, "buffer_search_main", API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_buffer_search_main ());

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_current_buffer: get current buffer
 */

SCM
weechat_guile_api_current_buffer ()
{
    char *result;
    SCM return_value;

    API_FUNC(1, "current_buffer", API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_current_buffer ());

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_buffer_clear: clear a buffer
 */

SCM
weechat_guile_api_buffer_clear (SCM buffer)
{
    API_FUNC(1, "buffer_clear", API_RETURN_ERROR);
    if (!scm_is_string (buffer))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_buffer_clear (API_STR2PTR(scm_i_string_chars (buffer)));

    API_RETURN_OK;
}

/*
 * weechat_guile_api_buffer_close: close a buffer
 */

SCM
weechat_guile_api_buffer_close (SCM buffer)
{
    API_FUNC(1, "buffer_close", API_RETURN_ERROR);
    if (!scm_is_string (buffer))
        API_WRONG_ARGS(API_RETURN_ERROR);

    plugin_script_api_buffer_close (weechat_guile_plugin,
                                    guile_current_script,
                                    API_STR2PTR(scm_i_string_chars (buffer)));

    API_RETURN_OK;
}

/*
 * weechat_guile_api_buffer_merge: merge a buffer to another buffer
 */

SCM
weechat_guile_api_buffer_merge (SCM buffer, SCM target_buffer)
{
    API_FUNC(1, "buffer_merge", API_RETURN_ERROR);
    if (!scm_is_string (buffer) || !scm_is_string (target_buffer))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_buffer_merge (API_STR2PTR(scm_i_string_chars (buffer)),
                          API_STR2PTR(scm_i_string_chars (target_buffer)));

    API_RETURN_OK;
}

/*
 * weechat_guile_api_buffer_unmerge: unmerge a buffer from a group of merged
 *                                  buffers
 */

SCM
weechat_guile_api_buffer_unmerge (SCM buffer, SCM number)
{
    API_FUNC(1, "buffer_unmerge", API_RETURN_ERROR);
    if (!scm_is_string (buffer) || !scm_is_integer (number))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_buffer_unmerge (API_STR2PTR(scm_i_string_chars (buffer)),
                            scm_to_int (number));

    API_RETURN_OK;
}

/*
 * weechat_guile_api_buffer_get_integer: get a buffer property as integer
 */

SCM
weechat_guile_api_buffer_get_integer (SCM buffer, SCM property)
{
    int value;

    API_FUNC(1, "buffer_get_integer", API_RETURN_INT(-1));
    if (!scm_is_string (buffer) || !scm_is_string (property))
        API_WRONG_ARGS(API_RETURN_INT(-1));

    value = weechat_buffer_get_integer (API_STR2PTR(scm_i_string_chars (buffer)),
                                        scm_i_string_chars (property));

    API_RETURN_INT(value);
}

/*
 * weechat_guile_api_buffer_get_string: get a buffer property as string
 */

SCM
weechat_guile_api_buffer_get_string (SCM buffer, SCM property)
{
    const char *result;

    API_FUNC(1, "buffer_get_string", API_RETURN_EMPTY);
    if (!scm_is_string (buffer) || !scm_is_string (property))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_buffer_get_string (API_STR2PTR(scm_i_string_chars (buffer)),
                                        scm_i_string_chars (property));

    API_RETURN_STRING(result);
}

/*
 * weechat_guile_api_buffer_get_pointer: get a buffer property as pointer
 */

SCM
weechat_guile_api_buffer_get_pointer (SCM buffer, SCM property)
{
    char *result;
    SCM return_value;

    API_FUNC(1, "buffer_get_pointer", API_RETURN_EMPTY);
    if (!scm_is_string (buffer) || !scm_is_string (property))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_buffer_get_pointer (API_STR2PTR(scm_i_string_chars (buffer)),
                                                     scm_i_string_chars (property)));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_buffer_set: set a buffer property
 */

SCM
weechat_guile_api_buffer_set (SCM buffer, SCM property, SCM value)
{
    API_FUNC(1, "buffer_set", API_RETURN_ERROR);
    if (!scm_is_string (buffer) || !scm_is_string (property)
        || !scm_is_string (value))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_buffer_set (API_STR2PTR(scm_i_string_chars (buffer)),
                        scm_i_string_chars (property),
                        scm_i_string_chars (value));

    API_RETURN_OK;
}

/*
 * weechat_guile_api_buffer_string_replace_local_var: replace local variables
 *                                                    ($var) in a string, using
 *                                                    value of local variables
 */

SCM
weechat_guile_api_buffer_string_replace_local_var (SCM buffer, SCM string)
{
    char *result;
    SCM return_value;

    API_FUNC(1, "buffer_string_replace_local_var", API_RETURN_ERROR);
    if (!scm_is_string (buffer) || !scm_is_string (string))
        API_WRONG_ARGS(API_RETURN_ERROR);

    result = weechat_buffer_string_replace_local_var (API_STR2PTR(scm_i_string_chars (buffer)),
                                                      scm_i_string_chars (string));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_buffer_match_list: return 1 if buffer matches list of buffers
 */

SCM
weechat_guile_api_buffer_match_list (SCM buffer, SCM string)
{
    int value;

    API_FUNC(1, "buffer_match_list", API_RETURN_INT(0));
    if (!scm_is_string (buffer) || !scm_is_string (string))
        API_WRONG_ARGS(API_RETURN_INT(0));

    value = weechat_buffer_match_list (API_STR2PTR(scm_i_string_chars (buffer)),
                                       scm_i_string_chars (string));

    API_RETURN_INT(value);
}

/*
 * weechat_guile_api_current_window: get current window
 */

SCM
weechat_guile_api_current_window ()
{
    char *result;
    SCM return_value;

    API_FUNC(1, "current_window", API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_current_window ());

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_window_search_with_buffer: search a window with buffer
 *                                             pointer
 */

SCM
weechat_guile_api_window_search_with_buffer (SCM buffer)
{
    char *result;
    SCM return_value;

    API_FUNC(1, "window_search_with_buffer", API_RETURN_EMPTY);
    if (!scm_is_string (buffer))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_window_search_with_buffer (API_STR2PTR(scm_i_string_chars (buffer))));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_window_get_integer: get a window property as integer
 */

SCM
weechat_guile_api_window_get_integer (SCM window, SCM property)
{
    int value;

    API_FUNC(1, "window_get_integer", API_RETURN_INT(-1));
    if (!scm_is_string (window) || !scm_is_string (property))
        API_WRONG_ARGS(API_RETURN_INT(-1));

    value = weechat_window_get_integer (API_STR2PTR(scm_i_string_chars (window)),
                                        scm_i_string_chars (property));

    API_RETURN_INT(value);
}

/*
 * weechat_guile_api_window_get_string: get a window property as string
 */

SCM
weechat_guile_api_window_get_string (SCM window, SCM property)
{
    const char *result;

    API_FUNC(1, "window_get_string", API_RETURN_EMPTY);
    if (!scm_is_string (window) || !scm_is_string (property))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_window_get_string (API_STR2PTR(scm_i_string_chars (window)),
                                        scm_i_string_chars (property));

    API_RETURN_STRING(result);
}

/*
 * weechat_guile_api_window_get_pointer: get a window property as pointer
 */

SCM
weechat_guile_api_window_get_pointer (SCM window, SCM property)
{
    char *result;
    SCM return_value;

    API_FUNC(1, "window_get_pointer", API_RETURN_EMPTY);
    if (!scm_is_string (window) || !scm_is_string (property))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_window_get_pointer (API_STR2PTR(scm_i_string_chars (window)),
                                                     scm_i_string_chars (property)));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_window_set_title: set window title
 */

SCM
weechat_guile_api_window_set_title (SCM title)
{
    API_FUNC(1, "window_set_title", API_RETURN_ERROR);
    if (!scm_is_string (title))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_window_set_title (scm_i_string_chars (title));

    API_RETURN_OK;
}

/*
 * weechat_guile_api_nicklist_add_group: add a group in nicklist
 */

SCM
weechat_guile_api_nicklist_add_group (SCM buffer, SCM parent_group, SCM name,
                                      SCM color, SCM visible)
{
    char *result;
    SCM return_value;

    API_FUNC(1, "nicklist_add_group", API_RETURN_EMPTY);
    if (!scm_is_string (buffer) || !scm_is_string (parent_group)
        || !scm_is_string (name) || !scm_is_string (color)
        || !scm_is_integer (visible))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_nicklist_add_group (API_STR2PTR(scm_i_string_chars (buffer)),
                                                     API_STR2PTR(scm_i_string_chars (parent_group)),
                                                     scm_i_string_chars (name),
                                                     scm_i_string_chars (color),
                                                     scm_to_int (visible)));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_nicklist_search_group: search a group in nicklist
 */

SCM
weechat_guile_api_nicklist_search_group (SCM buffer, SCM from_group, SCM name)
{
    char *result;
    SCM return_value;

    API_FUNC(1, "nicklist_search_group", API_RETURN_EMPTY);
    if (!scm_is_string (buffer) || !scm_is_string (from_group)
        || !scm_is_string (name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_nicklist_search_group (API_STR2PTR(scm_i_string_chars (buffer)),
                                                        API_STR2PTR(scm_i_string_chars (from_group)),
                                                        scm_i_string_chars (name)));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_nicklist_add_nick: add a nick in nicklist
 */

SCM
weechat_guile_api_nicklist_add_nick (SCM buffer, SCM group, SCM name,
                                     SCM color, SCM prefix, SCM prefix_color,
                                     SCM visible)
{
    char *result;
    SCM return_value;

    API_FUNC(1, "nicklist_add_nick", API_RETURN_EMPTY);
    if (!scm_is_string (buffer) || !scm_is_string (group)
        || !scm_is_string (name) || !scm_is_string (color)
        || !scm_is_string (prefix) || !scm_is_string (prefix_color)
        || !scm_is_integer (visible))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_nicklist_add_nick (API_STR2PTR(scm_i_string_chars (buffer)),
                                                    API_STR2PTR(scm_i_string_chars (group)),
                                                    scm_i_string_chars (name),
                                                    scm_i_string_chars (color),
                                                    scm_i_string_chars (prefix),
                                                    scm_i_string_chars (prefix_color),
                                                    scm_to_int (visible)));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_nicklist_search_nick: search a nick in nicklist
 */

SCM
weechat_guile_api_nicklist_search_nick (SCM buffer, SCM from_group, SCM name)
{
    char *result;
    SCM return_value;

    API_FUNC(1, "nicklist_search_nick", API_RETURN_EMPTY);
    if (!scm_is_string (buffer) || !scm_is_string (from_group)
        || !scm_is_string (name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_nicklist_search_nick (API_STR2PTR(scm_i_string_chars (buffer)),
                                                       API_STR2PTR(scm_i_string_chars (from_group)),
                                                       scm_i_string_chars (name)));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_nicklist_remove_group: remove a group from nicklist
 */

SCM
weechat_guile_api_nicklist_remove_group (SCM buffer, SCM group)
{
    API_FUNC(1, "nicklist_remove_group", API_RETURN_ERROR);
    if (!scm_is_string (buffer) || !scm_is_string (group))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_nicklist_remove_group (API_STR2PTR(scm_i_string_chars (buffer)),
                                   API_STR2PTR(scm_i_string_chars (group)));

    API_RETURN_OK;
}

/*
 * weechat_guile_api_nicklist_remove_nick: remove a nick from nicklist
 */

SCM
weechat_guile_api_nicklist_remove_nick (SCM buffer, SCM nick)
{
    API_FUNC(1, "nicklist_remove_nick", API_RETURN_ERROR);
    if (!scm_is_string (buffer) || !scm_is_string (nick))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_nicklist_remove_nick (API_STR2PTR(scm_i_string_chars (buffer)),
                                  API_STR2PTR(scm_i_string_chars (nick)));

    API_RETURN_OK;
}

/*
 * weechat_guile_api_nicklist_remove_all: remove all groups/nicks from nicklist
 */

SCM
weechat_guile_api_nicklist_remove_all (SCM buffer)
{
    API_FUNC(1, "nicklist_remove_all", API_RETURN_ERROR);
    if (!scm_is_string (buffer))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_nicklist_remove_all (API_STR2PTR(scm_i_string_chars (buffer)));

    API_RETURN_OK;
}

/*
 * weechat_guile_api_nicklist_group_get_integer: get a group property as integer
 */

SCM
weechat_guile_api_nicklist_group_get_integer (SCM buffer, SCM group,
                                              SCM property)
{
    int value;

    API_FUNC(1, "nicklist_group_get_integer", API_RETURN_INT(-1));
    if (!scm_is_string (buffer) || !scm_is_string (group)
        || !scm_is_string (property))
        API_WRONG_ARGS(API_RETURN_INT(-1));

    value = weechat_nicklist_group_get_integer (API_STR2PTR(scm_i_string_chars (buffer)),
                                                API_STR2PTR(scm_i_string_chars (group)),
                                                scm_i_string_chars (property));

    API_RETURN_INT(value);
}

/*
 * weechat_guile_api_nicklist_group_get_string: get a group property as string
 */

SCM
weechat_guile_api_nicklist_group_get_string (SCM buffer, SCM group,
                                             SCM property)
{
    const char *result;

    API_FUNC(1, "nicklist_group_get_string", API_RETURN_EMPTY);
    if (!scm_is_string (buffer) || !scm_is_string (group)
        || !scm_is_string (property))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_nicklist_group_get_string (API_STR2PTR(scm_i_string_chars (buffer)),
                                                API_STR2PTR(scm_i_string_chars (group)),
                                                scm_i_string_chars (property));

    API_RETURN_STRING(result);
}

/*
 * weechat_guile_api_nicklist_group_get_pointer: get a group property as pointer
 */

SCM
weechat_guile_api_nicklist_group_get_pointer (SCM buffer, SCM group,
                                              SCM property)
{
    char *result;
    SCM return_value;

    API_FUNC(1, "nicklist_group_get_pointer", API_RETURN_EMPTY);
    if (!scm_is_string (buffer) || !scm_is_string (group)
        || !scm_is_string (property))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_nicklist_group_get_pointer (API_STR2PTR(scm_i_string_chars (buffer)),
                                                             API_STR2PTR(scm_i_string_chars (group)),
                                                             scm_i_string_chars (property)));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_nicklist_group_set: set a group property
 */

SCM
weechat_guile_api_nicklist_group_set (SCM buffer, SCM group, SCM property,
                                      SCM value)
{
    API_FUNC(1, "nicklist_group_set", API_RETURN_ERROR);
    if (!scm_is_string (buffer) || !scm_is_string (group) || !scm_is_string (property) || !scm_is_string (value))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_nicklist_group_set (API_STR2PTR(scm_i_string_chars (buffer)),
                                API_STR2PTR(scm_i_string_chars (group)),
                                scm_i_string_chars (property),
                                scm_i_string_chars (value));

    API_RETURN_OK;
}

/*
 * weechat_guile_api_nicklist_nick_get_integer: get a nick property as integer
 */

SCM
weechat_guile_api_nicklist_nick_get_integer (SCM buffer, SCM nick, SCM property)
{
    int value;

    API_FUNC(1, "nicklist_nick_get_integer", API_RETURN_INT(-1));
    if (!scm_is_string (buffer) || !scm_is_string (nick)
        || !scm_is_string (property))
        API_WRONG_ARGS(API_RETURN_INT(-1));

    value = weechat_nicklist_nick_get_integer (API_STR2PTR(scm_i_string_chars (buffer)),
                                               API_STR2PTR(scm_i_string_chars (nick)),
                                               scm_i_string_chars (property));

    API_RETURN_INT(value);
}

/*
 * weechat_guile_api_nicklist_nick_get_string: get a nick property as string
 */

SCM
weechat_guile_api_nicklist_nick_get_string (SCM buffer, SCM nick, SCM property)
{
    const char *result;

    API_FUNC(1, "nicklist_nick_get_string", API_RETURN_EMPTY);
    if (!scm_is_string (buffer) || !scm_is_string (nick)
        || !scm_is_string (property))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_nicklist_nick_get_string (API_STR2PTR(scm_i_string_chars (buffer)),
                                               API_STR2PTR(scm_i_string_chars (nick)),
                                               scm_i_string_chars (property));

    API_RETURN_STRING(result);
}

/*
 * weechat_guile_api_nicklist_nick_get_pointer: get a nick property as pointer
 */

SCM
weechat_guile_api_nicklist_nick_get_pointer (SCM buffer, SCM nick, SCM property)
{
    char *result;
    SCM return_value;

    API_FUNC(1, "nicklist_nick_get_pointer", API_RETURN_EMPTY);
    if (!scm_is_string (buffer) || !scm_is_string (nick)
        || !scm_is_string (property))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_nicklist_nick_get_pointer (API_STR2PTR(scm_i_string_chars (buffer)),
                                                            API_STR2PTR(scm_i_string_chars (nick)),
                                                            scm_i_string_chars (property)));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_nicklist_nick_set: set a nick property
 */

SCM
weechat_guile_api_nicklist_nick_set (SCM buffer, SCM nick, SCM property,
                                     SCM value)
{
    API_FUNC(1, "nicklist_nick_set", API_RETURN_ERROR);
    if (!scm_is_string (buffer) || !scm_is_string (nick)
        || !scm_is_string (property) || !scm_is_string (value))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_nicklist_nick_set (API_STR2PTR(scm_i_string_chars (buffer)),
                               API_STR2PTR(scm_i_string_chars (nick)),
                               scm_i_string_chars (property),
                               scm_i_string_chars (value));

    API_RETURN_OK;
}

/*
 * weechat_guile_api_bar_item_search: search a bar item
 */

SCM
weechat_guile_api_bar_item_search (SCM name)
{
    char *result;
    SCM return_value;

    API_FUNC(1, "bar_item_search", API_RETURN_EMPTY);
    if (!scm_is_string (name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_bar_item_search (scm_i_string_chars (name)));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_bar_item_build_cb: callback for building bar item
 */

char *
weechat_guile_api_bar_item_build_cb (void *data, struct t_gui_bar_item *item,
                                     struct t_gui_window *window)
{
    struct t_plugin_script_cb *script_callback;
    void *func_argv[3];
    char empty_arg[1] = { '\0' }, *ret;

    script_callback = (struct t_plugin_script_cb *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        func_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        func_argv[1] = API_PTR2STR(item);
        func_argv[2] = API_PTR2STR(window);

        ret = (char *)weechat_guile_exec (script_callback->script,
                                          WEECHAT_SCRIPT_EXEC_STRING,
                                          script_callback->function,
                                          "sss", func_argv);

        if (func_argv[1])
            free (func_argv[1]);
        if (func_argv[2])
            free (func_argv[2]);

        return ret;
    }

    return NULL;
}

/*
 * weechat_guile_api_bar_item_new: add a new bar item
 */

SCM
weechat_guile_api_bar_item_new (SCM name, SCM function, SCM data)
{
    char *result;
    SCM return_value;

    API_FUNC(1, "bar_item_new", API_RETURN_EMPTY);
    if (!scm_is_string (name) || !scm_is_string (function)
        || !scm_is_string (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(plugin_script_api_bar_item_new (weechat_guile_plugin,
                                                         guile_current_script,
                                                         scm_i_string_chars (name),
                                                         &weechat_guile_api_bar_item_build_cb,
                                                         scm_i_string_chars (function),
                                                         scm_i_string_chars (data)));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_bar_item_update: update a bar item on screen
 */

SCM
weechat_guile_api_bar_item_update (SCM name)
{
    API_FUNC(1, "bar_item_update", API_RETURN_ERROR);
    if (!scm_is_string (name))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_bar_item_update (scm_i_string_chars (name));

    API_RETURN_OK;
}

/*
 * weechat_guile_api_bar_item_remove: remove a bar item
 */

SCM
weechat_guile_api_bar_item_remove (SCM item)
{
    API_FUNC(1, "bar_item_remove", API_RETURN_ERROR);
    if (!scm_is_string (item))
        API_WRONG_ARGS(API_RETURN_ERROR);

    plugin_script_api_bar_item_remove (weechat_guile_plugin,
                                       guile_current_script,
                                       API_STR2PTR(scm_i_string_chars (item)));

    API_RETURN_OK;
}

/*
 * weechat_guile_api_bar_search: search a bar
 */

SCM
weechat_guile_api_bar_search (SCM name)
{
    char *result;
    SCM return_value;

    API_FUNC(1, "bar_search", API_RETURN_EMPTY);
    if (!scm_is_string (name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_bar_search (scm_i_string_chars (name)));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_bar_new: add a new bar
 */

SCM
weechat_guile_api_bar_new (SCM args)
{
    SCM name, hidden, priority, type, conditions, position, filling_top_bottom;
    SCM filling_left_right, size, size_max, color_fg, color_delim, color_bg;
    SCM separator, items;
    char *result;
    SCM return_value;

    API_FUNC(1, "bar_new", API_RETURN_EMPTY);
    if (!scm_list_p (args) || (scm_to_int (scm_length (args)) != 15))
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
    separator = scm_list_ref (args, scm_from_int (13));
    items = scm_list_ref (args, scm_from_int (14));

    if (!scm_is_string (name) || !scm_is_string (hidden)
        || !scm_is_string (priority) || !scm_is_string (type)
        || !scm_is_string (conditions) || !scm_is_string (position)
        || !scm_is_string (filling_top_bottom) || !scm_is_string (filling_left_right)
        || !scm_is_string (size) || !scm_is_string (size_max)
        || !scm_is_string (color_fg) || !scm_is_string (color_delim)
        || !scm_is_string (color_bg) || !scm_is_string (separator)
        || !scm_is_string (items))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_bar_new (scm_i_string_chars (name),
                                          scm_i_string_chars (hidden),
                                          scm_i_string_chars (priority),
                                          scm_i_string_chars (type),
                                          scm_i_string_chars (conditions),
                                          scm_i_string_chars (position),
                                          scm_i_string_chars (filling_top_bottom),
                                          scm_i_string_chars (filling_left_right),
                                          scm_i_string_chars (size),
                                          scm_i_string_chars (size_max),
                                          scm_i_string_chars (color_fg),
                                          scm_i_string_chars (color_delim),
                                          scm_i_string_chars (color_bg),
                                          scm_i_string_chars (separator),
                                          scm_i_string_chars (items)));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_bar_set: set a bar property
 */

SCM
weechat_guile_api_bar_set (SCM bar, SCM property, SCM value)
{
    API_FUNC(1, "bar_set", API_RETURN_ERROR);
    if (!scm_is_string (bar) || !scm_is_string (property)
        || !scm_is_string (value))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_bar_set (API_STR2PTR(scm_i_string_chars (bar)),
                     scm_i_string_chars (property),
                     scm_i_string_chars (value));

    API_RETURN_OK;
}

/*
 * weechat_guile_api_bar_update: update a bar on screen
 */

SCM
weechat_guile_api_bar_update (SCM name)
{
    API_FUNC(1, "bar_update", API_RETURN_ERROR);
    if (!scm_is_string (name))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_bar_update (scm_i_string_chars (name));

    API_RETURN_OK;
}

/*
 * weechat_guile_api_bar_remove: remove a bar
 */

SCM
weechat_guile_api_bar_remove (SCM bar)
{
    API_FUNC(1, "bar_remove", API_RETURN_ERROR);
    if (!scm_is_string (bar))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_bar_remove (API_STR2PTR(scm_i_string_chars (bar)));

    API_RETURN_OK;
}

/*
 * weechat_guile_api_command: send command to server
 */

SCM
weechat_guile_api_command (SCM buffer, SCM command)
{
    API_FUNC(1, "command", API_RETURN_ERROR);
    if (!scm_is_string (buffer) || !scm_is_string (command))
        API_WRONG_ARGS(API_RETURN_ERROR);

    plugin_script_api_command (weechat_guile_plugin,
                               guile_current_script,
                               API_STR2PTR(scm_i_string_chars (buffer)),
                               scm_i_string_chars (command));

    API_RETURN_OK;
}

/*
 * weechat_guile_api_info_get: get info (as string)
 */

SCM
weechat_guile_api_info_get (SCM info_name, SCM arguments)
{
    const char *result;

    API_FUNC(1, "info_get", API_RETURN_EMPTY);
    if (!scm_is_string (info_name) || !scm_is_string (arguments))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_info_get (scm_i_string_chars (info_name),
                               scm_i_string_chars (arguments));

    API_RETURN_STRING(result);
}

/*
 * weechat_guile_api_info_get_hashtable: get info (as hashtable)
 */

SCM
weechat_guile_api_info_get_hashtable (SCM info_name, SCM hash)
{
    struct t_hashtable *c_hashtable, *result_hashtable;
    SCM result_alist;

    API_FUNC(1, "info_get_hashtable", API_RETURN_EMPTY);
    if (!scm_is_string (info_name) || !scm_list_p (hash))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    c_hashtable = weechat_guile_alist_to_hashtable (hash,
                                                    WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE);

    result_hashtable = weechat_info_get_hashtable (scm_i_string_chars (info_name),
                                                   c_hashtable);
    result_alist = weechat_guile_hashtable_to_alist (result_hashtable);

    if (c_hashtable)
        weechat_hashtable_free (c_hashtable);
    if (result_hashtable)
        weechat_hashtable_free (result_hashtable);

    return result_alist;
}

/*
 * weechat_guile_api_infolist_new: create new infolist
 */

SCM
weechat_guile_api_infolist_new ()
{
    char *result;
    SCM return_value;

    API_FUNC(1, "infolist_new", API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_infolist_new ());

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_infolist_new_item: create new item in infolist
 */

SCM
weechat_guile_api_infolist_new_item (SCM infolist)
{
    char *result;
    SCM return_value;

    API_FUNC(1, "infolist_new_item", API_RETURN_EMPTY);
    if (!scm_is_string (infolist))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_infolist_new_item (API_STR2PTR(scm_i_string_chars (infolist))));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_infolist_new_var_integer: create new integer variable in
 *                                             infolist
 */

SCM
weechat_guile_api_infolist_new_var_integer (SCM infolist, SCM name, SCM value)
{
    char *result;
    SCM return_value;

    API_FUNC(1, "infolist_new_var_integer", API_RETURN_EMPTY);
    if (!scm_is_string (infolist) || !scm_is_string (name)
        || !scm_is_integer (value))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_infolist_new_var_integer (API_STR2PTR(scm_i_string_chars (infolist)),
                                                           scm_i_string_chars (name),
                                                           scm_to_int (value)));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_infolist_new_var_string: create new string variable in
 *                                            infolist
 */

SCM
weechat_guile_api_infolist_new_var_string (SCM infolist, SCM name, SCM value)
{
    char *result;
    SCM return_value;

    API_FUNC(1, "infolist_new_var_string", API_RETURN_EMPTY);
    if (!scm_is_string (infolist) || !scm_is_string (name)
        || !scm_is_string (value))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_infolist_new_var_string (API_STR2PTR(scm_i_string_chars (infolist)),
                                                          scm_i_string_chars (name),
                                                          scm_i_string_chars (value)));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_infolist_new_var_pointer: create new pointer variable in
 *                                             infolist
 */

SCM
weechat_guile_api_infolist_new_var_pointer (SCM infolist, SCM name, SCM value)
{
    char *result;
    SCM return_value;

    API_FUNC(1, "infolist_new_var_pointer", API_RETURN_EMPTY);
    if (!scm_is_string (infolist) || !scm_is_string (name)
        || !scm_is_string (value))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_infolist_new_var_pointer (API_STR2PTR(scm_i_string_chars (infolist)),
                                                           scm_i_string_chars (name),
                                                           API_STR2PTR(scm_i_string_chars (value))));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_infolist_new_var_time: create new time variable in infolist
 */

SCM
weechat_guile_api_infolist_new_var_time (SCM infolist, SCM name, SCM value)
{
    char *result;
    SCM return_value;

    API_FUNC(1, "infolist_new_var_time", API_RETURN_EMPTY);
    if (!scm_is_string (infolist) || !scm_is_string (name)
        || !scm_is_integer (value))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_infolist_new_var_time (API_STR2PTR(scm_i_string_chars (infolist)),
                                                        scm_i_string_chars (name),
                                                        scm_to_int (value)));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_infolist_get: get list with infos
 */

SCM
weechat_guile_api_infolist_get (SCM name, SCM pointer, SCM arguments)
{
    char *result;
    SCM return_value;

    API_FUNC(1, "infolist_get", API_RETURN_EMPTY);
    if (!scm_is_string (name) || !scm_is_string (pointer)
        || !scm_is_string (arguments))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_infolist_get (scm_i_string_chars (name),
                                               API_STR2PTR(scm_i_string_chars (pointer)),
                                               scm_i_string_chars (arguments)));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_infolist_next: move item pointer to next item in infolist
 */

SCM
weechat_guile_api_infolist_next (SCM infolist)
{
    int value;

    API_FUNC(1, "infolist_next", API_RETURN_INT(0));
    if (!scm_is_string (infolist))
        API_WRONG_ARGS(API_RETURN_INT(0));

    value = weechat_infolist_next (API_STR2PTR(scm_i_string_chars (infolist)));

    API_RETURN_INT(value);
}

/*
 * weechat_guile_api_infolist_prev: move item pointer to previous item in
 *                                  infolist
 */

SCM
weechat_guile_api_infolist_prev (SCM infolist)
{
    int value;

    API_FUNC(1, "infolist_prev", API_RETURN_INT(0));
    if (!scm_is_string (infolist))
        API_WRONG_ARGS(API_RETURN_INT(0));

    value = weechat_infolist_prev (API_STR2PTR(scm_i_string_chars (infolist)));

    API_RETURN_INT(value);
}

/*
 * weechat_guile_api_infolist_reset_item_cursor: reset pointer to current item
 *                                               in infolist
 */

SCM
weechat_guile_api_infolist_reset_item_cursor (SCM infolist)
{
    API_FUNC(1, "infolist_reset_item_cursor", API_RETURN_ERROR);
    if (!scm_is_string (infolist))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_infolist_reset_item_cursor (API_STR2PTR(scm_i_string_chars (infolist)));

    API_RETURN_OK;
}

/*
 * weechat_guile_api_infolist_fields: get list of fields for current item of infolist
 */

SCM
weechat_guile_api_infolist_fields (SCM infolist)
{
    const char *result;

    API_FUNC(1, "infolist_fields", API_RETURN_EMPTY);
    if (!scm_is_string (infolist))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_infolist_fields (API_STR2PTR(scm_i_string_chars (infolist)));

    API_RETURN_STRING(result);
}

/*
 * weechat_guile_api_infolist_integer: get integer value of a variable in
 *                                     infolist
 */

SCM
weechat_guile_api_infolist_integer (SCM infolist, SCM variable)
{
    int value;

    API_FUNC(1, "infolist_integer", API_RETURN_INT(0));
    if (!scm_is_string (infolist) || !scm_is_string (variable))
        API_WRONG_ARGS(API_RETURN_INT(0));

    value = weechat_infolist_integer (API_STR2PTR(scm_i_string_chars (infolist)),
                                      scm_i_string_chars (variable));

    API_RETURN_INT(value);
}

/*
 * weechat_guile_api_infolist_string: get string value of a variable in infolist
 */

SCM
weechat_guile_api_infolist_string (SCM infolist, SCM variable)
{
    const char *result;

    API_FUNC(1, "infolist_string", API_RETURN_EMPTY);
    if (!scm_is_string (infolist) || !scm_is_string (variable))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_infolist_string (API_STR2PTR(scm_i_string_chars (infolist)),
                                      scm_i_string_chars (variable));

    API_RETURN_STRING(result);
}

/*
 * weechat_guile_api_infolist_pointer: get pointer value of a variable in
 *                                     infolist
 */

SCM
weechat_guile_api_infolist_pointer (SCM infolist, SCM variable)
{
    char *result;
    SCM return_value;

    API_FUNC(1, "infolist_pointer", API_RETURN_EMPTY);
    if (!scm_is_string (infolist) || !scm_is_string (variable))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_infolist_pointer (API_STR2PTR(scm_i_string_chars (infolist)),
                                                   scm_i_string_chars (variable)));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_infolist_time: get time value of a variable in infolist
 */

SCM
weechat_guile_api_infolist_time (SCM infolist, SCM variable)
{
    char timebuffer[64], *result;
    time_t time;
    struct tm *date_tmp;
    SCM return_value;

    API_FUNC(1, "infolist_time", API_RETURN_EMPTY);
    if (!scm_is_string (infolist) || !scm_is_string (variable))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    timebuffer[0] = '\0';
    time = weechat_infolist_time (API_STR2PTR(scm_i_string_chars (infolist)),
                                  scm_i_string_chars (variable));
    date_tmp = localtime (&time);
    if (date_tmp)
        strftime (timebuffer, sizeof (timebuffer), "%F %T", date_tmp);
    result = strdup (timebuffer);

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_infolist_free: free infolist
 */

SCM
weechat_guile_api_infolist_free (SCM infolist)
{
    API_FUNC(1, "infolist_free", API_RETURN_ERROR);
    if (!scm_is_string (infolist))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_infolist_free (API_STR2PTR(scm_i_string_chars (infolist)));

    API_RETURN_OK;
}

/*
 * weechat_guile_api_hdata_get: get hdata
 */

SCM
weechat_guile_api_hdata_get (SCM name)
{
    char *result;
    SCM return_value;

    API_FUNC(1, "hdata_get", API_RETURN_EMPTY);
    if (!scm_is_string (name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_hdata_get (scm_i_string_chars (name)));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_hdata_get_var_offset: get offset of variable in hdata
 */

SCM
weechat_guile_api_hdata_get_var_offset (SCM hdata, SCM name)
{
    int value;

    API_FUNC(1, "hdata_get_var_offset", API_RETURN_INT(0));
    if (!scm_is_string (hdata) || !scm_is_string (name))
        API_WRONG_ARGS(API_RETURN_INT(0));

    value = weechat_hdata_get_var_offset (API_STR2PTR(scm_i_string_chars (hdata)),
                                          scm_i_string_chars (name));

    API_RETURN_INT(value);
}

/*
 * weechat_guile_api_hdata_get_var_type_string: get type of variable as string
 *                                              in hdata
 */

SCM
weechat_guile_api_hdata_get_var_type_string (SCM hdata, SCM name)
{
    const char *result;

    API_FUNC(1, "hdata_get_var_type_string", API_RETURN_EMPTY);
    if (!scm_is_string (hdata) || !scm_is_string (name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_hdata_get_var_type_string (API_STR2PTR(scm_i_string_chars (hdata)),
                                                scm_i_string_chars (name));

    API_RETURN_STRING(result);
}

/*
 * weechat_guile_api_hdata_get_var_array_size: get array size for variable in
 *                                             hdata
 */

SCM
weechat_guile_api_hdata_get_var_array_size (SCM hdata, SCM pointer, SCM name)
{
    int value;

    API_FUNC(1, "hdata_get_var_array_size", API_RETURN_INT(-1));
    if (!scm_is_string (hdata) || !scm_is_string (pointer)
        || !scm_is_string (name))
        API_WRONG_ARGS(API_RETURN_INT(-1));

    value = weechat_hdata_get_var_array_size (API_STR2PTR(scm_i_string_chars (hdata)),
                                              API_STR2PTR(scm_i_string_chars (pointer)),
                                              scm_i_string_chars (name));

    API_RETURN_INT(value);
}

/*
 * weechat_guile_api_hdata_get_var_array_size_string: get array size for variable
 *                                                    in hdata (as string)
 */

SCM
weechat_guile_api_hdata_get_var_array_size_string (SCM hdata, SCM pointer,
                                                    SCM name)
{
    const char *result;

    API_FUNC(1, "hdata_get_var_array_size_string", API_RETURN_EMPTY);
    if (!scm_is_string (hdata) || !scm_is_string (pointer)
        || !scm_is_string (name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_hdata_get_var_array_size_string (API_STR2PTR(scm_i_string_chars (hdata)),
                                                      API_STR2PTR(scm_i_string_chars (pointer)),
                                                      scm_i_string_chars (name));

    API_RETURN_STRING(result);
}

/*
 * weechat_guile_api_hdata_get_var_hdata: get hdata for variable in hdata
 */

SCM
weechat_guile_api_hdata_get_var_hdata (SCM hdata, SCM name)
{
    const char *result;

    API_FUNC(1, "hdata_get_var_hdata", API_RETURN_EMPTY);
    if (!scm_is_string (hdata) || !scm_is_string (name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_hdata_get_var_hdata (API_STR2PTR(scm_i_string_chars (hdata)),
                                          scm_i_string_chars (name));

    API_RETURN_STRING(result);
}

/*
 * weechat_guile_api_hdata_get_list: get list pointer in hdata
 */

SCM
weechat_guile_api_hdata_get_list (SCM hdata, SCM name)
{
    char *result;
    SCM return_value;

    API_FUNC(1, "hdata_get_list", API_RETURN_EMPTY);
    if (!scm_is_string (hdata) || !scm_is_string (name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_hdata_get_list (API_STR2PTR(scm_i_string_chars (hdata)),
                                                 scm_i_string_chars (name)));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_hdata_check_pointer: check pointer with hdata/list
 */

SCM
weechat_guile_api_hdata_check_pointer (SCM hdata, SCM list, SCM pointer)
{
    int value;

    API_FUNC(1, "hdata_check_pointer", API_RETURN_INT(0));
    if (!scm_is_string (hdata) || !scm_is_string (list)
        || !scm_is_string (pointer))
        API_WRONG_ARGS(API_RETURN_INT(0));

    value = weechat_hdata_check_pointer (API_STR2PTR(scm_i_string_chars (hdata)),
                                         API_STR2PTR(scm_i_string_chars (list)),
                                         API_STR2PTR(scm_i_string_chars (pointer)));

    API_RETURN_INT(value);
}

/*
 * weechat_guile_api_hdata_move: move pointer to another element in list
 */

SCM
weechat_guile_api_hdata_move (SCM hdata, SCM pointer, SCM count)
{
    char *result;
    SCM return_value;

    API_FUNC(1, "hdata_move", API_RETURN_EMPTY);
    if (!scm_is_string (hdata) || !scm_is_string (pointer)
        || !scm_is_integer (count))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_hdata_move (API_STR2PTR(scm_i_string_chars (hdata)),
                                 API_STR2PTR(scm_i_string_chars (pointer)),
                                 scm_to_int (count));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_hdata_char: get char value of a variable in structure
 *                               using hdata
 */

SCM
weechat_guile_api_hdata_char (SCM hdata, SCM pointer, SCM name)
{
    int value;

    API_FUNC(1, "hdata_char", API_RETURN_INT(0));
    if (!scm_is_string (hdata) || !scm_is_string (pointer)
        || !scm_is_string (name))
        API_WRONG_ARGS(API_RETURN_INT(0));

    value = (int)weechat_hdata_char (API_STR2PTR(scm_i_string_chars (hdata)),
                                     API_STR2PTR(scm_i_string_chars (pointer)),
                                     scm_i_string_chars (name));

    API_RETURN_INT(value);
}

/*
 * weechat_guile_api_hdata_integer: get integer value of a variable in structure
 *                                  using hdata
 */

SCM
weechat_guile_api_hdata_integer (SCM hdata, SCM pointer, SCM name)
{
    int value;

    API_FUNC(1, "hdata_integer", API_RETURN_INT(0));
    if (!scm_is_string (hdata) || !scm_is_string (pointer)
        || !scm_is_string (name))
        API_WRONG_ARGS(API_RETURN_INT(0));

    value = weechat_hdata_integer (API_STR2PTR(scm_i_string_chars (hdata)),
                                   API_STR2PTR(scm_i_string_chars (pointer)),
                                   scm_i_string_chars (name));

    API_RETURN_INT(value);
}

/*
 * weechat_guile_api_hdata_long: get long value of a variable in structure using
 *                               hdata
 */

SCM
weechat_guile_api_hdata_long (SCM hdata, SCM pointer, SCM name)
{
    long value;

    API_FUNC(1, "hdata_long", API_RETURN_LONG(0));
    if (!scm_is_string (hdata) || !scm_is_string (pointer)
        || !scm_is_string (name))
        API_WRONG_ARGS(API_RETURN_LONG(0));

    value = weechat_hdata_long (API_STR2PTR(scm_i_string_chars (hdata)),
                                API_STR2PTR(scm_i_string_chars (pointer)),
                                scm_i_string_chars (name));

    API_RETURN_LONG(value);
}

/*
 * weechat_guile_api_hdata_string: get string value of a variable in structure
 *                                 using hdata
 */

SCM
weechat_guile_api_hdata_string (SCM hdata, SCM pointer, SCM name)
{
    const char *result;

    API_FUNC(1, "hdata_string", API_RETURN_EMPTY);
    if (!scm_is_string (hdata) || !scm_is_string (pointer)
        || !scm_is_string (name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_hdata_string (API_STR2PTR(scm_i_string_chars (hdata)),
                                   API_STR2PTR(scm_i_string_chars (pointer)),
                                   scm_i_string_chars (name));

    API_RETURN_STRING(result);
}

/*
 * weechat_guile_api_hdata_pointer: get pointer value of a variable in structure
 *                                  using hdata
 */

SCM
weechat_guile_api_hdata_pointer (SCM hdata, SCM pointer, SCM name)
{
    char *result;
    SCM return_value;

    API_FUNC(1, "hdata_pointer", API_RETURN_EMPTY);
    if (!scm_is_string (hdata) || !scm_is_string (pointer)
        || !scm_is_string (name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_hdata_pointer (API_STR2PTR(scm_i_string_chars (hdata)),
                                                API_STR2PTR(scm_i_string_chars (pointer)),
                                                scm_i_string_chars (name)));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_hdata_time: get time value of a variable in structure using
 *                               hdata
 */

SCM
weechat_guile_api_hdata_time (SCM hdata, SCM pointer, SCM name)
{
    char timebuffer[64], *result;
    time_t time;
    SCM return_value;

    API_FUNC(1, "hdata_time", API_RETURN_EMPTY);
    if (!scm_is_string (hdata) || !scm_is_string (pointer) || !scm_is_string (name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    timebuffer[0] = '\0';
    time = weechat_hdata_time (API_STR2PTR(scm_i_string_chars (hdata)),
                               API_STR2PTR(scm_i_string_chars (pointer)),
                               scm_i_string_chars (name));
    snprintf (timebuffer, sizeof (timebuffer), "%ld", (long int)time);
    result = strdup (timebuffer);

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_hdata_hashtable: get hashtable value of a variable in
 *                                    structure using hdata
 */

SCM
weechat_guile_api_hdata_hashtable (SCM hdata, SCM pointer, SCM name)
{
    SCM result_alist;

    API_FUNC(1, "hdata_hashtable", API_RETURN_EMPTY);
    if (!scm_is_string (hdata) || !scm_is_string (pointer)
        || !scm_is_string (name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result_alist = weechat_guile_hashtable_to_alist (
        weechat_hdata_hashtable (API_STR2PTR(scm_i_string_chars (hdata)),
                                 API_STR2PTR(scm_i_string_chars (pointer)),
                                 scm_i_string_chars (name)));

    return result_alist;
}

/*
 * weechat_guile_api_hdata_update: update data in a hdata
 */

SCM
weechat_guile_api_hdata_update (SCM hdata, SCM pointer, SCM hashtable)
{
    struct t_hashtable *c_hashtable;
    int value;

    API_FUNC(1, "hdata_update", API_RETURN_INT(0));
    if (!scm_is_string (hdata) || !scm_is_string (pointer) || !scm_list_p (hashtable))
        API_WRONG_ARGS(API_RETURN_INT(0));

    c_hashtable = weechat_guile_alist_to_hashtable (hashtable,
                                                    WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE);

    value = weechat_hdata_update (API_STR2PTR(scm_i_string_chars (hdata)),
                                  API_STR2PTR(scm_i_string_chars (pointer)),
                                  c_hashtable);

    if (c_hashtable)
        weechat_hashtable_free (c_hashtable);

    API_RETURN_INT(value);
}

/*
 * weechat_guile_api_hdata_get_string: get hdata property as string
 */

SCM
weechat_guile_api_hdata_get_string (SCM hdata, SCM property)
{
    const char *result;

    API_FUNC(1, "hdata_get_string", API_RETURN_EMPTY);
    if (!scm_is_string (hdata) || !scm_is_string (property))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_hdata_get_var_type_string (API_STR2PTR(scm_i_string_chars (hdata)),
                                                scm_i_string_chars (property));

    API_RETURN_STRING(result);
}

/*
 * weechat_guile_api_upgrade_new: create an upgrade file
 */

SCM
weechat_guile_api_upgrade_new (SCM filename, SCM write)
{
    char *result;
    SCM return_value;

    API_FUNC(1, "upgrade_new", API_RETURN_EMPTY);
    if (!scm_is_string (filename) || !scm_is_integer (write))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_upgrade_new (scm_i_string_chars (filename),
                                              scm_to_int (write)));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_guile_api_upgrade_write_object: write object in upgrade file
 */

SCM
weechat_guile_api_upgrade_write_object (SCM upgrade_file, SCM object_id,
                                        SCM infolist)
{
    int rc;

    API_FUNC(1, "upgrade_write_object", API_RETURN_INT(0));
    if (!scm_is_string (upgrade_file) || !scm_is_integer (object_id)
        || !scm_is_string (infolist))
        API_WRONG_ARGS(API_RETURN_INT(0));

    rc = weechat_upgrade_write_object (API_STR2PTR(scm_i_string_chars (upgrade_file)),
                                       scm_to_int (object_id),
                                       API_STR2PTR(scm_i_string_chars (infolist)));

    API_RETURN_INT(rc);
}

/*
 * weechat_guile_api_upgrade_read_cb: callback for reading object in upgrade
 *                                    file
 */

int
weechat_guile_api_upgrade_read_cb (void *data,
                                   struct t_upgrade_file *upgrade_file,
                                   int object_id,
                                   struct t_infolist *infolist)
{
    struct t_plugin_script_cb *script_callback;
    void *func_argv[4];
    char empty_arg[1] = { '\0' }, str_object_id[32];
    int *rc, ret;

    script_callback = (struct t_plugin_script_cb *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        snprintf (str_object_id, sizeof (str_object_id), "%d", object_id);

        func_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        func_argv[1] = API_PTR2STR(upgrade_file);
        func_argv[2] = str_object_id;
        func_argv[3] = API_PTR2STR(infolist);

        rc = (int *) weechat_guile_exec (script_callback->script,
                                         WEECHAT_SCRIPT_EXEC_INT,
                                         script_callback->function,
                                         "ssss", func_argv);

        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (func_argv[1])
            free (func_argv[1]);
        if (func_argv[3])
            free (func_argv[3]);

        return ret;
    }

    return WEECHAT_RC_ERROR;
}

/*
 * weechat_guile_api_upgrade_read: read upgrade file
 */

SCM
weechat_guile_api_upgrade_read (SCM upgrade_file, SCM function, SCM data)
{
    int rc;

    API_FUNC(1, "upgrade_read", API_RETURN_INT(0));
    if (!scm_is_string (upgrade_file) || !scm_is_string (function)
        || !scm_is_string (data))
        API_WRONG_ARGS(API_RETURN_INT(0));

    rc = plugin_script_api_upgrade_read (weechat_guile_plugin,
                                         guile_current_script,
                                         API_STR2PTR(scm_i_string_chars (upgrade_file)),
                                         &weechat_guile_api_upgrade_read_cb,
                                         scm_i_string_chars (function),
                                         scm_i_string_chars (data));

    API_RETURN_INT(rc);
}

/*
 * weechat_guile_api_upgrade_close: close upgrade file
 */

SCM
weechat_guile_api_upgrade_close (SCM upgrade_file)
{
    API_FUNC(1, "upgrade_close", API_RETURN_ERROR);
    if (!scm_is_string (upgrade_file))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_upgrade_close (API_STR2PTR(scm_i_string_chars (upgrade_file)));

    API_RETURN_OK;
}

/*
 * weechat_guile_api_module_init: init main module with API
 */

void
weechat_guile_api_module_init (void *data)
{
    scm_t_bits port_type;

    /* make C compiler happy */
    (void) data;

    port_type = scm_make_port_type ("weechat_stdout",
                                    &weechat_guile_port_fill_input,
                                    &weechat_guile_port_write);
    guile_port = scm_new_port_table_entry (port_type);
    SCM_SET_CELL_TYPE (guile_port, port_type | SCM_OPN | SCM_WRTNG);
    scm_set_current_output_port (guile_port);
    scm_set_current_error_port (guile_port);

    /* interface functions */
    API_DEF_FUNC(register, 7);
    API_DEF_FUNC(plugin_get_name, 1);
    API_DEF_FUNC(charset_set, 1);
    API_DEF_FUNC(iconv_to_internal, 2);
    API_DEF_FUNC(iconv_from_internal, 2);
    API_DEF_FUNC(gettext, 1);
    API_DEF_FUNC(ngettext, 3);
    API_DEF_FUNC(string_match, 3);
    API_DEF_FUNC(string_has_highlight, 2);
    API_DEF_FUNC(string_has_highlight_regex, 2);
    API_DEF_FUNC(string_mask_to_regex, 1);
    API_DEF_FUNC(string_remove_color, 2);
    API_DEF_FUNC(string_is_command_char, 1);
    API_DEF_FUNC(string_input_for_buffer, 1);
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
    API_DEF_FUNC(config_option_is_null, 1);
    API_DEF_FUNC(config_option_default_is_null, 1);
    API_DEF_FUNC(config_boolean, 1);
    API_DEF_FUNC(config_boolean_default, 1);
    API_DEF_FUNC(config_integer, 1);
    API_DEF_FUNC(config_integer_default, 1);
    API_DEF_FUNC(config_string, 1);
    API_DEF_FUNC(config_string_default, 1);
    API_DEF_FUNC(config_color, 1);
    API_DEF_FUNC(config_color_default, 1);
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
    API_DEF_FUNC(print_y, 3);
    API_DEF_FUNC(log_print, 1);
    API_DEF_FUNC(hook_command, 7);
    API_DEF_FUNC(hook_command_run, 3);
    API_DEF_FUNC(hook_timer, 5);
    API_DEF_FUNC(hook_fd, 6);
    API_DEF_FUNC(hook_process, 4);
    API_DEF_FUNC(hook_process_hashtable, 5);
    API_DEF_FUNC(hook_connect, 8);
    API_DEF_FUNC(hook_print, 6);
    API_DEF_FUNC(hook_signal, 3);
    API_DEF_FUNC(hook_signal_send, 3);
    API_DEF_FUNC(hook_hsignal, 3);
    API_DEF_FUNC(hook_hsignal_send, 2);
    API_DEF_FUNC(hook_config, 3);
    API_DEF_FUNC(hook_completion, 4);
    API_DEF_FUNC(hook_completion_list_add, 4);
    API_DEF_FUNC(hook_modifier, 3);
    API_DEF_FUNC(hook_modifier_exec, 3);
    API_DEF_FUNC(hook_info, 5);
    API_DEF_FUNC(hook_info_hashtable, 6);
    API_DEF_FUNC(hook_infolist, 6);
    API_DEF_FUNC(hook_focus, 3);
    API_DEF_FUNC(unhook, 1);
    API_DEF_FUNC(unhook_all, 0);
    API_DEF_FUNC(buffer_new, 5);
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
    API_DEF_FUNC(info_get, 2);
    API_DEF_FUNC(info_get_hashtable, 2);
    API_DEF_FUNC(infolist_new, 0);
    API_DEF_FUNC(infolist_new_item, 1);
    API_DEF_FUNC(infolist_new_var_integer, 3);
    API_DEF_FUNC(infolist_new_var_string, 3);
    API_DEF_FUNC(infolist_new_var_pointer, 3);
    API_DEF_FUNC(infolist_new_var_time, 3);
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
    API_DEF_FUNC(hdata_char, 3);
    API_DEF_FUNC(hdata_integer, 3);
    API_DEF_FUNC(hdata_long, 3);
    API_DEF_FUNC(hdata_string, 3);
    API_DEF_FUNC(hdata_pointer, 3);
    API_DEF_FUNC(hdata_time, 3);
    API_DEF_FUNC(hdata_hashtable, 3);
    API_DEF_FUNC(hdata_update, 3);
    API_DEF_FUNC(hdata_get_string, 2);
    API_DEF_FUNC(upgrade_new, 2);
    API_DEF_FUNC(upgrade_write_object, 3);
    API_DEF_FUNC(upgrade_read, 3);
    API_DEF_FUNC(upgrade_close, 1);

    /* interface constants */
    scm_c_define ("weechat:WEECHAT_RC_OK", scm_from_int (WEECHAT_RC_OK));
    scm_c_define ("weechat:WEECHAT_RC_OK_EAT", scm_from_int (WEECHAT_RC_OK_EAT));
    scm_c_define ("weechat:WEECHAT_RC_ERROR", scm_from_int (WEECHAT_RC_ERROR));

    scm_c_define ("weechat:WEECHAT_CONFIG_READ_OK", scm_from_int (WEECHAT_CONFIG_READ_OK));
    scm_c_define ("weechat:WEECHAT_CONFIG_READ_MEMORY_ERROR", scm_from_int (WEECHAT_CONFIG_READ_MEMORY_ERROR));
    scm_c_define ("weechat:WEECHAT_CONFIG_READ_FILE_NOT_FOUND", scm_from_int (WEECHAT_CONFIG_READ_FILE_NOT_FOUND));
    scm_c_define ("weechat:WEECHAT_CONFIG_WRITE_OK", scm_from_int (WEECHAT_CONFIG_WRITE_OK));
    scm_c_define ("weechat:WEECHAT_CONFIG_WRITE_ERROR", scm_from_int (WEECHAT_CONFIG_WRITE_ERROR));
    scm_c_define ("weechat:WEECHAT_CONFIG_WRITE_MEMORY_ERROR", scm_from_int (WEECHAT_CONFIG_WRITE_MEMORY_ERROR));
    scm_c_define ("weechat:WEECHAT_CONFIG_OPTION_SET_OK_CHANGED", scm_from_int (WEECHAT_CONFIG_OPTION_SET_OK_CHANGED));
    scm_c_define ("weechat:WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE", scm_from_int (WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE));
    scm_c_define ("weechat:WEECHAT_CONFIG_OPTION_SET_ERROR", scm_from_int (WEECHAT_CONFIG_OPTION_SET_ERROR));
    scm_c_define ("weechat:WEECHAT_CONFIG_OPTION_SET_OPTION_NOT_FOUND", scm_from_int (WEECHAT_CONFIG_OPTION_SET_OPTION_NOT_FOUND));
    scm_c_define ("weechat:WEECHAT_CONFIG_OPTION_UNSET_OK_NO_RESET", scm_from_int (WEECHAT_CONFIG_OPTION_UNSET_OK_NO_RESET));
    scm_c_define ("weechat:WEECHAT_CONFIG_OPTION_UNSET_OK_RESET", scm_from_int (WEECHAT_CONFIG_OPTION_UNSET_OK_RESET));
    scm_c_define ("weechat:WEECHAT_CONFIG_OPTION_UNSET_OK_REMOVED", scm_from_int (WEECHAT_CONFIG_OPTION_UNSET_OK_REMOVED));
    scm_c_define ("weechat:WEECHAT_CONFIG_OPTION_UNSET_ERROR", scm_from_int (WEECHAT_CONFIG_OPTION_UNSET_ERROR));

    scm_c_define ("weechat:WEECHAT_LIST_POS_SORT", scm_from_locale_string (WEECHAT_LIST_POS_SORT));
    scm_c_define ("weechat:WEECHAT_LIST_POS_BEGINNING", scm_from_locale_string (WEECHAT_LIST_POS_BEGINNING));
    scm_c_define ("weechat:WEECHAT_LIST_POS_END", scm_from_locale_string (WEECHAT_LIST_POS_END));

    scm_c_define ("weechat:WEECHAT_HOTLIST_LOW", scm_from_locale_string (WEECHAT_HOTLIST_LOW));
    scm_c_define ("weechat:WEECHAT_HOTLIST_MESSAGE", scm_from_locale_string (WEECHAT_HOTLIST_MESSAGE));
    scm_c_define ("weechat:WEECHAT_HOTLIST_PRIVATE", scm_from_locale_string (WEECHAT_HOTLIST_PRIVATE));
    scm_c_define ("weechat:WEECHAT_HOTLIST_HIGHLIGHT", scm_from_locale_string (WEECHAT_HOTLIST_HIGHLIGHT));

    scm_c_define ("weechat:WEECHAT_HOOK_PROCESS_RUNNING", scm_from_int (WEECHAT_HOOK_PROCESS_RUNNING));
    scm_c_define ("weechat:WEECHAT_HOOK_PROCESS_ERROR", scm_from_int (WEECHAT_HOOK_PROCESS_ERROR));

    scm_c_define ("weechat:WEECHAT_HOOK_CONNECT_OK", scm_from_int (WEECHAT_HOOK_CONNECT_OK));
    scm_c_define ("weechat:WEECHAT_HOOK_CONNECT_ADDRESS_NOT_FOUND", scm_from_int (WEECHAT_HOOK_CONNECT_ADDRESS_NOT_FOUND));
    scm_c_define ("weechat:WEECHAT_HOOK_CONNECT_IP_ADDRESS_NOT_FOUND", scm_from_int (WEECHAT_HOOK_CONNECT_IP_ADDRESS_NOT_FOUND));
    scm_c_define ("weechat:WEECHAT_HOOK_CONNECT_CONNECTION_REFUSED", scm_from_int (WEECHAT_HOOK_CONNECT_CONNECTION_REFUSED));
    scm_c_define ("weechat:WEECHAT_HOOK_CONNECT_PROXY_ERROR", scm_from_int (WEECHAT_HOOK_CONNECT_PROXY_ERROR));
    scm_c_define ("weechat:WEECHAT_HOOK_CONNECT_LOCAL_HOSTNAME_ERROR", scm_from_int (WEECHAT_HOOK_CONNECT_LOCAL_HOSTNAME_ERROR));
    scm_c_define ("weechat:WEECHAT_HOOK_CONNECT_GNUTLS_INIT_ERROR", scm_from_int (WEECHAT_HOOK_CONNECT_GNUTLS_INIT_ERROR));
    scm_c_define ("weechat:WEECHAT_HOOK_CONNECT_GNUTLS_HANDSHAKE_ERROR", scm_from_int (WEECHAT_HOOK_CONNECT_GNUTLS_HANDSHAKE_ERROR));
    scm_c_define ("weechat:WEECHAT_HOOK_CONNECT_MEMORY_ERROR", scm_from_int (WEECHAT_HOOK_CONNECT_MEMORY_ERROR));
    scm_c_define ("weechat:WEECHAT_HOOK_CONNECT_TIMEOUT", scm_from_int (WEECHAT_HOOK_CONNECT_TIMEOUT));
    scm_c_define ("weechat:WEECHAT_HOOK_CONNECT_SOCKET_ERROR", scm_from_int (WEECHAT_HOOK_CONNECT_SOCKET_ERROR));

    scm_c_define ("weechat:WEECHAT_HOOK_SIGNAL_STRING", scm_from_locale_string (WEECHAT_HOOK_SIGNAL_STRING));
    scm_c_define ("weechat:WEECHAT_HOOK_SIGNAL_INT", scm_from_locale_string (WEECHAT_HOOK_SIGNAL_INT));
    scm_c_define ("weechat:WEECHAT_HOOK_SIGNAL_POINTER", scm_from_locale_string (WEECHAT_HOOK_SIGNAL_POINTER));

    scm_c_export ("weechat:WEECHAT_RC_OK",
                  "weechat:WEECHAT_RC_OK_EAT",
                  "weechat:WEECHAT_RC_ERROR",
                  "weechat:WEECHAT_CONFIG_READ_OK",
                  "weechat:WEECHAT_CONFIG_READ_MEMORY_ERROR",
                  "weechat:WEECHAT_CONFIG_READ_FILE_NOT_FOUND",
                  "weechat:WEECHAT_CONFIG_WRITE_OK",
                  "weechat:WEECHAT_CONFIG_WRITE_ERROR",
                  "weechat:WEECHAT_CONFIG_WRITE_MEMORY_ERROR",
                  "weechat:WEECHAT_CONFIG_OPTION_SET_OK_CHANGED",
                  "weechat:WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE",
                  "weechat:WEECHAT_CONFIG_OPTION_SET_ERROR",
                  "weechat:WEECHAT_CONFIG_OPTION_SET_OPTION_NOT_FOUND",
                  "weechat:WEECHAT_CONFIG_OPTION_UNSET_OK_NO_RESET",
                  "weechat:WEECHAT_CONFIG_OPTION_UNSET_OK_RESET",
                  "weechat:WEECHAT_CONFIG_OPTION_UNSET_OK_REMOVED",
                  "weechat:WEECHAT_CONFIG_OPTION_UNSET_ERROR",
                  "weechat:WEECHAT_LIST_POS_SORT",
                  "weechat:WEECHAT_LIST_POS_BEGINNING",
                  "weechat:WEECHAT_LIST_POS_END",
                  "weechat:WEECHAT_HOTLIST_LOW",
                  "weechat:WEECHAT_HOTLIST_MESSAGE",
                  "weechat:WEECHAT_HOTLIST_PRIVATE",
                  "weechat:WEECHAT_HOTLIST_HIGHLIGHT",
                  "weechat:WEECHAT_HOOK_PROCESS_RUNNING",
                  "weechat:WEECHAT_HOOK_PROCESS_ERROR",
                  "weechat:WEECHAT_HOOK_CONNECT_OK",
                  "weechat:WEECHAT_HOOK_CONNECT_ADDRESS_NOT_FOUND",
                  "weechat:WEECHAT_HOOK_CONNECT_IP_ADDRESS_NOT_FOUND",
                  "weechat:WEECHAT_HOOK_CONNECT_CONNECTION_REFUSED",
                  "weechat:WEECHAT_HOOK_CONNECT_PROXY_ERROR",
                  "weechat:WEECHAT_HOOK_CONNECT_LOCAL_HOSTNAME_ERROR",
                  "weechat:WEECHAT_HOOK_CONNECT_GNUTLS_INIT_ERROR",
                  "weechat:WEECHAT_HOOK_CONNECT_GNUTLS_HANDSHAKE_ERROR",
                  "weechat:WEECHAT_HOOK_CONNECT_MEMORY_ERROR",
                  "weechat:WEECHAT_HOOK_CONNECT_TIMEOUT",
                  "weechat:WEECHAT_HOOK_CONNECT_SOCKET_ERROR",
                  "weechat:WEECHAT_HOOK_SIGNAL_STRING",
                  "weechat:WEECHAT_HOOK_SIGNAL_INT",
                  "weechat:WEECHAT_HOOK_SIGNAL_POINTER",
                  NULL);
}
