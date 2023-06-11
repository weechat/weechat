/*
 * weechat-js-api.cpp - javascript API functions
 *
 * Copyright (C) 2013 Koka El Kiwi <kokakiwi@kokakiwi.net>
 * Copyright (C) 2015-2023 Sébastien Helleu <flashcode@flashtux.org>
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

#include <cstdlib>
#include <cstring>
#include <string>
#include <ctime>

extern "C"
{
#include "../weechat-plugin.h"
#include "../plugin-script.h"
#include "../plugin-script-api.h"
}

#include "weechat-js.h"
#include "weechat-js-v8.h"
#include "weechat-js-api.h"


#define API_DEF_FUNC(__name)                                            \
    weechat_obj->Set(                                                   \
        v8::String::New(#__name),                                       \
        v8::FunctionTemplate::New(weechat_js_api_##__name));
#define API_DEF_CONST_INT(__name)                                       \
    weechat_obj->Set(v8::String::New(#__name),                          \
                     v8::Integer::New(__name));
#define API_DEF_CONST_STR(__name)                                       \
    weechat_obj->Set(v8::String::New(#__name),                          \
                     v8::String::New(__name));
#define API_FUNC(__name)                                                \
    static v8::Handle<v8::Value>                                        \
    weechat_js_api_##__name(const v8::Arguments &args)
#define API_INIT_FUNC(__init, __name, __args_fmt, __ret)                \
    std::string js_function_name(__name);                               \
    std::string js_args(__args_fmt);                                    \
    int js_args_len = js_args.size();                                   \
    int num;                                                            \
    if (__init                                                          \
        && (!js_current_script || !js_current_script->name))            \
    {                                                                   \
        WEECHAT_SCRIPT_MSG_NOT_INIT(JS_CURRENT_SCRIPT_NAME,             \
                                    js_function_name.c_str());          \
        __ret;                                                          \
    }                                                                   \
    if (args.Length() < js_args_len)                                    \
    {                                                                   \
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(JS_CURRENT_SCRIPT_NAME,           \
                                      js_function_name.c_str());        \
        __ret;                                                          \
    }                                                                   \
    for (num = 0; num < js_args_len; num++)                             \
    {                                                                   \
        if (((js_args[num] == 's') && (!args[num]->IsString()))         \
            || ((js_args[num] == 'S') && (!(args[num]->IsString()       \
                  || args[num]->IsNull() || args[num]->IsUndefined()))) \
            || ((js_args[num] == 'i') && (!args[num]->IsInt32()))       \
            || ((js_args[num] == 'n') && (!args[num]->IsNumber()))      \
            || ((js_args[num] == 'h') && (!args[num]->IsObject())))     \
        {                                                               \
            WEECHAT_SCRIPT_MSG_WRONG_ARGS(JS_CURRENT_SCRIPT_NAME,       \
                                          js_function_name.c_str());    \
            __ret;                                                      \
        }                                                               \
    }
#define API_WRONG_ARGS(__ret)                                           \
    {                                                                   \
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(JS_CURRENT_SCRIPT_NAME,           \
                                      js_function_name.c_str());        \
        __ret;                                                          \
    }

#define API_PTR2STR(__pointer)                                          \
    plugin_script_ptr2str (__pointer)
#define API_STR2PTR(__string)                                           \
    plugin_script_str2ptr (weechat_js_plugin,                           \
                           JS_CURRENT_SCRIPT_NAME,                      \
                           js_function_name.c_str(), __string)
#define API_RETURN_OK return v8::True();
#define API_RETURN_ERROR return v8::False();
#define API_RETURN_EMPTY                                                \
    return v8::String::New("");
#define API_RETURN_STRING(__string)                                     \
    if (__string)                                                       \
        return v8::String::New(__string);                               \
    return v8::String::New("")
#define API_RETURN_STRING_FREE(__string)                                \
    if (__string)                                                       \
    {                                                                   \
        v8::Handle<v8::Value> return_value = v8::String::New(__string); \
        free ((void *)__string);                                        \
        return return_value;                                            \
    }                                                                   \
    return v8::String::New("")
#define API_RETURN_INT(__int)                                           \
    return v8::Integer::New(__int)
#define API_RETURN_LONG(__int)                                          \
    return v8::Number::New(__int)


/*
 * Registers a javascript script.
 */

API_FUNC(register)
{
    API_INIT_FUNC(0, "register", "sssssss", API_RETURN_ERROR);

    if (js_registered_script)
    {
        /* script already registered */
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: script \"%s\" already "
                                         "registered (register ignored)"),
                        weechat_prefix ("error"), JS_PLUGIN_NAME,
                        js_registered_script->name);
        API_RETURN_ERROR;
    }

    js_current_script = NULL;
    js_registered_script = NULL;

    v8::String::Utf8Value name(args[0]);
    v8::String::Utf8Value author(args[1]);
    v8::String::Utf8Value version(args[2]);
    v8::String::Utf8Value license(args[3]);
    v8::String::Utf8Value description(args[4]);
    v8::String::Utf8Value shutdown_func(args[5]);
    v8::String::Utf8Value charset(args[6]);

    if (plugin_script_search (js_scripts, *name))
    {
        /* another script already exists with same name */
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to register script "
                                         "\"%s\" (another script already "
                                         "exists with this name)"),
                        weechat_prefix ("error"), JS_PLUGIN_NAME, *name);
        API_RETURN_ERROR;
    }

    /* register script */
    js_current_script = plugin_script_add (weechat_js_plugin,
                                           &js_data,
                                           (js_current_script_filename) ?
                                           js_current_script_filename : "",
                                           *name, *author, *version,
                                           *license, *description,
                                           *shutdown_func, *charset);

    if (js_current_script)
    {
        js_registered_script = js_current_script;
        if ((weechat_js_plugin->debug >= 2) || !js_quiet)
        {
            weechat_printf (NULL,
                            weechat_gettext ("%s: registered script \"%s\", "
                                             "version %s (%s)"),
                            JS_PLUGIN_NAME, *name, *version, *description);
        }
        js_current_script->interpreter = js_current_interpreter;
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
    const char *result;

    API_INIT_FUNC(1, "plugin_get_name", "s", API_RETURN_EMPTY);

    v8::String::Utf8Value plugin(args[0]);

    result = weechat_plugin_get_name (
        (struct t_weechat_plugin *)API_STR2PTR(*plugin));

    API_RETURN_STRING(result);
}

API_FUNC(charset_set)
{
    API_INIT_FUNC(1, "charset_set", "s", API_RETURN_ERROR);

    v8::String::Utf8Value charset(args[0]);

    plugin_script_api_charset_set (js_current_script, *charset);

    API_RETURN_OK;
}

API_FUNC(iconv_to_internal)
{
    char *result;

    API_INIT_FUNC(1, "iconv_to_internal", "ss", API_RETURN_EMPTY);

    v8::String::Utf8Value charset(args[0]);
    v8::String::Utf8Value string(args[1]);

    result = weechat_iconv_to_internal (*charset, *string);

    API_RETURN_STRING_FREE(result);
}

API_FUNC(iconv_from_internal)
{
    char *result;

    API_INIT_FUNC(1, "iconv_from_internal", "ss", API_RETURN_EMPTY);

    v8::String::Utf8Value charset(args[0]);
    v8::String::Utf8Value string(args[1]);

    result = weechat_iconv_from_internal (*charset, *string);

    API_RETURN_STRING_FREE(result);
}

API_FUNC(gettext)
{
    const char *result;

    API_INIT_FUNC(1, "gettext", "s", API_RETURN_EMPTY);

    v8::String::Utf8Value string(args[0]);

    result = weechat_gettext (*string);

    API_RETURN_STRING(result);
}

API_FUNC(ngettext)
{
    const char *result;

    API_INIT_FUNC(1, "ngettext", "ssi", API_RETURN_EMPTY);

    v8::String::Utf8Value single(args[0]);
    v8::String::Utf8Value plural(args[1]);
    int count = args[2]->IntegerValue();

    result = weechat_ngettext (*single, *plural, count);

    API_RETURN_STRING(result);
}

API_FUNC(strlen_screen)
{
    int value;

    API_INIT_FUNC(1, "strlen_screen", "s", API_RETURN_INT(0));

    v8::String::Utf8Value string(args[0]);

    value = weechat_strlen_screen (*string);

    API_RETURN_INT(value);
}

API_FUNC(string_match)
{
    int value;

    API_INIT_FUNC(1, "string_match", "ssi", API_RETURN_INT(0));

    v8::String::Utf8Value string(args[0]);
    v8::String::Utf8Value mask(args[1]);
    int case_sensitive = args[2]->IntegerValue();

    value = weechat_string_match (*string, *mask, case_sensitive);

    API_RETURN_INT(value);
}

API_FUNC(string_match_list)
{
    int value;

    API_INIT_FUNC(1, "string_match_list", "ssi", API_RETURN_INT(0));

    v8::String::Utf8Value string(args[0]);
    v8::String::Utf8Value masks(args[1]);
    int case_sensitive = args[2]->IntegerValue();

    value = plugin_script_api_string_match_list (weechat_js_plugin,
                                                 *string,
                                                 *masks,
                                                 case_sensitive);

    API_RETURN_INT(value);
}

API_FUNC(string_has_highlight)
{
    int value;

    API_INIT_FUNC(1, "string_has_highlight", "ss", API_RETURN_INT(0));

    v8::String::Utf8Value string(args[0]);
    v8::String::Utf8Value highlight_words(args[1]);

    value = weechat_string_has_highlight (*string, *highlight_words);

    API_RETURN_INT(value);
}

API_FUNC(string_has_highlight_regex)
{
    int value;

    API_INIT_FUNC(1, "string_has_highlight_regex", "ss", API_RETURN_INT(0));

    v8::String::Utf8Value string(args[0]);
    v8::String::Utf8Value regex(args[1]);

    value = weechat_string_has_highlight_regex (*string, *regex);

    API_RETURN_INT(value);
}

API_FUNC(string_mask_to_regex)
{
    const char *result;

    API_INIT_FUNC(1, "string_mask_to_regex", "s", API_RETURN_EMPTY);

    v8::String::Utf8Value mask(args[0]);

    result = weechat_string_mask_to_regex (*mask);

    API_RETURN_STRING_FREE(result);
}

API_FUNC(string_format_size)
{
    unsigned long long size;
    char *result;

    API_INIT_FUNC(1, "string_format_size", "n", API_RETURN_EMPTY);

    size = args[0]->IntegerValue();

    result = weechat_string_format_size (size);

    API_RETURN_STRING_FREE(result);
}

API_FUNC(string_parse_size)
{
    unsigned long long value;

    API_INIT_FUNC(1, "string_parse_size", "s", API_RETURN_LONG(0));

    v8::String::Utf8Value size(args[0]);

    value = weechat_string_parse_size (*size);

    API_RETURN_LONG(value);
}

API_FUNC(string_color_code_size)
{
    int size;

    API_INIT_FUNC(1, "string_color_code_size", "s", API_RETURN_INT(0));

    v8::String::Utf8Value string(args[0]);

    size = weechat_string_color_code_size (*string);

    API_RETURN_INT(size);
}

API_FUNC(string_remove_color)
{
    char *result;

    API_INIT_FUNC(1, "string_remove_color", "ss", API_RETURN_EMPTY);

    v8::String::Utf8Value string(args[0]);
    v8::String::Utf8Value replacement(args[1]);

    result = weechat_string_remove_color (*string, *replacement);

    API_RETURN_STRING_FREE(result);
}

API_FUNC(string_is_command_char)
{
    int value;

    API_INIT_FUNC(1, "string_is_command_char", "s", API_RETURN_INT(0));

    v8::String::Utf8Value string(args[0]);

    value = weechat_string_is_command_char (*string);

    API_RETURN_INT(value);
}

API_FUNC(string_input_for_buffer)
{
    const char *result;

    API_INIT_FUNC(1, "string_input_for_buffer", "s", API_RETURN_EMPTY);

    v8::String::Utf8Value string(args[0]);

    result = weechat_string_input_for_buffer (*string);

    API_RETURN_STRING(result);
}

API_FUNC(string_eval_expression)
{
    struct t_hashtable *pointers, *extra_vars, *options;
    char *result;

    API_INIT_FUNC(1, "string_eval_expression", "shhh", API_RETURN_EMPTY);

    v8::String::Utf8Value expr(args[0]);
    pointers = weechat_js_object_to_hashtable (
        args[1]->ToObject(),
        WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_POINTER);
    extra_vars = weechat_js_object_to_hashtable (
        args[2]->ToObject(),
        WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING);
    options = weechat_js_object_to_hashtable (
        args[3]->ToObject(),
        WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING);

    result = weechat_string_eval_expression (*expr, pointers, extra_vars,
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
    struct t_hashtable *pointers, *extra_vars, *options;
    char *result;

    API_INIT_FUNC(1, "string_eval_path_home", "shhh", API_RETURN_EMPTY);

    v8::String::Utf8Value path(args[0]);
    pointers = weechat_js_object_to_hashtable (
        args[1]->ToObject(),
        WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_POINTER);
    extra_vars = weechat_js_object_to_hashtable (
        args[2]->ToObject(),
        WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING);
    options = weechat_js_object_to_hashtable (
        args[3]->ToObject(),
        WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING);

    result = weechat_string_eval_path_home (*path, pointers, extra_vars,
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
    int mode;

    API_INIT_FUNC(1, "mkdir_home", "si", API_RETURN_ERROR);

    v8::String::Utf8Value directory(args[0]);
    mode = args[1]->IntegerValue();

    if (weechat_mkdir_home (*directory, mode))
        API_RETURN_OK;

    API_RETURN_ERROR;
}

API_FUNC(mkdir)
{
    int mode;

    API_INIT_FUNC(1, "mkdir", "si", API_RETURN_ERROR);

    v8::String::Utf8Value directory(args[0]);
    mode = args[1]->IntegerValue();

    if (weechat_mkdir (*directory, mode))
        API_RETURN_OK;

    API_RETURN_ERROR;
}

API_FUNC(mkdir_parents)
{
    int mode;

    API_INIT_FUNC(1, "mkdir_parents", "si", API_RETURN_ERROR);

    v8::String::Utf8Value directory(args[0]);
    mode = args[1]->IntegerValue();

    if (weechat_mkdir_parents (*directory, mode))
        API_RETURN_OK;

    API_RETURN_ERROR;
}

API_FUNC(list_new)
{
    const char *result;

    API_INIT_FUNC(1, "list_new", "", API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_list_new ());

    API_RETURN_STRING(result);
}

API_FUNC(list_add)
{
    const char *result;

    API_INIT_FUNC(1, "list_add", "ssss", API_RETURN_EMPTY);

    v8::String::Utf8Value weelist(args[0]);
    v8::String::Utf8Value data(args[1]);
    v8::String::Utf8Value where(args[2]);
    v8::String::Utf8Value user_data(args[3]);

    result = API_PTR2STR(
        weechat_list_add ((struct t_weelist *)API_STR2PTR(*weelist),
                          *data,
                          *where,
                          API_STR2PTR(*user_data)));

    API_RETURN_STRING(result);
}

API_FUNC(list_search)
{
    const char *result;

    API_INIT_FUNC(1, "list_search", "ss", API_RETURN_EMPTY);

    v8::String::Utf8Value weelist(args[0]);
    v8::String::Utf8Value data(args[1]);

    result = API_PTR2STR(
        weechat_list_search (
            (struct t_weelist *)API_STR2PTR(*weelist), *data));

    API_RETURN_STRING(result);
}

API_FUNC(list_search_pos)
{
    int pos;

    API_INIT_FUNC(1, "list_search_pos", "ss", API_RETURN_INT(-1));

    v8::String::Utf8Value weelist(args[0]);
    v8::String::Utf8Value data(args[1]);

    pos = weechat_list_search_pos (
        (struct t_weelist *)API_STR2PTR(*weelist), *data);

    API_RETURN_INT(pos);
}

API_FUNC(list_casesearch)
{
    const char *result;

    API_INIT_FUNC(1, "list_casesearch", "ss", API_RETURN_EMPTY);

    v8::String::Utf8Value weelist(args[0]);
    v8::String::Utf8Value data(args[1]);

    result = API_PTR2STR(
        weechat_list_casesearch (
            (struct t_weelist *)API_STR2PTR(*weelist), *data));

    API_RETURN_STRING(result);
}

API_FUNC(list_casesearch_pos)
{
    int pos;

    API_INIT_FUNC(1, "list_casesearch_pos", "ss", API_RETURN_INT(-1));

    v8::String::Utf8Value weelist(args[0]);
    v8::String::Utf8Value data(args[1]);

    pos = weechat_list_casesearch_pos (
        (struct t_weelist *)API_STR2PTR(*weelist), *data);

    API_RETURN_INT(pos);
}

API_FUNC(list_get)
{
    int position;
    const char *result;

    API_INIT_FUNC(1, "list_get", "si", API_RETURN_EMPTY);

    v8::String::Utf8Value weelist(args[0]);
    position = args[1]->IntegerValue();

    result = API_PTR2STR(
        weechat_list_get ((struct t_weelist *)API_STR2PTR(*weelist),
                          position));

    API_RETURN_STRING(result);
}

API_FUNC(list_set)
{
    API_INIT_FUNC(1, "list_set", "ss", API_RETURN_ERROR);

    v8::String::Utf8Value item(args[0]);
    v8::String::Utf8Value new_value(args[1]);

    weechat_list_set ((struct t_weelist_item *)API_STR2PTR(*item), *new_value);

    API_RETURN_OK;
}

API_FUNC(list_next)
{
    const char *result;

    API_INIT_FUNC(1, "list_next", "s", API_RETURN_EMPTY);

    v8::String::Utf8Value item(args[0]);

    result = API_PTR2STR(
        weechat_list_next ((struct t_weelist_item *)API_STR2PTR(*item)));

    API_RETURN_STRING(result);
}

API_FUNC(list_prev)
{
    const char *result;

    API_INIT_FUNC(1, "list_prev", "s", API_RETURN_EMPTY);

    v8::String::Utf8Value item(args[0]);

    result = API_PTR2STR(
        weechat_list_prev ((struct t_weelist_item *)API_STR2PTR(*item)));

    API_RETURN_STRING(result);
}

API_FUNC(list_string)
{
    const char *result;

    API_INIT_FUNC(1, "list_string", "s", API_RETURN_EMPTY);

    v8::String::Utf8Value item(args[0]);

    result = weechat_list_string (
        (struct t_weelist_item *)API_STR2PTR(*item));

    API_RETURN_STRING(result);
}

API_FUNC(list_size)
{
    int size;

    API_INIT_FUNC(1, "list_size", "s", API_RETURN_INT(0));

    v8::String::Utf8Value weelist(args[0]);

    size = weechat_list_size ((struct t_weelist *)API_STR2PTR(*weelist));

    API_RETURN_INT(size);
}

API_FUNC(list_remove)
{
    API_INIT_FUNC(1, "list_remove", "ss", API_RETURN_ERROR);

    v8::String::Utf8Value weelist(args[0]);
    v8::String::Utf8Value item(args[1]);

    weechat_list_remove ((struct t_weelist *)API_STR2PTR(*weelist),
                         (struct t_weelist_item *)API_STR2PTR(*item));

    API_RETURN_OK;
}

API_FUNC(list_remove_all)
{
    API_INIT_FUNC(1, "list_remove_all", "s", API_RETURN_ERROR);

    v8::String::Utf8Value weelist(args[0]);

    weechat_list_remove_all ((struct t_weelist *)API_STR2PTR(*weelist));

    API_RETURN_OK;
}

API_FUNC(list_free)
{
    API_INIT_FUNC(1, "list_free", "s", API_RETURN_ERROR);
    if (args.Length() < 1)
        API_WRONG_ARGS(API_RETURN_ERROR);

    v8::String::Utf8Value weelist(args[0]);

    weechat_list_free ((struct t_weelist *)API_STR2PTR(*weelist));

    API_RETURN_OK;
}

int
weechat_js_api_config_reload_cb (const void *pointer, void *data,
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

        rc = (int *)weechat_js_exec (script,
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
    const char *result;

    API_INIT_FUNC(1, "config_new", "sss", API_RETURN_EMPTY);

    v8::String::Utf8Value name(args[0]);
    v8::String::Utf8Value function(args[1]);
    v8::String::Utf8Value data(args[2]);

    result = API_PTR2STR(
        plugin_script_api_config_new (weechat_js_plugin,
                                      js_current_script,
                                      *name,
                                      &weechat_js_api_config_reload_cb,
                                      *function,
                                      *data));

    API_RETURN_STRING(result);
}

struct t_hashtable *
weechat_js_api_config_update_cb (const void *pointer, void *data,
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

        ret_hashtable = (struct t_hashtable *)weechat_js_exec (
            script,
            WEECHAT_SCRIPT_EXEC_HASHTABLE,
            ptr_function,
            "ssih", func_argv);

        return ret_hashtable;
    }

    return NULL;
}

API_FUNC(config_set_version)
{
    int rc, version;

    API_INIT_FUNC(1, "config_set_version", "siss", API_RETURN_INT(0));

    v8::String::Utf8Value config_file(args[0]);
    version = args[1]->IntegerValue();
    v8::String::Utf8Value function(args[2]);
    v8::String::Utf8Value data(args[3]);

    rc = plugin_script_api_config_set_version (
        weechat_js_plugin,
        js_current_script,
        (struct t_config_file *)API_STR2PTR(*config_file),
        version,
        &weechat_js_api_config_update_cb,
        *function,
        *data);

    API_RETURN_INT(rc);
}

int
weechat_js_api_config_read_cb (const void *pointer, void *data,
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

        rc = (int *)weechat_js_exec (script,
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
weechat_js_api_config_section_write_cb (const void *pointer, void *data,
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

        rc = (int *)weechat_js_exec (script,
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
weechat_js_api_config_section_write_default_cb (const void *pointer, void *data,
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

        rc = (int *)weechat_js_exec (script,
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
weechat_js_api_config_section_create_option_cb (const void *pointer, void *data,
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

        rc = (int *)weechat_js_exec (script,
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
weechat_js_api_config_section_delete_option_cb (const void *pointer, void *data,
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

        rc = (int *)weechat_js_exec (script,
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
    int user_can_add_options, user_can_delete_options;
    const char *result;

    API_INIT_FUNC(1, "config_new_section", "ssiissssssssss", API_RETURN_EMPTY);

    v8::String::Utf8Value config_file(args[0]);
    v8::String::Utf8Value name(args[1]);
    user_can_add_options = args[2]->IntegerValue();
    user_can_delete_options = args[3]->IntegerValue();
    v8::String::Utf8Value function_read(args[4]);
    v8::String::Utf8Value data_read(args[5]);
    v8::String::Utf8Value function_write(args[6]);
    v8::String::Utf8Value data_write(args[7]);
    v8::String::Utf8Value function_write_default(args[8]);
    v8::String::Utf8Value data_write_default(args[9]);
    v8::String::Utf8Value function_create_option(args[10]);
    v8::String::Utf8Value data_create_option(args[11]);
    v8::String::Utf8Value function_delete_option(args[12]);
    v8::String::Utf8Value data_delete_option(args[13]);

    result = API_PTR2STR(
        plugin_script_api_config_new_section (
            weechat_js_plugin,
            js_current_script,
            (struct t_config_file *)API_STR2PTR(*config_file),
            *name,
            user_can_add_options,
            user_can_delete_options,
            &weechat_js_api_config_read_cb,
            *function_read,
            *data_read,
            &weechat_js_api_config_section_write_cb,
            *function_write,
            *data_write,
            &weechat_js_api_config_section_write_default_cb,
            *function_write_default,
            *data_write_default,
            &weechat_js_api_config_section_create_option_cb,
            *function_create_option,
            *data_create_option,
            &weechat_js_api_config_section_delete_option_cb,
            *function_delete_option,
            *data_delete_option));

    API_RETURN_STRING(result);
}

API_FUNC(config_search_section)
{
    const char *result;

    API_INIT_FUNC(1, "config_search_section", "ss", API_RETURN_EMPTY);

    v8::String::Utf8Value config_file(args[0]);
    v8::String::Utf8Value section_name(args[1]);

    result = API_PTR2STR(
        weechat_config_search_section (
            (struct t_config_file *) API_STR2PTR(*config_file),
            *section_name));

    API_RETURN_STRING(result);
}

int
weechat_js_api_config_option_check_value_cb (const void *pointer, void *data,
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

        rc = (int *)weechat_js_exec (script,
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
weechat_js_api_config_option_change_cb (const void *pointer, void *data,
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

        rc = weechat_js_exec (script,
                              WEECHAT_SCRIPT_EXEC_IGNORE,
                              ptr_function,
                              "ss", func_argv);

        if (rc)
            free (rc);
    }
}

void
weechat_js_api_config_option_delete_cb (const void *pointer, void *data,
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

        rc = weechat_js_exec (script,
                              WEECHAT_SCRIPT_EXEC_IGNORE,
                              ptr_function,
                              "ss", func_argv);

        if (rc)
            free (rc);
    }
}

API_FUNC(config_new_option)
{
    int min, max, null_value_allowed;
    char *default_value, *value;
    const char *result;

    API_INIT_FUNC(1, "config_new_option", "ssssssiiSSissssss", API_RETURN_EMPTY);

    v8::String::Utf8Value config_file(args[0]);
    v8::String::Utf8Value section(args[1]);
    v8::String::Utf8Value name(args[2]);
    v8::String::Utf8Value type(args[3]);
    v8::String::Utf8Value description(args[4]);
    v8::String::Utf8Value string_values(args[5]);
    min = args[6]->IntegerValue();
    max = args[7]->IntegerValue();

    v8::String::Utf8Value v8_default_value(args[8]);
    if (args[8]->IsNull() || args[8]->IsUndefined())
        default_value = NULL;
    else
        default_value = *v8_default_value;

    v8::String::Utf8Value v8_value(args[9]);
    if (args[8]->IsNull() || args[8]->IsUndefined())
        value = NULL;
    else
        value = *v8_value;

    null_value_allowed = args[10]->IntegerValue();
    v8::String::Utf8Value function_check_value(args[11]);
    v8::String::Utf8Value data_check_value(args[12]);
    v8::String::Utf8Value function_change(args[13]);
    v8::String::Utf8Value data_change(args[14]);
    v8::String::Utf8Value function_delete(args[15]);
    v8::String::Utf8Value data_delete(args[16]);

    result = API_PTR2STR(
        plugin_script_api_config_new_option (
            weechat_js_plugin,
            js_current_script,
            (struct t_config_file *)API_STR2PTR(*config_file),
            (struct t_config_section *)API_STR2PTR(*section),
            *name,
            *type,
            *description,
            *string_values,
            min,
            max,
            default_value,
            value,
            null_value_allowed,
            &weechat_js_api_config_option_check_value_cb,
            *function_check_value,
            *data_check_value,
            &weechat_js_api_config_option_change_cb,
            *function_change,
            *data_change,
            &weechat_js_api_config_option_delete_cb,
            *function_delete,
            *data_delete));

    API_RETURN_STRING(result);
}

API_FUNC(config_search_option)
{
    const char *result;

    API_INIT_FUNC(1, "config_search_option", "sss", API_RETURN_EMPTY);

    v8::String::Utf8Value config_file(args[0]);
    v8::String::Utf8Value section(args[1]);
    v8::String::Utf8Value option_name(args[2]);

    result = API_PTR2STR(
        weechat_config_search_option (
            (struct t_config_file *)API_STR2PTR(*config_file),
            (struct t_config_section *)API_STR2PTR(*section),
            *option_name));

    API_RETURN_STRING(result);
}

API_FUNC(config_string_to_boolean)
{
    int value;

    API_INIT_FUNC(1, "config_string_to_boolean", "s", API_RETURN_INT(0));

    v8::String::Utf8Value text(args[0]);

    value = weechat_config_string_to_boolean (*text);

    API_RETURN_INT(value);
}

API_FUNC(config_option_reset)
{
    int run_callback, rc;

    API_INIT_FUNC(1, "config_option_reset", "si", API_RETURN_INT(0));

    v8::String::Utf8Value option(args[0]);
    run_callback = args[1]->IntegerValue();

    rc = weechat_config_option_reset (
        (struct t_config_option *)API_STR2PTR(*option), run_callback);

    API_RETURN_INT(rc);
}

API_FUNC(config_option_set)
{
    int run_callback, rc;

    API_INIT_FUNC(1, "config_option_set", "ssi", API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));

    v8::String::Utf8Value option(args[0]);
    v8::String::Utf8Value value(args[1]);
    run_callback = args[2]->IntegerValue();

    rc = weechat_config_option_set (
        (struct t_config_option *)API_STR2PTR(*option), *value, run_callback);

    API_RETURN_INT(rc);
}

API_FUNC(config_option_set_null)
{
    int run_callback, rc;

    API_INIT_FUNC(1, "config_option_set_null", "si", API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));

    v8::String::Utf8Value option(args[0]);
    run_callback = args[1]->IntegerValue();

    rc = weechat_config_option_set_null (
        (struct t_config_option *)API_STR2PTR(*option), run_callback);

    API_RETURN_INT(rc);
}

API_FUNC(config_option_unset)
{
    int rc;

    API_INIT_FUNC(1, "config_option_unset", "s", API_RETURN_INT(WEECHAT_CONFIG_OPTION_UNSET_ERROR));

    v8::String::Utf8Value option(args[0]);

    rc = weechat_config_option_unset (
        (struct t_config_option *) API_STR2PTR(*option));

    API_RETURN_INT(rc);
}

API_FUNC(config_option_rename)
{
    API_INIT_FUNC(1, "config_option_rename", "ss", API_RETURN_ERROR);

    v8::String::Utf8Value option(args[0]);
    v8::String::Utf8Value new_name(args[1]);

    weechat_config_option_rename (
        (struct t_config_option *)API_STR2PTR(*option), *new_name);

    API_RETURN_OK;
}

API_FUNC(config_option_is_null)
{
    int value;

    API_INIT_FUNC(1, "config_option_is_null", "s", API_RETURN_INT(1));

    v8::String::Utf8Value option(args[0]);

    value = weechat_config_option_is_null (
        (struct t_config_option *)API_STR2PTR(*option));

    API_RETURN_INT(value);
}

API_FUNC(config_option_default_is_null)
{
    int value;

    API_INIT_FUNC(1, "config_option_default_is_null", "s", API_RETURN_INT(1));

    v8::String::Utf8Value option(args[0]);

    value = weechat_config_option_default_is_null (
        (struct t_config_option *)API_STR2PTR(*option));

    API_RETURN_INT(value);
}

API_FUNC(config_boolean)
{
    int value;

    API_INIT_FUNC(1, "config_boolean", "s", API_RETURN_INT(0));

    v8::String::Utf8Value option(args[0]);

    value = weechat_config_boolean (
        (struct t_config_option *)API_STR2PTR(*option));

    API_RETURN_INT(value);
}

API_FUNC(config_boolean_default)
{
    int value;

    API_INIT_FUNC(1, "config_boolean_default", "s", API_RETURN_INT(0));

    v8::String::Utf8Value option(args[0]);

    value = weechat_config_boolean_default (
        (struct t_config_option *)API_STR2PTR(*option));

    API_RETURN_INT(value);
}

API_FUNC(config_integer)
{
    int value;

    API_INIT_FUNC(1, "config_integer", "s", API_RETURN_INT(0));

    v8::String::Utf8Value option(args[0]);

    value = weechat_config_integer (
        (struct t_config_option *)API_STR2PTR(*option));

    API_RETURN_INT(value);
}

API_FUNC(config_integer_default)
{
    int value;

    API_INIT_FUNC(1, "config_integer_default", "s", API_RETURN_INT(0));

    v8::String::Utf8Value option(args[0]);

    value = weechat_config_integer_default (
        (struct t_config_option *)API_STR2PTR(*option));

    API_RETURN_INT(value);
}

API_FUNC(config_string)
{
    const char *result;

    API_INIT_FUNC(1, "config_string", "s", API_RETURN_EMPTY);

    v8::String::Utf8Value option(args[0]);

    result = weechat_config_string (
        (struct t_config_option *)API_STR2PTR(*option));

    API_RETURN_STRING(result);
}

API_FUNC(config_string_default)
{
    const char *result;

    API_INIT_FUNC(1, "config_string_default", "s", API_RETURN_EMPTY);

    v8::String::Utf8Value option(args[0]);

    result = weechat_config_string_default (
        (struct t_config_option *)API_STR2PTR(*option));

    API_RETURN_STRING(result);
}

API_FUNC(config_color)
{
    const char *result;

    API_INIT_FUNC(1, "config_color", "s", API_RETURN_EMPTY);

    v8::String::Utf8Value option(args[0]);

    result = weechat_config_color (
        (struct t_config_option *)API_STR2PTR(*option));

    API_RETURN_STRING(result);
}

API_FUNC(config_color_default)
{
    const char *result;

    API_INIT_FUNC(1, "config_color_default", "s", API_RETURN_EMPTY);

    v8::String::Utf8Value option(args[0]);

    result = weechat_config_color_default (
        (struct t_config_option *)API_STR2PTR(*option));

    API_RETURN_STRING(result);
}

API_FUNC(config_write_option)
{
    API_INIT_FUNC(1, "config_write_option", "ss", API_RETURN_ERROR);

    v8::String::Utf8Value config_file(args[0]);
    v8::String::Utf8Value option(args[1]);

    weechat_config_write_option (
        (struct t_config_file *)API_STR2PTR(*config_file),
        (struct t_config_option *)API_STR2PTR(*option));

    API_RETURN_OK;
}

API_FUNC(config_write_line)
{
    API_INIT_FUNC(1, "config_write_line", "sss", API_RETURN_ERROR);

    v8::String::Utf8Value config_file(args[0]);
    v8::String::Utf8Value option_name(args[1]);
    v8::String::Utf8Value value(args[2]);

    weechat_config_write_line (
        (struct t_config_file *)API_STR2PTR(*config_file),
        *option_name,
        "%s",
        *value);

    API_RETURN_OK;
}

API_FUNC(config_write)
{
    int rc;

    API_INIT_FUNC(1, "config_write", "s", API_RETURN_INT(WEECHAT_CONFIG_WRITE_ERROR));

    v8::String::Utf8Value config_file(args[0]);

    rc = weechat_config_write (
        (struct t_config_file *)API_STR2PTR(*config_file));

    API_RETURN_INT(rc);
}

API_FUNC(config_read)
{
    int rc;

    API_INIT_FUNC(1, "config_read", "s", API_RETURN_INT(-1));

    v8::String::Utf8Value config_file(args[0]);

    rc = weechat_config_read (
        (struct t_config_file *)API_STR2PTR(*config_file));

    API_RETURN_INT(rc);
}

API_FUNC(config_reload)
{
    int rc;

    API_INIT_FUNC(1, "config_reload", "s", API_RETURN_INT(-1));

    v8::String::Utf8Value config_file(args[0]);

    rc = weechat_config_reload (
        (struct t_config_file *)API_STR2PTR(*config_file));

    API_RETURN_INT(rc);
}

API_FUNC(config_option_free)
{
    API_INIT_FUNC(1, "config_option_free", "s", API_RETURN_ERROR);

    v8::String::Utf8Value option(args[0]);

    weechat_config_option_free (
        (struct t_config_option *)API_STR2PTR(*option));

    API_RETURN_OK;
}

API_FUNC(config_section_free_options)
{
    API_INIT_FUNC(1, "config_section_free_options", "s", API_RETURN_ERROR);

    v8::String::Utf8Value section(args[0]);

    weechat_config_section_free_options (
        (struct t_config_section *)API_STR2PTR(*section));

    API_RETURN_OK;
}

API_FUNC(config_section_free)
{
    API_INIT_FUNC(1, "config_section_free", "s", API_RETURN_ERROR);

    v8::String::Utf8Value section(args[0]);

    weechat_config_section_free (
        (struct t_config_section *)API_STR2PTR(*section));

    API_RETURN_OK;
}

API_FUNC(config_free)
{
    API_INIT_FUNC(1, "config_free", "s", API_RETURN_ERROR);

    v8::String::Utf8Value config_file(args[0]);

    weechat_config_free ((struct t_config_file *)API_STR2PTR(*config_file));

    API_RETURN_OK;
}

API_FUNC(config_get)
{
    const char *result;

    API_INIT_FUNC(1, "config_get", "s", API_RETURN_EMPTY);

    v8::String::Utf8Value option(args[0]);

    result = API_PTR2STR(weechat_config_get (*option));

    API_RETURN_STRING(result);
}

API_FUNC(config_get_plugin)
{
    const char *result;

    API_INIT_FUNC(1, "config_get_plugin", "s", API_RETURN_EMPTY);

    v8::String::Utf8Value option(args[0]);

    result = plugin_script_api_config_get_plugin (weechat_js_plugin,
                                                  js_current_script,
                                                  *option);

    API_RETURN_STRING(result);
}

API_FUNC(config_is_set_plugin)
{
    int rc;

    API_INIT_FUNC(1, "config_is_set_plugin", "s", API_RETURN_INT(0));

    v8::String::Utf8Value option(args[0]);

    rc = plugin_script_api_config_is_set_plugin (weechat_js_plugin,
                                                 js_current_script,
                                                 *option);

    API_RETURN_INT(rc);
}

API_FUNC(config_set_plugin)
{
    int rc;

    API_INIT_FUNC(1, "config_set_plugin", "ss", API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));

    v8::String::Utf8Value option(args[0]);
    v8::String::Utf8Value value(args[1]);

    rc = plugin_script_api_config_set_plugin (weechat_js_plugin,
                                              js_current_script,
                                              *option,
                                              *value);

    API_RETURN_INT(rc);
}

API_FUNC(config_set_desc_plugin)
{
    API_INIT_FUNC(1, "config_set_desc_plugin", "ss", API_RETURN_ERROR);

    v8::String::Utf8Value option(args[0]);
    v8::String::Utf8Value description(args[1]);

    plugin_script_api_config_set_desc_plugin (weechat_js_plugin,
                                              js_current_script,
                                              *option,
                                              *description);

    API_RETURN_OK;
}

API_FUNC(config_unset_plugin)
{
    int rc;

    API_INIT_FUNC(1, "config_unset_plugin", "s", API_RETURN_INT(WEECHAT_CONFIG_OPTION_UNSET_ERROR));

    v8::String::Utf8Value option(args[0]);

    rc = plugin_script_api_config_unset_plugin (weechat_js_plugin,
                                                js_current_script,
                                                *option);

    API_RETURN_INT(rc);
}

API_FUNC(key_bind)
{
    struct t_hashtable *hashtable;
    v8::Handle<v8::Object> obj;
    int num_keys;

    API_INIT_FUNC(1, "key_bind", "sh", API_RETURN_INT(0));

    v8::String::Utf8Value context(args[0]);
    hashtable = weechat_js_object_to_hashtable (
        args[1]->ToObject(),
        WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING);

    num_keys = weechat_key_bind (*context, hashtable);

    if (hashtable)
        weechat_hashtable_free (hashtable);

    API_RETURN_INT(num_keys);
}

API_FUNC(key_unbind)
{
    int num_keys;

    API_INIT_FUNC(1, "key_unbind", "ss", API_RETURN_INT(0));

    v8::String::Utf8Value context(args[0]);
    v8::String::Utf8Value key(args[1]);

    num_keys = weechat_key_unbind (*context, *key);

    API_RETURN_INT(num_keys);
}

API_FUNC(prefix)
{
    const char *result;

    API_INIT_FUNC(0, "prefix", "s", API_RETURN_EMPTY);

    v8::String::Utf8Value prefix(args[0]);

    result = weechat_prefix (*prefix);

    API_RETURN_STRING(result);
}

API_FUNC(color)
{
    const char *result;

    API_INIT_FUNC(0, "color", "s", API_RETURN_EMPTY);

    v8::String::Utf8Value color(args[0]);

    result = weechat_color (*color);

    API_RETURN_STRING(result);
}

API_FUNC(print)
{
    API_INIT_FUNC(0, "print", "ss", API_RETURN_ERROR);

    v8::String::Utf8Value buffer(args[0]);
    v8::String::Utf8Value message(args[1]);

    plugin_script_api_printf (weechat_js_plugin,
                              js_current_script,
                              (struct t_gui_buffer *)API_STR2PTR(*buffer),
                              "%s", *message);

    API_RETURN_OK;
}

API_FUNC(print_date_tags)
{
    long date;

    API_INIT_FUNC(1, "print_date_tags", "snss", API_RETURN_ERROR);

    v8::String::Utf8Value buffer(args[0]);
    date = args[1]->IntegerValue();
    v8::String::Utf8Value tags(args[2]);
    v8::String::Utf8Value message(args[3]);

    plugin_script_api_printf_date_tags (
        weechat_js_plugin,
        js_current_script,
        (struct t_gui_buffer *)API_STR2PTR(*buffer),
        (time_t)date,
        *tags,
        "%s", *message);

    API_RETURN_OK;
}

API_FUNC(print_y)
{
    int y;

    API_INIT_FUNC(1, "print_y", "sis", API_RETURN_ERROR);

    v8::String::Utf8Value buffer(args[0]);
    y = args[1]->IntegerValue();
    v8::String::Utf8Value message(args[2]);

    plugin_script_api_printf_y (weechat_js_plugin,
                                js_current_script,
                                (struct t_gui_buffer *)API_STR2PTR(*buffer),
                                y,
                                "%s", *message);

    API_RETURN_OK;
}

API_FUNC(print_y_date_tags)
{
    int y;
    long date;

    API_INIT_FUNC(1, "print_y_date_tags", "sinss", API_RETURN_ERROR);

    v8::String::Utf8Value buffer(args[0]);
    y = args[1]->IntegerValue();
    date = args[2]->IntegerValue();
    v8::String::Utf8Value tags(args[3]);
    v8::String::Utf8Value message(args[4]);

    plugin_script_api_printf_y_date_tags (weechat_js_plugin,
                                          js_current_script,
                                          (struct t_gui_buffer *)API_STR2PTR(*buffer),
                                          y,
                                          (time_t)date,
                                          *tags,
                                          "%s", *message);

    API_RETURN_OK;
}

API_FUNC(log_print)
{
    API_INIT_FUNC(1, "log_print", "s", API_RETURN_ERROR);

    v8::String::Utf8Value message(args[0]);

    plugin_script_api_log_printf (weechat_js_plugin,
                                  js_current_script,
                                  "%s", *message);

    API_RETURN_OK;
}

int
weechat_js_api_hook_command_cb (const void *pointer, void *data,
                                struct t_gui_buffer *buffer,
                                int argc, char **argv, char **argv_eol)
{
    struct t_plugin_script *script;
    void *func_argv[3];
    char empty_arg[1] = { '\0' };
    const char *ptr_function, *ptr_data;
    int *rc, ret;

    /* make C++ compiler happy */
    (void) argv;

    script = (struct t_plugin_script *)pointer;
    plugin_script_get_function_and_data (data, &ptr_function, &ptr_data);

    if (ptr_function && ptr_function[0])
    {
        func_argv[0] = (ptr_data) ? (char *)ptr_data : empty_arg;
        func_argv[1] = (char *)API_PTR2STR(buffer);
        func_argv[2] = (argc > 1) ? argv_eol[1] : empty_arg;

        rc = (int *)weechat_js_exec (script,
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
    const char *result;

    API_INIT_FUNC(1, "hook_command", "sssssss", API_RETURN_EMPTY);

    v8::String::Utf8Value command(args[0]);
    v8::String::Utf8Value description(args[1]);
    v8::String::Utf8Value arguments(args[2]);
    v8::String::Utf8Value args_description(args[3]);
    v8::String::Utf8Value completion(args[4]);
    v8::String::Utf8Value function(args[5]);
    v8::String::Utf8Value data(args[6]);

    result = API_PTR2STR(
        plugin_script_api_hook_command (weechat_js_plugin,
                                        js_current_script,
                                        *command,
                                        *description,
                                        *arguments,
                                        *args_description,
                                        *completion,
                                        &weechat_js_api_hook_command_cb,
                                        *function,
                                        *data));

    API_RETURN_STRING(result);
}

int
weechat_js_api_hook_completion_cb (const void *pointer, void *data,
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

        rc = (int *)weechat_js_exec (script,
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
    const char *result;

    API_INIT_FUNC(1, "hook_completion", "ssss", API_RETURN_EMPTY);

    v8::String::Utf8Value completion(args[0]);
    v8::String::Utf8Value description(args[1]);
    v8::String::Utf8Value function(args[2]);
    v8::String::Utf8Value data(args[3]);

    result = API_PTR2STR(
        plugin_script_api_hook_completion (
            weechat_js_plugin,
            js_current_script,
            *completion,
            *description,
            &weechat_js_api_hook_completion_cb,
            *function,
            *data));

    API_RETURN_STRING(result);
}

/*
 * This function deprecated since WeeChat 2.9, kept for compatibility.
 * It is replaced by completion_get_string.
 */

API_FUNC(hook_completion_get_string)
{
    const char *result;

    API_INIT_FUNC(1, "hook_completion_get_string", "ss", API_RETURN_EMPTY);

    v8::String::Utf8Value completion(args[0]);
    v8::String::Utf8Value property(args[1]);

    result = weechat_hook_completion_get_string (
        (struct t_gui_completion *)API_STR2PTR(*completion),
        *property);

    API_RETURN_STRING(result);
}

/*
 * This function deprecated since WeeChat 2.9, kept for compatibility.
 * It is replaced by completion_list_add.
 */

API_FUNC(hook_completion_list_add)
{
    int nick_completion;

    API_INIT_FUNC(1, "hook_completion_list_add", "ssis", API_RETURN_ERROR);

    v8::String::Utf8Value completion(args[0]);
    v8::String::Utf8Value word(args[1]);
    nick_completion = args[2]->IntegerValue();
    v8::String::Utf8Value where(args[3]);

    weechat_hook_completion_list_add (
        (struct t_gui_completion *)API_STR2PTR(*completion),
        *word,
        nick_completion,
        *where);

    API_RETURN_OK;
}

int
weechat_js_api_hook_command_run_cb (const void *pointer, void *data,
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

        rc = (int *)weechat_js_exec (script,
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
    const char *result;

    API_INIT_FUNC(1, "hook_command_run", "sss", API_RETURN_EMPTY);

    v8::String::Utf8Value command(args[0]);
    v8::String::Utf8Value function(args[1]);
    v8::String::Utf8Value data(args[2]);

    result = API_PTR2STR(
        plugin_script_api_hook_command_run (
            weechat_js_plugin,
            js_current_script,
            *command,
            &weechat_js_api_hook_command_run_cb,
            *function,
            *data));

    API_RETURN_STRING(result);
}

int
weechat_js_api_hook_timer_cb (const void *pointer, void *data,
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

        rc = (int *)weechat_js_exec (script,
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
    const char *result;

    API_INIT_FUNC(1, "hook_timer", "niiss", API_RETURN_EMPTY);

    interval = args[0]->IntegerValue();
    align_second = args[1]->IntegerValue();
    max_calls = args[2]->IntegerValue();
    v8::String::Utf8Value function(args[3]);
    v8::String::Utf8Value data(args[4]);

    result = API_PTR2STR(
        plugin_script_api_hook_timer (
            weechat_js_plugin,
            js_current_script,
            interval,
            align_second,
            max_calls,
            &weechat_js_api_hook_timer_cb,
            *function,
            *data));

    API_RETURN_STRING(result);
}

int
weechat_js_api_hook_fd_cb (const void *pointer, void *data, int fd)
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

        rc = (int *)weechat_js_exec (script,
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
    const char *result;

    API_INIT_FUNC(1, "hook_fd", "iiiiss", API_RETURN_EMPTY);

    fd = args[0]->IntegerValue();
    read = args[1]->IntegerValue();
    write = args[2]->IntegerValue();
    exception = args[3]->IntegerValue();
    v8::String::Utf8Value function(args[4]);
    v8::String::Utf8Value data(args[5]);

    result = API_PTR2STR(
        plugin_script_api_hook_fd (
            weechat_js_plugin,
            js_current_script,
            fd,
            read,
            write,
            exception,
            &weechat_js_api_hook_fd_cb,
            *function,
            *data));

    API_RETURN_STRING(result);
}

int
weechat_js_api_hook_process_cb (const void *pointer, void *data,
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

            result = (char *) weechat_js_exec (script,
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

        rc = (int *)weechat_js_exec (script,
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
    int timeout;
    const char *result;

    API_INIT_FUNC(1, "hook_process", "siss", API_RETURN_EMPTY);

    v8::String::Utf8Value command(args[0]);
    timeout = args[1]->IntegerValue();
    v8::String::Utf8Value function(args[2]);
    v8::String::Utf8Value data(args[3]);

    result = API_PTR2STR(
        plugin_script_api_hook_process (
            weechat_js_plugin,
            js_current_script,
            *command,
            timeout,
            &weechat_js_api_hook_process_cb,
            *function,
            *data));

    API_RETURN_STRING(result);
}

API_FUNC(hook_process_hashtable)
{
    struct t_hashtable *options;
    int timeout;
    const char *result;

    API_INIT_FUNC(1, "hook_process_hashtable", "shiss", API_RETURN_EMPTY);

    v8::String::Utf8Value command(args[0]);
    options = weechat_js_object_to_hashtable (
        args[1]->ToObject(),
        WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING);
    timeout = args[2]->IntegerValue();
    v8::String::Utf8Value function(args[3]);
    v8::String::Utf8Value data(args[4]);

    result = API_PTR2STR(
        plugin_script_api_hook_process_hashtable (
            weechat_js_plugin,
            js_current_script,
            *command,
            options,
            timeout,
            &weechat_js_api_hook_process_cb,
            *function,
            *data));

    if (options)
        weechat_hashtable_free (options);

    API_RETURN_STRING(result);
}

int
weechat_js_api_hook_connect_cb (const void *pointer, void *data,
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

        rc = (int *)weechat_js_exec (script,
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
    int port, ipv6, retry;
    const char *result;

    API_INIT_FUNC(1, "hook_connect", "ssiiisss", API_RETURN_EMPTY);

    v8::String::Utf8Value proxy(args[0]);
    v8::String::Utf8Value address(args[1]);
    port = args[2]->IntegerValue();
    ipv6 = args[3]->IntegerValue();
    retry = args[4]->IntegerValue();
    v8::String::Utf8Value local_hostname(args[5]);
    v8::String::Utf8Value function(args[6]);
    v8::String::Utf8Value data(args[7]);

    result = API_PTR2STR(
        plugin_script_api_hook_connect (
            weechat_js_plugin,
            js_current_script,
            *proxy,
            *address,
            port,
            ipv6,
            retry,
            NULL, /* gnutls session */
            NULL, /* gnutls callback */
            0,    /* gnutls DH key size */
            NULL, /* gnutls priorities */
            *local_hostname,
            &weechat_js_api_hook_connect_cb,
            *function,
            *data));

    API_RETURN_STRING(result);
}

struct t_hashtable *
weechat_js_api_hook_line_cb (const void *pointer, void *data,
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

        ret_hashtable = (struct t_hashtable *)weechat_js_exec (
            script,
            WEECHAT_SCRIPT_EXEC_HASHTABLE,
            ptr_function,
            "sh", func_argv);

        return ret_hashtable;
    }

    return NULL;
}

API_FUNC(hook_line)
{
    const char *result;

    API_INIT_FUNC(1, "hook_line", "sssss", API_RETURN_EMPTY);

    v8::String::Utf8Value buffer_type(args[0]);
    v8::String::Utf8Value buffer_name(args[1]);
    v8::String::Utf8Value tags(args[2]);
    v8::String::Utf8Value function(args[3]);
    v8::String::Utf8Value data(args[4]);

    result = API_PTR2STR(
        plugin_script_api_hook_line (
            weechat_js_plugin,
            js_current_script,
            *buffer_type,
            *buffer_name,
            *tags,
            &weechat_js_api_hook_line_cb,
            *function,
            *data));

    API_RETURN_STRING(result);
}

int
weechat_js_api_hook_print_cb (const void *pointer, void *data,
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
        snprintf (timebuffer, sizeof (timebuffer), "%ld", (long int)date);

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

        rc = (int *)weechat_js_exec (script,
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
    int strip_colors;
    const char *result;

    API_INIT_FUNC(1, "hook_print", "sssiss", API_RETURN_EMPTY);

    v8::String::Utf8Value buffer(args[0]);
    v8::String::Utf8Value tags(args[1]);
    v8::String::Utf8Value message(args[2]);
    strip_colors = args[3]->IntegerValue();
    v8::String::Utf8Value function(args[4]);
    v8::String::Utf8Value data(args[5]);

    result = API_PTR2STR(
        plugin_script_api_hook_print (
            weechat_js_plugin,
            js_current_script,
            (struct t_gui_buffer *)API_STR2PTR(*buffer),
            *tags,
            *message,
            strip_colors,
            &weechat_js_api_hook_print_cb,
            *function,
            *data));

    API_RETURN_STRING(result);
}

int
weechat_js_api_hook_signal_cb (const void *pointer, void *data,
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

        rc = (int *)weechat_js_exec (script,
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
    const char *result;

    API_INIT_FUNC(1, "hook_signal", "sss", API_RETURN_EMPTY);

    v8::String::Utf8Value signal(args[0]);
    v8::String::Utf8Value function(args[1]);
    v8::String::Utf8Value data(args[2]);

    result = API_PTR2STR(
        plugin_script_api_hook_signal (
            weechat_js_plugin,
            js_current_script,
            *signal,
            &weechat_js_api_hook_signal_cb,
            *function,
            *data));

    API_RETURN_STRING(result);
}

API_FUNC(hook_signal_send)
{
    char *error;
    int number, rc;

    API_INIT_FUNC(1, "hook_signal_send", "sss", API_RETURN_INT(WEECHAT_RC_ERROR));

    v8::String::Utf8Value signal(args[0]);
    v8::String::Utf8Value type_data(args[1]);
    v8::String::Utf8Value signal_data(args[2]);

    if (strcmp (*type_data, WEECHAT_HOOK_SIGNAL_STRING) == 0)
    {
        rc = weechat_hook_signal_send (*signal, *type_data, *signal_data);
        API_RETURN_INT(rc);
    }
    else if (strcmp (*type_data, WEECHAT_HOOK_SIGNAL_INT) == 0)
    {
        error = NULL;
        number = (int)strtol (*signal_data, &error, 10);
        if (error && !error[0])
            rc = weechat_hook_signal_send (*signal, *type_data, &number);
        else
            rc = WEECHAT_RC_ERROR;
        API_RETURN_INT(rc);
    }
    else if (strcmp (*type_data, WEECHAT_HOOK_SIGNAL_POINTER) == 0)
    {
        rc = weechat_hook_signal_send (*signal, *type_data,
                                       API_STR2PTR(*signal_data));
        API_RETURN_INT(rc);
    }

    API_RETURN_INT(WEECHAT_RC_ERROR);
}

int
weechat_js_api_hook_hsignal_cb (const void *pointer, void *data,
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

        rc = (int *)weechat_js_exec (script,
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
    const char *result;

    API_INIT_FUNC(1, "hook_hsignal", "sss", API_RETURN_EMPTY);

    v8::String::Utf8Value signal(args[0]);
    v8::String::Utf8Value function(args[1]);
    v8::String::Utf8Value data(args[2]);

    result = API_PTR2STR(
        plugin_script_api_hook_hsignal (
            weechat_js_plugin,
            js_current_script,
            *signal,
            &weechat_js_api_hook_hsignal_cb,
            *function,
            *data));

    API_RETURN_STRING(result);
}

API_FUNC(hook_hsignal_send)
{
    struct t_hashtable *hashtable;
    int rc;

    API_INIT_FUNC(1, "hook_hsignal_send", "sh", API_RETURN_INT(WEECHAT_RC_ERROR));

    v8::String::Utf8Value signal(args[0]);
    hashtable = weechat_js_object_to_hashtable (
        args[1]->ToObject(),
        WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING);

    rc = weechat_hook_hsignal_send (*signal, hashtable);

    if (hashtable)
        weechat_hashtable_free (hashtable);

    API_RETURN_INT(rc);
}

int
weechat_js_api_hook_config_cb (const void *pointer, void *data,
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

        rc = (int *)weechat_js_exec (script,
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
    const char *result;

    API_INIT_FUNC(1, "hook_config", "sss", API_RETURN_EMPTY);

    v8::String::Utf8Value option(args[0]);
    v8::String::Utf8Value function(args[1]);
    v8::String::Utf8Value data(args[2]);

    result = API_PTR2STR(
        plugin_script_api_hook_config (
            weechat_js_plugin,
            js_current_script,
            *option,
            &weechat_js_api_hook_config_cb,
            *function,
            *data));

    API_RETURN_STRING(result);
}

char *
weechat_js_api_hook_modifier_cb (const void *pointer, void *data,
                                 const char *modifier,
                                 const char *modifier_data, const char *string)
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

        return (char *)weechat_js_exec (script,
                                        WEECHAT_SCRIPT_EXEC_STRING,
                                        ptr_function,
                                        "ssss", func_argv);
    }

    return NULL;
}

API_FUNC(hook_modifier)
{
    const char *result;

    API_INIT_FUNC(1, "hook_modifier", "sss", API_RETURN_EMPTY);

    v8::String::Utf8Value modifier(args[0]);
    v8::String::Utf8Value function(args[1]);
    v8::String::Utf8Value data(args[2]);

    result = API_PTR2STR(
        plugin_script_api_hook_modifier (
            weechat_js_plugin,
            js_current_script,
            *modifier,
            &weechat_js_api_hook_modifier_cb,
            *function,
            *data));

    API_RETURN_STRING(result);
}

API_FUNC(hook_modifier_exec)
{
    char *result;

    API_INIT_FUNC(1, "hook_modifier_exec", "sss", API_RETURN_EMPTY);

    v8::String::Utf8Value modifier(args[0]);
    v8::String::Utf8Value modifier_data(args[1]);
    v8::String::Utf8Value string(args[2]);

    result = weechat_hook_modifier_exec (*modifier, *modifier_data, *string);

    API_RETURN_STRING_FREE(result);
}

char *
weechat_js_api_hook_info_cb (const void *pointer, void *data,
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

        return (char *)weechat_js_exec (script,
                                        WEECHAT_SCRIPT_EXEC_STRING,
                                        ptr_function,
                                        "sss", func_argv);
    }

    return NULL;
}

API_FUNC(hook_info)
{
    const char *result;

    API_INIT_FUNC(1, "hook_info", "sssss", API_RETURN_EMPTY);

    v8::String::Utf8Value info_name(args[0]);
    v8::String::Utf8Value description(args[1]);
    v8::String::Utf8Value args_description(args[2]);
    v8::String::Utf8Value function(args[3]);
    v8::String::Utf8Value data(args[4]);

    result = API_PTR2STR(
        plugin_script_api_hook_info (
            weechat_js_plugin,
            js_current_script,
            *info_name,
            *description,
            *args_description,
            &weechat_js_api_hook_info_cb,
            *function,
            *data));

    API_RETURN_STRING(result);
}

struct t_hashtable *
weechat_js_api_hook_info_hashtable_cb (const void *pointer, void *data,
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

        ret_hashtable = (struct t_hashtable *)weechat_js_exec (
            script,
            WEECHAT_SCRIPT_EXEC_HASHTABLE,
            ptr_function,
            "ssh", func_argv);

        return ret_hashtable;
    }

    return NULL;
}

API_FUNC(hook_info_hashtable)
{
    const char *result;

    API_INIT_FUNC(1, "hook_info_hashtable", "ssssss", API_RETURN_EMPTY);

    v8::String::Utf8Value info_name(args[0]);
    v8::String::Utf8Value description(args[1]);
    v8::String::Utf8Value args_description(args[2]);
    v8::String::Utf8Value output_description(args[3]);
    v8::String::Utf8Value function(args[4]);
    v8::String::Utf8Value data(args[5]);

    result = API_PTR2STR(
        plugin_script_api_hook_info_hashtable (
            weechat_js_plugin,
            js_current_script,
            *info_name,
            *description,
            *args_description,
            *output_description,
            &weechat_js_api_hook_info_hashtable_cb,
            *function,
            *data));

    API_RETURN_STRING(result);
}

struct t_infolist *
weechat_js_api_hook_infolist_cb (const void *pointer, void *data,
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

        result = (struct t_infolist *)weechat_js_exec (
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
    const char *result;

    API_INIT_FUNC(1, "hook_infolist", "ssssss", API_RETURN_EMPTY);

    v8::String::Utf8Value infolist_name(args[0]);
    v8::String::Utf8Value description(args[1]);
    v8::String::Utf8Value pointer_description(args[2]);
    v8::String::Utf8Value args_description(args[3]);
    v8::String::Utf8Value function(args[4]);
    v8::String::Utf8Value data(args[5]);

    result = API_PTR2STR(
        plugin_script_api_hook_infolist (
            weechat_js_plugin,
            js_current_script,
            *infolist_name,
            *description,
            *pointer_description,
            *args_description,
            &weechat_js_api_hook_infolist_cb,
            *function,
            *data));

    API_RETURN_STRING(result);
}

struct t_hashtable *
weechat_js_api_hook_focus_cb (const void *pointer, void *data,
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

        ret_hashtable = (struct t_hashtable *)weechat_js_exec (
            script,
            WEECHAT_SCRIPT_EXEC_HASHTABLE,
            ptr_function,
            "sh", func_argv);

        return ret_hashtable;
    }

    return NULL;
}

API_FUNC(hook_focus)
{
    const char *result;

    API_INIT_FUNC(1, "hook_focus", "sss", API_RETURN_EMPTY);

    v8::String::Utf8Value area(args[0]);
    v8::String::Utf8Value function(args[1]);
    v8::String::Utf8Value data(args[2]);

    result = API_PTR2STR(
        plugin_script_api_hook_focus (
            weechat_js_plugin,
            js_current_script,
            *area,
            &weechat_js_api_hook_focus_cb,
            *function,
            *data));

    API_RETURN_STRING(result);
}

API_FUNC(hook_set)
{
    API_INIT_FUNC(1, "hook_set", "sss", API_RETURN_ERROR);

    v8::String::Utf8Value hook(args[0]);
    v8::String::Utf8Value property(args[1]);
    v8::String::Utf8Value value(args[2]);

    weechat_hook_set ((struct t_hook *)API_STR2PTR(*hook), *property, *value);

    API_RETURN_OK;
}

API_FUNC(unhook)
{
    API_INIT_FUNC(1, "unhook", "s", API_RETURN_ERROR);

    v8::String::Utf8Value hook(args[0]);

    weechat_unhook ((struct t_hook *)API_STR2PTR(*hook));

    API_RETURN_OK;
}

API_FUNC(unhook_all)
{
    API_INIT_FUNC(1, "unhook_all", "", API_RETURN_ERROR);

    v8::String::Utf8Value hook(args[0]);

    weechat_unhook_all (js_current_script->name);

    API_RETURN_OK;
}

int
weechat_js_api_buffer_input_data_cb (const void *pointer, void *data,
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

        rc = (int *)weechat_js_exec (script,
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
weechat_js_api_buffer_close_cb (const void *pointer, void *data,
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

        rc = (int *)weechat_js_exec (script,
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
    const char *result;

    API_INIT_FUNC(1, "buffer_new", "sssss", API_RETURN_EMPTY);

    v8::String::Utf8Value name(args[0]);
    v8::String::Utf8Value function_input(args[1]);
    v8::String::Utf8Value data_input(args[2]);
    v8::String::Utf8Value function_close(args[3]);
    v8::String::Utf8Value data_close(args[4]);

    result = API_PTR2STR(
        plugin_script_api_buffer_new (
            weechat_js_plugin,
            js_current_script,
            *name,
            &weechat_js_api_buffer_input_data_cb,
            *function_input,
            *data_input,
            &weechat_js_api_buffer_close_cb,
            *function_close,
            *data_close));

    API_RETURN_STRING(result);
}

API_FUNC(buffer_new_props)
{
    struct t_hashtable *properties;
    const char *result;

    API_INIT_FUNC(1, "buffer_new_props", "shssss", API_RETURN_EMPTY);

    v8::String::Utf8Value name(args[0]);
    properties = weechat_js_object_to_hashtable (
        args[1]->ToObject(),
        WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING);
    v8::String::Utf8Value function_input(args[2]);
    v8::String::Utf8Value data_input(args[3]);
    v8::String::Utf8Value function_close(args[4]);
    v8::String::Utf8Value data_close(args[5]);

    result = API_PTR2STR(
        plugin_script_api_buffer_new_props (
            weechat_js_plugin,
            js_current_script,
            *name,
            properties,
            &weechat_js_api_buffer_input_data_cb,
            *function_input,
            *data_input,
            &weechat_js_api_buffer_close_cb,
            *function_close,
            *data_close));

    if (properties)
        weechat_hashtable_free (properties);

    API_RETURN_STRING(result);
}

API_FUNC(buffer_search)
{
    const char *result;

    API_INIT_FUNC(1, "buffer_search", "ss", API_RETURN_EMPTY);

    v8::String::Utf8Value plugin(args[0]);
    v8::String::Utf8Value name(args[1]);

    result = API_PTR2STR(weechat_buffer_search (*plugin, *name));

    API_RETURN_STRING(result);
}

API_FUNC(buffer_search_main)
{
    const char *result;

    API_INIT_FUNC(1, "buffer_search_main", "", API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_buffer_search_main ());

    API_RETURN_STRING(result);
}

API_FUNC(current_buffer)
{
    const char *result;

    API_INIT_FUNC(1, "current_buffer", "", API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_current_buffer ());

    API_RETURN_STRING(result);
}

API_FUNC(buffer_clear)
{
    API_INIT_FUNC(1, "buffer_clear", "s", API_RETURN_ERROR);

    v8::String::Utf8Value buffer(args[0]);

    weechat_buffer_clear ((struct t_gui_buffer *)API_STR2PTR(*buffer));

    API_RETURN_OK;
}

API_FUNC(buffer_close)
{
    API_INIT_FUNC(1, "buffer_close", "s", API_RETURN_ERROR);

    v8::String::Utf8Value buffer(args[0]);

    weechat_buffer_close ((struct t_gui_buffer *)API_STR2PTR(*buffer));

    API_RETURN_OK;
}

API_FUNC(buffer_merge)
{
    API_INIT_FUNC(1, "buffer_merge", "ss", API_RETURN_ERROR);

    v8::String::Utf8Value buffer(args[0]);
    v8::String::Utf8Value target_buffer(args[1]);

    weechat_buffer_merge ((struct t_gui_buffer *)API_STR2PTR(*buffer),
                          (struct t_gui_buffer *)API_STR2PTR(*target_buffer));

    API_RETURN_OK;
}

API_FUNC(buffer_unmerge)
{
    int number;

    API_INIT_FUNC(1, "buffer_merge", "si", API_RETURN_ERROR);

    v8::String::Utf8Value buffer(args[0]);
    number = args[1]->IntegerValue();

    weechat_buffer_unmerge ((struct t_gui_buffer *)API_STR2PTR(*buffer),
                            number);

    API_RETURN_OK;
}

API_FUNC(buffer_get_integer)
{
    int value;

    API_INIT_FUNC(1, "buffer_get_integer", "ss", API_RETURN_INT(-1));

    v8::String::Utf8Value buffer(args[0]);
    v8::String::Utf8Value property(args[1]);

    value = weechat_buffer_get_integer (
        (struct t_gui_buffer *)API_STR2PTR(*buffer),
        *property);

    API_RETURN_INT(value);
}

API_FUNC(buffer_get_string)
{
    const char *result;

    API_INIT_FUNC(1, "buffer_get_string", "ss", API_RETURN_EMPTY);

    v8::String::Utf8Value buffer(args[0]);
    v8::String::Utf8Value property(args[1]);

    result = weechat_buffer_get_string (
        (struct t_gui_buffer *)API_STR2PTR(*buffer),
        *property);

    API_RETURN_STRING(result);
}

API_FUNC(buffer_get_pointer)
{
    const char *result;

    API_INIT_FUNC(1, "buffer_get_pointer", "ss", API_RETURN_EMPTY);

    v8::String::Utf8Value buffer(args[0]);
    v8::String::Utf8Value property(args[1]);

    result = API_PTR2STR(
        weechat_buffer_get_pointer (
            (struct t_gui_buffer *)API_STR2PTR(*buffer),
            *property));

    API_RETURN_STRING(result);
}

API_FUNC(buffer_set)
{
    API_INIT_FUNC(1, "buffer_set", "sss", API_RETURN_ERROR);

    v8::String::Utf8Value buffer(args[0]);
    v8::String::Utf8Value property(args[1]);
    v8::String::Utf8Value value(args[2]);

    weechat_buffer_set (
            (struct t_gui_buffer *)API_STR2PTR(*buffer),
            *property,
            *value);

    API_RETURN_OK;
}

API_FUNC(buffer_string_replace_local_var)
{
    char *result;

    API_INIT_FUNC(1, "buffer_string_replace_local_var", "ss", API_RETURN_EMPTY);

    v8::String::Utf8Value buffer(args[0]);
    v8::String::Utf8Value string(args[1]);

    result = weechat_buffer_string_replace_local_var (
        (struct t_gui_buffer *)API_STR2PTR(*buffer),
        *string);

    API_RETURN_STRING_FREE(result);
}

API_FUNC(buffer_match_list)
{
    int value;

    API_INIT_FUNC(1, "buffer_match_list", "ss", API_RETURN_INT(0));

    v8::String::Utf8Value buffer(args[0]);
    v8::String::Utf8Value string(args[1]);

    value = weechat_buffer_match_list (
        (struct t_gui_buffer *)API_STR2PTR(*buffer),
        *string);

    API_RETURN_INT(value);
}

API_FUNC(current_window)
{
    const char *result;

    API_INIT_FUNC(1, "current_window", "", API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_current_window ());

    API_RETURN_STRING(result);
}

API_FUNC(window_search_with_buffer)
{
    const char *result;

    API_INIT_FUNC(1, "window_search_with_buffer", "s", API_RETURN_EMPTY);

    v8::String::Utf8Value buffer(args[0]);

    result = API_PTR2STR(
        weechat_window_search_with_buffer (
            (struct t_gui_buffer *)API_STR2PTR(*buffer)));

    API_RETURN_STRING(result);
}

API_FUNC(window_get_integer)
{
    int value;

    API_INIT_FUNC(1, "window_get_integer", "ss", API_RETURN_INT(-1));

    v8::String::Utf8Value window(args[0]);
    v8::String::Utf8Value property(args[1]);

    value = weechat_window_get_integer (
        (struct t_gui_window *)API_STR2PTR(*window),
        *property);

    API_RETURN_INT(value);
}

API_FUNC(window_get_string)
{
    const char *result;

    API_INIT_FUNC(1, "window_get_string", "ss", API_RETURN_EMPTY);

    v8::String::Utf8Value window(args[0]);
    v8::String::Utf8Value property(args[1]);

    result = weechat_window_get_string (
        (struct t_gui_window *)API_STR2PTR(*window),
        *property);

    API_RETURN_STRING(result);
}

API_FUNC(window_get_pointer)
{
    const char *result;

    API_INIT_FUNC(1, "window_get_pointer", "ss", API_RETURN_EMPTY);

    v8::String::Utf8Value window(args[0]);
    v8::String::Utf8Value property(args[1]);

    result = API_PTR2STR(
        weechat_window_get_pointer (
            (struct t_gui_window *)API_STR2PTR(*window),
            *property));

    API_RETURN_STRING(result);
}

API_FUNC(window_set_title)
{
    API_INIT_FUNC(1, "window_set_title", "s", API_RETURN_ERROR);

    v8::String::Utf8Value title(args[0]);

    weechat_window_set_title (*title);

    API_RETURN_OK;
}

API_FUNC(nicklist_add_group)
{
    int visible;
    const char *result;

    API_INIT_FUNC(1, "nicklist_add_group", "ssssi", API_RETURN_EMPTY);

    v8::String::Utf8Value buffer(args[0]);
    v8::String::Utf8Value parent_group(args[1]);
    v8::String::Utf8Value name(args[2]);
    v8::String::Utf8Value color(args[3]);
    visible = args[4]->IntegerValue();

    result = API_PTR2STR(
        weechat_nicklist_add_group (
            (struct t_gui_buffer *)API_STR2PTR(*buffer),
            (struct t_gui_nick_group *)API_STR2PTR(*parent_group),
            *name,
            *color,
            visible));

    API_RETURN_STRING(result);
}

API_FUNC(nicklist_search_group)
{
    const char *result;

    API_INIT_FUNC(1, "nicklist_search_group", "sss", API_RETURN_EMPTY);

    v8::String::Utf8Value buffer(args[0]);
    v8::String::Utf8Value from_group(args[1]);
    v8::String::Utf8Value name(args[2]);

    result = API_PTR2STR(
        weechat_nicklist_search_group (
            (struct t_gui_buffer *)API_STR2PTR(*buffer),
            (struct t_gui_nick_group *)API_STR2PTR(*from_group),
            *name));

    API_RETURN_STRING(result);
}

API_FUNC(nicklist_add_nick)
{
    int visible;
    const char *result;

    API_INIT_FUNC(1, "nicklist_add_nick", "ssssssi", API_RETURN_EMPTY);

    v8::String::Utf8Value buffer(args[0]);
    v8::String::Utf8Value group(args[1]);
    v8::String::Utf8Value name(args[2]);
    v8::String::Utf8Value color(args[3]);
    v8::String::Utf8Value prefix(args[4]);
    v8::String::Utf8Value prefix_color(args[5]);
    visible = args[6]->IntegerValue();

    result = API_PTR2STR(
        weechat_nicklist_add_nick (
            (struct t_gui_buffer *)API_STR2PTR(*buffer),
            (struct t_gui_nick_group *)API_STR2PTR(*group),
            *name,
            *color,
            *prefix,
            *prefix_color,
            visible));

    API_RETURN_STRING(result);
}

API_FUNC(nicklist_search_nick)
{
    const char *result;

    API_INIT_FUNC(1, "nicklist_search_nick", "sss", API_RETURN_EMPTY);

    v8::String::Utf8Value buffer(args[0]);
    v8::String::Utf8Value from_group(args[1]);
    v8::String::Utf8Value name(args[2]);

    result = API_PTR2STR(
        weechat_nicklist_search_nick (
            (struct t_gui_buffer *)API_STR2PTR(*buffer),
            (struct t_gui_nick_group *)API_STR2PTR(*from_group),
            *name));

    API_RETURN_STRING(result);
}

API_FUNC(nicklist_remove_group)
{
    API_INIT_FUNC(1, "nicklist_remove_group", "ss", API_RETURN_ERROR);

    v8::String::Utf8Value buffer(args[0]);
    v8::String::Utf8Value group(args[1]);

    weechat_nicklist_remove_group (
            (struct t_gui_buffer *)API_STR2PTR(*buffer),
            (struct t_gui_nick_group *)API_STR2PTR(*group));

    API_RETURN_OK;
}

API_FUNC(nicklist_remove_nick)
{
    API_INIT_FUNC(1, "nicklist_remove_nick", "ss", API_RETURN_ERROR);

    v8::String::Utf8Value buffer(args[0]);
    v8::String::Utf8Value nick(args[1]);

    weechat_nicklist_remove_nick (
            (struct t_gui_buffer *)API_STR2PTR(*buffer),
            (struct t_gui_nick *)API_STR2PTR(*nick));

    API_RETURN_OK;
}

API_FUNC(nicklist_remove_all)
{
    API_INIT_FUNC(1, "nicklist_remove_all", "s", API_RETURN_ERROR);

    v8::String::Utf8Value buffer(args[0]);

    weechat_nicklist_remove_all ((struct t_gui_buffer *)API_STR2PTR(*buffer));

    API_RETURN_OK;
}

API_FUNC(nicklist_group_get_integer)
{
    int value;

    API_INIT_FUNC(1, "nicklist_group_get_integer", "sss", API_RETURN_INT(-1));

    v8::String::Utf8Value buffer(args[0]);
    v8::String::Utf8Value group(args[1]);
    v8::String::Utf8Value property(args[2]);

    value = weechat_nicklist_group_get_integer (
        (struct t_gui_buffer *)API_STR2PTR(*buffer),
        (struct t_gui_nick_group *)API_STR2PTR(*group),
        *property);

    API_RETURN_INT(value);
}

API_FUNC(nicklist_group_get_string)
{
    const char *result;

    API_INIT_FUNC(1, "nicklist_group_get_string", "sss", API_RETURN_EMPTY);

    v8::String::Utf8Value buffer(args[0]);
    v8::String::Utf8Value group(args[1]);
    v8::String::Utf8Value property(args[2]);

    result = weechat_nicklist_group_get_string (
        (struct t_gui_buffer *)API_STR2PTR(*buffer),
        (struct t_gui_nick_group *)API_STR2PTR(*group),
        *property);

    API_RETURN_STRING(result);
}

API_FUNC(nicklist_group_get_pointer)
{
    const char *result;

    API_INIT_FUNC(1, "nicklist_group_get_pointer", "sss", API_RETURN_EMPTY);

    v8::String::Utf8Value buffer(args[0]);
    v8::String::Utf8Value group(args[1]);
    v8::String::Utf8Value property(args[2]);

    result = API_PTR2STR(
        weechat_nicklist_group_get_pointer (
            (struct t_gui_buffer *)API_STR2PTR(*buffer),
            (struct t_gui_nick_group *)API_STR2PTR(*group),
            *property));

    API_RETURN_STRING(result);
}

API_FUNC(nicklist_group_set)
{
    API_INIT_FUNC(1, "nicklist_group_set", "ssss", API_RETURN_ERROR);

    v8::String::Utf8Value buffer(args[0]);
    v8::String::Utf8Value group(args[1]);
    v8::String::Utf8Value property(args[2]);
    v8::String::Utf8Value value(args[3]);

    weechat_nicklist_group_set (
            (struct t_gui_buffer *)API_STR2PTR(*buffer),
            (struct t_gui_nick_group *)API_STR2PTR(*group),
            *property,
            *value);

    API_RETURN_OK;
}

API_FUNC(nicklist_nick_get_integer)
{
    int value;

    API_INIT_FUNC(1, "nicklist_nick_get_integer", "sss", API_RETURN_INT(-1));

    v8::String::Utf8Value buffer(args[0]);
    v8::String::Utf8Value nick(args[1]);
    v8::String::Utf8Value property(args[2]);

    value = weechat_nicklist_nick_get_integer (
        (struct t_gui_buffer *)API_STR2PTR(*buffer),
        (struct t_gui_nick *)API_STR2PTR(*nick),
        *property);

    API_RETURN_INT(value);
}

API_FUNC(nicklist_nick_get_string)
{
    const char *result;

    API_INIT_FUNC(1, "nicklist_nick_get_string", "sss", API_RETURN_EMPTY);

    v8::String::Utf8Value buffer(args[0]);
    v8::String::Utf8Value nick(args[1]);
    v8::String::Utf8Value property(args[2]);

    result = weechat_nicklist_nick_get_string (
        (struct t_gui_buffer *)API_STR2PTR(*buffer),
        (struct t_gui_nick *)API_STR2PTR(*nick),
        *property);

    API_RETURN_STRING(result);
}

API_FUNC(nicklist_nick_get_pointer)
{
    const char *result;

    API_INIT_FUNC(1, "nicklist_nick_get_pointer", "sss", API_RETURN_EMPTY);

    v8::String::Utf8Value buffer(args[0]);
    v8::String::Utf8Value nick(args[1]);
    v8::String::Utf8Value property(args[2]);

    result = API_PTR2STR(
        weechat_nicklist_nick_get_pointer (
            (struct t_gui_buffer *)API_STR2PTR(*buffer),
            (struct t_gui_nick *)API_STR2PTR(*nick),
            *property));

    API_RETURN_STRING(result);
}

API_FUNC(nicklist_nick_set)
{
    API_INIT_FUNC(1, "nicklist_nick_set", "ssss", API_RETURN_ERROR);

    v8::String::Utf8Value buffer(args[0]);
    v8::String::Utf8Value nick(args[1]);
    v8::String::Utf8Value property(args[2]);
    v8::String::Utf8Value value(args[3]);

    weechat_nicklist_nick_set (
            (struct t_gui_buffer *)API_STR2PTR(*buffer),
            (struct t_gui_nick *)API_STR2PTR(*nick),
            *property,
            *value);

    API_RETURN_OK;
}

API_FUNC(bar_item_search)
{
    const char *result;

    API_INIT_FUNC(1, "bar_item_search", "s", API_RETURN_EMPTY);

    v8::String::Utf8Value name(args[0]);

    result = API_PTR2STR(weechat_bar_item_search (*name));

    API_RETURN_STRING(result);
}

char *
weechat_js_api_bar_item_build_cb (const void *pointer, void *data,
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

            ret = (char *)weechat_js_exec (script,
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

            ret = (char *)weechat_js_exec (script,
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
    const char *result;

    API_INIT_FUNC(1, "bar_item_new", "sss", API_RETURN_EMPTY);

    v8::String::Utf8Value name(args[0]);
    v8::String::Utf8Value function(args[1]);
    v8::String::Utf8Value data(args[2]);

    result = API_PTR2STR(
        plugin_script_api_bar_item_new (
            weechat_js_plugin,
            js_current_script,
            *name,
            &weechat_js_api_bar_item_build_cb,
            *function,
            *data));

    API_RETURN_STRING(result);
}

API_FUNC(bar_item_update)
{
    API_INIT_FUNC(1, "bar_item_update", "s", API_RETURN_ERROR);

    v8::String::Utf8Value name(args[0]);

    weechat_bar_item_update (*name);

    API_RETURN_OK;
}

API_FUNC(bar_item_remove)
{
    API_INIT_FUNC(1, "bar_item_remove", "s", API_RETURN_ERROR);

    v8::String::Utf8Value item(args[0]);

    weechat_bar_item_remove ((struct t_gui_bar_item *)API_STR2PTR(*item));

    API_RETURN_OK;
}

API_FUNC(bar_search)
{
    const char *result;

    API_INIT_FUNC(1, "bar_search", "s", API_RETURN_EMPTY);

    v8::String::Utf8Value name(args[0]);

    result = API_PTR2STR(weechat_bar_search (*name));

    API_RETURN_STRING(result);
}

API_FUNC(bar_new)
{
    const char *result;

    API_INIT_FUNC(1, "bar_new", "ssssssssssssssss", API_RETURN_EMPTY);

    v8::String::Utf8Value name(args[0]);
    v8::String::Utf8Value hidden(args[1]);
    v8::String::Utf8Value priority(args[2]);
    v8::String::Utf8Value type(args[3]);
    v8::String::Utf8Value conditions(args[4]);
    v8::String::Utf8Value position(args[5]);
    v8::String::Utf8Value filling_top_bottom(args[6]);
    v8::String::Utf8Value filling_left_right(args[7]);
    v8::String::Utf8Value size(args[8]);
    v8::String::Utf8Value size_max(args[9]);
    v8::String::Utf8Value color_fg(args[10]);
    v8::String::Utf8Value color_delim(args[11]);
    v8::String::Utf8Value color_bg(args[12]);
    v8::String::Utf8Value color_bg_inactive(args[13]);
    v8::String::Utf8Value separator(args[14]);
    v8::String::Utf8Value items(args[15]);

    result = API_PTR2STR(weechat_bar_new (*name,
                                          *hidden,
                                          *priority,
                                          *type,
                                          *conditions,
                                          *position,
                                          *filling_top_bottom,
                                          *filling_left_right,
                                          *size,
                                          *size_max,
                                          *color_fg,
                                          *color_delim,
                                          *color_bg,
                                          *color_bg_inactive,
                                          *separator,
                                          *items));

    API_RETURN_STRING(result);
}

API_FUNC(bar_set)
{
    int rc;

    API_INIT_FUNC(1, "bar_set", "sss", API_RETURN_INT(0));

    v8::String::Utf8Value bar(args[0]);
    v8::String::Utf8Value property(args[1]);
    v8::String::Utf8Value value(args[2]);

    rc = weechat_bar_set (
            (struct t_gui_bar *)API_STR2PTR(*bar),
            *property,
            *value);

    API_RETURN_INT(rc);
}

API_FUNC(bar_update)
{
    API_INIT_FUNC(1, "bar_update", "s", API_RETURN_ERROR);

    v8::String::Utf8Value name(args[0]);

    weechat_bar_update (*name);

    API_RETURN_OK;
}

API_FUNC(bar_remove)
{
    API_INIT_FUNC(1, "bar_remove", "s", API_RETURN_ERROR);

    v8::String::Utf8Value bar(args[0]);

    weechat_bar_remove ((struct t_gui_bar *)API_STR2PTR(*bar));

    API_RETURN_OK;
}

API_FUNC(command)
{
    int rc;

    API_INIT_FUNC(1, "command", "ss", API_RETURN_INT(WEECHAT_RC_ERROR));

    v8::String::Utf8Value buffer(args[0]);
    v8::String::Utf8Value command(args[1]);

    rc = plugin_script_api_command (weechat_js_plugin,
                                    js_current_script,
                                    (struct t_gui_buffer *)API_STR2PTR(*buffer),
                                    *command);

    API_RETURN_INT(rc);
}

API_FUNC(command_options)
{
    struct t_hashtable *options;
    int rc;

    API_INIT_FUNC(1, "command_options", "ssh", API_RETURN_INT(WEECHAT_RC_ERROR));

    v8::String::Utf8Value buffer(args[0]);
    v8::String::Utf8Value command(args[1]);
    options = weechat_js_object_to_hashtable (
        args[2]->ToObject(),
        WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING);

    rc = plugin_script_api_command_options (weechat_js_plugin,
                                            js_current_script,
                                            (struct t_gui_buffer *)API_STR2PTR(*buffer),
                                            *command,
                                            options);

    if (options)
        weechat_hashtable_free (options);

    API_RETURN_INT(rc);
}

API_FUNC(completion_new)
{
    const char *result;

    API_INIT_FUNC(1, "completion_new", "s", API_RETURN_EMPTY);

    v8::String::Utf8Value buffer(args[0]);

    result = API_PTR2STR(
        weechat_completion_new ((struct t_gui_buffer *)API_STR2PTR(*buffer)));

    API_RETURN_STRING(result);
}

API_FUNC(completion_search)
{
    int position, direction, rc;

    API_INIT_FUNC(1, "completion_search", "ssii", API_RETURN_INT(0));

    v8::String::Utf8Value completion(args[0]);
    v8::String::Utf8Value data(args[1]);
    position = args[2]->IntegerValue();
    direction = args[3]->IntegerValue();

    rc = weechat_completion_search (
        (struct t_gui_completion *)API_STR2PTR(*completion),
        (const char *)(*data),
        position,
        direction);

    API_RETURN_INT(rc);
}

API_FUNC(completion_get_string)
{
    const char *result;

    API_INIT_FUNC(1, "completion_get_string", "ss", API_RETURN_EMPTY);

    v8::String::Utf8Value completion(args[0]);
    v8::String::Utf8Value property(args[1]);

    result = weechat_completion_get_string (
        (struct t_gui_completion *)API_STR2PTR(*completion),
        *property);

    API_RETURN_STRING(result);
}

API_FUNC(completion_list_add)
{
    int nick_completion;

    API_INIT_FUNC(1, "completion_list_add", "ssis", API_RETURN_ERROR);

    v8::String::Utf8Value completion(args[0]);
    v8::String::Utf8Value word(args[1]);
    nick_completion = args[2]->IntegerValue();
    v8::String::Utf8Value where(args[3]);

    weechat_completion_list_add (
        (struct t_gui_completion *)API_STR2PTR(*completion),
        *word,
        nick_completion,
        *where);

    API_RETURN_OK;
}

API_FUNC(completion_free)
{
    API_INIT_FUNC(1, "completion_free", "s", API_RETURN_ERROR);

    v8::String::Utf8Value completion(args[0]);

    weechat_completion_free ((struct t_gui_completion *)API_STR2PTR(*completion));

    API_RETURN_OK;
}

API_FUNC(info_get)
{
    const char *result;

    API_INIT_FUNC(1, "info_get", "ss", API_RETURN_EMPTY);

    v8::String::Utf8Value info_name(args[0]);
    v8::String::Utf8Value arguments(args[1]);

    result = weechat_info_get (*info_name, *arguments);

    API_RETURN_STRING_FREE(result);
}

API_FUNC(info_get_hashtable)
{
    struct t_hashtable *hashtable, *result_hashtable;
    v8::Handle<v8::Object> result_obj;

    API_INIT_FUNC(1, "info_get_hashtable", "sh", API_RETURN_EMPTY);

    v8::String::Utf8Value info_name(args[0]);
    hashtable = weechat_js_object_to_hashtable (
        args[1]->ToObject(),
        WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING);

    result_hashtable = weechat_info_get_hashtable (*info_name, hashtable);
    result_obj = weechat_js_hashtable_to_object (
        result_hashtable);

    if (hashtable)
        weechat_hashtable_free (hashtable);
    if (result_hashtable)
        weechat_hashtable_free (result_hashtable);

    return result_obj;
}

API_FUNC(infolist_new)
{
    const char *result;

    API_INIT_FUNC(1, "infolist_new", "", API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_infolist_new ());

    API_RETURN_STRING(result);
}

API_FUNC(infolist_new_item)
{
    const char *result;

    API_INIT_FUNC(1, "infolist_new_item", "s", API_RETURN_EMPTY);

    v8::String::Utf8Value infolist(args[0]);

    result = API_PTR2STR(
        weechat_infolist_new_item (
            (struct t_infolist *)API_STR2PTR(*infolist)));

    API_RETURN_STRING(result);
}

API_FUNC(infolist_new_var_integer)
{
    int value;
    const char *result;

    API_INIT_FUNC(1, "infolist_new_var_integer", "ssi", API_RETURN_EMPTY);

    v8::String::Utf8Value item(args[0]);
    v8::String::Utf8Value name(args[1]);
    value = args[2]->IntegerValue();

    result = API_PTR2STR(
        weechat_infolist_new_var_integer (
            (struct t_infolist_item *)API_STR2PTR(*item),
            *name,
            value));

    API_RETURN_STRING(result);
}

API_FUNC(infolist_new_var_string)
{
    const char *result;

    API_INIT_FUNC(1, "infolist_new_var_string", "sss", API_RETURN_EMPTY);

    v8::String::Utf8Value item(args[0]);
    v8::String::Utf8Value name(args[1]);
    v8::String::Utf8Value value(args[2]);

    result = API_PTR2STR(
        weechat_infolist_new_var_string (
            (struct t_infolist_item *)API_STR2PTR(*item),
            *name,
            *value));

    API_RETURN_STRING(result);
}

API_FUNC(infolist_new_var_pointer)
{
    const char *result;

    API_INIT_FUNC(1, "infolist_new_var_pointer", "sss", API_RETURN_EMPTY);

    v8::String::Utf8Value item(args[0]);
    v8::String::Utf8Value name(args[1]);
    v8::String::Utf8Value value(args[2]);

    result = API_PTR2STR(
        weechat_infolist_new_var_pointer (
            (struct t_infolist_item *)API_STR2PTR(*item),
            *name,
            API_STR2PTR(*value)));

    API_RETURN_STRING(result);
}

API_FUNC(infolist_new_var_time)
{
    long value;
    const char *result;

    API_INIT_FUNC(1, "infolist_new_var_time", "ssn", API_RETURN_EMPTY);

    v8::String::Utf8Value item(args[0]);
    v8::String::Utf8Value name(args[1]);
    value = args[2]->IntegerValue();

    result = API_PTR2STR(
        weechat_infolist_new_var_time (
            (struct t_infolist_item *)API_STR2PTR(*item),
            *name,
            (time_t)value));

    API_RETURN_STRING(result);
}

API_FUNC(infolist_search_var)
{
    const char *result;

    API_INIT_FUNC(1, "infolist_search_var", "ss", API_RETURN_EMPTY);

    v8::String::Utf8Value infolist(args[0]);
    v8::String::Utf8Value name(args[1]);

    result = API_PTR2STR(
        weechat_infolist_search_var (
            (struct t_infolist *)API_STR2PTR(*infolist),
            *name));

    API_RETURN_STRING(result);
}

API_FUNC(infolist_get)
{
    const char *result;

    API_INIT_FUNC(1, "infolist_get", "sss", API_RETURN_EMPTY);

    v8::String::Utf8Value name(args[0]);
    v8::String::Utf8Value pointer(args[1]);
    v8::String::Utf8Value arguments(args[2]);

    result = API_PTR2STR(
        weechat_infolist_get (
            *name,
            API_STR2PTR(*pointer),
            *arguments));

    API_RETURN_STRING(result);
}

API_FUNC(infolist_next)
{
    int value;

    API_INIT_FUNC(1, "infolist_next", "s", API_RETURN_INT(0));

    v8::String::Utf8Value infolist(args[0]);

    value = weechat_infolist_next (
        (struct t_infolist *)API_STR2PTR(*infolist));

    API_RETURN_INT(value);
}

API_FUNC(infolist_prev)
{
    int value;

    API_INIT_FUNC(1, "infolist_prev", "s", API_RETURN_INT(0));

    v8::String::Utf8Value infolist(args[0]);

    value = weechat_infolist_prev (
        (struct t_infolist *)API_STR2PTR(*infolist));

    API_RETURN_INT(value);
}

API_FUNC(infolist_reset_item_cursor)
{
    API_INIT_FUNC(1, "infolist_reset_item_cursor", "s", API_RETURN_ERROR);

    v8::String::Utf8Value infolist(args[0]);

    weechat_infolist_reset_item_cursor (
        (struct t_infolist *)API_STR2PTR(*infolist));

    API_RETURN_OK;
}

API_FUNC(infolist_fields)
{
    const char *result;

    API_INIT_FUNC(1, "infolist_fields", "s", API_RETURN_EMPTY);

    v8::String::Utf8Value infolist(args[0]);

    result = weechat_infolist_fields (
        (struct t_infolist *)API_STR2PTR(*infolist));

    API_RETURN_STRING(result);
}

API_FUNC(infolist_integer)
{
    int value;

    API_INIT_FUNC(1, "infolist_integer", "ss", API_RETURN_INT(0));

    v8::String::Utf8Value infolist(args[0]);
    v8::String::Utf8Value variable(args[1]);

    value = weechat_infolist_integer (
        (struct t_infolist *)API_STR2PTR(*infolist),
        *variable);

    API_RETURN_INT(value);
}

API_FUNC(infolist_string)
{
    const char *result;

    API_INIT_FUNC(1, "infolist_string", "ss", API_RETURN_EMPTY);

    v8::String::Utf8Value infolist(args[0]);
    v8::String::Utf8Value variable(args[1]);

    result = weechat_infolist_string (
        (struct t_infolist *)API_STR2PTR(*infolist),
        *variable);

    API_RETURN_STRING(result);
}

API_FUNC(infolist_pointer)
{
    const char *result;

    API_INIT_FUNC(1, "infolist_pointer", "ss", API_RETURN_EMPTY);

    v8::String::Utf8Value infolist(args[0]);
    v8::String::Utf8Value variable(args[1]);

    result = API_PTR2STR(
        weechat_infolist_pointer (
            (struct t_infolist *)API_STR2PTR(*infolist),
            *variable));

    API_RETURN_STRING(result);
}

API_FUNC(infolist_time)
{
    time_t time;

    API_INIT_FUNC(1, "infolist_time", "ss", API_RETURN_LONG(0));

    v8::String::Utf8Value infolist(args[0]);
    v8::String::Utf8Value variable(args[1]);

    time = weechat_infolist_time (
        (struct t_infolist *)API_STR2PTR(*infolist),
        *variable);

    API_RETURN_LONG(time);
}

API_FUNC(infolist_free)
{
    API_INIT_FUNC(1, "infolist_free", "s", API_RETURN_ERROR);

    v8::String::Utf8Value infolist(args[0]);

    weechat_infolist_free ((struct t_infolist *)API_STR2PTR(*infolist));

    API_RETURN_OK;
}

API_FUNC(hdata_get)
{
    const char *result;

    API_INIT_FUNC(1, "hdata_get", "s", API_RETURN_EMPTY);

    v8::String::Utf8Value name(args[0]);

    result = API_PTR2STR(weechat_hdata_get (*name));

    API_RETURN_STRING(result);
}

API_FUNC(hdata_get_var_offset)
{
    int value;

    API_INIT_FUNC(1, "hdata_get_var_offset", "ss", API_RETURN_INT(0));

    v8::String::Utf8Value hdata(args[0]);
    v8::String::Utf8Value name(args[1]);

    value = weechat_hdata_get_var_offset (
        (struct t_hdata *)API_STR2PTR(*hdata),
        *name);

    API_RETURN_INT(value);
}

API_FUNC(hdata_get_var_type_string)
{
    const char *result;

    API_INIT_FUNC(1, "hdata_get_var_type_string", "ss", API_RETURN_EMPTY);

    v8::String::Utf8Value hdata(args[0]);
    v8::String::Utf8Value name(args[1]);

    result = weechat_hdata_get_var_type_string (
        (struct t_hdata *)API_STR2PTR(*hdata),
        *name);

    API_RETURN_STRING(result);
}

API_FUNC(hdata_get_var_array_size)
{
    int value;

    API_INIT_FUNC(1, "hdata_get_var_array_size", "sss", API_RETURN_INT(-1));

    v8::String::Utf8Value hdata(args[0]);
    v8::String::Utf8Value pointer(args[1]);
    v8::String::Utf8Value name(args[2]);

    value = weechat_hdata_get_var_array_size (
        (struct t_hdata *)API_STR2PTR(*hdata),
        API_STR2PTR(*pointer),
        *name);

    API_RETURN_INT(value);
}

API_FUNC(hdata_get_var_array_size_string)
{
    const char *result;

    API_INIT_FUNC(1, "hdata_get_var_array_size_string", "sss", API_RETURN_EMPTY);

    v8::String::Utf8Value hdata(args[0]);
    v8::String::Utf8Value pointer(args[1]);
    v8::String::Utf8Value name(args[2]);

    result = weechat_hdata_get_var_array_size_string (
        (struct t_hdata *)API_STR2PTR(*hdata),
        API_STR2PTR(*pointer),
        *name);

    API_RETURN_STRING(result);
}

API_FUNC(hdata_get_var_hdata)
{
    const char *result;

    API_INIT_FUNC(1, "hdata_get_var_hdata", "ss", API_RETURN_EMPTY);

    v8::String::Utf8Value hdata(args[0]);
    v8::String::Utf8Value name(args[1]);

    result = weechat_hdata_get_var_hdata (
        (struct t_hdata *)API_STR2PTR(*hdata),
        *name);

    API_RETURN_STRING(result);
}

API_FUNC(hdata_get_list)
{
    const char *result;

    API_INIT_FUNC(1, "hdata_get_list", "s", API_RETURN_EMPTY);

    v8::String::Utf8Value hdata(args[0]);
    v8::String::Utf8Value name(args[1]);

    result = API_PTR2STR(
        weechat_hdata_get_list (
            (struct t_hdata *)API_STR2PTR(*hdata),
            *name));

    API_RETURN_STRING(result);
}

API_FUNC(hdata_check_pointer)
{
    int value;

    API_INIT_FUNC(1, "hdata_check_pointer", "sss", API_RETURN_INT(0));

    v8::String::Utf8Value hdata(args[0]);
    v8::String::Utf8Value list(args[1]);
    v8::String::Utf8Value pointer(args[2]);

    value = weechat_hdata_check_pointer (
        (struct t_hdata *)API_STR2PTR(*hdata),
        API_STR2PTR(*list),
        API_STR2PTR(*pointer));

    API_RETURN_INT(value);
}

API_FUNC(hdata_move)
{
    int count;
    const char *result;

    API_INIT_FUNC(1, "hdata_move", "ssi", API_RETURN_EMPTY);

    v8::String::Utf8Value hdata(args[0]);
    v8::String::Utf8Value pointer(args[1]);
    count = args[2]->IntegerValue();

    result = API_PTR2STR(
        weechat_hdata_move (
            (struct t_hdata *)API_STR2PTR(*hdata),
            API_STR2PTR(*pointer),
            count));

    API_RETURN_STRING(result);
}

API_FUNC(hdata_search)
{
    struct t_hashtable *pointers, *extra_vars, *options;
    int move;
    const char *result;

    API_INIT_FUNC(1, "hdata_search", "ssshhhi", API_RETURN_EMPTY);

    v8::String::Utf8Value hdata(args[0]);
    v8::String::Utf8Value pointer(args[1]);
    v8::String::Utf8Value search(args[2]);
    pointers = weechat_js_object_to_hashtable (
        args[3]->ToObject(),
        WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_POINTER);
    extra_vars = weechat_js_object_to_hashtable (
        args[4]->ToObject(),
        WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING);
    options = weechat_js_object_to_hashtable (
        args[5]->ToObject(),
        WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING);
    move = args[6]->IntegerValue();

    result = API_PTR2STR(
        weechat_hdata_search (
            (struct t_hdata *)API_STR2PTR(*hdata),
            API_STR2PTR(*pointer),
            *search,
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
    int value;

    API_INIT_FUNC(1, "hdata_char", "sss", API_RETURN_INT(0));

    v8::String::Utf8Value hdata(args[0]);
    v8::String::Utf8Value pointer(args[1]);
    v8::String::Utf8Value name(args[2]);

    value = (int)weechat_hdata_char (
        (struct t_hdata *)API_STR2PTR(*hdata),
        API_STR2PTR(*pointer),
        *name);

    API_RETURN_INT(value);
}

API_FUNC(hdata_integer)
{
    int value;

    API_INIT_FUNC(1, "hdata_integer", "sss", API_RETURN_INT(0));

    v8::String::Utf8Value hdata(args[0]);
    v8::String::Utf8Value pointer(args[1]);
    v8::String::Utf8Value name(args[2]);

    value = weechat_hdata_integer (
        (struct t_hdata *)API_STR2PTR(*hdata),
        API_STR2PTR(*pointer),
        *name);

    API_RETURN_INT(value);
}

API_FUNC(hdata_long)
{
    long value;

    API_INIT_FUNC(1, "hdata_long", "sss", API_RETURN_LONG(0));

    v8::String::Utf8Value hdata(args[0]);
    v8::String::Utf8Value pointer(args[1]);
    v8::String::Utf8Value name(args[2]);

    value = weechat_hdata_long (
        (struct t_hdata *)API_STR2PTR(*hdata),
        API_STR2PTR(*pointer),
        *name);

    API_RETURN_LONG(value);
}

API_FUNC(hdata_string)
{
    const char *result;

    API_INIT_FUNC(1, "hdata_string", "sss", API_RETURN_EMPTY);

    v8::String::Utf8Value hdata(args[0]);
    v8::String::Utf8Value pointer(args[1]);
    v8::String::Utf8Value name(args[2]);

    result = weechat_hdata_string (
        (struct t_hdata *)API_STR2PTR(*hdata),
        API_STR2PTR(*pointer),
        *name);

    API_RETURN_STRING(result);
}

API_FUNC(hdata_pointer)
{
    const char *result;

    API_INIT_FUNC(1, "hdata_pointer", "sss", API_RETURN_EMPTY);

    v8::String::Utf8Value hdata(args[0]);
    v8::String::Utf8Value pointer(args[1]);
    v8::String::Utf8Value name(args[2]);

    result = API_PTR2STR(
        weechat_hdata_pointer (
            (struct t_hdata *)API_STR2PTR(*hdata),
            API_STR2PTR(*pointer),
            *name));

    API_RETURN_STRING(result);
}

API_FUNC(hdata_time)
{
    time_t time;

    API_INIT_FUNC(1, "hdata_time", "sss", API_RETURN_LONG(0));

    v8::String::Utf8Value hdata(args[0]);
    v8::String::Utf8Value pointer(args[1]);
    v8::String::Utf8Value name(args[2]);

    time = weechat_hdata_time (
        (struct t_hdata *)API_STR2PTR(*hdata),
        API_STR2PTR(*pointer),
        *name);

    API_RETURN_LONG(time);
}

API_FUNC(hdata_hashtable)
{
    v8::Handle<v8::Object> result_obj;

    API_INIT_FUNC(1, "hdata_hashtable", "sss", API_RETURN_EMPTY);

    v8::String::Utf8Value hdata(args[0]);
    v8::String::Utf8Value pointer(args[1]);
    v8::String::Utf8Value name(args[2]);

    result_obj = weechat_js_hashtable_to_object (
        weechat_hdata_hashtable (
            (struct t_hdata *)API_STR2PTR(*hdata),
            API_STR2PTR(*pointer),
            *name));

    return result_obj;
}

API_FUNC(hdata_compare)
{
    int case_sensitive, rc;

    API_INIT_FUNC(1, "hdata_compare", "ssssi", API_RETURN_INT(0));

    v8::String::Utf8Value hdata(args[0]);
    v8::String::Utf8Value pointer1(args[1]);
    v8::String::Utf8Value pointer2(args[2]);
    v8::String::Utf8Value name(args[3]);
    case_sensitive = args[4]->IntegerValue();

    rc = weechat_hdata_compare (
        (struct t_hdata *)API_STR2PTR(*hdata),
        API_STR2PTR(*pointer1),
        API_STR2PTR(*pointer2),
        *name,
        case_sensitive);

    API_RETURN_INT(rc);
}

API_FUNC(hdata_update)
{
    struct t_hashtable *hashtable;
    int value;

    API_INIT_FUNC(1, "hdata_update", "ssh", API_RETURN_INT(0));

    v8::String::Utf8Value hdata(args[0]);
    v8::String::Utf8Value pointer(args[1]);
    hashtable = weechat_js_object_to_hashtable (
        args[2]->ToObject(),
        WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING);

    value = weechat_hdata_update (
        (struct t_hdata *)API_STR2PTR(*hdata),
        API_STR2PTR(*pointer),
        hashtable);

    if (hashtable)
        weechat_hashtable_free (hashtable);

    API_RETURN_INT(value);
}

API_FUNC(hdata_get_string)
{
    const char *result;

    API_INIT_FUNC(1, "hdata_get_string", "ss", API_RETURN_EMPTY);

    v8::String::Utf8Value hdata(args[0]);
    v8::String::Utf8Value property(args[1]);

    result = weechat_hdata_get_string (
        (struct t_hdata *)API_STR2PTR(*hdata),
        *property);

    API_RETURN_STRING(result);
}

int
weechat_js_api_upgrade_read_cb (const void *pointer, void *data,
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

        rc = (int *)weechat_js_exec (script,
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
    const char *result;

    API_INIT_FUNC(1, "upgrade_new", "sss", API_RETURN_EMPTY);

    v8::String::Utf8Value filename(args[0]);
    v8::String::Utf8Value function(args[0]);
    v8::String::Utf8Value data(args[0]);

    result = API_PTR2STR(
        plugin_script_api_upgrade_new (
            weechat_js_plugin,
            js_current_script,
            *filename,
            &weechat_js_api_upgrade_read_cb,
            *function,
            *data));

    API_RETURN_STRING(result);
}

API_FUNC(upgrade_write_object)
{
    int object_id, rc;

    API_INIT_FUNC(1, "upgrade_write_object", "sis", API_RETURN_INT(0));

    v8::String::Utf8Value upgrade_file(args[0]);
    object_id = args[1]->IntegerValue();
    v8::String::Utf8Value infolist(args[2]);

    rc = weechat_upgrade_write_object (
        (struct t_upgrade_file *)API_STR2PTR(*upgrade_file),
        object_id,
        (struct t_infolist *)API_STR2PTR(*infolist));

    API_RETURN_INT(rc);
}

API_FUNC(upgrade_read)
{
    int rc;

    API_INIT_FUNC(1, "upgrade_read", "s", API_RETURN_INT(0));

    v8::String::Utf8Value upgrade_file(args[0]);

    rc = weechat_upgrade_read (
        (struct t_upgrade_file *)API_STR2PTR(*upgrade_file));

    API_RETURN_INT(rc);
}

API_FUNC(upgrade_close)
{
    API_INIT_FUNC(1, "upgrade_close", "sss", API_RETURN_ERROR);

    v8::String::Utf8Value upgrade_file(args[0]);

    weechat_upgrade_close (
        (struct t_upgrade_file *)API_STR2PTR(*upgrade_file));

    API_RETURN_OK;
}

void
WeechatJsV8::loadLibs()
{
    v8::Local<v8::ObjectTemplate> weechat_obj = v8::ObjectTemplate::New();

    /* constants */
    API_DEF_CONST_INT(WEECHAT_RC_OK);
    API_DEF_CONST_INT(WEECHAT_RC_OK_EAT);
    API_DEF_CONST_INT(WEECHAT_RC_ERROR);

    API_DEF_CONST_INT(WEECHAT_CONFIG_READ_OK);
    API_DEF_CONST_INT(WEECHAT_CONFIG_READ_MEMORY_ERROR);
    API_DEF_CONST_INT(WEECHAT_CONFIG_READ_FILE_NOT_FOUND);
    API_DEF_CONST_INT(WEECHAT_CONFIG_WRITE_OK);
    API_DEF_CONST_INT(WEECHAT_CONFIG_WRITE_ERROR);
    API_DEF_CONST_INT(WEECHAT_CONFIG_WRITE_MEMORY_ERROR);
    API_DEF_CONST_INT(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED);
    API_DEF_CONST_INT(WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE);
    API_DEF_CONST_INT(WEECHAT_CONFIG_OPTION_SET_ERROR);
    API_DEF_CONST_INT(WEECHAT_CONFIG_OPTION_SET_OPTION_NOT_FOUND);
    API_DEF_CONST_INT(WEECHAT_CONFIG_OPTION_UNSET_OK_NO_RESET);
    API_DEF_CONST_INT(WEECHAT_CONFIG_OPTION_UNSET_OK_RESET);
    API_DEF_CONST_INT(WEECHAT_CONFIG_OPTION_UNSET_OK_REMOVED);
    API_DEF_CONST_INT(WEECHAT_CONFIG_OPTION_UNSET_ERROR);

    API_DEF_CONST_STR(WEECHAT_LIST_POS_SORT);
    API_DEF_CONST_STR(WEECHAT_LIST_POS_BEGINNING);
    API_DEF_CONST_STR(WEECHAT_LIST_POS_END);

    API_DEF_CONST_STR(WEECHAT_HOTLIST_LOW);
    API_DEF_CONST_STR(WEECHAT_HOTLIST_MESSAGE);
    API_DEF_CONST_STR(WEECHAT_HOTLIST_PRIVATE);
    API_DEF_CONST_STR(WEECHAT_HOTLIST_HIGHLIGHT);

    API_DEF_CONST_INT(WEECHAT_HOOK_PROCESS_RUNNING);
    API_DEF_CONST_INT(WEECHAT_HOOK_PROCESS_ERROR);

    API_DEF_CONST_INT(WEECHAT_HOOK_CONNECT_OK);
    API_DEF_CONST_INT(WEECHAT_HOOK_CONNECT_ADDRESS_NOT_FOUND);
    API_DEF_CONST_INT(WEECHAT_HOOK_CONNECT_IP_ADDRESS_NOT_FOUND);
    API_DEF_CONST_INT(WEECHAT_HOOK_CONNECT_CONNECTION_REFUSED);
    API_DEF_CONST_INT(WEECHAT_HOOK_CONNECT_PROXY_ERROR);
    API_DEF_CONST_INT(WEECHAT_HOOK_CONNECT_LOCAL_HOSTNAME_ERROR);
    API_DEF_CONST_INT(WEECHAT_HOOK_CONNECT_GNUTLS_INIT_ERROR);
    API_DEF_CONST_INT(WEECHAT_HOOK_CONNECT_GNUTLS_HANDSHAKE_ERROR);
    API_DEF_CONST_INT(WEECHAT_HOOK_CONNECT_MEMORY_ERROR);
    API_DEF_CONST_INT(WEECHAT_HOOK_CONNECT_TIMEOUT);
    API_DEF_CONST_INT(WEECHAT_HOOK_CONNECT_SOCKET_ERROR);

    API_DEF_CONST_STR(WEECHAT_HOOK_SIGNAL_STRING);
    API_DEF_CONST_STR(WEECHAT_HOOK_SIGNAL_INT);
    API_DEF_CONST_STR(WEECHAT_HOOK_SIGNAL_POINTER);

    /* functions */
    API_DEF_FUNC(register);
    API_DEF_FUNC(plugin_get_name);
    API_DEF_FUNC(charset_set);
    API_DEF_FUNC(iconv_to_internal);
    API_DEF_FUNC(iconv_from_internal);
    API_DEF_FUNC(gettext);
    API_DEF_FUNC(ngettext);
    API_DEF_FUNC(strlen_screen);
    API_DEF_FUNC(string_match);
    API_DEF_FUNC(string_match_list);
    API_DEF_FUNC(string_has_highlight);
    API_DEF_FUNC(string_has_highlight_regex);
    API_DEF_FUNC(string_mask_to_regex);
    API_DEF_FUNC(string_format_size);
    API_DEF_FUNC(string_parse_size);
    API_DEF_FUNC(string_color_code_size);
    API_DEF_FUNC(string_remove_color);
    API_DEF_FUNC(string_is_command_char);
    API_DEF_FUNC(string_input_for_buffer);
    API_DEF_FUNC(string_eval_expression);
    API_DEF_FUNC(string_eval_path_home);
    API_DEF_FUNC(mkdir_home);
    API_DEF_FUNC(mkdir);
    API_DEF_FUNC(mkdir_parents);
    API_DEF_FUNC(list_new);
    API_DEF_FUNC(list_add);
    API_DEF_FUNC(list_search);
    API_DEF_FUNC(list_search_pos);
    API_DEF_FUNC(list_casesearch);
    API_DEF_FUNC(list_casesearch_pos);
    API_DEF_FUNC(list_get);
    API_DEF_FUNC(list_set)
    API_DEF_FUNC(list_next);
    API_DEF_FUNC(list_prev);
    API_DEF_FUNC(list_string);
    API_DEF_FUNC(list_size);
    API_DEF_FUNC(list_remove);
    API_DEF_FUNC(list_remove_all);
    API_DEF_FUNC(list_free);
    API_DEF_FUNC(config_new);
    API_DEF_FUNC(config_set_version);
    API_DEF_FUNC(config_new_section);
    API_DEF_FUNC(config_search_section);
    API_DEF_FUNC(config_new_option);
    API_DEF_FUNC(config_search_option);
    API_DEF_FUNC(config_string_to_boolean);
    API_DEF_FUNC(config_option_reset);
    API_DEF_FUNC(config_option_set);
    API_DEF_FUNC(config_option_set_null);
    API_DEF_FUNC(config_option_unset);
    API_DEF_FUNC(config_option_rename);
    API_DEF_FUNC(config_option_is_null);
    API_DEF_FUNC(config_option_default_is_null);
    API_DEF_FUNC(config_boolean);
    API_DEF_FUNC(config_boolean_default);
    API_DEF_FUNC(config_integer);
    API_DEF_FUNC(config_integer_default);
    API_DEF_FUNC(config_string);
    API_DEF_FUNC(config_string_default);
    API_DEF_FUNC(config_color);
    API_DEF_FUNC(config_color_default);
    API_DEF_FUNC(config_write_option);
    API_DEF_FUNC(config_write_line);
    API_DEF_FUNC(config_write);
    API_DEF_FUNC(config_read);
    API_DEF_FUNC(config_reload);
    API_DEF_FUNC(config_option_free);
    API_DEF_FUNC(config_section_free_options);
    API_DEF_FUNC(config_section_free);
    API_DEF_FUNC(config_free);
    API_DEF_FUNC(config_get);
    API_DEF_FUNC(config_get_plugin);
    API_DEF_FUNC(config_is_set_plugin);
    API_DEF_FUNC(config_set_plugin);
    API_DEF_FUNC(config_set_desc_plugin);
    API_DEF_FUNC(config_unset_plugin);
    API_DEF_FUNC(key_bind);
    API_DEF_FUNC(key_unbind);
    API_DEF_FUNC(prefix);
    API_DEF_FUNC(color);
    API_DEF_FUNC(print);
    API_DEF_FUNC(print_date_tags);
    API_DEF_FUNC(print_y);
    API_DEF_FUNC(print_y_date_tags);
    API_DEF_FUNC(log_print);
    API_DEF_FUNC(hook_command);
    API_DEF_FUNC(hook_completion);
    API_DEF_FUNC(hook_completion_get_string);
    API_DEF_FUNC(hook_completion_list_add);
    API_DEF_FUNC(hook_command_run);
    API_DEF_FUNC(hook_timer);
    API_DEF_FUNC(hook_fd);
    API_DEF_FUNC(hook_process);
    API_DEF_FUNC(hook_process_hashtable);
    API_DEF_FUNC(hook_connect);
    API_DEF_FUNC(hook_line);
    API_DEF_FUNC(hook_print);
    API_DEF_FUNC(hook_signal);
    API_DEF_FUNC(hook_signal_send);
    API_DEF_FUNC(hook_hsignal);
    API_DEF_FUNC(hook_hsignal_send);
    API_DEF_FUNC(hook_config);
    API_DEF_FUNC(hook_modifier);
    API_DEF_FUNC(hook_modifier_exec);
    API_DEF_FUNC(hook_info);
    API_DEF_FUNC(hook_info_hashtable);
    API_DEF_FUNC(hook_infolist);
    API_DEF_FUNC(hook_focus);
    API_DEF_FUNC(hook_set);
    API_DEF_FUNC(unhook);
    API_DEF_FUNC(unhook_all);
    API_DEF_FUNC(buffer_new);
    API_DEF_FUNC(buffer_new_props);
    API_DEF_FUNC(buffer_search);
    API_DEF_FUNC(buffer_search_main);
    API_DEF_FUNC(current_buffer);
    API_DEF_FUNC(buffer_clear);
    API_DEF_FUNC(buffer_close);
    API_DEF_FUNC(buffer_merge);
    API_DEF_FUNC(buffer_unmerge);
    API_DEF_FUNC(buffer_get_integer);
    API_DEF_FUNC(buffer_get_string);
    API_DEF_FUNC(buffer_get_pointer);
    API_DEF_FUNC(buffer_set);
    API_DEF_FUNC(buffer_string_replace_local_var);
    API_DEF_FUNC(buffer_match_list);
    API_DEF_FUNC(current_window);
    API_DEF_FUNC(window_search_with_buffer);
    API_DEF_FUNC(window_get_integer);
    API_DEF_FUNC(window_get_string);
    API_DEF_FUNC(window_get_pointer);
    API_DEF_FUNC(window_set_title);
    API_DEF_FUNC(nicklist_add_group);
    API_DEF_FUNC(nicklist_search_group);
    API_DEF_FUNC(nicklist_add_nick);
    API_DEF_FUNC(nicklist_search_nick);
    API_DEF_FUNC(nicklist_remove_group);
    API_DEF_FUNC(nicklist_remove_nick);
    API_DEF_FUNC(nicklist_remove_all);
    API_DEF_FUNC(nicklist_group_get_integer);
    API_DEF_FUNC(nicklist_group_get_string);
    API_DEF_FUNC(nicklist_group_get_pointer);
    API_DEF_FUNC(nicklist_group_set);
    API_DEF_FUNC(nicklist_nick_get_integer);
    API_DEF_FUNC(nicklist_nick_get_string);
    API_DEF_FUNC(nicklist_nick_get_pointer);
    API_DEF_FUNC(nicklist_nick_set);
    API_DEF_FUNC(bar_item_search);
    API_DEF_FUNC(bar_item_new);
    API_DEF_FUNC(bar_item_update);
    API_DEF_FUNC(bar_item_remove);
    API_DEF_FUNC(bar_search);
    API_DEF_FUNC(bar_new);
    API_DEF_FUNC(bar_set);
    API_DEF_FUNC(bar_update);
    API_DEF_FUNC(bar_remove);
    API_DEF_FUNC(command);
    API_DEF_FUNC(command_options);
    API_DEF_FUNC(completion_new);
    API_DEF_FUNC(completion_search);
    API_DEF_FUNC(completion_get_string);
    API_DEF_FUNC(completion_list_add);
    API_DEF_FUNC(completion_free);
    API_DEF_FUNC(info_get);
    API_DEF_FUNC(info_get_hashtable);
    API_DEF_FUNC(infolist_new);
    API_DEF_FUNC(infolist_new_item);
    API_DEF_FUNC(infolist_new_var_integer);
    API_DEF_FUNC(infolist_new_var_string);
    API_DEF_FUNC(infolist_new_var_pointer);
    API_DEF_FUNC(infolist_new_var_time);
    API_DEF_FUNC(infolist_search_var);
    API_DEF_FUNC(infolist_get);
    API_DEF_FUNC(infolist_next);
    API_DEF_FUNC(infolist_prev);
    API_DEF_FUNC(infolist_reset_item_cursor);
    API_DEF_FUNC(infolist_fields);
    API_DEF_FUNC(infolist_integer);
    API_DEF_FUNC(infolist_string);
    API_DEF_FUNC(infolist_pointer);
    API_DEF_FUNC(infolist_time);
    API_DEF_FUNC(infolist_free);
    API_DEF_FUNC(hdata_get);
    API_DEF_FUNC(hdata_get_var_offset);
    API_DEF_FUNC(hdata_get_var_type_string);
    API_DEF_FUNC(hdata_get_var_array_size);
    API_DEF_FUNC(hdata_get_var_array_size_string);
    API_DEF_FUNC(hdata_get_var_hdata);
    API_DEF_FUNC(hdata_get_list);
    API_DEF_FUNC(hdata_check_pointer);
    API_DEF_FUNC(hdata_move);
    API_DEF_FUNC(hdata_search);
    API_DEF_FUNC(hdata_char);
    API_DEF_FUNC(hdata_integer);
    API_DEF_FUNC(hdata_long);
    API_DEF_FUNC(hdata_string);
    API_DEF_FUNC(hdata_pointer);
    API_DEF_FUNC(hdata_time);
    API_DEF_FUNC(hdata_hashtable);
    API_DEF_FUNC(hdata_compare);
    API_DEF_FUNC(hdata_update);
    API_DEF_FUNC(hdata_get_string);
    API_DEF_FUNC(upgrade_new);
    API_DEF_FUNC(upgrade_write_object);
    API_DEF_FUNC(upgrade_read);
    API_DEF_FUNC(upgrade_close);

    this->addGlobal ("weechat", weechat_obj);
}
