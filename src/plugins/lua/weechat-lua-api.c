/*
 * Copyright (C) 2006-2007 Emmanuel Bouthenot <kolter@openics.org>
 * Copyright (C) 2006-2012 Sebastien Helleu <flashcode@flashtux.org>
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
 * weechat-lua-api.c: lua API functions
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
#include "../plugin-script-callback.h"
#include "weechat-lua.h"


#define API_FUNC(__init, __name, __ret)                                 \
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
#define API_RETURN_OK return 1
#define API_RETURN_ERROR return 0
#define API_RETURN_EMPTY                                                \
    lua_pushstring (lua_current_interpreter, "");                       \
    return 0
#define API_RETURN_STRING(__string)                                     \
    lua_pushstring (lua_current_interpreter,                            \
                    (__string) ? __string : "");                        \
    return 1;
#define API_RETURN_STRING_FREE(__string)                                \
    lua_pushstring (lua_current_interpreter,                            \
                    (__string) ? __string : "");                        \
    if (__string)                                                       \
        free (__string);                                                \
    return 1;
#define API_RETURN_INT(__int)                                           \
    lua_pushnumber (lua_current_interpreter, __int);                    \
    return 1;
#define API_RETURN_LONG(__long)                                         \
    lua_pushnumber (lua_current_interpreter, __long);                   \
    return 1;

#define API_DEF_FUNC(__name)                                            \
    { #__name, &weechat_lua_api_##__name }


/*
 * weechat_lua_api_register: startup function for all WeeChat Lua scripts
 */

static int
weechat_lua_api_register (lua_State *L)
{
    const char *name, *author, *version, *license, *description;
    const char *shutdown_func, *charset;

    API_FUNC(0, "register", API_RETURN_ERROR);
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
    if (lua_gettop (lua_current_interpreter) < 7)
        API_WRONG_ARGS(API_RETURN_ERROR);

    name = lua_tostring (lua_current_interpreter, -7);
    author = lua_tostring (lua_current_interpreter, -6);
    version = lua_tostring (lua_current_interpreter, -5);
    license = lua_tostring (lua_current_interpreter, -4);
    description = lua_tostring (lua_current_interpreter, -3);
    shutdown_func = lua_tostring (lua_current_interpreter, -2);
    charset = lua_tostring (lua_current_interpreter, -1);

    if (plugin_script_search (weechat_lua_plugin, lua_scripts, name))
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
                                            &lua_scripts, &last_lua_script,
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
    }
    else
    {
        API_RETURN_ERROR;
    }

    API_RETURN_OK;
}

/*
 * weechat_lua_api_plugin_get_name: get name of plugin (return "core" for
 *                                  WeeChat core)
 */

static int
weechat_lua_api_plugin_get_name (lua_State *L)
{
    const char *plugin, *result;

    API_FUNC(1, "plugin_get_name", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 1)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    plugin = lua_tostring (lua_current_interpreter, -1);

    result = weechat_plugin_get_name (API_STR2PTR(plugin));

    API_RETURN_STRING(result);
}

/*
 * weechat_lua_api_charset_set: set script charset
 */

static int
weechat_lua_api_charset_set (lua_State *L)
{
    const char *charset;

    API_FUNC(1, "charset_set", API_RETURN_ERROR);
    if (lua_gettop (lua_current_interpreter) < 1)
        API_WRONG_ARGS(API_RETURN_ERROR);

    charset = lua_tostring (lua_current_interpreter, -1);

    plugin_script_api_charset_set (lua_current_script,
                                   charset);

    API_RETURN_OK;
}

/*
 * weechat_lua_api_iconv_to_internal: convert string to internal WeeChat charset
 */

static int
weechat_lua_api_iconv_to_internal (lua_State *L)
{
    const char *charset, *string;
    char *result;

    API_FUNC(1, "iconv_to_internal", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    charset = lua_tostring (lua_current_interpreter, -2);
    string = lua_tostring (lua_current_interpreter, -1);

    result = weechat_iconv_to_internal (charset, string);

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_iconv_from_internal: convert string from WeeChat internal
 *                                      charset to another one
 */

static int
weechat_lua_api_iconv_from_internal (lua_State *L)
{
    const char *charset, *string;
    char *result;

    API_FUNC(1, "iconv_from_internal", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    charset = lua_tostring (lua_current_interpreter, -2);
    string = lua_tostring (lua_current_interpreter, -1);

    result = weechat_iconv_from_internal (charset, string);

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_gettext: get translated string
 */

static int
weechat_lua_api_gettext (lua_State *L)
{
    const char *string, *result;

    API_FUNC(1, "gettext", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 1)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    string = lua_tostring (lua_current_interpreter, -1);

    result = weechat_gettext (string);

    API_RETURN_STRING(result);
}

/*
 * weechat_lua_api_ngettext: get translated string with plural form
 */

static int
weechat_lua_api_ngettext (lua_State *L)
{
    const char *single, *plural, *result;
    int count;

    API_FUNC(1, "ngettext", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    single = lua_tostring (lua_current_interpreter, -3);
    plural = lua_tostring (lua_current_interpreter, -2);
    count = lua_tonumber (lua_current_interpreter, -1);

    result = weechat_ngettext (single, plural, count);

    API_RETURN_STRING(result);
}

/*
 * weechat_lua_api_string_match: return 1 if string matches a mask
 *                               mask can begin or end with "*", no other "*"
 *                               are allowed inside mask
 */

static int
weechat_lua_api_string_match (lua_State *L)
{
    const char *string, *mask;
    int case_sensitive, value;

    API_FUNC(1, "string_match", API_RETURN_INT(0));
    if (lua_gettop (lua_current_interpreter) < 3)
        API_WRONG_ARGS(API_RETURN_INT(0));

    string = lua_tostring (lua_current_interpreter, -3);
    mask = lua_tostring (lua_current_interpreter, -2);
    case_sensitive = lua_tonumber (lua_current_interpreter, -1);

    value = weechat_string_match (string, mask, case_sensitive);

    API_RETURN_INT(value);
}

/*
 * weechat_lua_api_string_has_highlight: return 1 if string contains a
 *                                       highlight (using list of words to
 *                                       highlight)
 *                                       return 0 if no highlight is found in
 *                                       string
 */

static int
weechat_lua_api_string_has_highlight (lua_State *L)
{
    const char *string, *highlight_words;
    int value;

    API_FUNC(1, "string_has_highlight", API_RETURN_INT(0));
    if (lua_gettop (lua_current_interpreter) < 2)
        API_WRONG_ARGS(API_RETURN_INT(0));

    string = lua_tostring (lua_current_interpreter, -2);
    highlight_words = lua_tostring (lua_current_interpreter, -1);

    value = weechat_string_has_highlight (string, highlight_words);

    API_RETURN_INT(value);
}

/*
 * weechat_lua_api_string_has_highlight_regex: return 1 if string contains a
 *                                             highlight (using regular
 *                                             expression)
 *                                             return 0 if no highlight is
 *                                             found in string
 */

static int
weechat_lua_api_string_has_highlight_regex (lua_State *L)
{
    const char *string, *regex;
    int value;

    API_FUNC(1, "string_has_highlight_regex", API_RETURN_INT(0));
    if (lua_gettop (lua_current_interpreter) < 2)
        API_WRONG_ARGS(API_RETURN_INT(0));

    string = lua_tostring (lua_current_interpreter, -2);
    regex = lua_tostring (lua_current_interpreter, -1);

    value = weechat_string_has_highlight_regex (string, regex);

    API_RETURN_INT(value);
}

/*
 * weechat_lua_api_string_mask_to_regex: convert a mask (string with only
 *                                       "*" as wildcard) to a regex, paying
 *                                       attention to special chars in a
 *                                       regex
 */

static int
weechat_lua_api_string_mask_to_regex (lua_State *L)
{
    const char *mask;
    char *result;

    API_FUNC(1, "string_mask_to_regex", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 1)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    mask = lua_tostring (lua_current_interpreter, -1);

    result = weechat_string_mask_to_regex (mask);

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_string_remove_color: remove WeeChat color codes from string
 */

static int
weechat_lua_api_string_remove_color (lua_State *L)
{
    const char *string, *replacement;
    char *result;

    API_FUNC(1, "string_remove_color", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    string = lua_tostring (lua_current_interpreter, -2);
    replacement = lua_tostring (lua_current_interpreter, -1);

    result = weechat_string_remove_color (string, replacement);

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_string_is_command_char: check if first char of string is a
 *                                         command char
 */

static int
weechat_lua_api_string_is_command_char (lua_State *L)
{
    const char *string;
    int value;

    API_FUNC(1, "string_is_command_char", API_RETURN_INT(0));
    if (lua_gettop (lua_current_interpreter) < 1)
        API_WRONG_ARGS(API_RETURN_INT(0));

    string = lua_tostring (lua_current_interpreter, -1);

    value = weechat_string_is_command_char (string);

    API_RETURN_INT(value);
}

/*
 * weechat_lua_api_string_input_for_buffer: return string with input text
 *                                          for buffer or empty string if
 *                                          it's a command
 */

static int
weechat_lua_api_string_input_for_buffer (lua_State *L)
{
    const char *string, *result;

    API_FUNC(1, "string_input_for_buffer", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 1)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    string = lua_tostring (lua_current_interpreter, -1);

    result = weechat_string_input_for_buffer (string);

    API_RETURN_STRING(result);
}

/*
 * weechat_lua_api_mkdir_home: create a directory in WeeChat home
 */

static int
weechat_lua_api_mkdir_home (lua_State *L)
{
    const char *directory;
    int mode;

    API_FUNC(1, "mkdir_home", API_RETURN_ERROR);
    if (lua_gettop (lua_current_interpreter) < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    directory = lua_tostring (lua_current_interpreter, -2);
    mode = lua_tonumber (lua_current_interpreter, -1);

    if (weechat_mkdir_home (directory, mode))
        API_RETURN_OK;

    API_RETURN_OK;
}

/*
 * weechat_lua_api_mkdir: create a directory
 */

static int
weechat_lua_api_mkdir (lua_State *L)
{
    const char *directory;
    int mode;

    API_FUNC(1, "mkdir", API_RETURN_ERROR);
    if (lua_gettop (lua_current_interpreter) < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    directory = lua_tostring (lua_current_interpreter, -2);
    mode = lua_tonumber (lua_current_interpreter, -1);

    if (weechat_mkdir (directory, mode))
        API_RETURN_OK;

    API_RETURN_OK;
}

/*
 * weechat_lua_api_mkdir_parents: create a directory and make parent
 *                                directories as needed
 */

static int
weechat_lua_api_mkdir_parents (lua_State *L)
{
    const char *directory;
    int mode;

    API_FUNC(1, "mkdir_parents", API_RETURN_ERROR);
    if (lua_gettop (lua_current_interpreter) < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    directory = lua_tostring (lua_current_interpreter, -2);
    mode = lua_tonumber (lua_current_interpreter, -1);

    if (weechat_mkdir_parents (directory, mode))
        API_RETURN_OK;

    API_RETURN_OK;
}

/*
 * weechat_lua_api_list_new: create a new list
 */

static int
weechat_lua_api_list_new (lua_State *L)
{
    char *result;

    API_FUNC(1, "list_new", API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_list_new ());

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_list_add: add a string to list
 */

static int
weechat_lua_api_list_add (lua_State *L)
{
    const char *weelist, *data, *where, *user_data;
    char *result;

    API_FUNC(1, "list_add", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 4)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    weelist = lua_tostring (lua_current_interpreter, -4);
    data = lua_tostring (lua_current_interpreter, -3);
    where = lua_tostring (lua_current_interpreter, -2);
    user_data = lua_tostring (lua_current_interpreter, -1);

    result = API_PTR2STR(weechat_list_add (API_STR2PTR(weelist),
                                           data,
                                           where,
                                           API_STR2PTR(user_data)));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_list_search: search a string in list
 */

static int
weechat_lua_api_list_search (lua_State *L)
{
    const char *weelist, *data;
    char *result;

    API_FUNC(1, "list_search", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    weelist = lua_tostring (lua_current_interpreter, -2);
    data = lua_tostring (lua_current_interpreter, -1);

    result = API_PTR2STR(weechat_list_search (API_STR2PTR(weelist),
                                              data));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_list_search_pos: search position of a string in list
 */

static int
weechat_lua_api_list_search_pos (lua_State *L)
{
    const char *weelist, *data;
    int pos;

    API_FUNC(1, "list_search_pos", API_RETURN_INT(-1));
    if (lua_gettop (lua_current_interpreter) < 2)
        API_WRONG_ARGS(API_RETURN_INT(-1));

    weelist = lua_tostring (lua_current_interpreter, -2);
    data = lua_tostring (lua_current_interpreter, -1);

    pos = weechat_list_search_pos (API_STR2PTR(weelist), data);

    API_RETURN_INT(pos);
}

/*
 * weechat_lua_api_list_casesearch: search a string in list (ignore case)
 */

static int
weechat_lua_api_list_casesearch (lua_State *L)
{
    const char *weelist, *data;
    char *result;

    API_FUNC(1, "list_casesearch", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    weelist = lua_tostring (lua_current_interpreter, -2);
    data = lua_tostring (lua_current_interpreter, -1);

    result = API_PTR2STR(weechat_list_casesearch (API_STR2PTR(weelist),
                                                  data));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_list_casesearch_pos: search position of a string in list
 *                                      (ignore case)
 */

static int
weechat_lua_api_list_casesearch_pos (lua_State *L)
{
    const char *weelist, *data;
    int pos;

    API_FUNC(1, "list_casesearch_pos", API_RETURN_INT(-1));
    if (lua_gettop (lua_current_interpreter) < 2)
        API_WRONG_ARGS(API_RETURN_INT(-1));

    weelist = lua_tostring (lua_current_interpreter, -2);
    data = lua_tostring (lua_current_interpreter, -1);

    pos = weechat_list_casesearch_pos (API_STR2PTR(weelist), data);

    API_RETURN_INT(pos);
}

/*
 * weechat_lua_api_list_get: get item by position
 */

static int
weechat_lua_api_list_get (lua_State *L)
{
    const char *weelist;
    char *result;
    int position;

    API_FUNC(1, "list_get", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    weelist = lua_tostring (lua_current_interpreter, -2);
    position = lua_tonumber (lua_current_interpreter, -1);

    result = API_PTR2STR(weechat_list_get (API_STR2PTR(weelist),
                                           position));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_list_set: set new value for item
 */

static int
weechat_lua_api_list_set (lua_State *L)
{
    const char *item, *new_value;

    API_FUNC(1, "list_set", API_RETURN_ERROR);
    if (lua_gettop (lua_current_interpreter) < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    item = lua_tostring (lua_current_interpreter, -2);
    new_value = lua_tostring (lua_current_interpreter, -1);

    weechat_list_set (API_STR2PTR(item),
                      new_value);

    API_RETURN_OK;
}

/*
 * weechat_lua_api_list_next: get next item
 */

static int
weechat_lua_api_list_next (lua_State *L)
{
    const char *item;
    char *result;

    API_FUNC(1, "list_next", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 1)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    item = lua_tostring (lua_current_interpreter, -1);

    result = API_PTR2STR(weechat_list_next (API_STR2PTR(item)));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_list_prev: get previous item
 */

static int
weechat_lua_api_list_prev (lua_State *L)
{
    const char *item;
    char *result;

    API_FUNC(1, "list_prev", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 1)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    item = lua_tostring (lua_current_interpreter, -1);

    result = API_PTR2STR(weechat_list_prev (API_STR2PTR(item)));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_list_string: get string value of item
 */

static int
weechat_lua_api_list_string (lua_State *L)
{
    const char *item, *result;

    API_FUNC(1, "list_string", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 1)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    item = lua_tostring (lua_current_interpreter, -1);

    result = weechat_list_string (API_STR2PTR(item));

    API_RETURN_STRING(result);
}

/*
 * weechat_lua_api_list_size: get number of elements in list
 */

static int
weechat_lua_api_list_size (lua_State *L)
{
    const char *weelist;
    int size;

    API_FUNC(1, "list_size", API_RETURN_INT(0));
    if (lua_gettop (lua_current_interpreter) < 1)
        API_WRONG_ARGS(API_RETURN_INT(0));

    weelist = lua_tostring (lua_current_interpreter, -1);

    size = weechat_list_size (API_STR2PTR(weelist));

    API_RETURN_INT(size);
}

/*
 * weechat_lua_api_list_remove: remove item from list
 */

static int
weechat_lua_api_list_remove (lua_State *L)
{
    const char *weelist, *item;

    API_FUNC(1, "list_remove", API_RETURN_ERROR);
    if (lua_gettop (lua_current_interpreter) < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    weelist = lua_tostring (lua_current_interpreter, -2);
    item = lua_tostring (lua_current_interpreter, -1);

    weechat_list_remove (API_STR2PTR(weelist),
                         API_STR2PTR(item));

    API_RETURN_OK;
}

/*
 * weechat_lua_api_list_remove_all: remove all items from list
 */

static int
weechat_lua_api_list_remove_all (lua_State *L)
{
    const char *weelist;

    API_FUNC(1, "list_remove_all", API_RETURN_ERROR);
    if (lua_gettop (lua_current_interpreter) < 1)
        API_WRONG_ARGS(API_RETURN_ERROR);

    weelist = lua_tostring (lua_current_interpreter, -1);

    weechat_list_remove_all (API_STR2PTR(weelist));

    API_RETURN_OK;
}

/*
 * weechat_lua_api_list_free: free list
 */

static int
weechat_lua_api_list_free (lua_State *L)
{
    const char *weelist;

    API_FUNC(1, "list_free", API_RETURN_ERROR);
    if (lua_gettop (lua_current_interpreter) < 1)
        API_WRONG_ARGS(API_RETURN_ERROR);

    weelist = lua_tostring (lua_current_interpreter, -1);

    weechat_list_free (API_STR2PTR(weelist));

    API_RETURN_OK;
}

/*
 * weechat_lua_api_config_reload_cb: callback for ccnfig reload
 */

int
weechat_lua_api_config_reload_cb (void *data,
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

        rc = (int *) weechat_lua_exec (script_callback->script,
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
 * weechat_lua_api_config_new: create a new configuration file
 */

static int
weechat_lua_api_config_new (lua_State *L)
{
    const char *name, *function, *data;
    char *result;

    API_FUNC(1, "config_new", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    name = lua_tostring (lua_current_interpreter, -3);
    function = lua_tostring (lua_current_interpreter, -2);
    data = lua_tostring (lua_current_interpreter, -1);

    result = API_PTR2STR(plugin_script_api_config_new (weechat_lua_plugin,
                                                       lua_current_script,
                                                       name,
                                                       &weechat_lua_api_config_reload_cb,
                                                       function,
                                                       data));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_config_read_cb: callback for reading option in section
 */

int
weechat_lua_api_config_read_cb (void *data,
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

        rc = (int *) weechat_lua_exec (script_callback->script,
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
 * weechat_lua_api_config_section_write_cb: callback for writing section
 */

int
weechat_lua_api_config_section_write_cb (void *data,
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

        rc = (int *) weechat_lua_exec (script_callback->script,
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
 * weechat_lua_api_config_section_write_default_cb: callback for writing
 *                                                  default values for section
 */

int
weechat_lua_api_config_section_write_default_cb (void *data,
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

        rc = (int *) weechat_lua_exec (script_callback->script,
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
 * weechat_lua_api_config_section_create_option_cb: callback to create an option
 */

int
weechat_lua_api_config_section_create_option_cb (void *data,
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

        rc = (int *) weechat_lua_exec (script_callback->script,
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
 * weechat_lua_api_config_section_delete_option_cb: callback to delete an option
 */

int
weechat_lua_api_config_section_delete_option_cb (void *data,
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

        rc = (int *) weechat_lua_exec (script_callback->script,
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
 * weechat_lua_api_config_new_section: create a new section in configuration file
 */

static int
weechat_lua_api_config_new_section (lua_State *L)
{
    const char *config_file, *name, *function_read, *data_read;
    const char *function_write, *data_write, *function_write_default;
    const char *data_write_default, *function_create_option;
    const char *data_create_option, *function_delete_option;
    const char *data_delete_option;
    char *result;
    int user_can_add_options, user_can_delete_options;

    API_FUNC(1, "config_new_section", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 14)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    config_file = lua_tostring (lua_current_interpreter, -14);
    name = lua_tostring (lua_current_interpreter, -13);
    user_can_add_options = lua_tonumber (lua_current_interpreter, -12);
    user_can_delete_options = lua_tonumber (lua_current_interpreter, -11);
    function_read = lua_tostring (lua_current_interpreter, -10);
    data_read = lua_tostring (lua_current_interpreter, -9);
    function_write = lua_tostring (lua_current_interpreter, -8);
    data_write = lua_tostring (lua_current_interpreter, -7);
    function_write_default = lua_tostring (lua_current_interpreter, -6);
    data_write_default = lua_tostring (lua_current_interpreter, -5);
    function_create_option = lua_tostring (lua_current_interpreter, -4);
    data_create_option = lua_tostring (lua_current_interpreter, -3);
    function_delete_option = lua_tostring (lua_current_interpreter, -2);
    data_delete_option = lua_tostring (lua_current_interpreter, -1);

    result = API_PTR2STR(plugin_script_api_config_new_section (weechat_lua_plugin,
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

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_config_search_section: search a section in configuration file
 */

static int
weechat_lua_api_config_search_section (lua_State *L)
{
    const char *config_file, *section_name;
    char *result;

    API_FUNC(1, "config_search_section", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    config_file = lua_tostring (lua_current_interpreter, -2);
    section_name = lua_tostring (lua_current_interpreter, -1);

    result = API_PTR2STR(weechat_config_search_section (API_STR2PTR(config_file),
                                                        section_name));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_config_option_check_value_cb: callback for checking new
 *                                               value for option
 */

int
weechat_lua_api_config_option_check_value_cb (void *data,
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

        rc = (int *) weechat_lua_exec (script_callback->script,
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
 * weechat_lua_api_config_option_change_cb: callback for option changed
 */

void
weechat_lua_api_config_option_change_cb (void *data,
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

        rc = (int *) weechat_lua_exec (script_callback->script,
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
 * weechat_lua_api_config_option_delete_cb: callback when option is deleted
 */

void
weechat_lua_api_config_option_delete_cb (void *data,
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

        rc = (int *) weechat_lua_exec (script_callback->script,
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
 * weechat_lua_api_config_new_option: create a new option in section
 */

static int
weechat_lua_api_config_new_option (lua_State *L)
{
    const char *config_file, *section, *name, *type, *description;
    const char *string_values, *default_value, *value;
    const char *function_check_value, *data_check_value, *function_change;
    const char *data_change, *function_delete, *data_delete;
    char *result;
    int min, max, null_value_allowed;

    API_FUNC(1, "config_new_option", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 17)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    config_file = lua_tostring (lua_current_interpreter, -17);
    section = lua_tostring (lua_current_interpreter, -16);
    name = lua_tostring (lua_current_interpreter, -15);
    type = lua_tostring (lua_current_interpreter, -14);
    description = lua_tostring (lua_current_interpreter, -13);
    string_values = lua_tostring (lua_current_interpreter, -12);
    min = lua_tonumber (lua_current_interpreter, -11);
    max = lua_tonumber (lua_current_interpreter, -10);
    default_value = lua_tostring (lua_current_interpreter, -9);
    value = lua_tostring (lua_current_interpreter, -8);
    null_value_allowed = lua_tonumber (lua_current_interpreter, -7);
    function_check_value = lua_tostring (lua_current_interpreter, -6);
    data_check_value = lua_tostring (lua_current_interpreter, -5);
    function_change = lua_tostring (lua_current_interpreter, -4);
    data_change = lua_tostring (lua_current_interpreter, -3);
    function_delete = lua_tostring (lua_current_interpreter, -2);
    data_delete = lua_tostring (lua_current_interpreter, -1);

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

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_config_search_option: search option in configuration file or section
 */

static int
weechat_lua_api_config_search_option (lua_State *L)
{
    const char *config_file, *section, *option_name;
    char *result;

    API_FUNC(1, "config_search_option", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    config_file = lua_tostring (lua_current_interpreter, -3);
    section = lua_tostring (lua_current_interpreter, -2);
    option_name = lua_tostring (lua_current_interpreter, -1);

    result = API_PTR2STR(weechat_config_search_option (API_STR2PTR(config_file),
                                                       API_STR2PTR(section),
                                                       option_name));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_config_string_to_boolean: return boolean value of a string
 */

static int
weechat_lua_api_config_string_to_boolean (lua_State *L)
{
    const char *text;
    int value;

    API_FUNC(1, "config_string_to_boolean", API_RETURN_INT(0));
    if (lua_gettop (lua_current_interpreter) < 1)
        API_WRONG_ARGS(API_RETURN_INT(0));

    text = lua_tostring (lua_current_interpreter, -1);

    value = weechat_config_string_to_boolean (text);

    API_RETURN_INT(value);
}

/*
 * weechat_lua_api_config_option_reset: reset option with default value
 */

static int
weechat_lua_api_config_option_reset (lua_State *L)
{
    const char *option;
    int run_callback, rc;

    API_FUNC(1, "config_option_reset", API_RETURN_INT(0));
    if (lua_gettop (lua_current_interpreter) < 2)
        API_WRONG_ARGS(API_RETURN_INT(0));

    option = lua_tostring (lua_current_interpreter, -2);
    run_callback = lua_tonumber (lua_current_interpreter, -1);

    rc = weechat_config_option_reset (API_STR2PTR(option),
                                      run_callback);

    API_RETURN_INT(rc);
}

/*
 * weechat_lua_api_config_option_set: set new value for option
 */

static int
weechat_lua_api_config_option_set (lua_State *L)
{
    const char *option, *new_value;
    int run_callback, rc;

    API_FUNC(1, "config_option_set", API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));
    if (lua_gettop (lua_current_interpreter) < 3)
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));

    option = lua_tostring (lua_current_interpreter, -3);
    new_value = lua_tostring (lua_current_interpreter, -2);
    run_callback = lua_tonumber (lua_current_interpreter, -1);

    rc = weechat_config_option_set (API_STR2PTR(option),
                                    new_value,
                                    run_callback);

    API_RETURN_INT(rc);
}

/*
 * weechat_lua_api_config_option_set_null: set null (undefined) value for
 *                                         option
 */

static int
weechat_lua_api_config_option_set_null (lua_State *L)
{
    const char *option;
    int run_callback, rc;

    API_FUNC(1, "config_option_set_null", API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));
    if (lua_gettop (lua_current_interpreter) < 2)
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));

    option = lua_tostring (lua_current_interpreter, -2);
    run_callback = lua_tonumber (lua_current_interpreter, -1);

    rc = weechat_config_option_set_null (API_STR2PTR(option),
                                         run_callback);

    API_RETURN_INT(rc);
}

/*
 * weechat_lua_api_config_option_unset: unset an option
 */

static int
weechat_lua_api_config_option_unset (lua_State *L)
{
    const char *option;
    int rc;

    API_FUNC(1, "config_option_unset", API_RETURN_INT(WEECHAT_CONFIG_OPTION_UNSET_ERROR));
    if (lua_gettop (lua_current_interpreter) < 1)
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_OPTION_UNSET_ERROR));

    option = lua_tostring (lua_current_interpreter, -1);

    rc = weechat_config_option_unset (API_STR2PTR(option));

    API_RETURN_INT(rc);
}

/*
 * weechat_lua_api_config_option_rename: rename an option
 */

static int
weechat_lua_api_config_option_rename (lua_State *L)
{
    const char *option, *new_name;

    API_FUNC(1, "config_option_rename", API_RETURN_ERROR);
    if (lua_gettop (lua_current_interpreter) < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    option = lua_tostring (lua_current_interpreter, -2);
    new_name = lua_tostring (lua_current_interpreter, -1);

    weechat_config_option_rename (API_STR2PTR(option),
                                  new_name);

    API_RETURN_OK;
}

/*
 * weechat_lua_api_config_option_is_null: return 1 if value of option is null
 */

static int
weechat_lua_api_config_option_is_null (lua_State *L)
{
    const char *option;
    int value;

    API_FUNC(1, "config_option_is_null", API_RETURN_INT(1));
    if (lua_gettop (lua_current_interpreter) < 1)
        API_WRONG_ARGS(API_RETURN_INT(1));

    option = lua_tostring (lua_current_interpreter, -1);

    value = weechat_config_option_is_null (API_STR2PTR(option));

    API_RETURN_INT(value);
}

/*
 * weechat_lua_api_config_option_default_is_null: return 1 if default value of
 *                                                option is null
 */

static int
weechat_lua_api_config_option_default_is_null (lua_State *L)
{
    const char *option;
    int value;

    API_FUNC(1, "config_option_default_is_null", API_RETURN_INT(1));
    if (lua_gettop (lua_current_interpreter) < 1)
        API_WRONG_ARGS(API_RETURN_INT(1));

    option = lua_tostring (lua_current_interpreter, -1);

    value = weechat_config_option_default_is_null (API_STR2PTR(option));

    API_RETURN_INT(value);
}

/*
 * weechat_lua_api_config_boolean: return boolean value of option
 */

static int
weechat_lua_api_config_boolean (lua_State *L)
{
    const char *option;
    int value;

    API_FUNC(1, "config_boolean", API_RETURN_INT(0));
    if (lua_gettop (lua_current_interpreter) < 1)
        API_WRONG_ARGS(API_RETURN_INT(0));

    option = lua_tostring (lua_current_interpreter, -1);

    value = weechat_config_boolean (API_STR2PTR(option));

    API_RETURN_INT(value);
}

/*
 * weechat_lua_api_config_boolean_default: return default boolean value of option
 */

static int
weechat_lua_api_config_boolean_default (lua_State *L)
{
    const char *option;
    int value;

    API_FUNC(1, "config_boolean_default", API_RETURN_INT(0));
    if (lua_gettop (lua_current_interpreter) < 1)
        API_WRONG_ARGS(API_RETURN_INT(0));

    option = lua_tostring (lua_current_interpreter, -1);

    value = weechat_config_boolean_default (API_STR2PTR(option));

    API_RETURN_INT(value);
}

/*
 * weechat_lua_api_config_integer: return integer value of option
 */

static int
weechat_lua_api_config_integer (lua_State *L)
{
    const char *option;
    int value;

    API_FUNC(1, "config_integer", API_RETURN_INT(0));
    if (lua_gettop (lua_current_interpreter) < 1)
        API_WRONG_ARGS(API_RETURN_INT(0));

    option = lua_tostring (lua_current_interpreter, -1);

    value = weechat_config_integer (API_STR2PTR(option));

    API_RETURN_INT(value);
}

/*
 * weechat_lua_api_config_integer_default: return default integer value of option
 */

static int
weechat_lua_api_config_integer_default (lua_State *L)
{
    const char *option;
    int value;

    API_FUNC(1, "config_integer_default", API_RETURN_INT(0));
    if (lua_gettop (lua_current_interpreter) < 1)
        API_WRONG_ARGS(API_RETURN_INT(0));

    option = lua_tostring (lua_current_interpreter, -1);

    value = weechat_config_integer_default (API_STR2PTR(option));

    API_RETURN_INT(value);
}

/*
 * weechat_lua_api_config_string: return string value of option
 */

static int
weechat_lua_api_config_string (lua_State *L)
{
    const char *option, *result;

    API_FUNC(1, "config_string", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 1)
        API_WRONG_ARGS(API_RETURN_INT(0));

    option = lua_tostring (lua_current_interpreter, -1);

    result = weechat_config_string (API_STR2PTR(option));

    API_RETURN_STRING(result);
}

/*
 * weechat_lua_api_config_string_default: return default string value of option
 */

static int
weechat_lua_api_config_string_default (lua_State *L)
{
    const char *option, *result;

    API_FUNC(1, "config_string_default", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 1)
        API_WRONG_ARGS(API_RETURN_INT(0));

    option = lua_tostring (lua_current_interpreter, -1);

    result = weechat_config_string_default (API_STR2PTR(option));

    API_RETURN_STRING(result);
}

/*
 * weechat_lua_api_config_color: return color value of option
 */

static int
weechat_lua_api_config_color (lua_State *L)
{
    const char *option, *result;

    API_FUNC(1, "config_color", API_RETURN_INT(0));
    if (lua_gettop (lua_current_interpreter) < 1)
        API_WRONG_ARGS(API_RETURN_INT(0));

    option = lua_tostring (lua_current_interpreter, -1);

    result = weechat_config_color (API_STR2PTR(option));

    API_RETURN_STRING(result);
}

/*
 * weechat_lua_api_config_color_default: return default color value of option
 */

static int
weechat_lua_api_config_color_default (lua_State *L)
{
    const char *option, *result;

    API_FUNC(1, "config_color_default", API_RETURN_INT(0));
    if (lua_gettop (lua_current_interpreter) < 1)
        API_WRONG_ARGS(API_RETURN_INT(0));

    option = lua_tostring (lua_current_interpreter, -1);

    result = weechat_config_color_default (API_STR2PTR(option));

    API_RETURN_STRING(result);
}

/*
 * weechat_lua_api_config_write_option: write an option in configuration file
 */

static int
weechat_lua_api_config_write_option (lua_State *L)
{
    const char *config_file, *option;

    API_FUNC(1, "config_write_option", API_RETURN_ERROR);
    if (lua_gettop (lua_current_interpreter) < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    config_file = lua_tostring (lua_current_interpreter, -2);
    option = lua_tostring (lua_current_interpreter, -1);

    weechat_config_write_option (API_STR2PTR(config_file),
                                 API_STR2PTR(option));

    API_RETURN_OK;
}

/*
 * weechat_lua_api_config_write_line: write a line in configuration file
 */

static int
weechat_lua_api_config_write_line (lua_State *L)
{
    const char *config_file, *option_name, *value;

    API_FUNC(1, "config_write_line", API_RETURN_ERROR);
    if (lua_gettop (lua_current_interpreter) < 3)
        API_WRONG_ARGS(API_RETURN_ERROR);

    config_file = lua_tostring (lua_current_interpreter, -3);
    option_name = lua_tostring (lua_current_interpreter, -2);
    value = lua_tostring (lua_current_interpreter, -1);

    weechat_config_write_line (API_STR2PTR(config_file),
                               option_name,
                               "%s",
                               value);

    API_RETURN_OK;
}

/*
 * weechat_lua_api_config_write: write configuration file
 */

static int
weechat_lua_api_config_write (lua_State *L)
{
    const char *config_file;
    int rc;

    API_FUNC(1, "config_write", API_RETURN_INT(-1));
    if (lua_gettop (lua_current_interpreter) < 1)
        API_WRONG_ARGS(API_RETURN_INT(-1));

    config_file = lua_tostring (lua_current_interpreter, -1);

    rc = weechat_config_write (API_STR2PTR(config_file));

    API_RETURN_INT(rc);
}

/*
 * weechat_lua_api_config_read: read configuration file
 */

static int
weechat_lua_api_config_read (lua_State *L)
{
    const char *config_file;
    int rc;

    API_FUNC(1, "config_read", API_RETURN_INT(-1));
    if (lua_gettop (lua_current_interpreter) < 1)
        API_WRONG_ARGS(API_RETURN_INT(-1));

    config_file = lua_tostring (lua_current_interpreter, -1);

    rc = weechat_config_read (API_STR2PTR(config_file));

    API_RETURN_INT(rc);
}

/*
 * weechat_lua_api_config_reload: reload configuration file
 */

static int
weechat_lua_api_config_reload (lua_State *L)
{
    const char *config_file;
    int rc;

    API_FUNC(1, "config_reload", API_RETURN_INT(-1));
    if (lua_gettop (lua_current_interpreter) < 1)
        API_WRONG_ARGS(API_RETURN_INT(-1));

    config_file = lua_tostring (lua_current_interpreter, -1);

    rc = weechat_config_reload (API_STR2PTR(config_file));

    API_RETURN_INT(rc);
}

/*
 * weechat_lua_api_config_option_free: free an option in configuration file
 */

static int
weechat_lua_api_config_option_free (lua_State *L)
{
    const char *option;

    API_FUNC(1, "config_option_free", API_RETURN_ERROR);
    if (lua_gettop (lua_current_interpreter) < 1)
        API_WRONG_ARGS(API_RETURN_ERROR);

    option = lua_tostring (lua_current_interpreter, -1);

    plugin_script_api_config_option_free (weechat_lua_plugin,
                                          lua_current_script,
                                          API_STR2PTR(option));

    API_RETURN_OK;
}

/*
 * weechat_lua_api_config_section_free_options: free all options of a section
 *                                              in configuration file
 */

static int
weechat_lua_api_config_section_free_options (lua_State *L)
{
    const char *section;

    API_FUNC(1, "config_section_free_options", API_RETURN_ERROR);
    if (lua_gettop (lua_current_interpreter) < 1)
        API_WRONG_ARGS(API_RETURN_ERROR);

    section = lua_tostring (lua_current_interpreter, -1);

    plugin_script_api_config_section_free_options (weechat_lua_plugin,
                                                   lua_current_script,
                                                   API_STR2PTR(section));

    API_RETURN_OK;
}

/*
 * weechat_lua_api_config_section_free: free section in configuration file
 */

static int
weechat_lua_api_config_section_free (lua_State *L)
{
    const char *section;

    API_FUNC(1, "config_section_free", API_RETURN_ERROR);
    if (lua_gettop (lua_current_interpreter) < 1)
        API_WRONG_ARGS(API_RETURN_ERROR);

    section = lua_tostring (lua_current_interpreter, -1);

    plugin_script_api_config_section_free (weechat_lua_plugin,
                                           lua_current_script,
                                           API_STR2PTR(section));

    API_RETURN_OK;
}

/*
 * weechat_lua_api_config_free: free configuration file
 */

static int
weechat_lua_api_config_free (lua_State *L)
{
    const char *config_file;

    API_FUNC(1, "config_free", API_RETURN_ERROR);
    if (lua_gettop (lua_current_interpreter) < 1)
        API_WRONG_ARGS(API_RETURN_ERROR);

    config_file = lua_tostring (lua_current_interpreter, -1);

    plugin_script_api_config_free (weechat_lua_plugin,
                                   lua_current_script,
                                   API_STR2PTR(config_file));

    API_RETURN_OK;
}

/*
 * weechat_lua_api_config_get: get config option
 */

static int
weechat_lua_api_config_get (lua_State *L)
{
    const char *option;
    char *result;

    API_FUNC(1, "config_get", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 1)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    option = lua_tostring (lua_current_interpreter, -1);

    result = API_PTR2STR(weechat_config_get (option));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_config_get_plugin: get value of a plugin option
 */

static int
weechat_lua_api_config_get_plugin (lua_State *L)
{
    const char *option, *result;

    API_FUNC(1, "config_get_plugin", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 1)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    option = lua_tostring (lua_current_interpreter, -1);

    result = plugin_script_api_config_get_plugin (weechat_lua_plugin,
                                                  lua_current_script,
                                                  option);

    API_RETURN_STRING(result);
}

/*
 * weechat_lua_api_config_is_set_plugin: check if a plugin option is set
 */

static int
weechat_lua_api_config_is_set_plugin (lua_State *L)
{
    const char *option;
    int rc;

    API_FUNC(1, "config_is_set_plugin", API_RETURN_INT(0));
    if (lua_gettop (lua_current_interpreter) < 1)
        API_WRONG_ARGS(API_RETURN_INT(0));

    option = lua_tostring (lua_current_interpreter, -1);

    rc = plugin_script_api_config_is_set_plugin (weechat_lua_plugin,
                                                 lua_current_script,
                                                 option);

    API_RETURN_INT(rc);
}

/*
 * weechat_lua_api_config_set_plugin: set value of a plugin option
 */

static int
weechat_lua_api_config_set_plugin (lua_State *L)
{
    const char *option, *value;
    int rc;

    API_FUNC(1, "config_set_plugin", API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));
    if (lua_gettop (lua_current_interpreter) < 2)
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));

    option = lua_tostring (lua_current_interpreter, -2);
    value = lua_tostring (lua_current_interpreter, -1);

    rc = plugin_script_api_config_set_plugin (weechat_lua_plugin,
                                              lua_current_script,
                                              option,
                                              value);

    API_RETURN_INT(rc);
}

/*
 * weechat_lua_api_config_set_desc_plugin: set description of a plugin option
 */

static int
weechat_lua_api_config_set_desc_plugin (lua_State *L)
{
    const char *option, *description;

    API_FUNC(1, "config_set_desc_plugin", API_RETURN_ERROR);
    if (lua_gettop (lua_current_interpreter) < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    option = lua_tostring (lua_current_interpreter, -2);
    description = lua_tostring (lua_current_interpreter, -1);

    plugin_script_api_config_set_desc_plugin (weechat_lua_plugin,
                                              lua_current_script,
                                              option,
                                              description);

    API_RETURN_OK;
}

/*
 * weechat_lua_api_config_unset_plugin: unset plugin option
 */

static int
weechat_lua_api_config_unset_plugin (lua_State *L)
{
    const char *option;
    int rc;

    API_FUNC(1, "config_unset_plugin", API_RETURN_INT(WEECHAT_CONFIG_OPTION_UNSET_ERROR));
    if (lua_gettop (lua_current_interpreter) < 1)
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_OPTION_UNSET_ERROR));

    option = lua_tostring (lua_current_interpreter, -1);

    rc = plugin_script_api_config_unset_plugin (weechat_lua_plugin,
                                                lua_current_script,
                                                option);

    API_RETURN_INT(rc);
}

/*
 * weechat_lua_api_key_bind: bind key(s)
 */

static int
weechat_lua_api_key_bind (lua_State *L)
{
    const char *context;
    struct t_hashtable *hashtable;
    int num_keys;

    API_FUNC(1, "key_bind", API_RETURN_INT(0));
    if (lua_gettop (lua_current_interpreter) < 2)
        API_WRONG_ARGS(API_RETURN_INT(0));

    context = lua_tostring (lua_current_interpreter, -2);
    hashtable = weechat_lua_tohashtable (lua_current_interpreter, -1,
                                         WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE);

    num_keys = weechat_key_bind (context, hashtable);

    if (hashtable)
        weechat_hashtable_free (hashtable);

    API_RETURN_INT(num_keys);
}

/*
 * weechat_lua_api_key_unbind: unbind key(s)
 */

static int
weechat_lua_api_key_unbind (lua_State *L)
{
    const char *context, *key;
    int num_keys;

    API_FUNC(1, "key_unbind", API_RETURN_INT(0));
    if (lua_gettop (lua_current_interpreter) < 2)
        API_WRONG_ARGS(API_RETURN_INT(0));

    context = lua_tostring (lua_current_interpreter, -2);
    key = lua_tostring (lua_current_interpreter, -1);

    num_keys = weechat_key_unbind (context, key);

    API_RETURN_INT(num_keys);
}

/*
 * weechat_lua_api_prefix: get a prefix, used for display
 */

static int
weechat_lua_api_prefix (lua_State *L)
{
    const char *prefix, *result;

    API_FUNC(0, "prefix", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 1)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    prefix = lua_tostring (lua_current_interpreter, -1);

    result = weechat_prefix (prefix);

    API_RETURN_STRING(result);
}

/*
 * weechat_lua_api_color: get a color code, used for display
 */

static int
weechat_lua_api_color (lua_State *L)
{
    const char *color, *result;

    API_FUNC(0, "color", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 1)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    color = lua_tostring (lua_current_interpreter, -1);

    result = weechat_color (color);

    API_RETURN_STRING(result);
}

/*
 * weechat_lua_api_print: print message in a buffer
 */

static int
weechat_lua_api_print (lua_State *L)
{
    const char *buffer, *message;

    API_FUNC(0, "print", API_RETURN_ERROR);
    if (lua_gettop (lua_current_interpreter) < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    buffer = lua_tostring (lua_current_interpreter, -2);
    message = lua_tostring (lua_current_interpreter, -1);

    plugin_script_api_printf (weechat_lua_plugin,
                              lua_current_script,
                              API_STR2PTR(buffer),
                              "%s", message);

    API_RETURN_OK;
}

/*
 * weechat_lua_api_print_date_tags: print message in a buffer with optional
 *                                  date and tags
 */

static int
weechat_lua_api_print_date_tags (lua_State *L)
{
    const char *buffer, *tags, *message;
    int date;

    API_FUNC(1, "print_date_tags", API_RETURN_ERROR);
    if (lua_gettop (lua_current_interpreter) < 4)
        API_WRONG_ARGS(API_RETURN_ERROR);

    buffer = lua_tostring (lua_current_interpreter, -4);
    date = lua_tonumber (lua_current_interpreter, -3);
    tags = lua_tostring (lua_current_interpreter, -2);
    message = lua_tostring (lua_current_interpreter, -1);

    plugin_script_api_printf_date_tags (weechat_lua_plugin,
                                        lua_current_script,
                                        API_STR2PTR(buffer),
                                        date,
                                        tags,
                                        "%s", message);

    API_RETURN_OK;
}

/*
 * weechat_lua_api_print_y: print message in a buffer with free content
 */

static int
weechat_lua_api_print_y (lua_State *L)
{
    const char *buffer, *message;
    int y;

    API_FUNC(1, "print_y", API_RETURN_ERROR);
    if (lua_gettop (lua_current_interpreter) < 3)
        API_WRONG_ARGS(API_RETURN_ERROR);

    buffer = lua_tostring (lua_current_interpreter, -3);
    y = lua_tonumber (lua_current_interpreter, -2);
    message = lua_tostring (lua_current_interpreter, -1);

    plugin_script_api_printf_y (weechat_lua_plugin,
                                lua_current_script,
                                API_STR2PTR(buffer),
                                y,
                                "%s", message);

    API_RETURN_OK;
}

/*
 * weechat_lua_api_log_print: print message in WeeChat log file
 */

static int
weechat_lua_api_log_print (lua_State *L)
{
    const char *message;

    API_FUNC(1, "log_print", API_RETURN_ERROR);
    if (lua_gettop (lua_current_interpreter) < 1)
        API_WRONG_ARGS(API_RETURN_ERROR);

    message = lua_tostring (lua_current_interpreter, -1);

    plugin_script_api_log_printf (weechat_lua_plugin,
                                  lua_current_script,
                                  "%s", message);

    API_RETURN_OK;
}

/*
 * weechat_lua_api_hook_command_cb: callback for command hooked
 */

int
weechat_lua_api_hook_command_cb (void *data, struct t_gui_buffer *buffer,
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

        rc = (int *) weechat_lua_exec (script_callback->script,
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
 * weechat_lua_api_hook_command: hook a command
 */

static int
weechat_lua_api_hook_command (lua_State *L)
{
    const char *command, *description, *args, *args_description, *completion;
    const char *function, *data;
    char *result;

    API_FUNC(1, "hook_command", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 7)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    command = lua_tostring (lua_current_interpreter, -7);
    description = lua_tostring (lua_current_interpreter, -6);
    args = lua_tostring (lua_current_interpreter, -5);
    args_description = lua_tostring (lua_current_interpreter, -4);
    completion = lua_tostring (lua_current_interpreter, -3);
    function = lua_tostring (lua_current_interpreter, -2);
    data = lua_tostring (lua_current_interpreter, -1);

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

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_hook_command_run_cb: callback for command_run hooked
 */

int
weechat_lua_api_hook_command_run_cb (void *data, struct t_gui_buffer *buffer,
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

        rc = (int *) weechat_lua_exec (script_callback->script,
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
 * weechat_lua_api_hook_command_run: hook a command_run
 */

static int
weechat_lua_api_hook_command_run (lua_State *L)
{
    const char *command, *function, *data;
    char *result;

    API_FUNC(1, "hook_command_run", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    command = lua_tostring (lua_current_interpreter, -3);
    function = lua_tostring (lua_current_interpreter, -2);
    data = lua_tostring (lua_current_interpreter, -1);

    result = API_PTR2STR(plugin_script_api_hook_command_run (weechat_lua_plugin,
                                                             lua_current_script,
                                                             command,
                                                             &weechat_lua_api_hook_command_run_cb,
                                                             function,
                                                             data));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_hook_timer_cb: callback for timer hooked
 */

int
weechat_lua_api_hook_timer_cb (void *data, int remaining_calls)
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

        rc = (int *) weechat_lua_exec (script_callback->script,
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
 * weechat_lua_api_hook_timer: hook a timer
 */

static int
weechat_lua_api_hook_timer (lua_State *L)
{
    int interval, align_second, max_calls;
    const char *function, *data;
    char *result;

    API_FUNC(1, "hook_timer", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 5)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    interval = lua_tonumber (lua_current_interpreter, -5);
    align_second = lua_tonumber (lua_current_interpreter, -4);
    max_calls = lua_tonumber (lua_current_interpreter, -3);
    function = lua_tostring (lua_current_interpreter, -2);
    data = lua_tostring (lua_current_interpreter, -1);

    result = API_PTR2STR(plugin_script_api_hook_timer (weechat_lua_plugin,
                                                       lua_current_script,
                                                       interval,
                                                       align_second,
                                                       max_calls,
                                                       &weechat_lua_api_hook_timer_cb,
                                                       function,
                                                       data));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_hook_fd_cb: callback for fd hooked
 */

int
weechat_lua_api_hook_fd_cb (void *data, int fd)
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

        rc = (int *) weechat_lua_exec (script_callback->script,
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
 * weechat_lua_api_hook_fd: hook a fd
 */

static int
weechat_lua_api_hook_fd (lua_State *L)
{
    int fd, read, write, exception;
    const char *function, *data;
    char *result;

    API_FUNC(1, "hook_fd", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 6)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    fd = lua_tonumber (lua_current_interpreter, -6);
    read = lua_tonumber (lua_current_interpreter, -5);
    write = lua_tonumber (lua_current_interpreter, -4);
    exception = lua_tonumber (lua_current_interpreter, -3);
    function = lua_tostring (lua_current_interpreter, -2);
    data = lua_tostring (lua_current_interpreter, -1);

    result = API_PTR2STR(plugin_script_api_hook_fd (weechat_lua_plugin,
                                                    lua_current_script,
                                                    fd,
                                                    read,
                                                    write,
                                                    exception,
                                                    &weechat_lua_api_hook_fd_cb,
                                                    function,
                                                    data));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_hook_process_cb: callback for process hooked
 */

int
weechat_lua_api_hook_process_cb (void *data,
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

        rc = (int *) weechat_lua_exec (script_callback->script,
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
 * weechat_lua_api_hook_process: hook a process
 */

static int
weechat_lua_api_hook_process (lua_State *L)
{
    const char *command, *function, *data;
    int timeout;
    char *result;

    API_FUNC(1, "hook_process", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 4)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    command = lua_tostring (lua_current_interpreter, -4);
    timeout = lua_tonumber (lua_current_interpreter, -3);
    function = lua_tostring (lua_current_interpreter, -2);
    data = lua_tostring (lua_current_interpreter, -1);

    result = API_PTR2STR(plugin_script_api_hook_process (weechat_lua_plugin,
                                                         lua_current_script,
                                                         command,
                                                         timeout,
                                                         &weechat_lua_api_hook_process_cb,
                                                         function,
                                                         data));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_hook_process_hashtable: hook a process with options in
 *                                         a hashtable
 */

static int
weechat_lua_api_hook_process_hashtable (lua_State *L)
{
    const char *command, *function, *data;
    struct t_hashtable *options;
    int timeout;
    char *result;

    API_FUNC(1, "hook_process_hashtable", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 5)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    command = lua_tostring (lua_current_interpreter, -5);
    options = weechat_lua_tohashtable (lua_current_interpreter, -4,
                                       WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE);
    timeout = lua_tonumber (lua_current_interpreter, -3);
    function = lua_tostring (lua_current_interpreter, -2);
    data = lua_tostring (lua_current_interpreter, -1);

    result = API_PTR2STR(plugin_script_api_hook_process_hashtable (weechat_lua_plugin,
                                                                   lua_current_script,
                                                                   command,
                                                                   options,
                                                                   timeout,
                                                                   &weechat_lua_api_hook_process_cb,
                                                                   function,
                                                                   data));

    if (options)
        weechat_hashtable_free (options);

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_hook_connect_cb: callback for connect hooked
 */

int
weechat_lua_api_hook_connect_cb (void *data, int status, int gnutls_rc,
                                 const char *error, const char *ip_address)
{
    struct t_plugin_script_cb *script_callback;
    void *func_argv[5];
    char str_status[32], str_gnutls_rc[32];
    char empty_arg[1] = { '\0' };
    int *rc, ret;

    script_callback = (struct t_plugin_script_cb *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        snprintf (str_status, sizeof (str_status), "%d", status);
        snprintf (str_gnutls_rc, sizeof (str_gnutls_rc), "%d", gnutls_rc);

        func_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        func_argv[1] = str_status;
        func_argv[2] = str_gnutls_rc;
        func_argv[3] = (ip_address) ? (char *)ip_address : empty_arg;
        func_argv[4] = (error) ? (char *)error : empty_arg;

        rc = (int *) weechat_lua_exec (script_callback->script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       script_callback->function,
                                       "sssss", func_argv);

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
 * weechat_lua_api_hook_connect: hook a connection
 */

static int
weechat_lua_api_hook_connect (lua_State *L)
{
    const char *proxy, *address, *local_hostname, *function, *data;
    int port, sock, ipv6;
    char *result;

    API_FUNC(1, "hook_connect", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 8)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    proxy = lua_tostring (lua_current_interpreter, -8);
    address = lua_tostring (lua_current_interpreter, -7);
    port = lua_tonumber (lua_current_interpreter, -6);
    sock = lua_tonumber (lua_current_interpreter, -5);
    ipv6 = lua_tonumber (lua_current_interpreter, -4);
    local_hostname = lua_tostring (lua_current_interpreter, -3);
    function = lua_tostring (lua_current_interpreter, -2);
    data = lua_tostring (lua_current_interpreter, -1);

    result = API_PTR2STR(plugin_script_api_hook_connect (weechat_lua_plugin,
                                                         lua_current_script,
                                                         proxy,
                                                         address,
                                                         port,
                                                         sock,
                                                         ipv6,
                                                         NULL, /* gnutls session */
                                                         NULL, /* gnutls callback */
                                                         0,    /* gnutls DH key size */
                                                         NULL, /* gnutls priorities */
                                                         local_hostname,
                                                         &weechat_lua_api_hook_connect_cb,
                                                         function,
                                                         data));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_hook_print_cb: callback for print hooked
 */

int
weechat_lua_api_hook_print_cb (void *data, struct t_gui_buffer *buffer,
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

        rc = (int *) weechat_lua_exec (script_callback->script,
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
 * weechat_lua_api_hook_print: hook a print
 */

static int
weechat_lua_api_hook_print (lua_State *L)
{
    const char *buffer, *tags, *message, *function, *data;
    char *result;
    int strip_colors;

    API_FUNC(1, "hook_print", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 6)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = lua_tostring (lua_current_interpreter, -6);
    tags = lua_tostring (lua_current_interpreter, -5);
    message = lua_tostring (lua_current_interpreter, -4);
    strip_colors = lua_tonumber (lua_current_interpreter, -3);
    function = lua_tostring (lua_current_interpreter, -2);
    data = lua_tostring (lua_current_interpreter, -1);

    result = API_PTR2STR(plugin_script_api_hook_print (weechat_lua_plugin,
                                                       lua_current_script,
                                                       API_STR2PTR(buffer),
                                                       tags,
                                                       message,
                                                       strip_colors,
                                                       &weechat_lua_api_hook_print_cb,
                                                       function,
                                                       data));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_hook_signal_cb: callback for signal hooked
 */

int
weechat_lua_api_hook_signal_cb (void *data, const char *signal,
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

        rc = (int *) weechat_lua_exec (script_callback->script,
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
 * weechat_lua_api_hook_signal: hook a signal
 */

static int
weechat_lua_api_hook_signal (lua_State *L)
{
    const char *signal, *function, *data;
    char *result;

    API_FUNC(1, "hook_signal", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    signal = lua_tostring (lua_current_interpreter, -3);
    function = lua_tostring (lua_current_interpreter, -2);
    data = lua_tostring (lua_current_interpreter, -1);

    result = API_PTR2STR(plugin_script_api_hook_signal (weechat_lua_plugin,
                                                        lua_current_script,
                                                        signal,
                                                        &weechat_lua_api_hook_signal_cb,
                                                        function,
                                                        data));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_hook_signal_send: send a signal
 */

static int
weechat_lua_api_hook_signal_send (lua_State *L)
{
    const char *signal, *type_data, *signal_data;
    int number;

    API_FUNC(1, "hook_signal_send", API_RETURN_ERROR);
    signal_data = NULL;

    if (lua_gettop (lua_current_interpreter) < 3)
        API_WRONG_ARGS(API_RETURN_ERROR);

    signal = lua_tostring (lua_current_interpreter, -3);
    type_data = lua_tostring (lua_current_interpreter, -2);

    if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_STRING) == 0)
    {
        signal_data = lua_tostring (lua_current_interpreter, -1);
        weechat_hook_signal_send (signal, type_data, (void *)signal_data);
        API_RETURN_OK;
    }
    else if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_INT) == 0)
    {
        number = lua_tonumber (lua_current_interpreter, -1);
        weechat_hook_signal_send (signal, type_data, &number);
        API_RETURN_OK;
    }
    else if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_POINTER) == 0)
    {
        signal_data = lua_tostring (lua_current_interpreter, -1);
        weechat_hook_signal_send (signal, type_data,
                                  API_STR2PTR(signal_data));
        API_RETURN_OK;
    }

    API_RETURN_ERROR;
}

/*
 * weechat_lua_api_hook_hsignal_cb: callback for hsignal hooked
 */

int
weechat_lua_api_hook_hsignal_cb (void *data, const char *signal,
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

        rc = (int *) weechat_lua_exec (script_callback->script,
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
 * weechat_lua_api_hook_hsignal: hook a hsignal
 */

static int
weechat_lua_api_hook_hsignal (lua_State *L)
{
    const char *signal, *function, *data;
    char *result;

    API_FUNC(1, "hook_hsignal", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    signal = lua_tostring (lua_current_interpreter, -3);
    function = lua_tostring (lua_current_interpreter, -2);
    data = lua_tostring (lua_current_interpreter, -1);

    result = API_PTR2STR(plugin_script_api_hook_hsignal (weechat_lua_plugin,
                                                         lua_current_script,
                                                         signal,
                                                         &weechat_lua_api_hook_hsignal_cb,
                                                         function,
                                                         data));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_hook_hsignal_send: send a hsignal
 */

static int
weechat_lua_api_hook_hsignal_send (lua_State *L)
{
    const char *signal;
    struct t_hashtable *hashtable;

    API_FUNC(1, "hook_hsignal_send", API_RETURN_ERROR);
    if (lua_gettop (lua_current_interpreter) < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    signal = lua_tostring (lua_current_interpreter, -2);
    hashtable = weechat_lua_tohashtable (lua_current_interpreter, -1,
                                         WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE);

    weechat_hook_hsignal_send (signal, hashtable);

    if (hashtable)
        weechat_hashtable_free (hashtable);

    API_RETURN_OK;
}

/*
 * weechat_lua_api_hook_config_cb: callback for config option hooked
 */

int
weechat_lua_api_hook_config_cb (void *data, const char *option,
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
        func_argv[1] = (option) ? (char *)option : empty_arg;
        func_argv[2] = (value) ? (char *)value : empty_arg;

        rc = (int *) weechat_lua_exec (script_callback->script,
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
 * weechat_lua_api_hook_config: hook a config option
 */

static int
weechat_lua_api_hook_config (lua_State *L)
{
    const char *option, *function, *data;
    char *result;

    API_FUNC(1, "hook_config", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    option = lua_tostring (lua_current_interpreter, -3);
    function = lua_tostring (lua_current_interpreter, -2);
    data = lua_tostring (lua_current_interpreter, -1);

    result = API_PTR2STR(plugin_script_api_hook_config (weechat_lua_plugin,
                                                        lua_current_script,
                                                        option,
                                                        &weechat_lua_api_hook_config_cb,
                                                        function,
                                                        data));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_hook_completion_cb: callback for completion hooked
 */

int
weechat_lua_api_hook_completion_cb (void *data, const char *completion_item,
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

        rc = (int *) weechat_lua_exec (script_callback->script,
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
 * weechat_lua_api_hook_completion: hook a completion
 */

static int
weechat_lua_api_hook_completion (lua_State *L)
{
    const char *completion, *description, *function, *data;
    char *result;

    API_FUNC(1, "hook_completion", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 4)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    completion = lua_tostring (lua_current_interpreter, -4);
    description = lua_tostring (lua_current_interpreter, -3);
    function = lua_tostring (lua_current_interpreter, -2);
    data = lua_tostring (lua_current_interpreter, -1);

    result = API_PTR2STR(plugin_script_api_hook_completion (weechat_lua_plugin,
                                                            lua_current_script,
                                                            completion,
                                                            description,
                                                            &weechat_lua_api_hook_completion_cb,
                                                            function,
                                                            data));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_hook_completion_list_add: add a word to list for a completion
 */

static int
weechat_lua_api_hook_completion_list_add (lua_State *L)
{
    const char *completion, *word, *where;
    int nick_completion;

    API_FUNC(1, "hook_completion_list_add", API_RETURN_ERROR);
    if (lua_gettop (lua_current_interpreter) < 4)
        API_WRONG_ARGS(API_RETURN_ERROR);

    completion = lua_tostring (lua_current_interpreter, -4);
    word = lua_tostring (lua_current_interpreter, -3);
    nick_completion = lua_tonumber (lua_current_interpreter, -2);
    where = lua_tostring (lua_current_interpreter, -1);

    weechat_hook_completion_list_add (API_STR2PTR(completion),
                                      word,
                                      nick_completion,
                                      where);

    API_RETURN_OK;
}

/*
 * weechat_lua_api_hook_modifier_cb: callback for modifier hooked
 */

char *
weechat_lua_api_hook_modifier_cb (void *data, const char *modifier,
                                  const char *modifier_data,
                                  const char *string)
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

        return (char *)weechat_lua_exec (script_callback->script,
                                         WEECHAT_SCRIPT_EXEC_STRING,
                                         script_callback->function,
                                         "ssss", func_argv);
    }

    return NULL;
}

/*
 * weechat_lua_api_hook_modifier: hook a modifier
 */

static int
weechat_lua_api_hook_modifier (lua_State *L)
{
    const char *modifier, *function, *data;
    char *result;

    API_FUNC(1, "hook_modifier", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    modifier = lua_tostring (lua_current_interpreter, -3);
    function = lua_tostring (lua_current_interpreter, -2);
    data = lua_tostring (lua_current_interpreter, -1);

    result = API_PTR2STR(plugin_script_api_hook_modifier (weechat_lua_plugin,
                                                          lua_current_script,
                                                          modifier,
                                                          &weechat_lua_api_hook_modifier_cb,
                                                          function,
                                                          data));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_hook_modifier_exec: execute a modifier hook
 */

static int
weechat_lua_api_hook_modifier_exec (lua_State *L)
{
    const char *modifier, *modifier_data, *string;
    char *result;

    API_FUNC(1, "hook_modifier_exec", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 3)
        API_WRONG_ARGS(API_RETURN_ERROR);

    modifier = lua_tostring (lua_current_interpreter, -3);
    modifier_data = lua_tostring (lua_current_interpreter, -2);
    string = lua_tostring (lua_current_interpreter, -1);

    result = weechat_hook_modifier_exec (modifier, modifier_data, string);

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_hook_info_cb: callback for info hooked
 */

const char *
weechat_lua_api_hook_info_cb (void *data, const char *info_name,
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

        return (const char *)weechat_lua_exec (script_callback->script,
                                               WEECHAT_SCRIPT_EXEC_STRING,
                                               script_callback->function,
                                               "sss", func_argv);
    }

    return NULL;
}

/*
 * weechat_lua_api_hook_info: hook an info
 */

static int
weechat_lua_api_hook_info (lua_State *L)
{
    const char *info_name, *description, *args_description, *function, *data;
    char *result;

    API_FUNC(1, "hook_info", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 5)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    info_name = lua_tostring (lua_current_interpreter, -5);
    description = lua_tostring (lua_current_interpreter, -4);
    args_description = lua_tostring (lua_current_interpreter, -3);
    function = lua_tostring (lua_current_interpreter, -2);
    data = lua_tostring (lua_current_interpreter, -1);

    result = API_PTR2STR(plugin_script_api_hook_info (weechat_lua_plugin,
                                                      lua_current_script,
                                                      info_name,
                                                      description,
                                                      args_description,
                                                      &weechat_lua_api_hook_info_cb,
                                                      function,
                                                      data));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_hook_info_hashtable_cb: callback for info_hashtable hooked
 */

struct t_hashtable *
weechat_lua_api_hook_info_hashtable_cb (void *data, const char *info_name,
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

        return (struct t_hashtable *)weechat_lua_exec (script_callback->script,
                                                       WEECHAT_SCRIPT_EXEC_HASHTABLE,
                                                       script_callback->function,
                                                       "ssh", func_argv);
    }

    return NULL;
}

/*
 * weechat_lua_api_hook_info_hashtable: hook an info_hashtable
 */

static int
weechat_lua_api_hook_info_hashtable (lua_State *L)
{
    const char *info_name, *description, *args_description;
    const char *output_description, *function, *data;
    char *result;

    API_FUNC(1, "hook_info_hashtable", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 6)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    info_name = lua_tostring (lua_current_interpreter, -6);
    description = lua_tostring (lua_current_interpreter, -5);
    args_description = lua_tostring (lua_current_interpreter, -4);
    output_description = lua_tostring (lua_current_interpreter, -3);
    function = lua_tostring (lua_current_interpreter, -2);
    data = lua_tostring (lua_current_interpreter, -1);

    result = API_PTR2STR(plugin_script_api_hook_info_hashtable (weechat_lua_plugin,
                                                                lua_current_script,
                                                                info_name,
                                                                description,
                                                                args_description,
                                                                output_description,
                                                                &weechat_lua_api_hook_info_hashtable_cb,
                                                                function,
                                                                data));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_hook_infolist_cb: callback for infolist hooked
 */

struct t_infolist *
weechat_lua_api_hook_infolist_cb (void *data, const char *info_name,
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
        func_argv[1] = (info_name) ? (char *)info_name : empty_arg;
        func_argv[2] = API_PTR2STR(pointer);
        func_argv[3] = (arguments) ? (char *)arguments : empty_arg;

        result = (struct t_infolist *)weechat_lua_exec (script_callback->script,
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
 * weechat_lua_api_hook_infolist: hook an infolist
 */

static int
weechat_lua_api_hook_infolist (lua_State *L)
{
    const char *infolist_name, *description, *pointer_description;
    const char *args_description, *function, *data;
    char *result;

    API_FUNC(1, "hook_infolist", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 6)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    infolist_name = lua_tostring (lua_current_interpreter, -6);
    description = lua_tostring (lua_current_interpreter, -5);
    pointer_description = lua_tostring (lua_current_interpreter, -4);
    args_description = lua_tostring (lua_current_interpreter, -3);
    function = lua_tostring (lua_current_interpreter, -2);
    data = lua_tostring (lua_current_interpreter, -1);

    result = API_PTR2STR(plugin_script_api_hook_infolist (weechat_lua_plugin,
                                                          lua_current_script,
                                                          infolist_name,
                                                          description,
                                                          pointer_description,
                                                          args_description,
                                                          &weechat_lua_api_hook_infolist_cb,
                                                          function,
                                                          data));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_hook_focus_cb: callback for focus hooked
 */

struct t_hashtable *
weechat_lua_api_hook_focus_cb (void *data,
                               struct t_hashtable *info)
{
    struct t_plugin_script_cb *script_callback;
    void *func_argv[2];
    char empty_arg[1] = { '\0' };

    script_callback = (struct t_plugin_script_cb *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        func_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        func_argv[1] = info;

        return (struct t_hashtable *)weechat_lua_exec (script_callback->script,
                                                       WEECHAT_SCRIPT_EXEC_HASHTABLE,
                                                       script_callback->function,
                                                       "sh", func_argv);
    }

    return NULL;
}

/*
 * weechat_lua_api_hook_focus: hook a focus
 */

static int
weechat_lua_api_hook_focus (lua_State *L)
{
    const char *area, *function, *data;
    char *result;

    API_FUNC(1, "hook_focus", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    area = lua_tostring (lua_current_interpreter, -3);
    function = lua_tostring (lua_current_interpreter, -2);
    data = lua_tostring (lua_current_interpreter, -1);

    result = API_PTR2STR(plugin_script_api_hook_focus (weechat_lua_plugin,
                                                       lua_current_script,
                                                       area,
                                                       &weechat_lua_api_hook_focus_cb,
                                                       function,
                                                       data));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_unhook: unhook something
 */

static int
weechat_lua_api_unhook (lua_State *L)
{
    const char *hook;

    API_FUNC(1, "unhook", API_RETURN_ERROR);
    if (lua_gettop (lua_current_interpreter) < 1)
        API_WRONG_ARGS(API_RETURN_ERROR);

    hook = lua_tostring (lua_current_interpreter, -1);

    plugin_script_api_unhook (weechat_lua_plugin,
                              lua_current_script,
                              API_STR2PTR(hook));

    API_RETURN_OK;
}

/*
 * weechat_lua_api_unhook_all: unhook all for script
 */

static int
weechat_lua_api_unhook_all (lua_State *L)
{
    API_FUNC(1, "unhook_all", API_RETURN_ERROR);

    plugin_script_api_unhook_all (weechat_lua_plugin, lua_current_script);

    API_RETURN_OK;
}

/*
 * weechat_lua_api_buffer_input_data_cb: callback for input data in a buffer
 */

int
weechat_lua_api_buffer_input_data_cb (void *data, struct t_gui_buffer *buffer,
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

        rc = (int *) weechat_lua_exec (script_callback->script,
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
 * weechat_lua_api_buffer_close_cb: callback for buffer closed
 */

int
weechat_lua_api_buffer_close_cb (void *data, struct t_gui_buffer *buffer)
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

        rc = (int *) weechat_lua_exec (script_callback->script,
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
 * weechat_lua_api_buffer_new: create a new buffer
 */

static int
weechat_lua_api_buffer_new (lua_State *L)
{
    const char *name, *function_input, *data_input, *function_close;
    const char *data_close;
    char *result;

    API_FUNC(1, "buffer_new", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 5)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    name = lua_tostring (lua_current_interpreter, -5);
    function_input = lua_tostring (lua_current_interpreter, -4);
    data_input = lua_tostring (lua_current_interpreter, -3);
    function_close = lua_tostring (lua_current_interpreter, -2);
    data_close = lua_tostring (lua_current_interpreter, -1);

    result = API_PTR2STR(plugin_script_api_buffer_new (weechat_lua_plugin,
                                                       lua_current_script,
                                                       name,
                                                       &weechat_lua_api_buffer_input_data_cb,
                                                       function_input,
                                                       data_input,
                                                       &weechat_lua_api_buffer_close_cb,
                                                       function_close,
                                                       data_close));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_buffer_search: search a buffer
 */

static int
weechat_lua_api_buffer_search (lua_State *L)
{
    const char *plugin, *name;
    char *result;

    API_FUNC(1, "buffer_search", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    plugin = lua_tostring (lua_current_interpreter, -2);
    name = lua_tostring (lua_current_interpreter, -1);

    result = API_PTR2STR(weechat_buffer_search (plugin, name));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_buffer_search_main: search main buffer (WeeChat core buffer)
 */

static int
weechat_lua_api_buffer_search_main (lua_State *L)
{
    char *result;

    API_FUNC(1, "buffer_search_main", API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_buffer_search_main ());

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_current_buffer: get current buffer
 */

static int
weechat_lua_api_current_buffer (lua_State *L)
{
    char *result;

    API_FUNC(1, "current_buffer", API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_current_buffer ());

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_buffer_clear: clear a buffer
 */

static int
weechat_lua_api_buffer_clear (lua_State *L)
{
    const char *buffer;

    API_FUNC(1, "buffer_clear", API_RETURN_ERROR);
    if (lua_gettop (lua_current_interpreter) < 1)
        API_WRONG_ARGS(API_RETURN_ERROR);

    buffer = lua_tostring (lua_current_interpreter, -1);

    weechat_buffer_clear (API_STR2PTR(buffer));

    API_RETURN_OK;
}

/*
 * weechat_lua_api_buffer_close: close a buffer
 */

static int
weechat_lua_api_buffer_close (lua_State *L)
{
    const char *buffer;

    API_FUNC(1, "buffer_close", API_RETURN_ERROR);
    if (lua_gettop (lua_current_interpreter) < 1)
        API_WRONG_ARGS(API_RETURN_ERROR);

    buffer = lua_tostring (lua_current_interpreter, -1);

    plugin_script_api_buffer_close (weechat_lua_plugin,
                                    lua_current_script,
                                    API_STR2PTR(buffer));

    API_RETURN_OK;
}

/*
 * weechat_lua_api_buffer_merge: merge a buffer to another buffer
 */

static int
weechat_lua_api_buffer_merge (lua_State *L)
{
    const char *buffer, *target_buffer;

    API_FUNC(1, "buffer_merge", API_RETURN_ERROR);
    if (lua_gettop (lua_current_interpreter) < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    buffer = lua_tostring (lua_current_interpreter, -2);
    target_buffer = lua_tostring (lua_current_interpreter, -1);

    weechat_buffer_merge (API_STR2PTR(buffer),
                          API_STR2PTR(target_buffer));

    API_RETURN_OK;
}

/*
 * weechat_lua_api_buffer_unmerge: unmerge a buffer from a group of merged
 *                                 buffers
 */

static int
weechat_lua_api_buffer_unmerge (lua_State *L)
{
    const char *buffer;
    int number;

    API_FUNC(1, "buffer_unmerge", API_RETURN_ERROR);
    if (lua_gettop (lua_current_interpreter) < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    buffer = lua_tostring (lua_current_interpreter, -2);
    number = lua_tonumber (lua_current_interpreter, -1);

    weechat_buffer_unmerge (API_STR2PTR(buffer), number);

    API_RETURN_OK;
}

/*
 * weechat_lua_api_buffer_get_integer: get a buffer property as integer
 */

static int
weechat_lua_api_buffer_get_integer (lua_State *L)
{
    const char *buffer, *property;
    int value;

    API_FUNC(1, "buffer_get_integer", API_RETURN_INT(-1));
    if (lua_gettop (lua_current_interpreter) < 2)
        API_WRONG_ARGS(API_RETURN_INT(-1));

    buffer = lua_tostring (lua_current_interpreter, -2);
    property = lua_tostring (lua_current_interpreter, -1);

    value = weechat_buffer_get_integer (API_STR2PTR(buffer),
                                        property);

    API_RETURN_INT(value);
}

/*
 * weechat_lua_api_buffer_get_string: get a buffer property as string
 */

static int
weechat_lua_api_buffer_get_string (lua_State *L)
{
    const char *buffer, *property, *result;

    API_FUNC(1, "buffer_get_string", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = lua_tostring (lua_current_interpreter, -2);
    property = lua_tostring (lua_current_interpreter, -1);

    result = weechat_buffer_get_string (API_STR2PTR(buffer),
                                        property);

    API_RETURN_STRING(result);
}

/*
 * weechat_lua_api_buffer_get_pointer: get a buffer property as pointer
 */

static int
weechat_lua_api_buffer_get_pointer (lua_State *L)
{
    const char *buffer, *property;
    char *result;

    API_FUNC(1, "buffer_get_pointer", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = lua_tostring (lua_current_interpreter, -2);
    property = lua_tostring (lua_current_interpreter, -1);

    result = API_PTR2STR(weechat_buffer_get_pointer (API_STR2PTR(buffer),
                                                     property));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_buffer_set: set a buffer property
 */

static int
weechat_lua_api_buffer_set (lua_State *L)
{
    const char *buffer, *property, *value;

    API_FUNC(1, "buffer_set", API_RETURN_ERROR);
    if (lua_gettop (lua_current_interpreter) < 3)
        API_WRONG_ARGS(API_RETURN_ERROR);

    buffer = lua_tostring (lua_current_interpreter, -3);
    property = lua_tostring (lua_current_interpreter, -2);
    value = lua_tostring (lua_current_interpreter, -1);

    weechat_buffer_set (API_STR2PTR(buffer), property, value);

    API_RETURN_OK;
}

/*
 * weechat_lua_api_buffer_string_replace_local_var: replace local variables ($var) in a string,
 *                                                  using value of local variables
 */

static int
weechat_lua_api_buffer_string_replace_local_var (lua_State *L)
{
    const char *buffer, *string;
    char *result;

    API_FUNC(1, "buffer_string_replace_local_var", API_RETURN_ERROR);
    if (lua_gettop (lua_current_interpreter) < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    buffer = lua_tostring (lua_current_interpreter, -2);
    string = lua_tostring (lua_current_interpreter, -1);

    result = weechat_buffer_string_replace_local_var (API_STR2PTR(buffer), string);

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_buffer_match_list: return 1 if buffer matches list of buffers
 */

static int
weechat_lua_api_buffer_match_list (lua_State *L)
{
    const char *buffer, *string;
    int value;

    API_FUNC(1, "buffer_match_list", API_RETURN_INT(0));
    if (lua_gettop (lua_current_interpreter) < 2)
        API_WRONG_ARGS(API_RETURN_INT(0));

    buffer = lua_tostring (lua_current_interpreter, -2);
    string = lua_tostring (lua_current_interpreter, -1);

    value = weechat_buffer_match_list (API_STR2PTR(buffer),
                                       string);

    API_RETURN_INT(value);
}

/*
 * weechat_lua_api_current_window: get current window
 */

static int
weechat_lua_api_current_window (lua_State *L)
{
    char *result;

    API_FUNC(1, "current_window", API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_current_window ());

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_window_search_with_buffer: search a window with buffer
 *                                            pointer
 */

static int
weechat_lua_api_window_search_with_buffer (lua_State *L)
{
    const char *buffer;
    char *result;

    API_FUNC(1, "window_search_with_buffer", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 1)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = lua_tostring (lua_current_interpreter, -1);

    result = API_PTR2STR(weechat_window_search_with_buffer (API_STR2PTR(buffer)));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_window_get_integer: get a window property as integer
 */

static int
weechat_lua_api_window_get_integer (lua_State *L)
{
    const char *window, *property;
    int value;

    API_FUNC(1, "window_get_integer", API_RETURN_INT(-1));
    if (lua_gettop (lua_current_interpreter) < 2)
        API_WRONG_ARGS(API_RETURN_INT(-1));

    window = lua_tostring (lua_current_interpreter, -2);
    property = lua_tostring (lua_current_interpreter, -1);

    value = weechat_window_get_integer (API_STR2PTR(window),
                                        property);

    API_RETURN_INT(value);
}

/*
 * weechat_lua_api_window_get_string: get a window property as string
 */

static int
weechat_lua_api_window_get_string (lua_State *L)
{
    const char *window, *property, *result;

    API_FUNC(1, "window_get_string", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    window = lua_tostring (lua_current_interpreter, -2);
    property = lua_tostring (lua_current_interpreter, -1);

    result = weechat_window_get_string (API_STR2PTR(window),
                                        property);

    API_RETURN_STRING(result);
}

/*
 * weechat_lua_api_window_get_pointer: get a window property as pointer
 */

static int
weechat_lua_api_window_get_pointer (lua_State *L)
{
    const char *window, *property;
    char *result;

    API_FUNC(1, "window_get_pointer", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    window = lua_tostring (lua_current_interpreter, -2);
    property = lua_tostring (lua_current_interpreter, -1);

    result = API_PTR2STR(weechat_window_get_pointer (API_STR2PTR(window),
                                                     property));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_window_set_title: set window title
 */

static int
weechat_lua_api_window_set_title (lua_State *L)
{
    const char *title;

    API_FUNC(1, "window_set_title", API_RETURN_ERROR);
    if (lua_gettop (lua_current_interpreter) < 1)
        API_WRONG_ARGS(API_RETURN_ERROR);

    title = lua_tostring (lua_current_interpreter, -1);

    weechat_window_set_title (title);

    API_RETURN_OK;
}

/*
 * weechat_lua_api_nicklist_add_group: add a group in nicklist
 */

static int
weechat_lua_api_nicklist_add_group (lua_State *L)
{
    const char *buffer, *parent_group, *name, *color;
    char *result;
    int visible;

    API_FUNC(1, "nicklist_add_group", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 5)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = lua_tostring (lua_current_interpreter, -5);
    parent_group = lua_tostring (lua_current_interpreter, -4);
    name = lua_tostring (lua_current_interpreter, -3);
    color = lua_tostring (lua_current_interpreter, -2);
    visible = lua_tonumber (lua_current_interpreter, -1);

    result = API_PTR2STR(weechat_nicklist_add_group (API_STR2PTR(buffer),
                                                     API_STR2PTR(parent_group),
                                                     name,
                                                     color,
                                                     visible));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_nicklist_search_group: search a group in nicklist
 */

static int
weechat_lua_api_nicklist_search_group (lua_State *L)
{
    const char *buffer, *from_group, *name;
    char *result;

    API_FUNC(1, "nicklist_search_group", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = lua_tostring (lua_current_interpreter, -3);
    from_group = lua_tostring (lua_current_interpreter, -2);
    name = lua_tostring (lua_current_interpreter, -1);

    result = API_PTR2STR(weechat_nicklist_search_group (API_STR2PTR(buffer),
                                                        API_STR2PTR(from_group),
                                                        name));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_nicklist_add_nick: add a nick in nicklist
 */

static int
weechat_lua_api_nicklist_add_nick (lua_State *L)
{
    const char *buffer, *group, *name, *color, *prefix, *prefix_color;
    char *result;
    int visible;

    API_FUNC(1, "nicklist_add_nick", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 7)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = lua_tostring (lua_current_interpreter, -7);
    group = lua_tostring (lua_current_interpreter, -6);
    name = lua_tostring (lua_current_interpreter, -5);
    color = lua_tostring (lua_current_interpreter, -4);
    prefix = lua_tostring (lua_current_interpreter, -3);
    prefix_color = lua_tostring (lua_current_interpreter, -2);
    visible = lua_tonumber (lua_current_interpreter, -1);

    result = API_PTR2STR(weechat_nicklist_add_nick (API_STR2PTR(buffer),
                                                    API_STR2PTR(group),
                                                    name,
                                                    color,
                                                    prefix,
                                                    prefix_color,
                                                    visible));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_nicklist_search_nick: search a nick in nicklist
 */

static int
weechat_lua_api_nicklist_search_nick (lua_State *L)
{
    const char *buffer, *from_group, *name;
    char *result;

    API_FUNC(1, "nicklist_search_nick", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = lua_tostring (lua_current_interpreter, -3);
    from_group = lua_tostring (lua_current_interpreter, -2);
    name = lua_tostring (lua_current_interpreter, -1);

    result = API_PTR2STR(weechat_nicklist_search_nick (API_STR2PTR(buffer),
                                                       API_STR2PTR(from_group),
                                                       name));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_nicklist_remove_group: remove a group from nicklist
 */

static int
weechat_lua_api_nicklist_remove_group (lua_State *L)
{
    const char *buffer, *group;

    API_FUNC(1, "nicklist_remove_group", API_RETURN_ERROR);
    if (lua_gettop (lua_current_interpreter) < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    buffer = lua_tostring (lua_current_interpreter, -3);
    group = lua_tostring (lua_current_interpreter, -2);

    weechat_nicklist_remove_group (API_STR2PTR(buffer),
                                   API_STR2PTR(group));

    API_RETURN_OK;
}

/*
 * weechat_lua_api_nicklist_remove_nick: remove a nick from nicklist
 */

static int
weechat_lua_api_nicklist_remove_nick (lua_State *L)
{
    const char *buffer, *nick;

    API_FUNC(1, "nicklist_remove_nick", API_RETURN_ERROR);
    if (lua_gettop (lua_current_interpreter) < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    buffer = lua_tostring (lua_current_interpreter, -3);
    nick = lua_tostring (lua_current_interpreter, -2);

    weechat_nicklist_remove_nick (API_STR2PTR(buffer),
                                  API_STR2PTR(nick));

    API_RETURN_OK;
}

/*
 * weechat_lua_api_nicklist_remove_all: remove all groups/nicks from nicklist
 */

static int
weechat_lua_api_nicklist_remove_all (lua_State *L)
{
    const char *buffer;

    API_FUNC(1, "nicklist_remove_all", API_RETURN_ERROR);
    if (lua_gettop (lua_current_interpreter) < 1)
        API_WRONG_ARGS(API_RETURN_ERROR);

    buffer = lua_tostring (lua_current_interpreter, -3);

    weechat_nicklist_remove_all (API_STR2PTR(buffer));

    API_RETURN_OK;
}

/*
 * weechat_lua_api_nicklist_group_get_integer: get a group property as integer
 */

static int
weechat_lua_api_nicklist_group_get_integer (lua_State *L)
{
    const char *buffer, *group, *property;
    int value;

    API_FUNC(1, "nicklist_group_get_integer", API_RETURN_INT(-1));
    if (lua_gettop (lua_current_interpreter) < 3)
        API_WRONG_ARGS(API_RETURN_INT(-1));

    buffer = lua_tostring (lua_current_interpreter, -3);
    group = lua_tostring (lua_current_interpreter, -2);
    property = lua_tostring (lua_current_interpreter, -1);

    value = weechat_nicklist_group_get_integer (API_STR2PTR(buffer),
                                                API_STR2PTR(group),
                                                property);

    API_RETURN_INT(value);
}

/*
 * weechat_lua_api_nicklist_group_get_string: get a group property as string
 */

static int
weechat_lua_api_nicklist_group_get_string (lua_State *L)
{
    const char *buffer, *group, *property, *result;

    API_FUNC(1, "nicklist_group_get_string", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = lua_tostring (lua_current_interpreter, -3);
    group = lua_tostring (lua_current_interpreter, -2);
    property = lua_tostring (lua_current_interpreter, -1);

    result = weechat_nicklist_group_get_string (API_STR2PTR(buffer),
                                                API_STR2PTR(group),
                                                property);

    API_RETURN_STRING(result);
}

/*
 * weechat_lua_api_nicklist_group_get_pointer: get a group property as pointer
 */

static int
weechat_lua_api_nicklist_group_get_pointer (lua_State *L)
{
    const char *buffer, *group, *property;
    char *result;

    API_FUNC(1, "nicklist_group_get_pointer", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = lua_tostring (lua_current_interpreter, -3);
    group = lua_tostring (lua_current_interpreter, -2);
    property = lua_tostring (lua_current_interpreter, -1);

    result = API_PTR2STR(weechat_nicklist_group_get_pointer (API_STR2PTR(buffer),
                                                             API_STR2PTR(group),
                                                             property));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_nicklist_group_set: set a group property
 */

static int
weechat_lua_api_nicklist_group_set (lua_State *L)
{
    const char *buffer, *group, *property, *value;

    API_FUNC(1, "nicklist_group_set", API_RETURN_ERROR);
    if (lua_gettop (lua_current_interpreter) < 4)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = lua_tostring (lua_current_interpreter, -4);
    group = lua_tostring (lua_current_interpreter, -3);
    property = lua_tostring (lua_current_interpreter, -2);
    value = lua_tostring (lua_current_interpreter, -1);

    weechat_nicklist_group_set (API_STR2PTR(buffer),
                                API_STR2PTR(group),
                                property,
                                value);

    API_RETURN_OK;
}

/*
 * weechat_lua_api_nicklist_nick_get_integer: get a nick property as integer
 */

static int
weechat_lua_api_nicklist_nick_get_integer (lua_State *L)
{
    const char *buffer, *nick, *property;
    int value;

    API_FUNC(1, "nicklist_nick_get_integer", API_RETURN_INT(-1));
    if (lua_gettop (lua_current_interpreter) < 3)
        API_WRONG_ARGS(API_RETURN_INT(-1));

    buffer = lua_tostring (lua_current_interpreter, -3);
    nick = lua_tostring (lua_current_interpreter, -2);
    property = lua_tostring (lua_current_interpreter, -1);

    value = weechat_nicklist_nick_get_integer (API_STR2PTR(buffer),
                                               API_STR2PTR(nick),
                                               property);

    API_RETURN_INT(value);
}

/*
 * weechat_lua_api_nicklist_nick_get_string: get a nick property as string
 */

static int
weechat_lua_api_nicklist_nick_get_string (lua_State *L)
{
    const char *buffer, *nick, *property, *result;

    API_FUNC(1, "nicklist_nick_get_string", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = lua_tostring (lua_current_interpreter, -3);
    nick = lua_tostring (lua_current_interpreter, -2);
    property = lua_tostring (lua_current_interpreter, -1);

    result = weechat_nicklist_nick_get_string (API_STR2PTR(buffer),
                                               API_STR2PTR(nick),
                                               property);

    API_RETURN_STRING(result);
}

/*
 * weechat_lua_api_nicklist_nick_get_pointer: get a nick property as pointer
 */

static int
weechat_lua_api_nicklist_nick_get_pointer (lua_State *L)
{
    const char *buffer, *nick, *property;
    char *result;

    API_FUNC(1, "nicklist_nick_get_pointer", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = lua_tostring (lua_current_interpreter, -3);
    nick = lua_tostring (lua_current_interpreter, -2);
    property = lua_tostring (lua_current_interpreter, -1);

    result = API_PTR2STR(weechat_nicklist_nick_get_pointer (API_STR2PTR(buffer),
                                                            API_STR2PTR(nick),
                                                            property));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_nicklist_nick_set: set a nick property
 */

static int
weechat_lua_api_nicklist_nick_set (lua_State *L)
{
    const char *buffer, *nick, *property, *value;

    API_FUNC(1, "nicklist_nick_set", API_RETURN_ERROR);
    if (lua_gettop (lua_current_interpreter) < 4)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    buffer = lua_tostring (lua_current_interpreter, -4);
    nick = lua_tostring (lua_current_interpreter, -3);
    property = lua_tostring (lua_current_interpreter, -2);
    value = lua_tostring (lua_current_interpreter, -1);

    weechat_nicklist_nick_set (API_STR2PTR(buffer),
                               API_STR2PTR(nick),
                               property,
                               value);

    API_RETURN_OK;
}

/*
 * weechat_lua_api_bar_item_search: search a bar item
 */

static int
weechat_lua_api_bar_item_search (lua_State *L)
{
    const char *name;
    char *result;

    API_FUNC(1, "bar_item_search", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 1)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    name = lua_tostring (lua_current_interpreter, -1);

    result = API_PTR2STR(weechat_bar_item_search (name));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_bar_item_build_cb: callback for building bar item
 */

char *
weechat_lua_api_bar_item_build_cb (void *data, struct t_gui_bar_item *item,
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

        ret = (char *)weechat_lua_exec (script_callback->script,
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
 * weechat_lua_api_bar_item_new: add a new bar item
 */

static int
weechat_lua_api_bar_item_new (lua_State *L)
{
    const char *name, *function, *data;
    char *result;

    API_FUNC(1, "bar_item_new", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    name = lua_tostring (lua_current_interpreter, -3);
    function = lua_tostring (lua_current_interpreter, -2);
    data = lua_tostring (lua_current_interpreter, -1);

    result = API_PTR2STR(plugin_script_api_bar_item_new (weechat_lua_plugin,
                                                         lua_current_script,
                                                         name,
                                                         &weechat_lua_api_bar_item_build_cb,
                                                         function,
                                                         data));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_bar_item_update: update a bar item on screen
 */

static int
weechat_lua_api_bar_item_update (lua_State *L)
{
    const char *name;

    API_FUNC(1, "bar_item_update", API_RETURN_ERROR);
    if (lua_gettop (lua_current_interpreter) < 1)
        API_WRONG_ARGS(API_RETURN_ERROR);

    name = lua_tostring (lua_current_interpreter, -1);

    weechat_bar_item_update (name);

    API_RETURN_OK;
}

/*
 * weechat_lua_api_bar_item_remove: remove a bar item
 */

static int
weechat_lua_api_bar_item_remove (lua_State *L)
{
    const char *item;

    API_FUNC(1, "bar_item_remove", API_RETURN_ERROR);
    if (lua_gettop (lua_current_interpreter) < 1)
        API_WRONG_ARGS(API_RETURN_ERROR);

    item = lua_tostring (lua_current_interpreter, -1);

    plugin_script_api_bar_item_remove (weechat_lua_plugin,
                                       lua_current_script,
                                       API_STR2PTR(item));

    API_RETURN_OK;
}

/*
 * weechat_lua_api_bar_search: search a bar
 */

static int
weechat_lua_api_bar_search (lua_State *L)
{
    const char *name;
    char *result;

    API_FUNC(1, "bar_search", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 1)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    name = lua_tostring (lua_current_interpreter, -1);

    result = API_PTR2STR(weechat_bar_search (name));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_bar_new: add a new bar
 */

static int
weechat_lua_api_bar_new (lua_State *L)
{
    const char *name, *hidden, *priority, *type, *conditions, *position;
    const char *filling_top_bottom, *filling_left_right, *size, *size_max;
    const char *color_fg, *color_delim, *color_bg, *separator, *items;
    char *result;

    API_FUNC(1, "bar_new", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 15)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    name = lua_tostring (lua_current_interpreter, -15);
    hidden = lua_tostring (lua_current_interpreter, -14);
    priority = lua_tostring (lua_current_interpreter, -13);
    type = lua_tostring (lua_current_interpreter, -12);
    conditions = lua_tostring (lua_current_interpreter, -11);
    position = lua_tostring (lua_current_interpreter, -10);
    filling_top_bottom = lua_tostring (lua_current_interpreter, -9);
    filling_left_right = lua_tostring (lua_current_interpreter, -8);
    size = lua_tostring (lua_current_interpreter, -7);
    size_max = lua_tostring (lua_current_interpreter, -6);
    color_fg = lua_tostring (lua_current_interpreter, -5);
    color_delim = lua_tostring (lua_current_interpreter, -4);
    color_bg = lua_tostring (lua_current_interpreter, -3);
    separator = lua_tostring (lua_current_interpreter, -2);
    items = lua_tostring (lua_current_interpreter, -1);

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
                                          separator,
                                          items));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_bar_set: set a bar property
 */

static int
weechat_lua_api_bar_set (lua_State *L)
{
    const char *bar, *property, *value;

    API_FUNC(1, "bar_set", API_RETURN_ERROR);
    if (lua_gettop (lua_current_interpreter) < 3)
        API_WRONG_ARGS(API_RETURN_ERROR);

    bar = lua_tostring (lua_current_interpreter, -3);
    property = lua_tostring (lua_current_interpreter, -2);
    value = lua_tostring (lua_current_interpreter, -1);

    weechat_bar_set (API_STR2PTR(bar),
                     property,
                     value);

    API_RETURN_OK;
}

/*
 * weechat_lua_api_bar_update: update a bar on screen
 */

static int
weechat_lua_api_bar_update (lua_State *L)
{
    const char *name;

    API_FUNC(1, "bar_update", API_RETURN_ERROR);
    if (lua_gettop (lua_current_interpreter) < 1)
        API_WRONG_ARGS(API_RETURN_ERROR);

    name = lua_tostring (lua_current_interpreter, -1);

    weechat_bar_update (name);

    API_RETURN_OK;
}

/*
 * weechat_lua_api_bar_remove: remove a bar
 */

static int
weechat_lua_api_bar_remove (lua_State *L)
{
    const char *bar;

    API_FUNC(1, "bar_remove", API_RETURN_ERROR);
    if (lua_gettop (lua_current_interpreter) < 1)
        API_WRONG_ARGS(API_RETURN_ERROR);

    bar = lua_tostring (lua_current_interpreter, -1);

    weechat_bar_remove (API_STR2PTR(bar));

    API_RETURN_OK;
}

/*
 * weechat_lua_api_command: send command to server
 */

static int
weechat_lua_api_command (lua_State *L)
{
    const char *buffer, *command;

    API_FUNC(1, "command", API_RETURN_ERROR);
    if (lua_gettop (lua_current_interpreter) < 2)
        API_WRONG_ARGS(API_RETURN_ERROR);

    buffer = lua_tostring (lua_current_interpreter, -2);
    command = lua_tostring (lua_current_interpreter, -1);

    plugin_script_api_command (weechat_lua_plugin,
                               lua_current_script,
                               API_STR2PTR(buffer),
                               command);

    API_RETURN_OK;
}

/*
 * weechat_lua_api_info_get: get info (as string)
 */

static int
weechat_lua_api_info_get (lua_State *L)
{
    const char *info_name, *arguments, *result;

    API_FUNC(1, "info_get", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    info_name = lua_tostring (lua_current_interpreter, -2);
    arguments = lua_tostring (lua_current_interpreter, -1);

    result = weechat_info_get (info_name, arguments);

    API_RETURN_STRING(result);
}

/*
 * weechat_lua_api_info_get_hashtable: get info (as hashtable)
 */

static int
weechat_lua_api_info_get_hashtable (lua_State *L)
{
    const char *info_name;
    struct t_hashtable *table, *result_hashtable;

    API_FUNC(1, "info_get_hashtable", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    info_name = lua_tostring (lua_current_interpreter, -2);
    table = weechat_lua_tohashtable (lua_current_interpreter, -1,
                                     WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE);

    result_hashtable = weechat_info_get_hashtable (info_name, table);

    weechat_lua_pushhashtable (lua_current_interpreter, result_hashtable);

    if (table)
        weechat_hashtable_free (table);
    if (result_hashtable)
        weechat_hashtable_free (result_hashtable);

    return 1;
}

/*
 * weechat_lua_api_infolist_new: create new infolist
 */

static int
weechat_lua_api_infolist_new (lua_State *L)
{
    char *result;

    API_FUNC(1, "infolist_new", API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_infolist_new ());

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_infolist_new_item: create new item in infolist
 */

static int
weechat_lua_api_infolist_new_item (lua_State *L)
{
    const char *infolist;
    char *result;

    API_FUNC(1, "infolist_new_item", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 1)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    infolist = lua_tostring (lua_current_interpreter, -1);

    result = API_PTR2STR(weechat_infolist_new_item (API_STR2PTR(infolist)));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_infolist_new_var_integer: create new integer variable in
 *                                           infolist
 */

static int
weechat_lua_api_infolist_new_var_integer (lua_State *L)
{
    const char *infolist, *name;
    char *result;
    int value;

    API_FUNC(1, "infolist_new_var_integer", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    infolist = lua_tostring (lua_current_interpreter, -3);
    name = lua_tostring (lua_current_interpreter, -2);
    value = lua_tonumber (lua_current_interpreter, -1);

    result = API_PTR2STR(weechat_infolist_new_var_integer (API_STR2PTR(infolist),
                                                           name,
                                                           value));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_infolist_new_var_string: create new string variable in
 *                                          infolist
 */

static int
weechat_lua_api_infolist_new_var_string (lua_State *L)
{
    const char *infolist, *name, *value;
    char *result;

    API_FUNC(1, "infolist_new_var_string", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    infolist = lua_tostring (lua_current_interpreter, -3);
    name = lua_tostring (lua_current_interpreter, -2);
    value = lua_tostring (lua_current_interpreter, -1);

    result = API_PTR2STR(weechat_infolist_new_var_string (API_STR2PTR(infolist),
                                                          name,
                                                          value));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_infolist_new_var_pointer: create new pointer variable in
 *                                           infolist
 */

static int
weechat_lua_api_infolist_new_var_pointer (lua_State *L)
{
    const char *infolist, *name, *value;
    char *result;

    API_FUNC(1, "infolist_new_var_pointer", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    infolist = lua_tostring (lua_current_interpreter, -3);
    name = lua_tostring (lua_current_interpreter, -2);
    value = lua_tostring (lua_current_interpreter, -1);

    result = API_PTR2STR(weechat_infolist_new_var_pointer (API_STR2PTR(infolist),
                                                           name,
                                                           API_STR2PTR(value)));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_infolist_new_var_time: create new time variable in infolist
 */

static int
weechat_lua_api_infolist_new_var_time (lua_State *L)
{
    const char *infolist, *name;
    char *result;
    int value;

    API_FUNC(1, "infolist_new_var_time", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    infolist = lua_tostring (lua_current_interpreter, -3);
    name = lua_tostring (lua_current_interpreter, -2);
    value = lua_tonumber (lua_current_interpreter, -1);

    result = API_PTR2STR(weechat_infolist_new_var_time (API_STR2PTR(infolist),
                                                        name,
                                                        value));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_infolist_get: get list with infos
 */

static int
weechat_lua_api_infolist_get (lua_State *L)
{
    const char *name, *pointer, *arguments;
    char *result;

    API_FUNC(1, "infolist_get", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    name = lua_tostring (lua_current_interpreter, -3);
    pointer = lua_tostring (lua_current_interpreter, -2);
    arguments = lua_tostring (lua_current_interpreter, -1);

    result = API_PTR2STR(weechat_infolist_get (name,
                                               API_STR2PTR(pointer),
                                               arguments));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_infolist_next: move item pointer to next item in infolist
 */

static int
weechat_lua_api_infolist_next (lua_State *L)
{
    const char *infolist;
    int value;

    API_FUNC(1, "infolist_next", API_RETURN_INT(0));
    if (lua_gettop (lua_current_interpreter) < 1)
        API_WRONG_ARGS(API_RETURN_INT(0));

    infolist = lua_tostring (lua_current_interpreter, -1);

    value = weechat_infolist_next (API_STR2PTR(infolist));

    API_RETURN_INT(value);
}

/*
 * weechat_lua_api_infolist_prev: move item pointer to previous item in infolist
 */

static int
weechat_lua_api_infolist_prev (lua_State *L)
{
    const char *infolist;
    int value;

    API_FUNC(1, "infolist_prev", API_RETURN_INT(0));
    if (lua_gettop (lua_current_interpreter) < 1)
        API_WRONG_ARGS(API_RETURN_INT(0));

    infolist = lua_tostring (lua_current_interpreter, -1);

    value = weechat_infolist_prev (API_STR2PTR(infolist));

    API_RETURN_INT(value);
}

/*
 * weechat_lua_api_infolist_reset_item_cursor: reset pointer to current item in
 *                                             infolist
 */

static int
weechat_lua_api_infolist_reset_item_cursor (lua_State *L)
{
    const char *infolist;

    API_FUNC(1, "infolist_reset_item_cursor", API_RETURN_ERROR);
    if (lua_gettop (lua_current_interpreter) < 1)
        API_WRONG_ARGS(API_RETURN_ERROR);

    infolist = lua_tostring (lua_current_interpreter, -1);

    weechat_infolist_reset_item_cursor (API_STR2PTR(infolist));

    API_RETURN_OK;
}

/*
 * weechat_lua_api_infolist_fields: get list of fields for current item of infolist
 */

static int
weechat_lua_api_infolist_fields (lua_State *L)
{
    const char *infolist, *result;

    API_FUNC(1, "infolist_fields", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 1)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    infolist = lua_tostring (lua_current_interpreter, -1);

    result = weechat_infolist_fields (API_STR2PTR(infolist));

    API_RETURN_STRING(result);
}

/*
 * weechat_lua_api_infolist_integer: get integer value of a variable in infolist
 */

static int
weechat_lua_api_infolist_integer (lua_State *L)
{
    const char *infolist, *variable;
    int value;

    API_FUNC(1, "infolist_integer", API_RETURN_INT(0));
    if (lua_gettop (lua_current_interpreter) < 2)
        API_WRONG_ARGS(API_RETURN_INT(0));

    infolist = lua_tostring (lua_current_interpreter, -2);
    variable = lua_tostring (lua_current_interpreter, -1);

    value = weechat_infolist_integer (API_STR2PTR(infolist),
                                      variable);

    API_RETURN_INT(value);
}

/*
 * weechat_lua_api_infolist_string: get string value of a variable in infolist
 */

static int
weechat_lua_api_infolist_string (lua_State *L)
{
    const char *infolist, *variable, *result;

    API_FUNC(1, "infolist_string", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    infolist = lua_tostring (lua_current_interpreter, -2);
    variable = lua_tostring (lua_current_interpreter, -1);

    result = weechat_infolist_string (API_STR2PTR(infolist),
                                     variable);

    API_RETURN_STRING(result);
}

/*
 * weechat_lua_api_infolist_pointer: get pointer value of a variable in infolist
 */

static int
weechat_lua_api_infolist_pointer (lua_State *L)
{
    const char *infolist, *variable;
    char *result;

    API_FUNC(1, "infolist_pointer", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    infolist = lua_tostring (lua_current_interpreter, -2);
    variable = lua_tostring (lua_current_interpreter, -1);

    result = API_PTR2STR(weechat_infolist_pointer (API_STR2PTR(infolist),
                                                   variable));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_infolist_time: get time value of a variable in infolist
 */

static int
weechat_lua_api_infolist_time (lua_State *L)
{
    const char *infolist, *variable;
    time_t time;
    struct tm *date_tmp;
    char timebuffer[64], *result;

    API_FUNC(1, "infolist_time", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    infolist = lua_tostring (lua_current_interpreter, -2);
    variable = lua_tostring (lua_current_interpreter, -1);

    timebuffer[0] = '\0';
    time = weechat_infolist_time (API_STR2PTR(infolist),
                                  variable);
    date_tmp = localtime (&time);
    if (date_tmp)
        strftime (timebuffer, sizeof (timebuffer), "%F %T", date_tmp);
    result = strdup (timebuffer);

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_infolist_free: free infolist
 */

static int
weechat_lua_api_infolist_free (lua_State *L)
{
    const char *infolist;

    API_FUNC(1, "infolist_free", API_RETURN_ERROR);
    if (lua_gettop (lua_current_interpreter) < 1)
        API_WRONG_ARGS(API_RETURN_ERROR);

    infolist = lua_tostring (lua_current_interpreter, -1);

    weechat_infolist_free (API_STR2PTR(infolist));

    API_RETURN_OK;
}

/*
 * weechat_lua_api_hdata_get: get hdata
 */

static int
weechat_lua_api_hdata_get (lua_State *L)
{
    const char *name;
    char *result;

    API_FUNC(1, "hdata_get", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 1)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    name = lua_tostring (lua_current_interpreter, -1);

    result = API_PTR2STR(weechat_hdata_get (name));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_hdata_get_var_offset: get offset of variable in hdata
 */

static int
weechat_lua_api_hdata_get_var_offset (lua_State *L)
{
    const char *hdata, *name;
    int value;

    API_FUNC(1, "hdata_get_var_offset", API_RETURN_INT(0));
    if (lua_gettop (lua_current_interpreter) < 2)
        API_WRONG_ARGS(API_RETURN_INT(0));

    hdata = lua_tostring (lua_current_interpreter, -2);
    name = lua_tostring (lua_current_interpreter, -1);

    value = weechat_hdata_get_var_offset (API_STR2PTR(hdata), name);

    API_RETURN_INT(value);
}

/*
 * weechat_lua_api_hdata_get_var_type_string: get type of variable as string in
 *                                            hdata
 */

static int
weechat_lua_api_hdata_get_var_type_string (lua_State *L)
{
    const char *hdata, *name, *result;

    API_FUNC(1, "hdata_get_var_type_string", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    hdata = lua_tostring (lua_current_interpreter, -2);
    name = lua_tostring (lua_current_interpreter, -1);

    result = weechat_hdata_get_var_type_string (API_STR2PTR(hdata), name);

    API_RETURN_STRING(result);
}

/*
 * weechat_lua_api_hdata_get_var_array_size: get array size for variable in
 *                                           hdata
 */

static int
weechat_lua_api_hdata_get_var_array_size (lua_State *L)
{
    const char *hdata, *pointer, *name;
    int value;

    API_FUNC(1, "hdata_get_var_array_size", API_RETURN_INT(-1));
    if (lua_gettop (lua_current_interpreter) < 3)
        API_WRONG_ARGS(API_RETURN_INT(-1));

    hdata = lua_tostring (lua_current_interpreter, -3);
    pointer = lua_tostring (lua_current_interpreter, -2);
    name = lua_tostring (lua_current_interpreter, -1);

    value = weechat_hdata_get_var_array_size (API_STR2PTR(hdata),
                                              API_STR2PTR(pointer),
                                              name);

    API_RETURN_INT(value);
}

/*
 * weechat_lua_api_hdata_get_var_array_size_string: get array size for variable
 *                                                  in hdata (as string)
 */

static int
weechat_lua_api_hdata_get_var_array_size_string (lua_State *L)
{
    const char *hdata, *pointer, *name, *result;

    API_FUNC(1, "hdata_get_var_array_size_string", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    hdata = lua_tostring (lua_current_interpreter, -3);
    pointer = lua_tostring (lua_current_interpreter, -2);
    name = lua_tostring (lua_current_interpreter, -1);

    result = weechat_hdata_get_var_array_size_string (API_STR2PTR(hdata),
                                                      API_STR2PTR(pointer),
                                                      name);

    API_RETURN_STRING(result);
}

/*
 * weechat_lua_api_hdata_get_var_hdata: get hdata for variable in hdata
 */

static int
weechat_lua_api_hdata_get_var_hdata (lua_State *L)
{
    const char *hdata, *name, *result;

    API_FUNC(1, "hdata_get_var_hdata", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    hdata = lua_tostring (lua_current_interpreter, -2);
    name = lua_tostring (lua_current_interpreter, -1);

    result = weechat_hdata_get_var_hdata (API_STR2PTR(hdata), name);

    API_RETURN_STRING(result);
}

/*
 * weechat_lua_api_hdata_get_list: get list pointer in hdata
 */

static int
weechat_lua_api_hdata_get_list (lua_State *L)
{
    const char *hdata, *name;
    char *result;

    API_FUNC(1, "hdata_get_list", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    hdata = lua_tostring (lua_current_interpreter, -2);
    name = lua_tostring (lua_current_interpreter, -1);

    result = API_PTR2STR(weechat_hdata_get_list (API_STR2PTR(hdata),
                                                 name));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_hdata_check_pointer: check pointer with hdata/list
 */

static int
weechat_lua_api_hdata_check_pointer (lua_State *L)
{
    const char *hdata, *list, *pointer;
    int value;

    API_FUNC(1, "hdata_check_pointer", API_RETURN_INT(0));
    if (lua_gettop (lua_current_interpreter) < 3)
        API_WRONG_ARGS(API_RETURN_INT(0));

    hdata = lua_tostring (lua_current_interpreter, -3);
    list = lua_tostring (lua_current_interpreter, -2);
    pointer = lua_tostring (lua_current_interpreter, -1);

    value = weechat_hdata_check_pointer (API_STR2PTR(hdata),
                                         API_STR2PTR(list),
                                         API_STR2PTR(pointer));

    API_RETURN_INT(value);
}

/*
 * weechat_lua_api_hdata_move: move pointer to another element in list
 */

static int
weechat_lua_api_hdata_move (lua_State *L)
{
    const char *hdata, *pointer;
    char *result;
    int count;

    API_FUNC(1, "hdata_move", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    hdata = lua_tostring (lua_current_interpreter, -3);
    pointer = lua_tostring (lua_current_interpreter, -2);
    count = lua_tonumber (lua_current_interpreter, -1);

    result = API_PTR2STR(weechat_hdata_move (API_STR2PTR(hdata),
                                             API_STR2PTR(pointer),
                                             count));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_hdata_char: get char value of a variable in structure using
 *                             hdata
 */

static int
weechat_lua_api_hdata_char (lua_State *L)
{
    const char *hdata, *pointer, *name;
    int value;

    API_FUNC(1, "hdata_char", API_RETURN_INT(0));
    if (lua_gettop (lua_current_interpreter) < 3)
        API_WRONG_ARGS(API_RETURN_INT(0));

    hdata = lua_tostring (lua_current_interpreter, -3);
    pointer = lua_tostring (lua_current_interpreter, -2);
    name = lua_tostring (lua_current_interpreter, -1);

    value = (int)weechat_hdata_char (API_STR2PTR(hdata),
                                     API_STR2PTR(pointer),
                                     name);

    API_RETURN_INT(value);
}

/*
 * weechat_lua_api_hdata_integer: get integer value of a variable in structure
 *                                using hdata
 */

static int
weechat_lua_api_hdata_integer (lua_State *L)
{
    const char *hdata, *pointer, *name;
    int value;

    API_FUNC(1, "hdata_integer", API_RETURN_INT(0));
    if (lua_gettop (lua_current_interpreter) < 3)
        API_WRONG_ARGS(API_RETURN_INT(0));

    hdata = lua_tostring (lua_current_interpreter, -3);
    pointer = lua_tostring (lua_current_interpreter, -2);
    name = lua_tostring (lua_current_interpreter, -1);

    value = weechat_hdata_integer (API_STR2PTR(hdata),
                                   API_STR2PTR(pointer),
                                   name);

    API_RETURN_INT(value);
}

/*
 * weechat_lua_api_hdata_long: get long value of a variable in structure using
 *                             hdata
 */

static int
weechat_lua_api_hdata_long (lua_State *L)
{
    const char *hdata, *pointer, *name;
    long value;

    API_FUNC(1, "hdata_long", API_RETURN_LONG(0));
    if (lua_gettop (lua_current_interpreter) < 3)
        API_WRONG_ARGS(API_RETURN_LONG(0));

    hdata = lua_tostring (lua_current_interpreter, -3);
    pointer = lua_tostring (lua_current_interpreter, -2);
    name = lua_tostring (lua_current_interpreter, -1);

    value = weechat_hdata_long (API_STR2PTR(hdata),
                                API_STR2PTR(pointer),
                                name);

    API_RETURN_LONG(value);
}

/*
 * weechat_lua_api_hdata_string: get string value of a variable in structure
 *                               using hdata
 */

static int
weechat_lua_api_hdata_string (lua_State *L)
{
    const char *hdata, *pointer, *name, *result;

    API_FUNC(1, "hdata_string", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    hdata = lua_tostring (lua_current_interpreter, -3);
    pointer = lua_tostring (lua_current_interpreter, -2);
    name = lua_tostring (lua_current_interpreter, -1);

    result = weechat_hdata_string (API_STR2PTR(hdata),
                                   API_STR2PTR(pointer),
                                   name);

    API_RETURN_STRING(result);
}

/*
 * weechat_lua_api_hdata_pointer: get pointer value of a variable in structure
 *                                using hdata
 */

static int
weechat_lua_api_hdata_pointer (lua_State *L)
{
    const char *hdata, *pointer, *name;
    char *result;

    API_FUNC(1, "hdata_pointer", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    hdata = lua_tostring (lua_current_interpreter, -3);
    pointer = lua_tostring (lua_current_interpreter, -2);
    name = lua_tostring (lua_current_interpreter, -1);

    result = API_PTR2STR(weechat_hdata_pointer (API_STR2PTR(hdata),
                                                API_STR2PTR(pointer),
                                                name));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_hdata_time: get time value of a variable in structure using
 *                             hdata
 */

static int
weechat_lua_api_hdata_time (lua_State *L)
{
    const char *hdata, *pointer, *name;
    time_t time;
    char timebuffer[64], *result;

    API_FUNC(1, "hdata_time", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    hdata = lua_tostring (lua_current_interpreter, -3);
    pointer = lua_tostring (lua_current_interpreter, -2);
    name = lua_tostring (lua_current_interpreter, -1);

    timebuffer[0] = '\0';
    time = weechat_hdata_time (API_STR2PTR(hdata),
                               API_STR2PTR(pointer),
                               name);
    snprintf (timebuffer, sizeof (timebuffer), "%ld", (long int)time);
    result = strdup (timebuffer);

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_hdata_hashtable: get hashtable value of a variable in
 *                                  structure using hdata
 */

static int
weechat_lua_api_hdata_hashtable (lua_State *L)
{
    const char *hdata, *pointer, *name;

    API_FUNC(1, "hdata_hashtable", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    hdata = lua_tostring (lua_current_interpreter, -3);
    pointer = lua_tostring (lua_current_interpreter, -2);
    name = lua_tostring (lua_current_interpreter, -1);

    weechat_lua_pushhashtable (lua_current_interpreter,
                               weechat_hdata_hashtable (API_STR2PTR(hdata),
                                                        API_STR2PTR(pointer),
                                                        name));

    return 1;
}

/*
 * weechat_lua_api_hdata_update: update data in a hdata
 */

static int
weechat_lua_api_hdata_update (lua_State *L)
{
    const char *hdata, *pointer;
    struct t_hashtable *hashtable;
    int value;

    API_FUNC(1, "hdata_update", API_RETURN_INT(0));
    if (lua_gettop (lua_current_interpreter) < 3)
        API_WRONG_ARGS(API_RETURN_INT(0));

    hdata = lua_tostring (lua_current_interpreter, -3);
    pointer = lua_tostring (lua_current_interpreter, -2);
    hashtable = weechat_lua_tohashtable (lua_current_interpreter, -1,
                                         WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE);

    value = weechat_hdata_update (API_STR2PTR(hdata),
                                  API_STR2PTR(pointer),
                                  hashtable);

    if (hashtable)
        weechat_hashtable_free (hashtable);

    API_RETURN_INT(value);
}

/*
 * weechat_lua_api_hdata_get_string: get hdata property as string
 */

static int
weechat_lua_api_hdata_get_string (lua_State *L)
{
    const char *hdata, *property, *result;

    API_FUNC(1, "hdata_get_string", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    hdata = lua_tostring (lua_current_interpreter, -2);
    property = lua_tostring (lua_current_interpreter, -1);

    result = weechat_hdata_get_string (API_STR2PTR(hdata), property);

    API_RETURN_STRING(result);
}

/*
 * weechat_lua_api_upgrade_new: create an upgrade file
 */

static int
weechat_lua_api_upgrade_new (lua_State *L)
{
    const char *filename;
    char *result;
    int write;

    API_FUNC(1, "upgrade_new", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 2)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    filename = lua_tostring (lua_current_interpreter, -2);
    write = lua_tonumber (lua_current_interpreter, -1);

    result = API_PTR2STR(weechat_upgrade_new (filename, write));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_upgrade_write_object: write object in upgrade file
 */

static int
weechat_lua_api_upgrade_write_object (lua_State *L)
{
    const char *upgrade_file, *infolist;
    int object_id, rc;

    API_FUNC(1, "upgrade_write_object", API_RETURN_INT(0));
    if (lua_gettop (lua_current_interpreter) < 3)
        API_WRONG_ARGS(API_RETURN_INT(0));

    upgrade_file = lua_tostring (lua_current_interpreter, -3);
    object_id = lua_tonumber (lua_current_interpreter, -2);
    infolist = lua_tostring (lua_current_interpreter, -1);

    rc = weechat_upgrade_write_object (API_STR2PTR(upgrade_file),
                                       object_id,
                                       API_STR2PTR(infolist));

    API_RETURN_INT(rc);
}

/*
 * weechat_lua_api_upgrade_read_cb: callback for reading object in upgrade file
 */

int
weechat_lua_api_upgrade_read_cb (void *data,
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

        rc = (int *) weechat_lua_exec (script_callback->script,
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
 * weechat_lua_api_upgrade_read: read upgrade file
 */

static int
weechat_lua_api_upgrade_read (lua_State *L)
{
    const char *upgrade_file, *function, *data;
    int rc;

    API_FUNC(1, "upgrade_read", API_RETURN_EMPTY);
    if (lua_gettop (lua_current_interpreter) < 3)
        API_WRONG_ARGS(API_RETURN_EMPTY);

    upgrade_file = lua_tostring (lua_current_interpreter, -3);
    function = lua_tostring (lua_current_interpreter, -2);
    data = lua_tostring (lua_current_interpreter, -1);

    rc = plugin_script_api_upgrade_read (weechat_lua_plugin,
                                         lua_current_script,
                                         API_STR2PTR(upgrade_file),
                                         &weechat_lua_api_upgrade_read_cb,
                                         function,
                                         data);

    API_RETURN_INT(rc);
}

/*
 * weechat_lua_api_upgrade_close: close upgrade file
 */

static int
weechat_lua_api_upgrade_close (lua_State *L)
{
    const char *upgrade_file;

    API_FUNC(1, "upgrade_close", API_RETURN_ERROR);
    if (lua_gettop (lua_current_interpreter) < 1)
        API_WRONG_ARGS(API_RETURN_INT(0));

    upgrade_file = lua_tostring (lua_current_interpreter, -1);

    weechat_upgrade_close (API_STR2PTR(upgrade_file));

    API_RETURN_OK;
}

/*
 * Lua constant as functions
 */

static int
weechat_lua_api_constant_weechat_rc_ok (lua_State *L)
{
    /* make C compiler happy */
    (void) L;

    lua_pushnumber (lua_current_interpreter, WEECHAT_RC_OK);
    return 1;
}

static int
weechat_lua_api_constant_weechat_rc_ok_eat (lua_State *L)
{
    /* make C compiler happy */
    (void) L;

    lua_pushnumber (lua_current_interpreter, WEECHAT_RC_OK_EAT);
    return 1;
}

static int
weechat_lua_api_constant_weechat_rc_error (lua_State *L)
{
    /* make C compiler happy */
    (void) L;

    lua_pushnumber (lua_current_interpreter, WEECHAT_RC_ERROR);
    return 1;
}

static int
weechat_lua_api_constant_weechat_config_read_ok (lua_State *L)
{
    /* make C compiler happy */
    (void) L;

    lua_pushnumber (lua_current_interpreter, WEECHAT_CONFIG_READ_OK);
    return 1;
}

static int
weechat_lua_api_constant_weechat_config_read_memory_error (lua_State *L)
{
    /* make C compiler happy */
    (void) L;

    lua_pushnumber (lua_current_interpreter, WEECHAT_CONFIG_READ_MEMORY_ERROR);
    return 1;
}

static int
weechat_lua_api_constant_weechat_config_read_file_not_found (lua_State *L)
{
    /* make C compiler happy */
    (void) L;

    lua_pushnumber (lua_current_interpreter, WEECHAT_CONFIG_READ_FILE_NOT_FOUND);
    return 1;
}

static int
weechat_lua_api_constant_weechat_config_write_ok (lua_State *L)
{
    /* make C compiler happy */
    (void) L;

    lua_pushnumber (lua_current_interpreter, WEECHAT_CONFIG_WRITE_OK);
    return 1;
}

static int
weechat_lua_api_constant_weechat_config_write_error (lua_State *L)
{
    /* make C compiler happy */
    (void) L;

    lua_pushnumber (lua_current_interpreter, WEECHAT_CONFIG_WRITE_ERROR);
    return 1;
}

static int
weechat_lua_api_constant_weechat_config_write_memory_error (lua_State *L)
{
    /* make C compiler happy */
    (void) L;

    lua_pushnumber (lua_current_interpreter, WEECHAT_CONFIG_WRITE_MEMORY_ERROR);
    return 1;
}

static int
weechat_lua_api_constant_weechat_config_option_set_ok_changed (lua_State *L)
{
    /* make C compiler happy */
    (void) L;

    lua_pushnumber (lua_current_interpreter, WEECHAT_CONFIG_OPTION_SET_OK_CHANGED);
    return 1;
}

static int
weechat_lua_api_constant_weechat_config_option_set_ok_same_value (lua_State *L)
{
    /* make C compiler happy */
    (void) L;

    lua_pushnumber (lua_current_interpreter, WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE);
    return 1;
}

static int
weechat_lua_api_constant_weechat_config_option_set_error (lua_State *L)
{
    /* make C compiler happy */
    (void) L;

    lua_pushnumber (lua_current_interpreter, WEECHAT_CONFIG_OPTION_SET_ERROR);
    return 1;
}

static int
weechat_lua_api_constant_weechat_config_option_set_option_not_found (lua_State *L)
{
    /* make C compiler happy */
    (void) L;

    lua_pushnumber (lua_current_interpreter, WEECHAT_CONFIG_OPTION_SET_OPTION_NOT_FOUND);
    return 1;
}

static int
weechat_lua_api_constant_weechat_config_option_unset_ok_no_reset (lua_State *L)
{
    /* make C compiler happy */
    (void) L;

    lua_pushnumber (lua_current_interpreter, WEECHAT_CONFIG_OPTION_UNSET_OK_NO_RESET);
    return 1;
}

static int
weechat_lua_api_constant_weechat_config_option_unset_ok_reset (lua_State *L)
{
    /* make C compiler happy */
    (void) L;

    lua_pushnumber (lua_current_interpreter, WEECHAT_CONFIG_OPTION_UNSET_OK_RESET);
    return 1;
}

static int
weechat_lua_api_constant_weechat_config_option_unset_ok_removed (lua_State *L)
{
    /* make C compiler happy */
    (void) L;

    lua_pushnumber (lua_current_interpreter, WEECHAT_CONFIG_OPTION_UNSET_OK_REMOVED);
    return 1;
}

static int
weechat_lua_api_constant_weechat_config_option_unset_error (lua_State *L)
{
    /* make C compiler happy */
    (void) L;

    lua_pushnumber (lua_current_interpreter, WEECHAT_CONFIG_OPTION_UNSET_ERROR);
    return 1;
}

static int
weechat_lua_api_constant_weechat_list_pos_sort (lua_State *L)
{
    /* make C compiler happy */
    (void) L;

    lua_pushstring (lua_current_interpreter, WEECHAT_LIST_POS_SORT);
    return 1;
}

static int
weechat_lua_api_constant_weechat_list_pos_beginning (lua_State *L)
{
    /* make C compiler happy */
    (void) L;

    lua_pushstring (lua_current_interpreter, WEECHAT_LIST_POS_BEGINNING);
    return 1;
}

static int
weechat_lua_api_constant_weechat_list_pos_end (lua_State *L)
{
    /* make C compiler happy */
    (void) L;

    lua_pushstring (lua_current_interpreter, WEECHAT_LIST_POS_END);
    return 1;
}

static int
weechat_lua_api_constant_weechat_hotlist_low (lua_State *L)
{
    /* make C compiler happy */
    (void) L;

    lua_pushstring (lua_current_interpreter, WEECHAT_HOTLIST_LOW);
    return 1;
}

static int
weechat_lua_api_constant_weechat_hotlist_message (lua_State *L)
{
    /* make C compiler happy */
    (void) L;

    lua_pushstring (lua_current_interpreter, WEECHAT_HOTLIST_MESSAGE);
    return 1;
}

static int
weechat_lua_api_constant_weechat_hotlist_private (lua_State *L)
{
    /* make C compiler happy */
    (void) L;

    lua_pushstring (lua_current_interpreter, WEECHAT_HOTLIST_PRIVATE);
    return 1;
}

static int
weechat_lua_api_constant_weechat_hotlist_highlight (lua_State *L)
{
    /* make C compiler happy */
    (void) L;

    lua_pushstring (lua_current_interpreter, WEECHAT_HOTLIST_HIGHLIGHT);
    return 1;
}

static int
weechat_lua_api_constant_weechat_hook_process_running (lua_State *L)
{
    /* make C compiler happy */
    (void) L;

    lua_pushnumber (lua_current_interpreter, WEECHAT_HOOK_PROCESS_RUNNING);
    return 1;
}

static int
weechat_lua_api_constant_weechat_hook_process_error (lua_State *L)
{
    /* make C compiler happy */
    (void) L;

    lua_pushnumber (lua_current_interpreter, WEECHAT_HOOK_PROCESS_ERROR);
    return 1;
}

static int
weechat_lua_api_constant_weechat_hook_connect_ok (lua_State *L)
{
    /* make C compiler happy */
    (void) L;

    lua_pushnumber (lua_current_interpreter, WEECHAT_HOOK_CONNECT_OK);
    return 1;
}

static int
weechat_lua_api_constant_weechat_hook_connect_address_not_found (lua_State *L)
{
    /* make C compiler happy */
    (void) L;

    lua_pushnumber (lua_current_interpreter, WEECHAT_HOOK_CONNECT_ADDRESS_NOT_FOUND);
    return 1;
}

static int
weechat_lua_api_constant_weechat_hook_connect_ip_address_not_found (lua_State *L)
{
    /* make C compiler happy */
    (void) L;

    lua_pushnumber (lua_current_interpreter, WEECHAT_HOOK_CONNECT_IP_ADDRESS_NOT_FOUND);
    return 1;
}

static int
weechat_lua_api_constant_weechat_hook_connect_connection_refused (lua_State *L)
{
    /* make C compiler happy */
    (void) L;

    lua_pushnumber (lua_current_interpreter, WEECHAT_HOOK_CONNECT_CONNECTION_REFUSED);
    return 1;
}

static int
weechat_lua_api_constant_weechat_hook_connect_proxy_error (lua_State *L)
{
    /* make C compiler happy */
    (void) L;

    lua_pushnumber (lua_current_interpreter, WEECHAT_HOOK_CONNECT_PROXY_ERROR);
    return 1;
}

static int
weechat_lua_api_constant_weechat_hook_connect_local_hostname_error (lua_State *L)
{
    /* make C compiler happy */
    (void) L;

    lua_pushnumber (lua_current_interpreter, WEECHAT_HOOK_CONNECT_LOCAL_HOSTNAME_ERROR);
    return 1;
}

static int
weechat_lua_api_constant_weechat_hook_connect_gnutls_init_error (lua_State *L)
{
    /* make C compiler happy */
    (void) L;

    lua_pushnumber (lua_current_interpreter, WEECHAT_HOOK_CONNECT_GNUTLS_INIT_ERROR);
    return 1;
}

static int
weechat_lua_api_constant_weechat_hook_connect_gnutls_handshake_error (lua_State *L)
{
    /* make C compiler happy */
    (void) L;

    lua_pushnumber (lua_current_interpreter, WEECHAT_HOOK_CONNECT_GNUTLS_HANDSHAKE_ERROR);
    return 1;
}

static int
weechat_lua_api_constant_weechat_hook_connect_memory_error (lua_State *L)
{
    /* make C compiler happy */
    (void) L;

    lua_pushnumber (lua_current_interpreter, WEECHAT_HOOK_CONNECT_MEMORY_ERROR);
    return 1;
}

static int
weechat_lua_api_constant_weechat_hook_connect_timeout (lua_State *L)
{
    /* make C compiler happy */
    (void) L;

    lua_pushnumber (lua_current_interpreter, WEECHAT_HOOK_CONNECT_TIMEOUT);
    return 1;
}

static int
weechat_lua_api_constant_weechat_hook_signal_string (lua_State *L)
{
    /* make C compiler happy */
    (void) L;

    lua_pushstring (lua_current_interpreter, WEECHAT_HOOK_SIGNAL_STRING);
    return 1;
}

static int
weechat_lua_api_constant_weechat_hook_signal_int (lua_State *L)
{
    /* make C compiler happy */
    (void) L;

    lua_pushstring (lua_current_interpreter, WEECHAT_HOOK_SIGNAL_INT);
    return 1;
}

static int
weechat_lua_api_constant_weechat_hook_signal_pointer (lua_State *L)
{
    /* make C compiler happy */
    (void) L;

    lua_pushstring (lua_current_interpreter, WEECHAT_HOOK_SIGNAL_POINTER);
    return 1;
}

/*
 * Lua subroutines
 */

const struct luaL_Reg weechat_lua_api_funcs[] = {
    API_DEF_FUNC(register),
    API_DEF_FUNC(plugin_get_name),
    API_DEF_FUNC(charset_set),
    API_DEF_FUNC(iconv_to_internal),
    API_DEF_FUNC(iconv_from_internal),
    API_DEF_FUNC(gettext),
    API_DEF_FUNC(ngettext),
    API_DEF_FUNC(string_match),
    API_DEF_FUNC(string_has_highlight),
    API_DEF_FUNC(string_has_highlight_regex),
    API_DEF_FUNC(string_mask_to_regex),
    API_DEF_FUNC(string_remove_color),
    API_DEF_FUNC(string_is_command_char),
    API_DEF_FUNC(string_input_for_buffer),
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
    API_DEF_FUNC(print),
    API_DEF_FUNC(print_date_tags),
    API_DEF_FUNC(print_y),
    API_DEF_FUNC(log_print),
    API_DEF_FUNC(hook_command),
    API_DEF_FUNC(hook_command_run),
    API_DEF_FUNC(hook_timer),
    API_DEF_FUNC(hook_fd),
    API_DEF_FUNC(hook_process),
    API_DEF_FUNC(hook_process_hashtable),
    API_DEF_FUNC(hook_connect),
    API_DEF_FUNC(hook_print),
    API_DEF_FUNC(hook_signal),
    API_DEF_FUNC(hook_signal_send),
    API_DEF_FUNC(hook_hsignal),
    API_DEF_FUNC(hook_hsignal_send),
    API_DEF_FUNC(hook_config),
    API_DEF_FUNC(hook_completion),
    API_DEF_FUNC(hook_completion_list_add),
    API_DEF_FUNC(hook_modifier),
    API_DEF_FUNC(hook_modifier_exec),
    API_DEF_FUNC(hook_info),
    API_DEF_FUNC(hook_info_hashtable),
    API_DEF_FUNC(hook_infolist),
    API_DEF_FUNC(hook_focus),
    API_DEF_FUNC(unhook),
    API_DEF_FUNC(unhook_all),
    API_DEF_FUNC(buffer_new),
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
    API_DEF_FUNC(info_get),
    API_DEF_FUNC(info_get_hashtable),
    API_DEF_FUNC(infolist_new),
    API_DEF_FUNC(infolist_new_item),
    API_DEF_FUNC(infolist_new_var_integer),
    API_DEF_FUNC(infolist_new_var_string),
    API_DEF_FUNC(infolist_new_var_pointer),
    API_DEF_FUNC(infolist_new_var_time),
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
    API_DEF_FUNC(hdata_char),
    API_DEF_FUNC(hdata_integer),
    API_DEF_FUNC(hdata_long),
    API_DEF_FUNC(hdata_string),
    API_DEF_FUNC(hdata_pointer),
    API_DEF_FUNC(hdata_time),
    API_DEF_FUNC(hdata_hashtable),
    API_DEF_FUNC(hdata_update),
    API_DEF_FUNC(hdata_get_string),
    API_DEF_FUNC(upgrade_new),
    API_DEF_FUNC(upgrade_write_object),
    API_DEF_FUNC(upgrade_read),
    API_DEF_FUNC(upgrade_close),

    /* define constants as function which returns values */

    { "WEECHAT_RC_OK", &weechat_lua_api_constant_weechat_rc_ok },
    { "WEECHAT_RC_OK_EAT", &weechat_lua_api_constant_weechat_rc_ok_eat },
    { "WEECHAT_RC_ERROR", &weechat_lua_api_constant_weechat_rc_error },

    { "WEECHAT_CONFIG_READ_OK", &weechat_lua_api_constant_weechat_config_read_ok },
    { "WEECHAT_CONFIG_READ_MEMORY_ERROR", &weechat_lua_api_constant_weechat_config_read_memory_error },
    { "WEECHAT_CONFIG_READ_FILE_NOT_FOUND", &weechat_lua_api_constant_weechat_config_read_file_not_found },
    { "WEECHAT_CONFIG_WRITE_OK", &weechat_lua_api_constant_weechat_config_write_ok },
    { "WEECHAT_CONFIG_WRITE_ERROR", &weechat_lua_api_constant_weechat_config_write_error },
    { "WEECHAT_CONFIG_WRITE_MEMORY_ERROR", &weechat_lua_api_constant_weechat_config_write_memory_error },
    { "WEECHAT_CONFIG_OPTION_SET_OK_CHANGED", &weechat_lua_api_constant_weechat_config_option_set_ok_changed },
    { "WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE", &weechat_lua_api_constant_weechat_config_option_set_ok_same_value },
    { "WEECHAT_CONFIG_OPTION_SET_ERROR", &weechat_lua_api_constant_weechat_config_option_set_error },
    { "WEECHAT_CONFIG_OPTION_SET_OPTION_NOT_FOUND", &weechat_lua_api_constant_weechat_config_option_set_option_not_found },
    { "WEECHAT_CONFIG_OPTION_UNSET_OK_NO_RESET", &weechat_lua_api_constant_weechat_config_option_unset_ok_no_reset },
    { "WEECHAT_CONFIG_OPTION_UNSET_OK_RESET", &weechat_lua_api_constant_weechat_config_option_unset_ok_reset },
    { "WEECHAT_CONFIG_OPTION_UNSET_OK_REMOVED", &weechat_lua_api_constant_weechat_config_option_unset_ok_removed },
    { "WEECHAT_CONFIG_OPTION_UNSET_ERROR", &weechat_lua_api_constant_weechat_config_option_unset_error },

    { "WEECHAT_LIST_POS_SORT", &weechat_lua_api_constant_weechat_list_pos_sort },
    { "WEECHAT_LIST_POS_BEGINNING", &weechat_lua_api_constant_weechat_list_pos_beginning },
    { "WEECHAT_LIST_POS_END", &weechat_lua_api_constant_weechat_list_pos_end },

    { "WEECHAT_HOTLIST_LOW", &weechat_lua_api_constant_weechat_hotlist_low },
    { "WEECHAT_HOTLIST_MESSAGE", &weechat_lua_api_constant_weechat_hotlist_message },
    { "WEECHAT_HOTLIST_PRIVATE", &weechat_lua_api_constant_weechat_hotlist_private },
    { "WEECHAT_HOTLIST_HIGHLIGHT", &weechat_lua_api_constant_weechat_hotlist_highlight },

    { "WEECHAT_HOOK_PROCESS_RUNNING", &weechat_lua_api_constant_weechat_hook_process_running },
    { "WEECHAT_HOOK_PROCESS_ERROR", &weechat_lua_api_constant_weechat_hook_process_error },

    { "WEECHAT_HOOK_CONNECT_OK", &weechat_lua_api_constant_weechat_hook_connect_ok },
    { "WEECHAT_HOOK_CONNECT_ADDRESS_NOT_FOUND", &weechat_lua_api_constant_weechat_hook_connect_address_not_found },
    { "WEECHAT_HOOK_CONNECT_IP_ADDRESS_NOT_FOUND", &weechat_lua_api_constant_weechat_hook_connect_ip_address_not_found },
    { "WEECHAT_HOOK_CONNECT_CONNECTION_REFUSED", &weechat_lua_api_constant_weechat_hook_connect_connection_refused },
    { "WEECHAT_HOOK_CONNECT_PROXY_ERROR", &weechat_lua_api_constant_weechat_hook_connect_proxy_error },
    { "WEECHAT_HOOK_CONNECT_LOCAL_HOSTNAME_ERROR", &weechat_lua_api_constant_weechat_hook_connect_local_hostname_error },
    { "WEECHAT_HOOK_CONNECT_GNUTLS_INIT_ERROR", &weechat_lua_api_constant_weechat_hook_connect_gnutls_init_error },
    { "WEECHAT_HOOK_CONNECT_GNUTLS_HANDSHAKE_ERROR", &weechat_lua_api_constant_weechat_hook_connect_gnutls_handshake_error },
    { "WEECHAT_HOOK_CONNECT_MEMORY_ERROR", &weechat_lua_api_constant_weechat_hook_connect_memory_error },
    { "WEECHAT_HOOK_CONNECT_TIMEOUT", &weechat_lua_api_constant_weechat_hook_connect_timeout },

    { "WEECHAT_HOOK_SIGNAL_STRING", &weechat_lua_api_constant_weechat_hook_signal_string },
    { "WEECHAT_HOOK_SIGNAL_INT", &weechat_lua_api_constant_weechat_hook_signal_int },
    { "WEECHAT_HOOK_SIGNAL_POINTER", &weechat_lua_api_constant_weechat_hook_signal_pointer },

    { NULL, NULL }
};
