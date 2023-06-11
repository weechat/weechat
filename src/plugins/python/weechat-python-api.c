/*
 * weechat-python-api.c - python API functions
 *
 * Copyright (C) 2003-2023 Sébastien Helleu <flashcode@flashtux.org>
 * Copyright (C) 2005-2007 Emmanuel Bouthenot <kolter@openics.org>
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

#include <Python.h>
#include <time.h>

#include "../weechat-plugin.h"
#include "../plugin-script.h"
#include "../plugin-script-api.h"
#include "weechat-python.h"


#define API_DEF_FUNC(__name)                                            \
    { #__name, &weechat_python_api_##__name, METH_VARARGS, "" }
#define API_FUNC(__name)                                                \
    static PyObject *                                                   \
    weechat_python_api_##__name (PyObject *self, PyObject *args)
#define API_INIT_FUNC(__init, __name, __ret)                            \
    char *python_function_name = __name;                                \
    (void) self;                                                        \
    if (__init                                                          \
        && (!python_current_script || !python_current_script->name))    \
    {                                                                   \
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME,         \
                                    python_function_name);              \
        __ret;                                                          \
    }
#define API_WRONG_ARGS(__ret)                                           \
    {                                                                   \
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME,       \
                                      python_function_name);            \
        __ret;                                                          \
    }
#define API_PTR2STR(__pointer)                                          \
    plugin_script_ptr2str (__pointer)
#define API_STR2PTR(__string)                                           \
    plugin_script_str2ptr (weechat_python_plugin,                       \
                           PYTHON_CURRENT_SCRIPT_NAME,                  \
                           python_function_name, __string)
#define API_RETURN_OK return PyLong_FromLong((long)1)
#define API_RETURN_ERROR return PyLong_FromLong ((long)0)
#define API_RETURN_EMPTY                                                \
    Py_INCREF (Py_None);                                                \
    return Py_None
#define API_RETURN_STRING(__string)                                     \
    if (__string)                                                       \
        return Py_BuildValue ("s", __string);                           \
    return Py_BuildValue ("s", "")
#define API_RETURN_STRING_FREE(__string)                                \
    if (__string)                                                       \
    {                                                                   \
        return_value = Py_BuildValue ("s", __string);                   \
        free (__string);                                                \
        return return_value;                                            \
    }                                                                   \
    return Py_BuildValue ("s", "")
#define API_RETURN_INT(__int)                                           \
    return PyLong_FromLong((long)__int)
#define API_RETURN_LONG(__long)                                         \
    return PyLong_FromLong(__long)


/*
 * Registers a python script.
 */

API_FUNC(register)
{
    char *name, *author, *version, *license, *shutdown_func, *description;
    char *charset;

    API_INIT_FUNC(0, "register", API_RETURN_ERROR);
    if (python_registered_script)
    {
        /* script already registered */
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: script \"%s\" already "
                                         "registered (register ignored)"),
                        weechat_prefix ("error"), PYTHON_PLUGIN_NAME,
                        python_registered_script->name);
        API_RETURN_ERROR;
    }
    python_current_script = NULL;
    python_registered_script = NULL;
    name = NULL;
    author = NULL;
    version = NULL;
    license = NULL;
    description = NULL;
    shutdown_func = NULL;
    charset = NULL;
    if (!PyArg_ParseTuple (args, "sssssss", &name, &author, &version,
                           &license, &description, &shutdown_func, &charset))
        API_WRONG_ARGS(API_RETURN_ERROR);

    if (plugin_script_search (python_scripts, name))
    {
        /* another script already exists with same name */
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to register script "
                                         "\"%s\" (another script already "
                                         "exists with this name)"),
                        weechat_prefix ("error"), PYTHON_PLUGIN_NAME, name);
        API_RETURN_ERROR;
    }

    /* register script */
    python_current_script = plugin_script_add (weechat_python_plugin,
                                               &python_data,
                                               (python_current_script_filename) ?
                                               python_current_script_filename : "",
                                               name, author, version, license,
                                               description, shutdown_func, charset);
    if (python_current_script)
    {
        python_registered_script = python_current_script;
        if ((weechat_python_plugin->debug >= 2) || !python_quiet)
        {
            weechat_printf (NULL,
                            weechat_gettext ("%s: registered script \"%s\", "
                                             "version %s (%s)"),
                            PYTHON_PLUGIN_NAME, name, version, description);
        }
        python_current_script->interpreter = (PyThreadState *)python_current_interpreter;
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

API_FUNC(plugin_get_name)
{
    char *plugin;
    const char *result;

    API_INIT_FUNC(1, "plugin_get_name", API_RETURN_EMPTY);
    plugin = NULL;
    if (!PyArg_ParseTuple (args, "s", &plugin))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_plugin_get_name (API_STR2PTR(plugin));

    API_RETURN_STRING(result);
}

API_FUNC(charset_set)
{
    char *charset;

    API_INIT_FUNC(1, "charset_set", API_RETURN_ERROR);
    charset = NULL;
    if (!PyArg_ParseTuple (args, "s", &charset))
        API_WRONG_ARGS(API_RETURN_ERROR);

    plugin_script_api_charset_set (python_current_script, charset);

    API_RETURN_OK;
}

API_FUNC(iconv_to_internal)
{
    char *charset, *string, *result;
    PyObject *return_value;

    API_INIT_FUNC(1, "iconv_to_internal", API_RETURN_EMPTY);
    charset = NULL;
    string = NULL;
    if (!PyArg_ParseTuple (args, "ss", &charset, &string))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_iconv_to_internal (charset, string);

    API_RETURN_STRING_FREE(result);
}

API_FUNC(iconv_from_internal)
{
    char *charset, *string, *result;
    PyObject *return_value;

    API_INIT_FUNC(1, "iconv_from_internal", API_RETURN_EMPTY);
    charset = NULL;
    string = NULL;
    if (!PyArg_ParseTuple (args, "ss", &charset, &string))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_iconv_from_internal (charset, string);

    API_RETURN_STRING_FREE(result);
}

API_FUNC(gettext)
{
    char *string;
    const char *result;

    API_INIT_FUNC(1, "gettext", API_RETURN_EMPTY);
    string = NULL;
    if (!PyArg_ParseTuple (args, "s", &string))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_gettext (string);

    API_RETURN_STRING(result);
}

API_FUNC(ngettext)
{
    char *single, *plural;
    const char *result;
    int count;

    API_INIT_FUNC(1, "ngettext", API_RETURN_EMPTY);
    single = NULL;
    plural = NULL;
    count = 0;
    if (!PyArg_ParseTuple (args, "ssi", &single, &plural, &count))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_ngettext (single, plural, count);

    API_RETURN_STRING(result);
}

API_FUNC(strlen_screen)
{
    char *string;
    int value;

    API_INIT_FUNC(1, "strlen_screen", API_RETURN_INT(0));
    string = NULL;
    if (!PyArg_ParseTuple (args, "s", &string))
        API_WRONG_ARGS(API_RETURN_INT(0));

    value = weechat_strlen_screen (string);

    API_RETURN_INT(value);
}

API_FUNC(string_match)
{
    char *string, *mask;
    int case_sensitive, value;

    API_INIT_FUNC(1, "string_match", API_RETURN_INT(0));
    string = NULL;
    mask = NULL;
    case_sensitive = 0;
    if (!PyArg_ParseTuple (args, "ssi", &string, &mask, &case_sensitive))
        API_WRONG_ARGS(API_RETURN_INT(0));

    value = weechat_string_match (string, mask, case_sensitive);

    API_RETURN_INT(value);
}

API_FUNC(string_match_list)
{
    char *string, *masks;
    int case_sensitive, value;

    API_INIT_FUNC(1, "string_match_list", API_RETURN_INT(0));
    string = NULL;
    masks = NULL;
    case_sensitive = 0;
    if (!PyArg_ParseTuple (args, "ssi", &string, &masks, &case_sensitive))
        API_WRONG_ARGS(API_RETURN_INT(0));

    value = plugin_script_api_string_match_list (weechat_python_plugin,
                                                 string,
                                                 masks,
                                                 case_sensitive);

    API_RETURN_INT(value);
}

API_FUNC(string_has_highlight)
{
    char *string, *highlight_words;
    int value;

    API_INIT_FUNC(1, "string_has_highlight", API_RETURN_INT(0));
    string = NULL;
    highlight_words = NULL;
    if (!PyArg_ParseTuple (args, "ss", &string, &highlight_words))
        API_WRONG_ARGS(API_RETURN_INT(0));

    value = weechat_string_has_highlight (string, highlight_words);

    API_RETURN_INT(value);
}

API_FUNC(string_has_highlight_regex)
{
    char *string, *regex;
    int value;

    API_INIT_FUNC(1, "string_has_highlight_regex", API_RETURN_INT(0));
    string = NULL;
    regex = NULL;
    if (!PyArg_ParseTuple (args, "ss", &string, &regex))
        API_WRONG_ARGS(API_RETURN_INT(0));

    value = weechat_string_has_highlight_regex (string, regex);

    API_RETURN_INT(value);
}

API_FUNC(string_mask_to_regex)
{
    char *mask, *result;
    PyObject *return_value;

    API_INIT_FUNC(1, "string_mask_to_regex", API_RETURN_EMPTY);
    mask = NULL;
    if (!PyArg_ParseTuple (args, "s", &mask))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_string_mask_to_regex (mask);

    API_RETURN_STRING_FREE(result);
}

API_FUNC(string_format_size)
{
    unsigned long long size;
    char *result;
    PyObject *return_value;

    API_INIT_FUNC(1, "string_format_size", API_RETURN_EMPTY);
    size = 0;
    if (!PyArg_ParseTuple (args, "K", &size))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_string_format_size (size);

    API_RETURN_STRING_FREE(result);
}

API_FUNC(string_parse_size)
{
    char *size;
    unsigned long long value;

    API_INIT_FUNC(1, "string_parse_size", API_RETURN_LONG(0));
    size = NULL;
    if (!PyArg_ParseTuple (args, "s", &size))
        API_WRONG_ARGS(API_RETURN_LONG(0));

    value = weechat_string_parse_size (size);

    API_RETURN_LONG(value);
}

API_FUNC(string_color_code_size)
{
    char *string;
    int size;

    API_INIT_FUNC(1, "string_color_code_size", API_RETURN_INT(0));
    string = NULL;
    if (!PyArg_ParseTuple (args, "s", &string))
        API_WRONG_ARGS(API_RETURN_INT(0));

    size = weechat_string_color_code_size (string);

    API_RETURN_INT(size);
}

API_FUNC(string_remove_color)
{
    char *string, *replacement, *result;
    PyObject *return_value;

    API_INIT_FUNC(1, "string_remove_color", API_RETURN_EMPTY);
    string = NULL;
    replacement = NULL;
    if (!PyArg_ParseTuple (args, "ss", &string, &replacement))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_string_remove_color (string, replacement);

    API_RETURN_STRING_FREE(result);
}

API_FUNC(string_is_command_char)
{
    char *string;
    int value;

    API_INIT_FUNC(1, "string_is_command_char", API_RETURN_INT(0));
    string = NULL;
    if (!PyArg_ParseTuple (args, "s", &string))
        API_WRONG_ARGS(API_RETURN_INT(0));

    value = weechat_string_is_command_char (string);

    API_RETURN_INT(value);
}

API_FUNC(string_input_for_buffer)
{
    char *string;
    const char *result;

    API_INIT_FUNC(1, "string_input_for_buffer", API_RETURN_EMPTY);
    string = NULL;
    if (!PyArg_ParseTuple (args, "s", &string))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_string_input_for_buffer (string);

    API_RETURN_STRING(result);
}

API_FUNC(string_eval_expression)
{
    char *expr, *result;
    struct t_hashtable *pointers, *extra_vars, *options;
    PyObject *dict, *dict2, *dict3, *return_value;

    API_INIT_FUNC(1, "string_eval_expression", API_RETURN_EMPTY);
    expr = NULL;
    pointers = NULL;
    extra_vars = NULL;
    options = NULL;
    if (!PyArg_ParseTuple (args, "sOOO", &expr, &dict, &dict2, &dict3))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    pointers = weechat_python_dict_to_hashtable (dict,
                                                 WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                                 WEECHAT_HASHTABLE_STRING,
                                                 WEECHAT_HASHTABLE_POINTER);
    extra_vars = weechat_python_dict_to_hashtable (dict2,
                                                   WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                                   WEECHAT_HASHTABLE_STRING,
                                                   WEECHAT_HASHTABLE_STRING);
    options = weechat_python_dict_to_hashtable (dict3,
                                                WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                                WEECHAT_HASHTABLE_STRING,
                                                WEECHAT_HASHTABLE_STRING);
    result = weechat_string_eval_expression (expr, pointers, extra_vars,
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
    char *path, *result;
    struct t_hashtable *pointers, *extra_vars, *options;
    PyObject *dict, *dict2, *dict3, *return_value;

    API_INIT_FUNC(1, "string_eval_path_home", API_RETURN_EMPTY);
    path = NULL;
    pointers = NULL;
    extra_vars = NULL;
    options = NULL;
    if (!PyArg_ParseTuple (args, "sOOO", &path, &dict, &dict2, &dict3))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    pointers = weechat_python_dict_to_hashtable (dict,
                                                 WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                                 WEECHAT_HASHTABLE_STRING,
                                                 WEECHAT_HASHTABLE_POINTER);
    extra_vars = weechat_python_dict_to_hashtable (dict2,
                                                   WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                                   WEECHAT_HASHTABLE_STRING,
                                                   WEECHAT_HASHTABLE_STRING);
    options = weechat_python_dict_to_hashtable (dict3,
                                                WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                                WEECHAT_HASHTABLE_STRING,
                                                WEECHAT_HASHTABLE_STRING);
    result = weechat_string_eval_path_home (path, pointers, extra_vars,
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
    char *directory;
    int mode;

    API_INIT_FUNC(1, "mkdir_home", API_RETURN_ERROR);
    directory = NULL;
    mode = 0;
    if (!PyArg_ParseTuple (args, "si", &directory, &mode))
        API_WRONG_ARGS(API_RETURN_ERROR);

    if (weechat_mkdir_home (directory, mode))
        API_RETURN_OK;

    API_RETURN_ERROR;
}

API_FUNC(mkdir)
{
    char *directory;
    int mode;

    API_INIT_FUNC(1, "mkdir", API_RETURN_ERROR);
    directory = NULL;
    mode = 0;
    if (!PyArg_ParseTuple (args, "si", &directory, &mode))
        API_WRONG_ARGS(API_RETURN_ERROR);

    if (weechat_mkdir (directory, mode))
        API_RETURN_OK;

    API_RETURN_ERROR;
}

API_FUNC(mkdir_parents)
{
    char *directory;
    int mode;

    API_INIT_FUNC(1, "mkdir_parents", API_RETURN_ERROR);
    directory = NULL;
    mode = 0;
    if (!PyArg_ParseTuple (args, "si", &directory, &mode))
        API_WRONG_ARGS(API_RETURN_ERROR);

    if (weechat_mkdir_parents (directory, mode))
        API_RETURN_OK;

    API_RETURN_ERROR;
}

API_FUNC(list_new)
{
    const char *result;

    /* make C compiler happy */
    (void) args;

    API_INIT_FUNC(1, "list_new", API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_list_new ());

    API_RETURN_STRING(result);
}

API_FUNC(list_add)
{
    char *weelist, *data, *where, *user_data;
    const char *result;

    API_INIT_FUNC(1, "list_add", API_RETURN_EMPTY);
    weelist = NULL;
    data = NULL;
    where = NULL;
    user_data = NULL;
    if (!PyArg_ParseTuple (args, "ssss", &weelist, &data, &where, &user_data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_list_add (API_STR2PTR(weelist),
                                           data,
                                           where,
                                           API_STR2PTR(user_data)));

    API_RETURN_STRING(result);
}

API_FUNC(list_search)
{
    char *weelist, *data;
    const char *result;

    API_INIT_FUNC(1, "list_search", API_RETURN_EMPTY);
    weelist = NULL;
    data = NULL;
    if (!PyArg_ParseTuple (args, "ss", &weelist, &data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_list_search (API_STR2PTR(weelist),
                                              data));

    API_RETURN_STRING(result);
}

API_FUNC(list_search_pos)
{
    char *weelist, *data;
    int pos;

    API_INIT_FUNC(1, "list_search_pos", API_RETURN_INT(-1));
    weelist = NULL;
    data = NULL;
    if (!PyArg_ParseTuple (args, "ss", &weelist, &data))
        API_WRONG_ARGS(API_RETURN_INT(-1));

    pos = weechat_list_search_pos (API_STR2PTR(weelist), data);

    API_RETURN_INT(pos);
}

API_FUNC(list_casesearch)
{
    char *weelist, *data;
    const char *result;

    API_INIT_FUNC(1, "list_casesearch", API_RETURN_EMPTY);
    weelist = NULL;
    data = NULL;
    if (!PyArg_ParseTuple (args, "ss", &weelist, &data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_list_casesearch (API_STR2PTR(weelist),
                                                  data));

    API_RETURN_STRING(result);
}

API_FUNC(list_casesearch_pos)
{
    char *weelist, *data;
    int pos;

    API_INIT_FUNC(1, "list_casesearch_pos", API_RETURN_INT(-1));
    weelist = NULL;
    data = NULL;
    if (!PyArg_ParseTuple (args, "ss", &weelist, &data))
        API_WRONG_ARGS(API_RETURN_INT(-1));

    pos = weechat_list_casesearch_pos (API_STR2PTR(weelist), data);

    API_RETURN_INT(pos);
}

API_FUNC(list_get)
{
    char *weelist;
    const char *result;
    int position;

    API_INIT_FUNC(1, "list_get", API_RETURN_EMPTY);
    weelist = NULL;
    position = 0;
    if (!PyArg_ParseTuple (args, "si", &weelist, &position))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_list_get (API_STR2PTR(weelist),
                                           position));
    API_RETURN_STRING(result);
}

API_FUNC(list_set)
{
    char *item, *new_value;

    API_INIT_FUNC(1, "list_set", API_RETURN_ERROR);
    item = NULL;
    new_value = NULL;
    if (!PyArg_ParseTuple (args, "ss", &item, &new_value))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_list_set (API_STR2PTR(item),
                      new_value);

    API_RETURN_OK;
}

API_FUNC(list_next)
{
    char *item;
    const char *result;

    API_INIT_FUNC(1, "list_next", API_RETURN_EMPTY);
    item = NULL;
    if (!PyArg_ParseTuple (args, "s", &item))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_list_next (API_STR2PTR(item)));

    API_RETURN_STRING(result);
}

API_FUNC(list_prev)
{
    char *item;
    const char *result;

    API_INIT_FUNC(1, "list_prev", API_RETURN_EMPTY);
    item = NULL;
    if (!PyArg_ParseTuple (args, "s", &item))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_list_prev (API_STR2PTR(item)));

    API_RETURN_STRING(result);
}

API_FUNC(list_string)
{
    char *item;
    const char *result;

    API_INIT_FUNC(1, "list_string", API_RETURN_EMPTY);
    item = NULL;
    if (!PyArg_ParseTuple (args, "s", &item))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_list_string (API_STR2PTR(item));

    API_RETURN_STRING(result);
}

API_FUNC(list_size)
{
    char *weelist;
    int size;

    API_INIT_FUNC(1, "list_size", API_RETURN_INT(0));
    weelist = NULL;
    if (!PyArg_ParseTuple (args, "s", &weelist))
        API_WRONG_ARGS(API_RETURN_INT(0));

    size = weechat_list_size (API_STR2PTR(weelist));

    API_RETURN_INT(size);
}

API_FUNC(list_remove)
{
    char *weelist, *item;

    API_INIT_FUNC(1, "list_remove", API_RETURN_ERROR);
    weelist = NULL;
    item = NULL;
    if (!PyArg_ParseTuple (args, "ss", &weelist, &item))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_list_remove (API_STR2PTR(weelist),
                         API_STR2PTR(item));

    API_RETURN_OK;
}

API_FUNC(list_remove_all)
{
    char *weelist;

    API_INIT_FUNC(1, "list_remove_all", API_RETURN_ERROR);
    weelist = NULL;
    if (!PyArg_ParseTuple (args, "s", &weelist))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_list_remove_all (API_STR2PTR(weelist));

    API_RETURN_OK;
}

API_FUNC(list_free)
{
    char *weelist;

    API_INIT_FUNC(1, "list_free", API_RETURN_ERROR);
    weelist = NULL;
    if (!PyArg_ParseTuple (args, "s", &weelist))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_list_free (API_STR2PTR(weelist));

    API_RETURN_OK;
}

int
weechat_python_api_config_reload_cb (const void *pointer, void *data,
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

        rc = (int *) weechat_python_exec (script,
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

API_FUNC(config_new)
{
    char *name, *function, *data;
    const char *result;

    API_INIT_FUNC(1, "config_new", API_RETURN_EMPTY);
    name = NULL;
    function = NULL;
    data = NULL;
    if (!PyArg_ParseTuple (args, "sss", &name, &function, &data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(plugin_script_api_config_new (weechat_python_plugin,
                                                       python_current_script,
                                                       name,
                                                       &weechat_python_api_config_reload_cb,
                                                       function,
                                                       data));

    API_RETURN_STRING(result);
}

struct t_hashtable *
weechat_python_api_config_update_cb (const void *pointer, void *data,
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

        ret_hashtable = weechat_python_exec (script,
                                             WEECHAT_SCRIPT_EXEC_HASHTABLE,
                                             ptr_function,
                                             "ssih", func_argv);

        return ret_hashtable;
    }

    return NULL;
}

API_FUNC(config_set_version)
{
    char *config_file, *function, *data;
    int rc, version;

    API_INIT_FUNC(1, "config_set_version", API_RETURN_INT(0));
    config_file = NULL;
    version = 0;
    function = NULL;
    data = NULL;
    if (!PyArg_ParseTuple (args, "siss", &config_file, &version, &function, &data))
        API_WRONG_ARGS(API_RETURN_INT(0));

    rc = plugin_script_api_config_set_version (
        weechat_python_plugin,
        python_current_script,
        API_STR2PTR(config_file),
        version,
        &weechat_python_api_config_update_cb,
        function,
        data);

    API_RETURN_INT(rc);
}

int
weechat_python_api_config_read_cb (const void *pointer, void *data,
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

        rc = (int *) weechat_python_exec (script,
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
weechat_python_api_config_section_write_cb (const void *pointer, void *data,
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

        rc = (int *) weechat_python_exec (script,
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
weechat_python_api_config_section_write_default_cb (const void *pointer, void *data,
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

        rc = (int *) weechat_python_exec (script,
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
weechat_python_api_config_section_create_option_cb (const void *pointer, void *data,
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

        rc = (int *) weechat_python_exec (script,
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
weechat_python_api_config_section_delete_option_cb (const void *pointer, void *data,
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

        rc = (int *) weechat_python_exec (script,
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

API_FUNC(config_new_section)
{
    char *config_file, *name, *function_read, *data_read, *function_write;
    char *data_write, *function_write_default, *data_write_default;
    char *function_create_option, *data_create_option, *function_delete_option;
    char *data_delete_option;
    const char *result;
    int user_can_add_options, user_can_delete_options;

    API_INIT_FUNC(1, "config_new_section", API_RETURN_EMPTY);
    config_file = NULL;
    name = NULL;
    user_can_add_options = 0;
    user_can_delete_options = 0;
    function_read = NULL;
    data_read = NULL;
    function_write = NULL;
    data_write = NULL;
    function_write_default = NULL;
    data_write_default = NULL;
    function_create_option = NULL;
    data_create_option = NULL;
    function_delete_option = NULL;
    data_delete_option = NULL;
    if (!PyArg_ParseTuple (args, "ssiissssssssss", &config_file, &name,
                           &user_can_add_options, &user_can_delete_options,
                           &function_read, &data_read, &function_write,
                           &data_write, &function_write_default,
                           &data_write_default, &function_create_option,
                           &data_create_option, &function_delete_option,
                           &data_delete_option))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(
        plugin_script_api_config_new_section (
            weechat_python_plugin,
            python_current_script,
            API_STR2PTR(config_file),
            name,
            user_can_add_options,
            user_can_delete_options,
            &weechat_python_api_config_read_cb,
            function_read,
            data_read,
            &weechat_python_api_config_section_write_cb,
            function_write,
            data_write,
            &weechat_python_api_config_section_write_default_cb,
            function_write_default,
            data_write_default,
            &weechat_python_api_config_section_create_option_cb,
            function_create_option,
            data_create_option,
            &weechat_python_api_config_section_delete_option_cb,
            function_delete_option,
            data_delete_option));

    API_RETURN_STRING(result);
}

API_FUNC(config_search_section)
{
    char *config_file, *section_name;
    const char *result;

    API_INIT_FUNC(1, "config_search_section", API_RETURN_EMPTY);
    config_file = NULL;
    section_name = NULL;
    if (!PyArg_ParseTuple (args, "ss", &config_file, &section_name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_config_search_section (API_STR2PTR(config_file),
                                                        section_name));

    API_RETURN_STRING(result);
}

int
weechat_python_api_config_option_check_value_cb (const void *pointer,
                                                 void *data,
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

        rc = (int *) weechat_python_exec (script,
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
weechat_python_api_config_option_change_cb (const void *pointer, void *data,
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

        rc = weechat_python_exec (script,
                                  WEECHAT_SCRIPT_EXEC_IGNORE,
                                  ptr_function,
                                  "ss", func_argv);

        if (rc)
            free (rc);
    }
}

void
weechat_python_api_config_option_delete_cb (const void *pointer, void *data,
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

        rc = weechat_python_exec (script,
                                  WEECHAT_SCRIPT_EXEC_IGNORE,
                                  ptr_function,
                                  "ss", func_argv);

        if (rc)
            free (rc);
    }
}

API_FUNC(config_new_option)
{
    char *config_file, *section, *name, *type, *description, *string_values;
    char *default_value, *value;
    char *function_check_value, *data_check_value, *function_change;
    char *data_change, *function_delete, *data_delete;
    const char *result;
    int min, max, null_value_allowed;

    API_INIT_FUNC(1, "config_new_option", API_RETURN_EMPTY);
    config_file = NULL;
    section = NULL;
    name = NULL;
    type = NULL;
    description = NULL;
    string_values = NULL;
    default_value = NULL;
    value = NULL;
    function_check_value = NULL;
    data_check_value = NULL;
    function_change = NULL;
    data_change = NULL;
    function_delete = NULL;
    data_delete = NULL;
    if (!PyArg_ParseTuple (args, "ssssssiizzissssss", &config_file, &section, &name,
                           &type, &description, &string_values, &min, &max,
                           &default_value, &value, &null_value_allowed,
                           &function_check_value, &data_check_value,
                           &function_change, &data_change, &function_delete,
                           &data_delete))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(plugin_script_api_config_new_option (weechat_python_plugin,
                                                              python_current_script,
                                                              API_STR2PTR(config_file),
                                                              API_STR2PTR(section),
                                                              name,
                                                              type,
                                                              description,
                                                              string_values,
                                                              min,
                                                              max,
                                                              default_value,
                                                              value,
                                                              null_value_allowed,
                                                              &weechat_python_api_config_option_check_value_cb,
                                                              function_check_value,
                                                              data_check_value,
                                                              &weechat_python_api_config_option_change_cb,
                                                              function_change,
                                                              data_change,
                                                              &weechat_python_api_config_option_delete_cb,
                                                              function_delete,
                                                              data_delete));

    API_RETURN_STRING(result);
}

API_FUNC(config_search_option)
{
    char *config_file, *section, *option_name;
    const char *result;

    API_INIT_FUNC(1, "config_search_option", API_RETURN_EMPTY);
    config_file = NULL;
    section = NULL;
    option_name = NULL;
    if (!PyArg_ParseTuple (args, "sss", &config_file, &section, &option_name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_config_search_option (API_STR2PTR(config_file),
                                                       API_STR2PTR(section),
                                                       option_name));

    API_RETURN_STRING(result);
}

API_FUNC(config_string_to_boolean)
{
    char *text;
    int value;

    API_INIT_FUNC(1, "config_string_to_boolean", API_RETURN_INT(0));
    text = NULL;
    if (!PyArg_ParseTuple (args, "s", &text))
        API_WRONG_ARGS(API_RETURN_INT(0));

    value = weechat_config_string_to_boolean (text);

    API_RETURN_INT(value);
}

API_FUNC(config_option_reset)
{
    char *option;
    int run_callback, rc;

    API_INIT_FUNC(1, "config_option_reset", API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));
    option = NULL;
    run_callback = 0;
    if (!PyArg_ParseTuple (args, "si", &option, &run_callback))
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));

    rc = weechat_config_option_reset (API_STR2PTR(option),
                                      run_callback);

    API_RETURN_INT(rc);
}

API_FUNC(config_option_set)
{
    char *option, *new_value;
    int run_callback, rc;

    API_INIT_FUNC(1, "config_option_set", API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));
    option = NULL;
    new_value = NULL;
    run_callback = 0;
    if (!PyArg_ParseTuple (args, "ssi", &option, &new_value, &run_callback))
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));

    rc = weechat_config_option_set (API_STR2PTR(option),
                                    new_value,
                                    run_callback);

    API_RETURN_INT(rc);
}

API_FUNC(config_option_set_null)
{
    char *option;
    int run_callback, rc;

    API_INIT_FUNC(1, "config_option_set_null", API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));
    option = NULL;
    run_callback = 0;
    if (!PyArg_ParseTuple (args, "si", &option, &run_callback))
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));

    rc = weechat_config_option_set_null (API_STR2PTR(option),
                                         run_callback);

    API_RETURN_INT(rc);
}

API_FUNC(config_option_unset)
{
    char *option;
    int rc;

    API_INIT_FUNC(1, "config_option_unset", API_RETURN_INT(WEECHAT_CONFIG_OPTION_UNSET_ERROR));
    option = NULL;
    if (!PyArg_ParseTuple (args, "s", &option))
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_OPTION_UNSET_ERROR));

    rc = weechat_config_option_unset (API_STR2PTR(option));

    API_RETURN_INT(rc);
}

API_FUNC(config_option_rename)
{
    char *option, *new_name;

    API_INIT_FUNC(1, "config_option_rename", API_RETURN_ERROR);
    option = NULL;
    new_name = NULL;
    if (!PyArg_ParseTuple (args, "ss", &option, &new_name))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_config_option_rename (API_STR2PTR(option),
                                  new_name);

    API_RETURN_OK;
}

API_FUNC(config_option_is_null)
{
    char *option;
    int value;

    API_INIT_FUNC(1, "config_option_is_null", API_RETURN_INT(1));
    option = NULL;
    if (!PyArg_ParseTuple (args, "s", &option))
        API_WRONG_ARGS(API_RETURN_INT(1));

    value = weechat_config_option_is_null (API_STR2PTR(option));

    API_RETURN_INT(value);
}

API_FUNC(config_option_default_is_null)
{
    char *option;
    int value;

    API_INIT_FUNC(1, "config_option_default_is_null", API_RETURN_INT(1));
    option = NULL;
    if (!PyArg_ParseTuple (args, "s", &option))
        API_WRONG_ARGS(API_RETURN_INT(1));

    value = weechat_config_option_default_is_null (API_STR2PTR(option));

    API_RETURN_INT(value);
}

API_FUNC(config_boolean)
{
    char *option;
    int value;

    API_INIT_FUNC(1, "config_boolean", API_RETURN_INT(0));
    option = NULL;
    if (!PyArg_ParseTuple (args, "s", &option))
        API_WRONG_ARGS(API_RETURN_INT(0));

    value = weechat_config_boolean (API_STR2PTR(option));

    API_RETURN_INT(value);
}

API_FUNC(config_boolean_default)
{
    char *option;
    int value;

    API_INIT_FUNC(1, "config_boolean_default", API_RETURN_INT(0));
    option = NULL;
    if (!PyArg_ParseTuple (args, "s", &option))
        API_WRONG_ARGS(API_RETURN_INT(0));

    value = weechat_config_boolean_default (API_STR2PTR(option));

    API_RETURN_INT(value);
}

API_FUNC(config_integer)
{
    char *option;
    int value;

    API_INIT_FUNC(1, "config_integer", API_RETURN_INT(0));
    option = NULL;
    if (!PyArg_ParseTuple (args, "s", &option))
        API_WRONG_ARGS(API_RETURN_INT(0));

    value = weechat_config_integer (API_STR2PTR(option));

    API_RETURN_INT(value);
}

API_FUNC(config_integer_default)
{
    char *option;
    int value;

    API_INIT_FUNC(1, "config_integer_default", API_RETURN_INT(0));
    option = NULL;
    if (!PyArg_ParseTuple (args, "s", &option))
        API_WRONG_ARGS(API_RETURN_INT(0));

    value = weechat_config_integer_default (API_STR2PTR(option));

    API_RETURN_INT(value);
}

API_FUNC(config_string)
{
    char *option;
    const char *result;

    API_INIT_FUNC(1, "config_string", API_RETURN_EMPTY);
    option = NULL;
    if (!PyArg_ParseTuple (args, "s", &option))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_config_string (API_STR2PTR(option));

    API_RETURN_STRING(result);
}

API_FUNC(config_string_default)
{
    char *option;
    const char *result;

    API_INIT_FUNC(1, "config_string_default", API_RETURN_EMPTY);
    option = NULL;
    if (!PyArg_ParseTuple (args, "s", &option))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_config_string_default (API_STR2PTR(option));

    API_RETURN_STRING(result);
}

API_FUNC(config_color)
{
    char *option;
    const char *result;

    API_INIT_FUNC(1, "config_color", API_RETURN_EMPTY);
    option = NULL;
    if (!PyArg_ParseTuple (args, "s", &option))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_config_color (API_STR2PTR(option));

    API_RETURN_STRING(result);
}

API_FUNC(config_color_default)
{
    char *option;
    const char *result;

    API_INIT_FUNC(1, "config_color_default", API_RETURN_EMPTY);
    option = NULL;
    if (!PyArg_ParseTuple (args, "s", &option))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_config_color_default (API_STR2PTR(option));

    API_RETURN_STRING(result);
}

API_FUNC(config_write_option)
{
    char *config_file, *option;

    API_INIT_FUNC(1, "config_write_option", API_RETURN_ERROR);
    config_file = NULL;
    option = NULL;
    if (!PyArg_ParseTuple (args, "ss", &config_file, &option))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_config_write_option (API_STR2PTR(config_file),
                                 API_STR2PTR(option));

    API_RETURN_OK;
}

API_FUNC(config_write_line)
{
    char *config_file, *option_name, *value;

    API_INIT_FUNC(1, "config_write_line", API_RETURN_ERROR);
    config_file = NULL;
    option_name = NULL;
    value = NULL;
    if (!PyArg_ParseTuple (args, "sss", &config_file, &option_name, &value))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_config_write_line (API_STR2PTR(config_file),
                               option_name,
                               "%s",
                               value);

    API_RETURN_OK;
}

API_FUNC(config_write)
{
    char *config_file;
    int rc;

    API_INIT_FUNC(1, "config_write", API_RETURN_INT(WEECHAT_CONFIG_WRITE_ERROR));
    config_file = NULL;
    if (!PyArg_ParseTuple (args, "s", &config_file))
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_WRITE_ERROR));

    rc = weechat_config_write (API_STR2PTR(config_file));

    API_RETURN_INT(rc);
}

API_FUNC(config_read)
{
    char *config_file;
    int rc;

    API_INIT_FUNC(1, "config_read", API_RETURN_INT(WEECHAT_CONFIG_READ_FILE_NOT_FOUND));
    config_file = NULL;
    if (!PyArg_ParseTuple (args, "s", &config_file))
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_READ_FILE_NOT_FOUND));

    rc = weechat_config_read (API_STR2PTR(config_file));

    API_RETURN_INT(rc);
}

API_FUNC(config_reload)
{
    char *config_file;
    int rc;

    API_INIT_FUNC(1, "config_reload", API_RETURN_INT(WEECHAT_CONFIG_READ_FILE_NOT_FOUND));
    config_file = NULL;
    if (!PyArg_ParseTuple (args, "s", &config_file))
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_READ_FILE_NOT_FOUND));

    rc = weechat_config_reload (API_STR2PTR(config_file));

    API_RETURN_INT(rc);
}

API_FUNC(config_option_free)
{
    char *option;

    API_INIT_FUNC(1, "config_option_free", API_RETURN_ERROR);
    option = NULL;
    if (!PyArg_ParseTuple (args, "s", &option))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_config_option_free (API_STR2PTR(option));

    API_RETURN_OK;
}

API_FUNC(config_section_free_options)
{
    char *section;

    API_INIT_FUNC(1, "config_section_free_options", API_RETURN_ERROR);
    section = NULL;
    if (!PyArg_ParseTuple (args, "s", &section))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_config_section_free_options (API_STR2PTR(section));

    API_RETURN_OK;
}

API_FUNC(config_section_free)
{
    char *section;

    API_INIT_FUNC(1, "config_section_free", API_RETURN_ERROR);
    section = NULL;
    if (!PyArg_ParseTuple (args, "s", &section))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_config_section_free (API_STR2PTR(section));

    API_RETURN_OK;
}

API_FUNC(config_free)
{
    char *config_file;

    API_INIT_FUNC(1, "config_free", API_RETURN_ERROR);
    config_file = NULL;
    if (!PyArg_ParseTuple (args, "s", &config_file))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_config_free (API_STR2PTR(config_file));

    API_RETURN_OK;
}

API_FUNC(config_get)
{
    char *option;
    const char *result;

    API_INIT_FUNC(1, "config_get", API_RETURN_EMPTY);
    option = NULL;
    if (!PyArg_ParseTuple (args, "s", &option))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_config_get (option));

    API_RETURN_STRING(result);
}

API_FUNC(config_get_plugin)
{
    char *option;
    const char *result;

    API_INIT_FUNC(1, "config_get_plugin", API_RETURN_EMPTY);
    option = NULL;
    if (!PyArg_ParseTuple (args, "s", &option))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = plugin_script_api_config_get_plugin (weechat_python_plugin,
                                                  python_current_script,
                                                  option);

    API_RETURN_STRING(result);
}

API_FUNC(config_is_set_plugin)
{
    char *option;
    int rc;

    API_INIT_FUNC(1, "config_is_set_plugin", API_RETURN_INT(0));
    option = NULL;
    if (!PyArg_ParseTuple (args, "s", &option))
        API_WRONG_ARGS(API_RETURN_INT(0));

    rc = plugin_script_api_config_is_set_plugin (weechat_python_plugin,
                                                 python_current_script,
                                                 option);

    API_RETURN_INT(rc);
}

API_FUNC(config_set_plugin)
{
    char *option, *value;
    int rc;

    API_INIT_FUNC(1, "config_set_plugin", API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));
    option = NULL;
    value = NULL;
    if (!PyArg_ParseTuple (args, "ss", &option, &value))
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));

    rc = plugin_script_api_config_set_plugin (weechat_python_plugin,
                                              python_current_script,
                                              option,
                                              value);

    API_RETURN_INT(rc);
}

API_FUNC(config_set_desc_plugin)
{
    char *option, *description;

    API_INIT_FUNC(1, "config_set_desc_plugin", API_RETURN_ERROR);
    option = NULL;
    description = NULL;
    if (!PyArg_ParseTuple (args, "ss", &option, &description))
        API_WRONG_ARGS(API_RETURN_ERROR);

    plugin_script_api_config_set_desc_plugin (weechat_python_plugin,
                                              python_current_script,
                                              option,
                                              description);

    API_RETURN_OK;
}

API_FUNC(config_unset_plugin)
{
    char *option;
    int rc;

    API_INIT_FUNC(1, "config_unset_plugin", API_RETURN_INT(WEECHAT_CONFIG_OPTION_UNSET_ERROR));
    option = NULL;
    if (!PyArg_ParseTuple (args, "s", &option))
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_OPTION_UNSET_ERROR));

    rc = plugin_script_api_config_unset_plugin (weechat_python_plugin,
                                                python_current_script,
                                                option);

    API_RETURN_INT(rc);
}

API_FUNC(key_bind)
{
    char *context;
    struct t_hashtable *hashtable;
    PyObject *dict;
    int num_keys;

    API_INIT_FUNC(1, "key_bind", API_RETURN_INT(0));
    context = NULL;
    dict = NULL;
    if (!PyArg_ParseTuple (args, "sO", &context, &dict))
        API_WRONG_ARGS(API_RETURN_INT(0));

    hashtable = weechat_python_dict_to_hashtable (dict,
                                                  WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                                  WEECHAT_HASHTABLE_STRING,
                                                  WEECHAT_HASHTABLE_STRING);

    num_keys = weechat_key_bind (context, hashtable);

    if (hashtable)
        weechat_hashtable_free (hashtable);

    API_RETURN_INT(num_keys);
}

API_FUNC(key_unbind)
{
    char *context, *key;
    int num_keys;

    API_INIT_FUNC(1, "key_unbind", API_RETURN_INT(0));
    context = NULL;
    key = NULL;
    if (!PyArg_ParseTuple (args, "ss", &context, &key))
        API_WRONG_ARGS(API_RETURN_INT(0));

    num_keys = weechat_key_unbind (context, key);

    API_RETURN_INT(num_keys);
}

API_FUNC(prefix)
{
    char *prefix;
    const char *result;

    API_INIT_FUNC(0, "prefix", API_RETURN_EMPTY);
    prefix = NULL;
    if (!PyArg_ParseTuple (args, "s", &prefix))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_prefix (prefix);

    API_RETURN_STRING(result);
}

API_FUNC(color)
{
    char *color;
    const char *result;

    API_INIT_FUNC(0, "color", API_RETURN_EMPTY);
    color = NULL;
    if (!PyArg_ParseTuple (args, "s", &color))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_color (color);

    API_RETURN_STRING(result);
}

API_FUNC(prnt)
{
    char *buffer, *message;

    API_INIT_FUNC(0, "prnt", API_RETURN_ERROR);
    buffer = NULL;
    message = NULL;
    if (!PyArg_ParseTuple (args, "ss", &buffer, &message))
        API_WRONG_ARGS(API_RETURN_ERROR);

    plugin_script_api_printf (weechat_python_plugin,
                              python_current_script,
                              API_STR2PTR(buffer),
                              "%s", message);

    API_RETURN_OK;
}

API_FUNC(prnt_date_tags)
{
    char *buffer, *tags, *message;
    long date;

    API_INIT_FUNC(1, "prnt_date_tags", API_RETURN_ERROR);
    buffer = NULL;
    date = 0;
    tags = NULL;
    message = NULL;
    if (!PyArg_ParseTuple (args, "slss", &buffer, &date, &tags, &message))
        API_WRONG_ARGS(API_RETURN_ERROR);

    plugin_script_api_printf_date_tags (weechat_python_plugin,
                                        python_current_script,
                                        API_STR2PTR(buffer),
                                        (time_t)date,
                                        tags,
                                        "%s", message);

    API_RETURN_OK;
}

API_FUNC(prnt_y)
{
    char *buffer, *message;
    int y;

    API_INIT_FUNC(1, "prnt_y", API_RETURN_ERROR);
    buffer = NULL;
    y = 0;
    message = NULL;
    if (!PyArg_ParseTuple (args, "sis", &buffer, &y, &message))
        API_WRONG_ARGS(API_RETURN_ERROR);

    plugin_script_api_printf_y (weechat_python_plugin,
                                python_current_script,
                                API_STR2PTR(buffer),
                                y,
                                "%s", message);

    API_RETURN_OK;
}

API_FUNC(prnt_y_date_tags)
{
    char *buffer, *tags, *message;
    int y;
    long date;

    API_INIT_FUNC(1, "prnt_y_date_tags", API_RETURN_ERROR);
    buffer = NULL;
    y = 0;
    date = 0;
    tags = NULL;
    message = NULL;
    if (!PyArg_ParseTuple (args, "silss", &buffer, &y, &date, &tags, &message))
        API_WRONG_ARGS(API_RETURN_ERROR);

    plugin_script_api_printf_y_date_tags (weechat_python_plugin,
                                          python_current_script,
                                          API_STR2PTR(buffer),
                                          y,
                                          (time_t)date,
                                          tags,
                                          "%s", message);

    API_RETURN_OK;
}

API_FUNC(log_print)
{
    char *message;

    API_INIT_FUNC(1, "log_print", API_RETURN_ERROR);
    message = NULL;
    if (!PyArg_ParseTuple (args, "s", &message))
        API_WRONG_ARGS(API_RETURN_ERROR);

    plugin_script_api_log_printf (weechat_python_plugin,
                                  python_current_script,
                                  "%s", message);

    API_RETURN_OK;
}

int
weechat_python_api_hook_command_cb (const void *pointer, void *data,
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

        rc = (int *) weechat_python_exec (script,
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

API_FUNC(hook_command)
{
    char *command, *description, *arguments, *args_description, *completion;
    char *function, *data;
    const char *result;

    API_INIT_FUNC(1, "hook_command", API_RETURN_EMPTY);
    command = NULL;
    description = NULL;
    arguments = NULL;
    args_description = NULL;
    completion = NULL;
    function = NULL;
    data = NULL;
    if (!PyArg_ParseTuple (args, "sssssss", &command, &description, &arguments,
                           &args_description, &completion, &function, &data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(plugin_script_api_hook_command (weechat_python_plugin,
                                                         python_current_script,
                                                         command,
                                                         description,
                                                         arguments,
                                                         args_description,
                                                         completion,
                                                         &weechat_python_api_hook_command_cb,
                                                         function,
                                                         data));

    API_RETURN_STRING(result);
}

int
weechat_python_api_hook_completion_cb (const void *pointer, void *data,
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

        rc = (int *) weechat_python_exec (script,
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

API_FUNC(hook_completion)
{
    char *completion, *description, *function, *data;
    const char *result;

    API_INIT_FUNC(1, "hook_completion", API_RETURN_EMPTY);
    completion = NULL;
    description = NULL;
    function = NULL;
    data = NULL;
    if (!PyArg_ParseTuple (args, "ssss", &completion, &description, &function,
                           &data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(plugin_script_api_hook_completion (weechat_python_plugin,
                                                            python_current_script,
                                                            completion,
                                                            description,
                                                            &weechat_python_api_hook_completion_cb,
                                                            function,
                                                            data));

    API_RETURN_STRING(result);
}

/*
 * This function deprecated since WeeChat 2.9, kept for compatibility.
 * It is replaced by completion_get_string.
 */

API_FUNC(hook_completion_get_string)
{
    char *completion, *property;
    const char *result;

    API_INIT_FUNC(1, "hook_completion_get_string", API_RETURN_EMPTY);
    completion = NULL;
    property = NULL;
    if (!PyArg_ParseTuple (args, "ss", &completion, &property))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_hook_completion_get_string (API_STR2PTR(completion),
                                                 property);

    API_RETURN_STRING(result);
}

/*
 * This function deprecated since WeeChat 2.9, kept for compatibility.
 * It is replaced by completion_list_add.
 */

API_FUNC(hook_completion_list_add)
{
    char *completion, *word, *where;
    int nick_completion;

    API_INIT_FUNC(1, "hook_completion_list_add", API_RETURN_ERROR);
    completion = NULL;
    word = NULL;
    nick_completion = 0;
    where = NULL;
    if (!PyArg_ParseTuple (args, "ssis", &completion, &word, &nick_completion,
                           &where))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_hook_completion_list_add (API_STR2PTR(completion),
                                      word,
                                      nick_completion,
                                      where);

    API_RETURN_OK;
}

int
weechat_python_api_hook_command_run_cb (const void *pointer, void *data,
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

        rc = (int *) weechat_python_exec (script,
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

API_FUNC(hook_command_run)
{
    char *command, *function, *data;
    const char *result;

    API_INIT_FUNC(1, "hook_command_run", API_RETURN_EMPTY);
    command = NULL;
    function = NULL;
    data = NULL;
    if (!PyArg_ParseTuple (args, "sss", &command, &function, &data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(plugin_script_api_hook_command_run (weechat_python_plugin,
                                                             python_current_script,
                                                             command,
                                                             &weechat_python_api_hook_command_run_cb,
                                                             function,
                                                             data));

    API_RETURN_STRING(result);
}

int
weechat_python_api_hook_timer_cb (const void *pointer, void *data,
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

        rc = (int *) weechat_python_exec (script,
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

API_FUNC(hook_timer)
{
    long interval;
    int align_second, max_calls;
    char *function, *data;
    const char *result;

    API_INIT_FUNC(1, "hook_timer", API_RETURN_EMPTY);
    interval = 10;
    align_second = 0;
    max_calls = 0;
    function = NULL;
    data = NULL;
    if (!PyArg_ParseTuple (args, "liiss", &interval, &align_second, &max_calls,
                           &function, &data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(plugin_script_api_hook_timer (weechat_python_plugin,
                                                       python_current_script,
                                                       interval,
                                                       align_second,
                                                       max_calls,
                                                       &weechat_python_api_hook_timer_cb,
                                                       function,
                                                       data));

    API_RETURN_STRING(result);
}

int
weechat_python_api_hook_fd_cb (const void *pointer, void *data, int fd)
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

        rc = (int *) weechat_python_exec (script,
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

API_FUNC(hook_fd)
{
    int fd, read, write, exception;
    char *function, *data;
    const char *result;

    API_INIT_FUNC(1, "hook_fd", API_RETURN_EMPTY);
    fd = 0;
    read = 0;
    write = 0;
    exception = 0;
    function = NULL;
    data = NULL;
    if (!PyArg_ParseTuple (args, "iiiiss", &fd, &read, &write, &exception,
                           &function, &data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(plugin_script_api_hook_fd (weechat_python_plugin,
                                                    python_current_script,
                                                    fd,
                                                    read,
                                                    write,
                                                    exception,
                                                    &weechat_python_api_hook_fd_cb,
                                                    function,
                                                    data));

    API_RETURN_STRING(result);
}

int
weechat_python_api_hook_process_cb (const void *pointer, void *data,
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

            result = (char *) weechat_python_exec (script,
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

        rc = (int *) weechat_python_exec (script,
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

API_FUNC(hook_process)
{
    char *command, *function, *data;
    const char *result;
    int timeout;

    API_INIT_FUNC(1, "hook_process", API_RETURN_EMPTY);
    command = NULL;
    timeout = 0;
    function = NULL;
    data = NULL;
    if (!PyArg_ParseTuple (args, "siss", &command, &timeout, &function, &data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(plugin_script_api_hook_process (weechat_python_plugin,
                                                         python_current_script,
                                                         command,
                                                         timeout,
                                                         &weechat_python_api_hook_process_cb,
                                                         function,
                                                         data));

    API_RETURN_STRING(result);
}

API_FUNC(hook_process_hashtable)
{
    char *command, *function, *data;
    const char *result;
    int timeout;
    struct t_hashtable *options;
    PyObject *dict;

    API_INIT_FUNC(1, "hook_process_hashtable", API_RETURN_EMPTY);
    command = NULL;
    dict = NULL;
    options = NULL;
    timeout = 0;
    function = NULL;
    data = NULL;
    if (!PyArg_ParseTuple (args, "sOiss", &command, &dict, &timeout, &function,
                           &data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    options = weechat_python_dict_to_hashtable (dict,
                                                WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                                WEECHAT_HASHTABLE_STRING,
                                                WEECHAT_HASHTABLE_STRING);
    result = API_PTR2STR(plugin_script_api_hook_process_hashtable (weechat_python_plugin,
                                                                   python_current_script,
                                                                   command,
                                                                   options,
                                                                   timeout,
                                                                   &weechat_python_api_hook_process_cb,
                                                                   function,
                                                                   data));
    if (options)
        weechat_hashtable_free (options);

    API_RETURN_STRING(result);
}

int
weechat_python_api_hook_connect_cb (const void *pointer, void *data,
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

        rc = (int *) weechat_python_exec (script,
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

API_FUNC(hook_connect)
{
    char *proxy, *address, *local_hostname, *function, *data;
    const char *result;
    int port, ipv6, retry;

    API_INIT_FUNC(1, "hook_connect", API_RETURN_EMPTY);
    proxy = NULL;
    address = NULL;
    port = 0;
    ipv6 = 0;
    retry = 0;
    local_hostname = NULL;
    function = NULL;
    data = NULL;
    if (!PyArg_ParseTuple (args, "ssiiisss", &proxy, &address, &port, &ipv6,
                           &retry, &local_hostname, &function, &data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(plugin_script_api_hook_connect (weechat_python_plugin,
                                                         python_current_script,
                                                         proxy,
                                                         address,
                                                         port,
                                                         ipv6,
                                                         retry,
                                                         NULL, /* gnutls session */
                                                         NULL, /* gnutls callback */
                                                         0,    /* gnutls DH key size */
                                                         NULL, /* gnutls priorities */
                                                         local_hostname,
                                                         &weechat_python_api_hook_connect_cb,
                                                         function,
                                                         data));

    API_RETURN_STRING(result);
}

struct t_hashtable *
weechat_python_api_hook_line_cb (const void *pointer, void *data,
                                 struct t_hashtable *line)
{
    struct t_plugin_script *script;
    void *func_argv[2];
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;
    struct t_hashtable *ret_hashtable;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = line;

        ret_hashtable = weechat_python_exec (script,
                                             WEECHAT_SCRIPT_EXEC_HASHTABLE,
                                             ptr_function,
                                             "sh", func_argv);

        return ret_hashtable;
    }

    return NULL;
}

API_FUNC(hook_line)
{
    char *buffer_type, *buffer_name, *tags, *function, *data;
    const char *result;

    API_INIT_FUNC(1, "hook_line", API_RETURN_EMPTY);
    buffer_type = NULL;
    buffer_name = NULL;
    tags = NULL;
    function = NULL;
    data = NULL;
    if (!PyArg_ParseTuple (args, "sssss", &buffer_type, &buffer_name, &tags,
                           &function, &data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(plugin_script_api_hook_line (weechat_python_plugin,
                                                      python_current_script,
                                                      buffer_type,
                                                      buffer_name,
                                                      tags,
                                                      &weechat_python_api_hook_line_cb,
                                                      function,
                                                      data));

    API_RETURN_STRING(result);
}

int
weechat_python_api_hook_print_cb (const void *pointer, void *data,
                                  struct t_gui_buffer *buffer,
                                  time_t date,
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

        rc = (int *) weechat_python_exec (script,
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
        if (func_argv[3])
            free (func_argv[3]);

        return ret;
    }

    return WEECHAT_RC_ERROR;
}

API_FUNC(hook_print)
{
    char *buffer, *tags, *message, *function, *data;
    const char *result;
    int strip_colors;

    API_INIT_FUNC(1, "hook_print", API_RETURN_EMPTY);
    buffer = NULL;
    tags = NULL;
    message = NULL;
    strip_colors = 0;
    function = NULL;
    data = NULL;
    if (!PyArg_ParseTuple (args, "sssiss", &buffer, &tags, &message,
                           &strip_colors, &function, &data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(plugin_script_api_hook_print (weechat_python_plugin,
                                                       python_current_script,
                                                       API_STR2PTR(buffer),
                                                       tags,
                                                       message,
                                                       strip_colors,
                                                       &weechat_python_api_hook_print_cb,
                                                       function,
                                                       data));

    API_RETURN_STRING(result);
}

int
weechat_python_api_hook_signal_cb (const void *pointer, void *data,
                                   const char *signal, const char *type_data,
                                   void *signal_data)
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

        rc = (int *) weechat_python_exec (script,
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

API_FUNC(hook_signal)
{
    char *signal, *function, *data;
    const char *result;

    API_INIT_FUNC(1, "hook_signal", API_RETURN_EMPTY);
    signal = NULL;
    function = NULL;
    data = NULL;
    if (!PyArg_ParseTuple (args, "sss", &signal, &function, &data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(plugin_script_api_hook_signal (weechat_python_plugin,
                                                        python_current_script,
                                                        signal,
                                                        &weechat_python_api_hook_signal_cb,
                                                        function,
                                                        data));

    API_RETURN_STRING(result);
}

API_FUNC(hook_signal_send)
{
    char *signal, *type_data, *signal_data, *error;
    int number, rc;

    API_INIT_FUNC(1, "hook_signal_send", API_RETURN_INT(WEECHAT_RC_ERROR));
    signal = NULL;
    type_data = NULL;
    signal_data = NULL;
    if (!PyArg_ParseTuple (args, "sss", &signal, &type_data, &signal_data))
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_RC_ERROR));

    if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_STRING) == 0)
    {
        rc = weechat_hook_signal_send (signal, type_data, signal_data);
        API_RETURN_INT(rc);
    }
    else if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_INT) == 0)
    {
        error = NULL;
        number = (int)strtol (signal_data, &error, 10);
        if (error && !error[0])
            rc = weechat_hook_signal_send (signal, type_data, &number);
        else
            rc = WEECHAT_RC_ERROR;
        API_RETURN_INT(rc);
    }
    else if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_POINTER) == 0)
    {
        rc = weechat_hook_signal_send (signal, type_data,
                                       API_STR2PTR(signal_data));
        API_RETURN_INT(rc);
    }

    API_RETURN_INT(WEECHAT_RC_ERROR);
}

int
weechat_python_api_hook_hsignal_cb (const void *pointer, void *data,
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

        rc = (int *) weechat_python_exec (script,
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

API_FUNC(hook_hsignal)
{
    char *signal, *function, *data;
    const char *result;

    API_INIT_FUNC(1, "hook_hsignal", API_RETURN_EMPTY);
    signal = NULL;
    function = NULL;
    data = NULL;
    if (!PyArg_ParseTuple (args, "sss", &signal, &function, &data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(plugin_script_api_hook_hsignal (weechat_python_plugin,
                                                         python_current_script,
                                                         signal,
                                                         &weechat_python_api_hook_hsignal_cb,
                                                         function,
                                                         data));

    API_RETURN_STRING(result);
}

API_FUNC(hook_hsignal_send)
{
    char *signal;
    struct t_hashtable *hashtable;
    PyObject *dict;
    int rc;

    API_INIT_FUNC(1, "hook_hsignal_send", API_RETURN_INT(WEECHAT_RC_ERROR));
    signal = NULL;
    dict = NULL;
    if (!PyArg_ParseTuple (args, "sO", &signal, &dict))
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_RC_ERROR));

    hashtable = weechat_python_dict_to_hashtable (dict,
                                                  WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                                  WEECHAT_HASHTABLE_STRING,
                                                  WEECHAT_HASHTABLE_STRING);

    rc = weechat_hook_hsignal_send (signal, hashtable);

    if (hashtable)
        weechat_hashtable_free (hashtable);

    API_RETURN_INT(rc);
}

int
weechat_python_api_hook_config_cb (const void *pointer, void *data,
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

        rc = (int *) weechat_python_exec (script,
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

API_FUNC(hook_config)
{
    char *option, *function, *data;
    const char *result;

    API_INIT_FUNC(1, "hook_config", API_RETURN_EMPTY);
    option = NULL;
    function = NULL;
    data = NULL;
    if (!PyArg_ParseTuple (args, "sss", &option, &function, &data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(plugin_script_api_hook_config (weechat_python_plugin,
                                                        python_current_script,
                                                        option,
                                                        &weechat_python_api_hook_config_cb,
                                                        function,
                                                        data));

    API_RETURN_STRING(result);
}

char *
weechat_python_api_hook_modifier_cb (const void *pointer, void *data,
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

        return (char *)weechat_python_exec (script,
                                            WEECHAT_SCRIPT_EXEC_STRING,
                                            ptr_function,
                                            "ssss", func_argv);
    }

    return NULL;
}

API_FUNC(hook_modifier)
{
    char *modifier, *function, *data;
    const char *result;

    API_INIT_FUNC(1, "hook_modifier", API_RETURN_EMPTY);
    modifier = NULL;
    function = NULL;
    data = NULL;
    if (!PyArg_ParseTuple (args, "sss", &modifier, &function, &data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(plugin_script_api_hook_modifier (weechat_python_plugin,
                                                          python_current_script,
                                                          modifier,
                                                          &weechat_python_api_hook_modifier_cb,
                                                          function,
                                                          data));

    API_RETURN_STRING(result);
}

API_FUNC(hook_modifier_exec)
{
    char *modifier, *modifier_data, *string, *result;
    PyObject *return_value;

    API_INIT_FUNC(1, "hook_modifier_exec", API_RETURN_EMPTY);
    modifier = NULL;
    modifier_data = NULL;
    string = NULL;
    if (!PyArg_ParseTuple (args, "sss", &modifier, &modifier_data, &string))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_hook_modifier_exec (modifier, modifier_data, string);

    API_RETURN_STRING_FREE(result);
}

char *
weechat_python_api_hook_info_cb (const void *pointer, void *data,
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

        return (char *)weechat_python_exec (script,
                                            WEECHAT_SCRIPT_EXEC_STRING,
                                            ptr_function,
                                            "sss", func_argv);
    }

    return NULL;
}

API_FUNC(hook_info)
{
    char *info_name, *description, *args_description, *function, *data;
    const char *result;

    API_INIT_FUNC(1, "hook_info", API_RETURN_EMPTY);
    info_name = NULL;
    description = NULL;
    args_description = NULL;
    function = NULL;
    data = NULL;
    if (!PyArg_ParseTuple (args, "sssss", &info_name, &description,
                           &args_description, &function, &data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(plugin_script_api_hook_info (weechat_python_plugin,
                                                      python_current_script,
                                                      info_name,
                                                      description,
                                                      args_description,
                                                      &weechat_python_api_hook_info_cb,
                                                      function,
                                                      data));

    API_RETURN_STRING(result);
}

struct t_hashtable *
weechat_python_api_hook_info_hashtable_cb (const void *pointer, void *data,
                                           const char *info_name,
                                           struct t_hashtable *hashtable)
{
    struct t_plugin_script *script;
    void *func_argv[3];
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;
    struct t_hashtable *ret_hashtable;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = (info_name) ? (char *)info_name : empty_arg;
        func_argv[2] = hashtable;

        ret_hashtable = weechat_python_exec (script,
                                             WEECHAT_SCRIPT_EXEC_HASHTABLE,
                                             ptr_function,
                                             "ssh", func_argv);

        return ret_hashtable;
    }

    return NULL;
}

API_FUNC(hook_info_hashtable)
{
    char *info_name, *description, *args_description, *output_description;
    char *function, *data;
    const char *result;

    API_INIT_FUNC(1, "hook_info_hashtable", API_RETURN_EMPTY);
    info_name = NULL;
    description = NULL;
    args_description = NULL;
    output_description = NULL;
    function = NULL;
    data = NULL;
    if (!PyArg_ParseTuple (args, "ssssss", &info_name, &description,
                           &args_description, &output_description,
                           &function, &data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(plugin_script_api_hook_info_hashtable (weechat_python_plugin,
                                                                python_current_script,
                                                                info_name,
                                                                description,
                                                                args_description,
                                                                output_description,
                                                                &weechat_python_api_hook_info_hashtable_cb,
                                                                function,
                                                                data));

    API_RETURN_STRING(result);
}

struct t_infolist *
weechat_python_api_hook_infolist_cb (const void *pointer, void *data,
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

        result = (struct t_infolist *)weechat_python_exec (
            script,
            WEECHAT_SCRIPT_EXEC_POINTER,
            ptr_function,
            "ssss", func_argv);

        return result;
    }

    return NULL;
}

API_FUNC(hook_infolist)
{
    char *infolist_name, *description, *pointer_description, *args_description;
    char *function, *data;
    const char *result;

    API_INIT_FUNC(1, "hook_infolist", API_RETURN_EMPTY);
    infolist_name = NULL;
    description = NULL;
    pointer_description = NULL;
    args_description = NULL;
    function = NULL;
    data = NULL;
    if (!PyArg_ParseTuple (args, "ssssss", &infolist_name, &description,
                           &pointer_description, &args_description, &function,
                           &data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(plugin_script_api_hook_infolist (weechat_python_plugin,
                                                          python_current_script,
                                                          infolist_name,
                                                          description,
                                                          pointer_description,
                                                          args_description,
                                                          &weechat_python_api_hook_infolist_cb,
                                                          function,
                                                          data));

    API_RETURN_STRING(result);
}

struct t_hashtable *
weechat_python_api_hook_focus_cb (const void *pointer, void *data,
                                  struct t_hashtable *info)
{
    struct t_plugin_script *script;
    void *func_argv[2];
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;
    struct t_hashtable *ret_hashtable;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = info;

        ret_hashtable = weechat_python_exec (script,
                                             WEECHAT_SCRIPT_EXEC_HASHTABLE,
                                             ptr_function,
                                             "sh", func_argv);

        return ret_hashtable;
    }

    return NULL;
}

API_FUNC(hook_focus)
{
    char *area, *function, *data;
    const char *result;

    API_INIT_FUNC(1, "hook_focus", API_RETURN_EMPTY);
    area = NULL;
    function = NULL;
    data = NULL;
    if (!PyArg_ParseTuple (args, "sss", &area, &function, &data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(plugin_script_api_hook_focus (weechat_python_plugin,
                                                       python_current_script,
                                                       area,
                                                       &weechat_python_api_hook_focus_cb,
                                                       function,
                                                       data));

    API_RETURN_STRING(result);
}

API_FUNC(hook_set)
{
    char *hook, *property, *value;

    API_INIT_FUNC(1, "hook_set", API_RETURN_ERROR);
    hook = NULL;
    property = NULL;
    value = NULL;
    if (!PyArg_ParseTuple (args, "sss", &hook, &property, &value))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_hook_set (API_STR2PTR(hook),
                      property,
                      value);

    API_RETURN_OK;
}

API_FUNC(unhook)
{
    char *hook;

    API_INIT_FUNC(1, "unhook", API_RETURN_ERROR);
    hook = NULL;
    if (!PyArg_ParseTuple (args, "s", &hook))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_unhook (API_STR2PTR(hook));

    API_RETURN_OK;
}

API_FUNC(unhook_all)
{
    /* make C compiler happy */
    (void) args;

    API_INIT_FUNC(1, "unhook_all", API_RETURN_ERROR);

    weechat_unhook_all (python_current_script->name);

    API_RETURN_OK;
}

int
weechat_python_api_buffer_input_data_cb (const void *pointer, void *data,
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

        rc = (int *) weechat_python_exec (script,
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
weechat_python_api_buffer_close_cb (const void *pointer, void *data,
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

        rc = (int *) weechat_python_exec (script,
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

API_FUNC(buffer_new)
{
    char *name, *function_input, *data_input, *function_close, *data_close;
    const char *result;

    API_INIT_FUNC(1, "buffer_new", API_RETURN_EMPTY);
    name = NULL;
    function_input = NULL;
    data_input = NULL;
    function_close = NULL;
    data_close = NULL;
    if (!PyArg_ParseTuple (args, "sssss", &name, &function_input, &data_input,
                           &function_close, &data_close))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(plugin_script_api_buffer_new (weechat_python_plugin,
                                                       python_current_script,
                                                       name,
                                                       &weechat_python_api_buffer_input_data_cb,
                                                       function_input,
                                                       data_input,
                                                       &weechat_python_api_buffer_close_cb,
                                                       function_close,
                                                       data_close));

    API_RETURN_STRING(result);
}

API_FUNC(buffer_new_props)
{
    char *name, *function_input, *data_input, *function_close, *data_close;
    struct t_hashtable *properties;
    PyObject *dict;
    const char *result;

    API_INIT_FUNC(1, "buffer_new_props", API_RETURN_EMPTY);
    name = NULL;
    dict = NULL;
    function_input = NULL;
    data_input = NULL;
    function_close = NULL;
    data_close = NULL;
    if (!PyArg_ParseTuple (args, "sOssss", &name, &dict,
                           &function_input, &data_input,
                           &function_close, &data_close))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    properties = weechat_python_dict_to_hashtable (dict,
                                                   WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                                   WEECHAT_HASHTABLE_STRING,
                                                   WEECHAT_HASHTABLE_STRING);
    result = API_PTR2STR(
        plugin_script_api_buffer_new_props (
            weechat_python_plugin,
            python_current_script,
            name,
            properties,
            &weechat_python_api_buffer_input_data_cb,
            function_input,
            data_input,
            &weechat_python_api_buffer_close_cb,
            function_close,
            data_close));

    if (properties)
        weechat_hashtable_free (properties);

    API_RETURN_STRING(result);
}

API_FUNC(buffer_search)
{
    char *plugin, *name;
    const char *result;

    API_INIT_FUNC(1, "buffer_search", API_RETURN_EMPTY);
    plugin = NULL;
    name = NULL;
    if (!PyArg_ParseTuple (args, "ss", &plugin, &name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_buffer_search (plugin, name));

    API_RETURN_STRING(result);
}

API_FUNC(buffer_search_main)
{
    const char *result;

    /* make C compiler happy */
    (void) args;

    API_INIT_FUNC(1, "buffer_search_main", API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_buffer_search_main ());

    API_RETURN_STRING(result);
}

API_FUNC(current_buffer)
{
    const char *result;

    /* make C compiler happy */
    (void) args;

    API_INIT_FUNC(1, "current_buffer", API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_current_buffer ());

    API_RETURN_STRING(result);
}

API_FUNC(buffer_clear)
{
    char *buffer;

    API_INIT_FUNC(1, "buffer_clear", API_RETURN_ERROR);
    buffer = NULL;
    if (!PyArg_ParseTuple (args, "s", &buffer))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_buffer_clear (API_STR2PTR(buffer));

    API_RETURN_OK;
}

API_FUNC(buffer_close)
{
    char *buffer;

    API_INIT_FUNC(1, "buffer_close", API_RETURN_ERROR);
    buffer = NULL;
    if (!PyArg_ParseTuple (args, "s", &buffer))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_buffer_close (API_STR2PTR(buffer));

    API_RETURN_OK;
}

API_FUNC(buffer_merge)
{
    char *buffer, *target_buffer;

    API_INIT_FUNC(1, "buffer_merge", API_RETURN_ERROR);
    buffer = NULL;
    target_buffer = NULL;
    if (!PyArg_ParseTuple (args, "ss", &buffer, &target_buffer))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_buffer_merge (API_STR2PTR(buffer),
                          API_STR2PTR(target_buffer));

    API_RETURN_OK;
}

API_FUNC(buffer_unmerge)
{
    char *buffer;
    int number;

    API_INIT_FUNC(1, "buffer_unmerge", API_RETURN_ERROR);
    buffer = NULL;
    number = 0;
    if (!PyArg_ParseTuple (args, "si", &buffer, &number))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_buffer_unmerge (API_STR2PTR(buffer), number);

    API_RETURN_OK;
}

API_FUNC(buffer_get_integer)
{
    char *buffer, *property;
    int value;

    API_INIT_FUNC(1, "buffer_get_integer", API_RETURN_INT(-1));
    buffer = NULL;
    property = NULL;
    if (!PyArg_ParseTuple (args, "ss", &buffer, &property))
        API_WRONG_ARGS(API_RETURN_INT(-1));

    value = weechat_buffer_get_integer (API_STR2PTR(buffer), property);

    API_RETURN_INT(value);
}

API_FUNC(buffer_get_string)
{
    char *buffer, *property;
    const char *result;

    API_INIT_FUNC(1, "buffer_get_string", API_RETURN_EMPTY);
    buffer = NULL;
    property = NULL;
    if (!PyArg_ParseTuple (args, "ss", &buffer, &property))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_buffer_get_string (API_STR2PTR(buffer), property);

    API_RETURN_STRING(result);
}

API_FUNC(buffer_get_pointer)
{
    char *buffer, *property;
    const char *result;

    API_INIT_FUNC(1, "buffer_get_pointer", API_RETURN_EMPTY);
    buffer = NULL;
    property = NULL;
    if (!PyArg_ParseTuple (args, "ss", &buffer, &property))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_buffer_get_pointer (API_STR2PTR(buffer),
                                                     property));

    API_RETURN_STRING(result);
}

API_FUNC(buffer_set)
{
    char *buffer, *property, *value;

    API_INIT_FUNC(1, "buffer_set", API_RETURN_ERROR);
    buffer = NULL;
    property = NULL;
    value = NULL;
    if (!PyArg_ParseTuple (args, "sss", &buffer, &property, &value))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_buffer_set (API_STR2PTR(buffer),
                        property,
                        value);

    API_RETURN_OK;
}

API_FUNC(buffer_string_replace_local_var)
{
    char *buffer, *string, *result;
    PyObject *return_value;

    API_INIT_FUNC(1, "buffer_string_replace_local_var", API_RETURN_EMPTY);
    buffer = NULL;
    string = NULL;
    if (!PyArg_ParseTuple (args, "ss", &buffer, &string))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_buffer_string_replace_local_var (API_STR2PTR(buffer), string);

    API_RETURN_STRING_FREE(result);
}

API_FUNC(buffer_match_list)
{
    char *buffer, *string;
    int value;

    API_INIT_FUNC(1, "buffer_match_list", API_RETURN_INT(0));
    buffer = NULL;
    string = NULL;
    if (!PyArg_ParseTuple (args, "ss", &buffer, &string))
        API_WRONG_ARGS(API_RETURN_INT(0));

    value = weechat_buffer_match_list (API_STR2PTR(buffer), string);

    API_RETURN_INT(value);
}

API_FUNC(current_window)
{
    const char *result;

    /* make C compiler happy */
    (void) args;

    API_INIT_FUNC(1, "current_window", API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_current_window ());

    API_RETURN_STRING(result);
}

API_FUNC(window_search_with_buffer)
{
    char *buffer;
    const char *result;

    API_INIT_FUNC(1, "window_search_with_buffer", API_RETURN_EMPTY);
    buffer = NULL;
    if (!PyArg_ParseTuple (args, "s", &buffer))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_window_search_with_buffer (API_STR2PTR(buffer)));

    API_RETURN_STRING(result);
}

API_FUNC(window_get_integer)
{
    char *window, *property;
    int value;

    API_INIT_FUNC(1, "window_get_integer", API_RETURN_INT(-1));
    window = NULL;
    property = NULL;
    if (!PyArg_ParseTuple (args, "ss", &window, &property))
        API_WRONG_ARGS(API_RETURN_INT(-1));

    value = weechat_window_get_integer (API_STR2PTR(window), property);

    API_RETURN_INT(value);
}

API_FUNC(window_get_string)
{
    char *window, *property;
    const char *result;

    API_INIT_FUNC(1, "window_get_string", API_RETURN_EMPTY);
    window = NULL;
    property = NULL;
    if (!PyArg_ParseTuple (args, "ss", &window, &property))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_window_get_string (API_STR2PTR(window), property);

    API_RETURN_STRING(result);
}

API_FUNC(window_get_pointer)
{
    char *window, *property;
    const char *result;

    API_INIT_FUNC(1, "window_get_pointer", API_RETURN_EMPTY);
    window = NULL;
    property = NULL;
    if (!PyArg_ParseTuple (args, "ss", &window, &property))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_window_get_pointer (API_STR2PTR(window),
                                                     property));

    API_RETURN_STRING(result);
}

API_FUNC(window_set_title)
{
    char *title;

    API_INIT_FUNC(1, "window_set_title", API_RETURN_ERROR);
    title = NULL;
    if (!PyArg_ParseTuple (args, "s", &title))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_window_set_title (title);

    API_RETURN_OK;
}

API_FUNC(nicklist_add_group)
{
    char *buffer, *parent_group, *name, *color;
    const char *result;
    int visible;

    API_INIT_FUNC(1, "nicklist_add_group", API_RETURN_EMPTY);
    buffer = NULL;
    parent_group = NULL;
    name = NULL;
    color = NULL;
    visible = 0;
    if (!PyArg_ParseTuple (args, "ssssi", &buffer, &parent_group, &name,
                           &color, &visible))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_nicklist_add_group (API_STR2PTR(buffer),
                                                     API_STR2PTR(parent_group),
                                                     name,
                                                     color,
                                                     visible));

    API_RETURN_STRING(result);
}

API_FUNC(nicklist_search_group)
{
    char *buffer, *from_group, *name;
    const char *result;

    API_INIT_FUNC(1, "nicklist_search_group", API_RETURN_EMPTY);
    buffer = NULL;
    from_group = NULL;
    name = NULL;
    if (!PyArg_ParseTuple (args, "sss", &buffer, &from_group, &name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_nicklist_search_group (API_STR2PTR(buffer),
                                                        API_STR2PTR(from_group),
                                                        name));

    API_RETURN_STRING(result);
}

API_FUNC(nicklist_add_nick)
{
    char *buffer, *group, *name, *color, *prefix, *prefix_color;
    const char *result;
    int visible;

    API_INIT_FUNC(1, "nicklist_add_nick", API_RETURN_EMPTY);
    buffer = NULL;
    group = NULL;
    name = NULL;
    color = NULL;
    prefix = NULL;
    prefix_color = NULL;
    visible = 0;
    if (!PyArg_ParseTuple (args, "ssssssi", &buffer, &group, &name, &color,
                           &prefix, &prefix_color, &visible))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_nicklist_add_nick (API_STR2PTR(buffer),
                                                    API_STR2PTR(group),
                                                    name,
                                                    color,
                                                    prefix,
                                                    prefix_color,
                                                    visible));

    API_RETURN_STRING(result);
}

API_FUNC(nicklist_search_nick)
{
    char *buffer, *from_group, *name;
    const char *result;

    API_INIT_FUNC(1, "nicklist_search_nick", API_RETURN_EMPTY);
    buffer = NULL;
    from_group = NULL;
    name = NULL;
    if (!PyArg_ParseTuple (args, "sss", &buffer, &from_group, &name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_nicklist_search_nick (API_STR2PTR(buffer),
                                                       API_STR2PTR(from_group),
                                                       name));

    API_RETURN_STRING(result);
}

API_FUNC(nicklist_remove_group)
{
    char *buffer, *group;

    API_INIT_FUNC(1, "nicklist_remove_group", API_RETURN_ERROR);
    buffer = NULL;
    group = NULL;
    if (!PyArg_ParseTuple (args, "ss", &buffer, &group))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_nicklist_remove_group (API_STR2PTR(buffer),
                                   API_STR2PTR(group));

    API_RETURN_OK;
}

API_FUNC(nicklist_remove_nick)
{
    char *buffer, *nick;

    API_INIT_FUNC(1, "nicklist_remove_nick", API_RETURN_ERROR);
    buffer = NULL;
    nick = NULL;
    if (!PyArg_ParseTuple (args, "ss", &buffer, &nick))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_nicklist_remove_nick (API_STR2PTR(buffer),
                                  API_STR2PTR(nick));

    API_RETURN_OK;
}

API_FUNC(nicklist_remove_all)
{
    char *buffer;

    API_INIT_FUNC(1, "nicklist_remove_all", API_RETURN_ERROR);
    buffer = NULL;
    if (!PyArg_ParseTuple (args, "s", &buffer))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_nicklist_remove_all (API_STR2PTR(buffer));

    API_RETURN_OK;
}

API_FUNC(nicklist_group_get_integer)
{
    char *buffer, *group, *property;
    int value;

    API_INIT_FUNC(1, "nicklist_group_get_integer", API_RETURN_INT(-1));
    buffer = NULL;
    group = NULL;
    property = NULL;
    if (!PyArg_ParseTuple (args, "sss", &buffer, &group, &property))
        API_WRONG_ARGS(API_RETURN_INT(-1));

    value = weechat_nicklist_group_get_integer (API_STR2PTR(buffer),
                                                API_STR2PTR(group),
                                                property);

    API_RETURN_INT(value);
}

API_FUNC(nicklist_group_get_string)
{
    char *buffer, *group, *property;
    const char *result;

    API_INIT_FUNC(1, "nicklist_group_get_string", API_RETURN_EMPTY);
    buffer = NULL;
    group = NULL;
    property = NULL;
    if (!PyArg_ParseTuple (args, "sss", &buffer, &group, &property))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_nicklist_group_get_string (API_STR2PTR(buffer),
                                                API_STR2PTR(group),
                                                property);

    API_RETURN_STRING(result);
}

API_FUNC(nicklist_group_get_pointer)
{
    char *buffer, *group, *property;
    const char *result;

    API_INIT_FUNC(1, "nicklist_group_get_pointer", API_RETURN_EMPTY);
    buffer = NULL;
    group = NULL;
    property = NULL;
    if (!PyArg_ParseTuple (args, "sss", &buffer, &group, &property))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_nicklist_group_get_pointer (API_STR2PTR(buffer),
                                                             API_STR2PTR(group),
                                                             property));

    API_RETURN_STRING(result);
}

API_FUNC(nicklist_group_set)
{
    char *buffer, *group, *property, *value;

    API_INIT_FUNC(1, "nicklist_group_set", API_RETURN_ERROR);
    buffer = NULL;
    group = NULL;
    property = NULL;
    value = NULL;
    if (!PyArg_ParseTuple (args, "ssss", &buffer, &group, &property, &value))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_nicklist_group_set (API_STR2PTR(buffer),
                                API_STR2PTR(group),
                                property,
                                value);

    API_RETURN_OK;
}

API_FUNC(nicklist_nick_get_integer)
{
    char *buffer, *nick, *property;
    int value;

    API_INIT_FUNC(1, "nicklist_nick_get_integer", API_RETURN_INT(-1));
    buffer = NULL;
    nick = NULL;
    property = NULL;
    if (!PyArg_ParseTuple (args, "sss", &buffer, &nick, &property))
        API_WRONG_ARGS(API_RETURN_INT(-1));

    value = weechat_nicklist_nick_get_integer (API_STR2PTR(buffer),
                                               API_STR2PTR(nick),
                                               property);

    API_RETURN_INT(value);
}

API_FUNC(nicklist_nick_get_string)
{
    char *buffer, *nick, *property;
    const char *result;

    API_INIT_FUNC(1, "nicklist_nick_get_string", API_RETURN_EMPTY);
    buffer = NULL;
    nick = NULL;
    property = NULL;
    if (!PyArg_ParseTuple (args, "sss", &buffer, &nick, &property))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_nicklist_nick_get_string (API_STR2PTR(buffer),
                                               API_STR2PTR(nick),
                                               property);

    API_RETURN_STRING(result);
}

API_FUNC(nicklist_nick_get_pointer)
{
    char *buffer, *nick, *property;
    const char *result;

    API_INIT_FUNC(1, "nicklist_nick_get_pointer", API_RETURN_EMPTY);
    buffer = NULL;
    nick = NULL;
    property = NULL;
    if (!PyArg_ParseTuple (args, "sss", &buffer, &nick, &property))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_nicklist_nick_get_pointer (API_STR2PTR(buffer),
                                                            API_STR2PTR(nick),
                                                            property));

    API_RETURN_STRING(result);
}

API_FUNC(nicklist_nick_set)
{
    char *buffer, *nick, *property, *value;

    API_INIT_FUNC(1, "nicklist_nick_set", API_RETURN_ERROR);
    buffer = NULL;
    nick = NULL;
    property = NULL;
    value = NULL;
    if (!PyArg_ParseTuple (args, "ssss", &buffer, &nick, &property, &value))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_nicklist_nick_set (API_STR2PTR(buffer),
                               API_STR2PTR(nick),
                               property,
                               value);

    API_RETURN_OK;
}

API_FUNC(bar_item_search)
{
    char *name;
    const char *result;

    API_INIT_FUNC(1, "bar_item_search", API_RETURN_EMPTY);
    name = NULL;
    if (!PyArg_ParseTuple (args, "s", &name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_bar_item_search (name));

    API_RETURN_STRING(result);
}

char *
weechat_python_api_bar_item_build_cb (const void *pointer, void *data,
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

            ret = (char *)weechat_python_exec (script,
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

            ret = (char *)weechat_python_exec (script,
                                               WEECHAT_SCRIPT_EXEC_STRING,
                                               ptr_function,
                                               "sss", func_argv);
        }

        return ret;
    }

    return NULL;
}

API_FUNC(bar_item_new)
{
    char *name, *function, *data;
    const char *result;

    API_INIT_FUNC(1, "bar_item_new", API_RETURN_EMPTY);
    name = NULL;
    function = NULL;
    data = NULL;
    if (!PyArg_ParseTuple (args, "sss", &name, &function, &data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(plugin_script_api_bar_item_new (weechat_python_plugin,
                                                         python_current_script,
                                                         name,
                                                         &weechat_python_api_bar_item_build_cb,
                                                         function,
                                                         data));

    API_RETURN_STRING(result);
}

API_FUNC(bar_item_update)
{
    char *name;

    API_INIT_FUNC(1, "bar_item_update", API_RETURN_ERROR);
    name = NULL;
    if (!PyArg_ParseTuple (args, "s", &name))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_bar_item_update (name);

    API_RETURN_OK;
}

API_FUNC(bar_item_remove)
{
    char *item;

    API_INIT_FUNC(1, "bar_item_remove", API_RETURN_ERROR);
    item = NULL;
    if (!PyArg_ParseTuple (args, "s", &item))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_bar_item_remove (API_STR2PTR(item));

    API_RETURN_OK;
}

API_FUNC(bar_search)
{
    char *name;
    const char *result;

    API_INIT_FUNC(1, "bar_search", API_RETURN_EMPTY);
    name = NULL;
    if (!PyArg_ParseTuple (args, "s", &name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_bar_search (name));

    API_RETURN_STRING(result);
}

API_FUNC(bar_new)
{
    char *name, *hidden, *priority, *type, *conditions, *position;
    char *filling_top_bottom, *filling_left_right, *size, *size_max;
    char *color_fg, *color_delim, *color_bg, *color_bg_inactive, *separator;
    char *items;
    const char *result;

    API_INIT_FUNC(1, "bar_new", API_RETURN_EMPTY);
    name = NULL;
    hidden = NULL;
    priority = NULL;
    type = NULL;
    conditions = NULL;
    position = NULL;
    filling_top_bottom = NULL;
    filling_left_right = NULL;
    size = NULL;
    size_max = NULL;
    color_fg = NULL;
    color_delim = NULL;
    color_bg = NULL;
    color_bg_inactive = NULL;
    separator = NULL;
    items = NULL;
    if (!PyArg_ParseTuple (args, "ssssssssssssssss", &name, &hidden, &priority,
                           &type, &conditions, &position, &filling_top_bottom,
                           &filling_left_right, &size, &size_max, &color_fg,
                           &color_delim, &color_bg, &color_bg_inactive,
                           &separator, &items))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_bar_new (name,
                                          hidden,
                                          priority,
                                          type,
                                          conditions,
                                          position,
                                          filling_top_bottom,
                                          filling_left_right,
                                          size,
                                          size_max,
                                          color_fg,
                                          color_delim,
                                          color_bg,
                                          color_bg_inactive,
                                          separator,
                                          items));

    API_RETURN_STRING(result);
}

API_FUNC(bar_set)
{
    char *bar, *property, *value;
    int rc;

    API_INIT_FUNC(1, "bar_set", API_RETURN_INT(0));
    bar = NULL;
    property = NULL;
    value = NULL;
    if (!PyArg_ParseTuple (args, "sss", &bar, &property, &value))
        API_WRONG_ARGS(API_RETURN_INT(0));

    rc = weechat_bar_set (API_STR2PTR(bar), property, value);

    API_RETURN_INT(rc);
}

API_FUNC(bar_update)
{
    char *name;

    API_INIT_FUNC(1, "bar_update", API_RETURN_ERROR);
    name = NULL;
    if (!PyArg_ParseTuple (args, "s", &name))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_bar_update (name);

    API_RETURN_OK;
}

API_FUNC(bar_remove)
{
    char *bar;

    API_INIT_FUNC(1, "bar_remove", API_RETURN_ERROR);
    bar = NULL;
    if (!PyArg_ParseTuple (args, "s", &bar))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_bar_remove (API_STR2PTR(bar));

    API_RETURN_OK;
}

API_FUNC(command)
{
    char *buffer, *command;
    int rc;

    API_INIT_FUNC(1, "command", API_RETURN_INT(WEECHAT_RC_ERROR));
    buffer = NULL;
    command = NULL;
    if (!PyArg_ParseTuple (args, "ss", &buffer, &command))
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_RC_ERROR));

    rc = plugin_script_api_command (weechat_python_plugin,
                                    python_current_script,
                                    API_STR2PTR(buffer),
                                    command);

    API_RETURN_INT(rc);
}

API_FUNC(command_options)
{
    char *buffer, *command;
    struct t_hashtable *options;
    int rc;
    PyObject *dict;

    API_INIT_FUNC(1, "command_options", API_RETURN_INT(WEECHAT_RC_ERROR));
    buffer = NULL;
    command = NULL;
    options = NULL;
    if (!PyArg_ParseTuple (args, "ssO", &buffer, &command, &dict))
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_RC_ERROR));

    options = weechat_python_dict_to_hashtable (dict,
                                                WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                                WEECHAT_HASHTABLE_STRING,
                                                WEECHAT_HASHTABLE_STRING);
    rc = plugin_script_api_command_options (weechat_python_plugin,
                                            python_current_script,
                                            API_STR2PTR(buffer),
                                            command,
                                            options);
    if (options)
        weechat_hashtable_free (options);

    API_RETURN_INT(rc);
}

API_FUNC(completion_new)
{
    char *buffer;
    const char *result;

    API_INIT_FUNC(1, "completion_new", API_RETURN_EMPTY);
    buffer = NULL;
    if (!PyArg_ParseTuple (args, "s", &buffer))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_completion_new (API_STR2PTR(buffer)));

    API_RETURN_STRING(result);
}

API_FUNC(completion_search)
{
    char *completion, *data;
    int position, direction, rc;

    API_INIT_FUNC(1, "completion_search", API_RETURN_INT(0));
    completion = NULL;
    position = 0;
    direction = 1;
    if (!PyArg_ParseTuple (args, "ssii", &completion, &data, &position,
                           &direction))
        API_WRONG_ARGS(API_RETURN_INT(0));

    rc = weechat_completion_search (API_STR2PTR(completion),
                                    data,
                                    position,
                                    direction);

    API_RETURN_INT(rc);
}

API_FUNC(completion_get_string)
{
    char *completion, *property;
    const char *result;

    API_INIT_FUNC(1, "completion_get_string", API_RETURN_EMPTY);
    completion = NULL;
    property = NULL;
    if (!PyArg_ParseTuple (args, "ss", &completion, &property))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_completion_get_string (API_STR2PTR(completion),
                                            property);

    API_RETURN_STRING(result);
}

API_FUNC(completion_list_add)
{
    char *completion, *word, *where;
    int nick_completion;

    API_INIT_FUNC(1, "completion_list_add", API_RETURN_ERROR);
    completion = NULL;
    word = NULL;
    nick_completion = 0;
    where = NULL;
    if (!PyArg_ParseTuple (args, "ssis", &completion, &word, &nick_completion,
                           &where))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_completion_list_add (API_STR2PTR(completion),
                                 word,
                                 nick_completion,
                                 where);

    API_RETURN_OK;
}

API_FUNC(completion_free)
{
    char *completion;

    API_INIT_FUNC(1, "completion_free", API_RETURN_ERROR);
    completion = NULL;
    if (!PyArg_ParseTuple (args, "s", &completion))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_completion_free (API_STR2PTR(completion));

    API_RETURN_OK;
}

API_FUNC(info_get)
{
    char *info_name, *arguments, *result;
    PyObject *return_value;

    API_INIT_FUNC(1, "info_get", API_RETURN_EMPTY);
    info_name = NULL;
    arguments = NULL;
    if (!PyArg_ParseTuple (args, "ss", &info_name, &arguments))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_info_get (info_name, arguments);

    API_RETURN_STRING_FREE(result);
}

API_FUNC(info_get_hashtable)
{
    char *info_name;
    struct t_hashtable *hashtable, *result_hashtable;
    PyObject *dict, *result_dict;

    API_INIT_FUNC(1, "info_get_hashtable", API_RETURN_EMPTY);
    info_name = NULL;
    dict = NULL;
    if (!PyArg_ParseTuple (args, "sO", &info_name, &dict))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    hashtable = weechat_python_dict_to_hashtable (dict,
                                                  WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                                  WEECHAT_HASHTABLE_STRING,
                                                  WEECHAT_HASHTABLE_STRING);

    result_hashtable = weechat_info_get_hashtable (info_name, hashtable);
    result_dict = weechat_python_hashtable_to_dict (result_hashtable);

    if (hashtable)
        weechat_hashtable_free (hashtable);
    if (result_hashtable)
        weechat_hashtable_free (result_hashtable);

    return result_dict;
}

API_FUNC(infolist_new)
{
    const char *result;

    /* make C compiler happy */
    (void) args;

    API_INIT_FUNC(1, "infolist_new", API_RETURN_EMPTY);
    result = API_PTR2STR(weechat_infolist_new ());

    API_RETURN_STRING(result);
}

API_FUNC(infolist_new_item)
{
    char *infolist;
    const char *result;

    API_INIT_FUNC(1, "infolist_new_item", API_RETURN_EMPTY);
    infolist = NULL;
    if (!PyArg_ParseTuple (args, "s", &infolist))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_infolist_new_item (API_STR2PTR(infolist)));

    API_RETURN_STRING(result);
}

API_FUNC(infolist_new_var_integer)
{
    char *item, *name;
    const char *result;
    int value;

    API_INIT_FUNC(1, "infolist_new_var_integer", API_RETURN_EMPTY);
    item = NULL;
    name = NULL;
    value = 0;
    if (!PyArg_ParseTuple (args, "ssi", &item, &name, &value))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_infolist_new_var_integer (API_STR2PTR(item),
                                                           name,
                                                           value));

    API_RETURN_STRING(result);
}

API_FUNC(infolist_new_var_string)
{
    char *item, *name, *value;
    const char *result;

    API_INIT_FUNC(1, "infolist_new_var_string", API_RETURN_EMPTY);
    item = NULL;
    name = NULL;
    value = NULL;
    if (!PyArg_ParseTuple (args, "sss", &item, &name, &value))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_infolist_new_var_string (API_STR2PTR(item),
                                                          name,
                                                          value));

    API_RETURN_STRING(result);
}

API_FUNC(infolist_new_var_pointer)
{
    char *item, *name, *value;
    const char *result;

    API_INIT_FUNC(1, "infolist_new_var_pointer", API_RETURN_EMPTY);
    item = NULL;
    name = NULL;
    value = NULL;
    if (!PyArg_ParseTuple (args, "sss", &item, &name, &value))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_infolist_new_var_pointer (API_STR2PTR(item),
                                                           name,
                                                           API_STR2PTR(value)));

    API_RETURN_STRING(result);
}

API_FUNC(infolist_new_var_time)
{
    char *item, *name;
    const char *result;
    long value;

    API_INIT_FUNC(1, "infolist_new_var_time", API_RETURN_EMPTY);
    item = NULL;
    name = NULL;
    value = 0;
    if (!PyArg_ParseTuple (args, "ssl", &item, &name, &value))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_infolist_new_var_time (API_STR2PTR(item),
                                                        name,
                                                        (time_t)value));

    API_RETURN_STRING(result);
}

API_FUNC(infolist_search_var)
{
    char *infolist, *name;
    const char *result;

    API_INIT_FUNC(1, "infolist_search_var", API_RETURN_EMPTY);
    infolist = NULL;
    name = NULL;
    if (!PyArg_ParseTuple (args, "ss", &infolist, &name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_infolist_search_var (API_STR2PTR(infolist),
                                                      name));

    API_RETURN_STRING(result);
}

API_FUNC(infolist_get)
{
    char *name, *pointer, *arguments;
    const char *result;

    API_INIT_FUNC(1, "infolist_get", API_RETURN_EMPTY);
    name = NULL;
    pointer = NULL;
    arguments = NULL;
    if (!PyArg_ParseTuple (args, "sss", &name, &pointer, &arguments))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_infolist_get (name,
                                               API_STR2PTR(pointer),
                                               arguments));

    API_RETURN_STRING(result);
}

API_FUNC(infolist_next)
{
    char *infolist;
    int value;

    API_INIT_FUNC(1, "infolist_next", API_RETURN_INT(0));
    infolist = NULL;
    if (!PyArg_ParseTuple (args, "s", &infolist))
        API_WRONG_ARGS(API_RETURN_INT(0));

    value = weechat_infolist_next (API_STR2PTR(infolist));

    API_RETURN_INT(value);
}

API_FUNC(infolist_prev)
{
    char *infolist;
    int value;

    API_INIT_FUNC(1, "infolist_prev", API_RETURN_INT(0));
    infolist = NULL;
    if (!PyArg_ParseTuple (args, "s", &infolist))
        API_WRONG_ARGS(API_RETURN_INT(0));

    value = weechat_infolist_prev (API_STR2PTR(infolist));

    API_RETURN_INT(value);
}

API_FUNC(infolist_reset_item_cursor)
{
    char *infolist;

    API_INIT_FUNC(1, "infolist_reset_item_cursor", API_RETURN_ERROR);
    infolist = NULL;
    if (!PyArg_ParseTuple (args, "s", &infolist))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_infolist_reset_item_cursor (API_STR2PTR(infolist));

    API_RETURN_OK;
}

API_FUNC(infolist_fields)
{
    char *infolist;
    const char *result;

    API_INIT_FUNC(1, "infolist_fields", API_RETURN_EMPTY);
    infolist = NULL;
    if (!PyArg_ParseTuple (args, "s", &infolist))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_infolist_fields (API_STR2PTR(infolist));

    API_RETURN_STRING(result);
}

API_FUNC(infolist_integer)
{
    char *infolist, *variable;
    int value;

    API_INIT_FUNC(1, "infolist_integer", API_RETURN_INT(0));
    infolist = NULL;
    variable = NULL;
    if (!PyArg_ParseTuple (args, "ss", &infolist, &variable))
        API_WRONG_ARGS(API_RETURN_INT(0));

    value = weechat_infolist_integer (API_STR2PTR(infolist),
                                      variable);

    API_RETURN_INT(value);
}

API_FUNC(infolist_string)
{
    char *infolist, *variable;
    const char *result;

    API_INIT_FUNC(1, "infolist_string", API_RETURN_EMPTY);
    infolist = NULL;
    variable = NULL;
    if (!PyArg_ParseTuple (args, "ss", &infolist, &variable))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_infolist_string (API_STR2PTR(infolist),
                                      variable);

    API_RETURN_STRING(result);
}

API_FUNC(infolist_pointer)
{
    char *infolist, *variable;
    const char *result;

    API_INIT_FUNC(1, "infolist_pointer", API_RETURN_EMPTY);
    infolist = NULL;
    variable = NULL;
    if (!PyArg_ParseTuple (args, "ss", &infolist, &variable))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_infolist_pointer (API_STR2PTR(infolist),
                                                   variable));

    API_RETURN_STRING(result);
}

API_FUNC(infolist_time)
{
    char *infolist, *variable;
    time_t time;

    API_INIT_FUNC(1, "infolist_time", API_RETURN_LONG(0));
    infolist = NULL;
    variable = NULL;
    if (!PyArg_ParseTuple (args, "ss", &infolist, &variable))
        API_WRONG_ARGS(API_RETURN_LONG(0));

    time = weechat_infolist_time (API_STR2PTR(infolist), variable);

    API_RETURN_LONG(time);
}

API_FUNC(infolist_free)
{
    char *infolist;

    API_INIT_FUNC(1, "infolist_free", API_RETURN_ERROR);
    infolist = NULL;
    if (!PyArg_ParseTuple (args, "s", &infolist))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_infolist_free (API_STR2PTR(infolist));

    API_RETURN_OK;
}

API_FUNC(hdata_get)
{
    char *name;
    const char *result;

    API_INIT_FUNC(1, "hdata_get", API_RETURN_EMPTY);
    name = NULL;
    if (!PyArg_ParseTuple (args, "s", &name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_hdata_get (name));

    API_RETURN_STRING(result);
}

API_FUNC(hdata_get_var_offset)
{
    char *hdata, *name;
    int value;

    API_INIT_FUNC(1, "hdata_get_var_offset", API_RETURN_INT(0));
    hdata = NULL;
    name = NULL;
    if (!PyArg_ParseTuple (args, "ss", &hdata, &name))
        API_WRONG_ARGS(API_RETURN_INT(0));

    value = weechat_hdata_get_var_offset (API_STR2PTR(hdata), name);

    API_RETURN_INT(value);
}

API_FUNC(hdata_get_var_type_string)
{
    char *hdata, *name;
    const char *result;

    API_INIT_FUNC(1, "hdata_get_var_type_string", API_RETURN_EMPTY);
    hdata = NULL;
    name = NULL;
    if (!PyArg_ParseTuple (args, "ss", &hdata, &name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_hdata_get_var_type_string (API_STR2PTR(hdata), name);

    API_RETURN_STRING(result);
}

API_FUNC(hdata_get_var_array_size)
{
    char *hdata, *pointer, *name;
    int value;

    API_INIT_FUNC(1, "hdata_get_var_array_size", API_RETURN_INT(-1));
    hdata = NULL;
    pointer = NULL;
    name = NULL;
    if (!PyArg_ParseTuple (args, "sss", &hdata, &pointer, &name))
        API_WRONG_ARGS(API_RETURN_INT(-1));

    value = weechat_hdata_get_var_array_size (API_STR2PTR(hdata),
                                              API_STR2PTR(pointer),
                                              name);

    API_RETURN_INT(value);
}

API_FUNC(hdata_get_var_array_size_string)
{
    char *hdata, *pointer, *name;
    const char *result;

    API_INIT_FUNC(1, "hdata_get_var_array_size_string", API_RETURN_EMPTY);
    hdata = NULL;
    pointer = NULL;
    name = NULL;
    if (!PyArg_ParseTuple (args, "sss", &hdata, &pointer, &name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_hdata_get_var_array_size_string (API_STR2PTR(hdata),
                                                      API_STR2PTR(pointer),
                                                      name);

    API_RETURN_STRING(result);
}

API_FUNC(hdata_get_var_hdata)
{
    char *hdata, *name;
    const char *result;

    API_INIT_FUNC(1, "hdata_get_var_hdata", API_RETURN_EMPTY);
    hdata = NULL;
    name = NULL;
    if (!PyArg_ParseTuple (args, "ss", &hdata, &name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_hdata_get_var_hdata (API_STR2PTR(hdata), name);

    API_RETURN_STRING(result);
}

API_FUNC(hdata_get_list)
{
    char *hdata, *name;
    const char *result;

    API_INIT_FUNC(1, "hdata_get_list", API_RETURN_EMPTY);
    hdata = NULL;
    name = NULL;
    if (!PyArg_ParseTuple (args, "ss", &hdata, &name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_hdata_get_list (API_STR2PTR(hdata),
                                                 name));

    API_RETURN_STRING(result);
}

API_FUNC(hdata_check_pointer)
{
    char *hdata, *list, *pointer;
    int value;

    API_INIT_FUNC(1, "hdata_check_pointer", API_RETURN_INT(0));
    hdata = NULL;
    list = NULL;
    pointer = NULL;
    if (!PyArg_ParseTuple (args, "sss", &hdata, &list, &pointer))
        API_WRONG_ARGS(API_RETURN_INT(0));

    value = weechat_hdata_check_pointer (API_STR2PTR(hdata),
                                         API_STR2PTR(list),
                                         API_STR2PTR(pointer));

    API_RETURN_INT(value);
}

API_FUNC(hdata_move)
{
    char *hdata, *pointer;
    const char *result;
    int count;

    API_INIT_FUNC(1, "hdata_move", API_RETURN_EMPTY);
    hdata = NULL;
    pointer = NULL;
    count = 0;
    if (!PyArg_ParseTuple (args, "ssi", &hdata, &pointer, &count))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_hdata_move (API_STR2PTR(hdata),
                                             API_STR2PTR(pointer),
                                             count));

    API_RETURN_STRING(result);
}

API_FUNC(hdata_search)
{
    char *hdata, *pointer, *search;
    const char *result;
    struct t_hashtable *pointers, *extra_vars, *options;
    PyObject *dict, *dict2, *dict3;
    int move;

    API_INIT_FUNC(1, "hdata_search", API_RETURN_EMPTY);
    hdata = NULL;
    pointer = NULL;
    search = NULL;
    pointers = NULL;
    extra_vars = NULL;
    options = NULL;
    move = 0;
    if (!PyArg_ParseTuple (args, "sssOOOi", &hdata, &pointer, &search,
                           &dict, &dict2, &dict3, &move))
    {
        API_WRONG_ARGS(API_RETURN_EMPTY);
    }

    pointers = weechat_python_dict_to_hashtable (dict,
                                                 WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                                 WEECHAT_HASHTABLE_STRING,
                                                 WEECHAT_HASHTABLE_POINTER);
    extra_vars = weechat_python_dict_to_hashtable (dict2,
                                                   WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                                   WEECHAT_HASHTABLE_STRING,
                                                   WEECHAT_HASHTABLE_STRING);
    options = weechat_python_dict_to_hashtable (dict3,
                                                WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                                WEECHAT_HASHTABLE_STRING,
                                                WEECHAT_HASHTABLE_STRING);

    result = API_PTR2STR(weechat_hdata_search (API_STR2PTR(hdata),
                                               API_STR2PTR(pointer),
                                               search,
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
    char *hdata, *pointer, *name;
    int value;

    API_INIT_FUNC(1, "hdata_char", API_RETURN_INT(0));
    hdata = NULL;
    pointer = NULL;
    name = NULL;
    if (!PyArg_ParseTuple (args, "sss", &hdata, &pointer, &name))
        API_WRONG_ARGS(API_RETURN_INT(0));

    value = (int)weechat_hdata_char (API_STR2PTR(hdata),
                                     API_STR2PTR(pointer),
                                     name);

    API_RETURN_INT(value);
}

API_FUNC(hdata_integer)
{
    char *hdata, *pointer, *name;
    int value;

    API_INIT_FUNC(1, "hdata_integer", API_RETURN_INT(0));
    hdata = NULL;
    pointer = NULL;
    name = NULL;
    if (!PyArg_ParseTuple (args, "sss", &hdata, &pointer, &name))
        API_WRONG_ARGS(API_RETURN_INT(0));

    value = weechat_hdata_integer (API_STR2PTR(hdata),
                                   API_STR2PTR(pointer),
                                   name);

    API_RETURN_INT(value);
}

API_FUNC(hdata_long)
{
    char *hdata, *pointer, *name;
    long value;

    API_INIT_FUNC(1, "hdata_long", API_RETURN_LONG(0));
    hdata = NULL;
    pointer = NULL;
    name = NULL;
    if (!PyArg_ParseTuple (args, "sss", &hdata, &pointer, &name))
        API_WRONG_ARGS(API_RETURN_LONG(0));

    value = weechat_hdata_long (API_STR2PTR(hdata),
                                API_STR2PTR(pointer),
                                name);

    API_RETURN_LONG(value);
}

API_FUNC(hdata_string)
{
    char *hdata, *pointer, *name;
    const char *result;

    API_INIT_FUNC(1, "hdata_string", API_RETURN_EMPTY);
    hdata = NULL;
    pointer = NULL;
    name = NULL;
    if (!PyArg_ParseTuple (args, "sss", &hdata, &pointer, &name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_hdata_string (API_STR2PTR(hdata),
                                   API_STR2PTR(pointer),
                                   name);

    API_RETURN_STRING(result);
}

API_FUNC(hdata_pointer)
{
    char *hdata, *pointer, *name;
    const char *result;

    API_INIT_FUNC(1, "hdata_pointer", API_RETURN_EMPTY);
    hdata = NULL;
    pointer = NULL;
    name = NULL;
    if (!PyArg_ParseTuple (args, "sss", &hdata, &pointer, &name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_hdata_pointer (API_STR2PTR(hdata),
                                                API_STR2PTR(pointer),
                                                name));

    API_RETURN_STRING(result);
}

API_FUNC(hdata_time)
{
    char *hdata, *pointer, *name;
    time_t time;

    API_INIT_FUNC(1, "hdata_time", API_RETURN_LONG(0));
    hdata = NULL;
    pointer = NULL;
    name = NULL;
    if (!PyArg_ParseTuple (args, "sss", &hdata, &pointer, &name))
        API_WRONG_ARGS(API_RETURN_LONG(0));

    time = weechat_hdata_time (API_STR2PTR(hdata),
                               API_STR2PTR(pointer),
                               name);

    API_RETURN_LONG(time);
}

API_FUNC(hdata_hashtable)
{
    char *hdata, *pointer, *name;
    PyObject *result_dict;

    API_INIT_FUNC(1, "hdata_hashtable", API_RETURN_EMPTY);
    hdata = NULL;
    pointer = NULL;
    name = NULL;
    if (!PyArg_ParseTuple (args, "sss", &hdata, &pointer, &name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result_dict = weechat_python_hashtable_to_dict (
        weechat_hdata_hashtable (API_STR2PTR(hdata),
                                 API_STR2PTR(pointer),
                                 name));

    return result_dict;
}

API_FUNC(hdata_compare)
{
    char *hdata, *pointer1, *pointer2, *name;
    int case_sensitive, rc;

    API_INIT_FUNC(1, "hdata_compare", API_RETURN_INT(0));
    hdata = NULL;
    pointer1 = NULL;
    pointer2 = NULL;
    name = NULL;
    case_sensitive = 0;
    if (!PyArg_ParseTuple (args, "ssssi", &hdata, &pointer1, &pointer2, &name,
                           &case_sensitive))
        API_WRONG_ARGS(API_RETURN_INT(0));

    rc = weechat_hdata_compare (API_STR2PTR(hdata),
                                API_STR2PTR(pointer1),
                                API_STR2PTR(pointer2),
                                name,
                                case_sensitive);

    API_RETURN_INT(rc);
}

API_FUNC(hdata_update)
{
    char *hdata, *pointer;
    struct t_hashtable *hashtable;
    PyObject *dict;
    int value;

    API_INIT_FUNC(1, "hdata_update", API_RETURN_INT(0));
    hdata = NULL;
    pointer = NULL;
    dict = NULL;
    if (!PyArg_ParseTuple (args, "ssO", &hdata, &pointer, &dict))
        API_WRONG_ARGS(API_RETURN_INT(0));
    hashtable = weechat_python_dict_to_hashtable (dict,
                                                  WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                                  WEECHAT_HASHTABLE_STRING,
                                                  WEECHAT_HASHTABLE_STRING);

    value = weechat_hdata_update (API_STR2PTR(hdata),
                                  API_STR2PTR(pointer),
                                  hashtable);

    if (hashtable)
        weechat_hashtable_free (hashtable);

    API_RETURN_INT(value);
}

API_FUNC(hdata_get_string)
{
    char *hdata, *property;
    const char *result;

    API_INIT_FUNC(1, "hdata_get_string", API_RETURN_EMPTY);
    hdata = NULL;
    property = NULL;
    if (!PyArg_ParseTuple (args, "ss", &hdata, &property))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = weechat_hdata_get_string (API_STR2PTR(hdata), property);

    API_RETURN_STRING(result);
}

int
weechat_python_api_upgrade_read_cb (const void *pointer, void *data,
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

        rc = (int *) weechat_python_exec (script,
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

API_FUNC(upgrade_new)
{
    char *filename, *function, *data;
    const char *result;

    API_INIT_FUNC(1, "upgrade_new", API_RETURN_EMPTY);
    filename = NULL;
    function = NULL;
    data = NULL;
    if (!PyArg_ParseTuple (args, "sss", &filename, &function, &data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    result = API_PTR2STR(
        plugin_script_api_upgrade_new (
            weechat_python_plugin,
            python_current_script,
            filename,
            &weechat_python_api_upgrade_read_cb,
            function,
            data));

    API_RETURN_STRING(result);
}

API_FUNC(upgrade_write_object)
{
    char *upgrade_file, *infolist;
    int object_id, rc;

    API_INIT_FUNC(1, "upgrade_write_object", API_RETURN_INT(0));
    upgrade_file = NULL;
    object_id = 0;
    infolist = NULL;
    if (!PyArg_ParseTuple (args, "sis", &upgrade_file, &object_id, &infolist))
        API_WRONG_ARGS(API_RETURN_INT(0));

    rc = weechat_upgrade_write_object (API_STR2PTR(upgrade_file),
                                       object_id,
                                       API_STR2PTR(infolist));

    API_RETURN_INT(rc);
}

API_FUNC(upgrade_read)
{
    char *upgrade_file;
    int rc;

    API_INIT_FUNC(1, "upgrade_read", API_RETURN_INT(0));
    upgrade_file = NULL;
    if (!PyArg_ParseTuple (args, "s", &upgrade_file))
        API_WRONG_ARGS(API_RETURN_INT(0));

    rc = weechat_upgrade_read (API_STR2PTR(upgrade_file));

    API_RETURN_INT(rc);
}

API_FUNC(upgrade_close)
{
    char *upgrade_file;

    API_INIT_FUNC(1, "upgrade_close", API_RETURN_ERROR);
    upgrade_file = NULL;
    if (!PyArg_ParseTuple (args, "s", &upgrade_file))
        API_WRONG_ARGS(API_RETURN_ERROR);

    weechat_upgrade_close (API_STR2PTR(upgrade_file));

    API_RETURN_OK;
}

/*
 * Initializes python functions.
 */

PyMethodDef weechat_python_funcs[] =
{
    API_DEF_FUNC(register),
    API_DEF_FUNC(plugin_get_name),
    API_DEF_FUNC(charset_set),
    API_DEF_FUNC(iconv_to_internal),
    API_DEF_FUNC(iconv_from_internal),
    API_DEF_FUNC(gettext),
    API_DEF_FUNC(ngettext),
    API_DEF_FUNC(strlen_screen),
    API_DEF_FUNC(string_match),
    API_DEF_FUNC(string_match_list),
    API_DEF_FUNC(string_has_highlight),
    API_DEF_FUNC(string_has_highlight_regex),
    API_DEF_FUNC(string_mask_to_regex),
    API_DEF_FUNC(string_format_size),
    API_DEF_FUNC(string_parse_size),
    API_DEF_FUNC(string_color_code_size),
    API_DEF_FUNC(string_remove_color),
    API_DEF_FUNC(string_is_command_char),
    API_DEF_FUNC(string_input_for_buffer),
    API_DEF_FUNC(string_eval_expression),
    API_DEF_FUNC(string_eval_path_home),
    API_DEF_FUNC(mkdir_home),
    API_DEF_FUNC(mkdir),
    API_DEF_FUNC(mkdir_parents),
    API_DEF_FUNC(list_new),
    API_DEF_FUNC(list_add),
    API_DEF_FUNC(list_search),
    API_DEF_FUNC(list_search_pos),
    API_DEF_FUNC(list_casesearch),
    API_DEF_FUNC(list_casesearch_pos),
    API_DEF_FUNC(list_get),
    API_DEF_FUNC(list_set),
    API_DEF_FUNC(list_next),
    API_DEF_FUNC(list_prev),
    API_DEF_FUNC(list_string),
    API_DEF_FUNC(list_size),
    API_DEF_FUNC(list_remove),
    API_DEF_FUNC(list_remove_all),
    API_DEF_FUNC(list_free),
    API_DEF_FUNC(config_new),
    API_DEF_FUNC(config_set_version),
    API_DEF_FUNC(config_new_section),
    API_DEF_FUNC(config_search_section),
    API_DEF_FUNC(config_new_option),
    API_DEF_FUNC(config_search_option),
    API_DEF_FUNC(config_string_to_boolean),
    API_DEF_FUNC(config_option_reset),
    API_DEF_FUNC(config_option_set),
    API_DEF_FUNC(config_option_set_null),
    API_DEF_FUNC(config_option_unset),
    API_DEF_FUNC(config_option_rename),
    API_DEF_FUNC(config_option_is_null),
    API_DEF_FUNC(config_option_default_is_null),
    API_DEF_FUNC(config_boolean),
    API_DEF_FUNC(config_boolean_default),
    API_DEF_FUNC(config_integer),
    API_DEF_FUNC(config_integer_default),
    API_DEF_FUNC(config_string),
    API_DEF_FUNC(config_string_default),
    API_DEF_FUNC(config_color),
    API_DEF_FUNC(config_color_default),
    API_DEF_FUNC(config_write_option),
    API_DEF_FUNC(config_write_line),
    API_DEF_FUNC(config_write),
    API_DEF_FUNC(config_read),
    API_DEF_FUNC(config_reload),
    API_DEF_FUNC(config_option_free),
    API_DEF_FUNC(config_section_free_options),
    API_DEF_FUNC(config_section_free),
    API_DEF_FUNC(config_free),
    API_DEF_FUNC(config_get),
    API_DEF_FUNC(config_get_plugin),
    API_DEF_FUNC(config_is_set_plugin),
    API_DEF_FUNC(config_set_plugin),
    API_DEF_FUNC(config_set_desc_plugin),
    API_DEF_FUNC(config_unset_plugin),
    API_DEF_FUNC(key_bind),
    API_DEF_FUNC(key_unbind),
    API_DEF_FUNC(prefix),
    API_DEF_FUNC(color),
    API_DEF_FUNC(prnt),
    API_DEF_FUNC(prnt_date_tags),
    API_DEF_FUNC(prnt_y),
    API_DEF_FUNC(prnt_y_date_tags),
    API_DEF_FUNC(log_print),
    API_DEF_FUNC(hook_command),
    API_DEF_FUNC(hook_completion),
    API_DEF_FUNC(hook_completion_get_string),
    API_DEF_FUNC(hook_completion_list_add),
    API_DEF_FUNC(hook_command_run),
    API_DEF_FUNC(hook_timer),
    API_DEF_FUNC(hook_fd),
    API_DEF_FUNC(hook_process),
    API_DEF_FUNC(hook_process_hashtable),
    API_DEF_FUNC(hook_connect),
    API_DEF_FUNC(hook_line),
    API_DEF_FUNC(hook_print),
    API_DEF_FUNC(hook_signal),
    API_DEF_FUNC(hook_signal_send),
    API_DEF_FUNC(hook_hsignal),
    API_DEF_FUNC(hook_hsignal_send),
    API_DEF_FUNC(hook_config),
    API_DEF_FUNC(hook_modifier),
    API_DEF_FUNC(hook_modifier_exec),
    API_DEF_FUNC(hook_info),
    API_DEF_FUNC(hook_info_hashtable),
    API_DEF_FUNC(hook_infolist),
    API_DEF_FUNC(hook_focus),
    API_DEF_FUNC(hook_set),
    API_DEF_FUNC(unhook),
    API_DEF_FUNC(unhook_all),
    API_DEF_FUNC(buffer_new),
    API_DEF_FUNC(buffer_new_props),
    API_DEF_FUNC(buffer_search),
    API_DEF_FUNC(buffer_search_main),
    API_DEF_FUNC(current_buffer),
    API_DEF_FUNC(buffer_clear),
    API_DEF_FUNC(buffer_close),
    API_DEF_FUNC(buffer_merge),
    API_DEF_FUNC(buffer_unmerge),
    API_DEF_FUNC(buffer_get_integer),
    API_DEF_FUNC(buffer_get_string),
    API_DEF_FUNC(buffer_get_pointer),
    API_DEF_FUNC(buffer_set),
    API_DEF_FUNC(buffer_string_replace_local_var),
    API_DEF_FUNC(buffer_match_list),
    API_DEF_FUNC(current_window),
    API_DEF_FUNC(window_search_with_buffer),
    API_DEF_FUNC(window_get_integer),
    API_DEF_FUNC(window_get_string),
    API_DEF_FUNC(window_get_pointer),
    API_DEF_FUNC(window_set_title),
    API_DEF_FUNC(nicklist_add_group),
    API_DEF_FUNC(nicklist_search_group),
    API_DEF_FUNC(nicklist_add_nick),
    API_DEF_FUNC(nicklist_search_nick),
    API_DEF_FUNC(nicklist_remove_group),
    API_DEF_FUNC(nicklist_remove_nick),
    API_DEF_FUNC(nicklist_remove_all),
    API_DEF_FUNC(nicklist_group_get_integer),
    API_DEF_FUNC(nicklist_group_get_string),
    API_DEF_FUNC(nicklist_group_get_pointer),
    API_DEF_FUNC(nicklist_group_set),
    API_DEF_FUNC(nicklist_nick_get_integer),
    API_DEF_FUNC(nicklist_nick_get_string),
    API_DEF_FUNC(nicklist_nick_get_pointer),
    API_DEF_FUNC(nicklist_nick_set),
    API_DEF_FUNC(bar_item_search),
    API_DEF_FUNC(bar_item_new),
    API_DEF_FUNC(bar_item_update),
    API_DEF_FUNC(bar_item_remove),
    API_DEF_FUNC(bar_search),
    API_DEF_FUNC(bar_new),
    API_DEF_FUNC(bar_set),
    API_DEF_FUNC(bar_update),
    API_DEF_FUNC(bar_remove),
    API_DEF_FUNC(command),
    API_DEF_FUNC(command_options),
    API_DEF_FUNC(completion_new),
    API_DEF_FUNC(completion_search),
    API_DEF_FUNC(completion_get_string),
    API_DEF_FUNC(completion_list_add),
    API_DEF_FUNC(completion_free),
    API_DEF_FUNC(info_get),
    API_DEF_FUNC(info_get_hashtable),
    API_DEF_FUNC(infolist_new),
    API_DEF_FUNC(infolist_new_item),
    API_DEF_FUNC(infolist_new_var_integer),
    API_DEF_FUNC(infolist_new_var_string),
    API_DEF_FUNC(infolist_new_var_pointer),
    API_DEF_FUNC(infolist_new_var_time),
    API_DEF_FUNC(infolist_search_var),
    API_DEF_FUNC(infolist_get),
    API_DEF_FUNC(infolist_next),
    API_DEF_FUNC(infolist_prev),
    API_DEF_FUNC(infolist_reset_item_cursor),
    API_DEF_FUNC(infolist_fields),
    API_DEF_FUNC(infolist_integer),
    API_DEF_FUNC(infolist_string),
    API_DEF_FUNC(infolist_pointer),
    API_DEF_FUNC(infolist_time),
    API_DEF_FUNC(infolist_free),
    API_DEF_FUNC(hdata_get),
    API_DEF_FUNC(hdata_get_var_offset),
    API_DEF_FUNC(hdata_get_var_type_string),
    API_DEF_FUNC(hdata_get_var_array_size),
    API_DEF_FUNC(hdata_get_var_array_size_string),
    API_DEF_FUNC(hdata_get_var_hdata),
    API_DEF_FUNC(hdata_get_list),
    API_DEF_FUNC(hdata_check_pointer),
    API_DEF_FUNC(hdata_move),
    API_DEF_FUNC(hdata_search),
    API_DEF_FUNC(hdata_char),
    API_DEF_FUNC(hdata_integer),
    API_DEF_FUNC(hdata_long),
    API_DEF_FUNC(hdata_string),
    API_DEF_FUNC(hdata_pointer),
    API_DEF_FUNC(hdata_time),
    API_DEF_FUNC(hdata_hashtable),
    API_DEF_FUNC(hdata_compare),
    API_DEF_FUNC(hdata_update),
    API_DEF_FUNC(hdata_get_string),
    API_DEF_FUNC(upgrade_new),
    API_DEF_FUNC(upgrade_write_object),
    API_DEF_FUNC(upgrade_read),
    API_DEF_FUNC(upgrade_close),
    { NULL, NULL, 0, NULL }
};
