/*
 * Copyright (c) 2003-2008 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* weechat-lua-api.c: Lua API functions */

#undef _

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <stdlib.h>
#include <string.h>

#include "../../weechat-plugin.h"
#include "../script.h"
#include "../script-api.h"
#include "../script-callback.h"
#include "weechat-lua.h"


#define LUA_RETURN_OK return 1
#define LUA_RETURN_ERROR return 0
#define LUA_RETURN_EMPTY                                \
    lua_pushstring (lua_current_interpreter, "");       \
    return 0
#define LUA_RETURN_STRING(__string)                     \
    lua_pushstring (lua_current_interpreter,            \
                    (__string) ? __string : "");        \
    return 1;
#define LUA_RETURN_STRING_FREE(__string)                \
    lua_pushstring (lua_current_interpreter,            \
                    (__string) ? __string : "");        \
    if (__string)                                       \
        free (__string);                                \
    return 1;
#define LUA_RETURN_INT(__int)                           \
    lua_pushnumber (lua_current_interpreter, __int);    \
    return 1;


/*
 * weechat_lua_api_register: startup function for all WeeChat Lua scripts
 */

static int
weechat_lua_api_register (lua_State *L)
{
    const char *name, *author, *version, *license, *description;
    const char *shutdown_func, *charset;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    lua_current_script = NULL;
    
    name = NULL;
    author = NULL;
    version = NULL;
    license = NULL;
    description = NULL;
    shutdown_func = NULL;
    charset = NULL;
    
    n = lua_gettop (lua_current_interpreter);

    if (n < 7)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("register");
        LUA_RETURN_ERROR;
    }
    
    name = lua_tostring (lua_current_interpreter, -7);
    author = lua_tostring (lua_current_interpreter, -6);
    version = lua_tostring (lua_current_interpreter, -5);
    license = lua_tostring (lua_current_interpreter, -4);
    description = lua_tostring (lua_current_interpreter, -3);
    shutdown_func = lua_tostring (lua_current_interpreter, -2);
    charset = lua_tostring (lua_current_interpreter, -1);
    
    if (script_search (weechat_lua_plugin, lua_scripts, (char *)name))
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to register script "
                                         "\"%s\" (another script already "
                                         "exists with this name)"),
                        weechat_prefix ("error"), "lua", name);
        LUA_RETURN_ERROR;
    }
    
    /* register script */
    lua_current_script = script_add (weechat_lua_plugin,
                                     &lua_scripts,
                                     (lua_current_script_filename) ?
                                     lua_current_script_filename : "",
                                     (char *)name,
                                     (char *)author,
                                     (char *)version,
                                     (char *)license,
                                     (char *)description,
                                     (char *)shutdown_func,
                                     (char *)charset);
    if (lua_current_script)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: registered script \"%s\", "
                                         "version %s (%s)"),
                        weechat_prefix ("info"), "lua",
                        name, version, description);
    }
    else
    {
        LUA_RETURN_ERROR;
    }
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_charset_set: set script charset
 */

static int
weechat_lua_api_charset_set (lua_State *L)
{
    const char *charset;
    int n;
    
    /* make C compiler happy */
    (void) L;
     
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("charset_set");
        LUA_RETURN_ERROR;
    }
    
    charset = NULL;
 
    n = lua_gettop (lua_current_interpreter);

    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("charset_set");
        LUA_RETURN_ERROR;
    }
    
    charset = lua_tostring (lua_current_interpreter, -1);
    
    script_api_charset_set (lua_current_script,
                            (char *) charset);

    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_iconv_to_internal: convert string to internal WeeChat charset
 */

static int
weechat_lua_api_iconv_to_internal (lua_State *L)
{
    const char *charset, *string;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("iconv_to_internal");
        LUA_RETURN_EMPTY;
    }
    
    charset = NULL;
    string = NULL;
    
    n = lua_gettop (lua_current_interpreter);

    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("iconv_to_internal");
        LUA_RETURN_EMPTY;
    }
    
    charset = lua_tostring (lua_current_interpreter, -2);
    string = lua_tostring (lua_current_interpreter, -1);
    
    result = weechat_iconv_to_internal ((char *)charset, (char *)string);
    LUA_RETURN_STRING_FREE(result);
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
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("iconv_from_internal");
        LUA_RETURN_EMPTY;
    }
    
    charset = NULL;
    string = NULL;
    
    n = lua_gettop (lua_current_interpreter);

    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("iconv_from_internal");
        LUA_RETURN_EMPTY;
    }
    
    charset = lua_tostring (lua_current_interpreter, -2);
    string = lua_tostring (lua_current_interpreter, -1);
    
    result = weechat_iconv_from_internal ((char *)charset, (char *)string);
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_gettext: get translated string
 */

static int
weechat_lua_api_gettext (lua_State *L)
{
    const char *string;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("gettext");
        LUA_RETURN_EMPTY;
    }
    
    string = NULL;
    
    n = lua_gettop (lua_current_interpreter);

    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("gettext");
        LUA_RETURN_EMPTY;
    }
    
    string = lua_tostring (lua_current_interpreter, -1);
    
    result = weechat_gettext ((char *)string);
    LUA_RETURN_STRING(result);
}

/*
 * weechat_lua_api_ngettext: get translated string with plural form
 */

static int
weechat_lua_api_ngettext (lua_State *L)
{
    const char *single, *plural;
    char *result;
    int n, count;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("ngettext");
        LUA_RETURN_EMPTY;
    }
    
    single = NULL;
    plural = NULL;
    count = 0;
    
    n = lua_gettop (lua_current_interpreter);

    if (n < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("ngettext");
        LUA_RETURN_EMPTY;
    }
    
    single = lua_tostring (lua_current_interpreter, -3);
    plural = lua_tostring (lua_current_interpreter, -2);
    count = lua_tonumber (lua_current_interpreter, -1);
    
    result = weechat_ngettext ((char *)single, (char *)plural, count);
    LUA_RETURN_STRING(result);
}

/*
 * weechat_lua_api_mkdir_home: create a directory in WeeChat home
 */

static int
weechat_lua_api_mkdir_home (lua_State *L)
{
    const char *directory;
    int mode, n;
    
    /* make C compiler happy */
    (void) L;
     
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("mkdir_home");
        LUA_RETURN_ERROR;
    }
    
    directory = NULL;
    mode = 0;
    
    n = lua_gettop (lua_current_interpreter);

    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("mkdir_home");
        LUA_RETURN_ERROR;
    }
    
    directory = lua_tostring (lua_current_interpreter, -2);
    mode = lua_tonumber (lua_current_interpreter, -1);
    
    if (weechat_mkdir_home ((char *)directory, mode))
        LUA_RETURN_OK;
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_mkdir: create a directory
 */

static int
weechat_lua_api_mkdir (lua_State *L)
{
    const char *directory;
    int mode, n;
    
    /* make C compiler happy */
    (void) L;
     
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("mkdir");
        LUA_RETURN_ERROR;
    }
    
    directory = NULL;
    mode = 0;
    
    n = lua_gettop (lua_current_interpreter);

    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("mkdir");
        LUA_RETURN_ERROR;
    }
    
    directory = lua_tostring (lua_current_interpreter, -2);
    mode = lua_tonumber (lua_current_interpreter, -1);
    
    if (weechat_mkdir ((char *)directory, mode))
        LUA_RETURN_OK;
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_list_new: create a new list
 */

static int
weechat_lua_api_list_new (lua_State *L)
{
    char *result;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("list_new");
        LUA_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_list_new ());
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_list_add: add a string to list
 */

static int
weechat_lua_api_list_add (lua_State *L)
{
    const char *weelist, *data, *where;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("list_add");
        LUA_RETURN_EMPTY;
    }
    
    weelist = NULL;
    data = NULL;
    where = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("list_add");
        LUA_RETURN_EMPTY;
    }
    
    weelist = lua_tostring (lua_current_interpreter, -3);
    data = lua_tostring (lua_current_interpreter, -2);
    where = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (weechat_list_add (script_str2ptr ((char *)weelist),
                                               (char *)data,
                                               (char *)where));
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_list_search: search a string in list
 */

static int
weechat_lua_api_list_search (lua_State *L)
{
    const char *weelist, *data;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("list_search");
        LUA_RETURN_EMPTY;
    }
    
    weelist = NULL;
    data = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("list_search");
        LUA_RETURN_EMPTY;
    }
    
    weelist = lua_tostring (lua_current_interpreter, -2);
    data = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (weechat_list_search (script_str2ptr ((char *)weelist),
                                                  (char *)data));
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_list_casesearch: search a string in list (ignore case)
 */

static int
weechat_lua_api_list_casesearch (lua_State *L)
{
    const char *weelist, *data;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("list_casesearch");
        LUA_RETURN_EMPTY;
    }
    
    weelist = NULL;
    data = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("list_casesearch");
        LUA_RETURN_EMPTY;
    }
    
    weelist = lua_tostring (lua_current_interpreter, -2);
    data = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (weechat_list_casesearch (script_str2ptr ((char *)weelist),
                                                      (char *)data));
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_list_get: get item by position
 */

static int
weechat_lua_api_list_get (lua_State *L)
{
    const char *weelist;
    char *result;
    int position, n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("list_get");
        LUA_RETURN_EMPTY;
    }
    
    weelist = NULL;
    position = 0;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("list_get");
        LUA_RETURN_EMPTY;
    }
    
    weelist = lua_tostring (lua_current_interpreter, -2);
    position = lua_tonumber (lua_current_interpreter, -1);
    
    result = script_ptr2str (weechat_list_get (script_str2ptr ((char *)weelist),
                                               position));
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_list_set: set new value for item
 */

static int
weechat_lua_api_list_set (lua_State *L)
{
    const char *item, *new_value;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("list_set");
        LUA_RETURN_ERROR;
    }
    
    item = NULL;
    new_value = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("list_set");
        LUA_RETURN_ERROR;
    }
    
    item = lua_tostring (lua_current_interpreter, -2);
    new_value = lua_tostring (lua_current_interpreter, -1);
    
    weechat_list_set (script_str2ptr ((char *)item),
                      (char *)new_value);
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_list_next: get next item
 */

static int
weechat_lua_api_list_next (lua_State *L)
{
    const char *item;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("list_next");
        LUA_RETURN_EMPTY;
    }
    
    item = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("list_next");
        LUA_RETURN_EMPTY;
    }
    
    item = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (weechat_list_next (script_str2ptr ((char *)item)));
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_list_prev: get previous item
 */

static int
weechat_lua_api_list_prev (lua_State *L)
{
    const char *item;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("list_prev");
        LUA_RETURN_EMPTY;
    }
    
    item = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("list_prev");
        LUA_RETURN_EMPTY;
    }
    
    item = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (weechat_list_prev (script_str2ptr ((char *)item)));
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_list_string: get string value of item
 */

static int
weechat_lua_api_list_string (lua_State *L)
{
    const char *item;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("list_string");
        LUA_RETURN_EMPTY;
    }
    
    item = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("list_string");
        LUA_RETURN_EMPTY;
    }
    
    item = lua_tostring (lua_current_interpreter, -1);
    
    result = weechat_list_string (script_str2ptr ((char *)item));
    LUA_RETURN_STRING(result);
}

/*
 * weechat_lua_api_list_size: get number of elements in list
 */

static int
weechat_lua_api_list_size (lua_State *L)
{
    const char *weelist;
    int n, size;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("list_size");
        LUA_RETURN_INT(0);
    }
    
    weelist = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("list_size");
        LUA_RETURN_INT(0);
    }
    
    weelist = lua_tostring (lua_current_interpreter, -1);
    
    size = weechat_list_size (script_str2ptr ((char *)weelist));
    LUA_RETURN_INT(size);
}

/*
 * weechat_lua_api_list_remove: remove item from list
 */

static int
weechat_lua_api_list_remove (lua_State *L)
{
    const char *weelist, *item;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("list_remove");
        LUA_RETURN_ERROR;
    }
    
    weelist = NULL;
    item = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("list_remove");
        LUA_RETURN_ERROR;
    }
    
    weelist = lua_tostring (lua_current_interpreter, -2);
    item = lua_tostring (lua_current_interpreter, -1);
    
    weechat_list_remove (script_str2ptr ((char *)weelist),
                         script_str2ptr ((char *)item));
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_list_remove_all: remove all items from list
 */

static int
weechat_lua_api_list_remove_all (lua_State *L)
{
    const char *weelist;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("list_remove_all");
        LUA_RETURN_ERROR;
    }
    
    weelist = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("list_remove_all");
        LUA_RETURN_ERROR;
    }
    
    weelist = lua_tostring (lua_current_interpreter, -1);
    
    weechat_list_remove_all (script_str2ptr ((char *)weelist));
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_list_free: free list
 */

static int
weechat_lua_api_list_free (lua_State *L)
{
    const char *weelist;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("list_free");
        LUA_RETURN_ERROR;
    }
    
    weelist = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("list_free");
        LUA_RETURN_ERROR;
    }
    
    weelist = lua_tostring (lua_current_interpreter, -1);
    
    weechat_list_free (script_str2ptr ((char *)weelist));
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_prefix: get a prefix, used for display
 */

static int
weechat_lua_api_prefix (lua_State *L)
{
    const char *prefix;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("prefix");
        LUA_RETURN_EMPTY;
    }
    
    prefix = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("prefix");
        LUA_RETURN_EMPTY;
    }
    
    prefix = lua_tostring (lua_current_interpreter, -1);
    
    result = weechat_prefix ((char *)prefix);
    LUA_RETURN_STRING(result);
}

/*
 * weechat_lua_api_color: get a color code, used for display
 */

static int
weechat_lua_api_color (lua_State *L)
{
    const char *color;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("color");
        LUA_RETURN_EMPTY;
    }
    
    color = NULL;
    
    n = lua_gettop (lua_current_interpreter);

    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("color");
        LUA_RETURN_EMPTY;
    }
    
    color = lua_tostring (lua_current_interpreter, -1);
    
    result = weechat_prefix ((char *)color);
    LUA_RETURN_STRING(result);
}

/*
 * weechat_lua_api_print: print message in a buffer
 */

static int
weechat_lua_api_print (lua_State *L)
{
    const char *buffer, *message;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("print");
        LUA_RETURN_ERROR;
    }
    
    buffer = NULL;
    message = NULL;
    
    n = lua_gettop (lua_current_interpreter);

    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("print");
        LUA_RETURN_ERROR;
    }
    
    buffer = lua_tostring (lua_current_interpreter, -2);
    message = lua_tostring (lua_current_interpreter, -1);
    
    script_api_printf (weechat_lua_plugin,
                       lua_current_script,
                       script_str2ptr ((char *)buffer),
                       "%s", (char *)message);
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_infobar_print: print message to infobar
 */

static int
weechat_lua_api_infobar_print (lua_State *L)
{
    const char *color, *message;
    int delay, n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("infobar_print");
        LUA_RETURN_ERROR;
    }
    
    delay = 1;
    color = NULL;
    message = NULL;

    n = lua_gettop (lua_current_interpreter);

    if (n < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("infobar_print");
        LUA_RETURN_ERROR;
    }
    
    delay = lua_tonumber (lua_current_interpreter, -3);
    color = lua_tostring (lua_current_interpreter, -2);
    message = lua_tostring (lua_current_interpreter, -1);
    
    script_api_infobar_printf (weechat_lua_plugin,
                               lua_current_script,
                               delay,
                               (char *)color,
                               "%s",
                               (char *)message);

    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_infobar_remove: remove message(s) in infobar
 */

static int
weechat_lua_api_infobar_remove (lua_State *L)
{
    int n, how_many;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("infobar_remove");
        LUA_RETURN_ERROR;
    }
    
    how_many = 0;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n == 1)
        how_many = lua_tonumber (lua_current_interpreter, -1);
    
    weechat_infobar_remove (how_many);
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_log_print: print message in WeeChat log file
 */

static int
weechat_lua_api_log_print (lua_State *L)
{
    const char *message;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("log_print");
        LUA_RETURN_ERROR;
    }
    
    message = NULL;
    
    n = lua_gettop (lua_current_interpreter);

    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("log_print");
        LUA_RETURN_ERROR;
    }

    message = lua_tostring (lua_current_interpreter, -1);
    
    script_api_log_printf (weechat_lua_plugin,
                           lua_current_script,
                           "%s", (char *) message);

    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_hook_command_cb: callback for command hooked
 */

int
weechat_lua_api_hook_command_cb (void *data, struct t_gui_buffer *buffer,
                                 int argc, char **argv, char **argv_eol)
{
    struct t_script_callback *script_callback;
    char *lua_argv[3], empty_arg[1] = { '\0' };
    int *rc, ret;

    /* make C compiler happy */
    (void) argv;
    
    script_callback = (struct t_script_callback *)data;
    
    lua_argv[0] = script_ptr2str (buffer);
    lua_argv[1] = (argc > 1) ? argv_eol[1] : empty_arg;
    lua_argv[2] = NULL;
    
    rc = (int *) weechat_lua_exec (script_callback->script,
                                   WEECHAT_SCRIPT_EXEC_INT,
                                   script_callback->function,
                                   lua_argv);
    
    if (!rc)
        ret = WEECHAT_RC_ERROR;
    else
    {
        ret = *rc;
        free (rc);
    }
    if (lua_argv[0])
        free (lua_argv[0]);
    
    return ret;
}

/*
 * weechat_lua_api_hook_command: hook a command
 */

static int
weechat_lua_api_hook_command (lua_State *L)
{
    const char *command, *description, *args, *args_description, *completion;
    const char *function;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_command");
        LUA_RETURN_EMPTY;
    }
    
    command = NULL;
    description = NULL;
    args = NULL;
    args_description = NULL;
    completion = NULL;
    function = NULL;
    
    n = lua_gettop (lua_current_interpreter);

    if (n < 6)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_command");
        LUA_RETURN_EMPTY;
    }
    
    command = lua_tostring (lua_current_interpreter, -6);
    description = lua_tostring (lua_current_interpreter, -5);
    args = lua_tostring (lua_current_interpreter, -4);
    args_description = lua_tostring (lua_current_interpreter, -3);
    completion = lua_tostring (lua_current_interpreter, -2);
    function = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (script_api_hook_command (weechat_lua_plugin,
                                                      lua_current_script,
                                                      (char *)command,
                                                      (char *)description,
                                                      (char *)args,
                                                      (char *)args_description,
                                                      (char *)completion,
                                                      &weechat_lua_api_hook_command_cb,
                                                      (char *)function));
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_hook_timer_cb: callback for timer hooked
 */

int
weechat_lua_api_hook_timer_cb (void *data)
{
    struct t_script_callback *script_callback;
    char *lua_argv[1] = { NULL };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    rc = (int *) weechat_lua_exec (script_callback->script,
                                   WEECHAT_SCRIPT_EXEC_INT,
                                   script_callback->function,
                                   lua_argv);
    
    if (!rc)
        ret = WEECHAT_RC_ERROR;
    else
    {
        ret = *rc;
        free (rc);
    }
    
    return ret;
}

/*
 * weechat_lua_api_hook_timer: hook a timer
 */

static int
weechat_lua_api_hook_timer (lua_State *L)
{
    int n, interval, align_second, max_calls;
    const char *function;
    char *result;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_timer");
        LUA_RETURN_EMPTY;
    }
    
    interval = 10;
    align_second = 0;
    max_calls = 0;
    function = NULL;
    
    n = lua_gettop (lua_current_interpreter);

    if (n < 4)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_timer");
        LUA_RETURN_EMPTY;
    }
    
    interval = lua_tonumber (lua_current_interpreter, -4);
    align_second = lua_tonumber (lua_current_interpreter, -3);
    max_calls = lua_tonumber (lua_current_interpreter, -2);
    function = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (script_api_hook_timer (weechat_lua_plugin,
                                                    lua_current_script,
                                                    interval,
                                                    align_second,
                                                    max_calls,
                                                    &weechat_lua_api_hook_timer_cb,
                                                    (char *)function));
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_hook_fd_cb: callback for fd hooked
 */

int
weechat_lua_api_hook_fd_cb (void *data)
{
    struct t_script_callback *script_callback;
    char *lua_argv[1] = { NULL };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    rc = (int *) weechat_lua_exec (script_callback->script,
                                   WEECHAT_SCRIPT_EXEC_INT,
                                   script_callback->function,
                                   lua_argv);
    
    if (!rc)
        ret = WEECHAT_RC_ERROR;
    else
    {
        ret = *rc;
        free (rc);
    }
    
    return ret;
}

/*
 * weechat_lua_api_hook_fd: hook a fd
 */

static int
weechat_lua_api_hook_fd (lua_State *L)
{
    int n, fd, read, write, exception;
    const char *function;
    char *result;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_fd");
        LUA_RETURN_EMPTY;
    }
    
    fd = 0;
    read = 0;
    write = 0;
    exception = 0;
    function = NULL;
    
    n = lua_gettop (lua_current_interpreter);

    if (n < 5)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_fd");
        LUA_RETURN_EMPTY;
    }
    
    fd = lua_tonumber (lua_current_interpreter, -5);
    read = lua_tonumber (lua_current_interpreter, -4);
    write = lua_tonumber (lua_current_interpreter, -3);
    exception = lua_tonumber (lua_current_interpreter, -2);
    function = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (script_api_hook_fd (weechat_lua_plugin,
                                                 lua_current_script,
                                                 fd,
                                                 read,
                                                 write,
                                                 exception,
                                                 &weechat_lua_api_hook_fd_cb,
                                                 (char *)function));
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_hook_print_cb: callback for print hooked
 */

int
weechat_lua_api_hook_print_cb (void *data, struct t_gui_buffer *buffer,
                               time_t date, char *prefix, char *message)
{
    struct t_script_callback *script_callback;
    char *lua_argv[5];
    static char timebuffer[64];
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;

    snprintf (timebuffer, sizeof (timebuffer) - 1, "%ld", date);
    
    lua_argv[0] = script_ptr2str (buffer);
    lua_argv[1] = timebuffer;
    lua_argv[2] = prefix;
    lua_argv[3] = message;
    lua_argv[4] = NULL;
    
    rc = (int *) weechat_lua_exec (script_callback->script,
                                   WEECHAT_SCRIPT_EXEC_INT,
                                   script_callback->function,
                                   lua_argv);
    
    if (!rc)
        ret = WEECHAT_RC_ERROR;
    else
    {
        ret = *rc;
        free (rc);
    }
    
    return ret;
}

/*
 * weechat_lua_api_hook_print: hook a print
 */

static int
weechat_lua_api_hook_print (lua_State *L)
{
    const char *buffer, *message, *function;
    char *result;
    int n, strip_colors;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_print");
        LUA_RETURN_EMPTY;
    }
    
    buffer = NULL;
    message = NULL;
    strip_colors = 0;
    function = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 4)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_print");
        LUA_RETURN_EMPTY;
    }
    
    buffer = lua_tostring (lua_current_interpreter, -4);
    message = lua_tostring (lua_current_interpreter, -3);
    strip_colors = lua_tonumber (lua_current_interpreter, -2);
    function = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (script_api_hook_print (weechat_lua_plugin,
                                                    lua_current_script,
                                                    script_str2ptr ((char *)buffer),
                                                    (char *)message,
                                                    strip_colors,
                                                    &weechat_lua_api_hook_print_cb,
                                                    (char *)function));
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_hook_signal_cb: callback for signal hooked
 */

int
weechat_lua_api_hook_signal_cb (void *data, char *signal, char *type_data,
                                void *signal_data)
{
    struct t_script_callback *script_callback;
    char *lua_argv[3];
    static char value_str[64];
    int *rc, ret, free_needed;
    
    script_callback = (struct t_script_callback *)data;

    lua_argv[0] = signal;
    free_needed = 0;
    if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_STRING) == 0)
    {
        lua_argv[1] = (char *)signal_data;
    }
    else if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_INT) == 0)
    {
        snprintf (value_str, sizeof (value_str) - 1,
                  "%d", *((int *)signal_data));
        lua_argv[1] = value_str;
    }
    else if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_POINTER) == 0)
    {
        lua_argv[1] = script_ptr2str (signal_data);
        free_needed = 1;
    }
    else
        lua_argv[1] = NULL;
    lua_argv[2] = NULL;
    
    rc = (int *) weechat_lua_exec (script_callback->script,
                                   WEECHAT_SCRIPT_EXEC_INT,
                                   script_callback->function,
                                   lua_argv);
    
    if (!rc)
        ret = WEECHAT_RC_ERROR;
    else
    {
        ret = *rc;
        free (rc);
    }
    
    return ret;
}

/*
 * weechat_lua_api_hook_signal: hook a signal
 */

static int
weechat_lua_api_hook_signal (lua_State *L)
{
    const char *signal, *function;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_signal");
        LUA_RETURN_EMPTY;
    }
    
    signal = NULL;
    function = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_signal");
        LUA_RETURN_EMPTY;
    }
    
    signal = lua_tostring (lua_current_interpreter, -2);
    function = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (script_api_hook_signal (weechat_lua_plugin,
                                                     lua_current_script,
                                                     (char *)signal,
                                                     &weechat_lua_api_hook_signal_cb,
                                                     (char *)function));
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_hook_signal_send: send a signal
 */

static int
weechat_lua_api_hook_signal_send (lua_State *L)
{
    const char *signal, *type_data, *signal_data;
    int n, number;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_signal_send");
        LUA_RETURN_ERROR;
    }
    
    signal = NULL;
    type_data = NULL;
    signal_data = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_signal_send");
        LUA_RETURN_ERROR;
    }
    
    signal = lua_tostring (lua_current_interpreter, -3);
    type_data = lua_tostring (lua_current_interpreter, -2);

    if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_STRING) == 0)
    {
        signal_data = lua_tostring (lua_current_interpreter, -1);
        weechat_hook_signal_send ((char *)signal, (char *)type_data,
                                  (char *)signal_data);
        LUA_RETURN_OK;
    }
    else if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_INT) == 0)
    {
        number = lua_tonumber (lua_current_interpreter, -1);
        weechat_hook_signal_send ((char *)signal, (char *)type_data,
                                  &number);
        LUA_RETURN_OK;
    }
    else if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_POINTER) == 0)
    {
        signal_data = lua_tostring (lua_current_interpreter, -1);
        weechat_hook_signal_send ((char *)signal, (char *)type_data,
                                  script_str2ptr ((char *)signal_data));
        LUA_RETURN_OK;
    }
    
    LUA_RETURN_ERROR;
}

/*
 * weechat_lua_api_hook_config_cb: callback for config option hooked
 */

int
weechat_lua_api_hook_config_cb (void *data, char *type, char *option,
                                char *value)
{
    struct t_script_callback *script_callback;
    char *lua_argv[4];
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    lua_argv[0] = type;
    lua_argv[1] = option;
    lua_argv[2] = value;
    lua_argv[3] = NULL;
    
    rc = (int *) weechat_lua_exec (script_callback->script,
                                   WEECHAT_SCRIPT_EXEC_INT,
                                   script_callback->function,
                                   lua_argv);
    
    if (!rc)
        ret = WEECHAT_RC_ERROR;
    else
    {
        ret = *rc;
        free (rc);
    }
    
    return ret;
}

/*
 * weechat_lua_api_hook_config: hook a config option
 */

static int
weechat_lua_api_hook_config (lua_State *L)
{
    const char *type, *option, *function;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_config");
        LUA_RETURN_EMPTY;
    }
    
    type = NULL;
    option = NULL;
    function = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_config");
        LUA_RETURN_EMPTY;
    }
    
    type = lua_tostring (lua_current_interpreter, -3);
    option = lua_tostring (lua_current_interpreter, -2);
    function = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (script_api_hook_config (weechat_lua_plugin,
                                                     lua_current_script,
                                                     (char *)type,
                                                     (char *)option,
                                                     &weechat_lua_api_hook_config_cb,
                                                     (char *)function));
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_hook_completion_cb: callback for completion hooked
 */

int
weechat_lua_api_hook_completion_cb (void *data, char *completion,
                                    struct t_gui_buffer *buffer,
                                    struct t_weelist *list)
{
    struct t_script_callback *script_callback;
    char *lua_argv[4];
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    lua_argv[0] = completion;
    lua_argv[1] = script_ptr2str (buffer);
    lua_argv[2] = script_ptr2str (list);
    lua_argv[3] = NULL;
    
    rc = (int *) weechat_lua_exec (script_callback->script,
                                   WEECHAT_SCRIPT_EXEC_INT,
                                   script_callback->function,
                                   lua_argv);
    
    if (!rc)
        ret = WEECHAT_RC_ERROR;
    else
    {
        ret = *rc;
        free (rc);
    }
    if (lua_argv[1])
        free (lua_argv[1]);
    if (lua_argv[2])
        free (lua_argv[2]);
    
    return ret;
}

/*
 * weechat_lua_api_hook_completion: hook a completion
 */

static int
weechat_lua_api_hook_completion (lua_State *L)
{
    const char *completion, *function;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_completion");
        LUA_RETURN_EMPTY;
    }
    
    completion = NULL;
    function = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_completion");
        LUA_RETURN_EMPTY;
    }
    
    completion = lua_tostring (lua_current_interpreter, -2);
    function = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (script_api_hook_completion (weechat_lua_plugin,
                                                         lua_current_script,
                                                         (char *)completion,
                                                         &weechat_lua_api_hook_completion_cb,
                                                         (char *)function));
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_hook_modifier_cb: callback for modifier hooked
 */

char *
weechat_lua_api_hook_modifier_cb (void *data, char *modifier,
                                  char *modifier_data, char *string)
{
    struct t_script_callback *script_callback;
    char *lua_argv[4];
    
    script_callback = (struct t_script_callback *)data;
    
    lua_argv[0] = modifier;
    lua_argv[1] = modifier_data;
    lua_argv[2] = string;
    lua_argv[3] = NULL;
    
    return (char *)weechat_lua_exec (script_callback->script,
                                     WEECHAT_SCRIPT_EXEC_STRING,
                                     script_callback->function,
                                     lua_argv);
}

/*
 * weechat_lua_api_hook_modifier: hook a modifier
 */

static int
weechat_lua_api_hook_modifier (lua_State *L)
{
    const char *modifier, *function;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_modifier");
        LUA_RETURN_EMPTY;
    }
    
    modifier = NULL;
    function = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_modifier");
        LUA_RETURN_EMPTY;
    }
    
    modifier = lua_tostring (lua_current_interpreter, -2);
    function = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (script_api_hook_modifier (weechat_lua_plugin,
                                                       lua_current_script,
                                                       (char *)modifier,
                                                       &weechat_lua_api_hook_modifier_cb,
                                                       (char *)function));
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_hook_modifier_exec: execute a modifier hook
 */

static int
weechat_lua_api_hook_modifier_exec (lua_State *L)
{
    const char *modifier, *modifier_data, *string;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_modifier_exec");
        LUA_RETURN_EMPTY;
    }
    
    modifier = NULL;
    modifier_data = NULL;
    string = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_modifier_exec");
        LUA_RETURN_ERROR;
    }
    
    modifier = lua_tostring (lua_current_interpreter, -3);
    modifier_data = lua_tostring (lua_current_interpreter, -2);
    string = lua_tostring (lua_current_interpreter, -1);
    
    result = weechat_hook_modifier_exec ((char *)modifier,
                                         (char *)modifier_data,
                                         (char *)string);
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_unhook: unhook something
 */

static int
weechat_lua_api_unhook (lua_State *L)
{
    const char *hook;
    int n;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("unhook");
        LUA_RETURN_ERROR;
    }
    
    hook = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("unhook");
        LUA_RETURN_ERROR;
    }
    
    hook = lua_tostring (lua_current_interpreter, -1);
    
    if (script_api_unhook (weechat_lua_plugin,
                           lua_current_script,
                           script_str2ptr ((char *)hook)))
        LUA_RETURN_OK;

    LUA_RETURN_ERROR;
}

/*
 * weechat_lua_api_unhook_all: unhook all for script
 */

static int
weechat_lua_api_unhook_all (lua_State *L)
{
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("unhook_all");
        LUA_RETURN_ERROR;
    }
    
    script_api_unhook_all (weechat_lua_plugin,
                           lua_current_script);
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_input_data_cb: callback for input data in a buffer
 */

int
weechat_lua_api_input_data_cb (void *data, struct t_gui_buffer *buffer,
                               char *input_data)
{
    struct t_script_callback *script_callback;
    char *lua_argv[3];
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    lua_argv[0] = script_ptr2str (buffer);
    lua_argv[2] = input_data;
    lua_argv[3] = NULL;
    
    rc = (int *) weechat_lua_exec (script_callback->script,
                                   WEECHAT_SCRIPT_EXEC_INT,
                                   script_callback->function,
                                   lua_argv);
    
    if (!rc)
        ret = WEECHAT_RC_ERROR;
    else
    {
        ret = *rc;
        free (rc);
    }
    if (lua_argv[0])
        free (lua_argv[0]);
    
    return ret;
}

/*
 * weechat_lua_api_buffer_new: create a new buffer
 */

static int
weechat_lua_api_buffer_new (lua_State *L)
{
    const char *category, *name, *function;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("buffer_new");
        LUA_RETURN_EMPTY;
    }
    
    category = NULL;
    name = NULL;
    function = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("buffer_new");
        LUA_RETURN_EMPTY;
    }
    
    category = lua_tostring (lua_current_interpreter, -3);
    name = lua_tostring (lua_current_interpreter, -2);
    function = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (script_api_buffer_new (weechat_lua_plugin,
                                                    lua_current_script,
                                                    (char *)category,
                                                    (char *)name,
                                                    &weechat_lua_api_input_data_cb,
                                                    (char *)function));
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_buffer_search: search a buffer
 */

static int
weechat_lua_api_buffer_search (lua_State *L)
{
    const char *category, *name;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("buffer_search");
        LUA_RETURN_EMPTY;
    }
    
    category = NULL;
    name = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("buffer_search");
        LUA_RETURN_EMPTY;
    }
    
    category = lua_tostring (lua_current_interpreter, -2);
    name = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (weechat_buffer_search ((char *)category,
                                                    (char *)name));
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_buffer_close: close a buffer
 */

static int
weechat_lua_api_buffer_close (lua_State *L)
{
    const char *buffer;
    int n, switch_to_another;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("buffer_close");
        LUA_RETURN_ERROR;
    }
    
    buffer = NULL;
    switch_to_another = 0;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("buffer_close");
        LUA_RETURN_ERROR;
    }
    
    buffer = lua_tostring (lua_current_interpreter, -2);
    switch_to_another = lua_tonumber (lua_current_interpreter, -1);
    
    script_api_buffer_close (weechat_lua_plugin,
                             lua_current_script,
                             script_str2ptr ((char *)buffer),
                             switch_to_another);
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_buffer_get: get a buffer property
 */

static int
weechat_lua_api_buffer_get (lua_State *L)
{
    const char *buffer, *property;
    char *value;
    int n;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("buffer_get");
        LUA_RETURN_EMPTY;
    }
    
    buffer = NULL;
    property = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("buffer_get");
        LUA_RETURN_EMPTY;
    }
    
    buffer = lua_tostring (lua_current_interpreter, -2);
    property = lua_tostring (lua_current_interpreter, -1);
    
    value = weechat_buffer_get (script_str2ptr ((char *)buffer),
                                (char *)property);
    LUA_RETURN_STRING(value);
}

/*
 * weechat_lua_api_buffer_set: set a buffer property
 */

static int
weechat_lua_api_buffer_set (lua_State *L)
{
    const char *buffer, *property, *value;
    int n;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("buffer_set");
        LUA_RETURN_ERROR;
    }
    
    buffer = NULL;
    property = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("buffer_set");
        LUA_RETURN_ERROR;
    }
    
    buffer = lua_tostring (lua_current_interpreter, -3);
    property = lua_tostring (lua_current_interpreter, -2);
    value = lua_tostring (lua_current_interpreter, -1);
    
    weechat_buffer_set (script_str2ptr ((char *)buffer),
                        (char *)property,
                        (char *)value);
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_nicklist_add_group: add a group in nicklist
 */

static int
weechat_lua_api_nicklist_add_group (lua_State *L)
{
    const char *buffer, *parent_group, *name, *color;
    char *result;
    int n, visible;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("nicklist_add_group");
        LUA_RETURN_EMPTY;
    }
    
    buffer = NULL;
    parent_group = NULL;
    name = NULL;
    color = NULL;
    visible = 0;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 5)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("nicklist_add_group");
        LUA_RETURN_EMPTY;
    }
    
    buffer = lua_tostring (lua_current_interpreter, -5);
    parent_group = lua_tostring (lua_current_interpreter, -4);
    name = lua_tostring (lua_current_interpreter, -3);
    color = lua_tostring (lua_current_interpreter, -2);
    visible = lua_tonumber (lua_current_interpreter, -1);
    
    result = script_ptr2str (weechat_nicklist_add_group (script_str2ptr ((char *)buffer),
                                                         script_str2ptr ((char *)parent_group),
                                                         (char *)name,
                                                         (char *)color,
                                                         visible));
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_nicklist_search_group: search a group in nicklist
 */

static int
weechat_lua_api_nicklist_search_group (lua_State *L)
{
    const char *buffer, *from_group, *name;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("nicklist_search_group");
        LUA_RETURN_EMPTY;
    }
    
    buffer = NULL;
    from_group = NULL;
    name = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("nicklist_search_group");
        LUA_RETURN_EMPTY;
    }
    
    buffer = lua_tostring (lua_current_interpreter, -3);
    from_group = lua_tostring (lua_current_interpreter, -2);
    name = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (weechat_nicklist_search_group (script_str2ptr ((char *)buffer),
                                                            script_str2ptr ((char *)from_group),
                                                            (char *)name));
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_nicklist_add_nick: add a nick in nicklist
 */

static int
weechat_lua_api_nicklist_add_nick (lua_State *L)
{
    const char *buffer, *group, *name, *color, *prefix, *prefix_color;
    char char_prefix, *result;
    int n, visible;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("nicklist_add_nick");
        LUA_RETURN_EMPTY;
    }
    
    buffer = NULL;
    group = NULL;
    name = NULL;
    color = NULL;
    prefix = NULL;
    prefix_color = NULL;
    visible = 0;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 7)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("nicklist_add_nick");
        LUA_RETURN_EMPTY;
    }
    
    buffer = lua_tostring (lua_current_interpreter, -7);
    group = lua_tostring (lua_current_interpreter, -6);
    name = lua_tostring (lua_current_interpreter, -5);
    color = lua_tostring (lua_current_interpreter, -4);
    prefix = lua_tostring (lua_current_interpreter, -3);
    prefix_color = lua_tostring (lua_current_interpreter, -2);
    visible = lua_tonumber (lua_current_interpreter, -1);
    
    if (prefix && prefix[0])
        char_prefix = prefix[0];
    else
        char_prefix = ' ';
    
    result = script_ptr2str (weechat_nicklist_add_nick (script_str2ptr ((char *)buffer),
                                                        script_str2ptr ((char *)group),
                                                        (char *)name,
                                                        (char *)color,
                                                        char_prefix,
                                                        (char *)prefix_color,
                                                        visible));
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_nicklist_search_nick: search a nick in nicklist
 */

static int
weechat_lua_api_nicklist_search_nick (lua_State *L)
{
    const char *buffer, *from_group, *name;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("nicklist_search_nick");
        LUA_RETURN_EMPTY;
    }
    
    buffer = NULL;
    from_group = NULL;
    name = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("nicklist_search_nick");
        LUA_RETURN_EMPTY;
    }
    
    buffer = lua_tostring (lua_current_interpreter, -3);
    from_group = lua_tostring (lua_current_interpreter, -2);
    name = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (weechat_nicklist_search_nick (script_str2ptr ((char *)buffer),
                                                           script_str2ptr ((char *)from_group),
                                                           (char *)name));
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_nicklist_remove_group: remove a group from nicklist
 */

static int
weechat_lua_api_nicklist_remove_group (lua_State *L)
{
    const char *buffer, *group;
    int n;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("nicklist_remove_group");
        LUA_RETURN_ERROR;
    }
    
    buffer = NULL;
    group = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("nicklist_remove_group");
        LUA_RETURN_ERROR;
    }
    
    buffer = lua_tostring (lua_current_interpreter, -3);
    group = lua_tostring (lua_current_interpreter, -2);
    
    weechat_nicklist_remove_group (script_str2ptr ((char *)buffer),
                                   script_str2ptr ((char *)group));
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_nicklist_remove_nick: remove a nick from nicklist
 */

static int
weechat_lua_api_nicklist_remove_nick (lua_State *L)
{
    const char *buffer, *nick;
    int n;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("nicklist_remove_nick");
        LUA_RETURN_ERROR;
    }
    
    buffer = NULL;
    nick = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("nicklist_remove_nick");
        LUA_RETURN_ERROR;
    }
    
    buffer = lua_tostring (lua_current_interpreter, -3);
    nick = lua_tostring (lua_current_interpreter, -2);
    
    weechat_nicklist_remove_nick (script_str2ptr ((char *)buffer),
                                  script_str2ptr ((char *)nick));
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_nicklist_remove_all: remove all groups/nicks from nicklist
 */

static int
weechat_lua_api_nicklist_remove_all (lua_State *L)
{
    const char *buffer;
    int n;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("nicklist_remove_all");
        LUA_RETURN_ERROR;
    }
    
    buffer = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("nicklist_remove_all");
        LUA_RETURN_ERROR;
    }
    
    buffer = lua_tostring (lua_current_interpreter, -3);
    
    weechat_nicklist_remove_all (script_str2ptr ((char *)buffer));
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_command: send command to server
 */

static int
weechat_lua_api_command (lua_State *L)
{
    const char *buffer, *command;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("command");
        LUA_RETURN_ERROR;
    }
    
    command = NULL;
    
    n = lua_gettop (lua_current_interpreter);

    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("command");
        LUA_RETURN_ERROR;
    }
    
    buffer = lua_tostring (lua_current_interpreter, -2);
    command = lua_tostring (lua_current_interpreter, -1);
    
    script_api_command (weechat_lua_plugin,
                        lua_current_script,
                        script_str2ptr ((char *)buffer),
                        (char *)command);

    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_info_get: get info about WeeChat
 */

static int
weechat_lua_api_info_get (lua_State *L)
{
    const char *info;
    char *value;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("info_get");
        LUA_RETURN_EMPTY;
    }
    
    info = NULL;
    
    n = lua_gettop (lua_current_interpreter);

    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("info_get");
        LUA_RETURN_EMPTY;
    }

    info = lua_tostring (lua_current_interpreter, -1);
    
    value = weechat_info_get ((char *)info);
    LUA_RETURN_STRING(value);
}

/*
 * weechat_lua_api_get_dcc_info: get infos about DCC
 */

/*
static int
weechat_lua_api_get_dcc_info (lua_State *L)
{
    t_plugin_dcc_info *dcc_info, *ptr_dcc;
    char timebuffer1[64];
    char timebuffer2[64];
    struct in_addr in;
    int i;
    
    // make C compiler happy
    (void) L;
    
    if (!lua_current_script)
    {
        lua_plugin->print_server (lua_plugin,
                                  "Lua error: unable to get DCC info, "
                                  "script not initialized");
	lua_pushnil (lua_current_interpreter);
	return 1;
    }
    
    dcc_info = lua_plugin->get_dcc_info (lua_plugin);
    if (!dcc_info)
    {
	lua_pushboolean (lua_current_interpreter, 0);
	return 1;
    }
    
    lua_newtable (lua_current_interpreter);

    for (i = 0, ptr_dcc = dcc_info; ptr_dcc; ptr_dcc = ptr_dcc->next_dcc, i++)
    {
	strftime(timebuffer1, sizeof(timebuffer1), "%F %T",
		 localtime(&ptr_dcc->start_time));
	strftime(timebuffer2, sizeof(timebuffer2), "%F %T",
		 localtime(&ptr_dcc->start_transfer));
	in.s_addr = htonl(ptr_dcc->addr);
	
	lua_pushnumber (lua_current_interpreter, i);
	lua_newtable (lua_current_interpreter);

	lua_pushstring (lua_current_interpreter, "server");
	lua_pushstring (lua_current_interpreter, ptr_dcc->server);
	lua_rawset (lua_current_interpreter, -3);
		    
	lua_pushstring (lua_current_interpreter, "channel");
	lua_pushstring (lua_current_interpreter, ptr_dcc->channel);
	lua_rawset (lua_current_interpreter, -3);
		    
	lua_pushstring (lua_current_interpreter, "type");
	lua_pushnumber (lua_current_interpreter, ptr_dcc->type);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "status");
	lua_pushnumber (lua_current_interpreter, ptr_dcc->status);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "start_time");
	lua_pushstring (lua_current_interpreter, timebuffer1);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "start_transfer");
	lua_pushstring (lua_current_interpreter, timebuffer2);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "address");
	lua_pushstring (lua_current_interpreter, inet_ntoa(in));
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "port");
	lua_pushnumber (lua_current_interpreter, ptr_dcc->port);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "nick");
	lua_pushstring (lua_current_interpreter, ptr_dcc->nick);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "remote_file");
	lua_pushstring (lua_current_interpreter, ptr_dcc->filename);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "local_file");
	lua_pushstring (lua_current_interpreter, ptr_dcc->local_filename);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "filename_suffix");
	lua_pushnumber (lua_current_interpreter, ptr_dcc->filename_suffix);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "size");
	lua_pushnumber (lua_current_interpreter, ptr_dcc->size);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "pos");
	lua_pushnumber (lua_current_interpreter, ptr_dcc->pos);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "start_resume");
	lua_pushnumber (lua_current_interpreter, ptr_dcc->start_resume);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "cps");
	lua_pushnumber (lua_current_interpreter, ptr_dcc->bytes_per_sec);
	lua_rawset (lua_current_interpreter, -3);

	lua_rawset (lua_current_interpreter, -3);
    }
    
    lua_plugin->free_dcc_info (lua_plugin, dcc_info);
    
    return 1;
}
*/

/*
 * weechat_lua_api_get_config: get value of a WeeChat config option
 */

/*
static int
weechat_lua_api_get_config (lua_State *L)
{
    const char *option;
    char *return_value;
    int n;
    
    // make C compiler happy
    (void) L;
     
    if (!lua_current_script)
    {
        lua_plugin->print_server (lua_plugin,
                                  "Lua error: unable to get config option, "
                                  "script not initialized");	
	lua_pushnumber (lua_current_interpreter, 0);
	return 1;
    }
    
    option = NULL;
 
    n = lua_gettop (lua_current_interpreter);

    if (n != 1)
    {
	lua_plugin->print_server (lua_plugin,
                                  "Lua error: wrong parameters for "
                                  "\"get_config\" function");
        lua_pushnumber (lua_current_interpreter, 0);
	return 1;
    }
    
    option = lua_tostring (lua_current_interpreter, -1);
    
    return_value = lua_plugin->get_config (lua_plugin, (char *) option);    
    if (return_value)
	lua_pushstring (lua_current_interpreter, return_value);
    else
	lua_pushstring (lua_current_interpreter, "");
    
    return 1;
}
*/

/*
 * weechat_lua_api_set_config: set value of a WeeChat config option
 */

/*
static int
weechat_lua_api_set_config (lua_State *L)
{
    const char *option, *value;
    int n;
    
    // make C compiler happy
    (void) L;
    
    if (!lua_current_script)
    {
        lua_plugin->print_server (lua_plugin,
                                  "Lua error: unable to set config option, "
                                  "script not initialized");
	lua_pushnumber (lua_current_interpreter, 0);
	return 1;
    }
    
    option = NULL;
    value = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n != 2)
    {
	lua_plugin->print_server (lua_plugin,
                                  "Lua error: wrong parameters for "
                                  "\"set_config\" function");
	lua_pushnumber (lua_current_interpreter, 0);
	return 1;
    }
    
    option = lua_tostring (lua_current_interpreter, -2);
    value = lua_tostring (lua_current_interpreter, -1);

    if (lua_plugin->set_config (lua_plugin, (char *) option, (char *) value))
	lua_pushnumber (lua_current_interpreter, 1);
    else
	lua_pushnumber (lua_current_interpreter, 0);
    
    return 1;
}
*/

/*
 * weechat_lua_api_get_plugin_config: get value of a plugin config option
 */

/*
static int
weechat_lua_api_get_plugin_config (lua_State *L)
{
    const char *option;
    char *return_value;
    int n;
    
    // make C compiler happy
    (void) L;
     
    if (!lua_current_script)
    {
        lua_plugin->print_server (lua_plugin,
                                  "Lua error: unable to get plugin config option, "
                                  "script not initialized");	
	lua_pushnumber (lua_current_interpreter, 0);
	return 1;
    }
    
    option = NULL;
 
    n = lua_gettop (lua_current_interpreter);

    if (n != 1)
    {
	lua_plugin->print_server (lua_plugin,
                                  "Lua error: wrong parameters for "
                                  "\"get_plugin_config\" function");
        lua_pushnumber (lua_current_interpreter, 0);
	return 1;
    }
    
    option = lua_tostring (lua_current_interpreter, -1);
    
    return_value = weechat_script_get_plugin_config (lua_plugin,
						     lua_current_script,
						     (char *) option);
    if (return_value)
	lua_pushstring (lua_current_interpreter, return_value);
    else
	lua_pushstring (lua_current_interpreter, "");
    
    return 1;
}
*/

/*
 * weechat_lua_api_set_plugin_config: set value of a plugin config option
 */

/*
static int
weechat_lua_api_set_plugin_config (lua_State *L)
{
    const char *option, *value;
    int n;
    
    // make C compiler happy
    (void) L;
 	
    if (!lua_current_script)
    {
        lua_plugin->print_server (lua_plugin,
                                  "Lua error: unable to set plugin config option, "
                                  "script not initialized");
	lua_pushnumber (lua_current_interpreter, 0);
	return 1;
    }
    
    option = NULL;
    value = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n != 2)
    {
	lua_plugin->print_server (lua_plugin,
                                  "Lua error: wrong parameters for "
                                  "\"set_plugin_config\" function");
	lua_pushnumber (lua_current_interpreter, 0);
	return 1;
    }
    
    option = lua_tostring (lua_current_interpreter, -2);
    value = lua_tostring (lua_current_interpreter, -1);

    if (weechat_script_set_plugin_config (lua_plugin,
					  lua_current_script,
					  (char *) option, (char *) value))
	lua_pushnumber (lua_current_interpreter, 1);
    else
	lua_pushnumber (lua_current_interpreter, 0);
    
    return 1;
}
*/

/*
 * weechat_lua_api_get_server_info: get infos about servers
 */

/*
static int
weechat_lua_api_get_server_info (lua_State *L)
{
    t_plugin_server_info *server_info, *ptr_server;
    char timebuffer[64];
    
    // make C compiler happy
    (void) L;
    
    if (!lua_current_script)
    {
        lua_plugin->print_server (lua_plugin,
                                  "Lua error: unable to get server infos, "
                                  "script not initialized");
	lua_pushnil (lua_current_interpreter);
	return 1;
    }
    
    server_info = lua_plugin->get_server_info (lua_plugin);
    if  (!server_info) {
	lua_pushboolean (lua_current_interpreter, 0);
	return 1;
    }

    lua_newtable (lua_current_interpreter);

    for (ptr_server = server_info; ptr_server; ptr_server = ptr_server->next_server)
    {
	strftime(timebuffer, sizeof(timebuffer), "%F %T",
		 localtime(&ptr_server->away_time));
	
	lua_pushstring (lua_current_interpreter, ptr_server->name);
	lua_newtable (lua_current_interpreter);
	
	lua_pushstring (lua_current_interpreter, "autoconnect");
	lua_pushnumber (lua_current_interpreter, ptr_server->autoconnect);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "autoreconnect");
	lua_pushnumber (lua_current_interpreter, ptr_server->autoreconnect);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "autoreconnect_delay");
	lua_pushnumber (lua_current_interpreter, ptr_server->autoreconnect_delay);
	lua_rawset (lua_current_interpreter, -3);
		
	lua_pushstring (lua_current_interpreter, "temp_server");
	lua_pushnumber (lua_current_interpreter, ptr_server->temp_server);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "address");
	lua_pushstring (lua_current_interpreter, ptr_server->address);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "port");
	lua_pushnumber (lua_current_interpreter, ptr_server->port);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "ipv6");
	lua_pushnumber (lua_current_interpreter, ptr_server->ipv6);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "ssl");
	lua_pushnumber (lua_current_interpreter, ptr_server->ssl);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "password");
	lua_pushstring (lua_current_interpreter, ptr_server->password);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "nick1");
	lua_pushstring (lua_current_interpreter, ptr_server->nick1);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "nick2");
	lua_pushstring (lua_current_interpreter, ptr_server->nick2);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "nick3");
	lua_pushstring (lua_current_interpreter, ptr_server->nick3);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "username");
	lua_pushstring (lua_current_interpreter, ptr_server->username);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "realname");
	lua_pushstring (lua_current_interpreter, ptr_server->realname);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "command");
	lua_pushstring (lua_current_interpreter, ptr_server->command);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "command_delay");
	lua_pushnumber (lua_current_interpreter, ptr_server->command_delay);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "autojoin");
	lua_pushstring (lua_current_interpreter, ptr_server->autojoin);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "autorejoin");
	lua_pushnumber (lua_current_interpreter, ptr_server->autorejoin);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "notify_levels");
	lua_pushstring (lua_current_interpreter, ptr_server->notify_levels);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "is_connected");
	lua_pushnumber (lua_current_interpreter, ptr_server->is_connected);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "ssl_connected");
	lua_pushnumber (lua_current_interpreter, ptr_server->ssl_connected);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "nick");
	lua_pushstring (lua_current_interpreter, ptr_server->nick);
	lua_rawset (lua_current_interpreter, -3);
        
        lua_pushstring (lua_current_interpreter, "nick_modes");
	lua_pushstring (lua_current_interpreter, ptr_server->nick_modes);
	lua_rawset (lua_current_interpreter, -3);
        
	lua_pushstring (lua_current_interpreter, "away_time");
	lua_pushstring (lua_current_interpreter, timebuffer);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "lag");
	lua_pushnumber (lua_current_interpreter, ptr_server->lag);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_rawset (lua_current_interpreter, -3);
    }

    lua_plugin->free_server_info(lua_plugin, server_info);
    
    return 1;
}
*/

/*
 * weechat_lua_api_get_channel_info: get infos about channels
 */

/*
static int
weechat_lua_api_get_channel_info (lua_State *L)
{
    t_plugin_channel_info *channel_info, *ptr_channel;
    const char *server;
    int n;
    
    // make C compiler happy
    (void) L;
 
    if (!lua_current_script)
    {
        lua_plugin->print_server (lua_plugin,
                                  "Lua error: unable to get channel infos, "
                                  "script not initialized");
	lua_pushnil (lua_current_interpreter);
	return 1;
    }
    
    server = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n != 1)
    {
        lua_plugin->print_server (lua_plugin,
                                  "Lua error: wrong parameters for "
                                  "\"get_channel_info\" function");
        lua_pushnil (lua_current_interpreter);
	return 1;
    }

    server = lua_tostring (lua_current_interpreter, -1);
    
    channel_info = lua_plugin->get_channel_info (lua_plugin, (char *) server);
    if  (!channel_info)
    {
	lua_pushboolean (lua_current_interpreter, 0);
	return 1;
    }

    lua_newtable (lua_current_interpreter);

    for (ptr_channel = channel_info; ptr_channel; ptr_channel = ptr_channel->next_channel)
    {
	lua_pushstring (lua_current_interpreter, ptr_channel->name);
	lua_newtable (lua_current_interpreter);
	
	lua_pushstring (lua_current_interpreter, "type");
	lua_pushnumber (lua_current_interpreter, ptr_channel->type);
	lua_rawset (lua_current_interpreter, -3);

	lua_pushstring (lua_current_interpreter, "topic");
	lua_pushstring (lua_current_interpreter, ptr_channel->topic);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "modes");
	lua_pushstring (lua_current_interpreter, ptr_channel->modes);
	lua_rawset (lua_current_interpreter, -3);

	lua_pushstring (lua_current_interpreter, "limit");
	lua_pushnumber (lua_current_interpreter, ptr_channel->limit);
	lua_rawset (lua_current_interpreter, -3);

	lua_pushstring (lua_current_interpreter, "key");
	lua_pushstring (lua_current_interpreter, ptr_channel->key);
	lua_rawset (lua_current_interpreter, -3);

	lua_pushstring (lua_current_interpreter, "nicks_count");
	lua_pushnumber (lua_current_interpreter, ptr_channel->nicks_count);
	lua_rawset (lua_current_interpreter, -3);

	lua_rawset (lua_current_interpreter, -3);
    }    

    lua_plugin->free_channel_info(lua_plugin, channel_info);
    
    return 1;
}
*/

/*
 * weechat_lua_api_get_nick_info: get infos about nicks
 */

/*
static int
weechat_lua_api_get_nick_info (lua_State *L)
{
    t_plugin_nick_info *nick_info, *ptr_nick;
    const char *server, *channel;
    int n;
    
    // make C compiler happy
    (void) L;
     
    if (!lua_current_script)
    {
        lua_plugin->print_server (lua_plugin,
                                  "Lua error: unable to get nick infos, "
                                  "script not initialized");
		lua_pushnil (lua_current_interpreter);
	return 1;
    }
    
    server = NULL;
    channel = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n != 2)
    {
        lua_plugin->print_server (lua_plugin,
                                  "Lua error: wrong parameters for "
                                  "\"get_nick_info\" function");
        lua_pushnil (lua_current_interpreter);
	return 1;
    }

    server = lua_tostring (lua_current_interpreter, -2);
    channel = lua_tostring (lua_current_interpreter, -1);
    
    nick_info = lua_plugin->get_nick_info (lua_plugin, (char *) server, (char *) channel);
    if  (!nick_info)
    {
	lua_pushboolean (lua_current_interpreter, 0);
	return 1;
    }

    lua_newtable (lua_current_interpreter);
    
    for(ptr_nick = nick_info; ptr_nick; ptr_nick = ptr_nick->next_nick)
    {
	lua_pushstring (lua_current_interpreter, ptr_nick->nick);
	lua_newtable (lua_current_interpreter);
	
	lua_pushstring (lua_current_interpreter, "flags");
	lua_pushnumber (lua_current_interpreter, ptr_nick->flags);
	lua_rawset (lua_current_interpreter, -3);

	lua_pushstring (lua_current_interpreter, "host");
	lua_pushstring (lua_current_interpreter,
			ptr_nick->host ? ptr_nick->host : "");
	lua_rawset (lua_current_interpreter, -3);
	
	lua_rawset (lua_current_interpreter, -3);
    }
    
    lua_plugin->free_nick_info(lua_plugin, nick_info);
    
    return 1;
}
*/

/*
 * weechat_lua_api_get_irc_color:
 *          get the numeric value which identify an irc color by its name
 */

/*
static int
weechat_lua_api_get_irc_color (lua_State *L)
{
    const char *color;
    int n;
    
    // make C compiler happy
    (void) L;
     
    if (!lua_current_script)
    {
        lua_plugin->print_server (lua_plugin,
                                  "Lua error: unable to get irc color, "
                                  "script not initialized");
        lua_pushnumber (lua_current_interpreter, -1);
	return 1;
    }
    
    color = NULL;
 
    n = lua_gettop (lua_current_interpreter);
    
    if (n != 1)
    {
	lua_plugin->print_server (lua_plugin,
                                  "Lua error: wrong parameters for "
                                  "\"get_irc_color\" function");
        lua_pushnumber (lua_current_interpreter, -1);
	return 1;
    }

    color = lua_tostring (lua_current_interpreter, -1);
    
    lua_pushnumber (lua_current_interpreter,
		    lua_plugin->get_irc_color (lua_plugin, (char *) color));
    return 1;
}
*/

/*
 * weechat_lua_api_get_window_info: get infos about windows
 */

/*
static int
weechat_lua_api_get_window_info (lua_State *L)
{
    t_plugin_window_info *window_info, *ptr_win;
    int i;
    
    // make C compiler happy
    (void) L;
    
    if (!lua_current_script)
    {
        lua_plugin->print_server (lua_plugin,
                                  "Lua error: unable to get window info, "
                                  "script not initialized");
	lua_pushnil (lua_current_interpreter);
	return 1;
    }
    
    window_info = lua_plugin->get_window_info (lua_plugin);
    if (!window_info)
    {
	lua_pushboolean (lua_current_interpreter, 0);
	return 1;
    }
    
    lua_newtable (lua_current_interpreter);

    i = 0;
    for (ptr_win = window_info; ptr_win; ptr_win = ptr_win->next_window)
    {
	lua_pushnumber (lua_current_interpreter, i);
	lua_newtable (lua_current_interpreter);

	lua_pushstring (lua_current_interpreter, "num_buffer");
	lua_pushnumber (lua_current_interpreter, ptr_win->num_buffer);
	lua_rawset (lua_current_interpreter, -3);
		    
	lua_pushstring (lua_current_interpreter, "win_x");
	lua_pushnumber (lua_current_interpreter, ptr_win->win_x);
	lua_rawset (lua_current_interpreter, -3);
		    
	lua_pushstring (lua_current_interpreter, "win_y");
	lua_pushnumber (lua_current_interpreter, ptr_win->win_y);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "win_width");
	lua_pushnumber (lua_current_interpreter, ptr_win->win_width);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "win_height");
	lua_pushnumber (lua_current_interpreter, ptr_win->win_height);
	lua_rawset (lua_current_interpreter, -3);

	lua_pushstring (lua_current_interpreter, "win_width_pct");
	lua_pushnumber (lua_current_interpreter, ptr_win->win_width_pct);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "win_height_pct");
	lua_pushnumber (lua_current_interpreter, ptr_win->win_height_pct);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_rawset (lua_current_interpreter, -3);
        
        i++;
    }
    
    lua_plugin->free_window_info (lua_plugin, window_info);
    
    return 1;
}
*/

/*
 * weechat_lua_api_get_buffer_info: get infos about buffers
 */

/*
static int
weechat_lua_api_get_buffer_info (lua_State *L)
{
    t_plugin_buffer_info *buffer_info, *ptr_buffer;
    
    // make C compiler happy
    (void) L;
    
    if (!lua_current_script)
    {
        lua_plugin->print_server (lua_plugin,
                                  "Lua error: unable to get buffer info, "
                                  "script not initialized");
	lua_pushnil (lua_current_interpreter);
	return 1;
    }
    
    buffer_info = lua_plugin->get_buffer_info (lua_plugin);
    if  (!buffer_info) {
	lua_pushboolean (lua_current_interpreter, 0);
	return 1;
    }

    lua_newtable (lua_current_interpreter);

    for (ptr_buffer = buffer_info; ptr_buffer; ptr_buffer = ptr_buffer->next_buffer)
    {
	lua_pushnumber (lua_current_interpreter, ptr_buffer->number);
	lua_newtable (lua_current_interpreter);
	
	lua_pushstring (lua_current_interpreter, "type");
	lua_pushnumber (lua_current_interpreter, ptr_buffer->type);
	lua_rawset (lua_current_interpreter, -3);
        
	lua_pushstring (lua_current_interpreter, "num_displayed");
	lua_pushnumber (lua_current_interpreter, ptr_buffer->num_displayed);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "server");
	lua_pushstring (lua_current_interpreter, 
			ptr_buffer->server_name == NULL ? "" : ptr_buffer->server_name);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "channel");
	lua_pushstring (lua_current_interpreter, 
			ptr_buffer->channel_name == NULL ? "" : ptr_buffer->channel_name);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "notify_level");
	lua_pushnumber (lua_current_interpreter, ptr_buffer->notify_level);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "log_filename");
	lua_pushstring (lua_current_interpreter, 
			ptr_buffer->log_filename == NULL ? "" : ptr_buffer->log_filename);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_rawset (lua_current_interpreter, -3);
    }
    
    lua_plugin->free_buffer_info(lua_plugin, buffer_info);
    
    return 1;
}
*/

/*
 * weechat_lua_api_get_buffer_data: get buffer content
 */

/*
static int
weechat_lua_api_get_buffer_data (lua_State *L)
{
    t_plugin_buffer_line *buffer_data, *ptr_data;
    const char *server, *channel;
    char timebuffer[64];
    int i, n;
    
    // make C compiler happy
    (void) L;
    
    if (!lua_current_script)
    {
        lua_plugin->print_server (lua_plugin,
                                  "Lua error: unable to get buffer data, "
                                  "script not initialized");
	lua_pushnil (lua_current_interpreter);
	return 1;
    }

    server = NULL;
    channel = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    if (n != 2)
    {
	lua_plugin->print_server (lua_plugin,
				  "Lua error: wrong parameters for "
				  "\"get_buffer_data\" function");
	lua_pushnumber (lua_current_interpreter, 0);
	return 1;
    }
    
    server  = lua_tostring (lua_current_interpreter, -2);
    channel = lua_tostring (lua_current_interpreter, -1);
    
    buffer_data = lua_plugin->get_buffer_data (lua_plugin, (char *) server, (char *) channel);
    if (!buffer_data)
    {
	lua_pushboolean (lua_current_interpreter, 0);
	return 1;
    }
    
    lua_newtable (lua_current_interpreter);

    for (i = 0, ptr_data = buffer_data; ptr_data; ptr_data = ptr_data->next_line, i++)
    {
	lua_pushnumber (lua_current_interpreter, i);
	lua_newtable (lua_current_interpreter);

	strftime(timebuffer, sizeof(timebuffer), "%F %T",
		 localtime(&ptr_data->date));

	lua_pushstring (lua_current_interpreter, "date");
	lua_pushstring (lua_current_interpreter, timebuffer);
	lua_rawset (lua_current_interpreter, -3);

	lua_pushstring (lua_current_interpreter, "nick");
	lua_pushstring (lua_current_interpreter,
			ptr_data->nick == NULL ? "" : ptr_data->nick);
	lua_rawset (lua_current_interpreter, -3);
		    
	lua_pushstring (lua_current_interpreter, "data");
	lua_pushstring (lua_current_interpreter,
			ptr_data->data == NULL ? "" : ptr_data->data);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_rawset (lua_current_interpreter, -3);
    }
    
    lua_plugin->free_buffer_data (lua_plugin, buffer_data);
    
    return 1;
}
*/

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
weechat_lua_api_constant_weechat_rc_error (lua_State *L)
{
    /* make C compiler happy */
    (void) L;
    
    lua_pushnumber (lua_current_interpreter, WEECHAT_RC_ERROR);
    return 1;
}
    
static int
weechat_lua_api_constant_weechat_rc_ok_ignore_weechat (lua_State *L)
{
    /* make C compiler happy */
    (void) L;
    
    lua_pushnumber (lua_current_interpreter, WEECHAT_RC_OK_IGNORE_WEECHAT);
    return 1;
}

static int
weechat_lua_api_constant_weechat_rc_ok_ignore_plugins (lua_State *L)
{
    /* make C compiler happy */
    (void) L;
    
    lua_pushnumber (lua_current_interpreter, WEECHAT_RC_OK_IGNORE_PLUGINS);
    return 1;
}

static int
weechat_lua_api_constant_weechat_rc_ok_ignore_all (lua_State *L)
{
    /* make C compiler happy */
    (void) L;
    
    lua_pushnumber (lua_current_interpreter, WEECHAT_RC_OK_IGNORE_ALL);
    return 1;
}

static int
weechat_lua_api_constant_weechat_rc_ok_with_highlight (lua_State *L)
{
    /* make C compiler happy */
    (void) L;
    
    lua_pushnumber (lua_current_interpreter, WEECHAT_RC_OK_WITH_HIGHLIGHT);
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

const struct luaL_reg weechat_lua_api_funcs[] = {
    { "register", &weechat_lua_api_register },
    { "charset_set", &weechat_lua_api_charset_set },
    { "iconv_to_internal", &weechat_lua_api_iconv_to_internal },
    { "iconv_from_internal", &weechat_lua_api_iconv_from_internal },
    { "gettext", &weechat_lua_api_gettext },
    { "ngettext", &weechat_lua_api_ngettext },
    { "mkdir_home", &weechat_lua_api_mkdir_home },
    { "mkdir", &weechat_lua_api_mkdir },
    { "list_new", &weechat_lua_api_list_new },
    { "list_add", &weechat_lua_api_list_add },
    { "list_search", &weechat_lua_api_list_search },
    { "list_casesearch", &weechat_lua_api_list_casesearch },
    { "list_get", &weechat_lua_api_list_get },
    { "list_set", &weechat_lua_api_list_set },
    { "list_next", &weechat_lua_api_list_next },
    { "list_prev", &weechat_lua_api_list_prev },
    { "list_string", &weechat_lua_api_list_string },
    { "list_size", &weechat_lua_api_list_size },
    { "list_remove", &weechat_lua_api_list_remove },
    { "list_remove_all", &weechat_lua_api_list_remove_all },
    { "list_free", &weechat_lua_api_list_free },
    { "prefix", &weechat_lua_api_prefix },
    { "color", &weechat_lua_api_color },
    { "print", &weechat_lua_api_print },
    { "infobar_print", &weechat_lua_api_infobar_print },
    { "infobar_remove", &weechat_lua_api_infobar_remove },
    { "log_print", &weechat_lua_api_log_print },
    { "hook_command", &weechat_lua_api_hook_command },
    { "hook_timer", &weechat_lua_api_hook_timer },
    { "hook_fd", &weechat_lua_api_hook_fd },
    { "hook_print", &weechat_lua_api_hook_print },
    { "hook_signal", &weechat_lua_api_hook_signal },
    { "hook_signal_send", &weechat_lua_api_hook_signal_send },
    { "hook_config", &weechat_lua_api_hook_config },
    { "hook_completion", &weechat_lua_api_hook_completion },
    { "hook_modifier", &weechat_lua_api_hook_modifier },
    { "hook_modifier_exec", &weechat_lua_api_hook_modifier_exec },
    { "unhook", &weechat_lua_api_unhook },
    { "unhook_all", &weechat_lua_api_unhook_all },
    { "buffer_new", &weechat_lua_api_buffer_new },
    { "buffer_search", &weechat_lua_api_buffer_search },
    { "buffer_close", &weechat_lua_api_buffer_close },
    { "buffer_get", &weechat_lua_api_buffer_get },
    { "buffer_set", &weechat_lua_api_buffer_set },
    { "nicklist_add_group", &weechat_lua_api_nicklist_add_group },
    { "nicklist_search_group", &weechat_lua_api_nicklist_search_group },
    { "nicklist_add_nick", &weechat_lua_api_nicklist_add_nick },
    { "nicklist_search_nick", &weechat_lua_api_nicklist_search_nick },
    { "nicklist_remove_group", &weechat_lua_api_nicklist_remove_group },
    { "nicklist_remove_nick", &weechat_lua_api_nicklist_remove_nick },
    { "nicklist_remove_all", &weechat_lua_api_nicklist_remove_all },
    { "command", &weechat_lua_api_command },
    { "info_get", &weechat_lua_api_info_get },
    //{ "get_dcc_info", &weechat_lua_api_get_dcc_info },
    //{ "get_config", &weechat_lua_api_get_config },
    //{ "set_config", &weechat_lua_api_set_config },
    //{ "get_plugin_config", &weechat_lua_api_get_plugin_config },
    //{ "set_plugin_config", &weechat_lua_api_set_plugin_config },
    //{ "get_server_info", &weechat_lua_api_get_server_info },
    //{ "get_channel_info", &weechat_lua_api_get_channel_info },
    //{ "get_nick_info", &weechat_lua_api_get_nick_info },
    //{ "get_irc_color", &weechat_lua_api_get_irc_color },
    //{ "get_window_info", &weechat_lua_api_get_window_info },
    //{ "get_buffer_info", &weechat_lua_api_get_buffer_info },
    //{ "get_buffer_data", &weechat_lua_api_get_buffer_data },
    /* define constants as function which returns values */
    { "WEECHAT_RC_OK", &weechat_lua_api_constant_weechat_rc_ok },
    { "WEECHAT_RC_ERROR", &weechat_lua_api_constant_weechat_rc_error },
    { "WEECHAT_RC_OK_IGNORE_WEECHAT", &weechat_lua_api_constant_weechat_rc_ok_ignore_weechat },
    { "WEECHAT_RC_OK_IGNORE_PLUGINS", &weechat_lua_api_constant_weechat_rc_ok_ignore_plugins },
    { "WEECHAT_RC_OK_IGNORE_ALL", &weechat_lua_api_constant_weechat_rc_ok_ignore_all },
    { "WEECHAT_RC_OK_WITH_HIGHLIGHT", &weechat_lua_api_constant_weechat_rc_ok_with_highlight },
    { "WEECHAT_LIST_POS_SORT", &weechat_lua_api_constant_weechat_list_pos_sort },
    { "WEECHAT_LIST_POS_BEGINNING", &weechat_lua_api_constant_weechat_list_pos_beginning },
    { "WEECHAT_LIST_POS_END", &weechat_lua_api_constant_weechat_list_pos_end },
    { "WEECHAT_HOTLIST_LOW", &weechat_lua_api_constant_weechat_hotlist_low },
    { "WEECHAT_HOTLIST_MESSAGE", &weechat_lua_api_constant_weechat_hotlist_message },
    { "WEECHAT_HOTLIST_PRIVATE", &weechat_lua_api_constant_weechat_hotlist_private },
    { "WEECHAT_HOTLIST_HIGHLIGHT", &weechat_lua_api_constant_weechat_hotlist_highlight },
    { "WEECHAT_HOOK_SIGNAL_STRING", &weechat_lua_api_constant_weechat_hook_signal_string },
    { "WEECHAT_HOOK_SIGNAL_INT", &weechat_lua_api_constant_weechat_hook_signal_int },
    { "WEECHAT_HOOK_SIGNAL_POINTER", &weechat_lua_api_constant_weechat_hook_signal_pointer },
    { NULL, NULL }
};
