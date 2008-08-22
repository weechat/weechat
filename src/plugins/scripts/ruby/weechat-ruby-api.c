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
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("register");
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
                        weechat_prefix ("error"), "ruby", c_name);
        RUBY_RETURN_ERROR;
    }
    
    /* register script */
    ruby_current_script = script_add (weechat_ruby_plugin,
                                      &ruby_scripts,
                                      (ruby_current_script_filename) ?
                                      ruby_current_script_filename : "",
                                      c_name, c_author, c_version, c_license,
                                      c_description, c_shutdown_func,
                                      c_charset);
    
    if (ruby_current_script)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s: registered script \"%s\", "
                                         "version %s (%s)"),
                        "ruby", c_name, c_version, c_description);
    }
    else
    {
        RUBY_RETURN_ERROR;
    }
    
    RUBY_RETURN_OK;
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
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("charset_set");
        RUBY_RETURN_ERROR;
    }
    
    c_charset = NULL;
    
    if (NIL_P (charset))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("charset_set");
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
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("iconv_to_internal");
        RUBY_RETURN_EMPTY;
    }
    
    c_charset = NULL;
    c_string = NULL;
    
    if (NIL_P (charset) || NIL_P (string))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("iconv_to_internal");
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
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("iconv_from_internal");
        RUBY_RETURN_EMPTY;
    }
    
    c_charset = NULL;
    c_string = NULL;
    
    if (NIL_P (charset) || NIL_P (string))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("iconv_from_internal");
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
    char *c_string, *result;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("gettext");
        RUBY_RETURN_EMPTY;
    }
    
    c_string = NULL;
    
    if (NIL_P (string))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("gettext");
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
    char *c_single, *c_plural, *result;
    int c_count;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("ngettext");
        RUBY_RETURN_EMPTY;
    }
    
    c_single = NULL;
    c_plural = NULL;
    c_count = 0;
    
    if (NIL_P (single) || NIL_P (plural) || NIL_P (count))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("ngettext");
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
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("mkdir_home");
        RUBY_RETURN_ERROR;
    }
    
    c_directory = NULL;
    c_mode = 0;
    
    if (NIL_P (directory) || NIL_P (mode))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("mkdir_home");
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
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("mkdir");
        RUBY_RETURN_ERROR;
    }
    
    c_directory = NULL;
    c_mode = 0;
    
    if (NIL_P (directory) || NIL_P (mode))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("mkdir");
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
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("list_new");
        RUBY_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_list_new ());
    
    RUBY_RETURN_STRING(result);
}

/*
 * weechat_ruby_api_list_add: add a string to list
 */

static VALUE
weechat_ruby_api_list_add (VALUE class, VALUE weelist, VALUE data, VALUE where)
{
    char *c_weelist, *c_data, *c_where, *result;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("list_add");
        RUBY_RETURN_EMPTY;
    }
    
    c_weelist = NULL;
    c_data = NULL;
    c_where = NULL;
    
    if (NIL_P (weelist) || NIL_P (data) || NIL_P (where))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("list_add");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (weelist, T_STRING);
    Check_Type (data, T_STRING);
    Check_Type (where, T_STRING);
    
    c_weelist = STR2CSTR (weelist);
    c_data = STR2CSTR (data);
    c_where = STR2CSTR (where);
    
    result = script_ptr2str (weechat_list_add (script_str2ptr(c_weelist),
                                               c_data,
                                               c_where));
    
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
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("list_search");
        RUBY_RETURN_EMPTY;
    }
    
    c_weelist = NULL;
    c_data = NULL;
    
    if (NIL_P (weelist) || NIL_P (data))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("list_search");
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
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("list_casesearch");
        RUBY_RETURN_EMPTY;
    }
    
    c_weelist = NULL;
    c_data = NULL;
    
    if (NIL_P (weelist) || NIL_P (data))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("list_casesearch");
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
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("list_get");
        RUBY_RETURN_EMPTY;
    }
    
    c_weelist = NULL;
    c_position = 0;
    
    if (NIL_P (weelist) || NIL_P (position))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("list_get");
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
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("list_set");
        RUBY_RETURN_ERROR;
    }
    
    c_item = NULL;
    c_new_value = NULL;
    
    if (NIL_P (item) || NIL_P (new_value))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("list_set");
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
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("list_next");
        RUBY_RETURN_EMPTY;
    }
    
    c_item = NULL;
    
    if (NIL_P (item))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("list_next");
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
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("list_prev");
        RUBY_RETURN_EMPTY;
    }
    
    c_item = NULL;
    
    if (NIL_P (item))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("list_prev");
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
    char *c_item, *result;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("list_string");
        RUBY_RETURN_EMPTY;
    }
    
    c_item = NULL;
    
    if (NIL_P (item))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("list_string");
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
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("list_size");
        RUBY_RETURN_INT(0);
    }
    
    c_weelist = NULL;
    
    if (NIL_P (weelist))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("list_size");
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
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("list_remove");
        RUBY_RETURN_ERROR;
    }
    
    c_weelist = NULL;
    c_item = NULL;
    
    if (NIL_P (weelist) || NIL_P (item))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("list_remove");
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
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("list_remove_all");
        RUBY_RETURN_ERROR;
    }
    
    c_weelist = NULL;
    
    if (NIL_P (weelist))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("list_remove_all");
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
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("list_free");
        RUBY_RETURN_ERROR;
    }
    
    c_weelist = NULL;
    
    if (NIL_P (weelist))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("list_free");
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
    char *ruby_argv[2];
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;

    if (script_callback->function && script_callback->function[0])
    {
        ruby_argv[0] = script_ptr2str (config_file);
        ruby_argv[1] = NULL;
        
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
        if (ruby_argv[0])
            free (ruby_argv[0]);
        
        return ret;
    }
    
    return 0;
}

/*
 * weechat_ruby_api_config_new: create a new configuration file
 */

static VALUE
weechat_ruby_api_config_new (VALUE class, VALUE name, VALUE function)
{
    char *c_name, *c_function, *result;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_new");
        RUBY_RETURN_EMPTY;
    }
    
    c_name = NULL;
    c_function = NULL;
    
    if (NIL_P (name) || NIL_P (function))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_new");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (name, T_STRING);
    Check_Type (function, T_STRING);
    
    c_name = STR2CSTR (name);
    c_function = STR2CSTR (function);
    
    result = script_ptr2str (script_api_config_new (weechat_ruby_plugin,
                                                    ruby_current_script,
                                                    c_name,
                                                    &weechat_ruby_api_config_reload_cb,
                                                    c_function));
    
    RUBY_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_config_read_cb: callback for reading option in section
 */

void
weechat_ruby_api_config_read_cb (void *data,
                                 struct t_config_file *config_file,
                                 const char *option_name, const char *value)
{
    struct t_script_callback *script_callback;
    char *ruby_argv[4];
    int *rc;
    
    script_callback = (struct t_script_callback *)data;

    if (script_callback->function && script_callback->function[0])
    {
        ruby_argv[0] = script_ptr2str (config_file);
        ruby_argv[1] = (char *)option_name;
        ruby_argv[2] = (char *)value;
        ruby_argv[3] = NULL;
        
        rc = (int *) weechat_ruby_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        ruby_argv);
        
        if (rc)
            free (rc);
        if (ruby_argv[0])
            free (ruby_argv[0]);
    }
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
    char *ruby_argv[3];
    int *rc;
    
    script_callback = (struct t_script_callback *)data;

    if (script_callback->function && script_callback->function[0])
    {
        ruby_argv[0] = script_ptr2str (config_file);
        ruby_argv[1] = (char *)section_name;
        ruby_argv[2] = NULL;
        
        rc = (int *) weechat_ruby_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        ruby_argv);
        
        if (rc)
            free (rc);
        if (ruby_argv[0])
            free (ruby_argv[0]);
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
    char *ruby_argv[3];
    int *rc;
    
    script_callback = (struct t_script_callback *)data;

    if (script_callback->function && script_callback->function[0])
    {
        ruby_argv[0] = script_ptr2str (config_file);
        ruby_argv[1] = (char *)section_name;
        ruby_argv[2] = NULL;
        
        rc = (int *) weechat_ruby_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        ruby_argv);
        
        if (rc)
            free (rc);
        if (ruby_argv[0])
            free (ruby_argv[0]);
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
    char *ruby_argv[5];
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;

    if (script_callback->function && script_callback->function[0])
    {
        ruby_argv[0] = script_ptr2str (config_file);
        ruby_argv[1] = script_ptr2str (section);
        ruby_argv[2] = (char *)option_name;
        ruby_argv[3] = (char *)value;
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
        if (ruby_argv[0])
            free (ruby_argv[0]);
        if (ruby_argv[1])
            free (ruby_argv[1]);
        
        return ret;
    }
    
    return 0;
}

/*
 * weechat_ruby_api_config_new_section: create a new section in configuration file
 */

static VALUE
weechat_ruby_api_config_new_section (VALUE class, VALUE config_file,
                                     VALUE name, VALUE user_can_add_options,
                                     VALUE user_can_delete_options,
                                     VALUE function_read,
                                     VALUE function_write,
                                     VALUE function_write_default,
                                     VALUE function_create_option)
{
    char *c_config_file, *c_name, *c_function_read, *c_function_write;
    char *c_function_write_default, *c_function_create_option;
    char *result;
    int c_user_can_add_options, c_user_can_delete_options;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_new_section");
        RUBY_RETURN_EMPTY;
    }
    
    c_config_file = NULL;
    c_name = NULL;
    c_user_can_add_options = 0;
    c_user_can_delete_options = 0;
    c_function_read = NULL;
    c_function_write = NULL;
    c_function_write_default = NULL;
    c_function_create_option = NULL;
    
    if (NIL_P (config_file) || NIL_P (name) || NIL_P (user_can_add_options)
        || NIL_P (user_can_delete_options) || NIL_P (function_read)
        || NIL_P (function_write) || NIL_P (function_write_default)
        || NIL_P (function_create_option))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_new_section");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (config_file, T_STRING);
    Check_Type (name, T_STRING);
    Check_Type (user_can_add_options, T_FIXNUM);
    Check_Type (user_can_delete_options, T_FIXNUM);
    Check_Type (function_read, T_STRING);
    Check_Type (function_write, T_STRING);
    Check_Type (function_write_default, T_STRING);
    Check_Type (function_create_option, T_STRING);
    
    c_config_file = STR2CSTR (config_file);
    c_name = STR2CSTR (name);
    c_user_can_add_options = FIX2INT (user_can_add_options);
    c_user_can_delete_options = FIX2INT (user_can_delete_options);
    c_function_read = STR2CSTR (function_read);
    c_function_write = STR2CSTR (function_write);
    c_function_write_default = STR2CSTR (function_write_default);
    c_function_create_option = STR2CSTR (function_create_option);
    
    result = script_ptr2str (script_api_config_new_section (weechat_ruby_plugin,
                                                            ruby_current_script,
                                                            script_str2ptr (c_config_file),
                                                            c_name,
                                                            c_user_can_add_options,
                                                            c_user_can_delete_options,
                                                            &weechat_ruby_api_config_read_cb,
                                                            c_function_read,
                                                            &weechat_ruby_api_config_section_write_cb,
                                                            c_function_write,
                                                            &weechat_ruby_api_config_section_write_default_cb,
                                                            c_function_write_default,
                                                            &weechat_ruby_api_config_section_create_option_cb,
                                                            c_function_create_option));
    
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
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_search_section");
        RUBY_RETURN_EMPTY;
    }
    
    c_config_file = NULL;
    c_section_name = NULL;
    
    if (NIL_P (config_file) || NIL_P (section_name))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_search_section");
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
 * weechat_ruby_api_config_option_change_cb: callback for option changed
 */

void
weechat_ruby_api_config_option_change_cb (void *data)
{
    struct t_script_callback *script_callback;
    char *ruby_argv[1];
    int *rc;
    
    script_callback = (struct t_script_callback *)data;

    if (script_callback->function && script_callback->function[0])
    {
        ruby_argv[0] = NULL;
        
        rc = (int *) weechat_ruby_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        ruby_argv);
        
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
                                    VALUE function)
{
    char *c_config_file, *c_section, *c_name, *c_type, *c_description;
    char *c_string_values, *c_default_value, *c_function, *result;
    int c_min, c_max;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_new_option");
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
    c_function = NULL;
    
    if (NIL_P (config_file) || NIL_P (section) || NIL_P (name) || NIL_P (type)
        || NIL_P (description) || NIL_P (string_values)
        || NIL_P (default_value) || NIL_P (function))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_new_option");
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
    Check_Type (function, T_STRING);
    
    c_config_file = STR2CSTR (config_file);
    c_section = STR2CSTR (section);
    c_name = STR2CSTR (name);
    c_type = STR2CSTR (type);
    c_description = STR2CSTR (description);
    c_string_values = STR2CSTR (string_values);
    c_min = FIX2INT (min);
    c_max = FIX2INT (max);
    c_default_value = STR2CSTR (default_value);
    c_function = STR2CSTR (function);
    
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
                                                           &weechat_ruby_api_config_option_change_cb,
                                                           c_function));
    
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
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_search_option");
        RUBY_RETURN_EMPTY;
    }
    
    c_config_file = NULL;
    c_section = NULL;
    c_option_name = NULL;
    
    if (NIL_P (config_file) || NIL_P (section) || NIL_P (option_name))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_search_option");
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
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_string_to_boolean");
        RUBY_RETURN_INT(0);
    }
    
    c_text = NULL;
    
    if (NIL_P (text))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_string_to_boolean");
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
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_option_reset");
        RUBY_RETURN_INT(0);
    }
    
    c_option = NULL;
    c_run_callback = 0;
    
    if (NIL_P (option) || NIL_P (run_callback))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_option_reset");
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
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_option_set");
        RUBY_RETURN_INT(0);
    }
    
    c_option = NULL;
    c_new_value = NULL;
    c_run_callback = 0;
    
    if (NIL_P (option) || NIL_P (new_value) || NIL_P (run_callback))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_option_set");
        RUBY_RETURN_INT(0);
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
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_option_unset");
        RUBY_RETURN_INT(0);
    }
    
    c_option = NULL;
    
    if (NIL_P (option))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_option_unset");
        RUBY_RETURN_INT(0);
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
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_option_rename");
        RUBY_RETURN_ERROR;
    }
    
    c_option = NULL;
    c_new_name = NULL;
    
    if (NIL_P (option) || NIL_P (new_name))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_option_rename");
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
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_boolean");
        RUBY_RETURN_INT(0);
    }
    
    c_option = NULL;
    
    if (NIL_P (option))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_boolean");
        RUBY_RETURN_INT(0);
    }
    
    Check_Type (option, T_STRING);
    
    c_option = STR2CSTR (option);
    
    value = weechat_config_boolean (script_str2ptr (c_option));
    
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
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_integer");
        RUBY_RETURN_INT(0);
    }
    
    c_option = NULL;
    
    if (NIL_P (option))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_integer");
        RUBY_RETURN_INT(0);
    }
    
    Check_Type (option, T_STRING);
    
    c_option = STR2CSTR (option);
    
    value = weechat_config_integer (script_str2ptr (c_option));
    
    RUBY_RETURN_INT(value);
}

/*
 * weechat_ruby_api_config_string: return string value of option
 */

static VALUE
weechat_ruby_api_config_string (VALUE class, VALUE option)
{
    char *c_option, *value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_string");
        RUBY_RETURN_EMPTY;
    }
    
    c_option = NULL;
    
    if (NIL_P (option))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_string");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (option, T_STRING);
    
    c_option = STR2CSTR (option);
    
    value = weechat_config_string (script_str2ptr (c_option));
    
    RUBY_RETURN_STRING(value);
}

/*
 * weechat_ruby_api_config_color: return color value of option
 */

static VALUE
weechat_ruby_api_config_color (VALUE class, VALUE option)
{
    char *c_option;
    int value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_color");
        RUBY_RETURN_INT(0);
    }
    
    c_option = NULL;
    
    if (NIL_P (option))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_color");
        RUBY_RETURN_INT(0);
    }
    
    Check_Type (option, T_STRING);
    
    c_option = STR2CSTR (option);
    
    value = weechat_config_color (script_str2ptr (c_option));
    
    RUBY_RETURN_INT(value);
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
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_write_line");
        RUBY_RETURN_ERROR;
    }
    
    c_config_file = NULL;
    c_option_name = NULL;
    c_value = NULL;
    
    if (NIL_P (config_file) || NIL_P (option_name) || NIL_P (value))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_write_line");
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
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_write");
        RUBY_RETURN_INT(-1);
    }
    
    c_config_file = NULL;
    
    if (NIL_P (config_file))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_write");
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
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_read");
        RUBY_RETURN_INT(-1);
    }
    
    c_config_file = NULL;
    
    if (NIL_P (config_file))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_read");
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
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_reload");
        RUBY_RETURN_INT(-1);
    }
    
    c_config_file = NULL;
    
    if (NIL_P (config_file))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_reload");
        RUBY_RETURN_INT(-1);
    }
    
    Check_Type (config_file, T_STRING);
    
    c_config_file = STR2CSTR (config_file);
    
    rc = weechat_config_reload (script_str2ptr (c_config_file));
    
    RUBY_RETURN_INT(rc);
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
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_free");
        RUBY_RETURN_ERROR;
    }
    
    c_config_file = NULL;
    
    if (NIL_P (config_file))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_free");
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
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_get");
        RUBY_RETURN_EMPTY;
    }
    
    if (NIL_P (option))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_get");
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
    char *c_option, *value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_get_plugin");
        RUBY_RETURN_EMPTY;
    }
    
    if (NIL_P (option))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_get_plugin");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (option, T_STRING);
    
    c_option = STR2CSTR (option);
    
    value = script_api_config_get_plugin (weechat_ruby_plugin,
                                          ruby_current_script,
                                          c_option);
    
    RUBY_RETURN_STRING(value);
}

/*
 * weechat_ruby_api_config_set_plugin: set value of a plugin option
 */

static VALUE
weechat_ruby_api_config_set_plugin (VALUE class, VALUE option, VALUE value)
{
    char *c_option, *c_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("config_set_plugin");
        RUBY_RETURN_ERROR;
    }
    
    if (NIL_P (option) || NIL_P (value))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("config_set_plugin");
        RUBY_RETURN_ERROR;
    }
    
    Check_Type (option, T_STRING);
    Check_Type (value, T_STRING);
    
    c_option = STR2CSTR (option);
    c_value = STR2CSTR (value);
    
    if (script_api_config_set_plugin (weechat_ruby_plugin,
                                      ruby_current_script,
                                      c_option,
                                      c_value))
        RUBY_RETURN_OK;
    
    RUBY_RETURN_ERROR;
}

/*
 * weechat_ruby_api_prefix: get a prefix, used for display
 */

static VALUE
weechat_ruby_api_prefix (VALUE class, VALUE prefix)
{
    char *c_prefix, *result;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("prefix");
        RUBY_RETURN_EMPTY;
    }
    
    c_prefix = NULL;
    
    if (NIL_P (prefix))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("prefix");
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
    char *c_color, *result;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("color");
        RUBY_RETURN_EMPTY;
    }
    
    c_color = NULL;
    
    if (NIL_P (color))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("color");
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
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("print");
        RUBY_RETURN_ERROR;
    }
    
    c_buffer = NULL;
    c_message = NULL;
    
    if (NIL_P (buffer) || NIL_P (message))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("print");
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
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("print_date_tags");
        RUBY_RETURN_ERROR;
    }
    
    c_buffer = NULL;
    c_date = 0;
    c_tags = NULL;
    c_message = NULL;
    
    if (NIL_P (buffer) || NIL_P (date) || NIL_P (tags) || NIL_P (message))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("print_date_tags");
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
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("print_y");
        RUBY_RETURN_ERROR;
    }
    
    c_buffer = NULL;
    c_y = 0;
    c_message = NULL;
    
    if (NIL_P (buffer) || NIL_P (message))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("print_y");
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
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("log_print");
        RUBY_RETURN_ERROR;
    }
    
    c_message = NULL;
    
    if (NIL_P (message))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("log_print");
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
    char *ruby_argv[3], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    /* make C compiler happy */
    (void) argv;
    
    script_callback = (struct t_script_callback *)data;
    
    ruby_argv[0] = script_ptr2str (buffer);
    ruby_argv[1] = (argc > 1) ? argv_eol[1] : empty_arg;
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
    if (ruby_argv[0])
        free (ruby_argv[0]);
    
    return ret;
}

/*
 * weechat_ruby_api_hook_command: hook a command
 */

static VALUE
weechat_ruby_api_hook_command (VALUE class, VALUE command, VALUE description,
                               VALUE args, VALUE args_description,
                               VALUE completion, VALUE function)
{
    char *c_command, *c_description, *c_args, *c_args_description;
    char *c_completion, *c_function, *result;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_command");
        RUBY_RETURN_EMPTY;
    }
    
    c_command = NULL;
    c_description = NULL;
    c_args = NULL;
    c_args_description = NULL;
    c_completion = NULL;
    c_function = NULL;
    
    if (NIL_P (command) || NIL_P (description) || NIL_P (args)
        || NIL_P (args_description) || NIL_P (completion) || NIL_P (function))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_command");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (command, T_STRING);
    Check_Type (description, T_STRING);
    Check_Type (args, T_STRING);
    Check_Type (args_description, T_STRING);
    Check_Type (completion, T_STRING);
    Check_Type (function, T_STRING);
    
    c_command = STR2CSTR (command);
    c_description = STR2CSTR (description);
    c_args = STR2CSTR (args);
    c_args_description = STR2CSTR (args_description);
    c_completion = STR2CSTR (completion);
    c_function = STR2CSTR (function);
    
    result = script_ptr2str (script_api_hook_command (weechat_ruby_plugin,
                                                      ruby_current_script,
                                                      c_command,
                                                      c_description,
                                                      c_args,
                                                      c_args_description,
                                                      c_completion,
                                                      &weechat_ruby_api_hook_command_cb,
                                                      c_function));
    
    RUBY_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_hook_timer_cb: callback for timer hooked
 */

int
weechat_ruby_api_hook_timer_cb (void *data)
{
    struct t_script_callback *script_callback;
    char *ruby_argv[1];
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    ruby_argv[0] = NULL;
    
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

/*
 * weechat_ruby_api_hook_timer: hook a timer
 */

static VALUE
weechat_ruby_api_hook_timer (VALUE class, VALUE interval, VALUE align_second,
                             VALUE max_calls, VALUE function)
{
    int c_interval, c_align_second, c_max_calls;
    char *c_function, *result;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_timer");
        RUBY_RETURN_EMPTY;
    }
    
    c_interval = 0;
    c_align_second = 0;
    c_max_calls = 0;
    c_function = NULL;
    
    if (NIL_P (interval) || NIL_P (align_second) || NIL_P (max_calls)
        || NIL_P (function))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_timer");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (interval, T_FIXNUM);
    Check_Type (align_second, T_FIXNUM);
    Check_Type (max_calls, T_FIXNUM);
    Check_Type (function, T_STRING);
    
    c_interval = FIX2INT (interval);
    c_align_second = FIX2INT (align_second);
    c_max_calls = FIX2INT (max_calls);
    c_function = STR2CSTR (function);
    
    result = script_ptr2str (script_api_hook_timer (weechat_ruby_plugin,
                                                    ruby_current_script,
                                                    c_interval,
                                                    c_align_second,
                                                    c_max_calls,
                                                    &weechat_ruby_api_hook_timer_cb,
                                                    c_function));
    
    RUBY_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_hook_fd_cb: callback for fd hooked
 */

int
weechat_ruby_api_hook_fd_cb (void *data)
{
    struct t_script_callback *script_callback;
    char *ruby_argv[1];
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    ruby_argv[0] = NULL;
    
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

/*
 * weechat_ruby_api_hook_fd: hook a fd
 */

static VALUE
weechat_ruby_api_hook_fd (VALUE class, VALUE fd, VALUE read, VALUE write,
                          VALUE exception, VALUE function)
{
    int c_fd, c_read, c_write, c_exception;
    char *c_function, *result;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_fd");
        RUBY_RETURN_EMPTY;
    }
    
    c_fd = 0;
    c_read = 0;
    c_write = 0;
    c_exception = 0;
    c_function = NULL;
    
    if (NIL_P (fd) || NIL_P (read) || NIL_P (write) || NIL_P (exception)
        || NIL_P (function))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_fd");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (fd, T_FIXNUM);
    Check_Type (read, T_FIXNUM);
    Check_Type (write, T_FIXNUM);
    Check_Type (exception, T_FIXNUM);
    Check_Type (function, T_STRING);
    
    c_fd = FIX2INT (fd);
    c_read = FIX2INT (read);
    c_write = FIX2INT (write);
    c_exception = FIX2INT (exception);
    c_function = STR2CSTR (function);
    
    result = script_ptr2str (script_api_hook_fd (weechat_ruby_plugin,
                                                 ruby_current_script,
                                                 c_fd,
                                                 c_read,
                                                 c_write,
                                                 c_exception,
                                                 &weechat_ruby_api_hook_fd_cb,
                                                 c_function));
    
    RUBY_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_hook_print_cb: callback for print hooked
 */

int
weechat_ruby_api_hook_print_cb (void *data, struct t_gui_buffer *buffer,
                                time_t date, int tags_count, char **tags,
                                const char *prefix, const char *message)
{
    struct t_script_callback *script_callback;
    char *ruby_argv[6];
    static char timebuffer[64];
    int *rc, ret;
    
    /* make C compiler happy */
    (void) tags_count;
    
    script_callback = (struct t_script_callback *)data;
    
    snprintf (timebuffer, sizeof (timebuffer) - 1, "%ld", (long int)date);
    
    ruby_argv[0] = script_ptr2str (buffer);
    ruby_argv[1] = timebuffer;
    ruby_argv[2] = weechat_string_build_with_exploded (tags, ",");
    ruby_argv[3] = (char *)prefix;
    ruby_argv[4] = (char *)message;
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
    if (ruby_argv[0])
        free (ruby_argv[0]);
    if (ruby_argv[2])
        free (ruby_argv[2]);
    
    return ret;
}

/*
 * weechat_ruby_api_hook_print: hook a print
 */

static VALUE
weechat_ruby_api_hook_print (VALUE class, VALUE buffer, VALUE tags,
                             VALUE message, VALUE strip_colors, VALUE function)
{
    char *c_buffer, *c_tags, *c_message, *c_function, *result;
    int c_strip_colors;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_print");
        RUBY_RETURN_EMPTY;
    }
    
    c_buffer = NULL;
    c_tags = NULL;
    c_message = NULL;
    c_strip_colors = 0;
    c_function = NULL;
    
    if (NIL_P (buffer) || NIL_P (tags) || NIL_P (message)
        || NIL_P (strip_colors) || NIL_P (function))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_print");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (buffer, T_STRING);
    Check_Type (tags, T_STRING);
    Check_Type (message, T_STRING);
    Check_Type (strip_colors, T_FIXNUM);
    Check_Type (function, T_STRING);
    
    c_buffer = STR2CSTR (buffer);
    c_tags = STR2CSTR (tags);
    c_message = STR2CSTR (message);
    c_strip_colors = FIX2INT (strip_colors);
    c_function = STR2CSTR (function);
    
    result = script_ptr2str (script_api_hook_print (weechat_ruby_plugin,
                                                    ruby_current_script,
                                                    script_str2ptr (c_buffer),
                                                    c_tags,
                                                    c_message,
                                                    c_strip_colors,
                                                    &weechat_ruby_api_hook_print_cb,
                                                    c_function));
    
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
    char *ruby_argv[3];
    static char value_str[64], empty_value[1] = { '\0' };
    int *rc, ret, free_needed;
    
    script_callback = (struct t_script_callback *)data;

    ruby_argv[0] = (char *)signal;
    free_needed = 0;
    if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_STRING) == 0)
    {
        ruby_argv[1] = (signal_data) ? (char *)signal_data : empty_value;
    }
    else if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_INT) == 0)
    {
        snprintf (value_str, sizeof (value_str) - 1,
                  "%d", *((int *)signal_data));
        ruby_argv[1] = value_str;
    }
    else if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_POINTER) == 0)
    {
        ruby_argv[1] = script_ptr2str (signal_data);
        free_needed = 1;
    }
    else
        ruby_argv[1] = NULL;
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
    if (free_needed && ruby_argv[1])
        free (ruby_argv[1]);
    
    return ret;
}

/*
 * weechat_ruby_api_hook_signal: hook a signal
 */

static VALUE
weechat_ruby_api_hook_signal (VALUE class, VALUE signal, VALUE function)
{
    char *c_signal, *c_function, *result;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_signal");
        RUBY_RETURN_EMPTY;
    }
    
    c_signal = NULL;
    c_function = NULL;
    
    if (NIL_P (signal) || NIL_P (function))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_signal");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (signal, T_STRING);
    Check_Type (function, T_STRING);
    
    c_signal = STR2CSTR (signal);
    c_function = STR2CSTR (function);
    
    result = script_ptr2str (script_api_hook_signal (weechat_ruby_plugin,
                                                     ruby_current_script,
                                                     c_signal,
                                                     &weechat_ruby_api_hook_signal_cb,
                                                     c_function));
    
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
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_signal_send");
        RUBY_RETURN_ERROR;
    }
    
    c_signal = NULL;
    c_type_data = NULL;
    c_signal_data = NULL;
    
    if (NIL_P (signal) || NIL_P (type_data) || NIL_P (signal_data))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_signal_send");
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
    char *ruby_argv[3];
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    ruby_argv[0] = (char *)option;
    ruby_argv[1] = (char *)value;
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

/*
 * weechat_ruby_api_hook_config: hook a config option
 */

static VALUE
weechat_ruby_api_hook_config (VALUE class, VALUE option, VALUE function)
{
    char *c_option, *c_function, *result;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_config");
        RUBY_RETURN_EMPTY;
    }
    
    c_option = NULL;
    c_function = NULL;
    
    if (NIL_P (option) || NIL_P (function))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_config");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (option, T_STRING);
    Check_Type (function, T_STRING);
    
    c_option = STR2CSTR (option);
    c_function = STR2CSTR (function);
    
    result = script_ptr2str (script_api_hook_config (weechat_ruby_plugin,
                                                     ruby_current_script,
                                                     c_option,
                                                     &weechat_ruby_api_hook_config_cb,
                                                     c_function));
    
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
    char *ruby_argv[4];
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    ruby_argv[0] = (char *)completion_item;
    ruby_argv[1] = script_ptr2str (buffer);
    ruby_argv[2] = script_ptr2str (completion);
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
    if (ruby_argv[2])
        free (ruby_argv[2]);
    
    return ret;
}

/*
 * weechat_ruby_api_hook_completion: hook a completion
 */

static VALUE
weechat_ruby_api_hook_completion (VALUE class, VALUE completion,
                                  VALUE function)
{
    char *c_completion, *c_function, *result;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_completion");
        RUBY_RETURN_EMPTY;
    }
    
    c_completion = NULL;
    c_function = NULL;
    
    if (NIL_P (completion) || NIL_P (function))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_completion");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (completion, T_STRING);
    Check_Type (function, T_STRING);
    
    c_completion = STR2CSTR (completion);
    c_function = STR2CSTR (function);
    
    result = script_ptr2str (script_api_hook_completion (weechat_ruby_plugin,
                                                         ruby_current_script,
                                                         c_completion,
                                                         &weechat_ruby_api_hook_completion_cb,
                                                         c_function));
    
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
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_completion_list_add");
        RUBY_RETURN_ERROR;
    }
    
    c_completion = NULL;
    c_word = NULL;
    c_nick_completion = 0;
    c_where = NULL;
    
    if (NIL_P (completion) || NIL_P (word) || NIL_P (nick_completion)
        || NIL_P (where))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_completion_list_add");
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
    char *ruby_argv[4];
    
    script_callback = (struct t_script_callback *)data;
    
    ruby_argv[0] = (char *)modifier;
    ruby_argv[1] = (char *)modifier_data;
    ruby_argv[2] = (char *)string;
    ruby_argv[3] = NULL;
    
    return (char *)weechat_ruby_exec (script_callback->script,
                                      WEECHAT_SCRIPT_EXEC_STRING,
                                      script_callback->function,
                                      ruby_argv);
}

/*
 * weechat_ruby_api_hook_modifier: hook a modifier
 */

static VALUE
weechat_ruby_api_hook_modifier (VALUE class, VALUE modifier, VALUE function)
{
    char *c_modifier, *c_function, *result;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_modifier");
        RUBY_RETURN_EMPTY;
    }
    
    c_modifier = NULL;
    c_function = NULL;
    
    if (NIL_P (modifier) || NIL_P (function))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_modifier");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (modifier, T_STRING);
    Check_Type (function, T_STRING);
    
    c_modifier = STR2CSTR (modifier);
    c_function = STR2CSTR (function);
    
    result = script_ptr2str (script_api_hook_modifier (weechat_ruby_plugin,
                                                       ruby_current_script,
                                                       c_modifier,
                                                       &weechat_ruby_api_hook_modifier_cb,
                                                       c_function));
    
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
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("hook_modifier_exec");
        RUBY_RETURN_EMPTY;
    }
    
    c_modifier = NULL;
    c_modifier_data = NULL;
    c_string = NULL;
    
    if (NIL_P (modifier) || NIL_P (modifier_data) || NIL_P (string))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("hook_modifier_exec");
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
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("unhook");
        RUBY_RETURN_ERROR;
    }
    
    c_hook = NULL;
    
    if (NIL_P (hook))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("unhook");
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
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("unhook_all");
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
    char *ruby_argv[3];
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    ruby_argv[0] = script_ptr2str (buffer);
    ruby_argv[1] = (char *)input_data;
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
    if (ruby_argv[0])
        free (ruby_argv[0]);
    
    return ret;
}

/*
 * weechat_ruby_api_buffer_close_cb: callback for closed buffer
 */

int
weechat_ruby_api_buffer_close_cb (void *data, struct t_gui_buffer *buffer)
{
    struct t_script_callback *script_callback;
    char *ruby_argv[2];
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    ruby_argv[0] = script_ptr2str (buffer);
    ruby_argv[1] = NULL;
    
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
    if (ruby_argv[0])
        free (ruby_argv[0]);
    
    return ret;
}

/*
 * weechat_ruby_api_buffer_new: create a new buffer
 */

static VALUE
weechat_ruby_api_buffer_new (VALUE class, VALUE category, VALUE name,
                             VALUE function_input, VALUE function_close)
{
    char *c_category, *c_name, *c_function_input, *c_function_close, *result;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("buffer_new");
        RUBY_RETURN_EMPTY;
    }
    
    c_category = NULL;
    c_name = NULL;
    c_function_input = NULL;
    c_function_close = NULL;
    
    if (NIL_P (category) || NIL_P (name) || NIL_P (function_input)
        || NIL_P (function_close))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("buffer_new");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (category, T_STRING);
    Check_Type (name, T_STRING);
    Check_Type (function_input, T_STRING);
    Check_Type (function_close, T_STRING);
    
    c_category = STR2CSTR (category);
    c_name = STR2CSTR (name);
    c_function_input = STR2CSTR (function_input);
    c_function_close = STR2CSTR (function_close);
    
    result = script_ptr2str (script_api_buffer_new (weechat_ruby_plugin,
                                                    ruby_current_script,
                                                    c_category,
                                                    c_name,
                                                    &weechat_ruby_api_buffer_input_data_cb,
                                                    c_function_input,
                                                    &weechat_ruby_api_buffer_close_cb,
                                                    c_function_close));
    
    RUBY_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_buffer_search: search a buffer
 */

static VALUE
weechat_ruby_api_buffer_search (VALUE class, VALUE category, VALUE name)
{
    char *c_category, *c_name, *result;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("buffer_search");
        RUBY_RETURN_EMPTY;
    }
    
    c_category = NULL;
    c_name = NULL;
    
    if (NIL_P (category) || NIL_P (name))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("buffer_search");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (category, T_STRING);
    Check_Type (name, T_STRING);
    
    c_category = STR2CSTR (category);
    c_name = STR2CSTR (name);
    
    result = script_ptr2str (weechat_buffer_search (c_category, c_name));
    
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
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("buffer_clear");
        RUBY_RETURN_ERROR;
    }
    
    c_buffer = NULL;
    
    if (NIL_P (buffer))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("buffer_clear");
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
weechat_ruby_api_buffer_close (VALUE class, VALUE buffer,
                               VALUE switch_to_another)
{
    char *c_buffer;
    int c_switch_to_another;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("buffer_close");
        RUBY_RETURN_ERROR;
    }
    
    c_buffer = NULL;
    c_switch_to_another = 0;
    
    if (NIL_P (buffer) || NIL_P (switch_to_another))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("buffer_close");
        RUBY_RETURN_ERROR;
    }
    
    Check_Type (buffer, T_STRING);
    Check_Type (switch_to_another, T_FIXNUM);
    
    c_buffer = STR2CSTR (buffer);
    c_switch_to_another = FIX2INT (switch_to_another);
    
    script_api_buffer_close (weechat_ruby_plugin,
                             ruby_current_script,
                             script_str2ptr (c_buffer),
                             c_switch_to_another);
    
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
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("buffer_get_integer");
        RUBY_RETURN_INT(-1);
    }
    
    if (NIL_P (buffer) || NIL_P (property))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("buffer_get_integer");
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
    char *c_buffer, *c_property, *value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("buffer_get_string");
        RUBY_RETURN_EMPTY;
    }
    
    if (NIL_P (buffer) || NIL_P (property))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("buffer_get_string");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (buffer, T_STRING);
    Check_Type (property, T_STRING);
    
    c_buffer = STR2CSTR (buffer);
    c_property = STR2CSTR (property);

    value = weechat_buffer_get_string (script_str2ptr (c_buffer),
                                       c_property);
    
    RUBY_RETURN_STRING(value);
}

/*
 * weechat_ruby_api_buffer_get_pointer: get a buffer property as pointer
 */

static VALUE
weechat_ruby_api_buffer_get_pointer (VALUE class, VALUE buffer, VALUE property)
{
    char *c_buffer, *c_property, *value;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("buffer_get_pointer");
        RUBY_RETURN_EMPTY;
    }
    
    if (NIL_P (buffer) || NIL_P (property))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("buffer_get_pointer");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (buffer, T_STRING);
    Check_Type (property, T_STRING);
    
    c_buffer = STR2CSTR (buffer);
    c_property = STR2CSTR (property);

    value = script_ptr2str (weechat_buffer_get_string (script_str2ptr (c_buffer),
                                                       c_property));
    
    RUBY_RETURN_STRING_FREE(value);
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
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("buffer_set");
        RUBY_RETURN_ERROR;
    }
    
    if (NIL_P (buffer) || NIL_P (property) || NIL_P (value))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("buffer_set");
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
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("nicklist_add_group");
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
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("nicklist_add_group");
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
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("nicklist_search_group");
        RUBY_RETURN_EMPTY;
    }
    
    c_buffer = NULL;
    c_from_group = NULL;
    c_name = NULL;
    
    if (NIL_P (buffer) || NIL_P (from_group) || NIL_P (name))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("nicklist_search_group");
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
    char *c_buffer, *c_group, *c_name, *c_color, *c_prefix, char_prefix;
    char *c_prefix_color, *result;
    int c_visible;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("nicklist_add_nick");
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
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("nicklist_add_nick");
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
    
    if (c_prefix && c_prefix[0])
        char_prefix = c_prefix[0];
    else
        char_prefix = ' ';
    
    result = script_ptr2str (weechat_nicklist_add_nick (script_str2ptr (c_buffer),
                                                        script_str2ptr (c_group),
                                                        c_name,
                                                        c_color,
                                                        char_prefix,
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
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("nicklist_search_nick");
        RUBY_RETURN_EMPTY;
    }
    
    c_buffer = NULL;
    c_from_group = NULL;
    c_name = NULL;
    
    if (NIL_P (buffer) || NIL_P (from_group) || NIL_P (name))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("nicklist_search_nick");
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
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("nicklist_remove_group");
        RUBY_RETURN_ERROR;
    }
    
    if (NIL_P (buffer) || NIL_P (group))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("nicklist_remove_group");
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
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("nicklist_remove_nick");
        RUBY_RETURN_ERROR;
    }
    
    if (NIL_P (buffer) || NIL_P (nick))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("nicklist_remove_nick");
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
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("nicklist_remove_all");
        RUBY_RETURN_ERROR;
    }
    
    if (NIL_P (buffer))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("nicklist_remove_all");
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
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("bar_item_search");
        RUBY_RETURN_EMPTY;
    }
    
    c_name = NULL;
    
    if (NIL_P (name))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("bar_item_search");
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
                                    struct t_gui_window *window,
                                    int max_width, int max_height)
{
    struct t_script_callback *script_callback;
    char *ruby_argv[5], *ret;
    static char str_width[32], str_height[32];
    
    script_callback = (struct t_script_callback *)data;

    snprintf (str_width, sizeof (str_width), "%d", max_width);
    snprintf (str_height, sizeof (str_height), "%d", max_height);
    
    ruby_argv[0] = script_ptr2str (item);
    ruby_argv[1] = script_ptr2str (window);
    ruby_argv[2] = str_width;
    ruby_argv[3] = str_height;
    ruby_argv[4] = NULL;
    
    ret = (char *)weechat_ruby_exec (script_callback->script,
                                     WEECHAT_SCRIPT_EXEC_STRING,
                                     script_callback->function,
                                     ruby_argv);
    
    if (ruby_argv[0])
        free (ruby_argv[0]);
    if (ruby_argv[1])
        free (ruby_argv[1]);
    
    return ret;
}

/*
 * weechat_ruby_api_bar_item_new: add a new bar item
 */

static VALUE
weechat_ruby_api_bar_item_new (VALUE class, VALUE name, VALUE function)
{
    char *c_name, *c_function, *result;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("bar_item_new");
        RUBY_RETURN_EMPTY;
    }
    
    c_name = NULL;
    c_function = NULL;
    
    if (NIL_P (name) || NIL_P (function))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("bar_item_new");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (name, T_STRING);
    Check_Type (function, T_STRING);
    
    c_name = STR2CSTR (name);
    c_function = STR2CSTR (function);
    
    result = script_ptr2str (script_api_bar_item_new (weechat_ruby_plugin,
                                                      ruby_current_script,
                                                      c_name,
                                                      &weechat_ruby_api_bar_item_build_cb,
                                                      c_function));
    
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
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("bar_item_update");
        RUBY_RETURN_ERROR;
    }
    
    if (NIL_P (name))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("bar_item_update");
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
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("bar_item_remove");
        RUBY_RETURN_ERROR;
    }
    
    if (NIL_P (item))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("bar_item_remove");
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
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("bar_search");
        RUBY_RETURN_EMPTY;
    }
    
    c_name = NULL;
    
    if (NIL_P (name))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("bar_search");
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
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("bar_new");
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
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("bar_new");
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
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("bar_set");
        RUBY_RETURN_ERROR;
    }
    
    if (NIL_P (bar) || NIL_P (property) || NIL_P (value))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("bar_set");
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
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("bar_update");
        RUBY_RETURN_ERROR;
    }
    
    if (NIL_P (name))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("bar_update");
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
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("bar_remove");
        RUBY_RETURN_ERROR;
    }
    
    if (NIL_P (bar))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("bar_remove");
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
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("command");
        RUBY_RETURN_ERROR;
    }
    
    if (NIL_P (buffer) || NIL_P (command))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("command");
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
weechat_ruby_api_info_get (VALUE class, VALUE info)
{
    char *c_info, *value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("info_get");
        RUBY_RETURN_EMPTY;
    }
    
    if (NIL_P (info))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("info_get");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (info, T_STRING);
    
    c_info = STR2CSTR (info);

    value = weechat_info_get (c_info);
    
    RUBY_RETURN_STRING(value);
}

/*
 * weechat_ruby_api_infolist_get: get list with infos
 */

static VALUE
weechat_ruby_api_infolist_get (VALUE class, VALUE name, VALUE pointer,
                               VALUE arguments)
{
    char *c_name, *c_pointer, *c_arguments, *value;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("infolist_get");
        RUBY_RETURN_EMPTY;
    }
    
    if (NIL_P (name) || NIL_P (pointer))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("infolist_get");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (name, T_STRING);
    Check_Type (pointer, T_STRING);
    Check_Type (arguments, T_STRING);
    
    c_name = STR2CSTR (name);
    c_pointer = STR2CSTR (pointer);
    c_arguments = STR2CSTR (arguments);
    
    value = script_ptr2str (weechat_infolist_get (c_name,
                                                  script_str2ptr (c_pointer),
                                                  c_arguments));
    
    RUBY_RETURN_STRING_FREE(value);
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
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("infolist_next");
        RUBY_RETURN_INT(0);
    }
    
    if (NIL_P (infolist))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("infolist_next");
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
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("infolist_prev");
        RUBY_RETURN_INT(0);
    }
    
    if (NIL_P (infolist))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("infolist_prev");
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
    char *c_infolist, *value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("infolist_fields");
        RUBY_RETURN_EMPTY;
    }
    
    if (NIL_P (infolist))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("infolist_fields");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (infolist, T_STRING);
    
    c_infolist = STR2CSTR (infolist);
    
    value = weechat_infolist_fields (script_str2ptr (c_infolist));
    
    RUBY_RETURN_STRING(value);
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
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("infolist_integer");
        RUBY_RETURN_INT(0);
    }
    
    if (NIL_P (infolist) || NIL_P (variable))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("infolist_integer");
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
    char *c_infolist, *c_variable, *value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("infolist_string");
        RUBY_RETURN_EMPTY;
    }
    
    if (NIL_P (infolist) || NIL_P (variable))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("infolist_string");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (infolist, T_STRING);
    Check_Type (variable, T_STRING);
    
    c_infolist = STR2CSTR (infolist);
    c_variable = STR2CSTR (variable);
    
    value = weechat_infolist_string (script_str2ptr (c_infolist), c_variable);
    
    RUBY_RETURN_STRING(value);
}

/*
 * weechat_ruby_api_infolist_pointer: get pointer value of a variable in infolist
 */

static VALUE
weechat_ruby_api_infolist_pointer (VALUE class, VALUE infolist, VALUE variable)
{
    char *c_infolist, *c_variable, *value;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("infolist_pointer");
        RUBY_RETURN_EMPTY;
    }
    
    if (NIL_P (infolist) || NIL_P (variable))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("infolist_pointer");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (infolist, T_STRING);
    Check_Type (variable, T_STRING);
    
    c_infolist = STR2CSTR (infolist);
    c_variable = STR2CSTR (variable);
    
    value = script_ptr2str (weechat_infolist_pointer (script_str2ptr (c_infolist), c_variable));
    
    RUBY_RETURN_STRING_FREE(value);
}

/*
 * weechat_ruby_api_infolist_time: get time value of a variable in infolist
 */

static VALUE
weechat_ruby_api_infolist_time (VALUE class, VALUE infolist, VALUE variable)
{
    char *c_infolist, *c_variable, timebuffer[64], *value;
    time_t time;
    VALUE return_value;
    
    /* make C compiler happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("infolist_time");
        RUBY_RETURN_EMPTY;
    }
    
    if (NIL_P (infolist) || NIL_P (variable))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("infolist_time");
        RUBY_RETURN_EMPTY;
    }
    
    Check_Type (infolist, T_STRING);
    Check_Type (variable, T_STRING);
    
    c_infolist = STR2CSTR (infolist);
    c_variable = STR2CSTR (variable);
    
    time = weechat_infolist_time (script_str2ptr (c_infolist), c_variable);
    strftime (timebuffer, sizeof (timebuffer), "%F %T", localtime (&time));
    value = strdup (timebuffer);
    
    RUBY_RETURN_STRING_FREE(value);
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
        WEECHAT_SCRIPT_MSG_NOT_INITIALIZED("infolist_free");
        RUBY_RETURN_ERROR;
    }
    
    if (NIL_P (infolist))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS("infolist_free");
        RUBY_RETURN_ERROR;
    }
    
    Check_Type (infolist, T_STRING);
    
    c_infolist = STR2CSTR (infolist);
    
    weechat_infolist_free (script_str2ptr (c_infolist));
    
    RUBY_RETURN_OK;
}

/*
 * weechat_ruby_api_init: init Ruby API: add variables and functions
 */

void
weechat_ruby_api_init (VALUE ruby_mWeechat)
{
    rb_define_const(ruby_mWeechat, "WEECHAT_RC_OK", INT2NUM(WEECHAT_RC_OK));
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
    
    rb_define_const(ruby_mWeechat, "WEECHAT_HOOK_SIGNAL_STRING", rb_str_new2(WEECHAT_HOOK_SIGNAL_STRING));
    rb_define_const(ruby_mWeechat, "WEECHAT_HOOK_SIGNAL_INT", rb_str_new2(WEECHAT_HOOK_SIGNAL_INT));
    rb_define_const(ruby_mWeechat, "WEECHAT_HOOK_SIGNAL_POINTER", rb_str_new2(WEECHAT_HOOK_SIGNAL_POINTER));
    
    rb_define_module_function (ruby_mWeechat, "register", &weechat_ruby_api_register, 7);
    rb_define_module_function (ruby_mWeechat, "charset_set", &weechat_ruby_api_charset_set, 1);
    rb_define_module_function (ruby_mWeechat, "iconv_to_internal", &weechat_ruby_api_iconv_to_internal, 2);
    rb_define_module_function (ruby_mWeechat, "iconv_from_internal", &weechat_ruby_api_iconv_from_internal, 2);
    rb_define_module_function (ruby_mWeechat, "gettext", &weechat_ruby_api_gettext, 1);
    rb_define_module_function (ruby_mWeechat, "ngettext", &weechat_ruby_api_ngettext, 3);
    rb_define_module_function (ruby_mWeechat, "mkdir_home", &weechat_ruby_api_mkdir_home, 2);
    rb_define_module_function (ruby_mWeechat, "mkdir", &weechat_ruby_api_mkdir, 2);
    rb_define_module_function (ruby_mWeechat, "list_new", &weechat_ruby_api_list_new, 0);
    rb_define_module_function (ruby_mWeechat, "list_add", &weechat_ruby_api_list_add, 3);
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
    rb_define_module_function (ruby_mWeechat, "config_new", &weechat_ruby_api_config_new, 2);
    rb_define_module_function (ruby_mWeechat, "config_new_section", &weechat_ruby_api_config_new_section, 8);
    rb_define_module_function (ruby_mWeechat, "config_search_section", &weechat_ruby_api_config_search_section, 2);
    rb_define_module_function (ruby_mWeechat, "config_new_option", &weechat_ruby_api_config_new_option, 10);
    rb_define_module_function (ruby_mWeechat, "config_search_option", &weechat_ruby_api_config_search_option, 3);
    rb_define_module_function (ruby_mWeechat, "config_string_to_boolean", &weechat_ruby_api_config_string_to_boolean, 1);
    rb_define_module_function (ruby_mWeechat, "config_option_reset", &weechat_ruby_api_config_option_reset, 2);
    rb_define_module_function (ruby_mWeechat, "config_option_set", &weechat_ruby_api_config_option_set, 3);
    rb_define_module_function (ruby_mWeechat, "config_option_unset", &weechat_ruby_api_config_option_unset, 1);
    rb_define_module_function (ruby_mWeechat, "config_option_rename", &weechat_ruby_api_config_option_rename, 2);
    rb_define_module_function (ruby_mWeechat, "config_boolean", &weechat_ruby_api_config_boolean, 1);
    rb_define_module_function (ruby_mWeechat, "config_integer", &weechat_ruby_api_config_integer, 1);
    rb_define_module_function (ruby_mWeechat, "config_string", &weechat_ruby_api_config_string, 1);
    rb_define_module_function (ruby_mWeechat, "config_color", &weechat_ruby_api_config_color, 1);
    rb_define_module_function (ruby_mWeechat, "config_write_line", &weechat_ruby_api_config_write_line, 3);
    rb_define_module_function (ruby_mWeechat, "config_write", &weechat_ruby_api_config_write, 1);
    rb_define_module_function (ruby_mWeechat, "config_read", &weechat_ruby_api_config_read, 1);
    rb_define_module_function (ruby_mWeechat, "config_reload", &weechat_ruby_api_config_reload, 1);
    rb_define_module_function (ruby_mWeechat, "config_free", &weechat_ruby_api_config_free, 1);
    rb_define_module_function (ruby_mWeechat, "config_get", &weechat_ruby_api_config_get, 1);
    rb_define_module_function (ruby_mWeechat, "config_get_plugin", &weechat_ruby_api_config_get_plugin, 1);
    rb_define_module_function (ruby_mWeechat, "config_set_plugin", &weechat_ruby_api_config_set_plugin, 2);
    rb_define_module_function (ruby_mWeechat, "prefix", &weechat_ruby_api_prefix, 1);
    rb_define_module_function (ruby_mWeechat, "color", &weechat_ruby_api_color, 1);
    rb_define_module_function (ruby_mWeechat, "print", &weechat_ruby_api_print, 2);
    rb_define_module_function (ruby_mWeechat, "print_date_tags", &weechat_ruby_api_print_date_tags, 4);
    rb_define_module_function (ruby_mWeechat, "print_y", &weechat_ruby_api_print_y, 3);
    rb_define_module_function (ruby_mWeechat, "log_print", &weechat_ruby_api_log_print, 1);
    rb_define_module_function (ruby_mWeechat, "hook_command", &weechat_ruby_api_hook_command, 6);
    rb_define_module_function (ruby_mWeechat, "hook_timer", &weechat_ruby_api_hook_timer, 4);
    rb_define_module_function (ruby_mWeechat, "hook_fd", &weechat_ruby_api_hook_fd, 5);
    rb_define_module_function (ruby_mWeechat, "hook_print", &weechat_ruby_api_hook_print, 5);
    rb_define_module_function (ruby_mWeechat, "hook_signal", &weechat_ruby_api_hook_signal, 2);
    rb_define_module_function (ruby_mWeechat, "hook_signal_send", &weechat_ruby_api_hook_signal_send, 3);
    rb_define_module_function (ruby_mWeechat, "hook_config", &weechat_ruby_api_hook_config, 2);
    rb_define_module_function (ruby_mWeechat, "hook_completion", &weechat_ruby_api_hook_completion, 2);
    rb_define_module_function (ruby_mWeechat, "hook_completion_list_add", &weechat_ruby_api_hook_completion_list_add, 4);
    rb_define_module_function (ruby_mWeechat, "hook_modifier", &weechat_ruby_api_hook_modifier, 2);
    rb_define_module_function (ruby_mWeechat, "hook_modifier_exec", &weechat_ruby_api_hook_modifier_exec, 3);
    rb_define_module_function (ruby_mWeechat, "unhook", &weechat_ruby_api_unhook, 1);
    rb_define_module_function (ruby_mWeechat, "unhook_all", &weechat_ruby_api_unhook_all, 0);
    rb_define_module_function (ruby_mWeechat, "buffer_new", &weechat_ruby_api_buffer_new, 4);
    rb_define_module_function (ruby_mWeechat, "buffer_search", &weechat_ruby_api_buffer_search, 2);
    rb_define_module_function (ruby_mWeechat, "buffer_clear", &weechat_ruby_api_buffer_clear, 1);
    rb_define_module_function (ruby_mWeechat, "buffer_close", &weechat_ruby_api_buffer_close, 1);
    rb_define_module_function (ruby_mWeechat, "buffer_get_integer", &weechat_ruby_api_buffer_get_integer, 2);
    rb_define_module_function (ruby_mWeechat, "buffer_get_string", &weechat_ruby_api_buffer_get_string, 2);
    rb_define_module_function (ruby_mWeechat, "buffer_get_pointer", &weechat_ruby_api_buffer_get_pointer, 2);
    rb_define_module_function (ruby_mWeechat, "buffer_set", &weechat_ruby_api_buffer_set, 3);
    rb_define_module_function (ruby_mWeechat, "nicklist_add_group", &weechat_ruby_api_nicklist_add_group, 5);
    rb_define_module_function (ruby_mWeechat, "nicklist_search_group", &weechat_ruby_api_nicklist_search_group, 3);
    rb_define_module_function (ruby_mWeechat, "nicklist_add_nick", &weechat_ruby_api_nicklist_add_nick, 7);
    rb_define_module_function (ruby_mWeechat, "nicklist_search_nick", &weechat_ruby_api_nicklist_search_nick, 3);
    rb_define_module_function (ruby_mWeechat, "nicklist_remove_group", &weechat_ruby_api_nicklist_remove_group, 2);
    rb_define_module_function (ruby_mWeechat, "nicklist_remove_nick", &weechat_ruby_api_nicklist_remove_nick, 2);
    rb_define_module_function (ruby_mWeechat, "nicklist_remove_all", &weechat_ruby_api_nicklist_remove_all, 1);
    rb_define_module_function (ruby_mWeechat, "bar_item_search", &weechat_ruby_api_bar_item_search, 1);
    rb_define_module_function (ruby_mWeechat, "bar_item_new", &weechat_ruby_api_bar_item_new, 2);
    rb_define_module_function (ruby_mWeechat, "bar_item_update", &weechat_ruby_api_bar_item_update, 1);
    rb_define_module_function (ruby_mWeechat, "bar_item_remove", &weechat_ruby_api_bar_item_remove, 1);
    rb_define_module_function (ruby_mWeechat, "bar_search", &weechat_ruby_api_bar_search, 1);
    rb_define_module_function (ruby_mWeechat, "bar_new", &weechat_ruby_api_bar_new, 15);
    rb_define_module_function (ruby_mWeechat, "bar_set", &weechat_ruby_api_bar_set, 3);
    rb_define_module_function (ruby_mWeechat, "bar_update", &weechat_ruby_api_bar_update, 1);
    rb_define_module_function (ruby_mWeechat, "bar_remove", &weechat_ruby_api_bar_remove, 1);
    rb_define_module_function (ruby_mWeechat, "command", &weechat_ruby_api_command, 2);
    rb_define_module_function (ruby_mWeechat, "info_get", &weechat_ruby_api_info_get, 1);
    rb_define_module_function (ruby_mWeechat, "infolist_get", &weechat_ruby_api_infolist_get, 3);
    rb_define_module_function (ruby_mWeechat, "infolist_next", &weechat_ruby_api_infolist_next, 1);
    rb_define_module_function (ruby_mWeechat, "infolist_prev", &weechat_ruby_api_infolist_prev, 1);
    rb_define_module_function (ruby_mWeechat, "infolist_fields", &weechat_ruby_api_infolist_fields, 1);
    rb_define_module_function (ruby_mWeechat, "infolist_integer", &weechat_ruby_api_infolist_integer, 2);
    rb_define_module_function (ruby_mWeechat, "infolist_string", &weechat_ruby_api_infolist_string, 2);
    rb_define_module_function (ruby_mWeechat, "infolist_pointer", &weechat_ruby_api_infolist_pointer, 2);
    rb_define_module_function (ruby_mWeechat, "infolist_time", &weechat_ruby_api_infolist_time, 2);
    rb_define_module_function (ruby_mWeechat, "infolist_free", &weechat_ruby_api_infolist_free, 1);
}
