/*
 * Copyright (C) 2003-2012 Sebastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * weechat-ruby-api.c: ruby API functions
 */

#undef _

#include <ruby.h>
#include <time.h>

#include "../weechat-plugin.h"
#include "../plugin-script.h"
#include "../plugin-script-api.h"
#include "../plugin-script-callback.h"
#include "weechat-ruby.h"


#define API_FUNC(__init, __name, __ret)                                 \
    char *ruby_function_name = __name;                                  \
    (void) class;                                                       \
    if (__init                                                          \
        && (!ruby_current_script || !ruby_current_script->name))        \
    {                                                                   \
        WEECHAT_SCRIPT_MSG_NOT_INIT(RUBY_CURRENT_SCRIPT_NAME,           \
                                    ruby_function_name);                \
        __ret;                                                          \
    }
#define API_WRONG_ARGS(__ret)                                           \
    {                                                                   \
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(RUBY_CURRENT_SCRIPT_NAME,         \
                                      ruby_function_name);              \
        __ret;                                                          \
    }
#define API_PTR2STR(__pointer)                                          \
    plugin_script_ptr2str (__pointer)
#define API_STR2PTR(__string)                                           \
    plugin_script_str2ptr (weechat_ruby_plugin,                         \
                           RUBY_CURRENT_SCRIPT_NAME,                    \
                           ruby_function_name, __string)
#define API_RETURN_OK return INT2FIX (1);
#define API_RETURN_ERROR return INT2FIX (0);
#define API_RETURN_EMPTY return Qnil;
#define API_RETURN_STRING(__string)                                     \
    if (__string)                                                       \
        return rb_str_new2 (__string);                                  \
    return rb_str_new2 ("")
#define API_RETURN_STRING_FREE(__string)                                \
    if (__string)                                                       \
    {                                                                   \
        return_value = rb_str_new2 (__string);                          \
        free (__string);                                                \
        return return_value;                                            \
    }                                                                   \
    return rb_str_new2 ("")
#define API_RETURN_INT(__int)                                           \
    return INT2FIX (__int);
#define API_RETURN_LONG(__long)                                         \
    return LONG2FIX (__long);

#define API_DEF_FUNC(__name, __argc)                                    \
    rb_define_module_function (ruby_mWeechat, #__name,                  \
                               &weechat_ruby_api_##__name, __argc);


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

    API_FUNC(0, "register", API_RETURN_ERROR);
    if (ruby_registered_script)
    {
        /* script already registered */
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: script \"%s\" already "
                                         "registered (register ignored)"),
                        weechat_prefix ("error"), RUBY_PLUGIN_NAME,
                        ruby_registered_script->name);
        API_RETURN_ERROR;
    }
    ruby_current_script = NULL;
    ruby_registered_script = NULL;
    if (NIL_P (name) || NIL_P (author) || NIL_P (version)
        || NIL_P (license) || NIL_P (description) || NIL_P (shutdown_func)
        || NIL_P (charset))
        API_WRONG_ARGS(API_RETURN_ERROR);

    Check_Type (name, T_STRING);
    Check_Type (author, T_STRING);
    Check_Type (version, T_STRING);
    Check_Type (license, T_STRING);
    Check_Type (description, T_STRING);
    Check_Type (shutdown_func, T_STRING);
    Check_Type (charset, T_STRING);

    c_name = StringValuePtr (name);
    c_author = StringValuePtr (author);
    c_version = StringValuePtr (version);
    c_license = StringValuePtr (license);
    c_description = StringValuePtr (description);
    c_shutdown_func = StringValuePtr (shutdown_func);
    c_charset = StringValuePtr (charset);

    if (plugin_script_search (weechat_ruby_plugin, ruby_scripts, c_name))
    {
        /* another script already exists with same name */
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to register script "
                                         "\"%s\" (another script already "
                                         "exists with this name)"),
                        weechat_prefix ("error"), RUBY_PLUGIN_NAME, c_name);
        API_RETURN_ERROR;
    }

    /* register script */
    ruby_current_script = plugin_script_add (weechat_ruby_plugin,
                                             &ruby_scripts, &last_ruby_script,
                                             (ruby_current_script_filename) ?
                                             ruby_current_script_filename : "",
                                             c_name, c_author, c_version, c_license,
                                             c_description, c_shutdown_func,
                                             c_charset);

    if (ruby_current_script)
    {
        ruby_registered_script = ruby_current_script;
        if ((weechat_ruby_plugin->debug >= 2) || !ruby_quiet)
        {
            weechat_printf (NULL,
                            weechat_gettext ("%s: registered script \"%s\", "
                                             "version %s (%s)"),
                            RUBY_PLUGIN_NAME, c_name, c_version, c_description);
        }
    }
    else
    {
        API_RETURN_ERROR;
    }

    API_RETURN_OK;
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

    API_FUNC(1, "plugin_get_name", API_RETURN_EMPTY);
    if (NIL_P (plugin))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (plugin, T_STRING);

    c_plugin = StringValuePtr (plugin);

    result = weechat_plugin_get_name (API_STR2PTR(c_plugin));

    API_RETURN_STRING(result);
}

/*
 * weechat_ruby_api_charset_set: set script charset
 */

static VALUE
weechat_ruby_api_charset_set (VALUE class, VALUE charset)
{
    char *c_charset;

    API_FUNC(1, "charset_set", API_RETURN_ERROR);
    if (NIL_P (charset))
        API_WRONG_ARGS(API_RETURN_ERROR);

    Check_Type (charset, T_STRING);

    c_charset = StringValuePtr (charset);

    plugin_script_api_charset_set (ruby_current_script, c_charset);

    API_RETURN_OK;
}

/*
 * weechat_ruby_api_iconv_to_internal: convert string to internal WeeChat charset
 */

static VALUE
weechat_ruby_api_iconv_to_internal (VALUE class, VALUE charset, VALUE string)
{
    char *c_charset, *c_string, *result;
    VALUE return_value;

    API_FUNC(1, "iconv_to_internal", API_RETURN_EMPTY);
    if (NIL_P (charset) || NIL_P (string))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (charset, T_STRING);
    Check_Type (string, T_STRING);

    c_charset = StringValuePtr (charset);
    c_string = StringValuePtr (string);

    result = weechat_iconv_to_internal (c_charset, c_string);

    API_RETURN_STRING_FREE(result);
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

    API_FUNC(1, "iconv_from_internal", API_RETURN_EMPTY);
    if (NIL_P (charset) || NIL_P (string))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (charset, T_STRING);
    Check_Type (string, T_STRING);

    c_charset = StringValuePtr (charset);
    c_string = StringValuePtr (string);

    result = weechat_iconv_from_internal (c_charset, c_string);

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_gettext: get translated string
 */

static VALUE
weechat_ruby_api_gettext (VALUE class, VALUE string)
{
    char *c_string;
    const char *result;

    API_FUNC(1, "gettext", API_RETURN_EMPTY);
    if (NIL_P (string))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (string, T_STRING);

    c_string = StringValuePtr (string);

    result = weechat_gettext (c_string);

    API_RETURN_STRING(result);
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

    API_FUNC(1, "ngettext", API_RETURN_EMPTY);
    if (NIL_P (single) || NIL_P (plural) || NIL_P (count))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (single, T_STRING);
    Check_Type (plural, T_STRING);
    Check_Type (count, T_FIXNUM);

    c_single = StringValuePtr (single);
    c_plural = StringValuePtr (plural);
    c_count = FIX2INT (count);

    result = weechat_ngettext (c_single, c_plural, c_count);

    API_RETURN_STRING(result);
}

/*
 * weechat_ruby_api_string_match: return 1 if string matches a mask
 *                                mask can begin or end with "*", no other "*"
 *                                are allowed inside mask
 */

static VALUE
weechat_ruby_api_string_match (VALUE class, VALUE string, VALUE mask,
                               VALUE case_sensitive)
{
    char *c_string, *c_mask;
    int c_case_sensitive, value;

    API_FUNC(1, "string_match", API_RETURN_INT(0));
    if (NIL_P (string) || NIL_P (mask) || NIL_P (case_sensitive))
        API_WRONG_ARGS(API_RETURN_INT(0));

    Check_Type (string, T_STRING);
    Check_Type (mask, T_STRING);
    Check_Type (case_sensitive, T_FIXNUM);

    c_string = StringValuePtr (string);
    c_mask = StringValuePtr (mask);
    c_case_sensitive = FIX2INT (case_sensitive);

    value = weechat_string_match (c_string, c_mask, c_case_sensitive);

    API_RETURN_INT(value);
}

/*
 * weechat_ruby_api_string_has_highlight: return 1 if string contains a
 *                                        highlight (using list of words to
 *                                        highlight)
 *                                        return 0 if no highlight is found in
 *                                        string
 */

static VALUE
weechat_ruby_api_string_has_highlight (VALUE class, VALUE string,
                                       VALUE highlight_words)
{
    char *c_string, *c_highlight_words;
    int value;

    API_FUNC(1, "string_has_highlight", API_RETURN_INT(0));
    if (NIL_P (string) || NIL_P (highlight_words))
        API_WRONG_ARGS(API_RETURN_INT(0));

    Check_Type (string, T_STRING);
    Check_Type (highlight_words, T_STRING);

    c_string = StringValuePtr (string);
    c_highlight_words = StringValuePtr (highlight_words);

    value = weechat_string_has_highlight (c_string, c_highlight_words);

    API_RETURN_INT(value);
}

/*
 * weechat_ruby_api_string_has_highlight_regex: return 1 if string contains a
 *                                              highlight (using regular
 *                                              expression)
 *                                              return 0 if no highlight is
 *                                              found in string
 */

static VALUE
weechat_ruby_api_string_has_highlight_regex (VALUE class, VALUE string,
                                             VALUE regex)
{
    char *c_string, *c_regex;
    int value;

    API_FUNC(1, "string_has_highlight_regex", API_RETURN_INT(0));
    if (NIL_P (string) || NIL_P (regex))
        API_WRONG_ARGS(API_RETURN_INT(0));

    Check_Type (string, T_STRING);
    Check_Type (regex, T_STRING);

    c_string = StringValuePtr (string);
    c_regex = StringValuePtr (regex);

    value = weechat_string_has_highlight_regex (c_string, c_regex);

    API_RETURN_INT(value);
}

/*
 * weechat_ruby_api_string_mask_to_regex: convert a mask (string with only
 *                                        "*" as wildcard) to a regex, paying
 *                                        attention to special chars in a
 *                                        regex
 */

static VALUE
weechat_ruby_api_string_mask_to_regex (VALUE class, VALUE mask)
{
    char *c_mask, *result;
    VALUE return_value;

    API_FUNC(1, "string_mask_to_regex", API_RETURN_EMPTY);
    if (NIL_P (mask))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (mask, T_STRING);

    c_mask = StringValuePtr (mask);

    result = weechat_string_mask_to_regex (c_mask);

    API_RETURN_STRING_FREE(result);
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

    API_FUNC(1, "string_remove_color", API_RETURN_EMPTY);
    if (NIL_P (string) || NIL_P (replacement))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (string, T_STRING);
    Check_Type (replacement, T_STRING);

    c_string = StringValuePtr (string);
    c_replacement = StringValuePtr (replacement);

    result = weechat_string_remove_color (c_string, c_replacement);

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_string_is_command_char: check if first char of string is a
 *                                          command char
 */

static VALUE
weechat_ruby_api_string_is_command_char (VALUE class, VALUE string)
{
    char *c_string;
    int value;

    API_FUNC(1, "string_is_command_char", API_RETURN_INT(0));
    if (NIL_P (string))
        API_WRONG_ARGS(API_RETURN_INT(0));

    Check_Type (string, T_STRING);

    c_string = StringValuePtr (string);

    value = weechat_string_is_command_char (c_string);

    API_RETURN_INT(value);
}

/*
 * weechat_ruby_api_string_input_for_buffer: return string with input text
 *                                           for buffer or empty string if
 *                                           it's a command
 */

static VALUE
weechat_ruby_api_string_input_for_buffer (VALUE class, VALUE string)
{
    char *c_string;
    const char *result;

    API_FUNC(1, "string_input_for_buffer", API_RETURN_EMPTY);
    if (NIL_P (string))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (string, T_STRING);

    c_string = StringValuePtr (string);

    result = weechat_string_input_for_buffer (c_string);

    API_RETURN_STRING(result);
}

/*
 * weechat_ruby_api_string_eval_expression: evaluate an expression and return
 *                                          result
 */

static VALUE
weechat_ruby_api_string_eval_expression (VALUE class, VALUE expr,
                                         VALUE pointers, VALUE extra_vars)
{
    char *c_expr, *result;
    struct t_hashtable *c_pointers, *c_extra_vars;
    VALUE return_value;

    API_FUNC(1, "string_eval_expression", API_RETURN_EMPTY);
    if (NIL_P (expr) || NIL_P (pointers) || NIL_P (extra_vars))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (expr, T_STRING);
    Check_Type (pointers, T_HASH);
    Check_Type (extra_vars, T_HASH);

    c_expr = StringValuePtr (expr);
    c_pointers = weechat_ruby_hash_to_hashtable (pointers,
                                                 WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                                 WEECHAT_HASHTABLE_STRING,
                                                 WEECHAT_HASHTABLE_POINTER);
    c_extra_vars = weechat_ruby_hash_to_hashtable (extra_vars,
                                                   WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                                   WEECHAT_HASHTABLE_STRING,
                                                   WEECHAT_HASHTABLE_STRING);

    result = weechat_string_eval_expression (c_expr, c_pointers, c_extra_vars);

    if (c_pointers)
        weechat_hashtable_free (c_pointers);
    if (c_extra_vars)
        weechat_hashtable_free (c_extra_vars);

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_mkdir_home: create a directory in WeeChat home
 */

static VALUE
weechat_ruby_api_mkdir_home (VALUE class, VALUE directory, VALUE mode)
{
    char *c_directory;
    int c_mode;

    API_FUNC(1, "mkdir_home", API_RETURN_ERROR);
    if (NIL_P (directory) || NIL_P (mode))
        API_WRONG_ARGS(API_RETURN_ERROR);

    Check_Type (directory, T_STRING);
    Check_Type (mode, T_FIXNUM);

    c_directory = StringValuePtr (directory);
    c_mode = FIX2INT (mode);

    if (weechat_mkdir_home (c_directory, c_mode))
        API_RETURN_OK;

    API_RETURN_ERROR;
}

/*
 * weechat_ruby_api_mkdir: create a directory
 */

static VALUE
weechat_ruby_api_mkdir (VALUE class, VALUE directory, VALUE mode)
{
    char *c_directory;
    int c_mode;

    API_FUNC(1, "mkdir", API_RETURN_ERROR);
    if (NIL_P (directory) || NIL_P (mode))
        API_WRONG_ARGS(API_RETURN_ERROR);

    Check_Type (directory, T_STRING);
    Check_Type (mode, T_FIXNUM);

    c_directory = StringValuePtr (directory);
    c_mode = FIX2INT (mode);

    if (weechat_mkdir (c_directory, c_mode))
        API_RETURN_OK;

    API_RETURN_ERROR;
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

    API_FUNC(1, "mkdir_parents", API_RETURN_ERROR);
    if (NIL_P (directory) || NIL_P (mode))
        API_WRONG_ARGS(API_RETURN_ERROR);

    Check_Type (directory, T_STRING);
    Check_Type (mode, T_FIXNUM);

    c_directory = StringValuePtr (directory);
    c_mode = FIX2INT (mode);

    if (weechat_mkdir_parents (c_directory, c_mode))
        API_RETURN_OK;

    API_RETURN_ERROR;
}

/*
 * weechat_ruby_api_list_new: create a new list
 */

static VALUE
weechat_ruby_api_list_new (VALUE class)
{
    char *result;

    API_FUNC(1, "list_new", API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_list_new ());

    API_RETURN_STRING(result);
}

/*
 * weechat_ruby_api_list_add: add a string to list
 */

static VALUE
weechat_ruby_api_list_add (VALUE class, VALUE weelist, VALUE data, VALUE where,
                           VALUE user_data)
{
    char *c_weelist, *c_data, *c_where, *c_user_data, *result;

    API_FUNC(1, "list_add", API_RETURN_EMPTY);
    if (NIL_P (weelist) || NIL_P (data) || NIL_P (where) || NIL_P (user_data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (weelist, T_STRING);
    Check_Type (data, T_STRING);
    Check_Type (where, T_STRING);
    Check_Type (user_data, T_STRING);

    c_weelist = StringValuePtr (weelist);
    c_data = StringValuePtr (data);
    c_where = StringValuePtr (where);
    c_user_data = StringValuePtr (user_data);

    result = API_PTR2STR(weechat_list_add (API_STR2PTR(c_weelist),
                                           c_data,
                                           c_where,
                                           API_STR2PTR(c_user_data)));

    API_RETURN_STRING(result);
}

/*
 * weechat_ruby_api_list_search: search a string in list
 */

static VALUE
weechat_ruby_api_list_search (VALUE class, VALUE weelist, VALUE data)
{
    char *c_weelist, *c_data, *result;

    API_FUNC(1, "list_search", API_RETURN_EMPTY);
    if (NIL_P (weelist) || NIL_P (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (weelist, T_STRING);
    Check_Type (data, T_STRING);

    c_weelist = StringValuePtr (weelist);
    c_data = StringValuePtr (data);

    result = API_PTR2STR(weechat_list_search (API_STR2PTR(c_weelist),
                                              c_data));

    API_RETURN_STRING(result);
}

/*
 * weechat_ruby_api_list_search_pos: search position of a string in list
 */

static VALUE
weechat_ruby_api_list_search_pos (VALUE class, VALUE weelist, VALUE data)
{
    char *c_weelist, *c_data;
    int pos;

    API_FUNC(1, "list_search_pos", API_RETURN_INT(-1));
    if (NIL_P (weelist) || NIL_P (data))
        API_WRONG_ARGS(API_RETURN_INT(-1));

    Check_Type (weelist, T_STRING);
    Check_Type (data, T_STRING);

    c_weelist = StringValuePtr (weelist);
    c_data = StringValuePtr (data);

    pos = weechat_list_search_pos (API_STR2PTR(c_weelist), c_data);

    API_RETURN_INT(pos);
}

/*
 * weechat_ruby_api_list_casesearch: search a string in list (ignore case)
 */

static VALUE
weechat_ruby_api_list_casesearch (VALUE class, VALUE weelist, VALUE data)
{
    char *c_weelist, *c_data, *result;

    API_FUNC(1, "list_casesearch", API_RETURN_EMPTY);
    if (NIL_P (weelist) || NIL_P (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (weelist, T_STRING);
    Check_Type (data, T_STRING);

    c_weelist = StringValuePtr (weelist);
    c_data = StringValuePtr (data);

    result = API_PTR2STR(weechat_list_casesearch (API_STR2PTR(c_weelist),
                                                  c_data));

    API_RETURN_STRING(result);
}

/*
 * weechat_ruby_api_list_casesearch_pos: search position of a string in list
 *                                       (ignore case)
 */

static VALUE
weechat_ruby_api_list_casesearch_pos (VALUE class, VALUE weelist, VALUE data)
{
    char *c_weelist, *c_data;
    int pos;

    API_FUNC(1, "list_casesearch_pos", API_RETURN_INT(-1));
    if (NIL_P (weelist) || NIL_P (data))
        API_WRONG_ARGS(API_RETURN_INT(-1));

    Check_Type (weelist, T_STRING);
    Check_Type (data, T_STRING);

    c_weelist = StringValuePtr (weelist);
    c_data = StringValuePtr (data);

    pos = weechat_list_casesearch_pos (API_STR2PTR(c_weelist), c_data);

    API_RETURN_INT(pos);
}

/*
 * weechat_ruby_api_list_get: get item by position
 */

static VALUE
weechat_ruby_api_list_get (VALUE class, VALUE weelist, VALUE position)
{
    char *c_weelist, *result;
    int c_position;

    API_FUNC(1, "list_get", API_RETURN_EMPTY);
    if (NIL_P (weelist) || NIL_P (position))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (weelist, T_STRING);
    Check_Type (position, T_FIXNUM);

    c_weelist = StringValuePtr (weelist);
    c_position = FIX2INT (position);

    result = API_PTR2STR(weechat_list_get (API_STR2PTR(c_weelist),
                                           c_position));

    API_RETURN_STRING(result);
}

/*
 * weechat_ruby_api_list_set: set new value for item
 */

static VALUE
weechat_ruby_api_list_set (VALUE class, VALUE item, VALUE new_value)
{
    char *c_item, *c_new_value;

    API_FUNC(1, "list_set", API_RETURN_ERROR);
    if (NIL_P (item) || NIL_P (new_value))
        API_WRONG_ARGS(API_RETURN_ERROR);

    Check_Type (item, T_STRING);
    Check_Type (new_value, T_STRING);

    c_item = StringValuePtr (item);
    c_new_value = StringValuePtr (new_value);

    weechat_list_set (API_STR2PTR(c_item),
                      c_new_value);

    API_RETURN_OK;
}

/*
 * weechat_ruby_api_list_next: get next item
 */

static VALUE
weechat_ruby_api_list_next (VALUE class, VALUE item)
{
    char *c_item, *result;

    API_FUNC(1, "list_next", API_RETURN_EMPTY);
    if (NIL_P (item))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (item, T_STRING);

    c_item = StringValuePtr (item);

    result = API_PTR2STR(weechat_list_next (API_STR2PTR(c_item)));

    API_RETURN_STRING(result);
}

/*
 * weechat_ruby_api_list_prev: get previous item
 */

static VALUE
weechat_ruby_api_list_prev (VALUE class, VALUE item)
{
    char *c_item, *result;

    API_FUNC(1, "list_prev", API_RETURN_EMPTY);
    if (NIL_P (item))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (item, T_STRING);

    c_item = StringValuePtr (item);

    result = API_PTR2STR(weechat_list_prev (API_STR2PTR(c_item)));

    API_RETURN_STRING(result);
}

/*
 * weechat_ruby_api_list_string: get string value of item
 */

static VALUE
weechat_ruby_api_list_string (VALUE class, VALUE item)
{
    char *c_item;
    const char *result;

    API_FUNC(1, "list_string", API_RETURN_EMPTY);
    if (NIL_P (item))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (item, T_STRING);

    c_item = StringValuePtr (item);

    result = weechat_list_string (API_STR2PTR(c_item));

    API_RETURN_STRING(result);
}

/*
 * weechat_ruby_api_list_size: get number of elements in list
 */

static VALUE
weechat_ruby_api_list_size (VALUE class, VALUE weelist)
{
    char *c_weelist;
    int size;

    API_FUNC(1, "list_size", API_RETURN_INT(0));
    if (NIL_P (weelist))
        API_WRONG_ARGS(API_RETURN_INT(0));

    Check_Type (weelist, T_STRING);

    c_weelist = StringValuePtr (weelist);

    size = weechat_list_size (API_STR2PTR(c_weelist));

    API_RETURN_INT(size);
}

/*
 * weechat_ruby_api_list_remove: remove item from list
 */

static VALUE
weechat_ruby_api_list_remove (VALUE class, VALUE weelist, VALUE item)
{
    char *c_weelist, *c_item;

    API_FUNC(1, "list_remove", API_RETURN_ERROR);
    if (NIL_P (weelist) || NIL_P (item))
        API_WRONG_ARGS(API_RETURN_ERROR);

    Check_Type (weelist, T_STRING);
    Check_Type (item, T_STRING);

    c_weelist = StringValuePtr (weelist);
    c_item = StringValuePtr (item);

    weechat_list_remove (API_STR2PTR(c_weelist),
                         API_STR2PTR(c_item));

    API_RETURN_OK;
}

/*
 * weechat_ruby_api_list_remove_all: remove all items from list
 */

static VALUE
weechat_ruby_api_list_remove_all (VALUE class, VALUE weelist)
{
    char *c_weelist;

    API_FUNC(1, "list_remove_all", API_RETURN_ERROR);
    if (NIL_P (weelist))
        API_WRONG_ARGS(API_RETURN_ERROR);

    Check_Type (weelist, T_STRING);

    c_weelist = StringValuePtr (weelist);

    weechat_list_remove_all (API_STR2PTR(c_weelist));

    API_RETURN_OK;
}

/*
 * weechat_ruby_api_list_free: free list
 */

static VALUE
weechat_ruby_api_list_free (VALUE class, VALUE weelist)
{
    char *c_weelist;

    API_FUNC(1, "list_free", API_RETURN_ERROR);
    if (NIL_P (weelist))
        API_WRONG_ARGS(API_RETURN_ERROR);

    Check_Type (weelist, T_STRING);

    c_weelist = StringValuePtr (weelist);

    weechat_list_free (API_STR2PTR(c_weelist));

    API_RETURN_OK;
}

/*
 * weechat_ruby_api_config_reload_cb: callback for config reload
 */

int
weechat_ruby_api_config_reload_cb (void *data,
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

        rc = (int *) weechat_ruby_exec (script_callback->script,
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
 * weechat_ruby_api_config_new: create a new configuration file
 */

static VALUE
weechat_ruby_api_config_new (VALUE class, VALUE name, VALUE function,
                             VALUE data)
{
    char *c_name, *c_function, *c_data, *result;
    VALUE return_value;

    API_FUNC(1, "config_new", API_RETURN_EMPTY);
    if (NIL_P (name) || NIL_P (function) || NIL_P (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (name, T_STRING);
    Check_Type (function, T_STRING);
    Check_Type (data, T_STRING);

    c_name = StringValuePtr (name);
    c_function = StringValuePtr (function);
    c_data = StringValuePtr (data);

    result = API_PTR2STR(plugin_script_api_config_new (weechat_ruby_plugin,
                                                       ruby_current_script,
                                                       c_name,
                                                       &weechat_ruby_api_config_reload_cb,
                                                       c_function,
                                                       c_data));

    API_RETURN_STRING_FREE(result);
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

        rc = (int *) weechat_ruby_exec (script_callback->script,
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
 * weechat_ruby_api_config_section_write_cb: callback for writing section
 */

int
weechat_ruby_api_config_section_write_cb (void *data,
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

        rc = (int *) weechat_ruby_exec (script_callback->script,
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
 * weechat_ruby_api_config_section_write_default_cb: callback for writing
 *                                                   default values for section
 */

int
weechat_ruby_api_config_section_write_default_cb (void *data,
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

        rc = (int *) weechat_ruby_exec (script_callback->script,
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
 * weechat_ruby_api_config_section_create_option_cb: callback to create an option
 */

int
weechat_ruby_api_config_section_create_option_cb (void *data,
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

        rc = (int *) weechat_ruby_exec (script_callback->script,
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
 * weechat_ruby_api_config_section_delete_option_cb: callback to delete an option
 */

int
weechat_ruby_api_config_section_delete_option_cb (void *data,
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

        rc = (int *) weechat_ruby_exec (script_callback->script,
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

    API_FUNC(1, "config_new_section", API_RETURN_EMPTY);
    if (NIL_P (config_file) || NIL_P (name) || NIL_P (user_can_add_options)
        || NIL_P (user_can_delete_options) || NIL_P (function_read)
        || NIL_P (data_read) || NIL_P (function_write) || NIL_P (data_write)
        || NIL_P (function_write_default) || NIL_P (data_write_default)
        || NIL_P (function_create_option) || NIL_P (data_create_option)
        || NIL_P (function_delete_option) || NIL_P (data_delete_option))
        API_WRONG_ARGS(API_RETURN_EMPTY);

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

    c_config_file = StringValuePtr (config_file);
    c_name = StringValuePtr (name);
    c_user_can_add_options = FIX2INT (user_can_add_options);
    c_user_can_delete_options = FIX2INT (user_can_delete_options);
    c_function_read = StringValuePtr (function_read);
    c_data_read = StringValuePtr (data_read);
    c_function_write = StringValuePtr (function_write);
    c_data_write = StringValuePtr (data_write);
    c_function_write_default = StringValuePtr (function_write_default);
    c_data_write_default = StringValuePtr (data_write_default);
    c_function_create_option = StringValuePtr (function_create_option);
    c_data_create_option = StringValuePtr (data_create_option);
    c_function_delete_option = StringValuePtr (function_delete_option);
    c_data_delete_option = StringValuePtr (data_delete_option);

    result = API_PTR2STR(plugin_script_api_config_new_section (weechat_ruby_plugin,
                                                               ruby_current_script,
                                                               API_STR2PTR(c_config_file),
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

    API_RETURN_STRING_FREE(result);
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

    API_FUNC(1, "config_search_section", API_RETURN_EMPTY);
    if (NIL_P (config_file) || NIL_P (section_name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (config_file, T_STRING);
    Check_Type (section_name, T_STRING);

    c_config_file = StringValuePtr (config_file);
    c_section_name = StringValuePtr (section_name);

    result = API_PTR2STR(weechat_config_search_section (API_STR2PTR(c_config_file),
                                                        c_section_name));

    API_RETURN_STRING_FREE(result);
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

        rc = (int *) weechat_ruby_exec (script_callback->script,
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
 * weechat_ruby_api_config_option_change_cb: callback for option changed
 */

void
weechat_ruby_api_config_option_change_cb (void *data,
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

        rc = (int *) weechat_ruby_exec (script_callback->script,
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
 * weechat_ruby_api_config_option_delete_cb: callback when option is deleted
 */

void
weechat_ruby_api_config_option_delete_cb (void *data,
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

        rc = (int *) weechat_ruby_exec (script_callback->script,
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
    char *c_string_values, *c_default_value, *c_value;
    char *c_function_check_value, *c_data_check_value, *c_function_change;
    char *c_data_change, *c_function_delete, *c_data_delete, *result;
    int c_min, c_max, c_null_value_allowed;
    VALUE return_value;

    API_FUNC(1, "config_new_option", API_RETURN_EMPTY);
    if (NIL_P (config_file) || NIL_P (section) || NIL_P (name) || NIL_P (type)
        || NIL_P (description) || NIL_P (string_values) || NIL_P (min)
        || NIL_P (max) || NIL_P (default_value) || NIL_P (value)
        || NIL_P (null_value_allowed) || NIL_P (function_check_value)
        || NIL_P (data_check_value) || NIL_P (function_change)
        || NIL_P (data_change) || NIL_P (function_delete)
        || NIL_P (data_delete))
        API_WRONG_ARGS(API_RETURN_EMPTY);

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

    c_config_file = StringValuePtr (config_file);
    c_section = StringValuePtr (section);
    c_name = StringValuePtr (name);
    c_type = StringValuePtr (type);
    c_description = StringValuePtr (description);
    c_string_values = StringValuePtr (string_values);
    c_min = FIX2INT (min);
    c_max = FIX2INT (max);
    c_default_value = StringValuePtr (default_value);
    c_value = StringValuePtr (value);
    c_null_value_allowed = FIX2INT (null_value_allowed);
    c_function_check_value = StringValuePtr (function_check_value);
    c_data_check_value = StringValuePtr (data_check_value);
    c_function_change = StringValuePtr (function_change);
    c_data_change = StringValuePtr (data_change);
    c_function_delete = StringValuePtr (function_delete);
    c_data_delete = StringValuePtr (data_delete);

    result = API_PTR2STR(plugin_script_api_config_new_option (weechat_ruby_plugin,
                                                              ruby_current_script,
                                                              API_STR2PTR(c_config_file),
                                                              API_STR2PTR(c_section),
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

    API_RETURN_STRING_FREE(result);
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

    API_FUNC(1, "config_search_option", API_RETURN_EMPTY);
    if (NIL_P (config_file) || NIL_P (section) || NIL_P (option_name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (config_file, T_STRING);
    Check_Type (section, T_STRING);
    Check_Type (option_name, T_STRING);

    c_config_file = StringValuePtr (config_file);
    c_section = StringValuePtr (section);
    c_option_name = StringValuePtr (option_name);

    result = API_PTR2STR(weechat_config_search_option (API_STR2PTR(c_config_file),
                                                       API_STR2PTR(c_section),
                                                       c_option_name));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_config_string_to_boolean: return boolean value of a string
 */

static VALUE
weechat_ruby_api_config_string_to_boolean (VALUE class, VALUE text)
{
    char *c_text;
    int value;

    API_FUNC(1, "config_string_to_boolean", API_RETURN_INT(0));
    if (NIL_P (text))
        API_WRONG_ARGS(API_RETURN_INT(0));

    Check_Type (text, T_STRING);

    c_text = StringValuePtr (text);

    value = weechat_config_string_to_boolean (c_text);

    API_RETURN_INT(value);
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

    API_FUNC(1, "config_option_reset", API_RETURN_INT(0));
    if (NIL_P (option) || NIL_P (run_callback))
        API_WRONG_ARGS(API_RETURN_INT(0));

    Check_Type (option, T_STRING);
    Check_Type (run_callback, T_FIXNUM);

    c_option = StringValuePtr (option);
    c_run_callback = FIX2INT (run_callback);

    rc = weechat_config_option_reset (API_STR2PTR(c_option),
                                      c_run_callback);

    API_RETURN_INT(rc);
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

    API_FUNC(1, "config_option_set", API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));
    if (NIL_P (option) || NIL_P (new_value) || NIL_P (run_callback))
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));

    Check_Type (option, T_STRING);
    Check_Type (new_value, T_STRING);
    Check_Type (run_callback, T_FIXNUM);

    c_option = StringValuePtr (option);
    c_new_value = StringValuePtr (new_value);
    c_run_callback = FIX2INT (run_callback);

    rc = weechat_config_option_set (API_STR2PTR(c_option),
                                    c_new_value,
                                    c_run_callback);

    API_RETURN_INT(rc);
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

    API_FUNC(1, "config_option_set_null", API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));
    if (NIL_P (option) || NIL_P (run_callback))
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));

    Check_Type (option, T_STRING);
    Check_Type (run_callback, T_FIXNUM);

    c_option = StringValuePtr (option);
    c_run_callback = FIX2INT (run_callback);

    rc = weechat_config_option_set_null (API_STR2PTR(c_option),
                                         c_run_callback);

    API_RETURN_INT(rc);
}

/*
 * weechat_ruby_api_config_option_unset: unset an option
 */

static VALUE
weechat_ruby_api_config_option_unset (VALUE class, VALUE option)
{
    char *c_option;
    int rc;

    API_FUNC(1, "config_option_unset", API_RETURN_INT(WEECHAT_CONFIG_OPTION_UNSET_ERROR));
    if (NIL_P (option))
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_OPTION_UNSET_ERROR));

    Check_Type (option, T_STRING);

    c_option = StringValuePtr (option);

    rc = weechat_config_option_unset (API_STR2PTR(c_option));

    API_RETURN_INT(rc);
}

/*
 * weechat_ruby_api_config_option_rename: rename an option
 */

static VALUE
weechat_ruby_api_config_option_rename (VALUE class, VALUE option,
                                       VALUE new_name)
{
    char *c_option, *c_new_name;

    API_FUNC(1, "config_option_rename", API_RETURN_ERROR);
    if (NIL_P (option) || NIL_P (new_name))
        API_WRONG_ARGS(API_RETURN_ERROR);

    Check_Type (option, T_STRING);
    Check_Type (new_name, T_STRING);

    c_option = StringValuePtr (option);
    c_new_name = StringValuePtr (new_name);

    weechat_config_option_rename (API_STR2PTR(c_option),
                                  c_new_name);

    API_RETURN_OK;
}

/*
 * weechat_ruby_api_config_option_is_null: return 1 if value of option is null
 */

static VALUE
weechat_ruby_api_config_option_is_null (VALUE class, VALUE option)
{
    char *c_option;
    int value;

    API_FUNC(1, "config_option_is_null", API_RETURN_INT(1));
    if (NIL_P (option))
        API_WRONG_ARGS(API_RETURN_INT(1));

    Check_Type (option, T_STRING);

    c_option = StringValuePtr (option);

    value = weechat_config_option_is_null (API_STR2PTR(c_option));

    API_RETURN_INT(value);
}

/*
 * weechat_ruby_api_config_option_default_is_null: return 1 if value of option is null
 */

static VALUE
weechat_ruby_api_config_option_default_is_null (VALUE class, VALUE option)
{
    char *c_option;
    int value;

    API_FUNC(1, "config_option_default_is_null", API_RETURN_INT(1));
    if (NIL_P (option))
        API_WRONG_ARGS(API_RETURN_INT(1));

    Check_Type (option, T_STRING);

    c_option = StringValuePtr (option);

    value = weechat_config_option_default_is_null (API_STR2PTR(c_option));

    API_RETURN_INT(value);
}

/*
 * weechat_ruby_api_config_boolean: return boolean value of option
 */

static VALUE
weechat_ruby_api_config_boolean (VALUE class, VALUE option)
{
    char *c_option;
    int value;

    API_FUNC(1, "config_boolean", API_RETURN_INT(0));
    if (NIL_P (option))
        API_WRONG_ARGS(API_RETURN_INT(0));

    Check_Type (option, T_STRING);

    c_option = StringValuePtr (option);

    value = weechat_config_boolean (API_STR2PTR(c_option));

    API_RETURN_INT(value);
}

/*
 * weechat_ruby_api_config_boolean_default: return default boolean value of option
 */

static VALUE
weechat_ruby_api_config_boolean_default (VALUE class, VALUE option)
{
    char *c_option;
    int value;

    API_FUNC(1, "config_boolean_default", API_RETURN_INT(0));
    if (NIL_P (option))
        API_WRONG_ARGS(API_RETURN_INT(0));

    Check_Type (option, T_STRING);

    c_option = StringValuePtr (option);

    value = weechat_config_boolean_default (API_STR2PTR(c_option));

    API_RETURN_INT(value);
}

/*
 * weechat_ruby_api_config_integer: return integer value of option
 */

static VALUE
weechat_ruby_api_config_integer (VALUE class, VALUE option)
{
    char *c_option;
    int value;

    API_FUNC(1, "config_integer", API_RETURN_INT(0));
    if (NIL_P (option))
        API_WRONG_ARGS(API_RETURN_INT(0));

    Check_Type (option, T_STRING);

    c_option = StringValuePtr (option);

    value = weechat_config_integer (API_STR2PTR(c_option));

    API_RETURN_INT(value);
}

/*
 * weechat_ruby_api_config_integer_default: return default integer value of option
 */

static VALUE
weechat_ruby_api_config_integer_default (VALUE class, VALUE option)
{
    char *c_option;
    int value;

    API_FUNC(1, "config_integer_default", API_RETURN_INT(0));
    if (NIL_P (option))
        API_WRONG_ARGS(API_RETURN_INT(0));

    Check_Type (option, T_STRING);

    c_option = StringValuePtr (option);

    value = weechat_config_integer_default (API_STR2PTR(c_option));

    API_RETURN_INT(value);
}

/*
 * weechat_ruby_api_config_string: return string value of option
 */

static VALUE
weechat_ruby_api_config_string (VALUE class, VALUE option)
{
    char *c_option;
    const char *result;

    API_FUNC(1, "config_string", API_RETURN_EMPTY);
    if (NIL_P (option))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (option, T_STRING);

    c_option = StringValuePtr (option);

    result = weechat_config_string (API_STR2PTR(c_option));

    API_RETURN_STRING(result);
}

/*
 * weechat_ruby_api_config_string_default: return default string value of option
 */

static VALUE
weechat_ruby_api_config_string_default (VALUE class, VALUE option)
{
    char *c_option;
    const char *result;

    API_FUNC(1, "config_string_default", API_RETURN_EMPTY);
    if (NIL_P (option))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (option, T_STRING);

    c_option = StringValuePtr (option);

    result = weechat_config_string_default (API_STR2PTR(c_option));

    API_RETURN_STRING(result);
}

/*
 * weechat_ruby_api_config_color: return color value of option
 */

static VALUE
weechat_ruby_api_config_color (VALUE class, VALUE option)
{
    char *c_option;
    const char *result;

    API_FUNC(1, "config_color", API_RETURN_INT(0));
    if (NIL_P (option))
        API_WRONG_ARGS(API_RETURN_INT(0));

    Check_Type (option, T_STRING);

    c_option = StringValuePtr (option);

    result = weechat_config_color (API_STR2PTR(c_option));

    API_RETURN_STRING(result);
}

/*
 * weechat_ruby_api_config_color_default: return default color value of option
 */

static VALUE
weechat_ruby_api_config_color_default (VALUE class, VALUE option)
{
    char *c_option;
    const char *result;

    API_FUNC(1, "config_color_default", API_RETURN_INT(0));
    if (NIL_P (option))
        API_WRONG_ARGS(API_RETURN_INT(0));

    Check_Type (option, T_STRING);

    c_option = StringValuePtr (option);

    result = weechat_config_color_default (API_STR2PTR(c_option));

    API_RETURN_STRING(result);
}

/*
 * weechat_ruby_api_config_write_option: write an option in configuration file
 */

static VALUE
weechat_ruby_api_config_write_option (VALUE class, VALUE config_file,
                                      VALUE option)
{
    char *c_config_file, *c_option;

    API_FUNC(1, "config_write_option", API_RETURN_ERROR);
    if (NIL_P (config_file) || NIL_P (option))
        API_WRONG_ARGS(API_RETURN_ERROR);

    Check_Type (config_file, T_STRING);
    Check_Type (option, T_STRING);

    c_config_file = StringValuePtr (config_file);
    c_option = StringValuePtr (option);

    weechat_config_write_option (API_STR2PTR(c_config_file),
                                 API_STR2PTR(c_option));

    API_RETURN_OK;
}

/*
 * weechat_ruby_api_config_write_line: write a line in configuration file
 */

static VALUE
weechat_ruby_api_config_write_line (VALUE class, VALUE config_file,
                                    VALUE option_name, VALUE value)
{
    char *c_config_file, *c_option_name, *c_value;

    API_FUNC(1, "config_write_line", API_RETURN_ERROR);
    if (NIL_P (config_file) || NIL_P (option_name) || NIL_P (value))
        API_WRONG_ARGS(API_RETURN_ERROR);

    Check_Type (config_file, T_STRING);
    Check_Type (option_name, T_STRING);
    Check_Type (value, T_STRING);

    c_config_file = StringValuePtr (config_file);
    c_option_name = StringValuePtr (option_name);
    c_value = StringValuePtr (value);

    weechat_config_write_line (API_STR2PTR(c_config_file),
                               c_option_name,
                               "%s",
                               c_value);

    API_RETURN_OK;
}

/*
 * weechat_ruby_api_config_write: write configuration file
 */

static VALUE
weechat_ruby_api_config_write (VALUE class, VALUE config_file)
{
    char *c_config_file;
    int rc;

    API_FUNC(1, "config_write", API_RETURN_INT(-1));
    if (NIL_P (config_file))
        API_WRONG_ARGS(API_RETURN_INT(-1));

    Check_Type (config_file, T_STRING);

    c_config_file = StringValuePtr (config_file);

    rc = weechat_config_write (API_STR2PTR(c_config_file));

    API_RETURN_INT(rc);
}

/*
 * weechat_ruby_api_config_read: read configuration file
 */

static VALUE
weechat_ruby_api_config_read (VALUE class, VALUE config_file)
{
    char *c_config_file;
    int rc;

    API_FUNC(1, "config_read", API_RETURN_INT(-1));
    if (NIL_P (config_file))
        API_WRONG_ARGS(API_RETURN_INT(-1));

    Check_Type (config_file, T_STRING);

    c_config_file = StringValuePtr (config_file);

    rc = weechat_config_read (API_STR2PTR(c_config_file));

    API_RETURN_INT(rc);
}

/*
 * weechat_ruby_api_config_reload: reload configuration file
 */

static VALUE
weechat_ruby_api_config_reload (VALUE class, VALUE config_file)
{
    char *c_config_file;
    int rc;

    API_FUNC(1, "config_reload", API_RETURN_INT(-1));
    if (NIL_P (config_file))
        API_WRONG_ARGS(API_RETURN_INT(-1));

    Check_Type (config_file, T_STRING);

    c_config_file = StringValuePtr (config_file);

    rc = weechat_config_reload (API_STR2PTR(c_config_file));

    API_RETURN_INT(rc);
}

/*
 * weechat_ruby_api_config_option_free: free an option in configuration file
 */

static VALUE
weechat_ruby_api_config_option_free (VALUE class, VALUE option)
{
    char *c_option;

    API_FUNC(1, "config_option_free", API_RETURN_ERROR);
    if (NIL_P (option))
        API_WRONG_ARGS(API_RETURN_ERROR);

    Check_Type (option, T_STRING);

    c_option = StringValuePtr (option);

    plugin_script_api_config_option_free (weechat_ruby_plugin,
                                          ruby_current_script,
                                          API_STR2PTR(c_option));

    API_RETURN_OK;
}

/*
 * weechat_ruby_api_config_section_free_options: free all options of a section
 *                                               in configuration file
 */

static VALUE
weechat_ruby_api_config_section_free_options (VALUE class, VALUE section)
{
    char *c_section;

    API_FUNC(1, "config_section_free_options", API_RETURN_ERROR);
    if (NIL_P (section))
        API_WRONG_ARGS(API_RETURN_ERROR);

    Check_Type (section, T_STRING);

    c_section = StringValuePtr (section);

    plugin_script_api_config_section_free_options (weechat_ruby_plugin,
                                                   ruby_current_script,
                                                   API_STR2PTR(c_section));

    API_RETURN_OK;
}

/*
 * weechat_ruby_api_config_section_free: free section in configuration file
 */

static VALUE
weechat_ruby_api_config_section_free (VALUE class, VALUE section)
{
    char *c_section;

    API_FUNC(1, "config_section_free", API_RETURN_ERROR);
    if (NIL_P (section))
        API_WRONG_ARGS(API_RETURN_ERROR);

    Check_Type (section, T_STRING);

    c_section = StringValuePtr (section);

    plugin_script_api_config_section_free (weechat_ruby_plugin,
                                           ruby_current_script,
                                           API_STR2PTR(c_section));

    API_RETURN_OK;
}

/*
 * weechat_ruby_api_config_free: free configuration file
 */

static VALUE
weechat_ruby_api_config_free (VALUE class, VALUE config_file)
{
    char *c_config_file;

    API_FUNC(1, "config_free", API_RETURN_ERROR);
    if (NIL_P (config_file))
        API_WRONG_ARGS(API_RETURN_ERROR);

    Check_Type (config_file, T_STRING);

    c_config_file = StringValuePtr (config_file);

    plugin_script_api_config_free (weechat_ruby_plugin,
                                   ruby_current_script,
                                   API_STR2PTR(c_config_file));

    API_RETURN_OK;
}

/*
 * weechat_ruby_api_config_get: get config option
 */

static VALUE
weechat_ruby_api_config_get (VALUE class, VALUE option)
{
    char *c_option, *result;
    VALUE return_value;

    API_FUNC(1, "config_get", API_RETURN_EMPTY);
    if (NIL_P (option))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (option, T_STRING);

    c_option = StringValuePtr (option);

    result = API_PTR2STR(weechat_config_get (c_option));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_config_get_plugin: get value of a plugin option
 */

static VALUE
weechat_ruby_api_config_get_plugin (VALUE class, VALUE option)
{
    char *c_option;
    const char *result;

    API_FUNC(1, "config_get_plugin", API_RETURN_EMPTY);
    if (NIL_P (option))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (option, T_STRING);

    c_option = StringValuePtr (option);

    result = plugin_script_api_config_get_plugin (weechat_ruby_plugin,
                                                  ruby_current_script,
                                                  c_option);

    API_RETURN_STRING(result);
}

/*
 * weechat_ruby_api_config_is_set_plugin: check if a plugin option is set
 */

static VALUE
weechat_ruby_api_config_is_set_plugin (VALUE class, VALUE option)
{
    char *c_option;
    int rc;

    API_FUNC(1, "config_is_set_plugin", API_RETURN_INT(0));
    if (NIL_P (option))
        API_WRONG_ARGS(API_RETURN_INT(0));

    Check_Type (option, T_STRING);

    c_option = StringValuePtr (option);

    rc = plugin_script_api_config_is_set_plugin (weechat_ruby_plugin,
                                                 ruby_current_script,
                                                 c_option);

    API_RETURN_INT(rc);
}

/*
 * weechat_ruby_api_config_set_plugin: set value of a plugin option
 */

static VALUE
weechat_ruby_api_config_set_plugin (VALUE class, VALUE option, VALUE value)
{
    char *c_option, *c_value;
    int rc;

    API_FUNC(1, "config_set_plugin", API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));
    if (NIL_P (option) || NIL_P (value))
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR));

    Check_Type (option, T_STRING);
    Check_Type (value, T_STRING);

    c_option = StringValuePtr (option);
    c_value = StringValuePtr (value);

    rc = plugin_script_api_config_set_plugin (weechat_ruby_plugin,
                                              ruby_current_script,
                                              c_option,
                                              c_value);

    API_RETURN_INT(rc);
}

/*
 * weechat_ruby_api_config_set_desc_plugin: set description of a plugin option
 */

static VALUE
weechat_ruby_api_config_set_desc_plugin (VALUE class, VALUE option,
                                         VALUE description)
{
    char *c_option, *c_description;

    API_FUNC(1, "config_set_desc_plugin", API_RETURN_ERROR);
    if (NIL_P (option) || NIL_P (description))
        API_WRONG_ARGS(API_RETURN_ERROR);

    Check_Type (option, T_STRING);
    Check_Type (description, T_STRING);

    c_option = StringValuePtr (option);
    c_description = StringValuePtr (description);

    plugin_script_api_config_set_desc_plugin (weechat_ruby_plugin,
                                              ruby_current_script,
                                              c_option,
                                              c_description);

    API_RETURN_OK;
}

/*
 * weechat_ruby_api_config_unset_plugin: unset plugin option
 */

static VALUE
weechat_ruby_api_config_unset_plugin (VALUE class, VALUE option)
{
    char *c_option;
    int rc;

    API_FUNC(1, "config_unset_plugin", API_RETURN_INT(WEECHAT_CONFIG_OPTION_UNSET_ERROR));
    if (NIL_P (option))
        API_WRONG_ARGS(API_RETURN_INT(WEECHAT_CONFIG_OPTION_UNSET_ERROR));

    Check_Type (option, T_STRING);

    c_option = StringValuePtr (option);

    rc = plugin_script_api_config_unset_plugin (weechat_ruby_plugin,
                                                ruby_current_script,
                                                c_option);

    API_RETURN_INT(rc);
}

/*
 * weechat_ruby_api_key_bind: bind key(s)
 */

static VALUE
weechat_ruby_api_key_bind (VALUE class, VALUE context, VALUE keys)
{
    char *c_context;
    struct t_hashtable *c_keys;
    int num_keys;

    API_FUNC(1, "key_bind", API_RETURN_INT(0));
    if (NIL_P (context) || NIL_P (keys))
        API_WRONG_ARGS(API_RETURN_INT(0));

    Check_Type (context, T_STRING);
    Check_Type (keys, T_HASH);

    c_context = StringValuePtr (context);
    c_keys = weechat_ruby_hash_to_hashtable (keys,
                                             WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                             WEECHAT_HASHTABLE_STRING,
                                             WEECHAT_HASHTABLE_STRING);

    num_keys = weechat_key_bind (c_context, c_keys);

    if (c_keys)
        weechat_hashtable_free (c_keys);

    API_RETURN_INT(num_keys);
}

/*
 * weechat_ruby_api_key_unbind: unbind key(s)
 */

static VALUE
weechat_ruby_api_key_unbind (VALUE class, VALUE context, VALUE key)
{
    char *c_context, *c_key;
    int num_keys;

    API_FUNC(1, "key_unbind", API_RETURN_INT(0));
    if (NIL_P (context) || NIL_P (key))
        API_WRONG_ARGS(API_RETURN_INT(0));

    Check_Type (context, T_STRING);
    Check_Type (key, T_STRING);

    c_context = StringValuePtr (context);
    c_key = StringValuePtr (key);

    num_keys = weechat_key_unbind (c_context, c_key);

    API_RETURN_INT(num_keys);
}

/*
 * weechat_ruby_api_prefix: get a prefix, used for display
 */

static VALUE
weechat_ruby_api_prefix (VALUE class, VALUE prefix)
{
    char *c_prefix;
    const char *result;

    API_FUNC(0, "prefix", API_RETURN_EMPTY);
    if (NIL_P (prefix))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (prefix, T_STRING);

    c_prefix = StringValuePtr (prefix);

    result = weechat_prefix (c_prefix);

    API_RETURN_STRING(result);
}

/*
 * weechat_ruby_api_color: get a color code, used for display
 */

static VALUE
weechat_ruby_api_color (VALUE class, VALUE color)
{
    char *c_color;
    const char *result;

    API_FUNC(0, "color", API_RETURN_EMPTY);
    if (NIL_P (color))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (color, T_STRING);

    c_color = StringValuePtr (color);

    result = weechat_color (c_color);

    API_RETURN_STRING(result);
}

/*
 * weechat_ruby_api_print: print message in a buffer
 */

static VALUE
weechat_ruby_api_print (VALUE class, VALUE buffer, VALUE message)
{
    char *c_buffer, *c_message;

    API_FUNC(0, "print", API_RETURN_ERROR);
    if (NIL_P (buffer) || NIL_P (message))
        API_WRONG_ARGS(API_RETURN_ERROR);

    Check_Type (buffer, T_STRING);
    Check_Type (message, T_STRING);

    c_buffer = StringValuePtr (buffer);
    c_message = StringValuePtr (message);

    plugin_script_api_printf (weechat_ruby_plugin,
                              ruby_current_script,
                              API_STR2PTR(c_buffer),
                              "%s", c_message);

    API_RETURN_OK;
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

    API_FUNC(1, "print_date_tags", API_RETURN_ERROR);
    if (NIL_P (buffer) || NIL_P (date) || NIL_P (tags) || NIL_P (message))
        API_WRONG_ARGS(API_RETURN_ERROR);

    Check_Type (buffer, T_STRING);
    Check_Type (date, T_FIXNUM);
    Check_Type (tags, T_STRING);
    Check_Type (message, T_STRING);

    c_buffer = StringValuePtr (buffer);
    c_date = FIX2INT (date);
    c_tags = StringValuePtr (tags);
    c_message = StringValuePtr (message);

    plugin_script_api_printf_date_tags (weechat_ruby_plugin,
                                        ruby_current_script,
                                        API_STR2PTR(c_buffer),
                                        c_date,
                                        c_tags,
                                        "%s", c_message);

    API_RETURN_OK;
}

/*
 * weechat_ruby_api_print_y: print message in a buffer with free content
 */

static VALUE
weechat_ruby_api_print_y (VALUE class, VALUE buffer, VALUE y, VALUE message)
{
    char *c_buffer, *c_message;
    int c_y;

    API_FUNC(1, "print_y", API_RETURN_ERROR);
    if (NIL_P (buffer) || NIL_P (y) || NIL_P (message))
        API_WRONG_ARGS(API_RETURN_ERROR);

    Check_Type (buffer, T_STRING);
    Check_Type (y, T_FIXNUM);
    Check_Type (message, T_STRING);

    c_buffer = StringValuePtr (buffer);
    c_y = FIX2INT (y);
    c_message = StringValuePtr (message);

    plugin_script_api_printf_y (weechat_ruby_plugin,
                                ruby_current_script,
                                API_STR2PTR(c_buffer),
                                c_y,
                                "%s", c_message);

    API_RETURN_OK;
}

/*
 * weechat_ruby_api_log_print: print message in WeeChat log file
 */

static VALUE
weechat_ruby_api_log_print (VALUE class, VALUE message)
{
    char *c_message;

    API_FUNC(1, "log_print", API_RETURN_ERROR);
    if (NIL_P (message))
        API_WRONG_ARGS(API_RETURN_ERROR);

    Check_Type (message, T_STRING);

    c_message = StringValuePtr (message);

    plugin_script_api_log_printf (weechat_ruby_plugin,
                                  ruby_current_script,
                                  "%s", c_message);

    API_RETURN_OK;
}

/*
 * weechat_ruby_api_hook_command_cb: callback for command hooked
 */

int
weechat_ruby_api_hook_command_cb (void *data, struct t_gui_buffer *buffer,
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

        rc = (int *) weechat_ruby_exec (script_callback->script,
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

    API_FUNC(1, "hook_command", API_RETURN_EMPTY);
    if (NIL_P (command) || NIL_P (description) || NIL_P (args)
        || NIL_P (args_description) || NIL_P (completion) || NIL_P (function)
        || NIL_P (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (command, T_STRING);
    Check_Type (description, T_STRING);
    Check_Type (args, T_STRING);
    Check_Type (args_description, T_STRING);
    Check_Type (completion, T_STRING);
    Check_Type (function, T_STRING);
    Check_Type (data, T_STRING);

    c_command = StringValuePtr (command);
    c_description = StringValuePtr (description);
    c_args = StringValuePtr (args);
    c_args_description = StringValuePtr (args_description);
    c_completion = StringValuePtr (completion);
    c_function = StringValuePtr (function);
    c_data = StringValuePtr (data);

    result = API_PTR2STR(plugin_script_api_hook_command (weechat_ruby_plugin,
                                                         ruby_current_script,
                                                         c_command,
                                                         c_description,
                                                         c_args,
                                                         c_args_description,
                                                         c_completion,
                                                         &weechat_ruby_api_hook_command_cb,
                                                         c_function,
                                                         c_data));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_hook_command_run_cb: callback for command_run hooked
 */

int
weechat_ruby_api_hook_command_run_cb (void *data, struct t_gui_buffer *buffer,
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

        rc = (int *) weechat_ruby_exec (script_callback->script,
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
 * weechat_ruby_api_hook_command_run: hook a command_run
 */

static VALUE
weechat_ruby_api_hook_command_run (VALUE class, VALUE command, VALUE function,
                                   VALUE data)
{
    char *c_command, *c_function, *c_data, *result;
    VALUE return_value;

    API_FUNC(1, "hook_command_run", API_RETURN_EMPTY);
    if (NIL_P (command) || NIL_P (function) || NIL_P (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (command, T_STRING);
    Check_Type (function, T_STRING);
    Check_Type (data, T_STRING);

    c_command = StringValuePtr (command);
    c_function = StringValuePtr (function);
    c_data = StringValuePtr (data);

    result = API_PTR2STR(plugin_script_api_hook_command_run (weechat_ruby_plugin,
                                                             ruby_current_script,
                                                             c_command,
                                                             &weechat_ruby_api_hook_command_run_cb,
                                                             c_function,
                                                             c_data));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_hook_timer_cb: callback for timer hooked
 */

int
weechat_ruby_api_hook_timer_cb (void *data, int remaining_calls)
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

        rc = (int *) weechat_ruby_exec (script_callback->script,
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
 * weechat_ruby_api_hook_timer: hook a timer
 */

static VALUE
weechat_ruby_api_hook_timer (VALUE class, VALUE interval, VALUE align_second,
                             VALUE max_calls, VALUE function, VALUE data)
{
    int c_interval, c_align_second, c_max_calls;
    char *c_function, *c_data, *result;
    VALUE return_value;

    API_FUNC(1, "hook_timer", API_RETURN_EMPTY);
    if (NIL_P (interval) || NIL_P (align_second) || NIL_P (max_calls)
        || NIL_P (function) || NIL_P (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (interval, T_FIXNUM);
    Check_Type (align_second, T_FIXNUM);
    Check_Type (max_calls, T_FIXNUM);
    Check_Type (function, T_STRING);
    Check_Type (data, T_STRING);

    c_interval = FIX2INT (interval);
    c_align_second = FIX2INT (align_second);
    c_max_calls = FIX2INT (max_calls);
    c_function = StringValuePtr (function);
    c_data = StringValuePtr (data);

    result = API_PTR2STR(plugin_script_api_hook_timer (weechat_ruby_plugin,
                                                       ruby_current_script,
                                                       c_interval,
                                                       c_align_second,
                                                       c_max_calls,
                                                       &weechat_ruby_api_hook_timer_cb,
                                                       c_function,
                                                       c_data));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_hook_fd_cb: callback for fd hooked
 */

int
weechat_ruby_api_hook_fd_cb (void *data, int fd)
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

        rc = (int *) weechat_ruby_exec (script_callback->script,
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
 * weechat_ruby_api_hook_fd: hook a fd
 */

static VALUE
weechat_ruby_api_hook_fd (VALUE class, VALUE fd, VALUE read, VALUE write,
                          VALUE exception, VALUE function, VALUE data)
{
    int c_fd, c_read, c_write, c_exception;
    char *c_function, *c_data, *result;
    VALUE return_value;

    API_FUNC(1, "hook_fd", API_RETURN_EMPTY);
    if (NIL_P (fd) || NIL_P (read) || NIL_P (write) || NIL_P (exception)
        || NIL_P (function) || NIL_P (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

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
    c_function = StringValuePtr (function);
    c_data = StringValuePtr (data);

    result = API_PTR2STR(plugin_script_api_hook_fd (weechat_ruby_plugin,
                                                    ruby_current_script,
                                                    c_fd,
                                                    c_read,
                                                    c_write,
                                                    c_exception,
                                                    &weechat_ruby_api_hook_fd_cb,
                                                    c_function,
                                                    c_data));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_hook_process_cb: callback for process hooked
 */

int
weechat_ruby_api_hook_process_cb (void *data,
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

        rc = (int *) weechat_ruby_exec (script_callback->script,
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
 * weechat_ruby_api_hook_process: hook a process
 */

static VALUE
weechat_ruby_api_hook_process (VALUE class, VALUE command, VALUE timeout,
                               VALUE function, VALUE data)
{
    char *c_command, *c_function, *c_data, *result;
    int c_timeout;
    VALUE return_value;

    API_FUNC(1, "hook_process", API_RETURN_EMPTY);
    if (NIL_P (command) || NIL_P (timeout) || NIL_P (function) || NIL_P (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (command, T_STRING);
    Check_Type (timeout, T_FIXNUM);
    Check_Type (function, T_STRING);
    Check_Type (data, T_STRING);

    c_command = StringValuePtr (command);
    c_timeout = FIX2INT (timeout);
    c_function = StringValuePtr (function);
    c_data = StringValuePtr (data);

    result = API_PTR2STR(plugin_script_api_hook_process (weechat_ruby_plugin,
                                                         ruby_current_script,
                                                         c_command,
                                                         c_timeout,
                                                         &weechat_ruby_api_hook_process_cb,
                                                         c_function,
                                                         c_data));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_hook_process_hashtable: hook a process with options in
 *                                          a hashtable
 */

static VALUE
weechat_ruby_api_hook_process_hashtable (VALUE class, VALUE command,
                                         VALUE options, VALUE timeout,
                                         VALUE function, VALUE data)
{
    char *c_command, *c_function, *c_data, *result;
    struct t_hashtable *c_options;
    int c_timeout;
    VALUE return_value;

    API_FUNC(1, "hook_process_hashtable", API_RETURN_EMPTY);
    if (NIL_P (command) || NIL_P (options) || NIL_P (timeout)
        || NIL_P (function) || NIL_P (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (command, T_STRING);
    Check_Type (options, T_HASH);
    Check_Type (timeout, T_FIXNUM);
    Check_Type (function, T_STRING);
    Check_Type (data, T_STRING);

    c_command = StringValuePtr (command);
    c_options = weechat_ruby_hash_to_hashtable (options,
                                                WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                                WEECHAT_HASHTABLE_STRING,
                                                WEECHAT_HASHTABLE_STRING);
    c_timeout = FIX2INT (timeout);
    c_function = StringValuePtr (function);
    c_data = StringValuePtr (data);

    result = API_PTR2STR(plugin_script_api_hook_process_hashtable (weechat_ruby_plugin,
                                                                   ruby_current_script,
                                                                   c_command,
                                                                   c_options,
                                                                   c_timeout,
                                                                   &weechat_ruby_api_hook_process_cb,
                                                                   c_function,
                                                                   c_data));

    if (c_options)
        weechat_hashtable_free (c_options);

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_hook_connect_cb: callback for connect hooked
 */

int
weechat_ruby_api_hook_connect_cb (void *data, int status, int gnutls_rc,
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

        rc = (int *) weechat_ruby_exec (script_callback->script,
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
 * weechat_ruby_api_hook_connect: hook a connection
 */

static VALUE
weechat_ruby_api_hook_connect (VALUE class, VALUE proxy, VALUE address,
                               VALUE port, VALUE ipv6, VALUE retry,
                               VALUE local_hostname, VALUE function,
                               VALUE data)
{
    char *c_proxy, *c_address, *c_local_hostname, *c_function, *c_data, *result;
    int c_port, c_ipv6, c_retry;
    VALUE return_value;

    API_FUNC(1, "hook_connect", API_RETURN_EMPTY);
    if (NIL_P (proxy) || NIL_P (address) || NIL_P (port) || NIL_P (ipv6)
        || NIL_P (retry) || NIL_P (local_hostname) || NIL_P (function)
        || NIL_P (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (proxy, T_STRING);
    Check_Type (address, T_STRING);
    Check_Type (port, T_FIXNUM);
    Check_Type (ipv6, T_FIXNUM);
    Check_Type (retry, T_FIXNUM);
    Check_Type (local_hostname, T_STRING);
    Check_Type (function, T_STRING);
    Check_Type (data, T_STRING);

    c_proxy = StringValuePtr (proxy);
    c_address = StringValuePtr (address);
    c_port = FIX2INT (port);
    c_ipv6 = FIX2INT (ipv6);
    c_retry = FIX2INT (retry);
    c_local_hostname = StringValuePtr (local_hostname);
    c_function = StringValuePtr (function);
    c_data = StringValuePtr (data);

    result = API_PTR2STR(plugin_script_api_hook_connect (weechat_ruby_plugin,
                                                         ruby_current_script,
                                                         c_proxy,
                                                         c_address,
                                                         c_port,
                                                         c_ipv6,
                                                         c_retry,
                                                         NULL, /* gnutls session */
                                                         NULL, /* gnutls callback */
                                                         0,    /* gnutls DH key size */
                                                         NULL, /* gnutls priorities */
                                                         c_local_hostname,
                                                         &weechat_ruby_api_hook_connect_cb,
                                                         c_function,
                                                         c_data));

    API_RETURN_STRING_FREE(result);
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

        rc = (int *) weechat_ruby_exec (script_callback->script,
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

    API_FUNC(1, "hook_print", API_RETURN_EMPTY);
    if (NIL_P (buffer) || NIL_P (tags) || NIL_P (message)
        || NIL_P (strip_colors) || NIL_P (function) || NIL_P (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (buffer, T_STRING);
    Check_Type (tags, T_STRING);
    Check_Type (message, T_STRING);
    Check_Type (strip_colors, T_FIXNUM);
    Check_Type (function, T_STRING);
    Check_Type (data, T_STRING);

    c_buffer = StringValuePtr (buffer);
    c_tags = StringValuePtr (tags);
    c_message = StringValuePtr (message);
    c_strip_colors = FIX2INT (strip_colors);
    c_function = StringValuePtr (function);
    c_data = StringValuePtr (data);

    result = API_PTR2STR(plugin_script_api_hook_print (weechat_ruby_plugin,
                                                       ruby_current_script,
                                                       API_STR2PTR(c_buffer),
                                                       c_tags,
                                                       c_message,
                                                       c_strip_colors,
                                                       &weechat_ruby_api_hook_print_cb,
                                                       c_function,
                                                       c_data));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_hook_signal_cb: callback for signal hooked
 */

int
weechat_ruby_api_hook_signal_cb (void *data, const char *signal, const char *type_data,
                                 void *signal_data)
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

        rc = (int *) weechat_ruby_exec (script_callback->script,
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
 * weechat_ruby_api_hook_signal: hook a signal
 */

static VALUE
weechat_ruby_api_hook_signal (VALUE class, VALUE signal, VALUE function,
                              VALUE data)
{
    char *c_signal, *c_function, *c_data, *result;
    VALUE return_value;

    API_FUNC(1, "hook_signal", API_RETURN_EMPTY);
    if (NIL_P (signal) || NIL_P (function) || NIL_P (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (signal, T_STRING);
    Check_Type (function, T_STRING);
    Check_Type (data, T_STRING);

    c_signal = StringValuePtr (signal);
    c_function = StringValuePtr (function);
    c_data = StringValuePtr (data);

    result = API_PTR2STR(plugin_script_api_hook_signal (weechat_ruby_plugin,
                                                        ruby_current_script,
                                                        c_signal,
                                                        &weechat_ruby_api_hook_signal_cb,
                                                        c_function,
                                                        c_data));

    API_RETURN_STRING_FREE(result);
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

    API_FUNC(1, "hook_signal_send", API_RETURN_ERROR);
    if (NIL_P (signal) || NIL_P (type_data) || NIL_P (signal_data))
        API_WRONG_ARGS(API_RETURN_ERROR);

    Check_Type (signal, T_STRING);
    Check_Type (type_data, T_STRING);

    c_signal = StringValuePtr (signal);
    c_type_data = StringValuePtr (type_data);

    if (strcmp (c_type_data, WEECHAT_HOOK_SIGNAL_STRING) == 0)
    {
        Check_Type (signal_data, T_STRING);
        c_signal_data = StringValuePtr (signal_data);
        weechat_hook_signal_send (c_signal, c_type_data, c_signal_data);
        API_RETURN_OK;
    }
    else if (strcmp (c_type_data, WEECHAT_HOOK_SIGNAL_INT) == 0)
    {
        Check_Type (signal_data, T_FIXNUM);
        number = FIX2INT (signal_data);
        weechat_hook_signal_send (c_signal, c_type_data, &number);
        API_RETURN_OK;
    }
    else if (strcmp (c_type_data, WEECHAT_HOOK_SIGNAL_POINTER) == 0)
    {
        Check_Type (signal_data, T_STRING);
        c_signal_data = StringValuePtr (signal_data);
        weechat_hook_signal_send (c_signal, c_type_data,
                                  API_STR2PTR(c_signal_data));
        API_RETURN_OK;
    }

    API_RETURN_ERROR;
}

/*
 * weechat_ruby_api_hook_hsignal_cb: callback for hsignal hooked
 */

int
weechat_ruby_api_hook_hsignal_cb (void *data, const char *signal,
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

        rc = (int *) weechat_ruby_exec (script_callback->script,
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
 * weechat_ruby_api_hook_hsignal: hook a hsignal
 */

static VALUE
weechat_ruby_api_hook_hsignal (VALUE class, VALUE signal, VALUE function,
                               VALUE data)
{
    char *c_signal, *c_function, *c_data, *result;
    VALUE return_value;

    API_FUNC(1, "hook_hsignal", API_RETURN_EMPTY);
    if (NIL_P (signal) || NIL_P (function) || NIL_P (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (signal, T_STRING);
    Check_Type (function, T_STRING);
    Check_Type (data, T_STRING);

    c_signal = StringValuePtr (signal);
    c_function = StringValuePtr (function);
    c_data = StringValuePtr (data);

    result = API_PTR2STR(plugin_script_api_hook_hsignal (weechat_ruby_plugin,
                                                         ruby_current_script,
                                                         c_signal,
                                                         &weechat_ruby_api_hook_hsignal_cb,
                                                         c_function,
                                                         c_data));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_hook_hsignal_send: send a hsignal
 */

static VALUE
weechat_ruby_api_hook_hsignal_send (VALUE class, VALUE signal, VALUE hashtable)
{
    char *c_signal;
    struct t_hashtable *c_hashtable;

    API_FUNC(1, "hook_hsignal_send", API_RETURN_ERROR);
    if (NIL_P (signal) || NIL_P (hashtable))
        API_WRONG_ARGS(API_RETURN_ERROR);

    Check_Type (signal, T_STRING);
    Check_Type (hashtable, T_HASH);

    c_signal = StringValuePtr (signal);
    c_hashtable = weechat_ruby_hash_to_hashtable (hashtable,
                                                  WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                                  WEECHAT_HASHTABLE_STRING,
                                                  WEECHAT_HASHTABLE_STRING);

    weechat_hook_hsignal_send (c_signal, c_hashtable);

    if (c_hashtable)
        weechat_hashtable_free (c_hashtable);

    API_RETURN_OK;
}

/*
 * weechat_ruby_api_hook_config_cb: callback for config option hooked
 */

int
weechat_ruby_api_hook_config_cb (void *data, const char *option, const char *value)
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

        rc = (int *) weechat_ruby_exec (script_callback->script,
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
 * weechat_ruby_api_hook_config: hook a config option
 */

static VALUE
weechat_ruby_api_hook_config (VALUE class, VALUE option, VALUE function,
                              VALUE data)
{
    char *c_option, *c_function, *c_data, *result;
    VALUE return_value;

    API_FUNC(1, "hook_config", API_RETURN_EMPTY);
    if (NIL_P (option) || NIL_P (function) || NIL_P (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (option, T_STRING);
    Check_Type (function, T_STRING);
    Check_Type (data, T_STRING);

    c_option = StringValuePtr (option);
    c_function = StringValuePtr (function);
    c_data = StringValuePtr (data);

    result = API_PTR2STR(plugin_script_api_hook_config (weechat_ruby_plugin,
                                                        ruby_current_script,
                                                        c_option,
                                                        &weechat_ruby_api_hook_config_cb,
                                                        c_function,
                                                        c_data));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_hook_completion_cb: callback for completion hooked
 */

int
weechat_ruby_api_hook_completion_cb (void *data, const char *completion_item,
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

        rc = (int *) weechat_ruby_exec (script_callback->script,
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
 * weechat_ruby_api_hook_completion: hook a completion
 */

static VALUE
weechat_ruby_api_hook_completion (VALUE class, VALUE completion,
                                  VALUE description, VALUE function,
                                  VALUE data)
{
    char *c_completion, *c_description, *c_function, *c_data, *result;
    VALUE return_value;

    API_FUNC(1, "hook_completion", API_RETURN_EMPTY);
    if (NIL_P (completion) || NIL_P (description) || NIL_P (function)
        || NIL_P (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (completion, T_STRING);
    Check_Type (description, T_STRING);
    Check_Type (function, T_STRING);
    Check_Type (data, T_STRING);

    c_completion = StringValuePtr (completion);
    c_description = StringValuePtr (description);
    c_function = StringValuePtr (function);
    c_data = StringValuePtr (data);

    result = API_PTR2STR(plugin_script_api_hook_completion (weechat_ruby_plugin,
                                                            ruby_current_script,
                                                            c_completion,
                                                            c_description,
                                                            &weechat_ruby_api_hook_completion_cb,
                                                            c_function,
                                                            c_data));

    API_RETURN_STRING_FREE(result);
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

    API_FUNC(1, "hook_completion_list_add", API_RETURN_ERROR);
    if (NIL_P (completion) || NIL_P (word) || NIL_P (nick_completion)
        || NIL_P (where))
        API_WRONG_ARGS(API_RETURN_ERROR);

    Check_Type (completion, T_STRING);
    Check_Type (word, T_STRING);
    Check_Type (nick_completion, T_FIXNUM);
    Check_Type (where, T_STRING);

    c_completion = StringValuePtr (completion);
    c_word = StringValuePtr (word);
    c_nick_completion = FIX2INT (nick_completion);
    c_where = StringValuePtr (where);

    weechat_hook_completion_list_add (API_STR2PTR(c_completion),
                                      c_word,
                                      c_nick_completion,
                                      c_where);

    API_RETURN_OK;
}

/*
 * weechat_ruby_api_hook_modifier_cb: callback for modifier hooked
 */

char *
weechat_ruby_api_hook_modifier_cb (void *data, const char *modifier,
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

        return (char *)weechat_ruby_exec (script_callback->script,
                                          WEECHAT_SCRIPT_EXEC_STRING,
                                          script_callback->function,
                                          "ssss", func_argv);
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

    API_FUNC(1, "hook_modifier", API_RETURN_EMPTY);
    if (NIL_P (modifier) || NIL_P (function) || NIL_P (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (modifier, T_STRING);
    Check_Type (function, T_STRING);
    Check_Type (data, T_STRING);

    c_modifier = StringValuePtr (modifier);
    c_function = StringValuePtr (function);
    c_data = StringValuePtr (data);

    result = API_PTR2STR(plugin_script_api_hook_modifier (weechat_ruby_plugin,
                                                          ruby_current_script,
                                                          c_modifier,
                                                          &weechat_ruby_api_hook_modifier_cb,
                                                          c_function,
                                                          c_data));

    API_RETURN_STRING_FREE(result);
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

    API_FUNC(1, "hook_modifier_exec", API_RETURN_EMPTY);
    if (NIL_P (modifier) || NIL_P (modifier_data) || NIL_P (string))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (modifier, T_STRING);
    Check_Type (modifier_data, T_STRING);
    Check_Type (string, T_STRING);

    c_modifier = StringValuePtr (modifier);
    c_modifier_data = StringValuePtr (modifier_data);
    c_string = StringValuePtr (string);

    result = weechat_hook_modifier_exec (c_modifier, c_modifier_data, c_string);

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_hook_info_cb: callback for info hooked
 */

const char *
weechat_ruby_api_hook_info_cb (void *data, const char *info_name,
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

        return (const char *)weechat_ruby_exec (script_callback->script,
                                                WEECHAT_SCRIPT_EXEC_STRING,
                                                script_callback->function,
                                                "sss", func_argv);
    }

    return NULL;
}

/*
 * weechat_ruby_api_hook_info: hook an info
 */

static VALUE
weechat_ruby_api_hook_info (VALUE class, VALUE info_name, VALUE description,
                            VALUE args_description, VALUE function, VALUE data)
{
    char *c_info_name, *c_description, *c_args_description, *c_function;
    char *c_data, *result;
    VALUE return_value;

    API_FUNC(1, "hook_info", API_RETURN_EMPTY);
    if (NIL_P (info_name) || NIL_P (description) || NIL_P (args_description)
        || NIL_P (function) || NIL_P (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (info_name, T_STRING);
    Check_Type (description, T_STRING);
    Check_Type (args_description, T_STRING);
    Check_Type (function, T_STRING);
    Check_Type (data, T_STRING);

    c_info_name = StringValuePtr (info_name);
    c_description = StringValuePtr (description);
    c_args_description = StringValuePtr (args_description);
    c_function = StringValuePtr (function);
    c_data = StringValuePtr (data);

    result = API_PTR2STR(plugin_script_api_hook_info (weechat_ruby_plugin,
                                                      ruby_current_script,
                                                      c_info_name,
                                                      c_description,
                                                      c_args_description,
                                                      &weechat_ruby_api_hook_info_cb,
                                                      c_function,
                                                      c_data));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_hook_info_hashtable_cb: callback for info_hashtable hooked
 */

struct t_hashtable *
weechat_ruby_api_hook_info_hashtable_cb (void *data, const char *info_name,
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

        return (struct t_hashtable *)weechat_ruby_exec (script_callback->script,
                                                        WEECHAT_SCRIPT_EXEC_HASHTABLE,
                                                        script_callback->function,
                                                        "ssh", func_argv);
    }

    return NULL;
}

/*
 * weechat_ruby_api_hook_info_hashtable: hook an info_hashtable
 */

static VALUE
weechat_ruby_api_hook_info_hashtable (VALUE class, VALUE info_name,
                                      VALUE description,
                                      VALUE args_description,
                                      VALUE output_description,
                                      VALUE function, VALUE data)
{
    char *c_info_name, *c_description, *c_args_description;
    char *c_output_description, *c_function;
    char *c_data, *result;
    VALUE return_value;

    API_FUNC(1, "hook_info_hashtable", API_RETURN_EMPTY);
    if (NIL_P (info_name) || NIL_P (description) || NIL_P (args_description)
        || NIL_P (output_description) || NIL_P (function) || NIL_P (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (info_name, T_STRING);
    Check_Type (description, T_STRING);
    Check_Type (args_description, T_STRING);
    Check_Type (output_description, T_STRING);
    Check_Type (function, T_STRING);
    Check_Type (data, T_STRING);

    c_info_name = StringValuePtr (info_name);
    c_description = StringValuePtr (description);
    c_args_description = StringValuePtr (args_description);
    c_output_description = StringValuePtr (output_description);
    c_function = StringValuePtr (function);
    c_data = StringValuePtr (data);

    result = API_PTR2STR(plugin_script_api_hook_info_hashtable (weechat_ruby_plugin,
                                                                ruby_current_script,
                                                                c_info_name,
                                                                c_description,
                                                                c_args_description,
                                                                c_output_description,
                                                                &weechat_ruby_api_hook_info_hashtable_cb,
                                                                c_function,
                                                                c_data));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_hook_infolist_cb: callback for infolist hooked
 */

struct t_infolist *
weechat_ruby_api_hook_infolist_cb (void *data, const char *infolist_name,
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

        result = (struct t_infolist *)weechat_ruby_exec (script_callback->script,
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
 * weechat_ruby_api_hook_infolist: hook an infolist
 */

static VALUE
weechat_ruby_api_hook_infolist (VALUE class, VALUE infolist_name,
                                VALUE description, VALUE pointer_description,
                                VALUE args_description, VALUE function,
                                VALUE data)
{
    char *c_infolist_name, *c_description, *c_pointer_description;
    char *c_args_description, *c_function, *c_data, *result;
    VALUE return_value;

    API_FUNC(1, "hook_infolist", API_RETURN_EMPTY);
    if (NIL_P (infolist_name) || NIL_P (description)
        || NIL_P (pointer_description) || NIL_P (args_description)
        || NIL_P (function) || NIL_P (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (infolist_name, T_STRING);
    Check_Type (description, T_STRING);
    Check_Type (pointer_description, T_STRING);
    Check_Type (args_description, T_STRING);
    Check_Type (function, T_STRING);
    Check_Type (data, T_STRING);

    c_infolist_name = StringValuePtr (infolist_name);
    c_description = StringValuePtr (description);
    c_pointer_description = StringValuePtr (pointer_description);
    c_args_description = StringValuePtr (args_description);
    c_function = StringValuePtr (function);
    c_data = StringValuePtr (data);

    result = API_PTR2STR(plugin_script_api_hook_infolist (weechat_ruby_plugin,
                                                          ruby_current_script,
                                                          c_infolist_name,
                                                          c_description,
                                                          c_pointer_description,
                                                          c_args_description,
                                                          &weechat_ruby_api_hook_infolist_cb,
                                                          c_function,
                                                          c_data));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_hook_focus_cb: callback for focus hooked
 */

struct t_hashtable *
weechat_ruby_api_hook_focus_cb (void *data, struct t_hashtable *info)
{
    struct t_plugin_script_cb *script_callback;
    void *func_argv[2];
    char empty_arg[1] = { '\0' };

    script_callback = (struct t_plugin_script_cb *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        func_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        func_argv[1] = info;

        return (struct t_hashtable *)weechat_ruby_exec (script_callback->script,
                                                        WEECHAT_SCRIPT_EXEC_HASHTABLE,
                                                        script_callback->function,
                                                        "sh", func_argv);
    }

    return NULL;
}

/*
 * weechat_ruby_api_hook_focus: hook a focus
 */

static VALUE
weechat_ruby_api_hook_focus (VALUE class, VALUE area, VALUE function,
                             VALUE data)
{
    char *c_area, *c_function, *c_data, *result;
    VALUE return_value;

    API_FUNC(1, "hook_focus", API_RETURN_EMPTY);
    if (NIL_P (area) || NIL_P (function) || NIL_P (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (area, T_STRING);
    Check_Type (function, T_STRING);
    Check_Type (data, T_STRING);

    c_area = StringValuePtr (area);
    c_function = StringValuePtr (function);
    c_data = StringValuePtr (data);

    result = API_PTR2STR(plugin_script_api_hook_focus (weechat_ruby_plugin,
                                                       ruby_current_script,
                                                       c_area,
                                                       &weechat_ruby_api_hook_focus_cb,
                                                       c_function,
                                                       c_data));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_unhook: unhook something
 */

static VALUE
weechat_ruby_api_unhook (VALUE class, VALUE hook)
{
    char *c_hook;

    API_FUNC(1, "unhook", API_RETURN_ERROR);
    if (NIL_P (hook))
        API_WRONG_ARGS(API_RETURN_ERROR);

    Check_Type (hook, T_STRING);

    c_hook = StringValuePtr (hook);

    plugin_script_api_unhook (weechat_ruby_plugin,
                              ruby_current_script,
                              API_STR2PTR(c_hook));

    API_RETURN_OK;
}

/*
 * weechat_ruby_api_unhook_all: unhook all for script
 */

static VALUE
weechat_ruby_api_unhook_all (VALUE class)
{
    API_FUNC(1, "unhook_all", API_RETURN_ERROR);

    plugin_script_api_unhook_all (weechat_ruby_plugin, ruby_current_script);

    API_RETURN_OK;
}

/*
 * weechat_ruby_api_buffer_input_data_cb: callback for input data in a buffer
 */

int
weechat_ruby_api_buffer_input_data_cb (void *data, struct t_gui_buffer *buffer,
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

        rc = (int *) weechat_ruby_exec (script_callback->script,
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
 * weechat_ruby_api_buffer_close_cb: callback for closed buffer
 */

int
weechat_ruby_api_buffer_close_cb (void *data, struct t_gui_buffer *buffer)
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

        rc = (int *) weechat_ruby_exec (script_callback->script,
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

    API_FUNC(1, "buffer_new", API_RETURN_EMPTY);
    if (NIL_P (name) || NIL_P (function_input) || NIL_P (data_input)
        || NIL_P (function_close) || NIL_P (data_close))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (name, T_STRING);
    Check_Type (function_input, T_STRING);
    Check_Type (data_input, T_STRING);
    Check_Type (function_close, T_STRING);
    Check_Type (data_close, T_STRING);

    c_name = StringValuePtr (name);
    c_function_input = StringValuePtr (function_input);
    c_data_input = StringValuePtr (data_input);
    c_function_close = StringValuePtr (function_close);
    c_data_close = StringValuePtr (data_close);

    result = API_PTR2STR(plugin_script_api_buffer_new (weechat_ruby_plugin,
                                                       ruby_current_script,
                                                       c_name,
                                                       &weechat_ruby_api_buffer_input_data_cb,
                                                       c_function_input,
                                                       c_data_input,
                                                       &weechat_ruby_api_buffer_close_cb,
                                                       c_function_close,
                                                       c_data_close));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_buffer_search: search a buffer
 */

static VALUE
weechat_ruby_api_buffer_search (VALUE class, VALUE plugin, VALUE name)
{
    char *c_plugin, *c_name, *result;
    VALUE return_value;

    API_FUNC(1, "buffer_search", API_RETURN_EMPTY);
    if (NIL_P (plugin) || NIL_P (name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (plugin, T_STRING);
    Check_Type (name, T_STRING);

    c_plugin = StringValuePtr (plugin);
    c_name = StringValuePtr (name);

    result = API_PTR2STR(weechat_buffer_search (c_plugin, c_name));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_buffer_search_main: search main buffer (WeeChat core buffer)
 */

static VALUE
weechat_ruby_api_buffer_search_main (VALUE class)
{
    char *result;
    VALUE return_value;

    API_FUNC(1, "buffer_search_main", API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_buffer_search_main ());

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_current_buffer: get current buffer
 */

static VALUE
weechat_ruby_api_current_buffer (VALUE class)
{
    char *result;
    VALUE return_value;

    API_FUNC(1, "current_buffer", API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_current_buffer ());

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_buffer_clear: clear a buffer
 */

static VALUE
weechat_ruby_api_buffer_clear (VALUE class, VALUE buffer)
{
    char *c_buffer;

    API_FUNC(1, "buffer_clear", API_RETURN_ERROR);
    if (NIL_P (buffer))
        API_WRONG_ARGS(API_RETURN_ERROR);

    Check_Type (buffer, T_STRING);

    c_buffer = StringValuePtr (buffer);

    weechat_buffer_clear (API_STR2PTR(c_buffer));

    API_RETURN_OK;
}

/*
 * weechat_ruby_api_buffer_close: close a buffer
 */

static VALUE
weechat_ruby_api_buffer_close (VALUE class, VALUE buffer)
{
    char *c_buffer;

    API_FUNC(1, "buffer_close", API_RETURN_ERROR);
    if (NIL_P (buffer))
        API_WRONG_ARGS(API_RETURN_ERROR);

    Check_Type (buffer, T_STRING);

    c_buffer = StringValuePtr (buffer);

    plugin_script_api_buffer_close (weechat_ruby_plugin,
                                    ruby_current_script,
                                    API_STR2PTR(c_buffer));

    API_RETURN_OK;
}

/*
 * weechat_ruby_api_buffer_merge: merge a buffer to another buffer
 */

static VALUE
weechat_ruby_api_buffer_merge (VALUE class, VALUE buffer, VALUE target_buffer)
{
    char *c_buffer, *c_target_buffer;

    API_FUNC(1, "buffer_merge", API_RETURN_ERROR);
    if (NIL_P (buffer) || NIL_P (target_buffer))
        API_WRONG_ARGS(API_RETURN_ERROR);

    Check_Type (buffer, T_STRING);
    Check_Type (target_buffer, T_STRING);

    c_buffer = StringValuePtr (buffer);
    c_target_buffer = StringValuePtr (target_buffer);

    weechat_buffer_merge (API_STR2PTR(c_buffer),
                          API_STR2PTR(c_target_buffer));

    API_RETURN_OK;
}

/*
 * weechat_ruby_api_buffer_unmerge: unmerge a buffer from a group of merged
 *                                  buffers
 */

static VALUE
weechat_ruby_api_buffer_unmerge (VALUE class, VALUE buffer, VALUE number)
{
    char *c_buffer;
    int c_number;

    API_FUNC(1, "buffer_unmerge", API_RETURN_ERROR);
    if (NIL_P (buffer) || NIL_P (number))
        API_WRONG_ARGS(API_RETURN_ERROR);

    Check_Type (buffer, T_STRING);
    Check_Type (number, T_FIXNUM);

    c_buffer = StringValuePtr (buffer);
    c_number = FIX2INT (number);

    weechat_buffer_unmerge (API_STR2PTR(c_buffer), c_number);

    API_RETURN_OK;
}

/*
 * weechat_ruby_api_buffer_get_integer: get a buffer property as integer
 */

static VALUE
weechat_ruby_api_buffer_get_integer (VALUE class, VALUE buffer, VALUE property)
{
    char *c_buffer, *c_property;
    int value;

    API_FUNC(1, "buffer_get_integer", API_RETURN_INT(-1));
    if (NIL_P (buffer) || NIL_P (property))
        API_WRONG_ARGS(API_RETURN_INT(-1));

    Check_Type (buffer, T_STRING);
    Check_Type (property, T_STRING);

    c_buffer = StringValuePtr (buffer);
    c_property = StringValuePtr (property);

    value = weechat_buffer_get_integer (API_STR2PTR(c_buffer),
                                        c_property);

    API_RETURN_INT(value);
}

/*
 * weechat_ruby_api_buffer_get_string: get a buffer property as string
 */

static VALUE
weechat_ruby_api_buffer_get_string (VALUE class, VALUE buffer, VALUE property)
{
    char *c_buffer, *c_property;
    const char *result;

    API_FUNC(1, "buffer_get_string", API_RETURN_EMPTY);
    if (NIL_P (buffer) || NIL_P (property))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (buffer, T_STRING);
    Check_Type (property, T_STRING);

    c_buffer = StringValuePtr (buffer);
    c_property = StringValuePtr (property);

    result = weechat_buffer_get_string (API_STR2PTR(c_buffer),
                                        c_property);

    API_RETURN_STRING(result);
}

/*
 * weechat_ruby_api_buffer_get_pointer: get a buffer property as pointer
 */

static VALUE
weechat_ruby_api_buffer_get_pointer (VALUE class, VALUE buffer, VALUE property)
{
    char *c_buffer, *c_property, *result;
    VALUE return_value;

    API_FUNC(1, "buffer_get_pointer", API_RETURN_EMPTY);
    if (NIL_P (buffer) || NIL_P (property))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (buffer, T_STRING);
    Check_Type (property, T_STRING);

    c_buffer = StringValuePtr (buffer);
    c_property = StringValuePtr (property);

    result = API_PTR2STR(weechat_buffer_get_pointer (API_STR2PTR(c_buffer),
                                                     c_property));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_buffer_set: set a buffer property
 */

static VALUE
weechat_ruby_api_buffer_set (VALUE class, VALUE buffer, VALUE property,
                             VALUE value)
{
    char *c_buffer, *c_property, *c_value;

    API_FUNC(1, "buffer_set", API_RETURN_ERROR);
    if (NIL_P (buffer) || NIL_P (property) || NIL_P (value))
        API_WRONG_ARGS(API_RETURN_ERROR);

    Check_Type (buffer, T_STRING);
    Check_Type (property, T_STRING);
    Check_Type (value, T_STRING);

    c_buffer = StringValuePtr (buffer);
    c_property = StringValuePtr (property);
    c_value = StringValuePtr (value);

    weechat_buffer_set (API_STR2PTR(c_buffer),
                        c_property,
                        c_value);

    API_RETURN_OK;
}

/*
 * weechat_ruby_api_buffer_string_replace_local_var: replace local variables ($var) in a string,
 *                                                   using value of local variables
 */

static VALUE
weechat_ruby_api_buffer_string_replace_local_var (VALUE class, VALUE buffer, VALUE string)
{
    char *c_buffer, *c_string, *result;
    VALUE return_value;

    API_FUNC(1, "buffer_string_replace_local_var", API_RETURN_ERROR);
    if (NIL_P (buffer) || NIL_P (string))
        API_WRONG_ARGS(API_RETURN_ERROR);

    Check_Type (buffer, T_STRING);
    Check_Type (string, T_STRING);

    c_buffer = StringValuePtr (buffer);
    c_string = StringValuePtr (string);

    result = weechat_buffer_string_replace_local_var (API_STR2PTR(c_buffer), c_string);

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_buffer_match_list: return 1 if buffer matches list of buffers
 */

static VALUE
weechat_ruby_api_buffer_match_list (VALUE class, VALUE buffer, VALUE string)
{
    char *c_buffer, *c_string;
    int value;

    API_FUNC(1, "buffer_match_list", API_RETURN_INT(0));
    if (NIL_P (buffer) || NIL_P (string))
        API_WRONG_ARGS(API_RETURN_INT(0));

    Check_Type (buffer, T_STRING);
    Check_Type (string, T_STRING);

    c_buffer = StringValuePtr (buffer);
    c_string = StringValuePtr (string);

    value = weechat_buffer_match_list (API_STR2PTR(c_buffer),
                                       c_string);

    API_RETURN_INT(value);
}

/*
 * weechat_ruby_api_current_window: get current window
 */

static VALUE
weechat_ruby_api_current_window (VALUE class)
{
    char *result;
    VALUE return_value;

    API_FUNC(1, "current_window", API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_current_window ());

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_window_search_with_buffer: search a window with buffer
 *                                             pointer
 */

static VALUE
weechat_ruby_api_window_search_with_buffer (VALUE class, VALUE buffer)
{
    char *c_buffer, *result;
    VALUE return_value;

    API_FUNC(1, "window_search_with_buffer", API_RETURN_EMPTY);
    if (NIL_P (buffer))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (buffer, T_STRING);

    c_buffer = StringValuePtr (buffer);

    result = API_PTR2STR(weechat_window_search_with_buffer (API_STR2PTR(c_buffer)));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_window_get_integer: get a window property as integer
 */

static VALUE
weechat_ruby_api_window_get_integer (VALUE class, VALUE window, VALUE property)
{
    char *c_window, *c_property;
    int value;

    API_FUNC(1, "window_get_integer", API_RETURN_INT(-1));
    if (NIL_P (window) || NIL_P (property))
        API_WRONG_ARGS(API_RETURN_INT(-1));

    Check_Type (window, T_STRING);
    Check_Type (property, T_STRING);

    c_window = StringValuePtr (window);
    c_property = StringValuePtr (property);

    value = weechat_window_get_integer (API_STR2PTR(c_window),
                                        c_property);

    API_RETURN_INT(value);
}

/*
 * weechat_ruby_api_window_get_string: get a window property as string
 */

static VALUE
weechat_ruby_api_window_get_string (VALUE class, VALUE window, VALUE property)
{
    char *c_window, *c_property;
    const char *result;

    API_FUNC(1, "window_get_string", API_RETURN_EMPTY);
    if (NIL_P (window) || NIL_P (property))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (window, T_STRING);
    Check_Type (property, T_STRING);

    c_window = StringValuePtr (window);
    c_property = StringValuePtr (property);

    result = weechat_window_get_string (API_STR2PTR(c_window),
                                        c_property);

    API_RETURN_STRING(result);
}

/*
 * weechat_ruby_api_window_get_pointer: get a window property as pointer
 */

static VALUE
weechat_ruby_api_window_get_pointer (VALUE class, VALUE window, VALUE property)
{
    char *c_window, *c_property, *result;
    VALUE return_value;

    API_FUNC(1, "window_get_pointer", API_RETURN_EMPTY);
    if (NIL_P (window) || NIL_P (property))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (window, T_STRING);
    Check_Type (property, T_STRING);

    c_window = StringValuePtr (window);
    c_property = StringValuePtr (property);

    result = API_PTR2STR(weechat_window_get_pointer (API_STR2PTR(c_window),
                                                     c_property));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_window_set_title: set window title
 */

static VALUE
weechat_ruby_api_window_set_title (VALUE class, VALUE title)
{
    char *c_title;

    API_FUNC(1, "window_set_title", API_RETURN_ERROR);
    if (NIL_P (title))
        API_WRONG_ARGS(API_RETURN_ERROR);

    Check_Type (title, T_STRING);

    c_title = StringValuePtr (title);

    weechat_window_set_title (c_title);

    API_RETURN_OK;
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

    API_FUNC(1, "nicklist_add_group", API_RETURN_EMPTY);
    if (NIL_P (buffer) || NIL_P (parent_group) || NIL_P (name) || NIL_P (color)
        || NIL_P (visible))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (buffer, T_STRING);
    Check_Type (parent_group, T_STRING);
    Check_Type (name, T_STRING);
    Check_Type (color, T_STRING);
    Check_Type (visible, T_FIXNUM);

    c_buffer = StringValuePtr (buffer);
    c_parent_group = StringValuePtr (parent_group);
    c_name = StringValuePtr (name);
    c_color = StringValuePtr (color);
    c_visible = FIX2INT (visible);

    result = API_PTR2STR(weechat_nicklist_add_group (API_STR2PTR(c_buffer),
                                                     API_STR2PTR(c_parent_group),
                                                     c_name,
                                                     c_color,
                                                     c_visible));

    API_RETURN_STRING_FREE(result);
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

    API_FUNC(1, "nicklist_search_group", API_RETURN_EMPTY);
    if (NIL_P (buffer) || NIL_P (from_group) || NIL_P (name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (buffer, T_STRING);
    Check_Type (from_group, T_STRING);
    Check_Type (name, T_STRING);

    c_buffer = StringValuePtr (buffer);
    c_from_group = StringValuePtr (from_group);
    c_name = StringValuePtr (name);

    result = API_PTR2STR(weechat_nicklist_search_group (API_STR2PTR(c_buffer),
                                                        API_STR2PTR(c_from_group),
                                                        c_name));

    API_RETURN_STRING_FREE(result);
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

    API_FUNC(1, "nicklist_add_nick", API_RETURN_EMPTY);
    if (NIL_P (buffer) || NIL_P (group) || NIL_P (name) || NIL_P (color)
        || NIL_P (prefix) || NIL_P (prefix_color) || NIL_P (visible))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (buffer, T_STRING);
    Check_Type (group, T_STRING);
    Check_Type (name, T_STRING);
    Check_Type (color, T_STRING);
    Check_Type (prefix, T_STRING);
    Check_Type (prefix_color, T_STRING);
    Check_Type (visible, T_FIXNUM);

    c_buffer = StringValuePtr (buffer);
    c_group = StringValuePtr (group);
    c_name = StringValuePtr (name);
    c_color = StringValuePtr (color);
    c_prefix = StringValuePtr (prefix);
    c_prefix_color = StringValuePtr (prefix_color);
    c_visible = FIX2INT (visible);

    result = API_PTR2STR(weechat_nicklist_add_nick (API_STR2PTR(c_buffer),
                                                    API_STR2PTR(c_group),
                                                    c_name,
                                                    c_color,
                                                    c_prefix,
                                                    c_prefix_color,
                                                    c_visible));

    API_RETURN_STRING_FREE(result);
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

    API_FUNC(1, "nicklist_search_nick", API_RETURN_EMPTY);
    if (NIL_P (buffer) || NIL_P (from_group) || NIL_P (name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (buffer, T_STRING);
    Check_Type (from_group, T_STRING);
    Check_Type (name, T_STRING);

    c_buffer = StringValuePtr (buffer);
    c_from_group = StringValuePtr (from_group);
    c_name = StringValuePtr (name);

    result = API_PTR2STR(weechat_nicklist_search_nick (API_STR2PTR(c_buffer),
                                                       API_STR2PTR(c_from_group),
                                                       c_name));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_nicklist_remove_group: remove a group from nicklist
 */

static VALUE
weechat_ruby_api_nicklist_remove_group (VALUE class, VALUE buffer, VALUE group)
{
    char *c_buffer, *c_group;

    API_FUNC(1, "nicklist_remove_group", API_RETURN_ERROR);
    if (NIL_P (buffer) || NIL_P (group))
        API_WRONG_ARGS(API_RETURN_ERROR);

    Check_Type (buffer, T_STRING);
    Check_Type (group, T_STRING);

    c_buffer = StringValuePtr (buffer);
    c_group = StringValuePtr (group);

    weechat_nicklist_remove_group (API_STR2PTR(c_buffer),
                                   API_STR2PTR(c_group));

    API_RETURN_OK;
}

/*
 * weechat_ruby_api_nicklist_remove_nick: remove a nick from nicklist
 */

static VALUE
weechat_ruby_api_nicklist_remove_nick (VALUE class, VALUE buffer, VALUE nick)
{
    char *c_buffer, *c_nick;

    API_FUNC(1, "nicklist_remove_nick", API_RETURN_ERROR);
    if (NIL_P (buffer) || NIL_P (nick))
        API_WRONG_ARGS(API_RETURN_ERROR);

    Check_Type (buffer, T_STRING);
    Check_Type (nick, T_STRING);

    c_buffer = StringValuePtr (buffer);
    c_nick = StringValuePtr (nick);

    weechat_nicklist_remove_nick (API_STR2PTR(c_buffer),
                                  API_STR2PTR(c_nick));

    API_RETURN_OK;
}

/*
 * weechat_ruby_api_nicklist_remove_all: remove all groups/nicks from nicklist
 */

static VALUE
weechat_ruby_api_nicklist_remove_all (VALUE class, VALUE buffer)
{
    char *c_buffer;

    API_FUNC(1, "nicklist_remove_all", API_RETURN_ERROR);
    if (NIL_P (buffer))
        API_WRONG_ARGS(API_RETURN_ERROR);

    Check_Type (buffer, T_STRING);

    c_buffer = StringValuePtr (buffer);

    weechat_nicklist_remove_all (API_STR2PTR(c_buffer));

    API_RETURN_OK;
}

/*
 * weechat_ruby_api_nicklist_group_get_integer: get a group property as integer
 */

static VALUE
weechat_ruby_api_nicklist_group_get_integer (VALUE class, VALUE buffer,
                                             VALUE group, VALUE property)
{
    char *c_buffer, *c_group, *c_property;
    int value;

    API_FUNC(1, "nicklist_group_get_integer", API_RETURN_INT(-1));
    if (NIL_P (buffer) || NIL_P (group) || NIL_P (property))
        API_WRONG_ARGS(API_RETURN_INT(-1));

    Check_Type (buffer, T_STRING);
    Check_Type (group, T_STRING);
    Check_Type (property, T_STRING);

    c_buffer = StringValuePtr (buffer);
    c_group = StringValuePtr (group);
    c_property = StringValuePtr (property);

    value = weechat_nicklist_group_get_integer (API_STR2PTR(c_buffer),
                                                API_STR2PTR(c_group),
                                                c_property);

    API_RETURN_INT(value);
}

/*
 * weechat_ruby_api_nicklist_group_get_string: get a group property as string
 */

static VALUE
weechat_ruby_api_nicklist_group_get_string (VALUE class, VALUE buffer,
                                            VALUE group, VALUE property)
{
    char *c_buffer, *c_group, *c_property;
    const char *result;

    API_FUNC(1, "nicklist_group_get_string", API_RETURN_EMPTY);
    if (NIL_P (buffer) || NIL_P (group) || NIL_P (property))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (buffer, T_STRING);
    Check_Type (group, T_STRING);
    Check_Type (property, T_STRING);

    c_buffer = StringValuePtr (buffer);
    c_group = StringValuePtr (group);
    c_property = StringValuePtr (property);

    result = weechat_nicklist_group_get_string (API_STR2PTR(c_buffer),
                                                API_STR2PTR(c_group),
                                                c_property);

    API_RETURN_STRING(result);
}

/*
 * weechat_ruby_api_nicklist_group_get_pointer: get a group property as pointer
 */

static VALUE
weechat_ruby_api_nicklist_group_get_pointer (VALUE class, VALUE buffer,
                                             VALUE group, VALUE property)
{
    char *c_buffer, *c_group, *c_property, *result;
    VALUE return_value;

    API_FUNC(1, "nicklist_group_get_pointer", API_RETURN_EMPTY);
    if (NIL_P (buffer) || NIL_P (group) || NIL_P (property))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (buffer, T_STRING);
    Check_Type (group, T_STRING);
    Check_Type (property, T_STRING);

    c_buffer = StringValuePtr (buffer);
    c_group = StringValuePtr (group);
    c_property = StringValuePtr (property);

    result = API_PTR2STR(weechat_nicklist_group_get_pointer (API_STR2PTR(c_buffer),
                                                             API_STR2PTR(c_group),
                                                             c_property));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_nicklist_group_set: set a group property
 */

static VALUE
weechat_ruby_api_nicklist_group_set (VALUE class, VALUE buffer, VALUE group,
                                     VALUE property, VALUE value)
{
    char *c_buffer, *c_group, *c_property, *c_value;

    API_FUNC(1, "nicklist_group_set", API_RETURN_ERROR);
    if (NIL_P (buffer) || NIL_P (group) || NIL_P (property) || NIL_P (value))
        API_WRONG_ARGS(API_RETURN_ERROR);

    Check_Type (buffer, T_STRING);
    Check_Type (group, T_STRING);
    Check_Type (property, T_STRING);
    Check_Type (value, T_STRING);

    c_buffer = StringValuePtr (buffer);
    c_group = StringValuePtr (group);
    c_property = StringValuePtr (property);
    c_value = StringValuePtr (value);

    weechat_nicklist_group_set (API_STR2PTR(c_buffer),
                                API_STR2PTR(c_group),
                                c_property,
                                c_value);

    API_RETURN_OK;
}

/*
 * weechat_ruby_api_nicklist_nick_get_integer: get a nick property as integer
 */

static VALUE
weechat_ruby_api_nicklist_nick_get_integer (VALUE class, VALUE buffer,
                                            VALUE nick, VALUE property)
{
    char *c_buffer, *c_nick, *c_property;
    int value;

    API_FUNC(1, "nicklist_nick_get_integer", API_RETURN_INT(-1));
    if (NIL_P (buffer) || NIL_P (nick) || NIL_P (property))
        API_WRONG_ARGS(API_RETURN_INT(-1));

    Check_Type (buffer, T_STRING);
    Check_Type (nick, T_STRING);
    Check_Type (property, T_STRING);

    c_buffer = StringValuePtr (buffer);
    c_nick = StringValuePtr (nick);
    c_property = StringValuePtr (property);

    value = weechat_nicklist_nick_get_integer (API_STR2PTR(c_buffer),
                                               API_STR2PTR(c_nick),
                                               c_property);

    API_RETURN_INT(value);
}

/*
 * weechat_ruby_api_nicklist_nick_get_string: get a nick property as string
 */

static VALUE
weechat_ruby_api_nicklist_nick_get_string (VALUE class, VALUE buffer,
                                           VALUE nick, VALUE property)
{
    char *c_buffer, *c_nick, *c_property;
    const char *result;

    API_FUNC(1, "nicklist_nick_get_string", API_RETURN_EMPTY);
    if (NIL_P (buffer) || NIL_P (nick) || NIL_P (property))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (buffer, T_STRING);
    Check_Type (nick, T_STRING);
    Check_Type (property, T_STRING);

    c_buffer = StringValuePtr (buffer);
    c_nick = StringValuePtr (nick);
    c_property = StringValuePtr (property);

    result = weechat_nicklist_nick_get_string (API_STR2PTR(c_buffer),
                                               API_STR2PTR(c_nick),
                                               c_property);

    API_RETURN_STRING(result);
}

/*
 * weechat_ruby_api_nicklist_nick_get_pointer: get a nick property as pointer
 */

static VALUE
weechat_ruby_api_nicklist_nick_get_pointer (VALUE class, VALUE buffer,
                                            VALUE nick, VALUE property)
{
    char *c_buffer, *c_nick, *c_property, *result;
    VALUE return_value;

    API_FUNC(1, "nicklist_nick_get_pointer", API_RETURN_EMPTY);
    if (NIL_P (buffer) || NIL_P (nick) || NIL_P (property))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (buffer, T_STRING);
    Check_Type (nick, T_STRING);
    Check_Type (property, T_STRING);

    c_buffer = StringValuePtr (buffer);
    c_nick = StringValuePtr (nick);
    c_property = StringValuePtr (property);

    result = API_PTR2STR(weechat_nicklist_nick_get_pointer (API_STR2PTR(c_buffer),
                                                            API_STR2PTR(c_nick),
                                                            c_property));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_nicklist_nick_set: set a nick property
 */

static VALUE
weechat_ruby_api_nicklist_nick_set (VALUE class, VALUE buffer, VALUE nick,
                                    VALUE property, VALUE value)
{
    char *c_buffer, *c_nick, *c_property, *c_value;

    API_FUNC(1, "nicklist_nick_set", API_RETURN_ERROR);
    if (NIL_P (buffer) || NIL_P (nick) || NIL_P (property) || NIL_P (value))
        API_WRONG_ARGS(API_RETURN_ERROR);

    Check_Type (buffer, T_STRING);
    Check_Type (nick, T_STRING);
    Check_Type (property, T_STRING);
    Check_Type (value, T_STRING);

    c_buffer = StringValuePtr (buffer);
    c_nick = StringValuePtr (nick);
    c_property = StringValuePtr (property);
    c_value = StringValuePtr (value);

    weechat_nicklist_nick_set (API_STR2PTR(c_buffer),
                               API_STR2PTR(c_nick),
                               c_property,
                               c_value);

    API_RETURN_OK;
}

/*
 * weechat_ruby_api_bar_item_search: search a bar item
 */

static VALUE
weechat_ruby_api_bar_item_search (VALUE class, VALUE name)
{
    char *c_name, *result;
    VALUE return_value;

    API_FUNC(1, "bar_item_search", API_RETURN_EMPTY);
    if (NIL_P (name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (name, T_STRING);

    c_name = StringValuePtr (name);

    result = API_PTR2STR(weechat_bar_item_search (c_name));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_bar_item_build_cb: callback for building bar item
 */

char *
weechat_ruby_api_bar_item_build_cb (void *data, struct t_gui_bar_item *item,
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

        ret = (char *)weechat_ruby_exec (script_callback->script,
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
 * weechat_ruby_api_bar_item_new: add a new bar item
 */

static VALUE
weechat_ruby_api_bar_item_new (VALUE class, VALUE name, VALUE function,
                               VALUE data)
{
    char *c_name, *c_function, *c_data, *result;
    VALUE return_value;

    API_FUNC(1, "bar_item_new", API_RETURN_EMPTY);
    if (NIL_P (name) || NIL_P (function) || NIL_P (data))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (name, T_STRING);
    Check_Type (function, T_STRING);
    Check_Type (data, T_STRING);

    c_name = StringValuePtr (name);
    c_function = StringValuePtr (function);
    c_data = StringValuePtr (data);

    result = API_PTR2STR(plugin_script_api_bar_item_new (weechat_ruby_plugin,
                                                         ruby_current_script,
                                                         c_name,
                                                         &weechat_ruby_api_bar_item_build_cb,
                                                         c_function,
                                                         c_data));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_bar_item_update: update a bar item on screen
 */

static VALUE
weechat_ruby_api_bar_item_update (VALUE class, VALUE name)
{
    char *c_name;

    API_FUNC(1, "bar_item_update", API_RETURN_ERROR);
    if (NIL_P (name))
        API_WRONG_ARGS(API_RETURN_ERROR);

    Check_Type (name, T_STRING);

    c_name = StringValuePtr (name);

    weechat_bar_item_update (c_name);

    API_RETURN_OK;
}

/*
 * weechat_ruby_api_bar_item_remove: remove a bar item
 */

static VALUE
weechat_ruby_api_bar_item_remove (VALUE class, VALUE item)
{
    char *c_item;

    API_FUNC(1, "bar_item_remove", API_RETURN_ERROR);
    if (NIL_P (item))
        API_WRONG_ARGS(API_RETURN_ERROR);

    Check_Type (item, T_STRING);

    c_item = StringValuePtr (item);

    plugin_script_api_bar_item_remove (weechat_ruby_plugin,
                                       ruby_current_script,
                                       API_STR2PTR(c_item));

    API_RETURN_OK;
}

/*
 * weechat_ruby_api_bar_search: search a bar
 */

static VALUE
weechat_ruby_api_bar_search (VALUE class, VALUE name)
{
    char *c_name, *result;
    VALUE return_value;

    API_FUNC(1, "bar_search", API_RETURN_EMPTY);
    if (NIL_P (name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (name, T_STRING);

    c_name = StringValuePtr (name);

    result = API_PTR2STR(weechat_bar_search (c_name));

    API_RETURN_STRING_FREE(result);
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

    API_FUNC(1, "bar_new", API_RETURN_EMPTY);
    if (NIL_P (name) || NIL_P (hidden) || NIL_P (priority) || NIL_P (type)
        || NIL_P (conditions) || NIL_P (position) || NIL_P (filling_top_bottom)
        || NIL_P (filling_left_right) || NIL_P (size) || NIL_P (size_max)
        || NIL_P (color_fg) || NIL_P (color_delim) || NIL_P (color_bg)
        || NIL_P (separator) || NIL_P (items))
        API_WRONG_ARGS(API_RETURN_EMPTY);

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

    c_name = StringValuePtr (name);
    c_hidden = StringValuePtr (hidden);
    c_priority = StringValuePtr (priority);
    c_type = StringValuePtr (type);
    c_conditions = StringValuePtr (conditions);
    c_position = StringValuePtr (position);
    c_filling_top_bottom = StringValuePtr (filling_top_bottom);
    c_filling_left_right = StringValuePtr (filling_left_right);
    c_size = StringValuePtr (size);
    c_size_max = StringValuePtr (size_max);
    c_color_fg = StringValuePtr (color_fg);
    c_color_delim = StringValuePtr (color_delim);
    c_color_bg = StringValuePtr (color_bg);
    c_separator = StringValuePtr (separator);
    c_items = StringValuePtr (items);

    result = API_PTR2STR(weechat_bar_new (c_name,
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

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_bar_set: set a bar property
 */

static VALUE
weechat_ruby_api_bar_set (VALUE class, VALUE bar, VALUE property, VALUE value)
{
    char *c_bar, *c_property, *c_value;

    API_FUNC(1, "bar_set", API_RETURN_ERROR);
    if (NIL_P (bar) || NIL_P (property) || NIL_P (value))
        API_WRONG_ARGS(API_RETURN_ERROR);

    Check_Type (bar, T_STRING);
    Check_Type (property, T_STRING);
    Check_Type (value, T_STRING);

    c_bar = StringValuePtr (bar);
    c_property = StringValuePtr (property);
    c_value = StringValuePtr (value);

    weechat_bar_set (API_STR2PTR(c_bar),
                     c_property,
                     c_value);

    API_RETURN_OK;
}

/*
 * weechat_ruby_api_bar_update: update a bar on screen
 */

static VALUE
weechat_ruby_api_bar_update (VALUE class, VALUE name)
{
    char *c_name;

    API_FUNC(1, "bar_update", API_RETURN_ERROR);
    if (NIL_P (name))
        API_WRONG_ARGS(API_RETURN_ERROR);

    Check_Type (name, T_STRING);

    c_name = StringValuePtr (name);

    weechat_bar_update (c_name);

    API_RETURN_OK;
}

/*
 * weechat_ruby_api_bar_remove: remove a bar
 */

static VALUE
weechat_ruby_api_bar_remove (VALUE class, VALUE bar)
{
    char *c_bar;

    API_FUNC(1, "bar_remove", API_RETURN_ERROR);
    if (NIL_P (bar))
        API_WRONG_ARGS(API_RETURN_ERROR);

    Check_Type (bar, T_STRING);

    c_bar = StringValuePtr (bar);

    weechat_bar_remove (API_STR2PTR(c_bar));

    API_RETURN_OK;
}

/*
 * weechat_ruby_api_command: send command to server
 */

static VALUE
weechat_ruby_api_command (VALUE class, VALUE buffer, VALUE command)
{
    char *c_buffer, *c_command;

    API_FUNC(1, "command", API_RETURN_ERROR);
    if (NIL_P (buffer) || NIL_P (command))
        API_WRONG_ARGS(API_RETURN_ERROR);

    Check_Type (buffer, T_STRING);
    Check_Type (command, T_STRING);

    c_buffer = StringValuePtr (buffer);
    c_command = StringValuePtr (command);

    plugin_script_api_command (weechat_ruby_plugin,
                               ruby_current_script,
                               API_STR2PTR(c_buffer),
                               c_command);

    API_RETURN_OK;
}

/*
 * weechat_ruby_api_info_get: get info (as string)
 */

static VALUE
weechat_ruby_api_info_get (VALUE class, VALUE info_name, VALUE arguments)
{
    char *c_info_name, *c_arguments;
    const char *result;

    API_FUNC(1, "info_get", API_RETURN_EMPTY);
    if (NIL_P (info_name) || NIL_P (arguments))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (info_name, T_STRING);
    Check_Type (arguments, T_STRING);

    c_info_name = StringValuePtr (info_name);
    c_arguments = StringValuePtr (arguments);

    result = weechat_info_get (c_info_name, c_arguments);

    API_RETURN_STRING(result);
}

/*
 * weechat_ruby_api_info_get_hashtable: get info (as hashtable)
 */

static VALUE
weechat_ruby_api_info_get_hashtable (VALUE class, VALUE info_name,
                                     VALUE hash)
{
    char *c_info_name;
    struct t_hashtable *c_hashtable, *result_hashtable;
    VALUE result_hash;

    API_FUNC(1, "info_get_hashtable", API_RETURN_EMPTY);
    if (NIL_P (info_name) || NIL_P (hash))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (info_name, T_STRING);
    Check_Type (hash, T_HASH);

    c_info_name = StringValuePtr (info_name);
    c_hashtable = weechat_ruby_hash_to_hashtable (hash,
                                                  WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                                  WEECHAT_HASHTABLE_STRING,
                                                  WEECHAT_HASHTABLE_STRING);

    result_hashtable = weechat_info_get_hashtable (c_info_name, c_hashtable);
    result_hash = weechat_ruby_hashtable_to_hash (result_hashtable);

    if (c_hashtable)
        weechat_hashtable_free (c_hashtable);
    if (result_hashtable)
        weechat_hashtable_free (result_hashtable);

    return result_hash;
}

/*
 * weechat_ruby_api_infolist_new: create new infolist
 */

static VALUE
weechat_ruby_api_infolist_new (VALUE class)
{
    char *result;
    VALUE return_value;

    API_FUNC(1, "infolist_new", API_RETURN_EMPTY);

    result = API_PTR2STR(weechat_infolist_new ());

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_infolist_new_item: create new item in infolist
 */

static VALUE
weechat_ruby_api_infolist_new_item (VALUE class, VALUE infolist)
{
    char *c_infolist, *result;
    VALUE return_value;

    API_FUNC(1, "infolist_new_item", API_RETURN_EMPTY);
    if (NIL_P (infolist))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (infolist, T_STRING);

    c_infolist = StringValuePtr (infolist);

    result = API_PTR2STR(weechat_infolist_new_item (API_STR2PTR(c_infolist)));

    API_RETURN_STRING_FREE(result);
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

    API_FUNC(1, "infolist_new_var_integer", API_RETURN_EMPTY);
    if (NIL_P (infolist) || NIL_P (name) || NIL_P (value))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (infolist, T_STRING);
    Check_Type (name, T_STRING);
    Check_Type (value, T_FIXNUM);

    c_infolist = StringValuePtr (infolist);
    c_name = StringValuePtr (name);
    c_value = FIX2INT (value);

    result = API_PTR2STR(weechat_infolist_new_var_integer (API_STR2PTR(c_infolist),
                                                           c_name,
                                                           c_value));

    API_RETURN_STRING_FREE(result);
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

    API_FUNC(1, "infolist_new_var_string", API_RETURN_EMPTY);
    if (NIL_P (infolist) || NIL_P (name) || NIL_P (value))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (infolist, T_STRING);
    Check_Type (name, T_STRING);
    Check_Type (value, T_STRING);

    c_infolist = StringValuePtr (infolist);
    c_name = StringValuePtr (name);
    c_value = StringValuePtr (value);

    result = API_PTR2STR(weechat_infolist_new_var_string (API_STR2PTR(c_infolist),
                                                          c_name,
                                                          c_value));

    API_RETURN_STRING_FREE(result);
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

    API_FUNC(1, "infolist_new_var_pointer", API_RETURN_EMPTY);
    if (NIL_P (infolist) || NIL_P (name) || NIL_P (value))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (infolist, T_STRING);
    Check_Type (name, T_STRING);
    Check_Type (value, T_STRING);

    c_infolist = StringValuePtr (infolist);
    c_name = StringValuePtr (name);
    c_value = StringValuePtr (value);

    result = API_PTR2STR(weechat_infolist_new_var_pointer (API_STR2PTR(c_infolist),
                                                           c_name,
                                                           API_STR2PTR(c_value)));

    API_RETURN_STRING_FREE(result);
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

    API_FUNC(1, "infolist_new_var_time", API_RETURN_EMPTY);
    if (NIL_P (infolist) || NIL_P (name) || NIL_P (value))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (infolist, T_STRING);
    Check_Type (name, T_STRING);
    Check_Type (value, T_FIXNUM);

    c_infolist = StringValuePtr (infolist);
    c_name = StringValuePtr (name);
    c_value = FIX2INT (value);

    result = API_PTR2STR(weechat_infolist_new_var_time (API_STR2PTR(c_infolist),
                                                        c_name,
                                                        c_value));

    API_RETURN_STRING_FREE(result);
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

    API_FUNC(1, "infolist_get", API_RETURN_EMPTY);
    if (NIL_P (name) || NIL_P (pointer) || NIL_P (arguments))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (name, T_STRING);
    Check_Type (pointer, T_STRING);
    Check_Type (arguments, T_STRING);

    c_name = StringValuePtr (name);
    c_pointer = StringValuePtr (pointer);
    c_arguments = StringValuePtr (arguments);

    result = API_PTR2STR(weechat_infolist_get (c_name,
                                               API_STR2PTR(c_pointer),
                                               c_arguments));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_infolist_next: move item pointer to next item in infolist
 */

static VALUE
weechat_ruby_api_infolist_next (VALUE class, VALUE infolist)
{
    char *c_infolist;
    int value;

    API_FUNC(1, "infolist_next", API_RETURN_INT(0));
    if (NIL_P (infolist))
        API_WRONG_ARGS(API_RETURN_INT(0));

    Check_Type (infolist, T_STRING);

    c_infolist = StringValuePtr (infolist);

    value = weechat_infolist_next (API_STR2PTR(c_infolist));

    API_RETURN_INT(value);
}

/*
 * weechat_ruby_api_infolist_prev: move item pointer to previous item in infolist
 */

static VALUE
weechat_ruby_api_infolist_prev (VALUE class, VALUE infolist)
{
    char *c_infolist;
    int value;

    API_FUNC(1, "infolist_prev", API_RETURN_INT(0));
    if (NIL_P (infolist))
        API_WRONG_ARGS(API_RETURN_INT(0));

    Check_Type (infolist, T_STRING);

    c_infolist = StringValuePtr (infolist);

    value = weechat_infolist_prev (API_STR2PTR(c_infolist));

    API_RETURN_INT(value);
}

/*
 * weechat_ruby_api_infolist_reset_item_cursor: reset pointer to current item
 *                                              in infolist
 */

static VALUE
weechat_ruby_api_infolist_reset_item_cursor (VALUE class, VALUE infolist)
{
    char *c_infolist;

    API_FUNC(1, "infolist_reset_item_cursor", API_RETURN_ERROR);
    if (NIL_P (infolist))
        API_WRONG_ARGS(API_RETURN_ERROR);

    Check_Type (infolist, T_STRING);

    c_infolist = StringValuePtr (infolist);

    weechat_infolist_reset_item_cursor (API_STR2PTR(c_infolist));

    API_RETURN_OK;
}

/*
 * weechat_ruby_api_infolist_fields: get list of fields for current item of infolist
 */

static VALUE
weechat_ruby_api_infolist_fields (VALUE class, VALUE infolist)
{
    char *c_infolist;
    const char *result;

    API_FUNC(1, "infolist_fields", API_RETURN_EMPTY);
    if (NIL_P (infolist))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (infolist, T_STRING);

    c_infolist = StringValuePtr (infolist);

    result = weechat_infolist_fields (API_STR2PTR(c_infolist));

    API_RETURN_STRING(result);
}

/*
 * weechat_ruby_api_infolist_integer: get integer value of a variable in infolist
 */

static VALUE
weechat_ruby_api_infolist_integer (VALUE class, VALUE infolist, VALUE variable)
{
    char *c_infolist, *c_variable;
    int value;

    API_FUNC(1, "infolist_integer", API_RETURN_INT(0));
    if (NIL_P (infolist) || NIL_P (variable))
        API_WRONG_ARGS(API_RETURN_INT(0));

    Check_Type (infolist, T_STRING);
    Check_Type (variable, T_STRING);

    c_infolist = StringValuePtr (infolist);
    c_variable = StringValuePtr (variable);

    value = weechat_infolist_integer (API_STR2PTR(c_infolist), c_variable);

    API_RETURN_INT(value);
}

/*
 * weechat_ruby_api_infolist_string: get string value of a variable in infolist
 */

static VALUE
weechat_ruby_api_infolist_string (VALUE class, VALUE infolist, VALUE variable)
{
    char *c_infolist, *c_variable;
    const char *result;

    API_FUNC(1, "infolist_string", API_RETURN_EMPTY);
    if (NIL_P (infolist) || NIL_P (variable))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (infolist, T_STRING);
    Check_Type (variable, T_STRING);

    c_infolist = StringValuePtr (infolist);
    c_variable = StringValuePtr (variable);

    result = weechat_infolist_string (API_STR2PTR(c_infolist), c_variable);

    API_RETURN_STRING(result);
}

/*
 * weechat_ruby_api_infolist_pointer: get pointer value of a variable in infolist
 */

static VALUE
weechat_ruby_api_infolist_pointer (VALUE class, VALUE infolist, VALUE variable)
{
    char *c_infolist, *c_variable, *result;
    VALUE return_value;

    API_FUNC(1, "infolist_pointer", API_RETURN_EMPTY);
    if (NIL_P (infolist) || NIL_P (variable))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (infolist, T_STRING);
    Check_Type (variable, T_STRING);

    c_infolist = StringValuePtr (infolist);
    c_variable = StringValuePtr (variable);

    result = API_PTR2STR(weechat_infolist_pointer (API_STR2PTR(c_infolist), c_variable));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_infolist_time: get time value of a variable in infolist
 */

static VALUE
weechat_ruby_api_infolist_time (VALUE class, VALUE infolist, VALUE variable)
{
    char *c_infolist, *c_variable, timebuffer[64], *result;
    time_t time;
    struct tm *date_tmp;
    VALUE return_value;

    API_FUNC(1, "infolist_time", API_RETURN_EMPTY);
    if (NIL_P (infolist) || NIL_P (variable))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (infolist, T_STRING);
    Check_Type (variable, T_STRING);

    c_infolist = StringValuePtr (infolist);
    c_variable = StringValuePtr (variable);

    timebuffer[0] = '\0';
    time = weechat_infolist_time (API_STR2PTR(c_infolist), c_variable);
    date_tmp = localtime (&time);
    if (date_tmp)
        strftime (timebuffer, sizeof (timebuffer), "%F %T", date_tmp);
    result = strdup (timebuffer);

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_infolist_free: free infolist
 */

static VALUE
weechat_ruby_api_infolist_free (VALUE class, VALUE infolist)
{
    char *c_infolist;

    API_FUNC(1, "infolist_free", API_RETURN_ERROR);
    if (NIL_P (infolist))
        API_WRONG_ARGS(API_RETURN_ERROR);

    Check_Type (infolist, T_STRING);

    c_infolist = StringValuePtr (infolist);

    weechat_infolist_free (API_STR2PTR(c_infolist));

    API_RETURN_OK;
}

/*
 * weechat_ruby_api_hdata_get: get hdata
 */

static VALUE
weechat_ruby_api_hdata_get (VALUE class, VALUE name)
{
    char *c_name, *result;
    VALUE return_value;

    API_FUNC(1, "hdata_get", API_RETURN_EMPTY);
    if (NIL_P (name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (name, T_STRING);

    c_name = StringValuePtr (name);

    result = API_PTR2STR(weechat_hdata_get (c_name));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_hdata_get_var_offset: get offset of variable in hdata
 */

static VALUE
weechat_ruby_api_hdata_get_var_offset (VALUE class, VALUE hdata, VALUE name)
{
    char *c_hdata, *c_name;
    int value;

    API_FUNC(1, "hdata_get_var_offset", API_RETURN_INT(0));
    if (NIL_P (hdata) || NIL_P (name))
        API_WRONG_ARGS(API_RETURN_INT(0));

    Check_Type (hdata, T_STRING);
    Check_Type (name, T_STRING);

    c_hdata = StringValuePtr (hdata);
    c_name = StringValuePtr (name);

    value = weechat_hdata_get_var_offset (API_STR2PTR(c_hdata), c_name);

    API_RETURN_INT(value);
}

/*
 * weechat_ruby_api_hdata_get_var_type_string: get type of variable as string
 *                                             in hdata
 */

static VALUE
weechat_ruby_api_hdata_get_var_type_string (VALUE class, VALUE hdata,
                                            VALUE name)
{
    char *c_hdata, *c_name;
    const char *result;

    API_FUNC(1, "hdata_get_var_type_string", API_RETURN_EMPTY);
    if (NIL_P (hdata) || NIL_P (name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (hdata, T_STRING);
    Check_Type (name, T_STRING);

    c_hdata = StringValuePtr (hdata);
    c_name = StringValuePtr (name);

    result = weechat_hdata_get_var_type_string (API_STR2PTR(c_hdata), c_name);

    API_RETURN_STRING(result);
}

/*
 * weechat_ruby_api_hdata_get_var_array_size: get array size for variable in hdata
 */

static VALUE
weechat_ruby_api_hdata_get_var_array_size (VALUE class, VALUE hdata, VALUE pointer,
                                           VALUE name)
{
    char *c_hdata, *c_pointer, *c_name;
    int value;

    API_FUNC(1, "hdata_get_var_array_size", API_RETURN_INT(-1));
    if (NIL_P (hdata) || NIL_P (pointer) || NIL_P (name))
        API_WRONG_ARGS(API_RETURN_INT(-1));

    Check_Type (hdata, T_STRING);
    Check_Type (pointer, T_STRING);
    Check_Type (name, T_STRING);

    c_hdata = StringValuePtr (hdata);
    c_pointer = StringValuePtr (pointer);
    c_name = StringValuePtr (name);

    value = weechat_hdata_get_var_array_size (API_STR2PTR(c_hdata),
                                              API_STR2PTR(c_pointer),
                                              c_name);

    API_RETURN_INT(value);
}

/*
 * weechat_ruby_api_hdata_get_var_array_size_string: get array size for variable
 *                                                   in hdata (as string)
 */

static VALUE
weechat_ruby_api_hdata_get_var_array_size_string (VALUE class, VALUE hdata,
                                                  VALUE pointer, VALUE name)
{
    char *c_hdata, *c_pointer, *c_name;
    const char *result;

    API_FUNC(1, "hdata_get_var_array_size_string", API_RETURN_EMPTY);
    if (NIL_P (hdata) || NIL_P (pointer) || NIL_P (name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (hdata, T_STRING);
    Check_Type (pointer, T_STRING);
    Check_Type (name, T_STRING);

    c_hdata = StringValuePtr (hdata);
    c_pointer = StringValuePtr (pointer);
    c_name = StringValuePtr (name);

    result = weechat_hdata_get_var_array_size_string (API_STR2PTR(c_hdata),
                                                      API_STR2PTR(c_pointer),
                                                      c_name);

    API_RETURN_STRING(result);
}

/*
 * weechat_ruby_api_hdata_get_var_hdata: get hdata for variable in hdata
 */

static VALUE
weechat_ruby_api_hdata_get_var_hdata (VALUE class, VALUE hdata, VALUE name)
{
    char *c_hdata, *c_name;
    const char *result;

    API_FUNC(1, "hdata_get_var_hdata", API_RETURN_EMPTY);
    if (NIL_P (hdata) || NIL_P (name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (hdata, T_STRING);
    Check_Type (name, T_STRING);

    c_hdata = StringValuePtr (hdata);
    c_name = StringValuePtr (name);

    result = weechat_hdata_get_var_hdata (API_STR2PTR(c_hdata), c_name);

    API_RETURN_STRING(result);
}

/*
 * weechat_ruby_api_hdata_get_list: get list pointer in hdata
 */

static VALUE
weechat_ruby_api_hdata_get_list (VALUE class, VALUE hdata, VALUE name)
{
    char *c_hdata, *c_name, *result;
    VALUE return_value;

    API_FUNC(1, "hdata_get_list", API_RETURN_EMPTY);
    if (NIL_P (hdata) || NIL_P (name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (hdata, T_STRING);
    Check_Type (name, T_STRING);

    c_hdata = StringValuePtr (hdata);
    c_name = StringValuePtr (name);

    result = API_PTR2STR(weechat_hdata_get_list (API_STR2PTR(c_hdata),
                                                 c_name));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_hdata_check_pointer: check pointer with hdata/list
 */

static VALUE
weechat_ruby_api_hdata_check_pointer (VALUE class, VALUE hdata, VALUE list,
                                      VALUE pointer)
{
    char *c_hdata, *c_list, *c_pointer;
    int value;

    API_FUNC(1, "hdata_check_pointer", API_RETURN_INT(0));
    if (NIL_P (hdata) || NIL_P (list) || NIL_P (pointer))
        API_WRONG_ARGS(API_RETURN_INT(0));

    Check_Type (hdata, T_STRING);
    Check_Type (list, T_STRING);
    Check_Type (pointer, T_STRING);

    c_hdata = StringValuePtr (hdata);
    c_list = StringValuePtr (list);
    c_pointer = StringValuePtr (pointer);

    value = weechat_hdata_check_pointer (API_STR2PTR(c_hdata),
                                         API_STR2PTR(c_list),
                                         API_STR2PTR(c_pointer));

    API_RETURN_INT(value);
}

/*
 * weechat_ruby_api_hdata_move: move pointer to another element in list
 */

static VALUE
weechat_ruby_api_hdata_move (VALUE class, VALUE hdata, VALUE pointer,
                             VALUE count)
{
    char *c_hdata, *c_pointer, *result;
    int c_count;
    VALUE return_value;

    API_FUNC(1, "hdata_move", API_RETURN_EMPTY);
    if (NIL_P (hdata) || NIL_P (pointer) || NIL_P (count))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (hdata, T_STRING);
    Check_Type (pointer, T_STRING);
    Check_Type (count, T_FIXNUM);

    c_hdata = StringValuePtr (hdata);
    c_pointer = StringValuePtr (pointer);
    c_count = FIX2INT (count);

    result = weechat_hdata_move (API_STR2PTR(c_hdata),
                                 API_STR2PTR(c_pointer),
                                 c_count);

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_hdata_char: get char value of a variable in structure using
 *                              hdata
 */

static VALUE
weechat_ruby_api_hdata_char (VALUE class, VALUE hdata, VALUE pointer,
                             VALUE name)
{
    char *c_hdata, *c_pointer, *c_name;
    int value;

    API_FUNC(1, "hdata_char", API_RETURN_INT(0));
    if (NIL_P (hdata) || NIL_P (pointer) || NIL_P (name))
        API_WRONG_ARGS(API_RETURN_INT(0));

    Check_Type (hdata, T_STRING);
    Check_Type (pointer, T_STRING);
    Check_Type (name, T_STRING);

    c_hdata = StringValuePtr (hdata);
    c_pointer = StringValuePtr (pointer);
    c_name = StringValuePtr (name);

    value = (int)weechat_hdata_char (API_STR2PTR(c_hdata),
                                     API_STR2PTR(c_pointer),
                                     c_name);

    API_RETURN_INT(value);
}

/*
 * weechat_ruby_api_hdata_integer: get integer value of a variable in structure
 *                                 using hdata
 */

static VALUE
weechat_ruby_api_hdata_integer (VALUE class, VALUE hdata, VALUE pointer,
                                VALUE name)
{
    char *c_hdata, *c_pointer, *c_name;
    int value;

    API_FUNC(1, "hdata_integer", API_RETURN_INT(0));
    if (NIL_P (hdata) || NIL_P (pointer) || NIL_P (name))
        API_WRONG_ARGS(API_RETURN_INT(0));

    Check_Type (hdata, T_STRING);
    Check_Type (pointer, T_STRING);
    Check_Type (name, T_STRING);

    c_hdata = StringValuePtr (hdata);
    c_pointer = StringValuePtr (pointer);
    c_name = StringValuePtr (name);

    value = weechat_hdata_integer (API_STR2PTR(c_hdata),
                                   API_STR2PTR(c_pointer),
                                   c_name);

    API_RETURN_INT(value);
}

/*
 * weechat_ruby_api_hdata_long: get long value of a variable in structure using
 *                              hdata
 */

static VALUE
weechat_ruby_api_hdata_long (VALUE class, VALUE hdata, VALUE pointer,
                             VALUE name)
{
    char *c_hdata, *c_pointer, *c_name;
    long value;

    API_FUNC(1, "hdata_long", API_RETURN_LONG(0));
    if (NIL_P (hdata) || NIL_P (pointer) || NIL_P (name))
        API_WRONG_ARGS(API_RETURN_LONG(0));

    Check_Type (hdata, T_STRING);
    Check_Type (pointer, T_STRING);
    Check_Type (name, T_STRING);

    c_hdata = StringValuePtr (hdata);
    c_pointer = StringValuePtr (pointer);
    c_name = StringValuePtr (name);

    value = weechat_hdata_long (API_STR2PTR(c_hdata),
                                API_STR2PTR(c_pointer),
                                c_name);

    API_RETURN_LONG(value);
}

/*
 * weechat_ruby_api_hdata_string: get string value of a variable in structure
 *                                using hdata
 */

static VALUE
weechat_ruby_api_hdata_string (VALUE class, VALUE hdata, VALUE pointer,
                               VALUE name)
{
    char *c_hdata, *c_pointer, *c_name;
    const char *result;

    API_FUNC(1, "hdata_string", API_RETURN_EMPTY);
    if (NIL_P (hdata) || NIL_P (pointer) || NIL_P (name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (hdata, T_STRING);
    Check_Type (pointer, T_STRING);
    Check_Type (name, T_STRING);

    c_hdata = StringValuePtr (hdata);
    c_pointer = StringValuePtr (pointer);
    c_name = StringValuePtr (name);

    result = weechat_hdata_string (API_STR2PTR(c_hdata),
                                   API_STR2PTR(c_pointer),
                                   c_name);

    API_RETURN_STRING(result);
}

/*
 * weechat_ruby_api_hdata_pointer: get pointer value of a variable in structure
 *                                 using hdata
 */

static VALUE
weechat_ruby_api_hdata_pointer (VALUE class, VALUE hdata, VALUE pointer,
                                VALUE name)
{
    char *c_hdata, *c_pointer, *c_name, *result;
    VALUE return_value;

    API_FUNC(1, "hdata_pointer", API_RETURN_EMPTY);
    if (NIL_P (hdata) || NIL_P (pointer) || NIL_P (name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (hdata, T_STRING);
    Check_Type (pointer, T_STRING);
    Check_Type (name, T_STRING);

    c_hdata = StringValuePtr (hdata);
    c_pointer = StringValuePtr (pointer);
    c_name = StringValuePtr (name);

    result = API_PTR2STR(weechat_hdata_pointer (API_STR2PTR(c_hdata),
                                                API_STR2PTR(c_pointer),
                                                c_name));

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_hdata_time: get time value of a variable in structure using
 *                              hdata
 */

static VALUE
weechat_ruby_api_hdata_time (VALUE class, VALUE hdata, VALUE pointer,
                             VALUE name)
{
    char *c_hdata, *c_pointer, *c_name, timebuffer[64], *result;
    time_t time;
    VALUE return_value;

    API_FUNC(1, "hdata_time", API_RETURN_EMPTY);
    if (NIL_P (hdata) || NIL_P (pointer) || NIL_P (name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (hdata, T_STRING);
    Check_Type (pointer, T_STRING);
    Check_Type (name, T_STRING);

    c_hdata = StringValuePtr (hdata);
    c_pointer = StringValuePtr (pointer);
    c_name = StringValuePtr (name);

    timebuffer[0] = '\0';
    time = weechat_hdata_time (API_STR2PTR(c_hdata),
                               API_STR2PTR(c_pointer),
                               c_name);
    snprintf (timebuffer, sizeof (timebuffer), "%ld", (long int)time);
    result = strdup (timebuffer);

    API_RETURN_STRING_FREE(result);
}

/*
 * weechat_ruby_api_hdata_hashtable: get hashtable value of a variable in
 *                                   structure using hdata
 */

static VALUE
weechat_ruby_api_hdata_hashtable (VALUE class, VALUE hdata, VALUE pointer,
                                  VALUE name)
{
    char *c_hdata, *c_pointer, *c_name;
    VALUE result_hash;

    API_FUNC(1, "hdata_hashtable", API_RETURN_EMPTY);
    if (NIL_P (hdata) || NIL_P (pointer) || NIL_P (name))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (hdata, T_STRING);
    Check_Type (pointer, T_STRING);
    Check_Type (name, T_STRING);

    c_hdata = StringValuePtr (hdata);
    c_pointer = StringValuePtr (pointer);
    c_name = StringValuePtr (name);

    result_hash = weechat_ruby_hashtable_to_hash (
        weechat_hdata_hashtable (API_STR2PTR(c_hdata),
                                 API_STR2PTR(c_pointer),
                                 c_name));

    return result_hash;
}

/*
 * weechat_ruby_api_hdata_update: update data in a hdata
 */

static VALUE
weechat_ruby_api_hdata_update (VALUE class, VALUE hdata, VALUE pointer,
                               VALUE hashtable)
{
    char *c_hdata, *c_pointer;
    struct t_hashtable *c_hashtable;
    int value;

    API_FUNC(1, "hdata_update", API_RETURN_INT(0));
    if (NIL_P (hdata) || NIL_P (pointer) || NIL_P (hashtable))
        API_WRONG_ARGS(API_RETURN_INT(0));

    Check_Type (hdata, T_STRING);
    Check_Type (pointer, T_STRING);
    Check_Type (hashtable, T_HASH);

    c_hdata = StringValuePtr (hdata);
    c_pointer = StringValuePtr (pointer);
    c_hashtable = weechat_ruby_hash_to_hashtable (hashtable,
                                                  WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                                  WEECHAT_HASHTABLE_STRING,
                                                  WEECHAT_HASHTABLE_STRING);

    value = weechat_hdata_update (API_STR2PTR(c_hdata),
                                  API_STR2PTR(c_pointer),
                                  c_hashtable);

    if (c_hashtable)
        weechat_hashtable_free (c_hashtable);

    API_RETURN_INT(value);
}

/*
 * weechat_ruby_api_hdata_get_string: get hdata property as string
 */

static VALUE
weechat_ruby_api_hdata_get_string (VALUE class, VALUE hdata, VALUE property)
{
    char *c_hdata, *c_property;
    const char *result;

    API_FUNC(1, "hdata_get_string", API_RETURN_EMPTY);
    if (NIL_P (hdata) || NIL_P (property))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (hdata, T_STRING);
    Check_Type (property, T_STRING);

    c_hdata = StringValuePtr (hdata);
    c_property = StringValuePtr (property);

    result = weechat_hdata_get_var_type_string (API_STR2PTR(c_hdata),
                                                c_property);

    API_RETURN_STRING(result);
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

    API_FUNC(1, "upgrade_new", API_RETURN_EMPTY);
    if (NIL_P (filename) || NIL_P (write))
        API_WRONG_ARGS(API_RETURN_EMPTY);

    Check_Type (filename, T_STRING);
    Check_Type (write, T_FIXNUM);

    c_filename = StringValuePtr (filename);
    c_write = FIX2INT (write);

    result = API_PTR2STR(weechat_upgrade_new (c_filename,
                                              c_write));

    API_RETURN_STRING_FREE(result);
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

    API_FUNC(1, "upgrade_write_object", API_RETURN_INT(0));
    if (NIL_P (upgrade_file) || NIL_P (object_id) || NIL_P (infolist))
        API_WRONG_ARGS(API_RETURN_INT(0));

    Check_Type (upgrade_file, T_STRING);
    Check_Type (object_id, T_FIXNUM);
    Check_Type (infolist, T_STRING);

    c_upgrade_file = StringValuePtr (upgrade_file);
    c_object_id = FIX2INT (object_id);
    c_infolist = StringValuePtr (infolist);

    rc = weechat_upgrade_write_object (API_STR2PTR(c_upgrade_file),
                                       c_object_id,
                                       API_STR2PTR(c_infolist));

    API_RETURN_INT(rc);
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

        rc = (int *) weechat_ruby_exec (script_callback->script,
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
 * weechat_ruby_api_upgrade_read: read upgrade file
 */

static VALUE
weechat_ruby_api_upgrade_read (VALUE class, VALUE upgrade_file,
                               VALUE function, VALUE data)
{
    char *c_upgrade_file, *c_function, *c_data;
    int rc;

    API_FUNC(1, "upgrade_read", API_RETURN_INT(0));
    if (NIL_P (upgrade_file) || NIL_P (function) || NIL_P (data))
        API_WRONG_ARGS(API_RETURN_INT(0));

    Check_Type (upgrade_file, T_STRING);
    Check_Type (function, T_STRING);
    Check_Type (data, T_STRING);

    c_upgrade_file = StringValuePtr (upgrade_file);
    c_function = StringValuePtr (function);
    c_data = StringValuePtr (data);

    rc = plugin_script_api_upgrade_read (weechat_ruby_plugin,
                                         ruby_current_script,
                                         API_STR2PTR(c_upgrade_file),
                                         &weechat_ruby_api_upgrade_read_cb,
                                         c_function,
                                         c_data);

    API_RETURN_INT(rc);
}

/*
 * weechat_ruby_api_upgrade_close: close upgrade file
 */

static VALUE
weechat_ruby_api_upgrade_close (VALUE class, VALUE upgrade_file)
{
    char *c_upgrade_file;

    API_FUNC(1, "upgrade_close", API_RETURN_ERROR);
    if (NIL_P (upgrade_file))
        API_WRONG_ARGS(API_RETURN_ERROR);

    Check_Type (upgrade_file, T_STRING);

    c_upgrade_file = StringValuePtr (upgrade_file);

    weechat_upgrade_close (API_STR2PTR(c_upgrade_file));

    API_RETURN_OK;
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
    rb_define_const(ruby_mWeechat, "WEECHAT_HOOK_CONNECT_TIMEOUT", INT2NUM(WEECHAT_HOOK_CONNECT_TIMEOUT));
    rb_define_const(ruby_mWeechat, "WEECHAT_HOOK_CONNECT_SOCKET_ERROR", INT2NUM(WEECHAT_HOOK_CONNECT_SOCKET_ERROR));

    rb_define_const(ruby_mWeechat, "WEECHAT_HOOK_SIGNAL_STRING", rb_str_new2(WEECHAT_HOOK_SIGNAL_STRING));
    rb_define_const(ruby_mWeechat, "WEECHAT_HOOK_SIGNAL_INT", rb_str_new2(WEECHAT_HOOK_SIGNAL_INT));
    rb_define_const(ruby_mWeechat, "WEECHAT_HOOK_SIGNAL_POINTER", rb_str_new2(WEECHAT_HOOK_SIGNAL_POINTER));

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
    API_DEF_FUNC(string_eval_expression, 3);
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
    API_DEF_FUNC(config_new_section, 14);
    API_DEF_FUNC(config_search_section, 2);
    API_DEF_FUNC(config_new_option, 17);
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
    API_DEF_FUNC(bar_new, 15);
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
}
