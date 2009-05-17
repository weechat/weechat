/*
 * Copyright (c) 2003-2009 by FlashCode <flashcode@flashtux.org>
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

/* weechat-ruby-api.c: Ruby API functions */


#undef _

#include <ruby.h>
#include <time.h>

#include "../../weechat-plugin.h"
#include "../script.h"
#include "../script-api.h"
#include "../script-callback.h"
#include "weechat-ruby.h"

#define RUBY_RETURN_OK return INT2FIX (1);
#define RUBY_RETURN_ERROR return INT2FIX (0);
#define RUBY_RETURN_EMPTY return Qnil;
#define RUBY_RETURN_STRING(__string)                        \
    if (__string)                                           \
        return rb_str_new2 (__string);                      \
    return rb_str_new2 ("")
#define RUBY_RETURN_STRING_FREE(__string)                   \
    if (__string)                                           \
    {                                                       \
        return_value = rb_str_new2 (__string);              \
        free (__string);                                    \
        return return_value;                                \
    }                                                       \
    return rb_str_new2 ("")
#define RUBY_RETURN_INT(__int)                  \
    return INT2FIX(__int);


/*
 * weechat_ruby_api_register: startup function for all WeeChat Ruby scripts
 */

static VALUE
weechat_ruby_api_register (VALUE class, VALUE name, VALUE author,
                           VALUE version, VALUE license, VALUE description,
                           VALUE shutdown_func, VALUE charset)
{
    char *c_name, *c_author, *c_version, *c_license, *c_description;
    char *c_shutdown_func, *c_charset;
    
    /* make C compiler happy */
    (void) class;
    
    ruby_current_script = NULL;
    
    c_name = NULL;
    c_author = NULL;
    c_version = NULL;
    c_license = NULL;
    c_description = NULL;
    c_shutdown_func = NULL;
    c_charset = NULL;
    
    if (NIL_P (name) || NIL_P (author) || NIL_P (version)
        || NIL_P (license) || NIL_P (description) || NIL_P (shutdown_func)
        || NIL_P (charset))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(ruby_current_script_filename, "register");
        RUBY_RETURN_ERROR;
    }
    
    Check_Type (name, T_STRING);
    Check_Type (author, T_STRING);
    Check_Type (version, T_STRING);
    Check_Type (license, T_STRING);
    Check_Type (description, T_STRING);
    Check_Type (shutdown_func, T_STRING);
    Check_Type (charset, T_STRING);
    
    c_name = STR2CSTR (name);
    c_author = STR2CSTR (author);
    c_version = STR2CSTR (version);
    c_license = STR2CSTR (license);
    c_description = STR2CSTR (description);
    c_shutdown_func = STR2CSTR (shutdown_func);
    c_charset = STR2CSTR (charset);
    
    if (script_search (weechat_ruby_plugin, ruby_scripts, c_name))
    {
        /* error: another scripts already exists with this name! */
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to register script "
                                         "\"%s\" (another script already "
                                         "exists with this name)"),
                        weechat_prefix ("error"), RUBY_PLUGIN_NAME, c_name);
        RUBY_RETURN_ERROR;
    }
    
    /* register script */
    ruby_current_script = script_add (weechat_ruby_plugin,
                                      &ruby_scripts, &last_ruby_script,
                                      (ruby_current_script_filename) ?
                                      ruby_current_script_filename : "",
                                      c_name, c_author, c_version, c_license,
                                      c_description, c_shutdown_func,
                                      c_charset);
    
    if (ruby_current_script)
    {
        if ((weechat_ruby_plugin->debug >= 1) || !ruby_quiet)
        {
            weechat_printf (NULL,
                            weechat_gettext ("%s: registered script \"%s\", "
                                             "version %s (%s)"),
                            RUBY_PLUGIN_NAME, c_name, c_version, c_description);
        }
    }
    else
    {
        RUBY_RETURN_ERROR;
    }
    
    RUBY_RETURN_OK;
}

/*
 * weechat_ruby_api_plugin_get_name: get name of plugin (return "core" for
 *                                   WeeChat core)
 */

static VALUE
weechat_ruby_api_plugin_get_name (VALUE class, VALUE plugin)
{
    char *c_plugin;
    const char *result;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "plugin_get_name");
        RUBY_RETURN_EMPTY;
    }
    
    c_plugin = NULL;
    
    if (NIL_P (plugin))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "plugin_get_name");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (plugin, T_STRING);
    
    c_plugin = STR2CSTR (plugin);
    
    result = weechat_plugin_get_name (script_str2ptr (c_plugin));
    
    RUBY_RETURN_STRING(result);
}

/*
 * weechat_ruby_api_charset_set: set script charset
 */

static VALUE
weechat_ruby_api_charset_set (VALUE class, VALUE charset)
{
    char *c_charset;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "charset_set");
        RUBY_RETURN_ERROR;
    }
    
    c_charset = NULL;
    
    if (NIL_P (charset))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "charset_set");
        RUBY_RETURN_ERROR;
    }
    
    Check_Type (charset, T_STRING);
    
    c_charset = STR2CSTR (charset);
    
    script_api_charset_set (ruby_current_script, c_charset);
    
    RUBY_RETURN_OK;
}

/*
 * weechat_ruby_api_iconv_to_internal: convert string to internal WeeChat charset
 */

static VALUE
weechat_ruby_api_iconv_to_internal (VALUE class, VALUE charset, VALUE string)
{
    char *c_charset, *c_string, *result;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "iconv_to_internal");
        RUBY_RETURN_EMPTY;
    }
    
    c_charset = NULL;
    c_string = NULL;
    
    if (NIL_P (charset) || NIL_P (string))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "iconv_to_internal");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (charset, T_STRING);
    Check_Type (string, T_STRING);
    
    c_charset = STR2CSTR (charset);
    c_string = STR2CSTR (string);
    
    result = weechat_iconv_to_internal (c_charset, c_string);
    
    RUBY_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_iconv_from_internal: convert string from WeeChat internal
 *                                       charset to another one
 */

static VALUE
weechat_ruby_api_iconv_from_internal (VALUE class, VALUE charset, VALUE string)
{
    char *c_charset, *c_string, *result;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "iconv_from_internal");
        RUBY_RETURN_EMPTY;
    }
    
    c_charset = NULL;
    c_string = NULL;
    
    if (NIL_P (charset) || NIL_P (string))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "iconv_from_internal");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (charset, T_STRING);
    Check_Type (string, T_STRING);
    
    c_charset = STR2CSTR (charset);
    c_string = STR2CSTR (string);
    
    result = weechat_iconv_from_internal (c_charset, c_string);
    
    RUBY_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_gettext: get translated string
 */

static VALUE
weechat_ruby_api_gettext (VALUE class, VALUE string)
{
    char *c_string;
    const char *result;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "gettext");
        RUBY_RETURN_EMPTY;
    }
    
    c_string = NULL;
    
    if (NIL_P (string))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "gettext");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (string, T_STRING);
    
    c_string = STR2CSTR (string);
    
    result = weechat_gettext (c_string);
    
    RUBY_RETURN_STRING(result);
}

/*
 * weechat_ruby_api_ngettext: get translated string with plural form
 */

static VALUE
weechat_ruby_api_ngettext (VALUE class, VALUE single, VALUE plural,
                           VALUE count)
{
    char *c_single, *c_plural;
    const char *result;
    int c_count;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "ngettext");
        RUBY_RETURN_EMPTY;
    }
    
    c_single = NULL;
    c_plural = NULL;
    c_count = 0;
    
    if (NIL_P (single) || NIL_P (plural) || NIL_P (count))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "ngettext");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (single, T_STRING);
    Check_Type (plural, T_STRING);
    Check_Type (count, T_FIXNUM);
    
    c_single = STR2CSTR (single);
    c_plural = STR2CSTR (plural);
    c_count = FIX2INT (count);
    
    result = weechat_ngettext (c_single, c_plural, c_count);
    
    RUBY_RETURN_STRING(result);
}

/*
 * weechat_ruby_api_string_remove_color: remove WeeChat color codes from string
 */

static VALUE
weechat_ruby_api_string_remove_color (VALUE class, VALUE string,
                                      VALUE replacement)
{
    char *c_string, *c_replacement, *result;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "string_remove_color");
        RUBY_RETURN_EMPTY;
    }
    
    c_string = NULL;
    c_replacement = NULL;
    
    if (NIL_P (string) || NIL_P (replacement))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "string_remove_color");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (string, T_STRING);
    Check_Type (replacement, T_STRING);
    
    c_string = STR2CSTR (string);
    c_replacement = STR2CSTR (replacement);
    
    result = weechat_string_remove_color (c_string, c_replacement);
    
    RUBY_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_mkdir_home: create a directory in WeeChat home
 */

static VALUE
weechat_ruby_api_mkdir_home (VALUE class, VALUE directory, VALUE mode)
{
    char *c_directory;
    int c_mode;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "mkdir_home");
        RUBY_RETURN_ERROR;
    }
    
    c_directory = NULL;
    c_mode = 0;
    
    if (NIL_P (directory) || NIL_P (mode))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "mkdir_home");
        RUBY_RETURN_ERROR;
    }
    
    Check_Type (directory, T_STRING);
    Check_Type (mode, T_FIXNUM);
    
    c_directory = STR2CSTR (directory);
    c_mode = FIX2INT (mode);
    
    if (weechat_mkdir_home (c_directory, c_mode))
        RUBY_RETURN_OK;
    
    RUBY_RETURN_ERROR;
}

/*
 * weechat_ruby_api_mkdir: create a directory
 */

static VALUE
weechat_ruby_api_mkdir (VALUE class, VALUE directory, VALUE mode)
{
    char *c_directory;
    int c_mode;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "mkdir");
        RUBY_RETURN_ERROR;
    }
    
    c_directory = NULL;
    c_mode = 0;
    
    if (NIL_P (directory) || NIL_P (mode))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "mkdir");
        RUBY_RETURN_ERROR;
    }
    
    Check_Type (directory, T_STRING);
    Check_Type (mode, T_FIXNUM);
    
    c_directory = STR2CSTR (directory);
    c_mode = FIX2INT (mode);
    
    if (weechat_mkdir (c_directory, c_mode))
        RUBY_RETURN_OK;
    
    RUBY_RETURN_ERROR;
}

/*
 * weechat_ruby_api_mkdir_parents: create a directory and make parent
 *                                 directories as needed
 */

static VALUE
weechat_ruby_api_mkdir_parents (VALUE class, VALUE directory, VALUE mode)
{
    char *c_directory;
    int c_mode;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "mkdir_parents");
        RUBY_RETURN_ERROR;
    }
    
    c_directory = NULL;
    c_mode = 0;
    
    if (NIL_P (directory) || NIL_P (mode))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "mkdir_parents");
        RUBY_RETURN_ERROR;
    }
    
    Check_Type (directory, T_STRING);
    Check_Type (mode, T_FIXNUM);
    
    c_directory = STR2CSTR (directory);
    c_mode = FIX2INT (mode);
    
    if (weechat_mkdir_parents (c_directory, c_mode))
        RUBY_RETURN_OK;
    
    RUBY_RETURN_ERROR;
}

/*
 * weechat_ruby_api_list_new: create a new list
 */

static VALUE
weechat_ruby_api_list_new (VALUE class)
{
    char *result;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "list_new");
        RUBY_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_list_new ());
    
    RUBY_RETURN_STRING(result);
}

/*
 * weechat_ruby_api_list_add: add a string to list
 */

static VALUE
weechat_ruby_api_list_add (VALUE class, VALUE weelist, VALUE data, VALUE where,
                           VALUE user_data)
{
    char *c_weelist, *c_data, *c_where, *c_user_data, *result;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "list_add");
        RUBY_RETURN_EMPTY;
    }
    
    c_weelist = NULL;
    c_data = NULL;
    c_where = NULL;
    c_user_data = NULL;
    
    if (NIL_P (weelist) || NIL_P (data) || NIL_P (where) || NIL_P (user_data))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "list_add");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (weelist, T_STRING);
    Check_Type (data, T_STRING);
    Check_Type (where, T_STRING);
    Check_Type (user_data, T_STRING);
    
    c_weelist = STR2CSTR (weelist);
    c_data = STR2CSTR (data);
    c_where = STR2CSTR (where);
    c_user_data = STR2CSTR (user_data);
    
    result = script_ptr2str (weechat_list_add (script_str2ptr(c_weelist),
                                               c_data,
                                               c_where,
                                               script_str2ptr (c_user_data)));
    
    RUBY_RETURN_STRING(result);
}

/*
 * weechat_ruby_api_list_search: search a string in list
 */

static VALUE
weechat_ruby_api_list_search (VALUE class, VALUE weelist, VALUE data)
{
    char *c_weelist, *c_data, *result;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "list_search");
        RUBY_RETURN_EMPTY;
    }
    
    c_weelist = NULL;
    c_data = NULL;
    
    if (NIL_P (weelist) || NIL_P (data))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "list_search");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (weelist, T_STRING);
    Check_Type (data, T_STRING);
    
    c_weelist = STR2CSTR (weelist);
    c_data = STR2CSTR (data);
    
    result = script_ptr2str (weechat_list_search (script_str2ptr(c_weelist),
                                                  c_data));
    
    RUBY_RETURN_STRING(result);
}

/*
 * weechat_ruby_api_list_casesearch: search a string in list (ignore case)
 */

static VALUE
weechat_ruby_api_list_casesearch (VALUE class, VALUE weelist, VALUE data)
{
    char *c_weelist, *c_data, *result;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "list_casesearch");
        RUBY_RETURN_EMPTY;
    }
    
    c_weelist = NULL;
    c_data = NULL;
    
    if (NIL_P (weelist) || NIL_P (data))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "list_casesearch");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (weelist, T_STRING);
    Check_Type (data, T_STRING);
    
    c_weelist = STR2CSTR (weelist);
    c_data = STR2CSTR (data);
    
    result = script_ptr2str (weechat_list_casesearch (script_str2ptr(c_weelist),
                                                      c_data));
    
    RUBY_RETURN_STRING(result);
}

/*
 * weechat_ruby_api_list_get: get item by position
 */

static VALUE
weechat_ruby_api_list_get (VALUE class, VALUE weelist, VALUE position)
{
    char *c_weelist, *result;
    int c_position;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "list_get");
        RUBY_RETURN_EMPTY;
    }
    
    c_weelist = NULL;
    c_position = 0;
    
    if (NIL_P (weelist) || NIL_P (position))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "list_get");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (weelist, T_STRING);
    Check_Type (position, T_FIXNUM);
    
    c_weelist = STR2CSTR (weelist);
    c_position = FIX2INT (position);
    
    result = script_ptr2str (weechat_list_get (script_str2ptr(c_weelist),
                                               c_position));
    
    RUBY_RETURN_STRING(result);
}

/*
 * weechat_ruby_api_list_set: set new value for item
 */

static VALUE
weechat_ruby_api_list_set (VALUE class, VALUE item, VALUE new_value)
{
    char *c_item, *c_new_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "list_set");
        RUBY_RETURN_ERROR;
    }
    
    c_item = NULL;
    c_new_value = NULL;
    
    if (NIL_P (item) || NIL_P (new_value))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "list_set");
        RUBY_RETURN_ERROR;
    }
    
    Check_Type (item, T_STRING);
    Check_Type (new_value, T_STRING);
    
    c_item = STR2CSTR (item);
    c_new_value = STR2CSTR (new_value);
    
    weechat_list_set (script_str2ptr(c_item),
                      c_new_value);
    
    RUBY_RETURN_OK;
}

/*
 * weechat_ruby_api_list_next: get next item
 */

static VALUE
weechat_ruby_api_list_next (VALUE class, VALUE item)
{
    char *c_item, *result;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "list_next");
        RUBY_RETURN_EMPTY;
    }
    
    c_item = NULL;
    
    if (NIL_P (item))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "list_next");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (item, T_STRING);
    
    c_item = STR2CSTR (item);
    
    result = script_ptr2str (weechat_list_next (script_str2ptr(c_item)));
    
    RUBY_RETURN_STRING(result);
}

/*
 * weechat_ruby_api_list_prev: get previous item
 */

static VALUE
weechat_ruby_api_list_prev (VALUE class, VALUE item)
{
    char *c_item, *result;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "list_prev");
        RUBY_RETURN_EMPTY;
    }
    
    c_item = NULL;
    
    if (NIL_P (item))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "list_prev");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (item, T_STRING);
    
    c_item = STR2CSTR (item);
    
    result = script_ptr2str (weechat_list_prev (script_str2ptr(c_item)));
    
    RUBY_RETURN_STRING(result);
}

/*
 * weechat_ruby_api_list_string: get string value of item
 */

static VALUE
weechat_ruby_api_list_string (VALUE class, VALUE item)
{
    char *c_item;
    const char *result;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "list_string");
        RUBY_RETURN_EMPTY;
    }
    
    c_item = NULL;
    
    if (NIL_P (item))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "list_string");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (item, T_STRING);
    
    c_item = STR2CSTR (item);
    
    result = weechat_list_string (script_str2ptr(c_item));
    
    RUBY_RETURN_STRING(result);
}

/*
 * weechat_ruby_api_list_size: get number of elements in list
 */

static VALUE
weechat_ruby_api_list_size (VALUE class, VALUE weelist)
{
    char *c_weelist;
    int size;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "list_size");
        RUBY_RETURN_INT(0);
    }
    
    c_weelist = NULL;
    
    if (NIL_P (weelist))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "list_size");
        RUBY_RETURN_INT(0);
    }
    
    Check_Type (weelist, T_STRING);
    
    c_weelist = STR2CSTR (weelist);
    
    size = weechat_list_size (script_str2ptr(c_weelist));
    
    RUBY_RETURN_INT(size);
}

/*
 * weechat_ruby_api_list_remove: remove item from list
 */

static VALUE
weechat_ruby_api_list_remove (VALUE class, VALUE weelist, VALUE item)
{
    char *c_weelist, *c_item;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "list_remove");
        RUBY_RETURN_ERROR;
    }
    
    c_weelist = NULL;
    c_item = NULL;
    
    if (NIL_P (weelist) || NIL_P (item))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "list_remove");
        RUBY_RETURN_ERROR;
    }
    
    Check_Type (weelist, T_STRING);
    Check_Type (item, T_STRING);
    
    c_weelist = STR2CSTR (weelist);
    c_item = STR2CSTR (item);
    
    weechat_list_remove (script_str2ptr (c_weelist),
                         script_str2ptr (c_item));
    
    RUBY_RETURN_OK;
}

/*
 * weechat_ruby_api_list_remove_all: remove all items from list
 */

static VALUE
weechat_ruby_api_list_remove_all (VALUE class, VALUE weelist)
{
    char *c_weelist;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "list_remove_all");
        RUBY_RETURN_ERROR;
    }
    
    c_weelist = NULL;
    
    if (NIL_P (weelist))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "list_remove_all");
        RUBY_RETURN_ERROR;
    }
    
    Check_Type (weelist, T_STRING);
    
    c_weelist = STR2CSTR (weelist);
    
    weechat_list_remove_all (script_str2ptr (c_weelist));
    
    RUBY_RETURN_OK;
}

/*
 * weechat_ruby_api_list_free: free list
 */

static VALUE
weechat_ruby_api_list_free (VALUE class, VALUE weelist)
{
    char *c_weelist;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "list_free");
        RUBY_RETURN_ERROR;
    }
    
    c_weelist = NULL;
    
    if (NIL_P (weelist))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "list_free");
        RUBY_RETURN_ERROR;
    }
    
    Check_Type (weelist, T_STRING);
    
    c_weelist = STR2CSTR (weelist);
    
    weechat_list_free (script_str2ptr (c_weelist));
    
    RUBY_RETURN_OK;
}

/*
 * weechat_ruby_api_config_reload_cb: callback for config reload
 */

int
weechat_ruby_api_config_reload_cb (void *data,
                                   struct t_config_file *config_file)
{
    struct t_script_callback *script_callback;
    char *ruby_argv[3], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        ruby_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        ruby_argv[1] = script_ptr2str (config_file);
        ruby_argv[2] = NULL;
        
        rc = (int *) weechat_ruby_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        ruby_argv);
        
        if (!rc)
            ret = WEECHAT_CONFIG_READ_FILE_NOT_FOUND;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (ruby_argv[1])
            free (ruby_argv[1]);
        
        return ret;
    }
    
    return WEECHAT_CONFIG_READ_FILE_NOT_FOUND;
}

/*
 * weechat_ruby_api_config_new: create a new configuration file
 */

static VALUE
weechat_ruby_api_config_new (VALUE class, VALUE name, VALUE function,
                             VALUE data)
{
    char *c_name, *c_function, *c_data, *result;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "config_new");
        RUBY_RETURN_EMPTY;
    }
    
    c_name = NULL;
    c_function = NULL;
    c_data = NULL;
    
    if (NIL_P (name) || NIL_P (function) || NIL_P (data))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "config_new");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (name, T_STRING);
    Check_Type (function, T_STRING);
    Check_Type (data, T_STRING);
    
    c_name = STR2CSTR (name);
    c_function = STR2CSTR (function);
    c_data = STR2CSTR (data);
    
    result = script_ptr2str (script_api_config_new (weechat_ruby_plugin,
                                                    ruby_current_script,
                                                    c_name,
                                                    &weechat_ruby_api_config_reload_cb,
                                                    c_function,
                                                    c_data));
    
    RUBY_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_config_read_cb: callback for reading option in section
 */

int
weechat_ruby_api_config_read_cb (void *data,
                                 struct t_config_file *config_file,
                                 struct t_config_section *section,
                                 const char *option_name, const char *value)
{
    struct t_script_callback *script_callback;
    char *ruby_argv[6], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback && script_callback->function && script_callback->function[0])
    {
        ruby_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        ruby_argv[1] = script_ptr2str (config_file);
        ruby_argv[2] = script_ptr2str (section);
        ruby_argv[3] = (option_name) ? (char *)option_name : empty_arg;
        ruby_argv[4] = (value) ? (char *)value : empty_arg;
        ruby_argv[5] = NULL;
        
        rc = (int *) weechat_ruby_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        ruby_argv);
        
        if (!rc)
            ret = WEECHAT_CONFIG_OPTION_SET_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (ruby_argv[1])
            free (ruby_argv[1]);
        if (ruby_argv[2])
            free (ruby_argv[2]);
        
        return ret;
    }
    
    return WEECHAT_CONFIG_OPTION_SET_ERROR;
}

/*
 * weechat_ruby_api_config_section_write_cb: callback for writing section
 */

void
weechat_ruby_api_config_section_write_cb (void *data,
                                          struct t_config_file *config_file,
                                          const char *section_name)
{
    struct t_script_callback *script_callback;
    char *ruby_argv[4], empty_arg[1] = { '\0' };
    int *rc;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback && script_callback->function && script_callback->function[0])
    {
        ruby_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        ruby_argv[1] = script_ptr2str (config_file);
        ruby_argv[2] = (section_name) ? (char *)section_name : empty_arg;
        ruby_argv[3] = NULL;
        
        rc = (int *) weechat_ruby_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        ruby_argv);
        
        if (rc)
            free (rc);
        if (ruby_argv[1])
            free (ruby_argv[1]);
    }
}

/*
 * weechat_ruby_api_config_section_write_default_cb: callback for writing
 *                                                   default values for section
 */

void
weechat_ruby_api_config_section_write_default_cb (void *data,
                                                  struct t_config_file *config_file,
                                                  const char *section_name)
{
    struct t_script_callback *script_callback;
    char *ruby_argv[4], empty_arg[1] = { '\0' };
    int *rc;
    
    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        ruby_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        ruby_argv[1] = script_ptr2str (config_file);
        ruby_argv[2] = (section_name) ? (char *)section_name : empty_arg;
        ruby_argv[3] = NULL;
        
        rc = (int *) weechat_ruby_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        ruby_argv);
        
        if (rc)
            free (rc);
        if (ruby_argv[1])
            free (ruby_argv[1]);
    }
}

/*
 * weechat_ruby_api_config_section_create_option_cb: callback to create an option
 */

int
weechat_ruby_api_config_section_create_option_cb (void *data,
                                                  struct t_config_file *config_file,
                                                  struct t_config_section *section,
                                                  const char *option_name,
                                                  const char *value)
{
    struct t_script_callback *script_callback;
    char *ruby_argv[6], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback && script_callback->function && script_callback->function[0])
    {
        ruby_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        ruby_argv[1] = script_ptr2str (config_file);
        ruby_argv[2] = script_ptr2str (section);
        ruby_argv[3] = (option_name) ? (char *)option_name : empty_arg;
        ruby_argv[4] = (value) ? (char *)value : empty_arg;
        ruby_argv[5] = NULL;
        
        rc = (int *) weechat_ruby_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        ruby_argv);
        
        if (!rc)
            ret = WEECHAT_CONFIG_OPTION_SET_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (ruby_argv[1])
            free (ruby_argv[1]);
        if (ruby_argv[2])
            free (ruby_argv[2]);
        
        return ret;
    }
    
    return WEECHAT_CONFIG_OPTION_SET_ERROR;
}

/*
 * weechat_ruby_api_config_section_delete_option_cb: callback to delete an option
 */

int
weechat_ruby_api_config_section_delete_option_cb (void *data,
                                                  struct t_config_file *config_file,
                                                  struct t_config_section *section,
                                                  struct t_config_option *option)
{
    struct t_script_callback *script_callback;
    char *ruby_argv[5], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback && script_callback->function && script_callback->function[0])
    {
        ruby_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        ruby_argv[1] = script_ptr2str (config_file);
        ruby_argv[2] = script_ptr2str (section);
        ruby_argv[3] = script_ptr2str (option);
        ruby_argv[4] = NULL;
        
        rc = (int *) weechat_ruby_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        ruby_argv);
        
        if (!rc)
            ret = WEECHAT_CONFIG_OPTION_UNSET_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (ruby_argv[1])
            free (ruby_argv[1]);
        if (ruby_argv[2])
            free (ruby_argv[2]);
        if (ruby_argv[3])
            free (ruby_argv[3]);
        
        return ret;
    }
    
    return WEECHAT_CONFIG_OPTION_UNSET_ERROR;
}

/*
 * weechat_ruby_api_config_new_section: create a new section in configuration file
 */

static VALUE
weechat_ruby_api_config_new_section (VALUE class, VALUE config_file,
                                     VALUE name, VALUE user_can_add_options,
                                     VALUE user_can_delete_options,
                                     VALUE function_read,
                                     VALUE data_read,
                                     VALUE function_write,
                                     VALUE data_write,
                                     VALUE function_write_default,
                                     VALUE data_write_default,
                                     VALUE function_create_option,
                                     VALUE data_create_option,
                                     VALUE function_delete_option,
                                     VALUE data_delete_option)
{
    char *c_config_file, *c_name, *c_function_read, *c_data_read;
    char *c_function_write, *c_data_write, *c_function_write_default;
    char *c_data_write_default, *c_function_create_option;
    char *c_data_create_option, *c_function_delete_option;
    char *c_data_delete_option, *result;
    int c_user_can_add_options, c_user_can_delete_options;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "config_new_section");
        RUBY_RETURN_EMPTY;
    }
    
    c_config_file = NULL;
    c_name = NULL;
    c_user_can_add_options = 0;
    c_user_can_delete_options = 0;
    c_function_read = NULL;
    c_data_read = NULL;
    c_function_write = NULL;
    c_data_write = NULL;
    c_function_write_default = NULL;
    c_data_write_default = NULL;
    c_function_create_option = NULL;
    c_data_create_option = NULL;
    c_function_delete_option = NULL;
    c_data_delete_option = NULL;
    
    if (NIL_P (config_file) || NIL_P (name) || NIL_P (user_can_add_options)
        || NIL_P (user_can_delete_options) || NIL_P (function_read)
        || NIL_P (data_read) || NIL_P (function_write) || NIL_P (data_write)
        || NIL_P (function_write_default) || NIL_P (data_write_default)
        || NIL_P (function_create_option) || NIL_P (data_create_option)
        || NIL_P (function_delete_option) || NIL_P (data_delete_option))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "config_new_section");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (config_file, T_STRING);
    Check_Type (name, T_STRING);
    Check_Type (user_can_add_options, T_FIXNUM);
    Check_Type (user_can_delete_options, T_FIXNUM);
    Check_Type (function_read, T_STRING);
    Check_Type (data_read, T_STRING);
    Check_Type (function_write, T_STRING);
    Check_Type (data_write, T_STRING);
    Check_Type (function_write_default, T_STRING);
    Check_Type (data_write_default, T_STRING);
    Check_Type (function_create_option, T_STRING);
    Check_Type (data_create_option, T_STRING);
    Check_Type (function_delete_option, T_STRING);
    Check_Type (data_delete_option, T_STRING);
    
    c_config_file = STR2CSTR (config_file);
    c_name = STR2CSTR (name);
    c_user_can_add_options = FIX2INT (user_can_add_options);
    c_user_can_delete_options = FIX2INT (user_can_delete_options);
    c_function_read = STR2CSTR (function_read);
    c_data_read = STR2CSTR (data_read);
    c_function_write = STR2CSTR (function_write);
    c_data_write = STR2CSTR (data_write);
    c_function_write_default = STR2CSTR (function_write_default);
    c_data_write_default = STR2CSTR (data_write_default);
    c_function_create_option = STR2CSTR (function_create_option);
    c_data_create_option = STR2CSTR (data_create_option);
    c_function_delete_option = STR2CSTR (function_delete_option);
    c_data_delete_option = STR2CSTR (data_delete_option);
    
    result = script_ptr2str (script_api_config_new_section (weechat_ruby_plugin,
                                                            ruby_current_script,
                                                            script_str2ptr (c_config_file),
                                                            c_name,
                                                            c_user_can_add_options,
                                                            c_user_can_delete_options,
                                                            &weechat_ruby_api_config_read_cb,
                                                            c_function_read,
                                                            c_data_read,
                                                            &weechat_ruby_api_config_section_write_cb,
                                                            c_function_write,
                                                            c_data_write,
                                                            &weechat_ruby_api_config_section_write_default_cb,
                                                            c_function_write_default,
                                                            c_data_write_default,
                                                            &weechat_ruby_api_config_section_create_option_cb,
                                                            c_function_create_option,
                                                            c_data_create_option,
                                                            &weechat_ruby_api_config_section_delete_option_cb,
                                                            c_function_delete_option,
                                                            c_data_delete_option));
    
    RUBY_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_config_search_section: search section in configuration file
 */

static VALUE
weechat_ruby_api_config_search_section (VALUE class, VALUE config_file,
                                        VALUE section_name)
{
    char *c_config_file, *c_section_name, *result;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "config_search_section");
        RUBY_RETURN_EMPTY;
    }
    
    c_config_file = NULL;
    c_section_name = NULL;
    
    if (NIL_P (config_file) || NIL_P (section_name))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "config_search_section");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (config_file, T_STRING);
    Check_Type (section_name, T_STRING);
    
    c_config_file = STR2CSTR (config_file);
    c_section_name = STR2CSTR (section_name);
    
    result = script_ptr2str (weechat_config_search_section (script_str2ptr (c_config_file),
                                                            c_section_name));
    
    RUBY_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_config_option_check_value_cb: callback for checking new
 *                                                value for option
 */

int
weechat_ruby_api_config_option_check_value_cb (void *data,
                                               struct t_config_option *option,
                                               const char *value)
{
    struct t_script_callback *script_callback;
    char *ruby_argv[4], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback && script_callback->function && script_callback->function[0])
    {
        ruby_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        ruby_argv[1] = script_ptr2str (option);
        ruby_argv[2] = (value) ? (char *)value : empty_arg;
        ruby_argv[3] = NULL;
        
        rc = (int *) weechat_ruby_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        ruby_argv);
        
        if (!rc)
            ret = 0;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (ruby_argv[1])
            free (ruby_argv[1]);
        
        return ret;
    }
    
    return 0;
}

/*
 * weechat_ruby_api_config_option_change_cb: callback for option changed
 */

void
weechat_ruby_api_config_option_change_cb (void *data,
                                          struct t_config_option *option)
{
    struct t_script_callback *script_callback;
    char *ruby_argv[3], empty_arg[1] = { '\0' };
    int *rc;
    
    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        ruby_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        ruby_argv[1] = script_ptr2str (option);
        ruby_argv[2] = NULL;
        
        rc = (int *) weechat_ruby_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        ruby_argv);
        
        if (ruby_argv[1])
            free (ruby_argv[1]);
        
        if (rc)
            free (rc);
    }
}

/*
 * weechat_ruby_api_config_option_delete_cb: callback when option is deleted
 */

void
weechat_ruby_api_config_option_delete_cb (void *data,
                                          struct t_config_option *option)
{
    struct t_script_callback *script_callback;
    char *ruby_argv[3], empty_arg[1] = { '\0' };
    int *rc;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback && script_callback->function && script_callback->function[0])
    {
        ruby_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        ruby_argv[1] = script_ptr2str (option);
        ruby_argv[2] = NULL;
        
        rc = (int *) weechat_ruby_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        ruby_argv);
        
        if (ruby_argv[1])
            free (ruby_argv[1]);
        
        if (rc)
            free (rc);
    }
}

/*
 * weechat_ruby_api_config_new_option: create a new option in section
 */

static VALUE
weechat_ruby_api_config_new_option (VALUE class, VALUE config_file,
                                    VALUE section, VALUE name, VALUE type,
                                    VALUE description, VALUE string_values,
                                    VALUE min, VALUE max, VALUE default_value,
                                    VALUE value, VALUE null_value_allowed,
                                    VALUE function_check_value,
                                    VALUE data_check_value,
                                    VALUE function_change,
                                    VALUE data_change,
                                    VALUE function_delete,
                                    VALUE data_delete)
{
    char *c_config_file, *c_section, *c_name, *c_type, *c_description;
    char *c_string_values, *c_default_value, *c_value, *result;
    char *c_function_check_value, *c_data_check_value, *c_function_change;
    char *c_data_change, *c_function_delete, *c_data_delete;
    int c_min, c_max, c_null_value_allowed;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "config_new_option");
        RUBY_RETURN_EMPTY;
    }
    
    c_config_file = NULL;
    c_section = NULL;
    c_name = NULL;
    c_type = NULL;
    c_description = NULL;
    c_string_values = NULL;
    c_min = 0;
    c_max = 0;
    c_default_value = NULL;
    c_value = NULL;
    c_null_value_allowed = 0;
    c_function_check_value = NULL;
    c_data_check_value = NULL;
    c_function_change = NULL;
    c_data_change = NULL;
    c_function_delete = NULL;
    c_data_delete = NULL;
    
    if (NIL_P (config_file) || NIL_P (section) || NIL_P (name) || NIL_P (type)
        || NIL_P (description) || NIL_P (string_values)
        || NIL_P (default_value) || NIL_P (value) || NIL_P (null_value_allowed)
        || NIL_P (function_check_value) || NIL_P (data_check_value)
        || NIL_P (function_change) || NIL_P (data_change)
        || NIL_P (function_delete) || NIL_P (data_delete))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "config_new_option");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (config_file, T_STRING);
    Check_Type (section, T_STRING);
    Check_Type (name, T_STRING);
    Check_Type (type, T_STRING);
    Check_Type (description, T_STRING);
    Check_Type (string_values, T_STRING);
    Check_Type (min, T_FIXNUM);
    Check_Type (max, T_FIXNUM);
    Check_Type (default_value, T_STRING);
    Check_Type (value, T_STRING);
    Check_Type (null_value_allowed, T_FIXNUM);
    Check_Type (function_check_value, T_STRING);
    Check_Type (data_check_value, T_STRING);
    Check_Type (function_change, T_STRING);
    Check_Type (data_change, T_STRING);
    Check_Type (function_delete, T_STRING);
    Check_Type (data_delete, T_STRING);
    
    c_config_file = STR2CSTR (config_file);
    c_section = STR2CSTR (section);
    c_name = STR2CSTR (name);
    c_type = STR2CSTR (type);
    c_description = STR2CSTR (description);
    c_string_values = STR2CSTR (string_values);
    c_min = FIX2INT (min);
    c_max = FIX2INT (max);
    c_default_value = STR2CSTR (default_value);
    c_value = STR2CSTR (value);
    c_null_value_allowed = FIX2INT (null_value_allowed);
    c_function_check_value = STR2CSTR (function_check_value);
    c_data_check_value = STR2CSTR (data_check_value);
    c_function_change = STR2CSTR (function_change);
    c_data_change = STR2CSTR (data_change);
    c_function_delete = STR2CSTR (function_delete);
    c_data_delete = STR2CSTR (data_delete);
    
    result = script_ptr2str (script_api_config_new_option (weechat_ruby_plugin,
                                                           ruby_current_script,
                                                           script_str2ptr (c_config_file),
                                                           script_str2ptr (c_section),
                                                           c_name,
                                                           c_type,
                                                           c_description,
                                                           c_string_values,
                                                           c_min,
                                                           c_max,
                                                           c_default_value,
                                                           c_value,
                                                           c_null_value_allowed,
                                                           &weechat_ruby_api_config_option_check_value_cb,
                                                           c_function_check_value,
                                                           c_data_check_value,
                                                           &weechat_ruby_api_config_option_change_cb,
                                                           c_function_change,
                                                           c_data_change,
                                                           &weechat_ruby_api_config_option_delete_cb,
                                                           c_function_delete,
                                                           c_data_delete));
    
    RUBY_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_config_search_option: search option in configuration file or section
 */

static VALUE
weechat_ruby_api_config_search_option (VALUE class, VALUE config_file,
                                       VALUE section, VALUE option_name)
{
    char *c_config_file, *c_section, *c_option_name, *result;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "config_search_option");
        RUBY_RETURN_EMPTY;
    }
    
    c_config_file = NULL;
    c_section = NULL;
    c_option_name = NULL;
    
    if (NIL_P (config_file) || NIL_P (section) || NIL_P (option_name))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "config_search_option");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (config_file, T_STRING);
    Check_Type (section, T_STRING);
    Check_Type (option_name, T_STRING);
    
    c_config_file = STR2CSTR (config_file);
    c_section = STR2CSTR (section);
    c_option_name = STR2CSTR (option_name);
    
    result = script_ptr2str (weechat_config_search_option (script_str2ptr (c_config_file),
                                                           script_str2ptr (c_section),
                                                           c_option_name));
    
    RUBY_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_config_string_to_boolean: return boolean value of a string
 */

static VALUE
weechat_ruby_api_config_string_to_boolean (VALUE class, VALUE text)
{
    char *c_text;
    int value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "config_string_to_boolean");
        RUBY_RETURN_INT(0);
    }
    
    c_text = NULL;
    
    if (NIL_P (text))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "config_string_to_boolean");
        RUBY_RETURN_INT(0);
    }
    
    Check_Type (text, T_STRING);
    
    c_text = STR2CSTR (text);
    
    value = weechat_config_string_to_boolean (c_text);
    
    RUBY_RETURN_INT(value);
}

/*
 * weechat_ruby_api_config_option_reset: reset option with default value
 */

static VALUE
weechat_ruby_api_config_option_reset (VALUE class, VALUE option,
                                      VALUE run_callback)
{
    char *c_option;
    int c_run_callback, rc;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "config_option_reset");
        RUBY_RETURN_INT(0);
    }
    
    c_option = NULL;
    c_run_callback = 0;
    
    if (NIL_P (option) || NIL_P (run_callback))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "config_option_reset");
        RUBY_RETURN_INT(0);
    }
    
    Check_Type (option, T_STRING);
    Check_Type (run_callback, T_FIXNUM);
    
    c_option = STR2CSTR (option);
    c_run_callback = FIX2INT (run_callback);
    
    rc = weechat_config_option_reset (script_str2ptr (c_option),
                                      c_run_callback);
    
    RUBY_RETURN_INT(rc);
}

/*
 * weechat_ruby_api_config_option_set: set new value for option
 */

static VALUE
weechat_ruby_api_config_option_set (VALUE class, VALUE option, VALUE new_value,
                                    VALUE run_callback)
{
    char *c_option, *c_new_value;
    int c_run_callback, rc;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "config_option_set");
        RUBY_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR);
    }
    
    c_option = NULL;
    c_new_value = NULL;
    c_run_callback = 0;
    
    if (NIL_P (option) || NIL_P (new_value) || NIL_P (run_callback))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "config_option_set");
        RUBY_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR);
    }
    
    Check_Type (option, T_STRING);
    Check_Type (new_value, T_STRING);
    Check_Type (run_callback, T_FIXNUM);
    
    c_option = STR2CSTR (option);
    c_new_value = STR2CSTR (new_value);
    c_run_callback = FIX2INT (run_callback);
    
    rc = weechat_config_option_set (script_str2ptr (c_option),
                                    c_new_value,
                                    c_run_callback);
    
    RUBY_RETURN_INT(rc);
}

/*
 * weechat_ruby_api_config_option_set_null: set null (undefined) value for
 *                                          option
 */

static VALUE
weechat_ruby_api_config_option_set_null (VALUE class, VALUE option,
                                         VALUE run_callback)
{
    char *c_option;
    int c_run_callback, rc;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "config_option_set_null");
        RUBY_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR);
    }
    
    c_option = NULL;
    c_run_callback = 0;
    
    if (NIL_P (option) || NIL_P (run_callback))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "config_option_set_null");
        RUBY_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR);
    }
    
    Check_Type (option, T_STRING);
    Check_Type (run_callback, T_FIXNUM);
    
    c_option = STR2CSTR (option);
    c_run_callback = FIX2INT (run_callback);
    
    rc = weechat_config_option_set_null (script_str2ptr (c_option),
                                         c_run_callback);
    
    RUBY_RETURN_INT(rc);
}

/*
 * weechat_ruby_api_config_option_unset: unset an option
 */

static VALUE
weechat_ruby_api_config_option_unset (VALUE class, VALUE option)
{
    char *c_option;
    int rc;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "config_option_unset");
        RUBY_RETURN_INT(WEECHAT_CONFIG_OPTION_UNSET_ERROR);
    }
    
    c_option = NULL;
    
    if (NIL_P (option))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "config_option_unset");
        RUBY_RETURN_INT(WEECHAT_CONFIG_OPTION_UNSET_ERROR);
    }
    
    Check_Type (option, T_STRING);
    
    c_option = STR2CSTR (option);
    
    rc = weechat_config_option_unset (script_str2ptr (c_option));
    
    RUBY_RETURN_INT(rc);
}

/*
 * weechat_ruby_api_config_option_rename: rename an option
 */

static VALUE
weechat_ruby_api_config_option_rename (VALUE class, VALUE option,
                                       VALUE new_name)
{
    char *c_option, *c_new_name;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "config_option_rename");
        RUBY_RETURN_ERROR;
    }
    
    c_option = NULL;
    c_new_name = NULL;
    
    if (NIL_P (option) || NIL_P (new_name))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "config_option_rename");
        RUBY_RETURN_ERROR;
    }
    
    Check_Type (option, T_STRING);
    Check_Type (new_name, T_STRING);
    
    c_option = STR2CSTR (option);
    c_new_name = STR2CSTR (new_name);
    
    weechat_config_option_rename (script_str2ptr (c_option),
                                  c_new_name);
    
    RUBY_RETURN_OK;
}

/*
 * weechat_ruby_api_config_option_is_null: return 1 if value of option is null
 */

static VALUE
weechat_ruby_api_config_option_is_null (VALUE class, VALUE option)
{
    char *c_option;
    int value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "config_option_is_null");
        RUBY_RETURN_INT(1);
    }
    
    c_option = NULL;
    
    if (NIL_P (option))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "config_option_is_null");
        RUBY_RETURN_INT(1);
    }
    
    Check_Type (option, T_STRING);
    
    c_option = STR2CSTR (option);
    
    value = weechat_config_option_is_null (script_str2ptr (c_option));
    
    RUBY_RETURN_INT(value);
}

/*
 * weechat_ruby_api_config_option_default_is_null: return 1 if value of option is null
 */

static VALUE
weechat_ruby_api_config_option_default_is_null (VALUE class, VALUE option)
{
    char *c_option;
    int value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "config_option_default_is_null");
        RUBY_RETURN_INT(1);
    }
    
    c_option = NULL;
    
    if (NIL_P (option))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "config_option_default_is_null");
        RUBY_RETURN_INT(1);
    }
    
    Check_Type (option, T_STRING);
    
    c_option = STR2CSTR (option);
    
    value = weechat_config_option_default_is_null (script_str2ptr (c_option));
    
    RUBY_RETURN_INT(value);
}

/*
 * weechat_ruby_api_config_boolean: return boolean value of option
 */

static VALUE
weechat_ruby_api_config_boolean (VALUE class, VALUE option)
{
    char *c_option;
    int value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "config_boolean");
        RUBY_RETURN_INT(0);
    }
    
    c_option = NULL;
    
    if (NIL_P (option))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "config_boolean");
        RUBY_RETURN_INT(0);
    }
    
    Check_Type (option, T_STRING);
    
    c_option = STR2CSTR (option);
    
    value = weechat_config_boolean (script_str2ptr (c_option));
    
    RUBY_RETURN_INT(value);
}

/*
 * weechat_ruby_api_config_boolean_default: return default boolean value of option
 */

static VALUE
weechat_ruby_api_config_boolean_default (VALUE class, VALUE option)
{
    char *c_option;
    int value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "config_boolean_default");
        RUBY_RETURN_INT(0);
    }
    
    c_option = NULL;
    
    if (NIL_P (option))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "config_boolean_default");
        RUBY_RETURN_INT(0);
    }
    
    Check_Type (option, T_STRING);
    
    c_option = STR2CSTR (option);
    
    value = weechat_config_boolean_default (script_str2ptr (c_option));
    
    RUBY_RETURN_INT(value);
}

/*
 * weechat_ruby_api_config_integer: return integer value of option
 */

static VALUE
weechat_ruby_api_config_integer (VALUE class, VALUE option)
{
    char *c_option;
    int value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "config_integer");
        RUBY_RETURN_INT(0);
    }
    
    c_option = NULL;
    
    if (NIL_P (option))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "config_integer");
        RUBY_RETURN_INT(0);
    }
    
    Check_Type (option, T_STRING);
    
    c_option = STR2CSTR (option);
    
    value = weechat_config_integer (script_str2ptr (c_option));
    
    RUBY_RETURN_INT(value);
}

/*
 * weechat_ruby_api_config_integer_default: return default integer value of option
 */

static VALUE
weechat_ruby_api_config_integer_default (VALUE class, VALUE option)
{
    char *c_option;
    int value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "config_integer_default");
        RUBY_RETURN_INT(0);
    }
    
    c_option = NULL;
    
    if (NIL_P (option))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "config_integer_default");
        RUBY_RETURN_INT(0);
    }
    
    Check_Type (option, T_STRING);
    
    c_option = STR2CSTR (option);
    
    value = weechat_config_integer_default (script_str2ptr (c_option));
    
    RUBY_RETURN_INT(value);
}

/*
 * weechat_ruby_api_config_string: return string value of option
 */

static VALUE
weechat_ruby_api_config_string (VALUE class, VALUE option)
{
    char *c_option;
    const char *result;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "config_string");
        RUBY_RETURN_EMPTY;
    }
    
    c_option = NULL;
    
    if (NIL_P (option))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "config_string");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (option, T_STRING);
    
    c_option = STR2CSTR (option);
    
    result = weechat_config_string (script_str2ptr (c_option));
    
    RUBY_RETURN_STRING(result);
}

/*
 * weechat_ruby_api_config_string_default: return default string value of option
 */

static VALUE
weechat_ruby_api_config_string_default (VALUE class, VALUE option)
{
    char *c_option;
    const char *result;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "config_string_default");
        RUBY_RETURN_EMPTY;
    }
    
    c_option = NULL;
    
    if (NIL_P (option))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "config_string_default");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (option, T_STRING);
    
    c_option = STR2CSTR (option);
    
    result = weechat_config_string_default (script_str2ptr (c_option));
    
    RUBY_RETURN_STRING(result);
}

/*
 * weechat_ruby_api_config_color: return color value of option
 */

static VALUE
weechat_ruby_api_config_color (VALUE class, VALUE option)
{
    char *c_option;
    const char *result;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "config_color");
        RUBY_RETURN_INT(0);
    }
    
    c_option = NULL;
    
    if (NIL_P (option))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "config_color");
        RUBY_RETURN_INT(0);
    }
    
    Check_Type (option, T_STRING);
    
    c_option = STR2CSTR (option);
    
    result = weechat_config_color (script_str2ptr (c_option));
    
    RUBY_RETURN_STRING(result);
}

/*
 * weechat_ruby_api_config_color_default: return default color value of option
 */

static VALUE
weechat_ruby_api_config_color_default (VALUE class, VALUE option)
{
    char *c_option;
    const char *result;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "config_color_default");
        RUBY_RETURN_INT(0);
    }
    
    c_option = NULL;
    
    if (NIL_P (option))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "config_color_default");
        RUBY_RETURN_INT(0);
    }
    
    Check_Type (option, T_STRING);
    
    c_option = STR2CSTR (option);
    
    result = weechat_config_color_default (script_str2ptr (c_option));
    
    RUBY_RETURN_STRING(result);
}

/*
 * weechat_ruby_api_config_write_option: write an option in configuration file
 */

static VALUE
weechat_ruby_api_config_write_option (VALUE class, VALUE config_file,
                                      VALUE option)
{
    char *c_config_file, *c_option;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "config_write_option");
        RUBY_RETURN_ERROR;
    }
    
    c_config_file = NULL;
    c_option = NULL;
    
    if (NIL_P (config_file) || NIL_P (option))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "config_write_option");
        RUBY_RETURN_ERROR;
    }
    
    Check_Type (config_file, T_STRING);
    Check_Type (option, T_STRING);
    
    c_config_file = STR2CSTR (config_file);
    c_option = STR2CSTR (option);
    
    weechat_config_write_option (script_str2ptr (c_config_file),
                                 script_str2ptr (c_option));
    
    RUBY_RETURN_OK;
}

/*
 * weechat_ruby_api_config_write_line: write a line in configuration file
 */

static VALUE
weechat_ruby_api_config_write_line (VALUE class, VALUE config_file,
                                    VALUE option_name, VALUE value)
{
    char *c_config_file, *c_option_name, *c_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "config_write_line");
        RUBY_RETURN_ERROR;
    }
    
    c_config_file = NULL;
    c_option_name = NULL;
    c_value = NULL;
    
    if (NIL_P (config_file) || NIL_P (option_name) || NIL_P (value))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "config_write_line");
        RUBY_RETURN_ERROR;
    }
    
    Check_Type (config_file, T_STRING);
    Check_Type (option_name, T_STRING);
    Check_Type (value, T_STRING);
    
    c_config_file = STR2CSTR (config_file);
    c_option_name = STR2CSTR (option_name);
    c_value = STR2CSTR (value);
    
    weechat_config_write_line (script_str2ptr (c_config_file),
                               c_option_name,
                               "%s",
                               c_value);
    
    RUBY_RETURN_OK;
}

/*
 * weechat_ruby_api_config_write: write configuration file
 */

static VALUE
weechat_ruby_api_config_write (VALUE class, VALUE config_file)
{
    char *c_config_file;
    int rc;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "config_write");
        RUBY_RETURN_INT(-1);
    }
    
    c_config_file = NULL;
    
    if (NIL_P (config_file))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "config_write");
        RUBY_RETURN_INT(-1);
    }
    
    Check_Type (config_file, T_STRING);
    
    c_config_file = STR2CSTR (config_file);
    
    rc = weechat_config_write (script_str2ptr (c_config_file));
    
    RUBY_RETURN_INT(rc);
}

/*
 * weechat_ruby_api_config_read: read configuration file
 */

static VALUE
weechat_ruby_api_config_read (VALUE class, VALUE config_file)
{
    char *c_config_file;
    int rc;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "config_read");
        RUBY_RETURN_INT(-1);
    }
    
    c_config_file = NULL;
    
    if (NIL_P (config_file))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "config_read");
        RUBY_RETURN_INT(-1);
    }
    
    Check_Type (config_file, T_STRING);
    
    c_config_file = STR2CSTR (config_file);
    
    rc = weechat_config_read (script_str2ptr (c_config_file));
    
    RUBY_RETURN_INT(rc);
}

/*
 * weechat_ruby_api_config_reload: reload configuration file
 */

static VALUE
weechat_ruby_api_config_reload (VALUE class, VALUE config_file)
{
    char *c_config_file;
    int rc;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "config_reload");
        RUBY_RETURN_INT(-1);
    }
    
    c_config_file = NULL;
    
    if (NIL_P (config_file))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "config_reload");
        RUBY_RETURN_INT(-1);
    }
    
    Check_Type (config_file, T_STRING);
    
    c_config_file = STR2CSTR (config_file);
    
    rc = weechat_config_reload (script_str2ptr (c_config_file));
    
    RUBY_RETURN_INT(rc);
}

/*
 * weechat_ruby_api_config_option_free: free an option in configuration file
 */

static VALUE
weechat_ruby_api_config_option_free (VALUE class, VALUE option)
{
    char *c_option;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "config_option_free");
        RUBY_RETURN_ERROR;
    }
    
    c_option = NULL;
    
    if (NIL_P (option))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "config_option_free");
        RUBY_RETURN_ERROR;
    }
    
    Check_Type (option, T_STRING);
    
    c_option = STR2CSTR (option);
    
    script_api_config_option_free (weechat_ruby_plugin,
                                   ruby_current_script,
                                   script_str2ptr (c_option));
    
    RUBY_RETURN_OK;
}

/*
 * weechat_ruby_api_config_section_free_options: free all options of a section
 *                                               in configuration file
 */

static VALUE
weechat_ruby_api_config_section_free_options (VALUE class, VALUE section)
{
    char *c_section;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "config_section_free_options");
        RUBY_RETURN_ERROR;
    }
    
    c_section = NULL;
    
    if (NIL_P (section))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "config_section_free_options");
        RUBY_RETURN_ERROR;
    }
    
    Check_Type (section, T_STRING);
    
    c_section = STR2CSTR (section);
    
    script_api_config_section_free_options (weechat_ruby_plugin,
                                            ruby_current_script,
                                            script_str2ptr (c_section));
    
    RUBY_RETURN_OK;
}

/*
 * weechat_ruby_api_config_section_free: free section in configuration file
 */

static VALUE
weechat_ruby_api_config_section_free (VALUE class, VALUE section)
{
    char *c_section;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "config_section_free");
        RUBY_RETURN_ERROR;
    }
    
    c_section = NULL;
    
    if (NIL_P (section))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "config_section_free");
        RUBY_RETURN_ERROR;
    }
    
    Check_Type (section, T_STRING);
    
    c_section = STR2CSTR (section);
    
    script_api_config_section_free (weechat_ruby_plugin,
                                    ruby_current_script,
                                    script_str2ptr (c_section));
    
    RUBY_RETURN_OK;
}

/*
 * weechat_ruby_api_config_free: free configuration file
 */

static VALUE
weechat_ruby_api_config_free (VALUE class, VALUE config_file)
{
    char *c_config_file;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "config_free");
        RUBY_RETURN_ERROR;
    }
    
    c_config_file = NULL;
    
    if (NIL_P (config_file))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "config_free");
        RUBY_RETURN_ERROR;
    }
    
    Check_Type (config_file, T_STRING);
    
    c_config_file = STR2CSTR (config_file);
    
    script_api_config_free (weechat_ruby_plugin,
                            ruby_current_script,
                            script_str2ptr (c_config_file));
    
    RUBY_RETURN_OK;
}

/*
 * weechat_ruby_api_config_get: get config option
 */

static VALUE
weechat_ruby_api_config_get (VALUE class, VALUE option)
{
    char *c_option, *result;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "config_get");
        RUBY_RETURN_EMPTY;
    }
    
    if (NIL_P (option))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "config_get");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (option, T_STRING);
    
    c_option = STR2CSTR (option);

    result = script_ptr2str (weechat_config_get (c_option));
    
    RUBY_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_config_get_plugin: get value of a plugin option
 */

static VALUE
weechat_ruby_api_config_get_plugin (VALUE class, VALUE option)
{
    char *c_option;
    const char *result;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "config_get_plugin");
        RUBY_RETURN_EMPTY;
    }
    
    if (NIL_P (option))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "config_get_plugin");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (option, T_STRING);
    
    c_option = STR2CSTR (option);
    
    result = script_api_config_get_plugin (weechat_ruby_plugin,
                                           ruby_current_script,
                                           c_option);
    
    RUBY_RETURN_STRING(result);
}

/*
 * weechat_ruby_api_config_is_set_plugin: check if a plugin option is set
 */

static VALUE
weechat_ruby_api_config_is_set_plugin (VALUE class, VALUE option)
{
    char *c_option;
    int rc;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "config_is_set_plugin");
        RUBY_RETURN_INT(0);
    }
    
    if (NIL_P (option))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "config_is_set_plugin");
        RUBY_RETURN_INT(0);
    }
    
    Check_Type (option, T_STRING);
    
    c_option = STR2CSTR (option);
    
    rc = script_api_config_is_set_plugin (weechat_ruby_plugin,
                                          ruby_current_script,
                                          c_option);
    
    RUBY_RETURN_INT(rc);
}

/*
 * weechat_ruby_api_config_set_plugin: set value of a plugin option
 */

static VALUE
weechat_ruby_api_config_set_plugin (VALUE class, VALUE option, VALUE value)
{
    char *c_option, *c_value;
    int rc;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "config_set_plugin");
        RUBY_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR);
    }
    
    if (NIL_P (option) || NIL_P (value))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "config_set_plugin");
        RUBY_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR);
    }
    
    Check_Type (option, T_STRING);
    Check_Type (value, T_STRING);
    
    c_option = STR2CSTR (option);
    c_value = STR2CSTR (value);
    
    rc = script_api_config_set_plugin (weechat_ruby_plugin,
                                       ruby_current_script,
                                       c_option,
                                       c_value);
    
    RUBY_RETURN_INT(rc);
}

/*
 * weechat_ruby_api_config_unset_plugin: unset plugin option
 */

static VALUE
weechat_ruby_api_config_unset_plugin (VALUE class, VALUE option)
{
    char *c_option;
    int rc;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "config_unset_plugin");
        RUBY_RETURN_INT(WEECHAT_CONFIG_OPTION_UNSET_ERROR);
    }
    
    if (NIL_P (option))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "config_unset_plugin");
        RUBY_RETURN_INT(WEECHAT_CONFIG_OPTION_UNSET_ERROR);
    }
    
    Check_Type (option, T_STRING);
    
    c_option = STR2CSTR (option);
    
    rc = script_api_config_unset_plugin (weechat_ruby_plugin,
                                         ruby_current_script,
                                         c_option);
    
    RUBY_RETURN_INT(rc);
}

/*
 * weechat_ruby_api_prefix: get a prefix, used for display
 */

static VALUE
weechat_ruby_api_prefix (VALUE class, VALUE prefix)
{
    char *c_prefix;
    const char *result;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "prefix");
        RUBY_RETURN_EMPTY;
    }
    
    c_prefix = NULL;
    
    if (NIL_P (prefix))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "prefix");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (prefix, T_STRING);
    
    c_prefix = STR2CSTR (prefix);
    
    result = weechat_prefix (c_prefix);
    
    RUBY_RETURN_STRING(result);
}

/*
 * weechat_ruby_api_color: get a color code, used for display
 */

static VALUE
weechat_ruby_api_color (VALUE class, VALUE color)
{
    char *c_color;
    const char *result;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "color");
        RUBY_RETURN_EMPTY;
    }
    
    c_color = NULL;
    
    if (NIL_P (color))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "color");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (color, T_STRING);
    
    c_color = STR2CSTR (color);
    
    result = weechat_color (c_color);
    
    RUBY_RETURN_STRING(result);
}

/*
 * weechat_ruby_api_print: print message in a buffer
 */

static VALUE
weechat_ruby_api_print (VALUE class, VALUE buffer, VALUE message)
{
    char *c_buffer, *c_message;
    
    /* make C compiler happy */
    (void) class;
    
    c_buffer = NULL;
    c_message = NULL;
    
    if (NIL_P (buffer) || NIL_P (message))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "print");
        RUBY_RETURN_ERROR;
    }
    
    Check_Type (buffer, T_STRING);
    Check_Type (message, T_STRING);
    
    c_buffer = STR2CSTR (buffer);
    c_message = STR2CSTR (message);
    
    script_api_printf (weechat_ruby_plugin,
                       ruby_current_script,
                       script_str2ptr (c_buffer),
                       "%s", c_message);
    
    RUBY_RETURN_OK;
}

/*
 * weechat_ruby_api_print_date_tags: print message in a buffer with optional
 *                                   date and tags
 */

static VALUE
weechat_ruby_api_print_date_tags (VALUE class, VALUE buffer, VALUE date,
                                  VALUE tags, VALUE message)
{
    char *c_buffer, *c_tags, *c_message;
    int c_date;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "print_date_tags");
        RUBY_RETURN_ERROR;
    }
    
    c_buffer = NULL;
    c_date = 0;
    c_tags = NULL;
    c_message = NULL;
    
    if (NIL_P (buffer) || NIL_P (date) || NIL_P (tags) || NIL_P (message))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "print_date_tags");
        RUBY_RETURN_ERROR;
    }
    
    Check_Type (buffer, T_STRING);
    Check_Type (date, T_FIXNUM);
    Check_Type (tags, T_STRING);
    Check_Type (message, T_STRING);
    
    c_buffer = STR2CSTR (buffer);
    c_date = FIX2INT (date);
    c_tags = STR2CSTR (tags);
    c_message = STR2CSTR (message);
    
    script_api_printf_date_tags (weechat_ruby_plugin,
                                 ruby_current_script,
                                 script_str2ptr (c_buffer),
                                 c_date,
                                 c_tags,
                                 "%s", c_message);
    
    RUBY_RETURN_OK;
}

/*
 * weechat_ruby_api_print_y: print message in a buffer with free content
 */

static VALUE
weechat_ruby_api_print_y (VALUE class, VALUE buffer, VALUE y, VALUE message)
{
    char *c_buffer, *c_message;
    int c_y;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "print_y");
        RUBY_RETURN_ERROR;
    }
    
    c_buffer = NULL;
    c_y = 0;
    c_message = NULL;
    
    if (NIL_P (buffer) || NIL_P (message))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "print_y");
        RUBY_RETURN_ERROR;
    }
    
    Check_Type (buffer, T_STRING);
    Check_Type (y, T_FIXNUM);
    Check_Type (message, T_STRING);
    
    c_buffer = STR2CSTR (buffer);
    c_y = FIX2INT (y);
    c_message = STR2CSTR (message);
    
    script_api_printf_y (weechat_ruby_plugin,
                         ruby_current_script,
                         script_str2ptr (c_buffer),
                         c_y,
                         "%s", c_message);
    
    RUBY_RETURN_OK;
}

/*
 * weechat_ruby_api_log_print: print message in WeeChat log file
 */

static VALUE
weechat_ruby_api_log_print (VALUE class, VALUE message)
{
    char *c_message;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "log_print");
        RUBY_RETURN_ERROR;
    }
    
    c_message = NULL;
    
    if (NIL_P (message))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "log_print");
        RUBY_RETURN_ERROR;
    }
    
    Check_Type (message, T_STRING);
    
    c_message = STR2CSTR (message);
    
    script_api_log_printf (weechat_ruby_plugin,
                           ruby_current_script,
                           "%s", c_message);
    
    RUBY_RETURN_OK;
}

/*
 * weechat_ruby_api_hook_command_cb: callback for command hooked
 */

int
weechat_ruby_api_hook_command_cb (void *data, struct t_gui_buffer *buffer,
                                  int argc, char **argv, char **argv_eol)
{
    struct t_script_callback *script_callback;
    char *ruby_argv[4], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    /* make C compiler happy */
    (void) argv;
    
    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        ruby_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        ruby_argv[1] = script_ptr2str (buffer);
        ruby_argv[2] = (argc > 1) ? argv_eol[1] : empty_arg;
        ruby_argv[3] = NULL;
        
        rc = (int *) weechat_ruby_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        ruby_argv);
        
        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (ruby_argv[1])
            free (ruby_argv[1]);
        
        return ret;
    }
    
    return WEECHAT_RC_ERROR;
}

/*
 * weechat_ruby_api_hook_command: hook a command
 */

static VALUE
weechat_ruby_api_hook_command (VALUE class, VALUE command, VALUE description,
                               VALUE args, VALUE args_description,
                               VALUE completion, VALUE function, VALUE data)
{
    char *c_command, *c_description, *c_args, *c_args_description;
    char *c_completion, *c_function, *c_data, *result;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "hook_command");
        RUBY_RETURN_EMPTY;
    }
    
    c_command = NULL;
    c_description = NULL;
    c_args = NULL;
    c_args_description = NULL;
    c_completion = NULL;
    c_function = NULL;
    c_data = NULL;
    
    if (NIL_P (command) || NIL_P (description) || NIL_P (args)
        || NIL_P (args_description) || NIL_P (completion) || NIL_P (function)
        || NIL_P (data))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "hook_command");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (command, T_STRING);
    Check_Type (description, T_STRING);
    Check_Type (args, T_STRING);
    Check_Type (args_description, T_STRING);
    Check_Type (completion, T_STRING);
    Check_Type (function, T_STRING);
    Check_Type (data, T_STRING);
    
    c_command = STR2CSTR (command);
    c_description = STR2CSTR (description);
    c_args = STR2CSTR (args);
    c_args_description = STR2CSTR (args_description);
    c_completion = STR2CSTR (completion);
    c_function = STR2CSTR (function);
    c_data = STR2CSTR (data);
    
    result = script_ptr2str (script_api_hook_command (weechat_ruby_plugin,
                                                      ruby_current_script,
                                                      c_command,
                                                      c_description,
                                                      c_args,
                                                      c_args_description,
                                                      c_completion,
                                                      &weechat_ruby_api_hook_command_cb,
                                                      c_function,
                                                      c_data));
    
    RUBY_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_hook_command_run_cb: callback for command_run hooked
 */

int
weechat_ruby_api_hook_command_run_cb (void *data, struct t_gui_buffer *buffer,
                                      const char *command)
{
    struct t_script_callback *script_callback;
    char *ruby_argv[4], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        ruby_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        ruby_argv[1] = script_ptr2str (buffer);
        ruby_argv[2] = (command) ? (char *)command : empty_arg;
        ruby_argv[3] = NULL;
        
        rc = (int *) weechat_ruby_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        ruby_argv);
        
        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (ruby_argv[1])
            free (ruby_argv[1]);
        
        return ret;
    }
    
    return WEECHAT_RC_ERROR;
}

/*
 * weechat_ruby_api_hook_command_run: hook a command_run
 */

static VALUE
weechat_ruby_api_hook_command_run (VALUE class, VALUE command, VALUE function,
                                   VALUE data)
{
    char *c_command, *c_function, *c_data, *result;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "hook_command_run");
        RUBY_RETURN_EMPTY;
    }
    
    c_command = NULL;
    c_function = NULL;
    c_data = NULL;
    
    if (NIL_P (command) || NIL_P (function) || NIL_P (data))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "hook_command_run");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (command, T_STRING);
    Check_Type (function, T_STRING);
    Check_Type (data, T_STRING);
    
    c_command = STR2CSTR (command);
    c_function = STR2CSTR (function);
    c_data = STR2CSTR (data);
    
    result = script_ptr2str (script_api_hook_command_run (weechat_ruby_plugin,
                                                          ruby_current_script,
                                                          c_command,
                                                          &weechat_ruby_api_hook_command_run_cb,
                                                          c_function,
                                                          c_data));
    
    RUBY_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_hook_timer_cb: callback for timer hooked
 */

int
weechat_ruby_api_hook_timer_cb (void *data, int remaining_calls)
{
    struct t_script_callback *script_callback;
    char *ruby_argv[3], str_remaining_calls[32], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback && script_callback->function && script_callback->function[0])
    {
        snprintf (str_remaining_calls, sizeof (str_remaining_calls),
                  "%d", remaining_calls);
        
        ruby_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        ruby_argv[1] = str_remaining_calls;
        ruby_argv[2] = NULL;
        
        rc = (int *) weechat_ruby_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        ruby_argv);
        
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
 * weechat_ruby_api_hook_timer: hook a timer
 */

static VALUE
weechat_ruby_api_hook_timer (VALUE class, VALUE interval, VALUE align_second,
                             VALUE max_calls, VALUE function, VALUE data)
{
    int c_interval, c_align_second, c_max_calls;
    char *c_function, *c_data, *result;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "hook_timer");
        RUBY_RETURN_EMPTY;
    }
    
    c_interval = 0;
    c_align_second = 0;
    c_max_calls = 0;
    c_function = NULL;
    c_data = NULL;
    
    if (NIL_P (interval) || NIL_P (align_second) || NIL_P (max_calls)
        || NIL_P (function) || NIL_P (data))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "hook_timer");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (interval, T_FIXNUM);
    Check_Type (align_second, T_FIXNUM);
    Check_Type (max_calls, T_FIXNUM);
    Check_Type (function, T_STRING);
    Check_Type (data, T_STRING);
    
    c_interval = FIX2INT (interval);
    c_align_second = FIX2INT (align_second);
    c_max_calls = FIX2INT (max_calls);
    c_function = STR2CSTR (function);
    c_data = STR2CSTR (data);
    
    result = script_ptr2str (script_api_hook_timer (weechat_ruby_plugin,
                                                    ruby_current_script,
                                                    c_interval,
                                                    c_align_second,
                                                    c_max_calls,
                                                    &weechat_ruby_api_hook_timer_cb,
                                                    c_function,
                                                    c_data));
    
    RUBY_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_hook_fd_cb: callback for fd hooked
 */

int
weechat_ruby_api_hook_fd_cb (void *data, int fd)
{
    struct t_script_callback *script_callback;
    char *ruby_argv[3], str_fd[32], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        snprintf (str_fd, sizeof (str_fd), "%d", fd);
        
        ruby_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        ruby_argv[1] = str_fd;
        ruby_argv[2] = NULL;
        
        rc = (int *) weechat_ruby_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        ruby_argv);
        
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
 * weechat_ruby_api_hook_fd: hook a fd
 */

static VALUE
weechat_ruby_api_hook_fd (VALUE class, VALUE fd, VALUE read, VALUE write,
                          VALUE exception, VALUE function, VALUE data)
{
    int c_fd, c_read, c_write, c_exception;
    char *c_function, *c_data, *result;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "hook_fd");
        RUBY_RETURN_EMPTY;
    }
    
    c_fd = 0;
    c_read = 0;
    c_write = 0;
    c_exception = 0;
    c_function = NULL;
    c_data = NULL;
    
    if (NIL_P (fd) || NIL_P (read) || NIL_P (write) || NIL_P (exception)
        || NIL_P (function) || NIL_P (data))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "hook_fd");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (fd, T_FIXNUM);
    Check_Type (read, T_FIXNUM);
    Check_Type (write, T_FIXNUM);
    Check_Type (exception, T_FIXNUM);
    Check_Type (function, T_STRING);
    Check_Type (data, T_STRING);
    
    c_fd = FIX2INT (fd);
    c_read = FIX2INT (read);
    c_write = FIX2INT (write);
    c_exception = FIX2INT (exception);
    c_function = STR2CSTR (function);
    c_data = STR2CSTR (data);
    
    result = script_ptr2str (script_api_hook_fd (weechat_ruby_plugin,
                                                 ruby_current_script,
                                                 c_fd,
                                                 c_read,
                                                 c_write,
                                                 c_exception,
                                                 &weechat_ruby_api_hook_fd_cb,
                                                 c_function,
                                                 c_data));
    
    RUBY_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_hook_process_cb: callback for process hooked
 */

int
weechat_ruby_api_hook_process_cb (void *data,
                                  const char *command, int return_code,
                                  const char *stdout, const char *stderr)
{
    struct t_script_callback *script_callback;
    char *ruby_argv[6], str_rc[32], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        snprintf (str_rc, sizeof (str_rc), "%d", return_code);
        
        ruby_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        ruby_argv[1] = (command) ? (char *)command : empty_arg;
        ruby_argv[2] = str_rc;
        ruby_argv[3] = (stdout) ? (char *)stdout : empty_arg;
        ruby_argv[4] = (stderr) ? (char *)stderr : empty_arg;
        ruby_argv[5] = NULL;
        
        rc = (int *) weechat_ruby_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        ruby_argv);
        
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
 * weechat_ruby_api_hook_process: hook a process
 */

static VALUE
weechat_ruby_api_hook_process (VALUE class, VALUE command, VALUE timeout,
                               VALUE function, VALUE data)
{
    char *c_command, *c_function, *c_data, *result;
    int c_timeout;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "hook_process");
        RUBY_RETURN_EMPTY;
    }
    
    c_command = NULL;
    c_timeout = 0;
    c_function = NULL;
    c_data = NULL;
    
    if (NIL_P (command) || NIL_P (timeout) || NIL_P (function) || NIL_P (data))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "hook_process");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (command, T_STRING);
    Check_Type (timeout, T_FIXNUM);
    Check_Type (function, T_STRING);
    Check_Type (data, T_STRING);
    
    c_command = STR2CSTR (command);
    c_timeout = FIX2INT (timeout);
    c_function = STR2CSTR (function);
    c_data = STR2CSTR (data);
    
    result = script_ptr2str (script_api_hook_process (weechat_ruby_plugin,
                                                      ruby_current_script,
                                                      c_command,
                                                      c_timeout,
                                                      &weechat_ruby_api_hook_process_cb,
                                                      c_function,
                                                      c_data));
    
    RUBY_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_hook_connect_cb: callback for connect hooked
 */

int
weechat_ruby_api_hook_connect_cb (void *data, int status,
                                  const char *error, const char *ip_address)
{
    struct t_script_callback *script_callback;
    char *ruby_argv[5], str_status[32], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        snprintf (str_status, sizeof (str_status), "%d", status);
        
        ruby_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        ruby_argv[1] = str_status;
        ruby_argv[2] = (ip_address) ? (char *)ip_address : empty_arg;
        ruby_argv[3] = (error) ? (char *)error : empty_arg;
        ruby_argv[4] = NULL;
        
        rc = (int *) weechat_ruby_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        ruby_argv);
        
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
 * weechat_ruby_api_hook_connect: hook a connection
 */

static VALUE
weechat_ruby_api_hook_connect (VALUE class, VALUE proxy, VALUE address,
                               VALUE port, VALUE sock, VALUE ipv6,
                               VALUE local_hostname, VALUE function,
                               VALUE data)
{
    char *c_proxy, *c_address, *c_local_hostname, *c_function, *c_data, *result;
    int c_port, c_sock, c_ipv6;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "hook_connect");
        RUBY_RETURN_EMPTY;
    }
    
    c_proxy = NULL;
    c_address = NULL;
    c_port = 0;
    c_sock = 0;
    c_ipv6 = 0;
    c_local_hostname = NULL;
    c_function = NULL;
    c_data = NULL;
    
    if (NIL_P (proxy) || NIL_P (address) || NIL_P (port) || NIL_P (sock)
        || NIL_P (ipv6) || NIL_P (local_hostname) || NIL_P (function)
        || NIL_P (data))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "hook_connect");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (proxy, T_STRING);
    Check_Type (address, T_STRING);
    Check_Type (port, T_FIXNUM);
    Check_Type (sock, T_FIXNUM);
    Check_Type (ipv6, T_FIXNUM);
    Check_Type (local_hostname, T_STRING);
    Check_Type (function, T_STRING);
    Check_Type (data, T_STRING);
    
    c_proxy = STR2CSTR (proxy);
    c_address = STR2CSTR (address);
    c_port = FIX2INT (port);
    c_sock = FIX2INT (sock);
    c_ipv6 = FIX2INT (ipv6);
    c_local_hostname = STR2CSTR (local_hostname);
    c_function = STR2CSTR (function);
    c_data = STR2CSTR (data);
    
    result = script_ptr2str (script_api_hook_connect (weechat_ruby_plugin,
                                                      ruby_current_script,
                                                      c_proxy,
                                                      c_address,
                                                      c_port,
                                                      c_sock,
                                                      c_ipv6,
                                                      NULL, /* gnutls session */
                                                      c_local_hostname,
                                                      &weechat_ruby_api_hook_connect_cb,
                                                      c_function,
                                                      c_data));
    
    RUBY_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_hook_print_cb: callback for print hooked
 */

int
weechat_ruby_api_hook_print_cb (void *data, struct t_gui_buffer *buffer,
                                time_t date,
                                int tags_count, const char **tags,
                                int displayed, int highlight,
                                const char *prefix, const char *message)
{
    struct t_script_callback *script_callback;
    char *ruby_argv[9], empty_arg[1] = { '\0' };
    static char timebuffer[64];
    int *rc, ret;
    
    /* make C compiler happy */
    (void) tags_count;
    
    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        snprintf (timebuffer, sizeof (timebuffer) - 1, "%ld", (long int)date);
        
        ruby_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        ruby_argv[1] = script_ptr2str (buffer);
        ruby_argv[2] = timebuffer;
        ruby_argv[3] = weechat_string_build_with_exploded (tags, ",");
        if (!ruby_argv[3])
            ruby_argv[3] = strdup ("");
        ruby_argv[4] = (displayed) ? strdup ("1") : strdup ("0");
        ruby_argv[5] = (highlight) ? strdup ("1") : strdup ("0");
        ruby_argv[6] = (prefix) ? (char *)prefix : empty_arg;
        ruby_argv[7] = (message) ? (char *)message : empty_arg;
        ruby_argv[8] = NULL;
        
        rc = (int *) weechat_ruby_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        ruby_argv);
        
        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (ruby_argv[1])
            free (ruby_argv[1]);
        if (ruby_argv[3])
            free (ruby_argv[3]);
        if (ruby_argv[4])
            free (ruby_argv[4]);
        if (ruby_argv[5])
            free (ruby_argv[5]);
        
        return ret;
    }
    
    return WEECHAT_RC_ERROR;
}

/*
 * weechat_ruby_api_hook_print: hook a print
 */

static VALUE
weechat_ruby_api_hook_print (VALUE class, VALUE buffer, VALUE tags,
                             VALUE message, VALUE strip_colors, VALUE function,
                             VALUE data)
{
    char *c_buffer, *c_tags, *c_message, *c_function, *c_data, *result;
    int c_strip_colors;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "hook_print");
        RUBY_RETURN_EMPTY;
    }
    
    c_buffer = NULL;
    c_tags = NULL;
    c_message = NULL;
    c_strip_colors = 0;
    c_function = NULL;
    c_data = NULL;
    
    if (NIL_P (buffer) || NIL_P (tags) || NIL_P (message)
        || NIL_P (strip_colors) || NIL_P (function) || NIL_P (data))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "hook_print");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (buffer, T_STRING);
    Check_Type (tags, T_STRING);
    Check_Type (message, T_STRING);
    Check_Type (strip_colors, T_FIXNUM);
    Check_Type (function, T_STRING);
    Check_Type (data, T_STRING);
    
    c_buffer = STR2CSTR (buffer);
    c_tags = STR2CSTR (tags);
    c_message = STR2CSTR (message);
    c_strip_colors = FIX2INT (strip_colors);
    c_function = STR2CSTR (function);
    c_data = STR2CSTR (data);
    
    result = script_ptr2str (script_api_hook_print (weechat_ruby_plugin,
                                                    ruby_current_script,
                                                    script_str2ptr (c_buffer),
                                                    c_tags,
                                                    c_message,
                                                    c_strip_colors,
                                                    &weechat_ruby_api_hook_print_cb,
                                                    c_function,
                                                    c_data));
    
    RUBY_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_hook_signal_cb: callback for signal hooked
 */

int
weechat_ruby_api_hook_signal_cb (void *data, const char *signal, const char *type_data,
                                 void *signal_data)
{
    struct t_script_callback *script_callback;
    char *ruby_argv[4], empty_arg[1] = { '\0' };
    static char value_str[64];
    int *rc, ret, free_needed;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback && script_callback->function && script_callback->function[0])
    {
        ruby_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        ruby_argv[1] = (signal) ? (char *)signal : empty_arg;
        free_needed = 0;
        if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_STRING) == 0)
        {
            ruby_argv[2] = (signal_data) ? (char *)signal_data : empty_arg;
        }
        else if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_INT) == 0)
        {
            snprintf (value_str, sizeof (value_str) - 1,
                      "%d", *((int *)signal_data));
            ruby_argv[2] = value_str;
        }
        else if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_POINTER) == 0)
        {
            ruby_argv[2] = script_ptr2str (signal_data);
            free_needed = 1;
        }
        else
            ruby_argv[2] = empty_arg;
        ruby_argv[3] = NULL;
        
        rc = (int *) weechat_ruby_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        ruby_argv);
        
        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (free_needed && ruby_argv[2])
            free (ruby_argv[2]);
        
        return ret;
    }
    
    return WEECHAT_RC_ERROR;
}

/*
 * weechat_ruby_api_hook_signal: hook a signal
 */

static VALUE
weechat_ruby_api_hook_signal (VALUE class, VALUE signal, VALUE function,
                              VALUE data)
{
    char *c_signal, *c_function, *c_data, *result;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "hook_signal");
        RUBY_RETURN_EMPTY;
    }
    
    c_signal = NULL;
    c_function = NULL;
    c_data = NULL;
    
    if (NIL_P (signal) || NIL_P (function) || NIL_P (data))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "hook_signal");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (signal, T_STRING);
    Check_Type (function, T_STRING);
    Check_Type (data, T_STRING);
    
    c_signal = STR2CSTR (signal);
    c_function = STR2CSTR (function);
    c_data = STR2CSTR (data);
    
    result = script_ptr2str (script_api_hook_signal (weechat_ruby_plugin,
                                                     ruby_current_script,
                                                     c_signal,
                                                     &weechat_ruby_api_hook_signal_cb,
                                                     c_function,
                                                     c_data));
    
    RUBY_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_hook_signal_send: send a signal
 */

static VALUE
weechat_ruby_api_hook_signal_send (VALUE class, VALUE signal, VALUE type_data,
                                   VALUE signal_data)
{
    char *c_signal, *c_type_data, *c_signal_data;
    int number;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "hook_signal_send");
        RUBY_RETURN_ERROR;
    }
    
    c_signal = NULL;
    c_type_data = NULL;
    c_signal_data = NULL;
    
    if (NIL_P (signal) || NIL_P (type_data) || NIL_P (signal_data))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "hook_signal_send");
        RUBY_RETURN_ERROR;
    }
    
    Check_Type (signal, T_STRING);
    Check_Type (type_data, T_STRING);
    
    c_signal = STR2CSTR (signal);
    c_type_data = STR2CSTR (type_data);
    
    if (strcmp (c_type_data, WEECHAT_HOOK_SIGNAL_STRING) == 0)
    {
        Check_Type (signal_data, T_STRING);
        c_signal_data = STR2CSTR (signal_data);
        weechat_hook_signal_send (c_signal, c_type_data, c_signal_data);
        RUBY_RETURN_OK;
    }
    else if (strcmp (c_type_data, WEECHAT_HOOK_SIGNAL_INT) == 0)
    {
        Check_Type (signal_data, T_STRING);
        number = FIX2INT (signal_data);
        weechat_hook_signal_send (c_signal, c_type_data, &number);
        RUBY_RETURN_OK;
    }
    else if (strcmp (c_type_data, WEECHAT_HOOK_SIGNAL_POINTER) == 0)
    {
        Check_Type (signal_data, T_STRING);
        c_signal_data = STR2CSTR (signal_data);
        weechat_hook_signal_send (c_signal, c_type_data,
                                  script_str2ptr (c_signal_data));
        RUBY_RETURN_OK;
    }
    
    RUBY_RETURN_ERROR;
}

/*
 * weechat_ruby_api_hook_config_cb: callback for config option hooked
 */

int
weechat_ruby_api_hook_config_cb (void *data, const char *option, const char *value)
{
    struct t_script_callback *script_callback;
    char *ruby_argv[4], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback && script_callback->function && script_callback->function[0])
    {
        ruby_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        ruby_argv[1] = (option) ? (char *)option : empty_arg;
        ruby_argv[2] = (value) ? (char *)value : empty_arg;
        ruby_argv[3] = NULL;
        
        rc = (int *) weechat_ruby_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        ruby_argv);
        
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
 * weechat_ruby_api_hook_config: hook a config option
 */

static VALUE
weechat_ruby_api_hook_config (VALUE class, VALUE option, VALUE function,
                              VALUE data)
{
    char *c_option, *c_function, *c_data, *result;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "hook_config");
        RUBY_RETURN_EMPTY;
    }
    
    c_option = NULL;
    c_function = NULL;
    c_data = NULL;
    
    if (NIL_P (option) || NIL_P (function) || NIL_P (data))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "hook_config");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (option, T_STRING);
    Check_Type (function, T_STRING);
    Check_Type (data, T_STRING);
    
    c_option = STR2CSTR (option);
    c_function = STR2CSTR (function);
    c_data = STR2CSTR (data);
    
    result = script_ptr2str (script_api_hook_config (weechat_ruby_plugin,
                                                     ruby_current_script,
                                                     c_option,
                                                     &weechat_ruby_api_hook_config_cb,
                                                     c_function,
                                                     c_data));
    
    RUBY_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_hook_completion_cb: callback for completion hooked
 */

int
weechat_ruby_api_hook_completion_cb (void *data, const char *completion_item,
                                     struct t_gui_buffer *buffer,
                                     struct t_gui_completion *completion)
{
    struct t_script_callback *script_callback;
    char *ruby_argv[5], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        ruby_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        ruby_argv[1] = (completion_item) ? (char *)completion_item : empty_arg;
        ruby_argv[2] = script_ptr2str (buffer);
        ruby_argv[3] = script_ptr2str (completion);
        ruby_argv[4] = NULL;
        
        rc = (int *) weechat_ruby_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        ruby_argv);
        
        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (ruby_argv[2])
            free (ruby_argv[2]);
        if (ruby_argv[3])
            free (ruby_argv[3]);
        
        return ret;
    }
    
    return WEECHAT_RC_ERROR;
}

/*
 * weechat_ruby_api_hook_completion: hook a completion
 */

static VALUE
weechat_ruby_api_hook_completion (VALUE class, VALUE completion,
                                  VALUE description, VALUE function,
                                  VALUE data)
{
    char *c_completion, *c_description, *c_function, *c_data, *result;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "hook_completion");
        RUBY_RETURN_EMPTY;
    }
    
    c_completion = NULL;
    c_description = NULL;
    c_function = NULL;
    c_data = NULL;
    
    if (NIL_P (completion) || NIL_P (description) || NIL_P (function)
        || NIL_P (data))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "hook_completion");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (completion, T_STRING);
    Check_Type (description, T_STRING);
    Check_Type (function, T_STRING);
    Check_Type (data, T_STRING);
    
    c_completion = STR2CSTR (completion);
    c_description = STR2CSTR (description);
    c_function = STR2CSTR (function);
    c_data = STR2CSTR (data);
    
    result = script_ptr2str (script_api_hook_completion (weechat_ruby_plugin,
                                                         ruby_current_script,
                                                         c_completion,
                                                         c_description,
                                                         &weechat_ruby_api_hook_completion_cb,
                                                         c_function,
                                                         c_data));
    
    RUBY_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_hook_completion_list_add: add a word to list for a completion
 */

static VALUE
weechat_ruby_api_hook_completion_list_add (VALUE class, VALUE completion,
                                           VALUE word, VALUE nick_completion,
                                           VALUE where)
{
    char *c_completion, *c_word, *c_where;
    int c_nick_completion;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "hook_completion_list_add");
        RUBY_RETURN_ERROR;
    }
    
    c_completion = NULL;
    c_word = NULL;
    c_nick_completion = 0;
    c_where = NULL;
    
    if (NIL_P (completion) || NIL_P (word) || NIL_P (nick_completion)
        || NIL_P (where))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "hook_completion_list_add");
        RUBY_RETURN_ERROR;
    }
    
    Check_Type (completion, T_STRING);
    Check_Type (word, T_STRING);
    Check_Type (nick_completion, T_FIXNUM);
    Check_Type (where, T_STRING);
    
    c_completion = STR2CSTR (completion);
    c_word = STR2CSTR (word);
    c_nick_completion = FIX2INT (nick_completion);
    c_where = STR2CSTR (where);
    
    weechat_hook_completion_list_add (script_str2ptr (c_completion),
                                      c_word,
                                      c_nick_completion,
                                      c_where);
    
    RUBY_RETURN_OK;
}

/*
 * weechat_ruby_api_hook_modifier_cb: callback for modifier hooked
 */

char *
weechat_ruby_api_hook_modifier_cb (void *data, const char *modifier,
                                   const char *modifier_data,  const char *string)
{
    struct t_script_callback *script_callback;
    char *ruby_argv[5], empty_arg[1] = { '\0' };
    
    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        ruby_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        ruby_argv[1] = (modifier) ? (char *)modifier : empty_arg;
        ruby_argv[2] = (modifier_data) ? (char *)modifier_data : empty_arg;
        ruby_argv[3] = (string) ? (char *)string : empty_arg;
        ruby_argv[4] = NULL;
        
        return (char *)weechat_ruby_exec (script_callback->script,
                                          WEECHAT_SCRIPT_EXEC_STRING,
                                          script_callback->function,
                                          ruby_argv);
    }
    
    return NULL;
}

/*
 * weechat_ruby_api_hook_modifier: hook a modifier
 */

static VALUE
weechat_ruby_api_hook_modifier (VALUE class, VALUE modifier, VALUE function,
                                VALUE data)
{
    char *c_modifier, *c_function, *c_data, *result;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "hook_modifier");
        RUBY_RETURN_EMPTY;
    }
    
    c_modifier = NULL;
    c_function = NULL;
    c_data = NULL;
    
    if (NIL_P (modifier) || NIL_P (function) || NIL_P (data))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "hook_modifier");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (modifier, T_STRING);
    Check_Type (function, T_STRING);
    Check_Type (data, T_STRING);
    
    c_modifier = STR2CSTR (modifier);
    c_function = STR2CSTR (function);
    c_data = STR2CSTR (data);
    
    result = script_ptr2str (script_api_hook_modifier (weechat_ruby_plugin,
                                                       ruby_current_script,
                                                       c_modifier,
                                                       &weechat_ruby_api_hook_modifier_cb,
                                                       c_function,
                                                       c_data));
    
    RUBY_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_hook_modifier_exec: execute a modifier hook
 */

static VALUE
weechat_ruby_api_hook_modifier_exec (VALUE class, VALUE modifier,
                                     VALUE modifier_data, VALUE string)
{
    char *c_modifier, *c_modifier_data, *c_string, *result;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "hook_modifier_exec");
        RUBY_RETURN_EMPTY;
    }
    
    c_modifier = NULL;
    c_modifier_data = NULL;
    c_string = NULL;
    
    if (NIL_P (modifier) || NIL_P (modifier_data) || NIL_P (string))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "hook_modifier_exec");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (modifier, T_STRING);
    Check_Type (modifier_data, T_STRING);
    Check_Type (string, T_STRING);
    
    c_modifier = STR2CSTR (modifier);
    c_modifier_data = STR2CSTR (modifier_data);
    c_string = STR2CSTR (string);

    result = weechat_hook_modifier_exec (c_modifier, c_modifier_data, c_string);
    
    RUBY_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_hook_info_cb: callback for info hooked
 */

const char *
weechat_ruby_api_hook_info_cb (void *data, const char *info_name,
                               const char *arguments)
{
    struct t_script_callback *script_callback;
    char *ruby_argv[4], empty_arg[1] = { '\0' };
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback && script_callback->function && script_callback->function[0])
    {
        ruby_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        ruby_argv[1] = (info_name) ? (char *)info_name : empty_arg;
        ruby_argv[2] = (arguments) ? (char *)arguments : empty_arg;
        ruby_argv[3] = NULL;
        
        return (const char *)weechat_ruby_exec (script_callback->script,
                                                WEECHAT_SCRIPT_EXEC_STRING,
                                                script_callback->function,
                                                ruby_argv);
    }
    
    return NULL;
}

/*
 * weechat_ruby_api_hook_info: hook an info
 */

static VALUE
weechat_ruby_api_hook_info (VALUE class, VALUE info_name, VALUE description,
                            VALUE function, VALUE data)
{
    char *c_info_name, *c_description, *c_function, *c_data, *result;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "hook_info");
        RUBY_RETURN_EMPTY;
    }
    
    c_info_name = NULL;
    c_description = NULL;
    c_function = NULL;
    c_data = NULL;
    
    if (NIL_P (info_name) || NIL_P (description) || NIL_P (function)
        || NIL_P (data))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "hook_info");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (info_name, T_STRING);
    Check_Type (description, T_STRING);
    Check_Type (function, T_STRING);
    Check_Type (data, T_STRING);
    
    c_info_name = STR2CSTR (info_name);
    c_description = STR2CSTR (description);
    c_function = STR2CSTR (function);
    c_data = STR2CSTR (data);
    
    result = script_ptr2str (script_api_hook_info (weechat_ruby_plugin,
                                                   ruby_current_script,
                                                   c_info_name,
                                                   c_description,
                                                   &weechat_ruby_api_hook_info_cb,
                                                   c_function,
                                                   c_data));
    
    RUBY_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_hook_infolist_cb: callback for infolist hooked
 */

struct t_infolist *
weechat_ruby_api_hook_infolist_cb (void *data, const char *infolist_name,
                                   void *pointer, const char *arguments)
{
    struct t_script_callback *script_callback;
    char *ruby_argv[5], empty_arg[1] = { '\0' };
    struct t_infolist *result;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback && script_callback->function && script_callback->function[0])
    {
        ruby_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        ruby_argv[1] = (infolist_name) ? (char *)infolist_name : empty_arg;
        ruby_argv[2] = script_ptr2str (pointer);
        ruby_argv[3] = (arguments) ? (char *)arguments : empty_arg;
        ruby_argv[4] = NULL;
        
        result = (struct t_infolist *)weechat_ruby_exec (script_callback->script,
                                                         WEECHAT_SCRIPT_EXEC_STRING,
                                                         script_callback->function,
                                                         ruby_argv);
        
        if (ruby_argv[2])
            free (ruby_argv[2]);
        
        return result;
    }
    
    return NULL;
}

/*
 * weechat_ruby_api_hook_infolist: hook an infolist
 */

static VALUE
weechat_ruby_api_hook_infolist (VALUE class, VALUE infolist_name,
                                VALUE description, VALUE function,
                                VALUE data)
{
    char *c_infolist_name, *c_description, *c_function, *c_data, *result;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "hook_infolist");
        RUBY_RETURN_EMPTY;
    }
    
    c_infolist_name = NULL;
    c_description = NULL;
    c_function = NULL;
    c_data = NULL;
    
    if (NIL_P (infolist_name) || NIL_P (description) || NIL_P (function)
        || NIL_P (data))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "hook_infolist");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (infolist_name, T_STRING);
    Check_Type (description, T_STRING);
    Check_Type (function, T_STRING);
    Check_Type (data, T_STRING);
    
    c_infolist_name = STR2CSTR (infolist_name);
    c_description = STR2CSTR (description);
    c_function = STR2CSTR (function);
    c_data = STR2CSTR (data);
    
    result = script_ptr2str (script_api_hook_infolist (weechat_ruby_plugin,
                                                       ruby_current_script,
                                                       c_infolist_name,
                                                       c_description,
                                                       &weechat_ruby_api_hook_infolist_cb,
                                                       c_function,
                                                       c_data));
    
    RUBY_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_unhook: unhook something
 */

static VALUE
weechat_ruby_api_unhook (VALUE class, VALUE hook)
{
    char *c_hook;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "unhook");
        RUBY_RETURN_ERROR;
    }
    
    c_hook = NULL;
    
    if (NIL_P (hook))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "unhook");
        RUBY_RETURN_ERROR;
    }
    
    Check_Type (hook, T_STRING);
    
    c_hook = STR2CSTR (hook);
    
    script_api_unhook (weechat_ruby_plugin,
                       ruby_current_script,
                       script_str2ptr (c_hook));
    
    RUBY_RETURN_OK;
}

/*
 * weechat_ruby_api_unhook_all: unhook all for script
 */

static VALUE
weechat_ruby_api_unhook_all (VALUE class)
{
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "unhook_all");
        RUBY_RETURN_ERROR;
    }
    
    script_api_unhook_all (ruby_current_script);
    
    RUBY_RETURN_OK;
}

/*
 * weechat_ruby_api_buffer_input_data_cb: callback for input data in a buffer
 */

int
weechat_ruby_api_buffer_input_data_cb (void *data, struct t_gui_buffer *buffer,
                                       const char *input_data)
{
    struct t_script_callback *script_callback;
    char *ruby_argv[4], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback && script_callback->function && script_callback->function[0])
    {
        ruby_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        ruby_argv[1] = script_ptr2str (buffer);
        ruby_argv[2] = (input_data) ? (char *)input_data : empty_arg;
        ruby_argv[3] = NULL;
        
        rc = (int *) weechat_ruby_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        ruby_argv);
        
        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (ruby_argv[1])
            free (ruby_argv[1]);
        
        return ret;
    }
    
    return WEECHAT_RC_ERROR;
}

/*
 * weechat_ruby_api_buffer_close_cb: callback for closed buffer
 */

int
weechat_ruby_api_buffer_close_cb (void *data, struct t_gui_buffer *buffer)
{
    struct t_script_callback *script_callback;
    char *ruby_argv[3], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        ruby_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        ruby_argv[1] = script_ptr2str (buffer);
        ruby_argv[2] = NULL;
        
        rc = (int *) weechat_ruby_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        ruby_argv);
        
        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (ruby_argv[1])
            free (ruby_argv[1]);
        
        return ret;
    }
    
    return WEECHAT_RC_ERROR;
}

/*
 * weechat_ruby_api_buffer_new: create a new buffer
 */

static VALUE
weechat_ruby_api_buffer_new (VALUE class, VALUE name, VALUE function_input,
                             VALUE data_input, VALUE function_close,
                             VALUE data_close)
{
    char *c_name, *c_function_input, *c_data_input, *c_function_close;
    char *c_data_close, *result;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "buffer_new");
        RUBY_RETURN_EMPTY;
    }
    
    c_name = NULL;
    c_function_input = NULL;
    c_data_input = NULL;
    c_function_close = NULL;
    c_data_close = NULL;
    
    if (NIL_P (name) || NIL_P (function_input) || NIL_P (data_input)
        || NIL_P (function_close) || NIL_P (data_close))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "buffer_new");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (name, T_STRING);
    Check_Type (function_input, T_STRING);
    Check_Type (data_input, T_STRING);
    Check_Type (function_close, T_STRING);
    Check_Type (data_close, T_STRING);
    
    c_name = STR2CSTR (name);
    c_function_input = STR2CSTR (function_input);
    c_data_input = STR2CSTR (data_input);
    c_function_close = STR2CSTR (function_close);
    c_data_close = STR2CSTR (data_close);
    
    result = script_ptr2str (script_api_buffer_new (weechat_ruby_plugin,
                                                    ruby_current_script,
                                                    c_name,
                                                    &weechat_ruby_api_buffer_input_data_cb,
                                                    c_function_input,
                                                    c_data_input,
                                                    &weechat_ruby_api_buffer_close_cb,
                                                    c_function_close,
                                                    c_data_close));
    
    RUBY_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_buffer_search: search a buffer
 */

static VALUE
weechat_ruby_api_buffer_search (VALUE class, VALUE plugin, VALUE name)
{
    char *c_plugin, *c_name, *result;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "buffer_search");
        RUBY_RETURN_EMPTY;
    }

    c_plugin = NULL;
    c_name = NULL;
    
    if (NIL_P (plugin) || NIL_P (name))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "buffer_search");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (plugin, T_STRING);
    Check_Type (name, T_STRING);
    
    c_plugin = STR2CSTR (plugin);
    c_name = STR2CSTR (name);
    
    result = script_ptr2str (weechat_buffer_search (c_plugin, c_name));
    
    RUBY_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_current_buffer: get current buffer
 */

static VALUE
weechat_ruby_api_current_buffer (VALUE class)
{
    char *result;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "current_buffer");
        RUBY_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_current_buffer ());
    
    RUBY_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_buffer_clear: clear a buffer
 */

static VALUE
weechat_ruby_api_buffer_clear (VALUE class, VALUE buffer)
{
    char *c_buffer;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "buffer_clear");
        RUBY_RETURN_ERROR;
    }
    
    c_buffer = NULL;
    
    if (NIL_P (buffer))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "buffer_clear");
        RUBY_RETURN_ERROR;
    }
    
    Check_Type (buffer, T_STRING);
    
    c_buffer = STR2CSTR (buffer);
    
    weechat_buffer_clear (script_str2ptr (c_buffer));
    
    RUBY_RETURN_OK;
}

/*
 * weechat_ruby_api_buffer_close: close a buffer
 */

static VALUE
weechat_ruby_api_buffer_close (VALUE class, VALUE buffer)
{
    char *c_buffer;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "buffer_close");
        RUBY_RETURN_ERROR;
    }
    
    c_buffer = NULL;
    
    if (NIL_P (buffer))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "buffer_close");
        RUBY_RETURN_ERROR;
    }
    
    Check_Type (buffer, T_STRING);
    
    c_buffer = STR2CSTR (buffer);
    
    script_api_buffer_close (weechat_ruby_plugin,
                             ruby_current_script,
                             script_str2ptr (c_buffer));
    
    RUBY_RETURN_OK;
}

/*
 * weechat_ruby_api_buffer_get_integer: get a buffer property as integer
 */

static VALUE
weechat_ruby_api_buffer_get_integer (VALUE class, VALUE buffer, VALUE property)
{
    char *c_buffer, *c_property;
    int value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "buffer_get_integer");
        RUBY_RETURN_INT(-1);
    }
    
    if (NIL_P (buffer) || NIL_P (property))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "buffer_get_integer");
        RUBY_RETURN_INT(-1);
    }
    
    Check_Type (buffer, T_STRING);
    Check_Type (property, T_STRING);
    
    c_buffer = STR2CSTR (buffer);
    c_property = STR2CSTR (property);

    value = weechat_buffer_get_integer (script_str2ptr (c_buffer),
                                        c_property);
    
    RUBY_RETURN_INT(value);
}

/*
 * weechat_ruby_api_buffer_get_string: get a buffer property as string
 */

static VALUE
weechat_ruby_api_buffer_get_string (VALUE class, VALUE buffer, VALUE property)
{
    char *c_buffer, *c_property;
    const char *result;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "buffer_get_string");
        RUBY_RETURN_EMPTY;
    }
    
    if (NIL_P (buffer) || NIL_P (property))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "buffer_get_string");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (buffer, T_STRING);
    Check_Type (property, T_STRING);
    
    c_buffer = STR2CSTR (buffer);
    c_property = STR2CSTR (property);
    
    result = weechat_buffer_get_string (script_str2ptr (c_buffer),
                                       c_property);
    
    RUBY_RETURN_STRING(result);
}

/*
 * weechat_ruby_api_buffer_get_pointer: get a buffer property as pointer
 */

static VALUE
weechat_ruby_api_buffer_get_pointer (VALUE class, VALUE buffer, VALUE property)
{
    char *c_buffer, *c_property, *result;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "buffer_get_pointer");
        RUBY_RETURN_EMPTY;
    }
    
    if (NIL_P (buffer) || NIL_P (property))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "buffer_get_pointer");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (buffer, T_STRING);
    Check_Type (property, T_STRING);
    
    c_buffer = STR2CSTR (buffer);
    c_property = STR2CSTR (property);
    
    result = script_ptr2str (weechat_buffer_get_pointer (script_str2ptr (c_buffer),
                                                         c_property));
    
    RUBY_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_buffer_set: set a buffer property
 */

static VALUE
weechat_ruby_api_buffer_set (VALUE class, VALUE buffer, VALUE property,
                             VALUE value)
{
    char *c_buffer, *c_property, *c_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "buffer_set");
        RUBY_RETURN_ERROR;
    }
    
    if (NIL_P (buffer) || NIL_P (property) || NIL_P (value))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "buffer_set");
        RUBY_RETURN_ERROR;
    }
    
    Check_Type (buffer, T_STRING);
    Check_Type (property, T_STRING);
    Check_Type (value, T_STRING);
    
    c_buffer = STR2CSTR (buffer);
    c_property = STR2CSTR (property);
    c_value = STR2CSTR (value);
    
    weechat_buffer_set (script_str2ptr (c_buffer),
                        c_property,
                        c_value);
    
    RUBY_RETURN_OK;
}

/*
 * weechat_ruby_api_current_window: get current window
 */

static VALUE
weechat_ruby_api_current_window (VALUE class)
{
    char *result;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "current_window");
        RUBY_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_current_window ());
    
    RUBY_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_window_get_integer: get a window property as integer
 */

static VALUE
weechat_ruby_api_window_get_integer (VALUE class, VALUE window, VALUE property)
{
    char *c_window, *c_property;
    int value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "window_get_integer");
        RUBY_RETURN_INT(-1);
    }
    
    if (NIL_P (window) || NIL_P (property))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "window_get_integer");
        RUBY_RETURN_INT(-1);
    }
    
    Check_Type (window, T_STRING);
    Check_Type (property, T_STRING);
    
    c_window = STR2CSTR (window);
    c_property = STR2CSTR (property);

    value = weechat_window_get_integer (script_str2ptr (c_window),
                                        c_property);
    
    RUBY_RETURN_INT(value);
}

/*
 * weechat_ruby_api_window_get_string: get a window property as string
 */

static VALUE
weechat_ruby_api_window_get_string (VALUE class, VALUE window, VALUE property)
{
    char *c_window, *c_property;
    const char *result;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "window_get_string");
        RUBY_RETURN_EMPTY;
    }
    
    if (NIL_P (window) || NIL_P (property))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "window_get_string");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (window, T_STRING);
    Check_Type (property, T_STRING);
    
    c_window = STR2CSTR (window);
    c_property = STR2CSTR (property);
    
    result = weechat_window_get_string (script_str2ptr (c_window),
                                       c_property);
    
    RUBY_RETURN_STRING(result);
}

/*
 * weechat_ruby_api_window_get_pointer: get a window property as pointer
 */

static VALUE
weechat_ruby_api_window_get_pointer (VALUE class, VALUE window, VALUE property)
{
    char *c_window, *c_property, *result;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "window_get_pointer");
        RUBY_RETURN_EMPTY;
    }
    
    if (NIL_P (window) || NIL_P (property))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "window_get_pointer");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (window, T_STRING);
    Check_Type (property, T_STRING);
    
    c_window = STR2CSTR (window);
    c_property = STR2CSTR (property);
    
    result = script_ptr2str (weechat_window_get_pointer (script_str2ptr (c_window),
                                                         c_property));
    
    RUBY_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_window_set_title: set window title
 */

static VALUE
weechat_ruby_api_window_set_title (VALUE class, VALUE title)
{
    char *c_title;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "window_set_title");
        RUBY_RETURN_ERROR;
    }
    
    if (NIL_P (title))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "window_set_title");
        RUBY_RETURN_ERROR;
    }
    
    Check_Type (title, T_STRING);
    
    c_title = STR2CSTR (title);
    
    weechat_window_set_title (c_title);
    
    RUBY_RETURN_OK;
}

/*
 * weechat_ruby_api_nicklist_add_group: add a group in nicklist
 */

static VALUE
weechat_ruby_api_nicklist_add_group (VALUE class, VALUE buffer,
                                     VALUE parent_group, VALUE name,
                                     VALUE color, VALUE visible)
{
    char *c_buffer, *c_parent_group, *c_name, *c_color, *result;
    int c_visible;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "nicklist_add_group");
        RUBY_RETURN_EMPTY;
    }
    
    c_buffer = NULL;
    c_parent_group = NULL;
    c_name = NULL;
    c_color = NULL;
    c_visible = 0;
    
    if (NIL_P (buffer) || NIL_P (parent_group) || NIL_P (name) || NIL_P (color)
        || NIL_P (visible))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "nicklist_add_group");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (buffer, T_STRING);
    Check_Type (parent_group, T_STRING);
    Check_Type (name, T_STRING);
    Check_Type (color, T_STRING);
    Check_Type (visible, T_FIXNUM);
    
    c_buffer = STR2CSTR (buffer);
    c_parent_group = STR2CSTR (parent_group);
    c_name = STR2CSTR (name);
    c_color = STR2CSTR (color);
    c_visible = FIX2INT (visible);
    
    result = script_ptr2str (weechat_nicklist_add_group (script_str2ptr (c_buffer),
                                                         script_str2ptr (c_parent_group),
                                                         c_name,
                                                         c_color,
                                                         c_visible));
    
    RUBY_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_nicklist_search_group: search a group in nicklist
 */

static VALUE
weechat_ruby_api_nicklist_search_group (VALUE class, VALUE buffer,
                                        VALUE from_group, VALUE name)
{
    char *c_buffer, *c_from_group, *c_name, *result;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "nicklist_search_group");
        RUBY_RETURN_EMPTY;
    }
    
    c_buffer = NULL;
    c_from_group = NULL;
    c_name = NULL;
    
    if (NIL_P (buffer) || NIL_P (from_group) || NIL_P (name))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "nicklist_search_group");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (buffer, T_STRING);
    Check_Type (from_group, T_STRING);
    Check_Type (name, T_STRING);
    
    c_buffer = STR2CSTR (buffer);
    c_from_group = STR2CSTR (from_group);
    c_name = STR2CSTR (name);
    
    result = script_ptr2str (weechat_nicklist_search_group (script_str2ptr (c_buffer),
                                                            script_str2ptr (c_from_group),
                                                            c_name));
    
    RUBY_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_nicklist_add_nick: add a nick in nicklist
 */

static VALUE
weechat_ruby_api_nicklist_add_nick (VALUE class, VALUE buffer, VALUE group,
                                    VALUE name, VALUE color, VALUE prefix,
                                    VALUE prefix_color, VALUE visible)
{
    char *c_buffer, *c_group, *c_name, *c_color, *c_prefix, *c_prefix_color;
    char *result;
    int c_visible;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "nicklist_add_nick");
        RUBY_RETURN_EMPTY;
    }
    
    c_buffer = NULL;
    c_group = NULL;
    c_name = NULL;
    c_color = NULL;
    c_prefix = NULL;
    c_prefix_color = NULL;
    c_visible = 0;
    
    if (NIL_P (buffer) || NIL_P (group) || NIL_P (name) || NIL_P (color)
        || NIL_P (prefix) || NIL_P (prefix_color) || NIL_P (visible))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "nicklist_add_nick");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (buffer, T_STRING);
    Check_Type (group, T_STRING);
    Check_Type (name, T_STRING);
    Check_Type (color, T_STRING);
    Check_Type (prefix, T_STRING);
    Check_Type (prefix_color, T_STRING);
    Check_Type (visible, T_FIXNUM);
    
    c_buffer = STR2CSTR (buffer);
    c_group = STR2CSTR (group);
    c_name = STR2CSTR (name);
    c_color = STR2CSTR (color);
    c_prefix = STR2CSTR (prefix);
    c_prefix_color = STR2CSTR (prefix_color);
    c_visible = FIX2INT (visible);
    
    result = script_ptr2str (weechat_nicklist_add_nick (script_str2ptr (c_buffer),
                                                        script_str2ptr (c_group),
                                                        c_name,
                                                        c_color,
                                                        c_prefix,
                                                        c_prefix_color,
                                                        c_visible));
    
    RUBY_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_nicklist_search_nick: search a nick in nicklist
 */

static VALUE
weechat_ruby_api_nicklist_search_nick (VALUE class, VALUE buffer,
                                       VALUE from_group, VALUE name)
{
    char *c_buffer, *c_from_group, *c_name, *result;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "nicklist_search_nick");
        RUBY_RETURN_EMPTY;
    }
    
    c_buffer = NULL;
    c_from_group = NULL;
    c_name = NULL;
    
    if (NIL_P (buffer) || NIL_P (from_group) || NIL_P (name))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "nicklist_search_nick");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (buffer, T_STRING);
    Check_Type (from_group, T_STRING);
    Check_Type (name, T_STRING);
    
    c_buffer = STR2CSTR (buffer);
    c_from_group = STR2CSTR (from_group);
    c_name = STR2CSTR (name);
    
    result = script_ptr2str (weechat_nicklist_search_nick (script_str2ptr (c_buffer),
                                                           script_str2ptr (c_from_group),
                                                           c_name));
    
    RUBY_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_nicklist_remove_group: remove a group from nicklist
 */

static VALUE
weechat_ruby_api_nicklist_remove_group (VALUE class, VALUE buffer, VALUE group)
{
    char *c_buffer, *c_group;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "nicklist_remove_group");
        RUBY_RETURN_ERROR;
    }
    
    if (NIL_P (buffer) || NIL_P (group))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "nicklist_remove_group");
        RUBY_RETURN_ERROR;
    }
    
    Check_Type (buffer, T_STRING);
    Check_Type (group, T_STRING);
    
    c_buffer = STR2CSTR (buffer);
    c_group = STR2CSTR (group);
    
    weechat_nicklist_remove_group (script_str2ptr (c_buffer),
                                   script_str2ptr (c_group));
    
    RUBY_RETURN_OK;
}

/*
 * weechat_ruby_api_nicklist_remove_nick: remove a nick from nicklist
 */

static VALUE
weechat_ruby_api_nicklist_remove_nick (VALUE class, VALUE buffer, VALUE nick)
{
    char *c_buffer, *c_nick;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "nicklist_remove_nick");
        RUBY_RETURN_ERROR;
    }
    
    if (NIL_P (buffer) || NIL_P (nick))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "nicklist_remove_nick");
        RUBY_RETURN_ERROR;
    }
    
    Check_Type (buffer, T_STRING);
    Check_Type (nick, T_STRING);
    
    c_buffer = STR2CSTR (buffer);
    c_nick = STR2CSTR (nick);
    
    weechat_nicklist_remove_nick (script_str2ptr (c_buffer),
                                  script_str2ptr (c_nick));
    
    RUBY_RETURN_OK;
}

/*
 * weechat_ruby_api_nicklist_remove_all: remove all groups/nicks from nicklist
 */

static VALUE
weechat_ruby_api_nicklist_remove_all (VALUE class, VALUE buffer)
{
    char *c_buffer;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "nicklist_remove_all");
        RUBY_RETURN_ERROR;
    }
    
    if (NIL_P (buffer))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "nicklist_remove_all");
        RUBY_RETURN_ERROR;
    }
    
    Check_Type (buffer, T_STRING);
    
    c_buffer = STR2CSTR (buffer);
    
    weechat_nicklist_remove_all (script_str2ptr (c_buffer));
    
    RUBY_RETURN_OK;
}

/*
 * weechat_ruby_api_bar_item_search: search a bar item
 */

static VALUE
weechat_ruby_api_bar_item_search (VALUE class, VALUE name)
{
    char *c_name, *result;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "bar_item_search");
        RUBY_RETURN_EMPTY;
    }
    
    c_name = NULL;
    
    if (NIL_P (name))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "bar_item_search");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (name, T_STRING);
    
    c_name = STR2CSTR (name);
    
    result = script_ptr2str (weechat_bar_item_search (c_name));
    
    RUBY_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_bar_item_build_cb: callback for building bar item
 */

char *
weechat_ruby_api_bar_item_build_cb (void *data, struct t_gui_bar_item *item,
                                    struct t_gui_window *window)
{
    struct t_script_callback *script_callback;
    char *ruby_argv[4], empty_arg[1] = { '\0' }, *ret;
    
    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        ruby_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        ruby_argv[1] = script_ptr2str (item);
        ruby_argv[2] = script_ptr2str (window);
        ruby_argv[3] = NULL;
        
        ret = (char *)weechat_ruby_exec (script_callback->script,
                                         WEECHAT_SCRIPT_EXEC_STRING,
                                         script_callback->function,
                                         ruby_argv);
        
        if (ruby_argv[1])
            free (ruby_argv[1]);
        if (ruby_argv[2])
            free (ruby_argv[2]);
        
        return ret;
    }
    
    return NULL;
}

/*
 * weechat_ruby_api_bar_item_new: add a new bar item
 */

static VALUE
weechat_ruby_api_bar_item_new (VALUE class, VALUE name, VALUE function,
                               VALUE data)
{
    char *c_name, *c_function, *c_data, *result;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "bar_item_new");
        RUBY_RETURN_EMPTY;
    }
    
    c_name = NULL;
    c_function = NULL;
    c_data = NULL;
    
    if (NIL_P (name) || NIL_P (function) || NIL_P (data))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "bar_item_new");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (name, T_STRING);
    Check_Type (function, T_STRING);
    Check_Type (data, T_STRING);
    
    c_name = STR2CSTR (name);
    c_function = STR2CSTR (function);
    c_data = STR2CSTR (data);
    
    result = script_ptr2str (script_api_bar_item_new (weechat_ruby_plugin,
                                                      ruby_current_script,
                                                      c_name,
                                                      &weechat_ruby_api_bar_item_build_cb,
                                                      c_function,
                                                      c_data));
    
    RUBY_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_bar_item_update: update a bar item on screen
 */

static VALUE
weechat_ruby_api_bar_item_update (VALUE class, VALUE name)
{
    char *c_name;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "bar_item_update");
        RUBY_RETURN_ERROR;
    }
    
    if (NIL_P (name))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "bar_item_update");
        RUBY_RETURN_ERROR;
    }
    
    Check_Type (name, T_STRING);
    
    c_name = STR2CSTR (name);
    
    weechat_bar_item_update (c_name);
    
    RUBY_RETURN_OK;
}

/*
 * weechat_ruby_api_bar_item_remove: remove a bar item
 */

static VALUE
weechat_ruby_api_bar_item_remove (VALUE class, VALUE item)
{
    char *c_item;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "bar_item_remove");
        RUBY_RETURN_ERROR;
    }
    
    if (NIL_P (item))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "bar_item_remove");
        RUBY_RETURN_ERROR;
    }
    
    Check_Type (item, T_STRING);
    
    c_item = STR2CSTR (item);
    
    script_api_bar_item_remove (weechat_ruby_plugin,
                                ruby_current_script,
                                script_str2ptr (c_item));
    
    RUBY_RETURN_OK;
}

/*
 * weechat_ruby_api_bar_search: search a bar
 */

static VALUE
weechat_ruby_api_bar_search (VALUE class, VALUE name)
{
    char *c_name, *result;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "bar_search");
        RUBY_RETURN_EMPTY;
    }
    
    c_name = NULL;
    
    if (NIL_P (name))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "bar_search");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (name, T_STRING);
    
    c_name = STR2CSTR (name);
    
    result = script_ptr2str (weechat_bar_search (c_name));
    
    RUBY_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_bar_new: add a new bar
 */

static VALUE
weechat_ruby_api_bar_new (VALUE class, VALUE name, VALUE hidden,
                          VALUE priority, VALUE type, VALUE conditions,
                          VALUE position, VALUE filling_top_bottom,
                          VALUE filling_left_right, VALUE size,
                          VALUE size_max, VALUE color_fg, VALUE color_delim,
                          VALUE color_bg, VALUE separator, VALUE items)
{
    char *c_name, *c_hidden, *c_priority, *c_type, *c_conditions, *c_position;
    char *c_filling_top_bottom, *c_filling_left_right, *c_size, *c_size_max;
    char *c_color_fg, *c_color_delim, *c_color_bg, *c_separator, *c_items;
    char *result;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "bar_new");
        RUBY_RETURN_EMPTY;
    }
    
    c_name = NULL;
    c_hidden = NULL;
    c_priority = NULL;
    c_type = NULL;
    c_conditions = NULL;
    c_position = NULL;
    c_filling_top_bottom = NULL;
    c_filling_left_right = NULL;
    c_size = NULL;
    c_size_max = NULL;
    c_color_fg = NULL;
    c_color_delim = NULL;
    c_color_bg = NULL;
    c_separator = NULL;
    c_items = NULL;
    
    if (NIL_P (name) || NIL_P (hidden) || NIL_P (priority) || NIL_P (type)
        || NIL_P (conditions) || NIL_P (position) || NIL_P (filling_top_bottom)
        || NIL_P (filling_left_right) || NIL_P (size) || NIL_P (size_max)
        || NIL_P (color_fg) || NIL_P (color_delim) || NIL_P (color_bg)
        || NIL_P (separator) || NIL_P (items))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "bar_new");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (name, T_STRING);
    Check_Type (hidden, T_STRING);
    Check_Type (priority, T_STRING);
    Check_Type (type, T_STRING);
    Check_Type (conditions, T_STRING);
    Check_Type (position, T_STRING);
    Check_Type (filling_top_bottom, T_STRING);
    Check_Type (filling_left_right, T_STRING);
    Check_Type (size, T_STRING);
    Check_Type (size_max, T_STRING);
    Check_Type (color_fg, T_STRING);
    Check_Type (color_delim, T_STRING);
    Check_Type (color_bg, T_STRING);
    Check_Type (separator, T_STRING);
    Check_Type (items, T_STRING);
    
    c_name = STR2CSTR (name);
    c_hidden = STR2CSTR (hidden);
    c_priority = STR2CSTR (priority);
    c_type = STR2CSTR (type);
    c_conditions = STR2CSTR (conditions);
    c_position = STR2CSTR (position);
    c_filling_top_bottom = STR2CSTR (filling_top_bottom);
    c_filling_left_right = STR2CSTR (filling_left_right);
    c_size = STR2CSTR (size);
    c_size_max = STR2CSTR (size_max);
    c_color_fg = STR2CSTR (color_fg);
    c_color_delim = STR2CSTR (color_delim);
    c_color_bg = STR2CSTR (color_bg);
    c_separator = STR2CSTR (separator);
    c_items = STR2CSTR (items);
    
    result = script_ptr2str (weechat_bar_new (c_name,
                                              c_hidden,
                                              c_priority,
                                              c_type,
                                              c_conditions,
                                              c_position,
                                              c_filling_top_bottom,
                                              c_filling_left_right,
                                              c_size,
                                              c_size_max,
                                              c_color_fg,
                                              c_color_delim,
                                              c_color_bg,
                                              c_separator,
                                              c_items));
    
    RUBY_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_bar_set: set a bar property
 */

static VALUE
weechat_ruby_api_bar_set (VALUE class, VALUE bar, VALUE property, VALUE value)
{
    char *c_bar, *c_property, *c_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "bar_set");
        RUBY_RETURN_ERROR;
    }
    
    if (NIL_P (bar) || NIL_P (property) || NIL_P (value))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "bar_set");
        RUBY_RETURN_ERROR;
    }
    
    Check_Type (bar, T_STRING);
    Check_Type (property, T_STRING);
    Check_Type (value, T_STRING);
    
    c_bar = STR2CSTR (bar);
    c_property = STR2CSTR (property);
    c_value = STR2CSTR (value);
    
    weechat_buffer_set (script_str2ptr (c_bar),
                        c_property,
                        c_value);
    
    RUBY_RETURN_OK;
}

/*
 * weechat_ruby_api_bar_update: update a bar on screen
 */

static VALUE
weechat_ruby_api_bar_update (VALUE class, VALUE name)
{
    char *c_name;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "bar_update");
        RUBY_RETURN_ERROR;
    }
    
    if (NIL_P (name))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "bar_update");
        RUBY_RETURN_ERROR;
    }
    
    Check_Type (name, T_STRING);
    
    c_name = STR2CSTR (name);
    
    weechat_bar_update (c_name);
    
    RUBY_RETURN_OK;
}

/*
 * weechat_ruby_api_bar_remove: remove a bar
 */

static VALUE
weechat_ruby_api_bar_remove (VALUE class, VALUE bar)
{
    char *c_bar;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "bar_remove");
        RUBY_RETURN_ERROR;
    }
    
    if (NIL_P (bar))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "bar_remove");
        RUBY_RETURN_ERROR;
    }
    
    Check_Type (bar, T_STRING);
    
    c_bar = STR2CSTR (bar);
    
    weechat_bar_remove (script_str2ptr (c_bar));
    
    RUBY_RETURN_OK;
}

/*
 * weechat_ruby_api_command: send command to server
 */

static VALUE
weechat_ruby_api_command (VALUE class, VALUE buffer, VALUE command)
{
    char *c_buffer, *c_command;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "command");
        RUBY_RETURN_ERROR;
    }
    
    if (NIL_P (buffer) || NIL_P (command))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "command");
        RUBY_RETURN_ERROR;
    }
    
    Check_Type (buffer, T_STRING);
    Check_Type (command, T_STRING);
    
    c_buffer = STR2CSTR (buffer);
    c_command = STR2CSTR (command);
    
    script_api_command (weechat_ruby_plugin,
                        ruby_current_script,
                        script_str2ptr (c_buffer),
                        c_command);
    
    RUBY_RETURN_OK;
}

/*
 * weechat_ruby_api_info_get: get info about WeeChat
 */

static VALUE
weechat_ruby_api_info_get (VALUE class, VALUE info_name, VALUE arguments)
{
    char *c_info_name, *c_arguments;
    const char *result;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "info_get");
        RUBY_RETURN_EMPTY;
    }
    
    if (NIL_P (info_name) || NIL_P (arguments))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "info_get");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (info_name, T_STRING);
    Check_Type (arguments, T_STRING);
    
    c_info_name = STR2CSTR (info_name);
    c_arguments = STR2CSTR (arguments);
    
    result = weechat_info_get (c_info_name, c_arguments);
    
    RUBY_RETURN_STRING(result);
}

/*
 * weechat_ruby_api_infolist_new: create new infolist
 */

static VALUE
weechat_ruby_api_infolist_new (VALUE class)
{
    char *result;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "infolist_new");
        RUBY_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_infolist_new ());
    
    RUBY_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_infolist_new_var_integer: create new integer variable in
 *                                            infolist
 */

static VALUE
weechat_ruby_api_infolist_new_var_integer (VALUE class, VALUE infolist,
                                           VALUE name, VALUE value)
{
    char *c_infolist, *c_name, *result;
    int c_value;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "infolist_new_var_integer");
        RUBY_RETURN_EMPTY;
    }
    
    if (NIL_P (infolist) || NIL_P (name) || NIL_P (value))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "infolist_new_var_integer");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (infolist, T_STRING);
    Check_Type (name, T_STRING);
    Check_Type (value, T_FIXNUM);
    
    c_infolist = STR2CSTR (infolist);
    c_name = STR2CSTR (name);
    c_value = FIX2INT (value);
    
    result = script_ptr2str (weechat_infolist_new_var_integer (script_str2ptr (c_infolist),
                                                               c_name,
                                                               c_value));
    
    RUBY_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_infolist_new_var_string: create new string variable in
 *                                           infolist
 */

static VALUE
weechat_ruby_api_infolist_new_var_string (VALUE class, VALUE infolist,
                                          VALUE name, VALUE value)
{
    char *c_infolist, *c_name, *c_value, *result;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "infolist_new_var_string");
        RUBY_RETURN_EMPTY;
    }
    
    if (NIL_P (infolist) || NIL_P (name) || NIL_P (value))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "infolist_new_var_string");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (infolist, T_STRING);
    Check_Type (name, T_STRING);
    Check_Type (value, T_STRING);
    
    c_infolist = STR2CSTR (infolist);
    c_name = STR2CSTR (name);
    c_value = STR2CSTR (value);
    
    result = script_ptr2str (weechat_infolist_new_var_string (script_str2ptr (c_infolist),
                                                              c_name,
                                                              c_value));
    
    RUBY_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_infolist_new_var_pointer: create new pointer variable in
 *                                            infolist
 */

static VALUE
weechat_ruby_api_infolist_new_var_pointer (VALUE class, VALUE infolist,
                                           VALUE name, VALUE value)
{
    char *c_infolist, *c_name, *c_value, *result;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "infolist_new_var_pointer");
        RUBY_RETURN_EMPTY;
    }
    
    if (NIL_P (infolist) || NIL_P (name) || NIL_P (value))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "infolist_new_var_pointer");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (infolist, T_STRING);
    Check_Type (name, T_STRING);
    Check_Type (value, T_STRING);
    
    c_infolist = STR2CSTR (infolist);
    c_name = STR2CSTR (name);
    c_value = STR2CSTR (value);
    
    result = script_ptr2str (weechat_infolist_new_var_pointer (script_str2ptr (c_infolist),
                                                               c_name,
                                                               script_str2ptr (c_value)));
    
    RUBY_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_infolist_new_var_time: create new time variable in infolist
 */

static VALUE
weechat_ruby_api_infolist_new_var_time (VALUE class, VALUE infolist,
                                        VALUE name, VALUE value)
{
    char *c_infolist, *c_name, *result;
    int c_value;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "infolist_new_var_time");
        RUBY_RETURN_EMPTY;
    }
    
    if (NIL_P (infolist) || NIL_P (name) || NIL_P (value))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "infolist_new_var_time");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (infolist, T_STRING);
    Check_Type (name, T_STRING);
    Check_Type (value, T_FIXNUM);
    
    c_infolist = STR2CSTR (infolist);
    c_name = STR2CSTR (name);
    c_value = FIX2INT (value);
    
    result = script_ptr2str (weechat_infolist_new_var_time (script_str2ptr (c_infolist),
                                                            c_name,
                                                            c_value));
    
    RUBY_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_infolist_get: get list with infos
 */

static VALUE
weechat_ruby_api_infolist_get (VALUE class, VALUE name, VALUE pointer,
                               VALUE arguments)
{
    char *c_name, *c_pointer, *c_arguments, *result;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "infolist_get");
        RUBY_RETURN_EMPTY;
    }
    
    if (NIL_P (name) || NIL_P (pointer))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "infolist_get");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (name, T_STRING);
    Check_Type (pointer, T_STRING);
    Check_Type (arguments, T_STRING);
    
    c_name = STR2CSTR (name);
    c_pointer = STR2CSTR (pointer);
    c_arguments = STR2CSTR (arguments);
    
    result = script_ptr2str (weechat_infolist_get (c_name,
                                                   script_str2ptr (c_pointer),
                                                   c_arguments));
    
    RUBY_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_infolist_next: move item pointer to next item in infolist
 */

static VALUE
weechat_ruby_api_infolist_next (VALUE class, VALUE infolist)
{
    char *c_infolist;
    int value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "infolist_next");
        RUBY_RETURN_INT(0);
    }
    
    if (NIL_P (infolist))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "infolist_next");
        RUBY_RETURN_INT(0);
    }
    
    Check_Type (infolist, T_STRING);
    
    c_infolist = STR2CSTR (infolist);
    
    value = weechat_infolist_next (script_str2ptr (c_infolist));
    
    RUBY_RETURN_INT(value);
}

/*
 * weechat_ruby_api_infolist_prev: move item pointer to previous item in infolist
 */

static VALUE
weechat_ruby_api_infolist_prev (VALUE class, VALUE infolist)
{
    char *c_infolist;
    int value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "infolist_prev");
        RUBY_RETURN_INT(0);
    }
    
    if (NIL_P (infolist))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "infolist_prev");
        RUBY_RETURN_INT(0);
    }
    
    Check_Type (infolist, T_STRING);
    
    c_infolist = STR2CSTR (infolist);
    
    value = weechat_infolist_prev (script_str2ptr (c_infolist));
    
    RUBY_RETURN_INT(value);
}

/*
 * weechat_ruby_api_infolist_fields: get list of fields for current item of infolist
 */

static VALUE
weechat_ruby_api_infolist_fields (VALUE class, VALUE infolist)
{
    char *c_infolist;
    const char *result;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "infolist_fields");
        RUBY_RETURN_EMPTY;
    }
    
    if (NIL_P (infolist))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "infolist_fields");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (infolist, T_STRING);
    
    c_infolist = STR2CSTR (infolist);
    
    result = weechat_infolist_fields (script_str2ptr (c_infolist));
    
    RUBY_RETURN_STRING(result);
}

/*
 * weechat_ruby_api_infolist_integer: get integer value of a variable in infolist
 */

static VALUE
weechat_ruby_api_infolist_integer (VALUE class, VALUE infolist, VALUE variable)
{
    char *c_infolist, *c_variable;
    int value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "infolist_integer");
        RUBY_RETURN_INT(0);
    }
    
    if (NIL_P (infolist) || NIL_P (variable))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "infolist_integer");
        RUBY_RETURN_INT(0);
    }
    
    Check_Type (infolist, T_STRING);
    Check_Type (variable, T_STRING);
    
    c_infolist = STR2CSTR (infolist);
    c_variable = STR2CSTR (variable);
    
    value = weechat_infolist_integer (script_str2ptr (c_infolist), c_variable);
    
    RUBY_RETURN_INT(value);
}

/*
 * weechat_ruby_api_infolist_string: get string value of a variable in infolist
 */

static VALUE
weechat_ruby_api_infolist_string (VALUE class, VALUE infolist, VALUE variable)
{
    char *c_infolist, *c_variable;
    const char *result;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "infolist_string");
        RUBY_RETURN_EMPTY;
    }
    
    if (NIL_P (infolist) || NIL_P (variable))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "infolist_string");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (infolist, T_STRING);
    Check_Type (variable, T_STRING);
    
    c_infolist = STR2CSTR (infolist);
    c_variable = STR2CSTR (variable);
    
    result = weechat_infolist_string (script_str2ptr (c_infolist), c_variable);
    
    RUBY_RETURN_STRING(result);
}

/*
 * weechat_ruby_api_infolist_pointer: get pointer value of a variable in infolist
 */

static VALUE
weechat_ruby_api_infolist_pointer (VALUE class, VALUE infolist, VALUE variable)
{
    char *c_infolist, *c_variable, *result;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "infolist_pointer");
        RUBY_RETURN_EMPTY;
    }
    
    if (NIL_P (infolist) || NIL_P (variable))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "infolist_pointer");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (infolist, T_STRING);
    Check_Type (variable, T_STRING);
    
    c_infolist = STR2CSTR (infolist);
    c_variable = STR2CSTR (variable);
    
    result = script_ptr2str (weechat_infolist_pointer (script_str2ptr (c_infolist), c_variable));
    
    RUBY_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_infolist_time: get time value of a variable in infolist
 */

static VALUE
weechat_ruby_api_infolist_time (VALUE class, VALUE infolist, VALUE variable)
{
    char *c_infolist, *c_variable, timebuffer[64], *result;
    time_t time;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "infolist_time");
        RUBY_RETURN_EMPTY;
    }
    
    if (NIL_P (infolist) || NIL_P (variable))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "infolist_time");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (infolist, T_STRING);
    Check_Type (variable, T_STRING);
    
    c_infolist = STR2CSTR (infolist);
    c_variable = STR2CSTR (variable);
    
    time = weechat_infolist_time (script_str2ptr (c_infolist), c_variable);
    strftime (timebuffer, sizeof (timebuffer), "%F %T", localtime (&time));
    result = strdup (timebuffer);
    
    RUBY_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_infolist_free: free infolist
 */

static VALUE
weechat_ruby_api_infolist_free (VALUE class, VALUE infolist)
{
    char *c_infolist;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "infolist_free");
        RUBY_RETURN_ERROR;
    }
    
    if (NIL_P (infolist))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "infolist_free");
        RUBY_RETURN_ERROR;
    }
    
    Check_Type (infolist, T_STRING);
    
    c_infolist = STR2CSTR (infolist);
    
    weechat_infolist_free (script_str2ptr (c_infolist));
    
    RUBY_RETURN_OK;
}

/*
 * weechat_ruby_api_upgrade_new: create an upgrade file
 */

static VALUE
weechat_ruby_api_upgrade_new (VALUE class, VALUE filename, VALUE write)
{
    char *c_filename, *result;
    int c_write;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "upgrade_new");
        RUBY_RETURN_EMPTY;
    }
    
    c_filename = NULL;
    c_write = 0;
    
    if (NIL_P (filename) || NIL_P (write))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "upgrade_new");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (filename, T_STRING);
    Check_Type (write, T_FIXNUM);
    
    c_filename = STR2CSTR (filename);
    c_write = FIX2INT (write);
    
    result = script_ptr2str (weechat_upgrade_new (c_filename,
                                                  c_write));
    
    RUBY_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_upgrade_write_object: write object in upgrade file
 */

static VALUE
weechat_ruby_api_upgrade_write_object (VALUE class, VALUE upgrade_file,
                                       VALUE object_id, VALUE infolist)
{
    char *c_upgrade_file, *c_infolist;
    int c_object_id, rc;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "upgrade_write_object");
        RUBY_RETURN_INT(0);
    }
    
    if (NIL_P (upgrade_file) || NIL_P (object_id) || NIL_P (infolist))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "upgrade_write_object");
        RUBY_RETURN_INT(0);
    }
    
    Check_Type (upgrade_file, T_STRING);
    Check_Type (object_id, T_FIXNUM);
    Check_Type (infolist, T_STRING);
    
    c_upgrade_file = STR2CSTR (upgrade_file);
    c_object_id = FIX2INT (object_id);
    c_infolist = STR2CSTR (infolist);
    
    rc = weechat_upgrade_write_object (script_str2ptr (c_upgrade_file),
                                       c_object_id,
                                       script_str2ptr (c_infolist));
    
    RUBY_RETURN_INT(rc);
}

/*
 * weechat_ruby_api_upgrade_read_cb: callback for reading object in upgrade file
 */

int
weechat_ruby_api_upgrade_read_cb (void *data,
                                  struct t_upgrade_file *upgrade_file,
                                  int object_id,
                                  struct t_infolist *infolist)
{
    struct t_script_callback *script_callback;
    char *ruby_argv[5], empty_arg[1] = { '\0' }, str_object_id[32];
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback && script_callback->function && script_callback->function[0])
    {
        snprintf (str_object_id, sizeof (str_object_id), "%d", object_id);
        
        ruby_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        ruby_argv[1] = script_ptr2str (upgrade_file);
        ruby_argv[2] = str_object_id;
        ruby_argv[3] = script_ptr2str (infolist);
        ruby_argv[4] = NULL;
        
        rc = (int *) weechat_ruby_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        ruby_argv);
        
        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (ruby_argv[1])
            free (ruby_argv[1]);
        if (ruby_argv[3])
            free (ruby_argv[3]);
        
        return ret;
    }
    
    return WEECHAT_RC_ERROR;
}

/*
 * weechat_ruby_api_upgrade_read: read upgrade file
 */

static VALUE
weechat_ruby_api_upgrade_read (VALUE class, VALUE upgrade_file,
                               VALUE function, VALUE data)
{
    char *c_upgrade_file, *c_function, *c_data;
    int rc;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "upgrade_read");
        RUBY_RETURN_INT(0);
    }
    
    c_upgrade_file = NULL;
    c_function = NULL;
    c_data = NULL;
    
    if (NIL_P (upgrade_file) || NIL_P (function) || NIL_P (data))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "upgrade_read");
        RUBY_RETURN_INT(0);
    }
    
    Check_Type (upgrade_file, T_STRING);
    Check_Type (function, T_STRING);
    Check_Type (data, T_STRING);
    
    c_upgrade_file = STR2CSTR (upgrade_file);
    c_function = STR2CSTR (function);
    c_data = STR2CSTR (data);
    
    rc = script_api_upgrade_read (weechat_ruby_plugin,
                                  ruby_current_script,
                                  script_str2ptr (c_upgrade_file),
                                  &weechat_ruby_api_upgrade_read_cb,
                                  c_function,
                                  c_data);
    
    RUBY_RETURN_INT(rc);
}

/*
 * weechat_ruby_api_upgrade_close: close upgrade file
 */

static VALUE
weechat_ruby_api_upgrade_close (VALUE class, VALUE upgrade_file)
{
    char *c_upgrade_file;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME, "upgrade_close");
        RUBY_RETURN_ERROR;
    }
    
    if (NIL_P (upgrade_file))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME, "upgrade_close");
        RUBY_RETURN_ERROR;
    }
    
    Check_Type (upgrade_file, T_STRING);
    
    c_upgrade_file = STR2CSTR (upgrade_file);
    
    weechat_upgrade_close (script_str2ptr (c_upgrade_file));
    
    RUBY_RETURN_OK;
}

/*
 * weechat_ruby_api_init: init Ruby API: add variables and functions
 */

void
weechat_ruby_api_init (VALUE ruby_mWeechat)
{
    rb_define_const(ruby_mWeechat, "WEECHAT_RC_OK", INT2NUM(WEECHAT_RC_OK));
    rb_define_const(ruby_mWeechat, "WEECHAT_RC_OK_EAT", INT2NUM(WEECHAT_RC_OK_EAT));
    rb_define_const(ruby_mWeechat, "WEECHAT_RC_ERROR", INT2NUM(WEECHAT_RC_ERROR));
    
    rb_define_const(ruby_mWeechat, "WEECHAT_CONFIG_READ_OK", INT2NUM(WEECHAT_CONFIG_READ_OK));
    rb_define_const(ruby_mWeechat, "WEECHAT_CONFIG_READ_MEMORY_ERROR", INT2NUM(WEECHAT_CONFIG_READ_MEMORY_ERROR));
    rb_define_const(ruby_mWeechat, "WEECHAT_CONFIG_READ_FILE_NOT_FOUND", INT2NUM(WEECHAT_CONFIG_READ_FILE_NOT_FOUND));
    rb_define_const(ruby_mWeechat, "WEECHAT_CONFIG_WRITE_OK", INT2NUM(WEECHAT_CONFIG_WRITE_OK));
    rb_define_const(ruby_mWeechat, "WEECHAT_CONFIG_WRITE_ERROR", INT2NUM(WEECHAT_CONFIG_WRITE_ERROR));
    rb_define_const(ruby_mWeechat, "WEECHAT_CONFIG_WRITE_MEMORY_ERROR", INT2NUM(WEECHAT_CONFIG_WRITE_MEMORY_ERROR));
    rb_define_const(ruby_mWeechat, "WEECHAT_CONFIG_OPTION_SET_OK_CHANGED", INT2NUM(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED));
    rb_define_const(ruby_mWeechat, "WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE", INT2NUM(WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE));
    rb_define_const(ruby_mWeechat, "WEECHAT_CONFIG_OPTION_SET_ERROR", INT2NUM(WEECHAT_CONFIG_OPTION_SET_ERROR));
    rb_define_const(ruby_mWeechat, "WEECHAT_CONFIG_OPTION_SET_OPTION_NOT_FOUND", INT2NUM(WEECHAT_CONFIG_OPTION_SET_OPTION_NOT_FOUND));
    rb_define_const(ruby_mWeechat, "WEECHAT_CONFIG_OPTION_UNSET_OK_NO_RESET", INT2NUM(WEECHAT_CONFIG_OPTION_UNSET_OK_NO_RESET));
    rb_define_const(ruby_mWeechat, "WEECHAT_CONFIG_OPTION_UNSET_OK_RESET", INT2NUM(WEECHAT_CONFIG_OPTION_UNSET_OK_RESET));
    rb_define_const(ruby_mWeechat, "WEECHAT_CONFIG_OPTION_UNSET_OK_REMOVED", INT2NUM(WEECHAT_CONFIG_OPTION_UNSET_OK_REMOVED));
    rb_define_const(ruby_mWeechat, "WEECHAT_CONFIG_OPTION_UNSET_ERROR", INT2NUM(WEECHAT_CONFIG_OPTION_UNSET_ERROR));
    
    rb_define_const(ruby_mWeechat, "WEECHAT_LIST_POS_SORT", rb_str_new2(WEECHAT_LIST_POS_SORT));
    rb_define_const(ruby_mWeechat, "WEECHAT_LIST_POS_BEGINNING", rb_str_new2(WEECHAT_LIST_POS_BEGINNING));
    rb_define_const(ruby_mWeechat, "WEECHAT_LIST_POS_END", rb_str_new2(WEECHAT_LIST_POS_END));
    
    rb_define_const(ruby_mWeechat, "WEECHAT_HOTLIST_LOW", rb_str_new2(WEECHAT_HOTLIST_LOW));
    rb_define_const(ruby_mWeechat, "WEECHAT_HOTLIST_MESSAGE", rb_str_new2(WEECHAT_HOTLIST_MESSAGE));
    rb_define_const(ruby_mWeechat, "WEECHAT_HOTLIST_PRIVATE", rb_str_new2(WEECHAT_HOTLIST_PRIVATE));
    rb_define_const(ruby_mWeechat, "WEECHAT_HOTLIST_HIGHLIGHT", rb_str_new2(WEECHAT_HOTLIST_HIGHLIGHT));
    
    rb_define_const(ruby_mWeechat, "WEECHAT_HOOK_PROCESS_RUNNING", INT2NUM(WEECHAT_HOOK_PROCESS_RUNNING));
    rb_define_const(ruby_mWeechat, "WEECHAT_HOOK_PROCESS_ERROR", INT2NUM(WEECHAT_HOOK_PROCESS_ERROR));
    
    rb_define_const(ruby_mWeechat, "WEECHAT_HOOK_CONNECT_OK", INT2NUM(WEECHAT_HOOK_CONNECT_OK));
    rb_define_const(ruby_mWeechat, "WEECHAT_HOOK_CONNECT_ADDRESS_NOT_FOUND", INT2NUM(WEECHAT_HOOK_CONNECT_ADDRESS_NOT_FOUND));
    rb_define_const(ruby_mWeechat, "WEECHAT_HOOK_CONNECT_IP_ADDRESS_NOT_FOUND", INT2NUM(WEECHAT_HOOK_CONNECT_IP_ADDRESS_NOT_FOUND));
    rb_define_const(ruby_mWeechat, "WEECHAT_HOOK_CONNECT_CONNECTION_REFUSED", INT2NUM(WEECHAT_HOOK_CONNECT_CONNECTION_REFUSED));
    rb_define_const(ruby_mWeechat, "WEECHAT_HOOK_CONNECT_PROXY_ERROR", INT2NUM(WEECHAT_HOOK_CONNECT_PROXY_ERROR));
    rb_define_const(ruby_mWeechat, "WEECHAT_HOOK_CONNECT_LOCAL_HOSTNAME_ERROR", INT2NUM(WEECHAT_HOOK_CONNECT_LOCAL_HOSTNAME_ERROR));
    rb_define_const(ruby_mWeechat, "WEECHAT_HOOK_CONNECT_GNUTLS_INIT_ERROR", INT2NUM(WEECHAT_HOOK_CONNECT_GNUTLS_INIT_ERROR));
    rb_define_const(ruby_mWeechat, "WEECHAT_HOOK_CONNECT_GNUTLS_HANDSHAKE_ERROR", INT2NUM(WEECHAT_HOOK_CONNECT_GNUTLS_HANDSHAKE_ERROR));
    rb_define_const(ruby_mWeechat, "WEECHAT_HOOK_CONNECT_MEMORY_ERROR", INT2NUM(WEECHAT_HOOK_CONNECT_MEMORY_ERROR));
    
    rb_define_const(ruby_mWeechat, "WEECHAT_HOOK_SIGNAL_STRING", rb_str_new2(WEECHAT_HOOK_SIGNAL_STRING));
    rb_define_const(ruby_mWeechat, "WEECHAT_HOOK_SIGNAL_INT", rb_str_new2(WEECHAT_HOOK_SIGNAL_INT));
    rb_define_const(ruby_mWeechat, "WEECHAT_HOOK_SIGNAL_POINTER", rb_str_new2(WEECHAT_HOOK_SIGNAL_POINTER));
    
    rb_define_module_function (ruby_mWeechat, "register", &weechat_ruby_api_register, 7);
    rb_define_module_function (ruby_mWeechat, "plugin_get_name", &weechat_ruby_api_plugin_get_name, 1);
    rb_define_module_function (ruby_mWeechat, "charset_set", &weechat_ruby_api_charset_set, 1);
    rb_define_module_function (ruby_mWeechat, "iconv_to_internal", &weechat_ruby_api_iconv_to_internal, 2);
    rb_define_module_function (ruby_mWeechat, "iconv_from_internal", &weechat_ruby_api_iconv_from_internal, 2);
    rb_define_module_function (ruby_mWeechat, "gettext", &weechat_ruby_api_gettext, 1);
    rb_define_module_function (ruby_mWeechat, "ngettext", &weechat_ruby_api_ngettext, 3);
    rb_define_module_function (ruby_mWeechat, "string_remove_color", &weechat_ruby_api_string_remove_color, 2);
    rb_define_module_function (ruby_mWeechat, "mkdir_home", &weechat_ruby_api_mkdir_home, 2);
    rb_define_module_function (ruby_mWeechat, "mkdir", &weechat_ruby_api_mkdir, 2);
    rb_define_module_function (ruby_mWeechat, "mkdir_parents", &weechat_ruby_api_mkdir_parents, 2);
    rb_define_module_function (ruby_mWeechat, "list_new", &weechat_ruby_api_list_new, 0);
    rb_define_module_function (ruby_mWeechat, "list_add", &weechat_ruby_api_list_add, 4);
    rb_define_module_function (ruby_mWeechat, "list_search", &weechat_ruby_api_list_search, 2);
    rb_define_module_function (ruby_mWeechat, "list_casesearch", &weechat_ruby_api_list_casesearch, 2);
    rb_define_module_function (ruby_mWeechat, "list_get", &weechat_ruby_api_list_get, 2);
    rb_define_module_function (ruby_mWeechat, "list_set", &weechat_ruby_api_list_set, 2);
    rb_define_module_function (ruby_mWeechat, "list_next", &weechat_ruby_api_list_next, 1);
    rb_define_module_function (ruby_mWeechat, "list_prev", &weechat_ruby_api_list_prev, 1);
    rb_define_module_function (ruby_mWeechat, "list_string", &weechat_ruby_api_list_string, 1);
    rb_define_module_function (ruby_mWeechat, "list_size", &weechat_ruby_api_list_size, 1);
    rb_define_module_function (ruby_mWeechat, "list_remove", &weechat_ruby_api_list_remove, 2);
    rb_define_module_function (ruby_mWeechat, "list_remove_all", &weechat_ruby_api_list_remove_all, 1);
    rb_define_module_function (ruby_mWeechat, "list_free", &weechat_ruby_api_list_free, 1);
    rb_define_module_function (ruby_mWeechat, "config_new", &weechat_ruby_api_config_new, 3);
    rb_define_module_function (ruby_mWeechat, "config_new_section", &weechat_ruby_api_config_new_section, 14);
    rb_define_module_function (ruby_mWeechat, "config_search_section", &weechat_ruby_api_config_search_section, 2);
    rb_define_module_function (ruby_mWeechat, "config_new_option", &weechat_ruby_api_config_new_option, 17);
    rb_define_module_function (ruby_mWeechat, "config_search_option", &weechat_ruby_api_config_search_option, 3);
    rb_define_module_function (ruby_mWeechat, "config_string_to_boolean", &weechat_ruby_api_config_string_to_boolean, 1);
    rb_define_module_function (ruby_mWeechat, "config_option_reset", &weechat_ruby_api_config_option_reset, 2);
    rb_define_module_function (ruby_mWeechat, "config_option_set", &weechat_ruby_api_config_option_set, 3);
    rb_define_module_function (ruby_mWeechat, "config_option_set_null", &weechat_ruby_api_config_option_set_null, 2);
    rb_define_module_function (ruby_mWeechat, "config_option_unset", &weechat_ruby_api_config_option_unset, 1);
    rb_define_module_function (ruby_mWeechat, "config_option_rename", &weechat_ruby_api_config_option_rename, 2);
    rb_define_module_function (ruby_mWeechat, "config_option_is_null", &weechat_ruby_api_config_option_is_null, 1);
    rb_define_module_function (ruby_mWeechat, "config_option_default_is_null", &weechat_ruby_api_config_option_default_is_null, 1);
    rb_define_module_function (ruby_mWeechat, "config_boolean", &weechat_ruby_api_config_boolean, 1);
    rb_define_module_function (ruby_mWeechat, "config_boolean_default", &weechat_ruby_api_config_boolean_default, 1);
    rb_define_module_function (ruby_mWeechat, "config_integer", &weechat_ruby_api_config_integer, 1);
    rb_define_module_function (ruby_mWeechat, "config_integer_default", &weechat_ruby_api_config_integer_default, 1);
    rb_define_module_function (ruby_mWeechat, "config_string", &weechat_ruby_api_config_string, 1);
    rb_define_module_function (ruby_mWeechat, "config_string_default", &weechat_ruby_api_config_string_default, 1);
    rb_define_module_function (ruby_mWeechat, "config_color", &weechat_ruby_api_config_color, 1);
    rb_define_module_function (ruby_mWeechat, "config_color_default", &weechat_ruby_api_config_color_default, 1);
    rb_define_module_function (ruby_mWeechat, "config_write_option", &weechat_ruby_api_config_write_option, 2);
    rb_define_module_function (ruby_mWeechat, "config_write_line", &weechat_ruby_api_config_write_line, 3);
    rb_define_module_function (ruby_mWeechat, "config_write", &weechat_ruby_api_config_write, 1);
    rb_define_module_function (ruby_mWeechat, "config_read", &weechat_ruby_api_config_read, 1);
    rb_define_module_function (ruby_mWeechat, "config_reload", &weechat_ruby_api_config_reload, 1);
    rb_define_module_function (ruby_mWeechat, "config_option_free", &weechat_ruby_api_config_option_free, 1);
    rb_define_module_function (ruby_mWeechat, "config_section_free_options", &weechat_ruby_api_config_section_free_options, 1);
    rb_define_module_function (ruby_mWeechat, "config_section_free", &weechat_ruby_api_config_section_free, 1);
    rb_define_module_function (ruby_mWeechat, "config_free", &weechat_ruby_api_config_free, 1);
    rb_define_module_function (ruby_mWeechat, "config_get", &weechat_ruby_api_config_get, 1);
    rb_define_module_function (ruby_mWeechat, "config_get_plugin", &weechat_ruby_api_config_get_plugin, 1);
    rb_define_module_function (ruby_mWeechat, "config_is_set_plugin", &weechat_ruby_api_config_is_set_plugin, 1);
    rb_define_module_function (ruby_mWeechat, "config_set_plugin", &weechat_ruby_api_config_set_plugin, 2);
    rb_define_module_function (ruby_mWeechat, "config_unset_plugin", &weechat_ruby_api_config_unset_plugin, 1);
    rb_define_module_function (ruby_mWeechat, "prefix", &weechat_ruby_api_prefix, 1);
    rb_define_module_function (ruby_mWeechat, "color", &weechat_ruby_api_color, 1);
    rb_define_module_function (ruby_mWeechat, "print", &weechat_ruby_api_print, 2);
    rb_define_module_function (ruby_mWeechat, "print_date_tags", &weechat_ruby_api_print_date_tags, 4);
    rb_define_module_function (ruby_mWeechat, "print_y", &weechat_ruby_api_print_y, 3);
    rb_define_module_function (ruby_mWeechat, "log_print", &weechat_ruby_api_log_print, 1);
    rb_define_module_function (ruby_mWeechat, "hook_command", &weechat_ruby_api_hook_command, 7);
    rb_define_module_function (ruby_mWeechat, "hook_command_run", &weechat_ruby_api_hook_command_run, 3);
    rb_define_module_function (ruby_mWeechat, "hook_timer", &weechat_ruby_api_hook_timer, 5);
    rb_define_module_function (ruby_mWeechat, "hook_fd", &weechat_ruby_api_hook_fd, 6);
    rb_define_module_function (ruby_mWeechat, "hook_process", &weechat_ruby_api_hook_process, 4);
    rb_define_module_function (ruby_mWeechat, "hook_connect", &weechat_ruby_api_hook_connect, 8);
    rb_define_module_function (ruby_mWeechat, "hook_print", &weechat_ruby_api_hook_print, 6);
    rb_define_module_function (ruby_mWeechat, "hook_signal", &weechat_ruby_api_hook_signal, 3);
    rb_define_module_function (ruby_mWeechat, "hook_signal_send", &weechat_ruby_api_hook_signal_send, 3);
    rb_define_module_function (ruby_mWeechat, "hook_config", &weechat_ruby_api_hook_config, 3);
    rb_define_module_function (ruby_mWeechat, "hook_completion", &weechat_ruby_api_hook_completion, 4);
    rb_define_module_function (ruby_mWeechat, "hook_completion_list_add", &weechat_ruby_api_hook_completion_list_add, 4);
    rb_define_module_function (ruby_mWeechat, "hook_modifier", &weechat_ruby_api_hook_modifier, 3);
    rb_define_module_function (ruby_mWeechat, "hook_modifier_exec", &weechat_ruby_api_hook_modifier_exec, 3);
    rb_define_module_function (ruby_mWeechat, "hook_info", &weechat_ruby_api_hook_info, 4);
    rb_define_module_function (ruby_mWeechat, "hook_infolist", &weechat_ruby_api_hook_infolist, 4);
    rb_define_module_function (ruby_mWeechat, "unhook", &weechat_ruby_api_unhook, 1);
    rb_define_module_function (ruby_mWeechat, "unhook_all", &weechat_ruby_api_unhook_all, 0);
    rb_define_module_function (ruby_mWeechat, "buffer_new", &weechat_ruby_api_buffer_new, 5);
    rb_define_module_function (ruby_mWeechat, "buffer_search", &weechat_ruby_api_buffer_search, 2);
    rb_define_module_function (ruby_mWeechat, "current_buffer", &weechat_ruby_api_current_buffer, 0);
    rb_define_module_function (ruby_mWeechat, "buffer_clear", &weechat_ruby_api_buffer_clear, 1);
    rb_define_module_function (ruby_mWeechat, "buffer_close", &weechat_ruby_api_buffer_close, 1);
    rb_define_module_function (ruby_mWeechat, "buffer_get_integer", &weechat_ruby_api_buffer_get_integer, 2);
    rb_define_module_function (ruby_mWeechat, "buffer_get_string", &weechat_ruby_api_buffer_get_string, 2);
    rb_define_module_function (ruby_mWeechat, "buffer_get_pointer", &weechat_ruby_api_buffer_get_pointer, 2);
    rb_define_module_function (ruby_mWeechat, "buffer_set", &weechat_ruby_api_buffer_set, 3);
    rb_define_module_function (ruby_mWeechat, "current_window", &weechat_ruby_api_current_window, 0);
    rb_define_module_function (ruby_mWeechat, "window_get_integer", &weechat_ruby_api_window_get_integer, 2);
    rb_define_module_function (ruby_mWeechat, "window_get_string", &weechat_ruby_api_window_get_string, 2);
    rb_define_module_function (ruby_mWeechat, "window_get_pointer", &weechat_ruby_api_window_get_pointer, 2);
    rb_define_module_function (ruby_mWeechat, "window_set_title", &weechat_ruby_api_window_set_title, 1);
    rb_define_module_function (ruby_mWeechat, "nicklist_add_group", &weechat_ruby_api_nicklist_add_group, 5);
    rb_define_module_function (ruby_mWeechat, "nicklist_search_group", &weechat_ruby_api_nicklist_search_group, 3);
    rb_define_module_function (ruby_mWeechat, "nicklist_add_nick", &weechat_ruby_api_nicklist_add_nick, 7);
    rb_define_module_function (ruby_mWeechat, "nicklist_search_nick", &weechat_ruby_api_nicklist_search_nick, 3);
    rb_define_module_function (ruby_mWeechat, "nicklist_remove_group", &weechat_ruby_api_nicklist_remove_group, 2);
    rb_define_module_function (ruby_mWeechat, "nicklist_remove_nick", &weechat_ruby_api_nicklist_remove_nick, 2);
    rb_define_module_function (ruby_mWeechat, "nicklist_remove_all", &weechat_ruby_api_nicklist_remove_all, 1);
    rb_define_module_function (ruby_mWeechat, "bar_item_search", &weechat_ruby_api_bar_item_search, 1);
    rb_define_module_function (ruby_mWeechat, "bar_item_new", &weechat_ruby_api_bar_item_new, 3);
    rb_define_module_function (ruby_mWeechat, "bar_item_update", &weechat_ruby_api_bar_item_update, 1);
    rb_define_module_function (ruby_mWeechat, "bar_item_remove", &weechat_ruby_api_bar_item_remove, 1);
    rb_define_module_function (ruby_mWeechat, "bar_search", &weechat_ruby_api_bar_search, 1);
    rb_define_module_function (ruby_mWeechat, "bar_new", &weechat_ruby_api_bar_new, 15);
    rb_define_module_function (ruby_mWeechat, "bar_set", &weechat_ruby_api_bar_set, 3);
    rb_define_module_function (ruby_mWeechat, "bar_update", &weechat_ruby_api_bar_update, 1);
    rb_define_module_function (ruby_mWeechat, "bar_remove", &weechat_ruby_api_bar_remove, 1);
    rb_define_module_function (ruby_mWeechat, "command", &weechat_ruby_api_command, 2);
    rb_define_module_function (ruby_mWeechat, "info_get", &weechat_ruby_api_info_get, 2);
    rb_define_module_function (ruby_mWeechat, "infolist_new", &weechat_ruby_api_infolist_new, 0);
    rb_define_module_function (ruby_mWeechat, "infolist_new_var_integer", &weechat_ruby_api_infolist_new_var_integer, 3);
    rb_define_module_function (ruby_mWeechat, "infolist_new_var_string", &weechat_ruby_api_infolist_new_var_string, 3);
    rb_define_module_function (ruby_mWeechat, "infolist_new_var_pointer", &weechat_ruby_api_infolist_new_var_pointer, 3);
    rb_define_module_function (ruby_mWeechat, "infolist_new_var_time", &weechat_ruby_api_infolist_new_var_time, 3);
    rb_define_module_function (ruby_mWeechat, "infolist_get", &weechat_ruby_api_infolist_get, 3);
    rb_define_module_function (ruby_mWeechat, "infolist_next", &weechat_ruby_api_infolist_next, 1);
    rb_define_module_function (ruby_mWeechat, "infolist_prev", &weechat_ruby_api_infolist_prev, 1);
    rb_define_module_function (ruby_mWeechat, "infolist_fields", &weechat_ruby_api_infolist_fields, 1);
    rb_define_module_function (ruby_mWeechat, "infolist_integer", &weechat_ruby_api_infolist_integer, 2);
    rb_define_module_function (ruby_mWeechat, "infolist_string", &weechat_ruby_api_infolist_string, 2);
    rb_define_module_function (ruby_mWeechat, "infolist_pointer", &weechat_ruby_api_infolist_pointer, 2);
    rb_define_module_function (ruby_mWeechat, "infolist_time", &weechat_ruby_api_infolist_time, 2);
    rb_define_module_function (ruby_mWeechat, "infolist_free", &weechat_ruby_api_infolist_free, 1);
    rb_define_module_function (ruby_mWeechat, "upgrade_new", &weechat_ruby_api_upgrade_new, 2);
    rb_define_module_function (ruby_mWeechat, "upgrade_write_object", &weechat_ruby_api_upgrade_write_object, 3);
    rb_define_module_function (ruby_mWeechat, "upgrade_read", &weechat_ruby_api_upgrade_read, 3);
    rb_define_module_function (ruby_mWeechat, "upgrade_close", &weechat_ruby_api_upgrade_close, 1);
}
