/*
 * weechat-lua-api.c - lua API functions
 *
 * Copyright (C) 2006-2007 Emmanuel Bouthenot <kolter@openics.org>
 * Copyright (C) 2006-2025 Sébastien Helleu <flashcode@flashtux.org>
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

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../weechat-plugin.h"
#include "../plugin-script.h"
#include "../plugin-script-api.h"
#include "weechat-lua.h"


#define API_DEF_FUNC(__name)                                            \
    { #__name, &weechat_lua_api_##__name }
#define API_FUNC(__name)                                                \
    static int                                                          \
    weechat_lua_api_##__name (lua_State *L)
#define API_INIT_FUNC(__init, __name, __ret)                            \
    char *lua_function_name = __name;                                   \
    (void) L;                                                           \
    if (__init                                                          \
        && (!lua_current_script || !lua_current_script->name))          \
    {                                                                   \
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME,            \
                                    lua_function_name);                 \
        __ret;                                                          \
    }
#define API_WRONG_ARGS(__ret)                                           \
    {                                                                   \
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME,          \
                                      lua_function_name);               \
        __ret;                                                          \
    }
#define API_PTR2STR(__pointer)                                          \
    plugin_script_ptr2str (__pointer)
#define API_STR2PTR(__string)                                           \
    plugin_script_str2ptr (weechat_lua_plugin,                          \
                           LUA_CURRENT_SCRIPT_NAME,                     \
                           lua_function_name, __string)
#define API_RETURN_OK                                                   \
    {                                                                   \
        lua_pushinteger (L, 1);                                         \
        return 1;                                                       \
    }
#define API_RETURN_ERROR                                                \
    {                                                                   \
        lua_pushinteger (L, 0);                                         \
        return 1;                                                       \
    }
#define API_RETURN_EMPTY                                                \
    {                                                                   \
        lua_pushstring (L, "");                                         \
        return 0;                                                       \
    }
#define API_RETURN_STRING(__string)                                     \
    {                                                                   \
        lua_pushstring (L,                                              \
                        (__string) ? __string : "");                    \
        return 1;                                                       \
    }
#define API_RETURN_STRING_FREE(__string)                                \
    {                                                                   \
        lua_pushstring (L,                                              \
                        (__string) ? __string : "");                    \
        if (__string)                                                   \
            free (__string);                                            \
        return 1;                                                       \
    }
#if LUA_VERSION_NUM >= 503
#define API_RETURN_INT(__int)                                           \
    {                                                                   \
        lua_pushinteger (L, __int);                                     \
        return 1;                                                       \
    }
#define API_RETURN_LONG(__long)                                         \
    {                                                                   \
        lua_pushinteger (L, __long);                                    \
        return 1;                                                       \
    }
#else
#define API_RETURN_INT(__int)                                           \
    {                                                                   \
        lua_pushnumber (L, __int);                                      \
        return 1;                                                       \
    }
#define API_RETURN_LONG(__long)                                         \
    {                                                                   \
        lua_pushnumber (L, __long);                                     \
        return 1;                                                       \
    }
#endif /* LUA_VERSION_NUM >= 503 */


/*
 * Registers a lua script.
 */

API_FUNC(register)
{
    const char *name, *author, *version, *license, *description;
    const char *shutdown_func, *charset;

    API_INIT_FUNC(0, "register", API_RETURN_ERROR);
    if (lua_registered_script)
    {
        /* script already registered */
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: script \"%s\" already "
                                         "registered (register ignored)"),
                        weechat_prefix ("error"), LUA_PLUGIN_NAME,
                        lua_registered_script->name);
        API_RETURN_ERROR;
    }
    lua_current_script = NULL;
    lua_registered_script = NULL;
    if (lua_gettop (L) < 7)
        API_WRONG_ARGS(API_RETURN_ERROR);

    name = lua_tostring (L, -7);
    author = lua_tostring (L, -6);
    version = lua_tostring (L, -5);
    license = lua_tostring (L, -4);
    description = lua_tostring (L, -3);
    shutdown_func = lua_tostring (L, -2);
    charset = lua_tostring (L, -1);

    if (plugin_script_search (lua_scripts, name))
    {
        /* another script already exists with same name */
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to register script "
                                         "\"%s\" (another script already "
                                         "exists with this name)"),
                        weechat_prefix ("error"), LUA_PLUGIN_NAME, name);
        API_RETURN_ERROR;
    }

    /* register script */
    lua_current_script = plugin_script_add (weechat_lua_plugin,
                                            &lua_data,
                                            (lua_current_script_filename) ?
                                            lua_current_script_filename : "",
                                            name,
                                            author,
                                            version,
                                            license,
                                            description,
                                            shutdown_func,
                                            charset);
    if (lua_current_script)
    {
        lua_registered_script = lua_current_script;
        if ((weechat_lua_plugin->debug >= 2) || !lua_quiet)
        {
            weechat_printf (NULL,
                            weechat_gettext ("%s: registered script \"%s\", "
                                             "version %s (%s)"),
                            LUA_PLUGIN_NAME, name, version, description);
        }
        lua_current_script->interpreter = (lua_State *) lua_current_interpreter;
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
    const char *plugin, *result;

    API_INIT_FUNC(1, "plugin_get_name", API_RETURN_EMPTY);
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    plugin = lua_tostring (L, -1);

    result = weechat_plugin_get_name (API_STR2PTR(plugin));

    API_RETURN_STRING(result);
}

API_FUNC(charset_set)
{
    const char *charset;

    API_INIT_FUNC(1, "charset_set", API_RETURN_ERROR);
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_ERROR);

    charset = lua_tostring (L, -1);

    plugin_script_api_charset_set (lua_current_script,
                                   charset);

    API_RETURN_OK;
}

API_FUNC(iconv_to_internal)
{
    const char *charset, *string;
    char *result;

    API_INIT_FUNC(1, "iconv_to_internal", API_RETURN_EMPTY);
    if (lua_gettop (L) < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    charset = lua_tostring (L, -2);
    string = lua_tostring (L, -1);

    result = weechat_iconv_to_internal (charset, string);

    API_RETURN_STRING_FREE(result);
}

API_FUNC(iconv_from_internal)
{
    const char *charset, *string;
    char *result;

    API_INIT_FUNC(1, "iconv_from_internal", API_RETURN_EMPTY);
    if (lua_gettop (L) < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    charset = lua_tostring (L, -2);
    string = lua_tostring (L, -1);

    result = weechat_iconv_from_internal (charset, string);

    API_RETURN_STRING_FREE(result);
}

API_FUNC(gettext)
{
    const char *string, *result;

    API_INIT_FUNC(1, "gettext", API_RETURN_EMPTY);
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    string = lua_tostring (L, -1);

    result = weechat_gettext (string);

    API_RETURN_STRING(result);
}

API_FUNC(ngettext)
{
    const char *single, *plural, *result;
    int count;

    API_INIT_FUNC(1, "ngettext", API_RETURN_EMPTY);
    if (lua_gettop (L) < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    single = lua_tostring (L, -3);
    plural = lua_tostring (L, -2);
    count = lua_tonumber (L, -1);

    result = weechat_ngettext (single, plural, count);

    API_RETURN_STRING(result);
}

API_FUNC(strlen_screen)
{
    const char *string;
    int value;

    API_INIT_FUNC(1, "strlen_screen", API_RETURN_INT(0));
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_INT(0));

    string = lua_tostring (L, -1);

    value = weechat_strlen_screen (string);

    API_RETURN_INT(value);
}

API_FUNC(string_match)
{
    const char *string, *mask;
    int case_sensitive, value;

    API_INIT_FUNC(1, "string_match", API_RETURN_INT(0));
    if (lua_gettop (L) < 3)
        API_WRONG_ARGS(API_RETURN_INT(0));

    string = lua_tostring (L, -3);
    mask = lua_tostring (L, -2);
    case_sensitive = lua_tonumber (L, -1);

    value = weechat_string_match (string, mask, case_sensitive);

    API_RETURN_INT(value);
}

API_FUNC(string_match_list)
{
    const char *string, *masks;
    int case_sensitive, value;

    API_INIT_FUNC(1, "string_match_list", API_RETURN_INT(0));
    if (lua_gettop (L) < 3)
        API_WRONG_ARGS(API_RETURN_INT(0));

    string = lua_tostring (L, -3);
    masks = lua_tostring (L, -2);
    case_sensitive = lua_tonumber (L, -1);

    value = plugin_script_api_string_match_list (weechat_lua_plugin,
                                                 string,
                                                 masks,
                                                 case_sensitive);

    API_RETURN_INT(value);
}

API_FUNC(string_has_highlight)
{
    const char *string, *highlight_words;
    int value;

    API_INIT_FUNC(1, "string_has_highlight", API_RETURN_INT(0));
    if (lua_gettop (L) < 2)
        API_WRONG_ARGS(API_RETURN_INT(0));

    string = lua_tostring (L, -2);
    highlight_words = lua_tostring (L, -1);

    value = weechat_string_has_highlight (string, highlight_words);

    API_RETURN_INT(value);
}

API_FUNC(string_has_highlight_regex)
{
    const char *string, *regex;
    int value;

    API_INIT_FUNC(1, "string_has_highlight_regex", API_RETURN_INT(0));
    if (lua_gettop (L) < 2)
        API_WRONG_ARGS(API_RETURN_INT(0));

    string = lua_tostring (L, -2);
    regex = lua_tostring (L, -1);

    value = weechat_string_has_highlight_regex (string, regex);

    API_RETURN_INT(value);
}

API_FUNC(string_mask_to_regex)
{
    const char *mask;
    char *result;

    API_INIT_FUNC(1, "string_mask_to_regex", API_RETURN_EMPTY);
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    mask = lua_tostring (L, -1);

    result = weechat_string_mask_to_regex (mask);

    API_RETURN_STRING_FREE(result);
}

API_FUNC(string_format_size)
{
    unsigned long long size;
    char *result;

    API_INIT_FUNC(1, "string_format_size", API_RETURN_EMPTY);
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    size = lua_tonumber (L, -1);

    result = weechat_string_format_size (size);

    API_RETURN_STRING_FREE(result);
}

API_FUNC(string_parse_size)
{
    const char *size;
    unsigned long long value;

    API_INIT_FUNC(1, "string_parse_size", API_RETURN_LONG(0));
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_LONG(0));

    size = lua_tostring (L, -1);

    value = weechat_string_parse_size (size);

    API_RETURN_LONG(value);
}

API_FUNC(string_color_code_size)
{
    const char *string;
    int size;

    API_INIT_FUNC(1, "string_color_code_size", API_RETURN_INT(0));
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_INT(0));

    string = lua_tostring (L, -1);

    size = weechat_string_color_code_size (string);

    API_RETURN_INT(size);
}

API_FUNC(string_remove_color)
{
    const char *string, *replacement;
    char *result;

    API_INIT_FUNC(1, "string_remove_color", API_RETURN_EMPTY);
    if (lua_gettop (L) < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    string = lua_tostring (L, -2);
    replacement = lua_tostring (L, -1);

    result = weechat_string_remove_color (string, replacement);

    API_RETURN_STRING_FREE(result);
}

API_FUNC(string_is_command_char)
{
    const char *string;
    int value;

    API_INIT_FUNC(1, "string_is_command_char", API_RETURN_INT(0));
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_INT(0));

    string = lua_tostring (L, -1);

    value = weechat_string_is_command_char (string);

    API_RETURN_INT(value);
}

API_FUNC(string_input_for_buffer)
{
    const char *string, *result;

    API_INIT_FUNC(1, "string_input_for_buffer", API_RETURN_EMPTY);
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    string = lua_tostring (L, -1);

    result = weechat_string_input_for_buffer (string);

    API_RETURN_STRING(result);
}

API_FUNC(string_eval_expression)
{
    const char *expr;
    struct t_hashtable *pointers, *extra_vars, *options;
    char *result;

    API_INIT_FUNC(1, "string_eval_expression", API_RETURN_EMPTY);
    if (lua_gettop (L) < 4)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    expr = lua_tostring (L, -4);
    pointers = weechat_lua_tohashtable (L, -3,
                                        WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                        WEECHAT_HASHTABLE_STRING,
                                        WEECHAT_HASHTABLE_POINTER);
    extra_vars = weechat_lua_tohashtable (L, -2,
                                          WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                          WEECHAT_HASHTABLE_STRING,
                                          WEECHAT_HASHTABLE_STRING);
    options = weechat_lua_tohashtable (L, -1,
                                       WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                       WEECHAT_HASHTABLE_STRING,
                                       WEECHAT_HASHTABLE_STRING);

    result = weechat_string_eval_expression (expr, pointers, extra_vars,
                                             options);

    weechat_hashtable_free (pointers);
    weechat_hashtable_free (extra_vars);
    weechat_hashtable_free (options);

    API_RETURN_STRING_FREE(result);
}

API_FUNC(string_eval_path_home)
{
    const char *path;
    struct t_hashtable *pointers, *extra_vars, *options;
    char *result;

    API_INIT_FUNC(1, "string_eval_path_home", API_RETURN_EMPTY);
    if (lua_gettop (L) < 4)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    path = lua_tostring (L, -4);
    pointers = weechat_lua_tohashtable (L, -3,
                                        WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                        WEECHAT_HASHTABLE_STRING,
                                        WEECHAT_HASHTABLE_POINTER);
    extra_vars = weechat_lua_tohashtable (L, -2,
                                          WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                          WEECHAT_HASHTABLE_STRING,
                                          WEECHAT_HASHTABLE_STRING);
    options = weechat_lua_tohashtable (L, -1,
                                       WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                       WEECHAT_HASHTABLE_STRING,
                                       WEECHAT_HASHTABLE_STRING);

    result = weechat_string_eval_path_home (path, pointers, extra_vars,
                                            options);

    weechat_hashtable_free (pointers);
    weechat_hashtable_free (extra_vars);
    weechat_hashtable_free (options);

    API_RETURN_STRING_FREE(result);
}

API_FUNC(mkdir_home)
{
    const char *directory;
    int mode;

    API_INIT_FUNC(1, "mkdir_home", API_RETURN_ERROR);
    if (lua_gettop (L) < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    directory = lua_tostring (L, -2);
    mode = lua_tonumber (L, -1);

    if (weechat_mkdir_home (directory, mode))
        API_RETURN_OK;

    API_RETURN_ERROR;
}

API_FUNC(mkdir)
{
    const char *directory;
    int mode;

    API_INIT_FUNC(1, "mkdir", API_RETURN_ERROR);
    if (lua_gettop (L) < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    directory = lua_tostring (L, -2);
    mode = lua_tonumber (L, -1);

    if (weechat_mkdir (directory, mode))
        API_RETURN_OK;

    API_RETURN_ERROR;
}

API_FUNC(mkdir_parents)
{
    const char *directory;
    int mode;

    API_INIT_FUNC(1, "mkdir_parents", API_RETURN_ERROR);
    if (lua_gettop (L) < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    directory = lua_tostring (L, -2);
    mode = lua_tonumber (L, -1);

    if (weechat_mkdir_parents (directory, mode))
        API_RETURN_OK;

    API_RETURN_ERROR;
}

API_FUNC(list_new)
{
    const char *result;

    API_INIT_FUNC(1, "list_new", API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_list_new ());

    API_RETURN_STRING(result);
}

API_FUNC(list_add)
{
    const char *weelist, *data, *where, *user_data;
    const char *result;

    API_INIT_FUNC(1, "list_add", API_RETURN_EMPTY);
    if (lua_gettop (L) < 4)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    weelist = lua_tostring (L, -4);
    data = lua_tostring (L, -3);
    where = lua_tostring (L, -2);
    user_data = lua_tostring (L, -1);

    result = API_PTR2STR(weechat_list_add (API_STR2PTR(weelist),
                                           data,
                                           where,
                                           API_STR2PTR(user_data)));

    API_RETURN_STRING(result);
}

API_FUNC(list_search)
{
    const char *weelist, *data;
    const char *result;

    API_INIT_FUNC(1, "list_search", API_RETURN_EMPTY);
    if (lua_gettop (L) < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    weelist = lua_tostring (L, -2);
    data = lua_tostring (L, -1);

    result = API_PTR2STR(weechat_list_search (API_STR2PTR(weelist),
                                              data));

    API_RETURN_STRING(result);
}

API_FUNC(list_search_pos)
{
    const char *weelist, *data;
    int pos;

    API_INIT_FUNC(1, "list_search_pos", API_RETURN_INT(-1));
    if (lua_gettop (L) < 2)
        API_WRONG_ARGS(API_RETURN_INT(-1));

    weelist = lua_tostring (L, -2);
    data = lua_tostring (L, -1);

    pos = weechat_list_search_pos (API_STR2PTR(weelist), data);

    API_RETURN_INT(pos);
}

API_FUNC(list_casesearch)
{
    const char *weelist, *data;
    const char *result;

    API_INIT_FUNC(1, "list_casesearch", API_RETURN_EMPTY);
    if (lua_gettop (L) < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    weelist = lua_tostring (L, -2);
    data = lua_tostring (L, -1);

    result = API_PTR2STR(weechat_list_casesearch (API_STR2PTR(weelist),
                                                  data));

    API_RETURN_STRING(result);
}

API_FUNC(list_casesearch_pos)
{
    const char *weelist, *data;
    int pos;

    API_INIT_FUNC(1, "list_casesearch_pos", API_RETURN_INT(-1));
    if (lua_gettop (L) < 2)
        API_WRONG_ARGS(API_RETURN_INT(-1));

    weelist = lua_tostring (L, -2);
    data = lua_tostring (L, -1);

    pos = weechat_list_casesearch_pos (API_STR2PTR(weelist), data);

    API_RETURN_INT(pos);
}

API_FUNC(list_get)
{
    const char *weelist;
    const char *result;
    int position;

    API_INIT_FUNC(1, "list_get", API_RETURN_EMPTY);
    if (lua_gettop (L) < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    weelist = lua_tostring (L, -2);
    position = lua_tonumber (L, -1);

    result = API_PTR2STR(weechat_list_get (API_STR2PTR(weelist),
                                           position));

    API_RETURN_STRING(result);
}

API_FUNC(list_set)
{
    const char *item, *new_value;

    API_INIT_FUNC(1, "list_set", API_RETURN_ERROR);
    if (lua_gettop (L) < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    item = lua_tostring (L, -2);
    new_value = lua_tostring (L, -1);

    weechat_list_set (API_STR2PTR(item),
                      new_value);

    API_RETURN_OK;
}

API_FUNC(list_next)
{
    const char *item;
    const char *result;

    API_INIT_FUNC(1, "list_next", API_RETURN_EMPTY);
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    item = lua_tostring (L, -1);

    result = API_PTR2STR(weechat_list_next (API_STR2PTR(item)));

    API_RETURN_STRING(result);
}

API_FUNC(list_prev)
{
    const char *item;
    const char *result;

    API_INIT_FUNC(1, "list_prev", API_RETURN_EMPTY);
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    item = lua_tostring (L, -1);

    result = API_PTR2STR(weechat_list_prev (API_STR2PTR(item)));

    API_RETURN_STRING(result);
}

API_FUNC(list_string)
{
    const char *item, *result;

    API_INIT_FUNC(1, "list_string", API_RETURN_EMPTY);
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    item = lua_tostring (L, -1);

    result = weechat_list_string (API_STR2PTR(item));

    API_RETURN_STRING(result);
}

API_FUNC(list_size)
{
    const char *weelist;
    int size;

    API_INIT_FUNC(1, "list_size", API_RETURN_INT(0));
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_INT(0));

    weelist = lua_tostring (L, -1);

    size = weechat_list_size (API_STR2PTR(weelist));

    API_RETURN_INT(size);
}

API_FUNC(list_remove)
{
    const char *weelist, *item;

    API_INIT_FUNC(1, "list_remove", API_RETURN_ERROR);
    if (lua_gettop (L) < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    weelist = lua_tostring (L, -2);
    item = lua_tostring (L, -1);

    weechat_list_remove (API_STR2PTR(weelist),
                         API_STR2PTR(item));

    API_RETURN_OK;
}

API_FUNC(list_remove_all)
{
    const char *weelist;

    API_INIT_FUNC(1, "list_remove_all", API_RETURN_ERROR);
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_ERROR);

    weelist = lua_tostring (L, -1);

    weechat_list_remove_all (API_STR2PTR(weelist));

    API_RETURN_OK;
}

API_FUNC(list_free)
{
    const char *weelist;

    API_INIT_FUNC(1, "list_free", API_RETURN_ERROR);
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_ERROR);

    weelist = lua_tostring (L, -1);

    weechat_list_free (API_STR2PTR(weelist));

    API_RETURN_OK;
}

int
weechat_lua_api_config_reload_cb (const void *pointer, void *data,
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

        rc = (int *) weechat_lua_exec (script,
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
    const char *name, *function, *data;
    const char *result;

    API_INIT_FUNC(1, "config_new", API_RETURN_EMPTY);
    if (lua_gettop (L) < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    name = lua_tostring (L, -3);
    function = lua_tostring (L, -2);
    data = lua_tostring (L, -1);

    result = API_PTR2STR(plugin_script_api_config_new (weechat_lua_plugin,
                                                       lua_current_script,
                                                       name,
                                                       &weechat_lua_api_config_reload_cb,
                                                       function,
                                                       data));

    API_RETURN_STRING(result);
}

struct t_hashtable *
weechat_lua_api_config_update_cb (const void *pointer, void *data,
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

        ret_hashtable = weechat_lua_exec (script,
                                          WEECHAT_SCRIPT_EXEC_HASHTABLE,
                                          ptr_function,
                                          "ssih", func_argv);

        return ret_hashtable;
    }

    return NULL;
}

API_FUNC(config_set_version)
{
    const char *config_file, *function, *data;
    int rc, version;

    API_INIT_FUNC(1, "config_set_version", API_RETURN_INT(0));
    if (lua_gettop (L) < 4)
        API_WRONG_ARGS(API_RETURN_INT(0));

    config_file = lua_tostring (L, -4);
    version = lua_tonumber (L, -3);
    function = lua_tostring (L, -2);
    data = lua_tostring (L, -1);

    rc = plugin_script_api_config_set_version (
        weechat_lua_plugin,
        lua_current_script,
        API_STR2PTR(config_file),
        version,
        &weechat_lua_api_config_update_cb,
        function,
        data);

    API_RETURN_INT(rc);
}

int
weechat_lua_api_config_read_cb (const void *pointer, void *data,
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

        rc = (int *) weechat_lua_exec (script,
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
weechat_lua_api_config_section_write_cb (const void *pointer, void *data,
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

        rc = (int *) weechat_lua_exec (script,
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
weechat_lua_api_config_section_write_default_cb (const void *pointer, void *data,
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

        rc = (int *) weechat_lua_exec (script,
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
weechat_lua_api_config_section_create_option_cb (const void *pointer, void *data,
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

        rc = (int *) weechat_lua_exec (script,
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
weechat_lua_api_config_section_delete_option_cb (const void *pointer, void *data,
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

        rc = (int *) weechat_lua_exec (script,
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
    const char *config_file, *name, *function_read, *data_read;
    const char *function_write, *data_write, *function_write_default;
    const char *data_write_default, *function_create_option;
    const char *data_create_option, *function_delete_option;
    const char *data_delete_option;
    const char *result;
    int user_can_add_options, user_can_delete_options;

    API_INIT_FUNC(1, "config_new_section", API_RETURN_EMPTY);
    if (lua_gettop (L) < 14)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    config_file = lua_tostring (L, -14);
    name = lua_tostring (L, -13);
    user_can_add_options = lua_tonumber (L, -12);
    user_can_delete_options = lua_tonumber (L, -11);
    function_read = lua_tostring (L, -10);
    data_read = lua_tostring (L, -9);
    function_write = lua_tostring (L, -8);
    data_write = lua_tostring (L, -7);
    function_write_default = lua_tostring (L, -6);
    data_write_default = lua_tostring (L, -5);
    function_create_option = lua_tostring (L, -4);
    data_create_option = lua_tostring (L, -3);
    function_delete_option = lua_tostring (L, -2);
    data_delete_option = lua_tostring (L, -1);

    result = API_PTR2STR(
        plugin_script_api_config_new_section (
            weechat_lua_plugin,
            lua_current_script,
            API_STR2PTR(config_file),
            name,
            user_can_add_options,
            user_can_delete_options,
            &weechat_lua_api_config_read_cb,
            function_read,
            data_read,
            &weechat_lua_api_config_section_write_cb,
            function_write,
            data_write,
            &weechat_lua_api_config_section_write_default_cb,
            function_write_default,
            data_write_default,
            &weechat_lua_api_config_section_create_option_cb,
            function_create_option,
            data_create_option,
            &weechat_lua_api_config_section_delete_option_cb,
            function_delete_option,
            data_delete_option));

    API_RETURN_STRING(result);
}

API_FUNC(config_search_section)
{
    const char *config_file, *section_name;
    const char *result;

    API_INIT_FUNC(1, "config_search_section", API_RETURN_EMPTY);
    if (lua_gettop (L) < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    config_file = lua_tostring (L, -2);
    section_name = lua_tostring (L, -1);

    result = API_PTR2STR(weechat_config_search_section (API_STR2PTR(config_file),
                                                        section_name));

    API_RETURN_STRING(result);
}

int
weechat_lua_api_config_option_check_value_cb (const void *pointer, void *data,
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

        rc = (int *) weechat_lua_exec (script,
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
weechat_lua_api_config_option_change_cb (const void *pointer, void *data,
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

        rc = weechat_lua_exec (script,
                               WEECHAT_SCRIPT_EXEC_IGNORE,
                               ptr_function,
                               "ss", func_argv);
        free (rc);
    }
}

void
weechat_lua_api_config_option_delete_cb (const void *pointer, void *data,
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

        rc = weechat_lua_exec (script,
                               WEECHAT_SCRIPT_EXEC_IGNORE,
                               ptr_function,
                               "ss", func_argv);
        free (rc);
    }
}

API_FUNC(config_new_option)
{
    const char *config_file, *section, *name, *type, *description;
    const char *string_values, *default_value, *value;
    const char *function_check_value, *data_check_value, *function_change;
    const char *data_change, *function_delete, *data_delete;
    const char *result;
    int min, max, null_value_allowed;

    API_INIT_FUNC(1, "config_new_option", API_RETURN_EMPTY);
    if (lua_gettop (L) < 17)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    config_file = lua_tostring (L, -17);
    section = lua_tostring (L, -16);
    name = lua_tostring (L, -15);
    type = lua_tostring (L, -14);
    description = lua_tostring (L, -13);
    string_values = lua_tostring (L, -12);
    min = lua_tonumber (L, -11);
    max = lua_tonumber (L, -10);
    default_value = lua_tostring (L, -9);
    value = lua_tostring (L, -8);
    null_value_allowed = lua_tonumber (L, -7);
    function_check_value = lua_tostring (L, -6);
    data_check_value = lua_tostring (L, -5);
    function_change = lua_tostring (L, -4);
    data_change = lua_tostring (L, -3);
    function_delete = lua_tostring (L, -2);
    data_delete = lua_tostring (L, -1);

    result = API_PTR2STR(plugin_script_api_config_new_option (weechat_lua_plugin,
                                                              lua_current_script,
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
                                                              &weechat_lua_api_config_option_check_value_cb,
                                                              function_check_value,
                                                              data_check_value,
                                                              &weechat_lua_api_config_option_change_cb,
                                                              function_change,
                                                              data_change,
                                                              &weechat_lua_api_config_option_delete_cb,
                                                              function_delete,
                                                              data_delete));

    API_RETURN_STRING(result);
}

API_FUNC(config_search_option)
{
    const char *config_file, *section, *option_name;
    const char *result;

    API_INIT_FUNC(1, "config_search_option", API_RETURN_EMPTY);
    if (lua_gettop (L) < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    config_file = lua_tostring (L, -3);
    section = lua_tostring (L, -2);
    option_name = lua_tostring (L, -1);

    result = API_PTR2STR(weechat_config_search_option (API_STR2PTR(config_file),
                                                       API_STR2PTR(section),
                                                       option_name));

    API_RETURN_STRING(result);
}

API_FUNC(config_string_to_boolean)
{
    const char *text;
    int value;

    API_INIT_FUNC(1, "config_string_to_boolean", API_RETURN_INT(0));
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_INT(0));

    text = lua_tostring (L, -1);

    value = weechat_config_string_to_boolean (text);

    API_RETURN_INT(value);
}

API_FUNC(config_option_reset)
{
    const char *option;
    int run_callback, rc;

    API_INIT_FUNC(1, "config_option_reset", API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));
    if (lua_gettop (L) < 2)
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));

    option = lua_tostring (L, -2);
    run_callback = lua_tonumber (L, -1);

    rc = weechat_config_option_reset (API_STR2PTR(option),
                                      run_callback);

    API_RETURN_INT(rc);
}

API_FUNC(config_option_set)
{
    const char *option, *new_value;
    int run_callback, rc;

    API_INIT_FUNC(1, "config_option_set", API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));
    if (lua_gettop (L) < 3)
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));

    option = lua_tostring (L, -3);
    new_value = lua_tostring (L, -2);
    run_callback = lua_tonumber (L, -1);

    rc = weechat_config_option_set (API_STR2PTR(option),
                                    new_value,
                                    run_callback);

    API_RETURN_INT(rc);
}

API_FUNC(config_option_set_null)
{
    const char *option;
    int run_callback, rc;

    API_INIT_FUNC(1, "config_option_set_null", API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));
    if (lua_gettop (L) < 2)
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));

    option = lua_tostring (L, -2);
    run_callback = lua_tonumber (L, -1);

    rc = weechat_config_option_set_null (API_STR2PTR(option),
                                         run_callback);

    API_RETURN_INT(rc);
}

API_FUNC(config_option_unset)
{
    const char *option;
    int rc;

    API_INIT_FUNC(1, "config_option_unset", API_RETURN_INT(WEECHAT_CONFIG_OPTION_UNSET_ERROR));
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_OPTION_UNSET_ERROR));

    option = lua_tostring (L, -1);

    rc = weechat_config_option_unset (API_STR2PTR(option));

    API_RETURN_INT(rc);
}

API_FUNC(config_option_rename)
{
    const char *option, *new_name;

    API_INIT_FUNC(1, "config_option_rename", API_RETURN_ERROR);
    if (lua_gettop (L) < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    option = lua_tostring (L, -2);
    new_name = lua_tostring (L, -1);

    weechat_config_option_rename (API_STR2PTR(option),
                                  new_name);

    API_RETURN_OK;
}

API_FUNC(config_option_get_string)
{
    const char *option, *property, *result;

    API_INIT_FUNC(1, "config_option_get_string", API_RETURN_EMPTY);
    if (lua_gettop (L) < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    option = lua_tostring (L, -2);
    property = lua_tostring (L, -1);

    result = weechat_config_option_get_string (API_STR2PTR(option),
                                               property);

    API_RETURN_STRING(result);
}

API_FUNC(config_option_get_pointer)
{
    const char *option, *property;
    const char *result;

    API_INIT_FUNC(1, "config_option_get_pointer", API_RETURN_EMPTY);
    if (lua_gettop (L) < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    option = lua_tostring (L, -2);
    property = lua_tostring (L, -1);

    result = API_PTR2STR(weechat_config_option_get_pointer (API_STR2PTR(option),
                                                            property));

    API_RETURN_STRING(result);
}

API_FUNC(config_option_is_null)
{
    const char *option;
    int value;

    API_INIT_FUNC(1, "config_option_is_null", API_RETURN_INT(1));
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_INT(1));

    option = lua_tostring (L, -1);

    value = weechat_config_option_is_null (API_STR2PTR(option));

    API_RETURN_INT(value);
}

API_FUNC(config_option_default_is_null)
{
    const char *option;
    int value;

    API_INIT_FUNC(1, "config_option_default_is_null", API_RETURN_INT(1));
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_INT(1));

    option = lua_tostring (L, -1);

    value = weechat_config_option_default_is_null (API_STR2PTR(option));

    API_RETURN_INT(value);
}

API_FUNC(config_boolean)
{
    const char *option;
    int value;

    API_INIT_FUNC(1, "config_boolean", API_RETURN_INT(0));
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_INT(0));

    option = lua_tostring (L, -1);

    value = weechat_config_boolean (API_STR2PTR(option));

    API_RETURN_INT(value);
}

API_FUNC(config_boolean_default)
{
    const char *option;
    int value;

    API_INIT_FUNC(1, "config_boolean_default", API_RETURN_INT(0));
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_INT(0));

    option = lua_tostring (L, -1);

    value = weechat_config_boolean_default (API_STR2PTR(option));

    API_RETURN_INT(value);
}

API_FUNC(config_boolean_inherited)
{
    const char *option;
    int value;

    API_INIT_FUNC(1, "config_boolean_inherited", API_RETURN_INT(0));
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_INT(0));

    option = lua_tostring (L, -1);

    value = weechat_config_boolean_inherited (API_STR2PTR(option));

    API_RETURN_INT(value);
}

API_FUNC(config_integer)
{
    const char *option;
    int value;

    API_INIT_FUNC(1, "config_integer", API_RETURN_INT(0));
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_INT(0));

    option = lua_tostring (L, -1);

    value = weechat_config_integer (API_STR2PTR(option));

    API_RETURN_INT(value);
}

API_FUNC(config_integer_default)
{
    const char *option;
    int value;

    API_INIT_FUNC(1, "config_integer_default", API_RETURN_INT(0));
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_INT(0));

    option = lua_tostring (L, -1);

    value = weechat_config_integer_default (API_STR2PTR(option));

    API_RETURN_INT(value);
}

API_FUNC(config_integer_inherited)
{
    const char *option;
    int value;

    API_INIT_FUNC(1, "config_integer_inherited", API_RETURN_INT(0));
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_INT(0));

    option = lua_tostring (L, -1);

    value = weechat_config_integer_inherited (API_STR2PTR(option));

    API_RETURN_INT(value);
}

API_FUNC(config_string)
{
    const char *option, *result;

    API_INIT_FUNC(1, "config_string", API_RETURN_EMPTY);
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    option = lua_tostring (L, -1);

    result = weechat_config_string (API_STR2PTR(option));

    API_RETURN_STRING(result);
}

API_FUNC(config_string_default)
{
    const char *option, *result;

    API_INIT_FUNC(1, "config_string_default", API_RETURN_EMPTY);
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    option = lua_tostring (L, -1);

    result = weechat_config_string_default (API_STR2PTR(option));

    API_RETURN_STRING(result);
}

API_FUNC(config_string_inherited)
{
    const char *option, *result;

    API_INIT_FUNC(1, "config_string_inherited", API_RETURN_EMPTY);
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    option = lua_tostring (L, -1);

    result = weechat_config_string_inherited (API_STR2PTR(option));

    API_RETURN_STRING(result);
}

API_FUNC(config_color)
{
    const char *option, *result;

    API_INIT_FUNC(1, "config_color", API_RETURN_EMPTY);
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    option = lua_tostring (L, -1);

    result = weechat_config_color (API_STR2PTR(option));

    API_RETURN_STRING(result);
}

API_FUNC(config_color_default)
{
    const char *option, *result;

    API_INIT_FUNC(1, "config_color_default", API_RETURN_EMPTY);
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    option = lua_tostring (L, -1);

    result = weechat_config_color_default (API_STR2PTR(option));

    API_RETURN_STRING(result);
}

API_FUNC(config_color_inherited)
{
    const char *option, *result;

    API_INIT_FUNC(1, "config_color_inherited", API_RETURN_EMPTY);
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    option = lua_tostring (L, -1);

    result = weechat_config_color_inherited (API_STR2PTR(option));

    API_RETURN_STRING(result);
}

API_FUNC(config_enum)
{
    const char *option;
    int value;

    API_INIT_FUNC(1, "config_enum", API_RETURN_INT(0));
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_INT(0));

    option = lua_tostring (L, -1);

    value = weechat_config_enum (API_STR2PTR(option));

    API_RETURN_INT(value);
}

API_FUNC(config_enum_default)
{
    const char *option;
    int value;

    API_INIT_FUNC(1, "config_enum_default", API_RETURN_INT(0));
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_INT(0));

    option = lua_tostring (L, -1);

    value = weechat_config_enum_default (API_STR2PTR(option));

    API_RETURN_INT(value);
}

API_FUNC(config_enum_inherited)
{
    const char *option;
    int value;

    API_INIT_FUNC(1, "config_enum_inherited", API_RETURN_INT(0));
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_INT(0));

    option = lua_tostring (L, -1);

    value = weechat_config_enum_inherited (API_STR2PTR(option));

    API_RETURN_INT(value);
}

API_FUNC(config_write_option)
{
    const char *config_file, *option;

    API_INIT_FUNC(1, "config_write_option", API_RETURN_ERROR);
    if (lua_gettop (L) < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    config_file = lua_tostring (L, -2);
    option = lua_tostring (L, -1);

    weechat_config_write_option (API_STR2PTR(config_file),
                                 API_STR2PTR(option));

    API_RETURN_OK;
}

API_FUNC(config_write_line)
{
    const char *config_file, *option_name, *value;

    API_INIT_FUNC(1, "config_write_line", API_RETURN_ERROR);
    if (lua_gettop (L) < 3)
        API_WRONG_ARGS(API_RETURN_ERROR);

    config_file = lua_tostring (L, -3);
    option_name = lua_tostring (L, -2);
    value = lua_tostring (L, -1);

    weechat_config_write_line (API_STR2PTR(config_file),
                               option_name,
                               "%s",
                               value);

    API_RETURN_OK;
}

API_FUNC(config_write)
{
    const char *config_file;
    int rc;

    API_INIT_FUNC(1, "config_write", API_RETURN_INT(WEECHAT_CONFIG_WRITE_ERROR));
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_WRITE_ERROR));

    config_file = lua_tostring (L, -1);

    rc = weechat_config_write (API_STR2PTR(config_file));

    API_RETURN_INT(rc);
}

API_FUNC(config_read)
{
    const char *config_file;
    int rc;

    API_INIT_FUNC(1, "config_read", API_RETURN_INT(WEECHAT_CONFIG_READ_FILE_NOT_FOUND));
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_READ_FILE_NOT_FOUND));

    config_file = lua_tostring (L, -1);

    rc = weechat_config_read (API_STR2PTR(config_file));

    API_RETURN_INT(rc);
}

API_FUNC(config_reload)
{
    const char *config_file;
    int rc;

    API_INIT_FUNC(1, "config_reload", API_RETURN_INT(WEECHAT_CONFIG_READ_FILE_NOT_FOUND));
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_READ_FILE_NOT_FOUND));

    config_file = lua_tostring (L, -1);

    rc = weechat_config_reload (API_STR2PTR(config_file));

    API_RETURN_INT(rc);
}

API_FUNC(config_option_free)
{
    const char *option;

    API_INIT_FUNC(1, "config_option_free", API_RETURN_ERROR);
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_ERROR);

    option = lua_tostring (L, -1);

    weechat_config_option_free (API_STR2PTR(option));

    API_RETURN_OK;
}

API_FUNC(config_section_free_options)
{
    const char *section;

    API_INIT_FUNC(1, "config_section_free_options", API_RETURN_ERROR);
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_ERROR);

    section = lua_tostring (L, -1);

    weechat_config_section_free_options (API_STR2PTR(section));

    API_RETURN_OK;
}

API_FUNC(config_section_free)
{
    const char *section;

    API_INIT_FUNC(1, "config_section_free", API_RETURN_ERROR);
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_ERROR);

    section = lua_tostring (L, -1);

    weechat_config_section_free (API_STR2PTR(section));

    API_RETURN_OK;
}

API_FUNC(config_free)
{
    const char *config_file;

    API_INIT_FUNC(1, "config_free", API_RETURN_ERROR);
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_ERROR);

    config_file = lua_tostring (L, -1);

    weechat_config_free (API_STR2PTR(config_file));

    API_RETURN_OK;
}

API_FUNC(config_get)
{
    const char *option;
    const char *result;

    API_INIT_FUNC(1, "config_get", API_RETURN_EMPTY);
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    option = lua_tostring (L, -1);

    result = API_PTR2STR(weechat_config_get (option));

    API_RETURN_STRING(result);
}

API_FUNC(config_get_plugin)
{
    const char *option, *result;

    API_INIT_FUNC(1, "config_get_plugin", API_RETURN_EMPTY);
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    option = lua_tostring (L, -1);

    result = plugin_script_api_config_get_plugin (weechat_lua_plugin,
                                                  lua_current_script,
                                                  option);

    API_RETURN_STRING(result);
}

API_FUNC(config_is_set_plugin)
{
    const char *option;
    int rc;

    API_INIT_FUNC(1, "config_is_set_plugin", API_RETURN_INT(0));
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_INT(0));

    option = lua_tostring (L, -1);

    rc = plugin_script_api_config_is_set_plugin (weechat_lua_plugin,
                                                 lua_current_script,
                                                 option);

    API_RETURN_INT(rc);
}

API_FUNC(config_set_plugin)
{
    const char *option, *value;
    int rc;

    API_INIT_FUNC(1, "config_set_plugin", API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));
    if (lua_gettop (L) < 2)
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));

    option = lua_tostring (L, -2);
    value = lua_tostring (L, -1);

    rc = plugin_script_api_config_set_plugin (weechat_lua_plugin,
                                              lua_current_script,
                                              option,
                                              value);

    API_RETURN_INT(rc);
}

API_FUNC(config_set_desc_plugin)
{
    const char *option, *description;

    API_INIT_FUNC(1, "config_set_desc_plugin", API_RETURN_ERROR);
    if (lua_gettop (L) < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    option = lua_tostring (L, -2);
    description = lua_tostring (L, -1);

    plugin_script_api_config_set_desc_plugin (weechat_lua_plugin,
                                              lua_current_script,
                                              option,
                                              description);

    API_RETURN_OK;
}

API_FUNC(config_unset_plugin)
{
    const char *option;
    int rc;

    API_INIT_FUNC(1, "config_unset_plugin", API_RETURN_INT(WEECHAT_CONFIG_OPTION_UNSET_ERROR));
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_OPTION_UNSET_ERROR));

    option = lua_tostring (L, -1);

    rc = plugin_script_api_config_unset_plugin (weechat_lua_plugin,
                                                lua_current_script,
                                                option);

    API_RETURN_INT(rc);
}

API_FUNC(key_bind)
{
    const char *context;
    struct t_hashtable *hashtable;
    int num_keys;

    API_INIT_FUNC(1, "key_bind", API_RETURN_INT(0));
    if (lua_gettop (L) < 2)
        API_WRONG_ARGS(API_RETURN_INT(0));

    context = lua_tostring (L, -2);
    hashtable = weechat_lua_tohashtable (L, -1,
                                         WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                         WEECHAT_HASHTABLE_STRING,
                                         WEECHAT_HASHTABLE_STRING);

    num_keys = weechat_key_bind (context, hashtable);

    weechat_hashtable_free (hashtable);

    API_RETURN_INT(num_keys);
}

API_FUNC(key_unbind)
{
    const char *context, *key;
    int num_keys;

    API_INIT_FUNC(1, "key_unbind", API_RETURN_INT(0));
    if (lua_gettop (L) < 2)
        API_WRONG_ARGS(API_RETURN_INT(0));

    context = lua_tostring (L, -2);
    key = lua_tostring (L, -1);

    num_keys = weechat_key_unbind (context, key);

    API_RETURN_INT(num_keys);
}

API_FUNC(prefix)
{
    const char *prefix, *result;

    API_INIT_FUNC(0, "prefix", API_RETURN_EMPTY);
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    prefix = lua_tostring (L, -1);

    result = weechat_prefix (prefix);

    API_RETURN_STRING(result);
}

API_FUNC(color)
{
    const char *color, *result;

    API_INIT_FUNC(0, "color", API_RETURN_EMPTY);
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    color = lua_tostring (L, -1);

    result = weechat_color (color);

    API_RETURN_STRING(result);
}

API_FUNC(print)
{
    const char *buffer, *message;

    API_INIT_FUNC(0, "print", API_RETURN_ERROR);
    if (lua_gettop (L) < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    buffer = lua_tostring (L, -2);
    message = lua_tostring (L, -1);

    plugin_script_api_printf (weechat_lua_plugin,
                              lua_current_script,
                              API_STR2PTR(buffer),
                              "%s", message);

    API_RETURN_OK;
}

API_FUNC(print_date_tags)
{
    const char *buffer, *tags, *message;
    long date;

    API_INIT_FUNC(1, "print_date_tags", API_RETURN_ERROR);
    if (lua_gettop (L) < 4)
        API_WRONG_ARGS(API_RETURN_ERROR);

    buffer = lua_tostring (L, -4);
    date = lua_tonumber (L, -3);
    tags = lua_tostring (L, -2);
    message = lua_tostring (L, -1);

    plugin_script_api_printf_date_tags (weechat_lua_plugin,
                                        lua_current_script,
                                        API_STR2PTR(buffer),
                                        (time_t)date,
                                        tags,
                                        "%s", message);

    API_RETURN_OK;
}

API_FUNC(print_datetime_tags)
{
    const char *buffer, *tags, *message;
    long date;
    int date_usec;

    API_INIT_FUNC(1, "print_datetime_tags", API_RETURN_ERROR);
    if (lua_gettop (L) < 5)
        API_WRONG_ARGS(API_RETURN_ERROR);

    buffer = lua_tostring (L, -5);
    date = lua_tonumber (L, -4);
    date_usec = lua_tonumber (L, -3);
    tags = lua_tostring (L, -2);
    message = lua_tostring (L, -1);

    plugin_script_api_printf_datetime_tags (weechat_lua_plugin,
                                            lua_current_script,
                                            API_STR2PTR(buffer),
                                            (time_t)date,
                                            date_usec,
                                            tags,
                                            "%s", message);

    API_RETURN_OK;
}

API_FUNC(print_y)
{
    const char *buffer, *message;
    int y;

    API_INIT_FUNC(1, "print_y", API_RETURN_ERROR);
    if (lua_gettop (L) < 3)
        API_WRONG_ARGS(API_RETURN_ERROR);

    buffer = lua_tostring (L, -3);
    y = lua_tonumber (L, -2);
    message = lua_tostring (L, -1);

    plugin_script_api_printf_y (weechat_lua_plugin,
                                lua_current_script,
                                API_STR2PTR(buffer),
                                y,
                                "%s", message);

    API_RETURN_OK;
}

API_FUNC(print_y_date_tags)
{
    const char *buffer, *tags, *message;
    int y;
    long date;

    API_INIT_FUNC(1, "print_y_date_tags", API_RETURN_ERROR);
    if (lua_gettop (L) < 5)
        API_WRONG_ARGS(API_RETURN_ERROR);

    buffer = lua_tostring (L, -5);
    y = lua_tonumber (L, -4);
    date = lua_tonumber (L, -3);
    tags = lua_tostring (L, -2);
    message = lua_tostring (L, -1);

    plugin_script_api_printf_y_date_tags (weechat_lua_plugin,
                                          lua_current_script,
                                          API_STR2PTR(buffer),
                                          y,
                                          (time_t)date,
                                          tags,
                                          "%s", message);

    API_RETURN_OK;
}

API_FUNC(print_y_datetime_tags)
{
    const char *buffer, *tags, *message;
    int y, date_usec;
    long date;

    API_INIT_FUNC(1, "print_y_datetime_tags", API_RETURN_ERROR);
    if (lua_gettop (L) < 6)
        API_WRONG_ARGS(API_RETURN_ERROR);

    buffer = lua_tostring (L, -6);
    y = lua_tonumber (L, -5);
    date = lua_tonumber (L, -4);
    date_usec = lua_tonumber (L, -3);
    tags = lua_tostring (L, -2);
    message = lua_tostring (L, -1);

    plugin_script_api_printf_y_datetime_tags (weechat_lua_plugin,
                                              lua_current_script,
                                              API_STR2PTR(buffer),
                                              y,
                                              (time_t)date,
                                              date_usec,
                                              tags,
                                              "%s", message);

    API_RETURN_OK;
}

API_FUNC(log_print)
{
    const char *message;

    API_INIT_FUNC(1, "log_print", API_RETURN_ERROR);
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_ERROR);

    message = lua_tostring (L, -1);

    plugin_script_api_log_printf (weechat_lua_plugin,
                                  lua_current_script,
                                  "%s", message);

    API_RETURN_OK;
}

int
weechat_lua_api_hook_command_cb (const void *pointer, void *data,
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

        rc = (int *) weechat_lua_exec (script,
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
    const char *command, *description, *args, *args_description, *completion;
    const char *function, *data;
    const char *result;

    API_INIT_FUNC(1, "hook_command", API_RETURN_EMPTY);
    if (lua_gettop (L) < 7)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    command = lua_tostring (L, -7);
    description = lua_tostring (L, -6);
    args = lua_tostring (L, -5);
    args_description = lua_tostring (L, -4);
    completion = lua_tostring (L, -3);
    function = lua_tostring (L, -2);
    data = lua_tostring (L, -1);

    result = API_PTR2STR(plugin_script_api_hook_command (weechat_lua_plugin,
                                                         lua_current_script,
                                                         command,
                                                         description,
                                                         args,
                                                         args_description,
                                                         completion,
                                                         &weechat_lua_api_hook_command_cb,
                                                         function,
                                                         data));

    API_RETURN_STRING(result);
}

int
weechat_lua_api_hook_completion_cb (const void *pointer, void *data,
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

        rc = (int *) weechat_lua_exec (script,
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
    const char *completion, *description, *function, *data;
    const char *result;

    API_INIT_FUNC(1, "hook_completion", API_RETURN_EMPTY);
    if (lua_gettop (L) < 4)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    completion = lua_tostring (L, -4);
    description = lua_tostring (L, -3);
    function = lua_tostring (L, -2);
    data = lua_tostring (L, -1);

    result = API_PTR2STR(plugin_script_api_hook_completion (weechat_lua_plugin,
                                                            lua_current_script,
                                                            completion,
                                                            description,
                                                            &weechat_lua_api_hook_completion_cb,
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
    const char *completion, *property, *result;

    API_INIT_FUNC(1, "hook_completion_get_string", API_RETURN_EMPTY);
    if (lua_gettop (L) < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    completion = lua_tostring (L, -2);
    property = lua_tostring (L, -1);

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
    const char *completion, *word, *where;
    int nick_completion;

    API_INIT_FUNC(1, "hook_completion_list_add", API_RETURN_ERROR);
    if (lua_gettop (L) < 4)
        API_WRONG_ARGS(API_RETURN_ERROR);

    completion = lua_tostring (L, -4);
    word = lua_tostring (L, -3);
    nick_completion = lua_tonumber (L, -2);
    where = lua_tostring (L, -1);

    weechat_hook_completion_list_add (API_STR2PTR(completion),
                                      word,
                                      nick_completion,
                                      where);

    API_RETURN_OK;
}

int
weechat_lua_api_hook_command_run_cb (const void *pointer, void *data,
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

        rc = (int *) weechat_lua_exec (script,
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
    const char *command, *function, *data;
    const char *result;

    API_INIT_FUNC(1, "hook_command_run", API_RETURN_EMPTY);
    if (lua_gettop (L) < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    command = lua_tostring (L, -3);
    function = lua_tostring (L, -2);
    data = lua_tostring (L, -1);

    result = API_PTR2STR(plugin_script_api_hook_command_run (weechat_lua_plugin,
                                                             lua_current_script,
                                                             command,
                                                             &weechat_lua_api_hook_command_run_cb,
                                                             function,
                                                             data));

    API_RETURN_STRING(result);
}

int
weechat_lua_api_hook_timer_cb (const void *pointer, void *data,
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

        rc = (int *) weechat_lua_exec (script,
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
    const char *function, *data;
    const char *result;

    API_INIT_FUNC(1, "hook_timer", API_RETURN_EMPTY);
    if (lua_gettop (L) < 5)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    interval = lua_tonumber (L, -5);
    align_second = lua_tonumber (L, -4);
    max_calls = lua_tonumber (L, -3);
    function = lua_tostring (L, -2);
    data = lua_tostring (L, -1);

    result = API_PTR2STR(plugin_script_api_hook_timer (weechat_lua_plugin,
                                                       lua_current_script,
                                                       interval,
                                                       align_second,
                                                       max_calls,
                                                       &weechat_lua_api_hook_timer_cb,
                                                       function,
                                                       data));

    API_RETURN_STRING(result);
}

int
weechat_lua_api_hook_fd_cb (const void *pointer, void *data, int fd)
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

        rc = (int *) weechat_lua_exec (script,
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
    const char *function, *data;
    const char *result;

    API_INIT_FUNC(1, "hook_fd", API_RETURN_EMPTY);
    if (lua_gettop (L) < 6)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    fd = lua_tonumber (L, -6);
    read = lua_tonumber (L, -5);
    write = lua_tonumber (L, -4);
    exception = lua_tonumber (L, -3);
    function = lua_tostring (L, -2);
    data = lua_tostring (L, -1);

    result = API_PTR2STR(plugin_script_api_hook_fd (weechat_lua_plugin,
                                                    lua_current_script,
                                                    fd,
                                                    read,
                                                    write,
                                                    exception,
                                                    &weechat_lua_api_hook_fd_cb,
                                                    function,
                                                    data));

    API_RETURN_STRING(result);
}

int
weechat_lua_api_hook_process_cb (const void *pointer, void *data,
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

            result = (char *) weechat_lua_exec (script,
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

        rc = (int *) weechat_lua_exec (script,
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
    const char *command, *function, *data;
    int timeout;
    const char *result;

    API_INIT_FUNC(1, "hook_process", API_RETURN_EMPTY);
    if (lua_gettop (L) < 4)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    command = lua_tostring (L, -4);
    timeout = lua_tonumber (L, -3);
    function = lua_tostring (L, -2);
    data = lua_tostring (L, -1);

    result = API_PTR2STR(plugin_script_api_hook_process (weechat_lua_plugin,
                                                         lua_current_script,
                                                         command,
                                                         timeout,
                                                         &weechat_lua_api_hook_process_cb,
                                                         function,
                                                         data));

    API_RETURN_STRING(result);
}

API_FUNC(hook_process_hashtable)
{
    const char *command, *function, *data;
    struct t_hashtable *options;
    int timeout;
    const char *result;

    API_INIT_FUNC(1, "hook_process_hashtable", API_RETURN_EMPTY);
    if (lua_gettop (L) < 5)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    command = lua_tostring (L, -5);
    options = weechat_lua_tohashtable (L, -4,
                                       WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                       WEECHAT_HASHTABLE_STRING,
                                       WEECHAT_HASHTABLE_STRING);
    timeout = lua_tonumber (L, -3);
    function = lua_tostring (L, -2);
    data = lua_tostring (L, -1);

    result = API_PTR2STR(plugin_script_api_hook_process_hashtable (weechat_lua_plugin,
                                                                   lua_current_script,
                                                                   command,
                                                                   options,
                                                                   timeout,
                                                                   &weechat_lua_api_hook_process_cb,
                                                                   function,
                                                                   data));

    weechat_hashtable_free (options);

    API_RETURN_STRING(result);
}

int
weechat_lua_api_hook_url_cb (const void *pointer, void *data,
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

        rc = (int *) weechat_lua_exec (script,
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

API_FUNC(hook_url)
{
    const char *url, *function, *data;
    struct t_hashtable *options;
    int timeout;
    const char *result;

    API_INIT_FUNC(1, "hook_url", API_RETURN_EMPTY);
    if (lua_gettop (L) < 5)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    url = lua_tostring (L, -5);
    options = weechat_lua_tohashtable (L, -4,
                                       WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                       WEECHAT_HASHTABLE_STRING,
                                       WEECHAT_HASHTABLE_STRING);
    timeout = lua_tonumber (L, -3);
    function = lua_tostring (L, -2);
    data = lua_tostring (L, -1);

    result = API_PTR2STR(plugin_script_api_hook_url (weechat_lua_plugin,
                                                     lua_current_script,
                                                     url,
                                                     options,
                                                     timeout,
                                                     &weechat_lua_api_hook_url_cb,
                                                     function,
                                                     data));

    weechat_hashtable_free (options);

    API_RETURN_STRING(result);
}

int
weechat_lua_api_hook_connect_cb (const void *pointer, void *data,
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

        rc = (int *) weechat_lua_exec (script,
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
    const char *proxy, *address, *local_hostname, *function, *data;
    int port, ipv6, retry;
    const char *result;

    API_INIT_FUNC(1, "hook_connect", API_RETURN_EMPTY);
    if (lua_gettop (L) < 8)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    proxy = lua_tostring (L, -8);
    address = lua_tostring (L, -7);
    port = lua_tonumber (L, -6);
    ipv6 = lua_tonumber (L, -5);
    retry = lua_tonumber (L, -4);
    local_hostname = lua_tostring (L, -3);
    function = lua_tostring (L, -2);
    data = lua_tostring (L, -1);

    result = API_PTR2STR(plugin_script_api_hook_connect (weechat_lua_plugin,
                                                         lua_current_script,
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
                                                         &weechat_lua_api_hook_connect_cb,
                                                         function,
                                                         data));

    API_RETURN_STRING(result);
}

struct t_hashtable *
weechat_lua_api_hook_line_cb (const void *pointer, void *data,
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

        return (struct t_hashtable *)weechat_lua_exec (
            script,
            WEECHAT_SCRIPT_EXEC_HASHTABLE,
            ptr_function,
            "sh", func_argv);
    }

    return NULL;
}

API_FUNC(hook_line)
{
    const char *buffer_type, *buffer_name, *tags, *function, *data;
    const char *result;

    API_INIT_FUNC(1, "hook_line", API_RETURN_EMPTY);
    if (lua_gettop (L) < 5)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer_type = lua_tostring (L, -5);
    buffer_name = lua_tostring (L, -4);
    tags = lua_tostring (L, -3);
    function = lua_tostring (L, -2);
    data = lua_tostring (L, -1);

    result = API_PTR2STR(plugin_script_api_hook_line (weechat_lua_plugin,
                                                      lua_current_script,
                                                      buffer_type,
                                                      buffer_name,
                                                      tags,
                                                      &weechat_lua_api_hook_line_cb,
                                                      function,
                                                      data));

    API_RETURN_STRING(result);
}

int
weechat_lua_api_hook_print_cb (const void *pointer, void *data,
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

        rc = (int *) weechat_lua_exec (script,
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

API_FUNC(hook_print)
{
    const char *buffer, *tags, *message, *function, *data;
    const char *result;
    int strip_colors;

    API_INIT_FUNC(1, "hook_print", API_RETURN_EMPTY);
    if (lua_gettop (L) < 6)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = lua_tostring (L, -6);
    tags = lua_tostring (L, -5);
    message = lua_tostring (L, -4);
    strip_colors = lua_tonumber (L, -3);
    function = lua_tostring (L, -2);
    data = lua_tostring (L, -1);

    result = API_PTR2STR(plugin_script_api_hook_print (weechat_lua_plugin,
                                                       lua_current_script,
                                                       API_STR2PTR(buffer),
                                                       tags,
                                                       message,
                                                       strip_colors,
                                                       &weechat_lua_api_hook_print_cb,
                                                       function,
                                                       data));

    API_RETURN_STRING(result);
}

int
weechat_lua_api_hook_signal_cb (const void *pointer, void *data,
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

        rc = (int *) weechat_lua_exec (script,
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
    const char *signal, *function, *data;
    const char *result;

    API_INIT_FUNC(1, "hook_signal", API_RETURN_EMPTY);
    if (lua_gettop (L) < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    signal = lua_tostring (L, -3);
    function = lua_tostring (L, -2);
    data = lua_tostring (L, -1);

    result = API_PTR2STR(plugin_script_api_hook_signal (weechat_lua_plugin,
                                                        lua_current_script,
                                                        signal,
                                                        &weechat_lua_api_hook_signal_cb,
                                                        function,
                                                        data));

    API_RETURN_STRING(result);
}

API_FUNC(hook_signal_send)
{
    const char *signal, *type_data, *signal_data;
    int number, rc;

    API_INIT_FUNC(1, "hook_signal_send", API_RETURN_INT(WEECHAT_RC_ERROR));
    signal_data = NULL;

    if (lua_gettop (L) < 3)
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_RC_ERROR));

    signal = lua_tostring (L, -3);
    type_data = lua_tostring (L, -2);

    if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_STRING) == 0)
    {
        signal_data = lua_tostring (L, -1);
        rc = weechat_hook_signal_send (signal, type_data, (void *)signal_data);
        API_RETURN_INT(rc);
    }
    else if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_INT) == 0)
    {
        number = lua_tonumber (L, -1);
        rc = weechat_hook_signal_send (signal, type_data, &number);
        API_RETURN_INT(rc);
    }
    else if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_POINTER) == 0)
    {
        signal_data = lua_tostring (L, -1);
        rc = weechat_hook_signal_send (signal, type_data,
                                       API_STR2PTR(signal_data));
        API_RETURN_INT(rc);
    }

    API_RETURN_INT(WEECHAT_RC_ERROR);
}

int
weechat_lua_api_hook_hsignal_cb (const void *pointer, void *data,
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

        rc = (int *) weechat_lua_exec (script,
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
    const char *signal, *function, *data;
    const char *result;

    API_INIT_FUNC(1, "hook_hsignal", API_RETURN_EMPTY);
    if (lua_gettop (L) < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    signal = lua_tostring (L, -3);
    function = lua_tostring (L, -2);
    data = lua_tostring (L, -1);

    result = API_PTR2STR(plugin_script_api_hook_hsignal (weechat_lua_plugin,
                                                         lua_current_script,
                                                         signal,
                                                         &weechat_lua_api_hook_hsignal_cb,
                                                         function,
                                                         data));

    API_RETURN_STRING(result);
}

API_FUNC(hook_hsignal_send)
{
    const char *signal;
    struct t_hashtable *hashtable;
    int rc;

    API_INIT_FUNC(1, "hook_hsignal_send", API_RETURN_INT(WEECHAT_RC_ERROR));
    if (lua_gettop (L) < 2)
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_RC_ERROR));

    signal = lua_tostring (L, -2);
    hashtable = weechat_lua_tohashtable (L, -1,
                                         WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                         WEECHAT_HASHTABLE_STRING,
                                         WEECHAT_HASHTABLE_STRING);

    rc = weechat_hook_hsignal_send (signal, hashtable);

    weechat_hashtable_free (hashtable);

    API_RETURN_INT(rc);
}

int
weechat_lua_api_hook_config_cb (const void *pointer, void *data,
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

        rc = (int *) weechat_lua_exec (script,
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
    const char *option, *function, *data;
    const char *result;

    API_INIT_FUNC(1, "hook_config", API_RETURN_EMPTY);
    if (lua_gettop (L) < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    option = lua_tostring (L, -3);
    function = lua_tostring (L, -2);
    data = lua_tostring (L, -1);

    result = API_PTR2STR(plugin_script_api_hook_config (weechat_lua_plugin,
                                                        lua_current_script,
                                                        option,
                                                        &weechat_lua_api_hook_config_cb,
                                                        function,
                                                        data));

    API_RETURN_STRING(result);
}

char *
weechat_lua_api_hook_modifier_cb (const void *pointer, void *data,
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

        return (char *)weechat_lua_exec (script,
                                         WEECHAT_SCRIPT_EXEC_STRING,
                                         ptr_function,
                                         "ssss", func_argv);
    }

    return NULL;
}

API_FUNC(hook_modifier)
{
    const char *modifier, *function, *data;
    const char *result;

    API_INIT_FUNC(1, "hook_modifier", API_RETURN_EMPTY);
    if (lua_gettop (L) < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    modifier = lua_tostring (L, -3);
    function = lua_tostring (L, -2);
    data = lua_tostring (L, -1);

    result = API_PTR2STR(plugin_script_api_hook_modifier (weechat_lua_plugin,
                                                          lua_current_script,
                                                          modifier,
                                                          &weechat_lua_api_hook_modifier_cb,
                                                          function,
                                                          data));

    API_RETURN_STRING(result);
}

API_FUNC(hook_modifier_exec)
{
    const char *modifier, *modifier_data, *string;
    char *result;

    API_INIT_FUNC(1, "hook_modifier_exec", API_RETURN_EMPTY);
    if (lua_gettop (L) < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    modifier = lua_tostring (L, -3);
    modifier_data = lua_tostring (L, -2);
    string = lua_tostring (L, -1);

    result = weechat_hook_modifier_exec (modifier, modifier_data, string);

    API_RETURN_STRING_FREE(result);
}

char *
weechat_lua_api_hook_info_cb (const void *pointer, void *data,
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

        return (char *)weechat_lua_exec (script,
                                         WEECHAT_SCRIPT_EXEC_STRING,
                                         ptr_function,
                                         "sss", func_argv);
    }

    return NULL;
}

API_FUNC(hook_info)
{
    const char *info_name, *description, *args_description, *function, *data;
    const char *result;

    API_INIT_FUNC(1, "hook_info", API_RETURN_EMPTY);
    if (lua_gettop (L) < 5)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    info_name = lua_tostring (L, -5);
    description = lua_tostring (L, -4);
    args_description = lua_tostring (L, -3);
    function = lua_tostring (L, -2);
    data = lua_tostring (L, -1);

    result = API_PTR2STR(plugin_script_api_hook_info (weechat_lua_plugin,
                                                      lua_current_script,
                                                      info_name,
                                                      description,
                                                      args_description,
                                                      &weechat_lua_api_hook_info_cb,
                                                      function,
                                                      data));

    API_RETURN_STRING(result);
}

struct t_hashtable *
weechat_lua_api_hook_info_hashtable_cb (const void *pointer, void *data,
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

        return (struct t_hashtable *)weechat_lua_exec (
            script,
            WEECHAT_SCRIPT_EXEC_HASHTABLE,
            ptr_function,
            "ssh", func_argv);
    }

    return NULL;
}

API_FUNC(hook_info_hashtable)
{
    const char *info_name, *description, *args_description;
    const char *output_description, *function, *data;
    const char *result;

    API_INIT_FUNC(1, "hook_info_hashtable", API_RETURN_EMPTY);
    if (lua_gettop (L) < 6)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    info_name = lua_tostring (L, -6);
    description = lua_tostring (L, -5);
    args_description = lua_tostring (L, -4);
    output_description = lua_tostring (L, -3);
    function = lua_tostring (L, -2);
    data = lua_tostring (L, -1);

    result = API_PTR2STR(plugin_script_api_hook_info_hashtable (weechat_lua_plugin,
                                                                lua_current_script,
                                                                info_name,
                                                                description,
                                                                args_description,
                                                                output_description,
                                                                &weechat_lua_api_hook_info_hashtable_cb,
                                                                function,
                                                                data));

    API_RETURN_STRING(result);
}

struct t_infolist *
weechat_lua_api_hook_infolist_cb (const void *pointer, void *data,
                                  const char *info_name,
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
        func_argv[1] = (info_name) ? (char *)info_name : empty_arg;
        func_argv[2] = (char *)API_PTR2STR(obj_pointer);
        func_argv[3] = (arguments) ? (char *)arguments : empty_arg;

        result = (struct t_infolist *)weechat_lua_exec (
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
    const char *infolist_name, *description, *pointer_description;
    const char *args_description, *function, *data;
    const char *result;

    API_INIT_FUNC(1, "hook_infolist", API_RETURN_EMPTY);
    if (lua_gettop (L) < 6)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    infolist_name = lua_tostring (L, -6);
    description = lua_tostring (L, -5);
    pointer_description = lua_tostring (L, -4);
    args_description = lua_tostring (L, -3);
    function = lua_tostring (L, -2);
    data = lua_tostring (L, -1);

    result = API_PTR2STR(plugin_script_api_hook_infolist (weechat_lua_plugin,
                                                          lua_current_script,
                                                          infolist_name,
                                                          description,
                                                          pointer_description,
                                                          args_description,
                                                          &weechat_lua_api_hook_infolist_cb,
                                                          function,
                                                          data));

    API_RETURN_STRING(result);
}

struct t_hashtable *
weechat_lua_api_hook_focus_cb (const void *pointer, void *data,
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

        return (struct t_hashtable *)weechat_lua_exec (
            script,
            WEECHAT_SCRIPT_EXEC_HASHTABLE,
            ptr_function,
            "sh", func_argv);
    }

    return NULL;
}

API_FUNC(hook_focus)
{
    const char *area, *function, *data;
    const char *result;

    API_INIT_FUNC(1, "hook_focus", API_RETURN_EMPTY);
    if (lua_gettop (L) < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    area = lua_tostring (L, -3);
    function = lua_tostring (L, -2);
    data = lua_tostring (L, -1);

    result = API_PTR2STR(plugin_script_api_hook_focus (weechat_lua_plugin,
                                                       lua_current_script,
                                                       area,
                                                       &weechat_lua_api_hook_focus_cb,
                                                       function,
                                                       data));

    API_RETURN_STRING(result);
}

API_FUNC(hook_set)
{
    const char *hook, *property, *value;

    API_INIT_FUNC(1, "hook_set", API_RETURN_ERROR);
    if (lua_gettop (L) < 3)
        API_WRONG_ARGS(API_RETURN_ERROR);

    hook = lua_tostring (L, -3);
    property = lua_tostring (L, -2);
    value = lua_tostring (L, -1);

    weechat_hook_set (API_STR2PTR(hook), property, value);

    API_RETURN_OK;
}

API_FUNC(unhook)
{
    const char *hook;

    API_INIT_FUNC(1, "unhook", API_RETURN_ERROR);
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_ERROR);

    hook = lua_tostring (L, -1);

    weechat_unhook (API_STR2PTR(hook));

    API_RETURN_OK;
}

API_FUNC(unhook_all)
{
    API_INIT_FUNC(1, "unhook_all", API_RETURN_ERROR);

    weechat_unhook_all (lua_current_script->name);

    API_RETURN_OK;
}

int
weechat_lua_api_buffer_input_data_cb (const void *pointer, void *data,
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

        rc = (int *) weechat_lua_exec (script,
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
weechat_lua_api_buffer_close_cb (const void *pointer, void *data,
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

        rc = (int *) weechat_lua_exec (script,
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
    const char *name, *function_input, *data_input, *function_close;
    const char *data_close;
    const char *result;

    API_INIT_FUNC(1, "buffer_new", API_RETURN_EMPTY);
    if (lua_gettop (L) < 5)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    name = lua_tostring (L, -5);
    function_input = lua_tostring (L, -4);
    data_input = lua_tostring (L, -3);
    function_close = lua_tostring (L, -2);
    data_close = lua_tostring (L, -1);

    result = API_PTR2STR(plugin_script_api_buffer_new (weechat_lua_plugin,
                                                       lua_current_script,
                                                       name,
                                                       &weechat_lua_api_buffer_input_data_cb,
                                                       function_input,
                                                       data_input,
                                                       &weechat_lua_api_buffer_close_cb,
                                                       function_close,
                                                       data_close));

    API_RETURN_STRING(result);
}

API_FUNC(buffer_new_props)
{
    const char *name, *function_input, *data_input, *function_close;
    const char *data_close;
    struct t_hashtable *properties;
    const char *result;

    API_INIT_FUNC(1, "buffer_new_props", API_RETURN_EMPTY);
    if (lua_gettop (L) < 6)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    name = lua_tostring (L, -6);
    properties = weechat_lua_tohashtable (L, -5,
                                          WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                          WEECHAT_HASHTABLE_STRING,
                                          WEECHAT_HASHTABLE_STRING);
    function_input = lua_tostring (L, -4);
    data_input = lua_tostring (L, -3);
    function_close = lua_tostring (L, -2);
    data_close = lua_tostring (L, -1);

    result = API_PTR2STR(
        plugin_script_api_buffer_new_props (
            weechat_lua_plugin,
            lua_current_script,
            name,
            properties,
            &weechat_lua_api_buffer_input_data_cb,
            function_input,
            data_input,
            &weechat_lua_api_buffer_close_cb,
            function_close,
            data_close));

    weechat_hashtable_free (properties);

    API_RETURN_STRING(result);
}

API_FUNC(buffer_search)
{
    const char *plugin, *name;
    const char *result;

    API_INIT_FUNC(1, "buffer_search", API_RETURN_EMPTY);
    if (lua_gettop (L) < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    plugin = lua_tostring (L, -2);
    name = lua_tostring (L, -1);

    result = API_PTR2STR(weechat_buffer_search (plugin, name));

    API_RETURN_STRING(result);
}

API_FUNC(buffer_search_main)
{
    const char *result;

    API_INIT_FUNC(1, "buffer_search_main", API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_buffer_search_main ());

    API_RETURN_STRING(result);
}

API_FUNC(current_buffer)
{
    const char *result;

    API_INIT_FUNC(1, "current_buffer", API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_current_buffer ());

    API_RETURN_STRING(result);
}

API_FUNC(buffer_clear)
{
    const char *buffer;

    API_INIT_FUNC(1, "buffer_clear", API_RETURN_ERROR);
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_ERROR);

    buffer = lua_tostring (L, -1);

    weechat_buffer_clear (API_STR2PTR(buffer));

    API_RETURN_OK;
}

API_FUNC(buffer_close)
{
    const char *buffer;

    API_INIT_FUNC(1, "buffer_close", API_RETURN_ERROR);
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_ERROR);

    buffer = lua_tostring (L, -1);

    weechat_buffer_close (API_STR2PTR(buffer));

    API_RETURN_OK;
}

API_FUNC(buffer_merge)
{
    const char *buffer, *target_buffer;

    API_INIT_FUNC(1, "buffer_merge", API_RETURN_ERROR);
    if (lua_gettop (L) < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    buffer = lua_tostring (L, -2);
    target_buffer = lua_tostring (L, -1);

    weechat_buffer_merge (API_STR2PTR(buffer),
                          API_STR2PTR(target_buffer));

    API_RETURN_OK;
}

API_FUNC(buffer_unmerge)
{
    const char *buffer;
    int number;

    API_INIT_FUNC(1, "buffer_unmerge", API_RETURN_ERROR);
    if (lua_gettop (L) < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    buffer = lua_tostring (L, -2);
    number = lua_tonumber (L, -1);

    weechat_buffer_unmerge (API_STR2PTR(buffer), number);

    API_RETURN_OK;
}

API_FUNC(buffer_get_integer)
{
    const char *buffer, *property;
    int value;

    API_INIT_FUNC(1, "buffer_get_integer", API_RETURN_INT(-1));
    if (lua_gettop (L) < 2)
        API_WRONG_ARGS(API_RETURN_INT(-1));

    buffer = lua_tostring (L, -2);
    property = lua_tostring (L, -1);

    value = weechat_buffer_get_integer (API_STR2PTR(buffer),
                                        property);

    API_RETURN_INT(value);
}

API_FUNC(buffer_get_string)
{
    const char *buffer, *property, *result;

    API_INIT_FUNC(1, "buffer_get_string", API_RETURN_EMPTY);
    if (lua_gettop (L) < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = lua_tostring (L, -2);
    property = lua_tostring (L, -1);

    result = weechat_buffer_get_string (API_STR2PTR(buffer),
                                        property);

    API_RETURN_STRING(result);
}

API_FUNC(buffer_get_pointer)
{
    const char *buffer, *property;
    const char *result;

    API_INIT_FUNC(1, "buffer_get_pointer", API_RETURN_EMPTY);
    if (lua_gettop (L) < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = lua_tostring (L, -2);
    property = lua_tostring (L, -1);

    result = API_PTR2STR(weechat_buffer_get_pointer (API_STR2PTR(buffer),
                                                     property));

    API_RETURN_STRING(result);
}

API_FUNC(buffer_set)
{
    const char *buffer, *property, *value;

    API_INIT_FUNC(1, "buffer_set", API_RETURN_ERROR);
    if (lua_gettop (L) < 3)
        API_WRONG_ARGS(API_RETURN_ERROR);

    buffer = lua_tostring (L, -3);
    property = lua_tostring (L, -2);
    value = lua_tostring (L, -1);

    weechat_buffer_set (API_STR2PTR(buffer), property, value);

    API_RETURN_OK;
}

API_FUNC(buffer_string_replace_local_var)
{
    const char *buffer, *string;
    char *result;

    API_INIT_FUNC(1, "buffer_string_replace_local_var", API_RETURN_EMPTY);
    if (lua_gettop (L) < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = lua_tostring (L, -2);
    string = lua_tostring (L, -1);

    result = weechat_buffer_string_replace_local_var (API_STR2PTR(buffer), string);

    API_RETURN_STRING_FREE(result);
}

API_FUNC(buffer_match_list)
{
    const char *buffer, *string;
    int value;

    API_INIT_FUNC(1, "buffer_match_list", API_RETURN_INT(0));
    if (lua_gettop (L) < 2)
        API_WRONG_ARGS(API_RETURN_INT(0));

    buffer = lua_tostring (L, -2);
    string = lua_tostring (L, -1);

    value = weechat_buffer_match_list (API_STR2PTR(buffer),
                                       string);

    API_RETURN_INT(value);
}

API_FUNC(line_search_by_id)
{
    const char *buffer, *result;
    int id;

    API_INIT_FUNC(1, "line_search_by_id", API_RETURN_EMPTY);
    if (lua_gettop (L) < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = lua_tostring (L, -2);
    id = lua_tonumber (L, -1);

    result = API_PTR2STR(weechat_line_search_by_id (API_STR2PTR(buffer), id));

    API_RETURN_STRING(result);
}

API_FUNC(current_window)
{
    const char *result;

    API_INIT_FUNC(1, "current_window", API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_current_window ());

    API_RETURN_STRING(result);
}

API_FUNC(window_search_with_buffer)
{
    const char *buffer;
    const char *result;

    API_INIT_FUNC(1, "window_search_with_buffer", API_RETURN_EMPTY);
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = lua_tostring (L, -1);

    result = API_PTR2STR(weechat_window_search_with_buffer (API_STR2PTR(buffer)));

    API_RETURN_STRING(result);
}

API_FUNC(window_get_integer)
{
    const char *window, *property;
    int value;

    API_INIT_FUNC(1, "window_get_integer", API_RETURN_INT(-1));
    if (lua_gettop (L) < 2)
        API_WRONG_ARGS(API_RETURN_INT(-1));

    window = lua_tostring (L, -2);
    property = lua_tostring (L, -1);

    value = weechat_window_get_integer (API_STR2PTR(window),
                                        property);

    API_RETURN_INT(value);
}

API_FUNC(window_get_string)
{
    const char *window, *property, *result;

    API_INIT_FUNC(1, "window_get_string", API_RETURN_EMPTY);
    if (lua_gettop (L) < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    window = lua_tostring (L, -2);
    property = lua_tostring (L, -1);

    result = weechat_window_get_string (API_STR2PTR(window),
                                        property);

    API_RETURN_STRING(result);
}

API_FUNC(window_get_pointer)
{
    const char *window, *property;
    const char *result;

    API_INIT_FUNC(1, "window_get_pointer", API_RETURN_EMPTY);
    if (lua_gettop (L) < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    window = lua_tostring (L, -2);
    property = lua_tostring (L, -1);

    result = API_PTR2STR(weechat_window_get_pointer (API_STR2PTR(window),
                                                     property));

    API_RETURN_STRING(result);
}

API_FUNC(window_set_title)
{
    const char *title;

    API_INIT_FUNC(1, "window_set_title", API_RETURN_ERROR);
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_ERROR);

    title = lua_tostring (L, -1);

    weechat_window_set_title (title);

    API_RETURN_OK;
}

API_FUNC(nicklist_add_group)
{
    const char *buffer, *parent_group, *name, *color;
    const char *result;
    int visible;

    API_INIT_FUNC(1, "nicklist_add_group", API_RETURN_EMPTY);
    if (lua_gettop (L) < 5)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = lua_tostring (L, -5);
    parent_group = lua_tostring (L, -4);
    name = lua_tostring (L, -3);
    color = lua_tostring (L, -2);
    visible = lua_tonumber (L, -1);

    result = API_PTR2STR(weechat_nicklist_add_group (API_STR2PTR(buffer),
                                                     API_STR2PTR(parent_group),
                                                     name,
                                                     color,
                                                     visible));

    API_RETURN_STRING(result);
}

API_FUNC(nicklist_search_group)
{
    const char *buffer, *from_group, *name;
    const char *result;

    API_INIT_FUNC(1, "nicklist_search_group", API_RETURN_EMPTY);
    if (lua_gettop (L) < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = lua_tostring (L, -3);
    from_group = lua_tostring (L, -2);
    name = lua_tostring (L, -1);

    result = API_PTR2STR(weechat_nicklist_search_group (API_STR2PTR(buffer),
                                                        API_STR2PTR(from_group),
                                                        name));

    API_RETURN_STRING(result);
}

API_FUNC(nicklist_add_nick)
{
    const char *buffer, *group, *name, *color, *prefix, *prefix_color;
    const char *result;
    int visible;

    API_INIT_FUNC(1, "nicklist_add_nick", API_RETURN_EMPTY);
    if (lua_gettop (L) < 7)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = lua_tostring (L, -7);
    group = lua_tostring (L, -6);
    name = lua_tostring (L, -5);
    color = lua_tostring (L, -4);
    prefix = lua_tostring (L, -3);
    prefix_color = lua_tostring (L, -2);
    visible = lua_tonumber (L, -1);

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
    const char *buffer, *from_group, *name;
    const char *result;

    API_INIT_FUNC(1, "nicklist_search_nick", API_RETURN_EMPTY);
    if (lua_gettop (L) < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = lua_tostring (L, -3);
    from_group = lua_tostring (L, -2);
    name = lua_tostring (L, -1);

    result = API_PTR2STR(weechat_nicklist_search_nick (API_STR2PTR(buffer),
                                                       API_STR2PTR(from_group),
                                                       name));

    API_RETURN_STRING(result);
}

API_FUNC(nicklist_remove_group)
{
    const char *buffer, *group;

    API_INIT_FUNC(1, "nicklist_remove_group", API_RETURN_ERROR);
    if (lua_gettop (L) < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    buffer = lua_tostring (L, -2);
    group = lua_tostring (L, -1);

    weechat_nicklist_remove_group (API_STR2PTR(buffer),
                                   API_STR2PTR(group));

    API_RETURN_OK;
}

API_FUNC(nicklist_remove_nick)
{
    const char *buffer, *nick;

    API_INIT_FUNC(1, "nicklist_remove_nick", API_RETURN_ERROR);
    if (lua_gettop (L) < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    buffer = lua_tostring (L, -2);
    nick = lua_tostring (L, -1);

    weechat_nicklist_remove_nick (API_STR2PTR(buffer),
                                  API_STR2PTR(nick));

    API_RETURN_OK;
}

API_FUNC(nicklist_remove_all)
{
    const char *buffer;

    API_INIT_FUNC(1, "nicklist_remove_all", API_RETURN_ERROR);
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_ERROR);

    buffer = lua_tostring (L, -1);

    weechat_nicklist_remove_all (API_STR2PTR(buffer));

    API_RETURN_OK;
}

API_FUNC(nicklist_group_get_integer)
{
    const char *buffer, *group, *property;
    int value;

    API_INIT_FUNC(1, "nicklist_group_get_integer", API_RETURN_INT(-1));
    if (lua_gettop (L) < 3)
        API_WRONG_ARGS(API_RETURN_INT(-1));

    buffer = lua_tostring (L, -3);
    group = lua_tostring (L, -2);
    property = lua_tostring (L, -1);

    value = weechat_nicklist_group_get_integer (API_STR2PTR(buffer),
                                                API_STR2PTR(group),
                                                property);

    API_RETURN_INT(value);
}

API_FUNC(nicklist_group_get_string)
{
    const char *buffer, *group, *property, *result;

    API_INIT_FUNC(1, "nicklist_group_get_string", API_RETURN_EMPTY);
    if (lua_gettop (L) < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = lua_tostring (L, -3);
    group = lua_tostring (L, -2);
    property = lua_tostring (L, -1);

    result = weechat_nicklist_group_get_string (API_STR2PTR(buffer),
                                                API_STR2PTR(group),
                                                property);

    API_RETURN_STRING(result);
}

API_FUNC(nicklist_group_get_pointer)
{
    const char *buffer, *group, *property;
    const char *result;

    API_INIT_FUNC(1, "nicklist_group_get_pointer", API_RETURN_EMPTY);
    if (lua_gettop (L) < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = lua_tostring (L, -3);
    group = lua_tostring (L, -2);
    property = lua_tostring (L, -1);

    result = API_PTR2STR(weechat_nicklist_group_get_pointer (API_STR2PTR(buffer),
                                                             API_STR2PTR(group),
                                                             property));

    API_RETURN_STRING(result);
}

API_FUNC(nicklist_group_set)
{
    const char *buffer, *group, *property, *value;

    API_INIT_FUNC(1, "nicklist_group_set", API_RETURN_ERROR);
    if (lua_gettop (L) < 4)
        API_WRONG_ARGS(API_RETURN_ERROR);

    buffer = lua_tostring (L, -4);
    group = lua_tostring (L, -3);
    property = lua_tostring (L, -2);
    value = lua_tostring (L, -1);

    weechat_nicklist_group_set (API_STR2PTR(buffer),
                                API_STR2PTR(group),
                                property,
                                value);

    API_RETURN_OK;
}

API_FUNC(nicklist_nick_get_integer)
{
    const char *buffer, *nick, *property;
    int value;

    API_INIT_FUNC(1, "nicklist_nick_get_integer", API_RETURN_INT(-1));
    if (lua_gettop (L) < 3)
        API_WRONG_ARGS(API_RETURN_INT(-1));

    buffer = lua_tostring (L, -3);
    nick = lua_tostring (L, -2);
    property = lua_tostring (L, -1);

    value = weechat_nicklist_nick_get_integer (API_STR2PTR(buffer),
                                               API_STR2PTR(nick),
                                               property);

    API_RETURN_INT(value);
}

API_FUNC(nicklist_nick_get_string)
{
    const char *buffer, *nick, *property, *result;

    API_INIT_FUNC(1, "nicklist_nick_get_string", API_RETURN_EMPTY);
    if (lua_gettop (L) < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = lua_tostring (L, -3);
    nick = lua_tostring (L, -2);
    property = lua_tostring (L, -1);

    result = weechat_nicklist_nick_get_string (API_STR2PTR(buffer),
                                               API_STR2PTR(nick),
                                               property);

    API_RETURN_STRING(result);
}

API_FUNC(nicklist_nick_get_pointer)
{
    const char *buffer, *nick, *property;
    const char *result;

    API_INIT_FUNC(1, "nicklist_nick_get_pointer", API_RETURN_EMPTY);
    if (lua_gettop (L) < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = lua_tostring (L, -3);
    nick = lua_tostring (L, -2);
    property = lua_tostring (L, -1);

    result = API_PTR2STR(weechat_nicklist_nick_get_pointer (API_STR2PTR(buffer),
                                                            API_STR2PTR(nick),
                                                            property));

    API_RETURN_STRING(result);
}

API_FUNC(nicklist_nick_set)
{
    const char *buffer, *nick, *property, *value;

    API_INIT_FUNC(1, "nicklist_nick_set", API_RETURN_ERROR);
    if (lua_gettop (L) < 4)
        API_WRONG_ARGS(API_RETURN_ERROR);

    buffer = lua_tostring (L, -4);
    nick = lua_tostring (L, -3);
    property = lua_tostring (L, -2);
    value = lua_tostring (L, -1);

    weechat_nicklist_nick_set (API_STR2PTR(buffer),
                               API_STR2PTR(nick),
                               property,
                               value);

    API_RETURN_OK;
}

API_FUNC(bar_item_search)
{
    const char *name;
    const char *result;

    API_INIT_FUNC(1, "bar_item_search", API_RETURN_EMPTY);
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    name = lua_tostring (L, -1);

    result = API_PTR2STR(weechat_bar_item_search (name));

    API_RETURN_STRING(result);
}

char *
weechat_lua_api_bar_item_build_cb (const void *pointer, void *data,
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

            ret = (char *)weechat_lua_exec (script,
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

            ret = (char *)weechat_lua_exec (script,
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
    const char *name, *function, *data;
    const char *result;

    API_INIT_FUNC(1, "bar_item_new", API_RETURN_EMPTY);
    if (lua_gettop (L) < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    name = lua_tostring (L, -3);
    function = lua_tostring (L, -2);
    data = lua_tostring (L, -1);

    result = API_PTR2STR(plugin_script_api_bar_item_new (weechat_lua_plugin,
                                                         lua_current_script,
                                                         name,
                                                         &weechat_lua_api_bar_item_build_cb,
                                                         function,
                                                         data));

    API_RETURN_STRING(result);
}

API_FUNC(bar_item_update)
{
    const char *name;

    API_INIT_FUNC(1, "bar_item_update", API_RETURN_ERROR);
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_ERROR);

    name = lua_tostring (L, -1);

    weechat_bar_item_update (name);

    API_RETURN_OK;
}

API_FUNC(bar_item_remove)
{
    const char *item;

    API_INIT_FUNC(1, "bar_item_remove", API_RETURN_ERROR);
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_ERROR);

    item = lua_tostring (L, -1);

    weechat_bar_item_remove (API_STR2PTR(item));

    API_RETURN_OK;
}

API_FUNC(bar_search)
{
    const char *name;
    const char *result;

    API_INIT_FUNC(1, "bar_search", API_RETURN_EMPTY);
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    name = lua_tostring (L, -1);

    result = API_PTR2STR(weechat_bar_search (name));

    API_RETURN_STRING(result);
}

API_FUNC(bar_new)
{
    const char *name, *hidden, *priority, *type, *conditions, *position;
    const char *filling_top_bottom, *filling_left_right, *size, *size_max;
    const char *color_fg, *color_delim, *color_bg, *color_bg_inactive;
    const char *separator, *items;
    const char *result;

    API_INIT_FUNC(1, "bar_new", API_RETURN_EMPTY);
    if (lua_gettop (L) < 16)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    name = lua_tostring (L, -16);
    hidden = lua_tostring (L, -15);
    priority = lua_tostring (L, -14);
    type = lua_tostring (L, -13);
    conditions = lua_tostring (L, -12);
    position = lua_tostring (L, -11);
    filling_top_bottom = lua_tostring (L, -10);
    filling_left_right = lua_tostring (L, -9);
    size = lua_tostring (L, -8);
    size_max = lua_tostring (L, -7);
    color_fg = lua_tostring (L, -6);
    color_delim = lua_tostring (L, -5);
    color_bg = lua_tostring (L, -4);
    color_bg_inactive = lua_tostring (L, -3);
    separator = lua_tostring (L, -2);
    items = lua_tostring (L, -1);

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
    const char *bar, *property, *value;
    int rc;

    API_INIT_FUNC(1, "bar_set", API_RETURN_INT(0));
    if (lua_gettop (L) < 3)
        API_WRONG_ARGS(API_RETURN_INT(0));

    bar = lua_tostring (L, -3);
    property = lua_tostring (L, -2);
    value = lua_tostring (L, -1);

    rc = weechat_bar_set (API_STR2PTR(bar), property, value);

    API_RETURN_INT(rc);
}

API_FUNC(bar_update)
{
    const char *name;

    API_INIT_FUNC(1, "bar_update", API_RETURN_ERROR);
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_ERROR);

    name = lua_tostring (L, -1);

    weechat_bar_update (name);

    API_RETURN_OK;
}

API_FUNC(bar_remove)
{
    const char *bar;

    API_INIT_FUNC(1, "bar_remove", API_RETURN_ERROR);
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_ERROR);

    bar = lua_tostring (L, -1);

    weechat_bar_remove (API_STR2PTR(bar));

    API_RETURN_OK;
}

API_FUNC(command)
{
    const char *buffer, *command;
    int rc;

    API_INIT_FUNC(1, "command", API_RETURN_INT(WEECHAT_RC_ERROR));
    if (lua_gettop (L) < 2)
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_RC_ERROR));

    buffer = lua_tostring (L, -2);
    command = lua_tostring (L, -1);

    rc = plugin_script_api_command (weechat_lua_plugin,
                                    lua_current_script,
                                    API_STR2PTR(buffer),
                                    command);

    API_RETURN_INT(rc);
}

API_FUNC(command_options)
{
    const char *buffer, *command;
    struct t_hashtable *options;
    int rc;

    API_INIT_FUNC(1, "command_options", API_RETURN_INT(WEECHAT_RC_ERROR));
    if (lua_gettop (L) < 3)
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_RC_ERROR));

    buffer = lua_tostring (L, -3);
    command = lua_tostring (L, -2);
    options = weechat_lua_tohashtable (L, -1,
                                       WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                       WEECHAT_HASHTABLE_STRING,
                                       WEECHAT_HASHTABLE_STRING);

    rc = plugin_script_api_command_options (weechat_lua_plugin,
                                            lua_current_script,
                                            API_STR2PTR(buffer),
                                            command,
                                            options);

    weechat_hashtable_free (options);

    API_RETURN_INT(rc);
}

API_FUNC(completion_new)
{
    const char *buffer;
    const char *result;

    API_INIT_FUNC(1, "completion_new", API_RETURN_EMPTY);
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = lua_tostring (L, -1);

    result = API_PTR2STR(weechat_completion_new (API_STR2PTR(buffer)));

    API_RETURN_STRING(result);
}

API_FUNC(completion_search)
{
    const char *completion, *data;
    int position, direction, rc;

    API_INIT_FUNC(1, "completion_search", API_RETURN_INT(0));
    if (lua_gettop (L) < 4)
        API_WRONG_ARGS(API_RETURN_INT(0));

    completion = lua_tostring (L, -4);
    data = lua_tostring (L, -3);
    position = lua_tonumber (L, -2);
    direction = lua_tonumber (L, -1);

    rc = weechat_completion_search (API_STR2PTR(completion), data, position,
                                    direction);

    API_RETURN_INT(rc);
}

API_FUNC(completion_get_string)
{
    const char *completion, *property, *result;

    API_INIT_FUNC(1, "completion_get_string", API_RETURN_EMPTY);
    if (lua_gettop (L) < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    completion = lua_tostring (L, -2);
    property = lua_tostring (L, -1);

    result = weechat_completion_get_string (API_STR2PTR(completion),
                                            property);

    API_RETURN_STRING(result);
}

API_FUNC(completion_set)
{
    const char *completion, *property, *value;

    API_INIT_FUNC(1, "completion_set", API_RETURN_ERROR);
    if (lua_gettop (L) < 3)
        API_WRONG_ARGS(API_RETURN_ERROR);

    completion = lua_tostring (L, -3);
    property = lua_tostring (L, -2);
    value = lua_tostring (L, -1);

    weechat_completion_set (API_STR2PTR(completion), property, value);

    API_RETURN_OK;
}

API_FUNC(completion_list_add)
{
    const char *completion, *word, *where;
    int nick_completion;

    API_INIT_FUNC(1, "completion_list_add", API_RETURN_ERROR);
    if (lua_gettop (L) < 4)
        API_WRONG_ARGS(API_RETURN_ERROR);

    completion = lua_tostring (L, -4);
    word = lua_tostring (L, -3);
    nick_completion = lua_tonumber (L, -2);
    where = lua_tostring (L, -1);

    weechat_completion_list_add (API_STR2PTR(completion),
                                 word,
                                 nick_completion,
                                 where);

    API_RETURN_OK;
}

API_FUNC(completion_free)
{
    const char *completion;

    API_INIT_FUNC(1, "completion_free", API_RETURN_ERROR);
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_ERROR);

    completion = lua_tostring (L, -1);

    weechat_completion_free (API_STR2PTR(completion));

    API_RETURN_OK;
}

API_FUNC(info_get)
{
    const char *info_name, *arguments;
    char *result;

    API_INIT_FUNC(1, "info_get", API_RETURN_EMPTY);
    if (lua_gettop (L) < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    info_name = lua_tostring (L, -2);
    arguments = lua_tostring (L, -1);

    result = weechat_info_get (info_name, arguments);

    API_RETURN_STRING_FREE(result);
}

API_FUNC(info_get_hashtable)
{
    const char *info_name;
    struct t_hashtable *table, *result_hashtable;

    API_INIT_FUNC(1, "info_get_hashtable", API_RETURN_EMPTY);
    if (lua_gettop (L) < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    info_name = lua_tostring (L, -2);
    table = weechat_lua_tohashtable (L, -1,
                                     WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                     WEECHAT_HASHTABLE_STRING,
                                     WEECHAT_HASHTABLE_STRING);

    result_hashtable = weechat_info_get_hashtable (info_name, table);

    weechat_lua_pushhashtable (L, result_hashtable);

    weechat_hashtable_free (table);
    weechat_hashtable_free (result_hashtable);

    return 1;
}

API_FUNC(infolist_new)
{
    const char *result;

    API_INIT_FUNC(1, "infolist_new", API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_infolist_new ());

    API_RETURN_STRING(result);
}

API_FUNC(infolist_new_item)
{
    const char *infolist;
    const char *result;

    API_INIT_FUNC(1, "infolist_new_item", API_RETURN_EMPTY);
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    infolist = lua_tostring (L, -1);

    result = API_PTR2STR(weechat_infolist_new_item (API_STR2PTR(infolist)));

    API_RETURN_STRING(result);
}

API_FUNC(infolist_new_var_integer)
{
    const char *item, *name;
    const char *result;
    int value;

    API_INIT_FUNC(1, "infolist_new_var_integer", API_RETURN_EMPTY);
    if (lua_gettop (L) < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    item = lua_tostring (L, -3);
    name = lua_tostring (L, -2);
    value = lua_tonumber (L, -1);

    result = API_PTR2STR(weechat_infolist_new_var_integer (API_STR2PTR(item),
                                                           name,
                                                           value));

    API_RETURN_STRING(result);
}

API_FUNC(infolist_new_var_string)
{
    const char *item, *name, *value;
    const char *result;

    API_INIT_FUNC(1, "infolist_new_var_string", API_RETURN_EMPTY);
    if (lua_gettop (L) < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    item = lua_tostring (L, -3);
    name = lua_tostring (L, -2);
    value = lua_tostring (L, -1);

    result = API_PTR2STR(weechat_infolist_new_var_string (API_STR2PTR(item),
                                                          name,
                                                          value));

    API_RETURN_STRING(result);
}

API_FUNC(infolist_new_var_pointer)
{
    const char *item, *name, *value;
    const char *result;

    API_INIT_FUNC(1, "infolist_new_var_pointer", API_RETURN_EMPTY);
    if (lua_gettop (L) < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    item = lua_tostring (L, -3);
    name = lua_tostring (L, -2);
    value = lua_tostring (L, -1);

    result = API_PTR2STR(weechat_infolist_new_var_pointer (API_STR2PTR(item),
                                                           name,
                                                           API_STR2PTR(value)));

    API_RETURN_STRING(result);
}

API_FUNC(infolist_new_var_time)
{
    const char *item, *name;
    const char *result;
    long value;

    API_INIT_FUNC(1, "infolist_new_var_time", API_RETURN_EMPTY);
    if (lua_gettop (L) < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    item = lua_tostring (L, -3);
    name = lua_tostring (L, -2);
    value = lua_tonumber (L, -1);

    result = API_PTR2STR(weechat_infolist_new_var_time (API_STR2PTR(item),
                                                        name,
                                                        (time_t)value));

    API_RETURN_STRING(result);
}

API_FUNC(infolist_search_var)
{
    const char *infolist, *name;
    const char *result;

    API_INIT_FUNC(1, "infolist_search_var", API_RETURN_EMPTY);
    if (lua_gettop (L) < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    infolist = lua_tostring (L, -2);
    name = lua_tostring (L, -1);

    result = API_PTR2STR(weechat_infolist_search_var (API_STR2PTR(infolist),
                                                      name));

    API_RETURN_STRING(result);
}

API_FUNC(infolist_get)
{
    const char *name, *pointer, *arguments;
    const char *result;

    API_INIT_FUNC(1, "infolist_get", API_RETURN_EMPTY);
    if (lua_gettop (L) < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    name = lua_tostring (L, -3);
    pointer = lua_tostring (L, -2);
    arguments = lua_tostring (L, -1);

    result = API_PTR2STR(weechat_infolist_get (name,
                                               API_STR2PTR(pointer),
                                               arguments));

    API_RETURN_STRING(result);
}

API_FUNC(infolist_next)
{
    const char *infolist;
    int value;

    API_INIT_FUNC(1, "infolist_next", API_RETURN_INT(0));
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_INT(0));

    infolist = lua_tostring (L, -1);

    value = weechat_infolist_next (API_STR2PTR(infolist));

    API_RETURN_INT(value);
}

API_FUNC(infolist_prev)
{
    const char *infolist;
    int value;

    API_INIT_FUNC(1, "infolist_prev", API_RETURN_INT(0));
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_INT(0));

    infolist = lua_tostring (L, -1);

    value = weechat_infolist_prev (API_STR2PTR(infolist));

    API_RETURN_INT(value);
}

API_FUNC(infolist_reset_item_cursor)
{
    const char *infolist;

    API_INIT_FUNC(1, "infolist_reset_item_cursor", API_RETURN_ERROR);
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_ERROR);

    infolist = lua_tostring (L, -1);

    weechat_infolist_reset_item_cursor (API_STR2PTR(infolist));

    API_RETURN_OK;
}

API_FUNC(infolist_fields)
{
    const char *infolist, *result;

    API_INIT_FUNC(1, "infolist_fields", API_RETURN_EMPTY);
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    infolist = lua_tostring (L, -1);

    result = weechat_infolist_fields (API_STR2PTR(infolist));

    API_RETURN_STRING(result);
}

API_FUNC(infolist_integer)
{
    const char *infolist, *variable;
    int value;

    API_INIT_FUNC(1, "infolist_integer", API_RETURN_INT(0));
    if (lua_gettop (L) < 2)
        API_WRONG_ARGS(API_RETURN_INT(0));

    infolist = lua_tostring (L, -2);
    variable = lua_tostring (L, -1);

    value = weechat_infolist_integer (API_STR2PTR(infolist),
                                      variable);

    API_RETURN_INT(value);
}

API_FUNC(infolist_string)
{
    const char *infolist, *variable, *result;

    API_INIT_FUNC(1, "infolist_string", API_RETURN_EMPTY);
    if (lua_gettop (L) < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    infolist = lua_tostring (L, -2);
    variable = lua_tostring (L, -1);

    result = weechat_infolist_string (API_STR2PTR(infolist),
                                     variable);

    API_RETURN_STRING(result);
}

API_FUNC(infolist_pointer)
{
    const char *infolist, *variable;
    const char *result;

    API_INIT_FUNC(1, "infolist_pointer", API_RETURN_EMPTY);
    if (lua_gettop (L) < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    infolist = lua_tostring (L, -2);
    variable = lua_tostring (L, -1);

    result = API_PTR2STR(weechat_infolist_pointer (API_STR2PTR(infolist),
                                                   variable));

    API_RETURN_STRING(result);
}

API_FUNC(infolist_time)
{
    const char *infolist, *variable;
    time_t time;

    API_INIT_FUNC(1, "infolist_time", API_RETURN_LONG(0));
    if (lua_gettop (L) < 2)
        API_WRONG_ARGS(API_RETURN_LONG(0));

    infolist = lua_tostring (L, -2);
    variable = lua_tostring (L, -1);

    time = weechat_infolist_time (API_STR2PTR(infolist), variable);

    API_RETURN_LONG(time);
}

API_FUNC(infolist_free)
{
    const char *infolist;

    API_INIT_FUNC(1, "infolist_free", API_RETURN_ERROR);
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_ERROR);

    infolist = lua_tostring (L, -1);

    weechat_infolist_free (API_STR2PTR(infolist));

    API_RETURN_OK;
}

API_FUNC(hdata_get)
{
    const char *name;
    const char *result;

    API_INIT_FUNC(1, "hdata_get", API_RETURN_EMPTY);
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    name = lua_tostring (L, -1);

    result = API_PTR2STR(weechat_hdata_get (name));

    API_RETURN_STRING(result);
}

API_FUNC(hdata_get_var_offset)
{
    const char *hdata, *name;
    int value;

    API_INIT_FUNC(1, "hdata_get_var_offset", API_RETURN_INT(0));
    if (lua_gettop (L) < 2)
        API_WRONG_ARGS(API_RETURN_INT(0));

    hdata = lua_tostring (L, -2);
    name = lua_tostring (L, -1);

    value = weechat_hdata_get_var_offset (API_STR2PTR(hdata), name);

    API_RETURN_INT(value);
}

API_FUNC(hdata_get_var_type_string)
{
    const char *hdata, *name, *result;

    API_INIT_FUNC(1, "hdata_get_var_type_string", API_RETURN_EMPTY);
    if (lua_gettop (L) < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    hdata = lua_tostring (L, -2);
    name = lua_tostring (L, -1);

    result = weechat_hdata_get_var_type_string (API_STR2PTR(hdata), name);

    API_RETURN_STRING(result);
}

API_FUNC(hdata_get_var_array_size)
{
    const char *hdata, *pointer, *name;
    int value;

    API_INIT_FUNC(1, "hdata_get_var_array_size", API_RETURN_INT(-1));
    if (lua_gettop (L) < 3)
        API_WRONG_ARGS(API_RETURN_INT(-1));

    hdata = lua_tostring (L, -3);
    pointer = lua_tostring (L, -2);
    name = lua_tostring (L, -1);

    value = weechat_hdata_get_var_array_size (API_STR2PTR(hdata),
                                              API_STR2PTR(pointer),
                                              name);

    API_RETURN_INT(value);
}

API_FUNC(hdata_get_var_array_size_string)
{
    const char *hdata, *pointer, *name, *result;

    API_INIT_FUNC(1, "hdata_get_var_array_size_string", API_RETURN_EMPTY);
    if (lua_gettop (L) < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    hdata = lua_tostring (L, -3);
    pointer = lua_tostring (L, -2);
    name = lua_tostring (L, -1);

    result = weechat_hdata_get_var_array_size_string (API_STR2PTR(hdata),
                                                      API_STR2PTR(pointer),
                                                      name);

    API_RETURN_STRING(result);
}

API_FUNC(hdata_get_var_hdata)
{
    const char *hdata, *name, *result;

    API_INIT_FUNC(1, "hdata_get_var_hdata", API_RETURN_EMPTY);
    if (lua_gettop (L) < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    hdata = lua_tostring (L, -2);
    name = lua_tostring (L, -1);

    result = weechat_hdata_get_var_hdata (API_STR2PTR(hdata), name);

    API_RETURN_STRING(result);
}

API_FUNC(hdata_get_list)
{
    const char *hdata, *name;
    const char *result;

    API_INIT_FUNC(1, "hdata_get_list", API_RETURN_EMPTY);
    if (lua_gettop (L) < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    hdata = lua_tostring (L, -2);
    name = lua_tostring (L, -1);

    result = API_PTR2STR(weechat_hdata_get_list (API_STR2PTR(hdata),
                                                 name));

    API_RETURN_STRING(result);
}

API_FUNC(hdata_check_pointer)
{
    const char *hdata, *list, *pointer;
    int value;

    API_INIT_FUNC(1, "hdata_check_pointer", API_RETURN_INT(0));
    if (lua_gettop (L) < 3)
        API_WRONG_ARGS(API_RETURN_INT(0));

    hdata = lua_tostring (L, -3);
    list = lua_tostring (L, -2);
    pointer = lua_tostring (L, -1);

    value = weechat_hdata_check_pointer (API_STR2PTR(hdata),
                                         API_STR2PTR(list),
                                         API_STR2PTR(pointer));

    API_RETURN_INT(value);
}

API_FUNC(hdata_move)
{
    const char *hdata, *pointer;
    const char *result;
    int count;

    API_INIT_FUNC(1, "hdata_move", API_RETURN_EMPTY);
    if (lua_gettop (L) < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    hdata = lua_tostring (L, -3);
    pointer = lua_tostring (L, -2);
    count = lua_tonumber (L, -1);

    result = API_PTR2STR(weechat_hdata_move (API_STR2PTR(hdata),
                                             API_STR2PTR(pointer),
                                             count));

    API_RETURN_STRING(result);
}

API_FUNC(hdata_search)
{
    const char *hdata, *pointer, *search;
    const char *result;
    struct t_hashtable *pointers, *extra_vars, *options;
    int move;

    API_INIT_FUNC(1, "hdata_search", API_RETURN_EMPTY);
    if (lua_gettop (L) < 7)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    hdata = lua_tostring (L, -7);
    pointer = lua_tostring (L, -6);
    search = lua_tostring (L, -5);
    pointers = weechat_lua_tohashtable (L, -4,
                                        WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                        WEECHAT_HASHTABLE_STRING,
                                        WEECHAT_HASHTABLE_POINTER);
    extra_vars = weechat_lua_tohashtable (L, -3,
                                          WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                          WEECHAT_HASHTABLE_STRING,
                                          WEECHAT_HASHTABLE_STRING);
    options = weechat_lua_tohashtable (L, -2,
                                       WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                       WEECHAT_HASHTABLE_STRING,
                                       WEECHAT_HASHTABLE_STRING);
    move = lua_tonumber (L, -1);

    result = API_PTR2STR(weechat_hdata_search (API_STR2PTR(hdata),
                                               API_STR2PTR(pointer),
                                               search,
                                               pointers,
                                               extra_vars,
                                               options,
                                               move));

    weechat_hashtable_free (pointers);
    weechat_hashtable_free (extra_vars);
    weechat_hashtable_free (options);

    API_RETURN_STRING(result);
}

API_FUNC(hdata_char)
{
    const char *hdata, *pointer, *name;
    int value;

    API_INIT_FUNC(1, "hdata_char", API_RETURN_INT(0));
    if (lua_gettop (L) < 3)
        API_WRONG_ARGS(API_RETURN_INT(0));

    hdata = lua_tostring (L, -3);
    pointer = lua_tostring (L, -2);
    name = lua_tostring (L, -1);

    value = (int)weechat_hdata_char (API_STR2PTR(hdata),
                                     API_STR2PTR(pointer),
                                     name);

    API_RETURN_INT(value);
}

API_FUNC(hdata_integer)
{
    const char *hdata, *pointer, *name;
    int value;

    API_INIT_FUNC(1, "hdata_integer", API_RETURN_INT(0));
    if (lua_gettop (L) < 3)
        API_WRONG_ARGS(API_RETURN_INT(0));

    hdata = lua_tostring (L, -3);
    pointer = lua_tostring (L, -2);
    name = lua_tostring (L, -1);

    value = weechat_hdata_integer (API_STR2PTR(hdata),
                                   API_STR2PTR(pointer),
                                   name);

    API_RETURN_INT(value);
}

API_FUNC(hdata_long)
{
    const char *hdata, *pointer, *name;
    long value;

    API_INIT_FUNC(1, "hdata_long", API_RETURN_LONG(0));
    if (lua_gettop (L) < 3)
        API_WRONG_ARGS(API_RETURN_LONG(0));

    hdata = lua_tostring (L, -3);
    pointer = lua_tostring (L, -2);
    name = lua_tostring (L, -1);

    value = weechat_hdata_long (API_STR2PTR(hdata),
                                API_STR2PTR(pointer),
                                name);

    API_RETURN_LONG(value);
}

API_FUNC(hdata_longlong)
{
    const char *hdata, *pointer, *name;
    long long value;

    API_INIT_FUNC(1, "hdata_longlong", API_RETURN_LONG(0));
    if (lua_gettop (L) < 3)
        API_WRONG_ARGS(API_RETURN_LONG(0));

    hdata = lua_tostring (L, -3);
    pointer = lua_tostring (L, -2);
    name = lua_tostring (L, -1);

    value = weechat_hdata_longlong (API_STR2PTR(hdata),
                                    API_STR2PTR(pointer),
                                    name);

    API_RETURN_LONG(value);
}

API_FUNC(hdata_string)
{
    const char *hdata, *pointer, *name, *result;

    API_INIT_FUNC(1, "hdata_string", API_RETURN_EMPTY);
    if (lua_gettop (L) < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    hdata = lua_tostring (L, -3);
    pointer = lua_tostring (L, -2);
    name = lua_tostring (L, -1);

    result = weechat_hdata_string (API_STR2PTR(hdata),
                                   API_STR2PTR(pointer),
                                   name);

    API_RETURN_STRING(result);
}

API_FUNC(hdata_pointer)
{
    const char *hdata, *pointer, *name;
    const char *result;

    API_INIT_FUNC(1, "hdata_pointer", API_RETURN_EMPTY);
    if (lua_gettop (L) < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    hdata = lua_tostring (L, -3);
    pointer = lua_tostring (L, -2);
    name = lua_tostring (L, -1);

    result = API_PTR2STR(weechat_hdata_pointer (API_STR2PTR(hdata),
                                                API_STR2PTR(pointer),
                                                name));

    API_RETURN_STRING(result);
}

API_FUNC(hdata_time)
{
    const char *hdata, *pointer, *name;
    time_t time;

    API_INIT_FUNC(1, "hdata_time", API_RETURN_LONG(0));
    if (lua_gettop (L) < 3)
        API_WRONG_ARGS(API_RETURN_LONG(0));

    hdata = lua_tostring (L, -3);
    pointer = lua_tostring (L, -2);
    name = lua_tostring (L, -1);

    time = weechat_hdata_time (API_STR2PTR(hdata),
                               API_STR2PTR(pointer),
                               name);

    API_RETURN_LONG(time);
}

API_FUNC(hdata_hashtable)
{
    const char *hdata, *pointer, *name;

    API_INIT_FUNC(1, "hdata_hashtable", API_RETURN_EMPTY);
    if (lua_gettop (L) < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    hdata = lua_tostring (L, -3);
    pointer = lua_tostring (L, -2);
    name = lua_tostring (L, -1);

    weechat_lua_pushhashtable (L,
                               weechat_hdata_hashtable (API_STR2PTR(hdata),
                                                        API_STR2PTR(pointer),
                                                        name));

    return 1;
}

API_FUNC(hdata_compare)
{
    const char *hdata, *pointer1, *pointer2, *name;
    int case_sensitive, rc;

    API_INIT_FUNC(1, "hdata_compare", API_RETURN_INT(0));
    if (lua_gettop (L) < 5)
        API_WRONG_ARGS(API_RETURN_INT(0));

    hdata = lua_tostring (L, -5);
    pointer1 = lua_tostring (L, -4);
    pointer2 = lua_tostring (L, -3);
    name = lua_tostring (L, -2);
    case_sensitive = lua_tonumber (L, -1);

    rc = weechat_hdata_compare (API_STR2PTR(hdata),
                                API_STR2PTR(pointer1),
                                API_STR2PTR(pointer2),
                                name,
                                case_sensitive);

    API_RETURN_INT(rc);
}

API_FUNC(hdata_update)
{
    const char *hdata, *pointer;
    struct t_hashtable *hashtable;
    int value;

    API_INIT_FUNC(1, "hdata_update", API_RETURN_INT(0));
    if (lua_gettop (L) < 3)
        API_WRONG_ARGS(API_RETURN_INT(0));

    hdata = lua_tostring (L, -3);
    pointer = lua_tostring (L, -2);
    hashtable = weechat_lua_tohashtable (L, -1,
                                         WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                         WEECHAT_HASHTABLE_STRING,
                                         WEECHAT_HASHTABLE_STRING);

    value = weechat_hdata_update (API_STR2PTR(hdata),
                                  API_STR2PTR(pointer),
                                  hashtable);

    weechat_hashtable_free (hashtable);

    API_RETURN_INT(value);
}

API_FUNC(hdata_get_string)
{
    const char *hdata, *property, *result;

    API_INIT_FUNC(1, "hdata_get_string", API_RETURN_EMPTY);
    if (lua_gettop (L) < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    hdata = lua_tostring (L, -2);
    property = lua_tostring (L, -1);

    result = weechat_hdata_get_string (API_STR2PTR(hdata), property);

    API_RETURN_STRING(result);
}

int
weechat_lua_api_upgrade_read_cb (const void *pointer, void *data,
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

        rc = (int *) weechat_lua_exec (script,
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
    const char *filename, *function, *data;
    const char *result;

    API_INIT_FUNC(1, "upgrade_new", API_RETURN_EMPTY);
    if (lua_gettop (L) < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    filename = lua_tostring (L, -3);
    function = lua_tostring (L, -2);
    data = lua_tostring (L, -1);

    result = API_PTR2STR(
        plugin_script_api_upgrade_new (
            weechat_lua_plugin,
            lua_current_script,
            filename,
            &weechat_lua_api_upgrade_read_cb,
            function,
            data));

    API_RETURN_STRING(result);
}

API_FUNC(upgrade_write_object)
{
    const char *upgrade_file, *infolist;
    int object_id, rc;

    API_INIT_FUNC(1, "upgrade_write_object", API_RETURN_INT(0));
    if (lua_gettop (L) < 3)
        API_WRONG_ARGS(API_RETURN_INT(0));

    upgrade_file = lua_tostring (L, -3);
    object_id = lua_tonumber (L, -2);
    infolist = lua_tostring (L, -1);

    rc = weechat_upgrade_write_object (API_STR2PTR(upgrade_file),
                                       object_id,
                                       API_STR2PTR(infolist));

    API_RETURN_INT(rc);
}

API_FUNC(upgrade_read)
{
    const char *upgrade_file;
    int rc;

    API_INIT_FUNC(1, "upgrade_read", API_RETURN_INT(0));
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_INT(0));

    upgrade_file = lua_tostring (L, -1);

    rc = weechat_upgrade_read (API_STR2PTR(upgrade_file));

    API_RETURN_INT(rc);
}

API_FUNC(upgrade_close)
{
    const char *upgrade_file;

    API_INIT_FUNC(1, "upgrade_close", API_RETURN_ERROR);
    if (lua_gettop (L) < 1)
        API_WRONG_ARGS(API_RETURN_ERROR);

    upgrade_file = lua_tostring (L, -1);

    weechat_upgrade_close (API_STR2PTR(upgrade_file));

    API_RETURN_OK;
}

/*
 * Lua functions.
 */

const struct luaL_Reg weechat_lua_api_funcs[] = {
    { "__output__", weechat_lua_output },
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
    API_DEF_FUNC(config_option_get_string),
    API_DEF_FUNC(config_option_get_pointer),
    API_DEF_FUNC(config_option_is_null),
    API_DEF_FUNC(config_option_default_is_null),
    API_DEF_FUNC(config_boolean),
    API_DEF_FUNC(config_boolean_default),
    API_DEF_FUNC(config_boolean_inherited),
    API_DEF_FUNC(config_integer),
    API_DEF_FUNC(config_integer_default),
    API_DEF_FUNC(config_integer_inherited),
    API_DEF_FUNC(config_string),
    API_DEF_FUNC(config_string_default),
    API_DEF_FUNC(config_string_inherited),
    API_DEF_FUNC(config_color),
    API_DEF_FUNC(config_color_default),
    API_DEF_FUNC(config_color_inherited),
    API_DEF_FUNC(config_enum),
    API_DEF_FUNC(config_enum_default),
    API_DEF_FUNC(config_enum_inherited),
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
    API_DEF_FUNC(print),
    API_DEF_FUNC(print_date_tags),
    API_DEF_FUNC(print_datetime_tags),
    API_DEF_FUNC(print_y),
    API_DEF_FUNC(print_y_date_tags),
    API_DEF_FUNC(print_y_datetime_tags),
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
    API_DEF_FUNC(hook_url),
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
    API_DEF_FUNC(line_search_by_id),
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
    API_DEF_FUNC(completion_set),
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
    API_DEF_FUNC(hdata_longlong),
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
    { NULL, NULL },
};
