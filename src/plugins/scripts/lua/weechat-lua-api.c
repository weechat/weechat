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
#include <time.h>

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
    
    if (script_search (weechat_lua_plugin, lua_scripts, name))
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
                                     name,
                                     author,
                                     version,
                                     license,
                                     description,
                                     shutdown_func,
                                     charset);
    if (lua_current_script)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s: registered script \"%s\", "
                                         "version %s (%s)"),
                        "lua", name, version, description);
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
                            charset);

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
    
    result = weechat_iconv_to_internal (charset, string);
    
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
    
    result = weechat_iconv_from_internal (charset, string);
    
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
    
    result = weechat_gettext (string);
    
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
    
    result = weechat_ngettext (single, plural, count);
    
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
    
    if (weechat_mkdir_home (directory, mode))
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
    
    if (weechat_mkdir (directory, mode))
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
    
    result = script_ptr2str (weechat_list_add (script_str2ptr (weelist),
                                               data,
                                               where));
    
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
    
    result = script_ptr2str (weechat_list_search (script_str2ptr (weelist),
                                                  data));
    
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
    
    result = script_ptr2str (weechat_list_casesearch (script_str2ptr (weelist),
                                                      data));
    
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
    
    result = script_ptr2str (weechat_list_get (script_str2ptr (weelist),
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
    
    weechat_list_set (script_str2ptr (item),
                      new_value);
    
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
    
    result = script_ptr2str (weechat_list_next (script_str2ptr (item)));
    
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
    
    result = script_ptr2str (weechat_list_prev (script_str2ptr (item)));
    
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
    
    result = weechat_list_string (script_str2ptr (item));
    
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
    
    size = weechat_list_size (script_str2ptr (weelist));
    
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
    
    weechat_list_remove (script_str2ptr (weelist),
                         script_str2ptr (item));
    
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
    
    weechat_list_remove_all (script_str2ptr (weelist));
    
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
    
    weechat_list_free (script_str2ptr (weelist));
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_config_reload_cb: callback for ccnfig reload
 */

int
weechat_lua_api_config_reload_cb (void *data,
                                  struct t_config_file *config_file)
{
    struct t_script_callback *script_callback;
    char *lua_argv[2];
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback->function && script_callback->function[0])
    {
        lua_argv[0] = script_ptr2str (config_file);
        lua_argv[1] = NULL;
        
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
    
    return 0;
}

/*
 * weechat_lua_api_config_new: create a new configuration file
 */

static int
weechat_lua_api_config_new (lua_State *L)
{
    const char *name, *function;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_new");
        LUA_RETURN_EMPTY;
    }
    
    name = NULL;
    function = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_new");
        LUA_RETURN_EMPTY;
    }
    
    name = lua_tostring (lua_current_interpreter, -2);
    function = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (script_api_config_new (weechat_lua_plugin,
                                                    lua_current_script,
                                                    name,
                                                    &weechat_lua_api_config_reload_cb,
                                                    function));
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_config_read_cb: callback for reading option in section
 */

void
weechat_lua_api_config_read_cb (void *data,
                                struct t_config_file *config_file,
                                const char *option_name, const char *value)
{
    struct t_script_callback *script_callback;
    char *lua_argv[4];
    int *rc;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback->function && script_callback->function[0])
    {
        lua_argv[0] = script_ptr2str (config_file);
        lua_argv[1] = (char *)option_name;
        lua_argv[2] = (char *)value;
        lua_argv[3] = NULL;
        
        rc = (int *) weechat_lua_exec (script_callback->script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       script_callback->function,
                                       lua_argv);
        
        if (rc)
            free (rc);
        if (lua_argv[0])
            free (lua_argv[0]);
    }
}

/*
 * weechat_lua_api_config_section_write_cb: callback for writing section
 */

void
weechat_lua_api_config_section_write_cb (void *data,
                                         struct t_config_file *config_file,
                                         const char *section_name)
{
    struct t_script_callback *script_callback;
    char *lua_argv[3];
    int *rc;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback->function && script_callback->function[0])
    {
        lua_argv[0] = script_ptr2str (config_file);
        lua_argv[1] = (char *)section_name;
        lua_argv[2] = NULL;
        
        rc = (int *) weechat_lua_exec (script_callback->script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       script_callback->function,
                                       lua_argv);
        
        if (rc)
            free (rc);
        if (lua_argv[0])
            free (lua_argv[0]);
    }
}

/*
 * weechat_lua_api_config_section_write_default_cb: callback for writing
 *                                                  default values for section
 */

void
weechat_lua_api_config_section_write_default_cb (void *data,
                                                 struct t_config_file *config_file,
                                                 const char *section_name)
{
    struct t_script_callback *script_callback;
    char *lua_argv[3];
    int *rc;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback->function && script_callback->function[0])
    {
        lua_argv[0] = script_ptr2str (config_file);
        lua_argv[1] = (char *)section_name;
        lua_argv[2] = NULL;
        
        rc = (int *) weechat_lua_exec (script_callback->script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       script_callback->function,
                                       lua_argv);
        
        if (rc)
            free (rc);
        if (lua_argv[0])
            free (lua_argv[0]);
    }
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
    struct t_script_callback *script_callback;
    char *lua_argv[5];
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback->function && script_callback->function[0])
    {
        lua_argv[0] = script_ptr2str (config_file);
        lua_argv[1] = script_ptr2str (section);
        lua_argv[2] = (char *)option_name;
        lua_argv[3] = (char *)value;
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
        if (lua_argv[0])
            free (lua_argv[0]);
        if (lua_argv[1])
            free (lua_argv[1]);
        
        return ret;
    }
    
    return 0;
}

/*
 * weechat_lua_api_config_new_section: create a new section in configuration file
 */

static int
weechat_lua_api_config_new_section (lua_State *L)
{
    const char *config_file, *name, *function_read, *function_write;
    const char *function_write_default, *function_create_option;
    char *result;
    int n, user_can_add_options, user_can_delete_options;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_new_section");
        LUA_RETURN_EMPTY;
    }
    
    config_file = NULL;
    name = NULL;
    user_can_add_options = 0;
    user_can_delete_options = 0;
    function_read = NULL;
    function_write = NULL;
    function_write_default = NULL;
    function_create_option = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 8)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_new_section");
        LUA_RETURN_EMPTY;
    }
    
    config_file = lua_tostring (lua_current_interpreter, -8);
    name = lua_tostring (lua_current_interpreter, -7);
    user_can_add_options = lua_tonumber (lua_current_interpreter, -6);
    user_can_delete_options = lua_tonumber (lua_current_interpreter, -5);
    function_read = lua_tostring (lua_current_interpreter, -4);
    function_write = lua_tostring (lua_current_interpreter, -3);
    function_write_default = lua_tostring (lua_current_interpreter, -2);
    function_create_option = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (script_api_config_new_section (weechat_lua_plugin,
                                                            lua_current_script,
                                                            script_str2ptr (config_file),
                                                            name,
                                                            user_can_add_options,
                                                            user_can_delete_options,
                                                            &weechat_lua_api_config_read_cb,
                                                            function_read,
                                                            &weechat_lua_api_config_section_write_cb,
                                                            function_write,
                                                            &weechat_lua_api_config_section_write_default_cb,
                                                            function_write_default,
                                                            &weechat_lua_api_config_section_create_option_cb,
                                                            function_create_option));
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_config_search_section: search a section in configuration file
 */

static int
weechat_lua_api_config_search_section (lua_State *L)
{
    const char *config_file, *section_name;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_search_section");
        LUA_RETURN_EMPTY;
    }
    
    config_file = NULL;
    section_name = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_search_section");
        LUA_RETURN_EMPTY;
    }
    
    config_file = lua_tostring (lua_current_interpreter, -2);
    section_name = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (weechat_config_search_section (script_str2ptr (config_file),
                                                            section_name));
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_config_option_change_cb: callback for option changed
 */

void
weechat_lua_api_config_option_change_cb (void *data)
{
    struct t_script_callback *script_callback;
    char *lua_argv[1];
    int *rc;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback->function && script_callback->function[0])
    {
        lua_argv[0] = NULL;
        
        rc = (int *) weechat_lua_exec (script_callback->script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       script_callback->function,
                                       lua_argv);
        
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
    const char *string_values, *default_value, *function;
    char *result;
    int n, min, max;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_new_option");
        LUA_RETURN_EMPTY;
    }
    
    config_file = NULL;
    section = NULL;
    name = NULL;
    type = NULL;
    description = NULL;
    string_values = NULL;
    min = 0;
    max = 0;
    default_value = NULL;
    function = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 10)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_new_option");
        LUA_RETURN_EMPTY;
    }
    
    config_file = lua_tostring (lua_current_interpreter, -10);
    section = lua_tostring (lua_current_interpreter, -9);
    name = lua_tostring (lua_current_interpreter, -8);
    type = lua_tostring (lua_current_interpreter, -7);
    description = lua_tostring (lua_current_interpreter, -6);
    string_values = lua_tostring (lua_current_interpreter, -5);
    min = lua_tonumber (lua_current_interpreter, -4);
    max = lua_tonumber (lua_current_interpreter, -3);
    default_value = lua_tostring (lua_current_interpreter, -2);
    function = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (script_api_config_new_option (weechat_lua_plugin,
                                                           lua_current_script,
                                                           script_str2ptr (config_file),
                                                           script_str2ptr (section),
                                                           name,
                                                           type,
                                                           description,
                                                           string_values,
                                                           min,
                                                           max,
                                                           default_value,
                                                           &weechat_lua_api_config_option_change_cb,
                                                           function));
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_config_search_option: search option in configuration file or section
 */

static int
weechat_lua_api_config_search_option (lua_State *L)
{
    const char *config_file, *section, *option_name;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_search_option");
        LUA_RETURN_EMPTY;
    }
    
    config_file = NULL;
    section = NULL;
    option_name = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_search_option");
        LUA_RETURN_EMPTY;
    }
    
    config_file = lua_tostring (lua_current_interpreter, -3);
    section = lua_tostring (lua_current_interpreter, -2);
    option_name = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (weechat_config_search_option (script_str2ptr (config_file),
                                                           script_str2ptr (section),
                                                           option_name));
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_config_string_to_boolean: return boolean value of a string
 */

static int
weechat_lua_api_config_string_to_boolean (lua_State *L)
{
    const char *text;
    int n, value;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_string_to_boolean");
        LUA_RETURN_INT(0);
    }
    
    text = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_string_to_boolean");
        LUA_RETURN_INT(0);
    }
    
    text = lua_tostring (lua_current_interpreter, -1);
    
    value = weechat_config_string_to_boolean (text);
    
    LUA_RETURN_INT(value);
}

/*
 * weechat_lua_api_config_option_reset: reset option with default value
 */

static int
weechat_lua_api_config_option_reset (lua_State *L)
{
    const char *option;
    int n, run_callback, rc;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_option_reset");
        LUA_RETURN_INT(0);
    }
    
    option = NULL;
    run_callback = 0;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_option_reset");
        LUA_RETURN_INT(0);
    }
    
    option = lua_tostring (lua_current_interpreter, -2);
    run_callback = lua_tonumber (lua_current_interpreter, -1);
    
    rc = weechat_config_option_reset (script_str2ptr (option),
                                      run_callback);
    
    LUA_RETURN_INT(rc);
}

/*
 * weechat_lua_api_config_option_set: set new value for option
 */

static int
weechat_lua_api_config_option_set (lua_State *L)
{
    const char *option, *new_value;
    int n, run_callback, rc;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_option_set");
        LUA_RETURN_INT(0);
    }
    
    option = NULL;
    new_value = NULL;
    run_callback = 0;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_option_set");
        LUA_RETURN_INT(0);
    }
    
    option = lua_tostring (lua_current_interpreter, -3);
    new_value = lua_tostring (lua_current_interpreter, -2);
    run_callback = lua_tonumber (lua_current_interpreter, -1);
    
    rc = weechat_config_option_set (script_str2ptr (option),
                                    new_value,
                                    run_callback);
    
    LUA_RETURN_INT(rc);
}

/*
 * weechat_lua_api_config_option_unset: unset an option
 */

static int
weechat_lua_api_config_option_unset (lua_State *L)
{
    const char *option;
    int n, rc;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_option_unset");
        LUA_RETURN_INT(0);
    }
    
    option = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_option_unset");
        LUA_RETURN_INT(0);
    }
    
    option = lua_tostring (lua_current_interpreter, -1);
    
    rc = weechat_config_option_unset (script_str2ptr (option));
    
    LUA_RETURN_INT(rc);
}

/*
 * weechat_lua_api_config_option_rename: rename an option
 */

static int
weechat_lua_api_config_option_rename (lua_State *L)
{
    const char *option, *new_name;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_option_rename");;
        LUA_RETURN_ERROR;
    }
    
    option = NULL;
    new_name = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_option_rename");
        LUA_RETURN_ERROR;
    }
    
    option = lua_tostring (lua_current_interpreter, -2);
    new_name = lua_tostring (lua_current_interpreter, -1);
    
    weechat_config_option_rename (script_str2ptr (option),
                                  new_name);
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_config_boolean: return boolean value of option
 */

static int
weechat_lua_api_config_boolean (lua_State *L)
{
    const char *option;
    int n, value;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_boolean");
        LUA_RETURN_INT(0);
    }
    
    option = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_boolean");
        LUA_RETURN_INT(0);
    }
    
    option = lua_tostring (lua_current_interpreter, -1);
    
    value = weechat_config_boolean (script_str2ptr (option));
    
    LUA_RETURN_INT(value);
}

/*
 * weechat_lua_api_config_integer: return integer value of option
 */

static int
weechat_lua_api_config_integer (lua_State *L)
{
    const char *option;
    int n, value;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_integer");
        LUA_RETURN_INT(0);
    }
    
    option = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_integer");
        LUA_RETURN_INT(0);
    }
    
    option = lua_tostring (lua_current_interpreter, -1);
    
    value = weechat_config_integer (script_str2ptr (option));
    
    LUA_RETURN_INT(value);
}

/*
 * weechat_lua_api_config_string: return string value of option
 */

static int
weechat_lua_api_config_string (lua_State *L)
{
    const char *option;
    char *value;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_string");
        LUA_RETURN_EMPTY;
    }
    
    option = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_string");
        LUA_RETURN_INT(0);
    }
    
    option = lua_tostring (lua_current_interpreter, -1);
    
    value = weechat_config_string (script_str2ptr (option));
    
    LUA_RETURN_STRING(value);
}

/*
 * weechat_lua_api_config_color: return color value of option
 */

static int
weechat_lua_api_config_color (lua_State *L)
{
    const char *option;
    int n, value;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_color");
        LUA_RETURN_INT(0);
    }
    
    option = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_color");
        LUA_RETURN_INT(0);
    }
    
    option = lua_tostring (lua_current_interpreter, -1);
    
    value = weechat_config_color (script_str2ptr (option));
    
    LUA_RETURN_INT(value);
}

/*
 * weechat_lua_api_config_write_line: write a line in configuration file
 */

static int
weechat_lua_api_config_write_line (lua_State *L)
{
    const char *config_file, *option_name, *value;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_write_line");
        LUA_RETURN_ERROR;
    }
    
    config_file = NULL;
    option_name = NULL;
    value = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_write_line");
        LUA_RETURN_ERROR;
    }
    
    config_file = lua_tostring (lua_current_interpreter, -3);
    option_name = lua_tostring (lua_current_interpreter, -2);
    value = lua_tostring (lua_current_interpreter, -1);
    
    weechat_config_write_line (script_str2ptr (config_file),
                               option_name,
                               "%s",
                               value);
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_config_write: write configuration file
 */

static int
weechat_lua_api_config_write (lua_State *L)
{
    const char *config_file;
    int n, rc;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_write");
        LUA_RETURN_INT(-1);
    }
    
    config_file = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_write");
        LUA_RETURN_INT(-1);
    }
    
    config_file = lua_tostring (lua_current_interpreter, -1);
    
    rc = weechat_config_write (script_str2ptr (config_file));
    
    LUA_RETURN_INT(rc);
}

/*
 * weechat_lua_api_config_read: read configuration file
 */

static int
weechat_lua_api_config_read (lua_State *L)
{
    const char *config_file;
    int n, rc;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_read");
        LUA_RETURN_INT(-1);
    }
    
    config_file = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_read");
        LUA_RETURN_INT(-1);
    }
    
    config_file = lua_tostring (lua_current_interpreter, -1);
    
    rc = weechat_config_read (script_str2ptr (config_file));
    
    LUA_RETURN_INT(rc);
}

/*
 * weechat_lua_api_config_reload: reload configuration file
 */

static int
weechat_lua_api_config_reload (lua_State *L)
{
    const char *config_file;
    int n, rc;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_reload");
        LUA_RETURN_INT(-1);
    }
    
    config_file = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_reload");
        LUA_RETURN_INT(-1);
    }
    
    config_file = lua_tostring (lua_current_interpreter, -1);
    
    rc = weechat_config_reload (script_str2ptr (config_file));
    
    LUA_RETURN_INT(rc);
}

/*
 * weechat_lua_api_config_free: free configuration file
 */

static int
weechat_lua_api_config_free (lua_State *L)
{
    const char *config_file;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_free");
        LUA_RETURN_ERROR;
    }
    
    config_file = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_free");
        LUA_RETURN_ERROR;
    }
    
    config_file = lua_tostring (lua_current_interpreter, -1);
    
    script_api_config_free (weechat_lua_plugin,
                            lua_current_script,
                            script_str2ptr (config_file));
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_config_get: get config option
 */

static int
weechat_lua_api_config_get (lua_State *L)
{
    const char *option;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_get");
        LUA_RETURN_EMPTY;
    }
    
    option = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_get");
        LUA_RETURN_EMPTY;
    }
    
    option = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (weechat_config_get (option));
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_config_get_plugin: get value of a plugin option
 */

static int
weechat_lua_api_config_get_plugin (lua_State *L)
{
    const char *option;
    char *value;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_get_plugin");
        LUA_RETURN_EMPTY;
    }
    
    option = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_get_plugin");
        LUA_RETURN_EMPTY;
    }
    
    option = lua_tostring (lua_current_interpreter, -1);
    
    value = script_api_config_get_plugin (weechat_lua_plugin,
                                          lua_current_script,
                                          option);
    
    LUA_RETURN_STRING(value);
}

/*
 * weechat_lua_api_config_set_plugin: set value of a plugin option
 */

static int
weechat_lua_api_config_set_plugin (lua_State *L)
{
    const char *option, *value;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_set_plugin");
        LUA_RETURN_ERROR;
    }
    
    option = NULL;
    value = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_set_plugin");
        LUA_RETURN_ERROR;
    }
    
    option = lua_tostring (lua_current_interpreter, -2);
    value = lua_tostring (lua_current_interpreter, -1);
    
    if (script_api_config_set_plugin (weechat_lua_plugin,
                                      lua_current_script,
                                      option,
                                      value))
        LUA_RETURN_OK;
    
    LUA_RETURN_ERROR;
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
    
    result = weechat_prefix (prefix);
    
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
    
    result = weechat_prefix (color);
    
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
                       script_str2ptr (buffer),
                       "%s", message);
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_print_date_tags: print message in a buffer with optional
 *                                  date and tags
 */

static int
weechat_lua_api_print_date_tags (lua_State *L)
{
    const char *buffer, *tags, *message;
    int n, date;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("print_date_tags");
        LUA_RETURN_ERROR;
    }
    
    buffer = NULL;
    date = 0;
    tags = NULL;
    message = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 4)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("print_date_tags");
        LUA_RETURN_ERROR;
    }
    
    buffer = lua_tostring (lua_current_interpreter, -4);
    date = lua_tonumber (lua_current_interpreter, -3);
    tags = lua_tostring (lua_current_interpreter, -2);
    message = lua_tostring (lua_current_interpreter, -1);
    
    script_api_printf_date_tags (weechat_lua_plugin,
                                 lua_current_script,
                                 script_str2ptr (buffer),
                                 date,
                                 tags,
                                 "%s", message);
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_print_y: print message in a buffer with free content
 */

static int
weechat_lua_api_print_y (lua_State *L)
{
    const char *buffer, *message;
    int n, y;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("print_y");
        LUA_RETURN_ERROR;
    }
    
    buffer = NULL;
    y = 0;
    message = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("print_y");
        LUA_RETURN_ERROR;
    }
    
    buffer = lua_tostring (lua_current_interpreter, -3);
    y = lua_tonumber (lua_current_interpreter, -2);
    message = lua_tostring (lua_current_interpreter, -1);
    
    script_api_printf_y (weechat_lua_plugin,
                         lua_current_script,
                         script_str2ptr (buffer),
                         y,
                         "%s", message);
    
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
                           "%s", message);

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
                                                      command,
                                                      description,
                                                      args,
                                                      args_description,
                                                      completion,
                                                      &weechat_lua_api_hook_command_cb,
                                                      function));
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_hook_timer_cb: callback for timer hooked
 */

int
weechat_lua_api_hook_timer_cb (void *data)
{
    struct t_script_callback *script_callback;
    char *lua_argv[1];
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    lua_argv[0] = NULL;
    
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
                                                    function));
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_hook_fd_cb: callback for fd hooked
 */

int
weechat_lua_api_hook_fd_cb (void *data)
{
    struct t_script_callback *script_callback;
    char *lua_argv[1];
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    lua_argv[0] = NULL;
    
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
                                                 function));
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_hook_print_cb: callback for print hooked
 */

int
weechat_lua_api_hook_print_cb (void *data, struct t_gui_buffer *buffer,
                               time_t date, int tags_count, char **tags,
                               const char *prefix, const char *message)
{
    struct t_script_callback *script_callback;
    char *lua_argv[6];
    static char timebuffer[64];
    int *rc, ret;
    
    /* make C compiler happy */
    (void) tags_count;
    
    script_callback = (struct t_script_callback *)data;

    snprintf (timebuffer, sizeof (timebuffer) - 1, "%ld", (long int)date);
    
    lua_argv[0] = script_ptr2str (buffer);
    lua_argv[1] = timebuffer;
    lua_argv[2] = weechat_string_build_with_exploded (tags, ",");
    lua_argv[3] = (char *)prefix;
    lua_argv[4] = (char *)message;
    lua_argv[5] = NULL;
    
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
    if (lua_argv[2])
        free (lua_argv[2]);
    
    return ret;
}

/*
 * weechat_lua_api_hook_print: hook a print
 */

static int
weechat_lua_api_hook_print (lua_State *L)
{
    const char *buffer, *tags, *message, *function;
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
    tags = NULL;
    message = NULL;
    strip_colors = 0;
    function = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 4)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_print");
        LUA_RETURN_EMPTY;
    }
    
    buffer = lua_tostring (lua_current_interpreter, -5);
    tags = lua_tostring (lua_current_interpreter, -4);
    message = lua_tostring (lua_current_interpreter, -3);
    strip_colors = lua_tonumber (lua_current_interpreter, -2);
    function = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (script_api_hook_print (weechat_lua_plugin,
                                                    lua_current_script,
                                                    script_str2ptr (buffer),
                                                    tags,
                                                    message,
                                                    strip_colors,
                                                    &weechat_lua_api_hook_print_cb,
                                                    function));
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_hook_signal_cb: callback for signal hooked
 */

int
weechat_lua_api_hook_signal_cb (void *data, const char *signal,
                                const char *type_data, void *signal_data)
{
    struct t_script_callback *script_callback;
    char *lua_argv[3];
    static char value_str[64], empty_value[1] = { '\0' };
    int *rc, ret, free_needed;
    
    script_callback = (struct t_script_callback *)data;

    lua_argv[0] = (char *)signal;
    free_needed = 0;
    if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_STRING) == 0)
    {
        lua_argv[1] = (signal_data) ? (char *)signal_data : empty_value;
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
                                                     signal,
                                                     &weechat_lua_api_hook_signal_cb,
                                                     function));
    
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
        weechat_hook_signal_send (signal, type_data, (void *)signal_data);
        LUA_RETURN_OK;
    }
    else if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_INT) == 0)
    {
        number = lua_tonumber (lua_current_interpreter, -1);
        weechat_hook_signal_send (signal, type_data, &number);
        LUA_RETURN_OK;
    }
    else if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_POINTER) == 0)
    {
        signal_data = lua_tostring (lua_current_interpreter, -1);
        weechat_hook_signal_send (signal, type_data,
                                  script_str2ptr (signal_data));
        LUA_RETURN_OK;
    }
    
    LUA_RETURN_ERROR;
}

/*
 * weechat_lua_api_hook_config_cb: callback for config option hooked
 */

int
weechat_lua_api_hook_config_cb (void *data, const char *option,
                                const char *value)
{
    struct t_script_callback *script_callback;
    char *lua_argv[3];
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    lua_argv[0] = (char *)option;
    lua_argv[1] = (char *)value;
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
    
    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_config");
        LUA_RETURN_EMPTY;
    }
    
    option = lua_tostring (lua_current_interpreter, -2);
    function = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (script_api_hook_config (weechat_lua_plugin,
                                                     lua_current_script,
                                                     option,
                                                     &weechat_lua_api_hook_config_cb,
                                                     function));
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_hook_completion_cb: callback for completion hooked
 */

int
weechat_lua_api_hook_completion_cb (void *data, const char *completion_item,
                                    struct t_gui_buffer *buffer,
                                    struct t_gui_completion *completion)
{
    struct t_script_callback *script_callback;
    char *lua_argv[4];
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    lua_argv[0] = (char *)completion_item;
    lua_argv[1] = script_ptr2str (buffer);
    lua_argv[2] = script_ptr2str (completion);
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
                                                         completion,
                                                         &weechat_lua_api_hook_completion_cb,
                                                         function));
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_hook_completion_list_add: add a word to list for a completion
 */

static int
weechat_lua_api_hook_completion_list_add (lua_State *L)
{
    const char *completion, *word, *where;
    int n, nick_completion;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_completion_list_add");
        LUA_RETURN_ERROR;
    }
    
    completion = NULL;
    word = NULL;
    nick_completion = 0;
    where = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 4)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_completion_list_add");
        LUA_RETURN_ERROR;
    }
    
    completion = lua_tostring (lua_current_interpreter, -4);
    word = lua_tostring (lua_current_interpreter, -3);
    nick_completion = lua_tonumber (lua_current_interpreter, -2);
    where = lua_tostring (lua_current_interpreter, -1);
    
    weechat_hook_completion_list_add (script_str2ptr (completion),
                                      word,
                                      nick_completion,
                                      where);
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_hook_modifier_cb: callback for modifier hooked
 */

char *
weechat_lua_api_hook_modifier_cb (void *data, const char *modifier,
                                  const char *modifier_data,
                                  const char *string)
{
    struct t_script_callback *script_callback;
    char *lua_argv[4];
    
    script_callback = (struct t_script_callback *)data;
    
    lua_argv[0] = (char *)modifier;
    lua_argv[1] = (char *)modifier_data;
    lua_argv[2] = (char *)string;
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
                                                       modifier,
                                                       &weechat_lua_api_hook_modifier_cb,
                                                       function));
    
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
    
    result = weechat_hook_modifier_exec (modifier, modifier_data, string);
    
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
    
    script_api_unhook (weechat_lua_plugin,
                       lua_current_script,
                       script_str2ptr (hook));
    
    LUA_RETURN_OK;
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
    
    script_api_unhook_all (lua_current_script);
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_buffer_input_data_cb: callback for input data in a buffer
 */

int
weechat_lua_api_buffer_input_data_cb (void *data, struct t_gui_buffer *buffer,
                                      const char *input_data)
{
    struct t_script_callback *script_callback;
    char *lua_argv[3];
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    lua_argv[0] = script_ptr2str (buffer);
    lua_argv[1] = (char *)input_data;
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
 * weechat_lua_api_buffer_close_cb: callback for buffer closed
 */

int
weechat_lua_api_buffer_close_cb (void *data, struct t_gui_buffer *buffer)
{
    struct t_script_callback *script_callback;
    char *lua_argv[2];
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    lua_argv[0] = script_ptr2str (buffer);
    lua_argv[1] = NULL;
    
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
    const char *category, *name, *function_input, *function_close;
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
    function_input = NULL;
    function_close = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 4)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("buffer_new");
        LUA_RETURN_EMPTY;
    }
    
    category = lua_tostring (lua_current_interpreter, -4);
    name = lua_tostring (lua_current_interpreter, -3);
    function_input = lua_tostring (lua_current_interpreter, -2);
    function_close = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (script_api_buffer_new (weechat_lua_plugin,
                                                    lua_current_script,
                                                    category,
                                                    name,
                                                    &weechat_lua_api_buffer_input_data_cb,
                                                    function_input,
                                                    &weechat_lua_api_buffer_close_cb,
                                                    function_close));
    
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
    
    result = script_ptr2str (weechat_buffer_search (category,
                                                    name));
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_buffer_clear: clear a buffer
 */

static int
weechat_lua_api_buffer_clear (lua_State *L)
{
    const char *buffer;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("buffer_clear");
        LUA_RETURN_ERROR;
    }
    
    buffer = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("buffer_clear");
        LUA_RETURN_ERROR;
    }
    
    buffer = lua_tostring (lua_current_interpreter, -1);
    
    weechat_buffer_clear (script_str2ptr (buffer));
    
    LUA_RETURN_OK;
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
                             script_str2ptr (buffer),
                             switch_to_another);
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_buffer_get_integer: get a buffer property as integer
 */

static int
weechat_lua_api_buffer_get_integer (lua_State *L)
{
    const char *buffer, *property;
    int n, value;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("buffer_get_integer");
        LUA_RETURN_INT(-1);
    }
    
    buffer = NULL;
    property = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("buffer_get_integer");
        LUA_RETURN_INT(-1);
    }
    
    buffer = lua_tostring (lua_current_interpreter, -2);
    property = lua_tostring (lua_current_interpreter, -1);
    
    value = weechat_buffer_get_integer (script_str2ptr (buffer),
                                        property);
    
    LUA_RETURN_INT(value);
}

/*
 * weechat_lua_api_buffer_get_string: get a buffer property as string
 */

static int
weechat_lua_api_buffer_get_string (lua_State *L)
{
    const char *buffer, *property;
    char *value;
    int n;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("buffer_get_string");
        LUA_RETURN_EMPTY;
    }
    
    buffer = NULL;
    property = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("buffer_get_string");
        LUA_RETURN_EMPTY;
    }
    
    buffer = lua_tostring (lua_current_interpreter, -2);
    property = lua_tostring (lua_current_interpreter, -1);
    
    value = weechat_buffer_get_string (script_str2ptr (buffer),
                                       property);
    
    LUA_RETURN_STRING(value);
}

/*
 * weechat_lua_api_buffer_get_pointer: get a buffer property as pointer
 */

static int
weechat_lua_api_buffer_get_pointer (lua_State *L)
{
    const char *buffer, *property;
    char *value;
    int n;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("buffer_get_pointer");
        LUA_RETURN_EMPTY;
    }
    
    buffer = NULL;
    property = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("buffer_get_pointer");
        LUA_RETURN_EMPTY;
    }
    
    buffer = lua_tostring (lua_current_interpreter, -2);
    property = lua_tostring (lua_current_interpreter, -1);
    
    value = script_ptr2str (weechat_buffer_get_string (script_str2ptr (buffer),
                                                       property));
    
    LUA_RETURN_STRING_FREE(value);
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
    
    weechat_buffer_set (script_str2ptr (buffer), property, (void *)value);
    
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
    
    result = script_ptr2str (weechat_nicklist_add_group (script_str2ptr (buffer),
                                                         script_str2ptr (parent_group),
                                                         name,
                                                         color,
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
    
    result = script_ptr2str (weechat_nicklist_search_group (script_str2ptr (buffer),
                                                            script_str2ptr (from_group),
                                                            name));
    
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
    
    result = script_ptr2str (weechat_nicklist_add_nick (script_str2ptr (buffer),
                                                        script_str2ptr (group),
                                                        name,
                                                        color,
                                                        char_prefix,
                                                        prefix_color,
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
    
    result = script_ptr2str (weechat_nicklist_search_nick (script_str2ptr (buffer),
                                                           script_str2ptr (from_group),
                                                           name));
    
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
    
    weechat_nicklist_remove_group (script_str2ptr (buffer),
                                   script_str2ptr (group));
    
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
    
    weechat_nicklist_remove_nick (script_str2ptr (buffer),
                                  script_str2ptr (nick));
    
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
    
    weechat_nicklist_remove_all (script_str2ptr (buffer));
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_bar_item_search: search a bar item
 */

static int
weechat_lua_api_bar_item_search (lua_State *L)
{
    const char *name;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("bar_item_search");
        LUA_RETURN_EMPTY;
    }
    
    name = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("bar_item_search");
        LUA_RETURN_EMPTY;
    }
    
    name = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (weechat_bar_item_search (name));
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_bar_item_build_cb: callback for building bar item
 */

char *
weechat_lua_api_bar_item_build_cb (void *data, struct t_gui_bar_item *item,
                                   struct t_gui_window *window,
                                   int max_width, int max_height)
{
    struct t_script_callback *script_callback;
    char *lua_argv[5], *ret;
    static char str_width[32], str_height[32];
    
    script_callback = (struct t_script_callback *)data;

    snprintf (str_width, sizeof (str_width), "%d", max_width);
    snprintf (str_height, sizeof (str_height), "%d", max_height);
    
    lua_argv[0] = script_ptr2str (item);
    lua_argv[1] = script_ptr2str (window);
    lua_argv[2] = str_width;
    lua_argv[3] = str_height;
    lua_argv[4] = NULL;
    
    ret = (char *)weechat_lua_exec (script_callback->script,
                                    WEECHAT_SCRIPT_EXEC_STRING,
                                    script_callback->function,
                                    lua_argv);
    
    if (lua_argv[0])
        free (lua_argv[0]);
    if (lua_argv[1])
        free (lua_argv[1]);
    
    return ret;
}

/*
 * weechat_lua_api_bar_item_new: add a new bar item
 */

static int
weechat_lua_api_bar_item_new (lua_State *L)
{
    const char *name, *function;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("bar_item_new");
        LUA_RETURN_EMPTY;
    }
    
    name = NULL;
    function = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("bar_item_new");
        LUA_RETURN_EMPTY;
    }
    
    name = lua_tostring (lua_current_interpreter, -2);
    function = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (script_api_bar_item_new (weechat_lua_plugin,
                                                      lua_current_script,
                                                      name,
                                                      &weechat_lua_api_bar_item_build_cb,
                                                      function));
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_bar_item_update: update a bar item on screen
 */

static int
weechat_lua_api_bar_item_update (lua_State *L)
{
    const char *name;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("bar_item_update");
        LUA_RETURN_ERROR;
    }
    
    name = NULL;
    
    n = lua_gettop (lua_current_interpreter);

    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("bar_item_update");
        LUA_RETURN_ERROR;
    }
    
    name = lua_tostring (lua_current_interpreter, -1);
    
    weechat_bar_item_update (name);
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_bar_item_remove: remove a bar item
 */

static int
weechat_lua_api_bar_item_remove (lua_State *L)
{
    const char *item;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("bar_item_remove");
        LUA_RETURN_ERROR;
    }
    
    item = NULL;
    
    n = lua_gettop (lua_current_interpreter);

    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("bar_item_remove");
        LUA_RETURN_ERROR;
    }
    
    item = lua_tostring (lua_current_interpreter, -1);
    
    script_api_bar_item_remove (weechat_lua_plugin,
                                lua_current_script,
                                script_str2ptr (item));
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_bar_search: search a bar
 */

static int
weechat_lua_api_bar_search (lua_State *L)
{
    const char *name;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("bar_search");
        LUA_RETURN_EMPTY;
    }
    
    name = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("bar_search");
        LUA_RETURN_EMPTY;
    }
    
    name = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (weechat_bar_search (name));
    
    LUA_RETURN_STRING_FREE(result);
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
    int n;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("bar_new");
        LUA_RETURN_EMPTY;
    }
    
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
    separator = NULL;
    items = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 15)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("bar_new");
        LUA_RETURN_EMPTY;
    }
    
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
    
    result = script_ptr2str (weechat_bar_new (name,
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
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_bar_set: set a bar property
 */

static int
weechat_lua_api_bar_set (lua_State *L)
{
    const char *bar, *property, *value;
    int n;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("bar_set");
        LUA_RETURN_ERROR;
    }
    
    bar = NULL;
    property = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("bar_set");
        LUA_RETURN_ERROR;
    }
    
    bar = lua_tostring (lua_current_interpreter, -3);
    property = lua_tostring (lua_current_interpreter, -2);
    value = lua_tostring (lua_current_interpreter, -1);
    
    weechat_buffer_set (script_str2ptr (bar),
                        property,
                        (void *)value);
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_bar_update: update a bar on screen
 */

static int
weechat_lua_api_bar_update (lua_State *L)
{
    const char *name;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("bar_update");
        LUA_RETURN_ERROR;
    }
    
    name = NULL;
    
    n = lua_gettop (lua_current_interpreter);

    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("bar_update");
        LUA_RETURN_ERROR;
    }
    
    name = lua_tostring (lua_current_interpreter, -1);
    
    weechat_bar_update (name);
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_bar_remove: remove a bar
 */

static int
weechat_lua_api_bar_remove (lua_State *L)
{
    const char *bar;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("bar_remove");
        LUA_RETURN_ERROR;
    }
    
    bar = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("bar_remove");
        LUA_RETURN_ERROR;
    }
    
    bar = lua_tostring (lua_current_interpreter, -1);
    
    weechat_bar_remove (script_str2ptr (bar));
    
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
    
    buffer = NULL;
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
                        script_str2ptr (buffer),
                        command);

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
    
    value = weechat_info_get (info);
    
    LUA_RETURN_STRING(value);
}

/*
 * weechat_lua_api_infolist_get: get list with infos
 */

static int
weechat_lua_api_infolist_get (lua_State *L)
{
    const char *name, *pointer, *arguments;
    char *value;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("infolist_get");
        LUA_RETURN_EMPTY;
    }
    
    name = NULL;
    pointer = NULL;
    arguments = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("infolist_get");
        LUA_RETURN_EMPTY;
    }
    
    name = lua_tostring (lua_current_interpreter, -3);
    pointer = lua_tostring (lua_current_interpreter, -2);
    arguments = lua_tostring (lua_current_interpreter, -1);
    
    value = script_ptr2str (weechat_infolist_get (name,
                                                  script_str2ptr (pointer),
                                                  arguments));
    
    LUA_RETURN_STRING_FREE(value);
}

/*
 * weechat_lua_api_infolist_next: move item pointer to next item in infolist
 */

static int
weechat_lua_api_infolist_next (lua_State *L)
{
    const char *infolist;
    int n, value;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("infolist_next");
        LUA_RETURN_INT(0);
    }
    
    infolist = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("infolist_next");
        LUA_RETURN_INT(0);
    }
    
    infolist = lua_tostring (lua_current_interpreter, -1);
    
    value = weechat_infolist_next (script_str2ptr (infolist));
    
    LUA_RETURN_INT(value);
}

/*
 * weechat_lua_api_infolist_prev: move item pointer to previous item in infolist
 */

static int
weechat_lua_api_infolist_prev (lua_State *L)
{
    const char *infolist;
    int n, value;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("infolist_prev");
        LUA_RETURN_INT(0);
    }
    
    infolist = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("infolist_prev");
        LUA_RETURN_INT(0);
    }
    
    infolist = lua_tostring (lua_current_interpreter, -1);
    
    value = weechat_infolist_prev (script_str2ptr (infolist));
    
    LUA_RETURN_INT(value);
}

/*
 * weechat_lua_api_infolist_fields: get list of fields for current item of infolist
 */

static int
weechat_lua_api_infolist_fields (lua_State *L)
{
    const char *infolist;
    char *value;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("infolist_fields");
        LUA_RETURN_EMPTY;
    }
    
    infolist = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("infolist_fields");
        LUA_RETURN_EMPTY;
    }
    
    infolist = lua_tostring (lua_current_interpreter, -1);
    
    value = weechat_infolist_fields (script_str2ptr (infolist));
    
    LUA_RETURN_STRING(value);
}

/*
 * weechat_lua_api_infolist_integer: get integer value of a variable in infolist
 */

static int
weechat_lua_api_infolist_integer (lua_State *L)
{
    const char *infolist, *variable;
    int n, value;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("infolist_integer");
        LUA_RETURN_INT(0);
    }
    
    infolist = NULL;
    variable = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("infolist_integer");
        LUA_RETURN_INT(0);
    }
    
    infolist = lua_tostring (lua_current_interpreter, -2);
    variable = lua_tostring (lua_current_interpreter, -1);
    
    value = weechat_infolist_integer (script_str2ptr (infolist),
                                      variable);
    
    LUA_RETURN_INT(value);
}

/*
 * weechat_lua_api_infolist_string: get string value of a variable in infolist
 */

static int
weechat_lua_api_infolist_string (lua_State *L)
{
    const char *infolist, *variable;
    char *value;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("infolist_string");
        LUA_RETURN_EMPTY;
    }
    
    infolist = NULL;
    variable = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("infolist_string");
        LUA_RETURN_EMPTY;
    }
    
    infolist = lua_tostring (lua_current_interpreter, -2);
    variable = lua_tostring (lua_current_interpreter, -1);
    
    value = weechat_infolist_string (script_str2ptr (infolist),
                                     variable);
    
    LUA_RETURN_STRING(value);
}

/*
 * weechat_lua_api_infolist_pointer: get pointer value of a variable in infolist
 */

static int
weechat_lua_api_infolist_pointer (lua_State *L)
{
    const char *infolist, *variable;
    char *value;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("infolist_pointer");
        LUA_RETURN_EMPTY;
    }
    
    infolist = NULL;
    variable = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("infolist_pointer");
        LUA_RETURN_EMPTY;
    }
    
    infolist = lua_tostring (lua_current_interpreter, -2);
    variable = lua_tostring (lua_current_interpreter, -1);
    
    value = script_ptr2str (weechat_infolist_pointer (script_str2ptr (infolist),
                                                      variable));
    
    LUA_RETURN_STRING_FREE(value);
}

/*
 * weechat_lua_api_infolist_time: get time value of a variable in infolist
 */

static int
weechat_lua_api_infolist_time (lua_State *L)
{
    const char *infolist, *variable;
    time_t time;
    char timebuffer[64], *value;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("infolist_time");
        LUA_RETURN_EMPTY;
    }
    
    infolist = NULL;
    variable = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("infolist_time");
        LUA_RETURN_EMPTY;
    }
    
    infolist = lua_tostring (lua_current_interpreter, -2);
    variable = lua_tostring (lua_current_interpreter, -1);
    
    time = weechat_infolist_time (script_str2ptr (infolist),
                                  variable);
    strftime (timebuffer, sizeof (timebuffer), "%F %T", localtime (&time));
    value = strdup (timebuffer);
    
    LUA_RETURN_STRING_FREE(value);
}

/*
 * weechat_lua_api_infolist_free: free infolist
 */

static int
weechat_lua_api_infolist_free (lua_State *L)
{
    const char *infolist;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("infolist_free");
        LUA_RETURN_ERROR;
    }
    
    infolist = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("infolist_free");
        LUA_RETURN_ERROR;
    }
    
    infolist = lua_tostring (lua_current_interpreter, -1);
    
    weechat_infolist_free (script_str2ptr (infolist));
    
    LUA_RETURN_OK;
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
    { "config_new", &weechat_lua_api_config_new },
    { "config_new_section", &weechat_lua_api_config_new_section },
    { "config_search_section", &weechat_lua_api_config_search_section },
    { "config_new_option", &weechat_lua_api_config_new_option },
    { "config_search_option", &weechat_lua_api_config_search_option },
    { "config_string_to_boolean", &weechat_lua_api_config_string_to_boolean },
    { "config_option_reset", &weechat_lua_api_config_option_reset },
    { "config_option_set", &weechat_lua_api_config_option_set },
    { "config_option_unset", &weechat_lua_api_config_option_unset },
    { "config_option_rename", &weechat_lua_api_config_option_rename },
    { "config_boolean", &weechat_lua_api_config_boolean },
    { "config_integer", &weechat_lua_api_config_integer },
    { "config_string", &weechat_lua_api_config_string },
    { "config_color", &weechat_lua_api_config_color },
    { "config_write_line", &weechat_lua_api_config_write_line },
    { "config_write", &weechat_lua_api_config_write },
    { "config_read", &weechat_lua_api_config_read },
    { "config_reload", &weechat_lua_api_config_reload },
    { "config_free", &weechat_lua_api_config_free },
    { "config_get", &weechat_lua_api_config_get },
    { "config_get_plugin", &weechat_lua_api_config_get_plugin },
    { "config_set_plugin", &weechat_lua_api_config_set_plugin },
    { "prefix", &weechat_lua_api_prefix },
    { "color", &weechat_lua_api_color },
    { "print", &weechat_lua_api_print },
    { "print_date_tags", &weechat_lua_api_print_date_tags },
    { "print_y", &weechat_lua_api_print_y },
    { "log_print", &weechat_lua_api_log_print },
    { "hook_command", &weechat_lua_api_hook_command },
    { "hook_timer", &weechat_lua_api_hook_timer },
    { "hook_fd", &weechat_lua_api_hook_fd },
    { "hook_print", &weechat_lua_api_hook_print },
    { "hook_signal", &weechat_lua_api_hook_signal },
    { "hook_signal_send", &weechat_lua_api_hook_signal_send },
    { "hook_config", &weechat_lua_api_hook_config },
    { "hook_completion", &weechat_lua_api_hook_completion },
    { "hook_completion_list_add", &weechat_lua_api_hook_completion_list_add },
    { "hook_modifier", &weechat_lua_api_hook_modifier },
    { "hook_modifier_exec", &weechat_lua_api_hook_modifier_exec },
    { "unhook", &weechat_lua_api_unhook },
    { "unhook_all", &weechat_lua_api_unhook_all },
    { "buffer_new", &weechat_lua_api_buffer_new },
    { "buffer_search", &weechat_lua_api_buffer_search },
    { "buffer_clear", &weechat_lua_api_buffer_clear },
    { "buffer_close", &weechat_lua_api_buffer_close },
    { "buffer_get_integer", &weechat_lua_api_buffer_get_integer },
    { "buffer_get_string", &weechat_lua_api_buffer_get_string },
    { "buffer_get_pointer", &weechat_lua_api_buffer_get_pointer },
    { "buffer_set", &weechat_lua_api_buffer_set },
    { "nicklist_add_group", &weechat_lua_api_nicklist_add_group },
    { "nicklist_search_group", &weechat_lua_api_nicklist_search_group },
    { "nicklist_add_nick", &weechat_lua_api_nicklist_add_nick },
    { "nicklist_search_nick", &weechat_lua_api_nicklist_search_nick },
    { "nicklist_remove_group", &weechat_lua_api_nicklist_remove_group },
    { "nicklist_remove_nick", &weechat_lua_api_nicklist_remove_nick },
    { "nicklist_remove_all", &weechat_lua_api_nicklist_remove_all },
    { "bar_item_search", &weechat_lua_api_bar_item_search },
    { "bar_item_new", &weechat_lua_api_bar_item_new },
    { "bar_item_update", &weechat_lua_api_bar_item_update },
    { "bar_item_remove", &weechat_lua_api_bar_item_remove },
    { "bar_search", &weechat_lua_api_bar_search },
    { "bar_new", &weechat_lua_api_bar_new },
    { "bar_set", &weechat_lua_api_bar_set },
    { "bar_update", &weechat_lua_api_bar_update },
    { "bar_remove", &weechat_lua_api_bar_remove },
    { "command", &weechat_lua_api_command },
    { "info_get", &weechat_lua_api_info_get },
    { "infolist_get", &weechat_lua_api_infolist_get },
    { "infolist_next", &weechat_lua_api_infolist_next },
    { "infolist_prev", &weechat_lua_api_infolist_prev },
    { "infolist_fields", &weechat_lua_api_infolist_fields },
    { "infolist_integer", &weechat_lua_api_infolist_integer },
    { "infolist_string", &weechat_lua_api_infolist_string },
    { "infolist_pointer", &weechat_lua_api_infolist_pointer },
    { "infolist_time", &weechat_lua_api_infolist_time },
    { "infolist_free", &weechat_lua_api_infolist_free },
    
    /* define constants as function which returns values */
    
    { "WEECHAT_RC_OK", &weechat_lua_api_constant_weechat_rc_ok },
    { "WEECHAT_RC_ERROR", &weechat_lua_api_constant_weechat_rc_error },
    
    { "WEECHAT_CONFIG_READ_OK", &weechat_lua_api_constant_weechat_config_read_ok },
    { "WEECHAT_CONFIG_READ_MEMORY_ERROR", &weechat_lua_api_constant_weechat_config_read_memory_error },
    { "WEECHAT_CONFIG_READ_FILE_NOT_FOUND", &weechat_lua_api_constant_weechat_config_read_file_not_found },
    { "WEECHAT_CONFIG_WRITE_OK", &weechat_lua_api_constant_weechat_config_write_ok },
    { "WEECHAT_CONFIG_WRITE_ERROR", &weechat_lua_api_constant_weechat_config_write_error },
    { "WEECHAT_CONFIG_WRITE_MEMORY_ERROR", &weechat_lua_api_constant_weechat_config_write_memory_error },
    { "WEECHAT_CONFIG_OPTION_SET_OK_CHANGED", &weechat_lua_api_constant_weechat_config_option_set_ok_changed },
    { "WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE", &weechat_lua_api_constant_weechat_config_option_set_ok_same_value },
    { "WEECHAT_CONFIG_OPTION_SER_ERROR", &weechat_lua_api_constant_weechat_config_option_set_error },
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
    
    { "WEECHAT_HOOK_SIGNAL_STRING", &weechat_lua_api_constant_weechat_hook_signal_string },
    { "WEECHAT_HOOK_SIGNAL_INT", &weechat_lua_api_constant_weechat_hook_signal_int },
    { "WEECHAT_HOOK_SIGNAL_POINTER", &weechat_lua_api_constant_weechat_hook_signal_pointer },
    
    { NULL, NULL }
};
