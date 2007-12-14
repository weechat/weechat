/*
 * Copyright (c) 2003-2007 by FlashCode <flashcode@flashtux.org>
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

/* plugin-api.c: WeeChat <--> plugin API */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "../core/weechat.h"
#include "../core/wee-config.h"
#include "../core/wee-hook.h"
#include "../core/wee-input.h"
#include "../core/wee-list.h"
#include "../core/wee-log.h"
#include "../core/wee-string.h"
#include "../core/wee-utf8.h"
#include "../core/wee-util.h"
#include "../gui/gui-chat.h"
#include "../gui/gui-color.h"
#include "../gui/gui-infobar.h"
#include "../gui/gui-input.h"
#include "../gui/gui-keyboard.h"
#include "../gui/gui-nicklist.h"
#include "../gui/gui-status.h"
#include "../gui/gui-window.h"
#include "plugin.h"
#include "plugin-config.h"
#include "plugin-infolist.h"


/*
 * plugin_api_charset_set: set plugin charset
 */

void
plugin_api_charset_set (struct t_weechat_plugin *plugin, char *charset)
{
    if (!plugin || !charset)
        return;

    if (plugin->charset)
        free (plugin->charset);
    
    plugin->charset = (charset) ? strdup (charset) : NULL;
}

/*
 * plugin_api_iconv_to_internal: encode string from a charset to WeeChat
 *                               internal charset
 */

char *
plugin_api_iconv_to_internal (struct t_weechat_plugin *plugin,
                              char *charset, char *string)
{
    /* make C compiler happy */
    (void) plugin;
    
    if (!string)
        return NULL;
    
    return string_iconv_to_internal (charset, string);
}

/*
 * plugin_api_iconv_from_internal: encode string from WeeChat internal
 *                                 charset to another
 */

char *
plugin_api_iconv_from_internal (struct t_weechat_plugin *plugin,
                                char *charset, char *string)
{
    /* make C compiler happy */
    (void) plugin;
    
    if (!string)
        return NULL;
    
    return string_iconv_from_internal (charset, string);
}

/*
 * plugin_api_gettext: translate a string using gettext
 */

char *
plugin_api_gettext (struct t_weechat_plugin *plugin, char *string)
{
    /* make C compiler happy */
    (void) plugin;
    
    return _(string);
}

/*
 * plugin_api_ngettext: translate a string using gettext
 */

char *
plugin_api_ngettext (struct t_weechat_plugin *plugin, char *single,
                     char *plural, int count)
{
    /* make C compiler happy */
    (void) plugin;
    
    return NG_(single, plural, count);
}

/*
 * plugin_api_strcasecmp: locale and case independent string comparison
 */

int
plugin_api_strcasecmp (struct t_weechat_plugin *plugin,
                       char *string1, char *string2)
{
    /* make C compiler happy */
    (void) plugin;
    
    return string_strcasecmp (string1, string2);
}

/*
 * plugin_api_strncasecmp: locale and case independent string comparison
 *                             with max length
 */

int
plugin_api_strncasecmp (struct t_weechat_plugin *plugin,
                        char *string1, char *string2, int max)
{
    /* make C compiler happy */
    (void) plugin;
    
    return string_strncasecmp (string1, string2, max);
}

/*
 * plugin_api_strcasestr: locale and case independent string search
 */

char *
plugin_api_strcasestr (struct t_weechat_plugin *plugin,
                       char *string1, char *string2)
{
    /* make C compiler happy */
    (void) plugin;
    
    return string_strcasestr (string1, string2);
}

/*
 * plugin_api_string_replace: replace a string by new one in a string
 */

char *
plugin_api_string_replace (struct t_weechat_plugin *plugin,
                           char *string, char *search, char *replace)
{
    /* make C compiler happy */
    (void) plugin;
    
    return string_replace (string, search, replace);
}

/*
 * plugin_api_string_explode: explode a string
 */

char **
plugin_api_string_explode (struct t_weechat_plugin *plugin, char *string,
                           char *separators, int keep_eol,
                           int num_items_max, int *num_items)
{
    /* make C compiler happy */
    (void) plugin;
    
    if (!string || !separators || !num_items)
        return NULL;
    
    return string_explode (string, separators, keep_eol,
                           num_items_max, num_items);
}

/*
 * plugin_api_string_free_exploded: free exploded string
 */

void
plugin_api_string_free_exploded (struct t_weechat_plugin *plugin,
                                 char **exploded_string)
{
    /* make C compiler happy */
    (void) plugin;
    
    string_free_exploded (exploded_string);
}

/*
 * plugin_api_string_split_command: split a ocmmanc
 */

char **
plugin_api_string_split_command (struct t_weechat_plugin *plugin, char *string,
                                 char separator)
{
    if (!plugin || !string)
        return NULL;
    
    return string_split_command (string, separator);
}

/*
 * plugin_api_string_free_splitted_command: free splitted command
 */

void
plugin_api_string_free_splitted_command (struct t_weechat_plugin *plugin,
                                         char **splitted_command)
{
    /* make C compiler happy */
    (void) plugin;
    
    string_free_splitted_command (splitted_command);
}

/*
 * plugin_api_utf8_has_8bits: return 1 if string has 8-bits chars, 0 if only
 *                            7-bits chars
 */

int
plugin_api_utf8_has_8bits (struct t_weechat_plugin *plugin, char *string)
{
    /* make C compiler happy */
    (void) plugin;
    
    return utf8_has_8bits (string);
}

/*
 * plugin_api_utf8_is_valid: return 1 if UTF-8 string is valid, 0 otherwise
 *                           if error is not NULL, it's set with first non
 *                           valid UTF-8 char in string, if any
 */

int
plugin_api_utf8_is_valid (struct t_weechat_plugin *plugin, char *string,
                          char **error)
{
    /* make C compiler happy */
    (void) plugin;
    
    return utf8_is_valid (string, error);
}

/*
 * plugin_api_utf8_normalize: normalize UTF-8 string: remove non UTF-8 chars
 *                            and replace them by a char
 */

void
plugin_api_utf8_normalize (struct t_weechat_plugin *plugin, char *string,
                           char replacement)
{
    /* make C compiler happy */
    (void) plugin;
    
    utf8_normalize (string, replacement);
}

/*
 * plugin_api_utf8_prev_char: return previous UTF-8 char in a string
 */

char *
plugin_api_utf8_prev_char (struct t_weechat_plugin *plugin, char *string_start,
                           char *string)
{
    /* make C compiler happy */
    (void) plugin;
    
    return utf8_prev_char (string_start, string);
}

/*
 * plugin_api_utf8_next_char: return next UTF-8 char in a string
 */

char *
plugin_api_utf8_next_char (struct t_weechat_plugin *plugin, char *string)
{
    /* make C compiler happy */
    (void) plugin;
    
    return utf8_next_char (string);
}

/*
 * plugin_api_utf8_char_size: return UTF-8 char size (in bytes)
 */

int
plugin_api_utf8_char_size (struct t_weechat_plugin *plugin, char *string)
{
    /* make C compiler happy */
    (void) plugin;
    
    return utf8_char_size (string);
}

/*
 * plugin_api_utf8_strlen: return length of an UTF-8 string (<= strlen(string))
 */

int
plugin_api_utf8_strlen (struct t_weechat_plugin *plugin, char *string)
{
    /* make C compiler happy */
    (void) plugin;
    
    return utf8_strlen (string);
}

/*
 * plugin_api_utf8_strnlen: return length of an UTF-8 string, for N bytes max
 *                          in string
 */

int
plugin_api_utf8_strnlen (struct t_weechat_plugin *plugin, char *string,
                         int bytes)
{
    /* make C compiler happy */
    (void) plugin;
    
    return utf8_strnlen (string, bytes);
}

/*
 * plugin_api_utf8_strlen_screen: return number of chars needed on screen to
 *                                display UTF-8 string
 */

int
plugin_api_utf8_strlen_screen (struct t_weechat_plugin *plugin, char *string)
{
    /* make C compiler happy */
    (void) plugin;
    
    return utf8_strlen_screen (string);
}

/*
 * plugin_api_utf8_charcasecmp: compare two utf8 chars (case is ignored)
 */

int
plugin_api_utf8_charcasecmp (struct t_weechat_plugin *plugin, char *string1,
                             char *string2)
{
    /* make C compiler happy */
    (void) plugin;
    
    return utf8_charcasecmp (string1, string2);
}

/*
 * plugin_api_utf8_char_size_screen: return number of chars needed on screen
 *                                   to display UTF-8 char
 */

int
plugin_api_utf8_char_size_screen (struct t_weechat_plugin *plugin, char *string)
{
    /* make C compiler happy */
    (void) plugin;
    
    return utf8_char_size_screen (string);
}

/*
 * plugin_api_utf8_add_offset: moves forward N chars in an UTF-8 string
 */

char *
plugin_api_utf8_add_offset (struct t_weechat_plugin *plugin, char *string,
                            int offset)
{
    /* make C compiler happy */
    (void) plugin;
    
    return utf8_add_offset (string, offset);
}

/*
 * plugin_api_utf8_real_pos: get real position in UTF-8
 *                           for example: ("aébc", 2) returns 3
 */

int
plugin_api_utf8_real_pos (struct t_weechat_plugin *plugin, char *string,
                          int pos)
{
    /* make C compiler happy */
    (void) plugin;
    
    return utf8_real_pos (string, pos);
}

/*
 * plugin_api_utf8_pos: get position in UTF-8
 *                      for example: ("aébc", 3) returns 2
 */

int
plugin_api_utf8_pos (struct t_weechat_plugin *plugin, char *string,
                     int real_pos)
{
    /* make C compiler happy */
    (void) plugin;
    
    return utf8_real_pos (string, real_pos);
}

/*
 * plugin_api_mkdir_home: create a directory in WeeChat home
 */

int
plugin_api_mkdir_home (struct t_weechat_plugin *plugin, char *directory,
                       int mode)
{
    char *dir_name;
    int dir_length;
    
    /* make C compiler happy */
    (void) plugin;
    
    if (!directory)
        return 0;
    
    /* build directory, adding WeeChat home */
    dir_length = strlen (weechat_home) + strlen (directory) + 2;
    dir_name =
        (char *) malloc (dir_length * sizeof (char));
    if (!dir_name)
        return 0;
    
    snprintf (dir_name, dir_length, "%s/%s", weechat_home, directory);
    
    if (mkdir (dir_name, mode) < 0)
    {
        if (errno != EEXIST)
        {
            free (dir_name);
            return 0;
        }
    }
    
    free (dir_name);
    return 1;
}

/*
 * plugin_api_mkdir: create a directory
 */

int
plugin_api_mkdir (struct t_weechat_plugin *plugin, char *directory,
                  int mode)
{
    /* make C compiler happy */
    (void) plugin;
    
    if (!directory)
        return 0;
    
    if (mkdir (directory, mode) < 0)
    {
        if (errno != EEXIST)
            return 0;
    }
    
    return 1;
}

/*
 * plugin_api_exec_on_files: find files in a directory and execute a
 *                           function on each file
 */

void
plugin_api_exec_on_files (struct t_weechat_plugin *plugin, char *directory,
                          int (*callback)(char *))
{
    /* make C compiler happy */
    (void) plugin;
    
    if (directory && callback)
        util_exec_on_files (directory, callback);
}

/*
 * plugin_api_timeval_diff: calculates difference between two times (return in
 *                          milliseconds)
 */

long
plugin_api_timeval_diff (struct t_weechat_plugin *plugin,
                         void *timeval1, void *timeval2)
{
    /* make C compiler happy */
    (void) plugin;

    return util_timeval_diff (timeval1, timeval2);
}

/*
 * plugin_api_list_new: create a new list
 */

struct t_weelist *
plugin_api_list_new (struct t_weechat_plugin *plugin)
{
    /* make C compiler happy */
    (void) plugin;

    return weelist_new ();
}

/*
 * plugin_api_list_add: add a new item in a list
 */

struct t_weelist_item *
plugin_api_list_add (struct t_weechat_plugin *plugin, void *list, char *data,
                     char *where)
{
    int position;
    
    /* make C compiler happy */
    (void) plugin;

    if (list && data && where)
    {
        position = WEELIST_POS_SORT;
        if (string_strcasecmp (where, "sort") == 0)
            position = WEELIST_POS_SORT;
        else if (string_strcasecmp (where, "beginning") == 0)
            position = WEELIST_POS_BEGINNING;
        else if (string_strcasecmp (where, "end") == 0)
            position = WEELIST_POS_END;
        
        return weelist_add (list, data, position);
    }
    return NULL;
}

/*
 * plugin_api_list_search: search an item in a list (case sensitive)
 */

struct t_weelist_item *
plugin_api_list_search (struct t_weechat_plugin *plugin, void *list,
                        char *data)
{
    /* make C compiler happy */
    (void) plugin;
    
    if (list && data)
        return weelist_search (list, data);
    else
        return NULL;
}

/*
 * plugin_api_list_casesearch: search an item in a list (case unsensitive)
 */

struct t_weelist_item *
plugin_api_list_casesearch (struct t_weechat_plugin *plugin, void *list,
                            char *data)
{
    /* make C compiler happy */
    (void) plugin;
    
    if (list && data)
        return weelist_casesearch (list, data);
    else
        return NULL;
}

/*
 * plugin_api_list_get: get an item with position in list
 */

struct t_weelist_item *
plugin_api_list_get (struct t_weechat_plugin *plugin, void *list, int position)
{
    /* make C compiler happy */
    (void) plugin;

    if (list)
        return weelist_get (list, position);
    else
        return NULL;
}

/*
 * plugin_api_list_next: get next item
 */

struct t_weelist_item *
plugin_api_list_next (struct t_weechat_plugin *plugin, void *item)
{
    /* make C compiler happy */
    (void) plugin;

    if (item)
        return ((struct t_weelist_item *)item)->next_item;
    else
        return NULL;
}

/*
 * plugin_api_list_prev: get previous item
 */

struct t_weelist_item *
plugin_api_list_prev (struct t_weechat_plugin *plugin, void *item)
{
    /* make C compiler happy */
    (void) plugin;

    if (item)
        return ((struct t_weelist_item *)item)->prev_item;
    else
        return NULL;
}

/*
 * plugin_api_list_string: get string value of an item
 */

char *
plugin_api_list_string (struct t_weechat_plugin *plugin, void *item)
{
    /* make C compiler happy */
    (void) plugin;

    if (item)
        return (char *)(((struct t_weelist_item *)item)->data);
    else
        return NULL;
}

/*
 * plugin_api_list_size: get size of a list (number of items)
 */

int
plugin_api_list_size (struct t_weechat_plugin *plugin, void *list)
{
    /* make C compiler happy */
    (void) plugin;

    if (list)
        return ((struct t_weelist *)list)->size;
    else
        return 0;
}

/*
 * plugin_api_list_remove: remove an item from a list
 */

void
plugin_api_list_remove (struct t_weechat_plugin *plugin, void *list,
                        void *item)
{
    /* make C compiler happy */
    (void) plugin;

    if (list && item)
        weelist_remove (list, item);
}

/*
 * plugin_api_list_remove_all: remove all item from a list
 */

void
plugin_api_list_remove_all (struct t_weechat_plugin *plugin, void *list)
{
    /* make C compiler happy */
    (void) plugin;
    
    if (list)
        weelist_remove_all (list);
}

/*
 * plugin_api_list_free: get size of a list (number of items)
 */

void
plugin_api_list_free (struct t_weechat_plugin *plugin, void *list)
{
    /* make C compiler happy */
    (void) plugin;

    if (list)
        weelist_free (list);
}

/*
 * plugin_api_config_new: create new config file structure
 */

struct t_config_file *
plugin_api_config_new (struct t_weechat_plugin *plugin, char *filename)
{
    return config_file_new (plugin, filename);
}

/*
 * plugin_api_config_new_section: create new section in a config
 */

struct t_config_section *
plugin_api_config_new_section (struct t_weechat_plugin *plugin,
                               void *config_file, char *name,
                               void (*callback_read)(void *, char *, char *),
                               void (*callback_write)(void *, char *),
                               void (*callback_write_default)(void *, char *))
{
    if (plugin && config_file_valid_for_plugin (plugin, config_file))
        return config_file_new_section (config_file, name, callback_read,
                                        callback_write, callback_write_default);
    else
        return NULL;
}

/*
 * plugin_api_config_search_section: search a section in a config
 */

struct t_config_section *
plugin_api_config_search_section (struct t_weechat_plugin *plugin,
                                  void *config_file, char *name)
{
    if (plugin && config_file_valid_for_plugin (plugin, config_file))
        return config_file_search_section (config_file, name);
    else
        return NULL;
}

/*
 * plugin_api_config_new_option: create new option in a config section
 */

struct t_config_option *
plugin_api_config_new_option (struct t_weechat_plugin *plugin,
                              void *section, char *name, char *type,
                              char *description, char *string_values,
                              int min, int max, char *default_value,
                              void (*callback_change)())
                              
{
    if (plugin && config_file_section_valid_for_plugin (plugin, section))
        return config_file_new_option (section, name, type, description,
                                       string_values, min, max, default_value,
                                       callback_change);
    else
        return NULL;
}

/*
 * plugin_api_config_search_option: search an option in a config or section
 */

struct t_config_option *
plugin_api_config_search_option (struct t_weechat_plugin *plugin,
                                 void *config_file, void *section, char *name)
{
    if (plugin
        && (!config_file || config_file_valid_for_plugin (plugin, config_file))
        && (!section || config_file_section_valid_for_plugin (plugin, section)))
        return config_file_search_option (config_file, section, name);
    else
        return NULL;
}

/*
 * plugin_api_config_option_set: set new value for an option
 *                               return: 2 if ok (value changed)
 *                                       1 if ok (value is the same)
 *                                       0 if failed
 */

int
plugin_api_config_option_set (struct t_weechat_plugin *plugin,
                              void *option, char *new_value)
{
    int rc;
    
    if (plugin && config_file_option_valid_for_plugin (plugin, option))
    {
        rc = config_file_option_set (option, new_value);
        if ((rc == 2) && (((struct t_config_option *)option)->callback_change))
            (void) (((struct t_config_option *)option)->callback_change) ();
        if (rc == 0)
            return 0;
        return 1;
    }
    return 0;
}

/*
 * plugin_api_config_string_to_boolean: return boolean value of a string
 */

char
plugin_api_config_string_to_boolean (struct t_weechat_plugin *plugin,
                                     char *string)
{
    /* make C compiler happy */
    (void) plugin;
    
    if (config_file_string_to_boolean (string) == CONFIG_OPTION_BOOLEAN)
        return CONFIG_BOOLEAN_TRUE;
    else
        return CONFIG_BOOLEAN_FALSE;
}

/*
 * plugin_api_config_boolean: return boolean value of an option
 */

char
plugin_api_config_boolean (struct t_weechat_plugin *plugin, void *option)
{
    if (plugin && config_file_option_valid_for_plugin (plugin, option)
        && (((struct t_config_option *)option)->type == CONFIG_OPTION_BOOLEAN))
        return CONFIG_BOOLEAN((struct t_config_option *)option);
    else
        return 0;
}

/*
 * plugin_api_config_integer: return integer value of an option
 */

int
plugin_api_config_integer (struct t_weechat_plugin *plugin, void *option)
{
    if (plugin && config_file_option_valid_for_plugin (plugin, option))
    {
        switch (((struct t_config_option *)option)->type)
        {
            case CONFIG_OPTION_BOOLEAN:
                if (CONFIG_BOOLEAN((struct t_config_option *)option) == CONFIG_BOOLEAN_TRUE)
                    return 1;
                else
                    return 0;
            case CONFIG_OPTION_INTEGER:
            case CONFIG_OPTION_COLOR:
                return CONFIG_INTEGER((struct t_config_option *)option);
            case CONFIG_OPTION_STRING:
                return 0;
        }
    }
    return 0;
}

/*
 * plugin_api_config_string: return string value of an option
 */

char *
plugin_api_config_string (struct t_weechat_plugin *plugin, void *option)
{
    if (plugin && config_file_option_valid_for_plugin (plugin, option))
    {
        if (((struct t_config_option *)option)->type == CONFIG_OPTION_STRING)
            return CONFIG_STRING((struct t_config_option *)option);
        if ((((struct t_config_option *)option)->type == CONFIG_OPTION_INTEGER)
            && (((struct t_config_option *)option)->string_values))
            return ((struct t_config_option *)option)->
                string_values[CONFIG_INTEGER(((struct t_config_option *)option))];
    }
    return NULL;
}

/*
 * plugin_api_config_color: return color value of an option
 */

int
plugin_api_config_color (struct t_weechat_plugin *plugin, void *option)
{
    if (plugin && config_file_option_valid_for_plugin (plugin, option)
        && (((struct t_config_option *)option)->type == CONFIG_OPTION_COLOR))
        return CONFIG_COLOR((struct t_config_option *)option);
    else
        return 0;
}

/*
 * plugin_api_config_read: read a configuration file
 */

int
plugin_api_config_read (struct t_weechat_plugin *plugin, void *config_file)
{
    if (plugin && config_file_valid_for_plugin (plugin, config_file))
        return config_file_read ((struct t_config_file *)config_file);
    else
        return -1;
}

/*
 * plugin_api_config_reload: reload a configuration file
 */

int
plugin_api_config_reload (struct t_weechat_plugin *plugin, void *config_file)
{
    if (plugin && config_file_valid_for_plugin (plugin, config_file))
        return config_file_reload ((struct t_config_file *)config_file);
    else
        return -1;
}

/*
 * plugin_api_config_write: write a configuration file
 */

int
plugin_api_config_write (struct t_weechat_plugin *plugin, void *config_file)
{
    if (plugin && config_file_valid_for_plugin (plugin, config_file))
        return config_file_write ((struct t_config_file *)config_file, 0);
    else
        return -1;
}

/*
 * plugin_api_config_write_line: write a line in configuration file
 */

void
plugin_api_config_write_line (struct t_weechat_plugin *plugin,
                              void *config_file, char *option_name,
                              char *value, ...)
{
    char buf[4096];
    va_list argptr;
    
    if (plugin && config_file_valid_for_plugin (plugin, config_file))
    {
        va_start (argptr, value);
        vsnprintf (buf, sizeof (buf) - 1, value, argptr);
        va_end (argptr);
        config_file_write_line ((struct t_config_file *)config_file,
                                option_name, buf);
    }
}

/*
 * plugin_api_config_free: free a configuration file
 */

void
plugin_api_config_free (struct t_weechat_plugin *plugin, void *config_file)
{
    if (plugin && config_file_valid_for_plugin (plugin, config_file))
        config_file_free ((struct t_config_file *)config_file);
}

/*
 * plugin_api_get_config_str_value: return string value for any option
 *                                  This function should never be called directly
 *                                  (only used by weechat_get_config)
 */

char *
plugin_api_get_config_str_value (struct t_config_option *option)
{
    char buf_temp[1024], *color_name;
    
    switch (option->type)
    {
        case CONFIG_OPTION_BOOLEAN:
            return (CONFIG_BOOLEAN(option) == CONFIG_BOOLEAN_TRUE) ?
                strdup ("on") : strdup ("off");
        case CONFIG_OPTION_INTEGER:
            if (option->string_values)
                snprintf (buf_temp, sizeof (buf_temp), "%s",
                          option->string_values[CONFIG_INTEGER(option)]);
            else
                snprintf (buf_temp, sizeof (buf_temp), "%d",
                          CONFIG_INTEGER(option));
            return strdup (buf_temp);
        case CONFIG_OPTION_STRING:
            return strdup (CONFIG_STRING(option));
        case CONFIG_OPTION_COLOR:
            color_name = gui_color_get_name (CONFIG_INTEGER(option));
            return (color_name) ? strdup (color_name) : strdup ("");
    }
    
    /* should never be executed! */
    return NULL;
}

/*
 * plugin_api_config_get: get value of a WeeChat config option
 */

struct t_config_option *
plugin_api_config_get (struct t_weechat_plugin *plugin, char *option_name)
{
    /* make C compiler happy */
    (void) plugin;
    
    return config_file_search_option (weechat_config_file, NULL,
                                      option_name);
}

/*
 * plugin_api_config_set: set value of a config option
 */

int
plugin_api_config_set (struct t_weechat_plugin *plugin, char *option_name,
                       char *value)
{
    struct t_config_option *ptr_option;
    int rc;
    
    /* make C compiler happy */
    (void) plugin;
    
    if (!option_name || !value)
        return 0;
    
    /* search and set WeeChat config option if found */
    ptr_option = config_file_search_option (weechat_config_file, NULL,
                                            option_name);
    if (ptr_option)
    {
        rc = config_file_option_set (ptr_option, value);
        if ((rc == 2) && (ptr_option->callback_change))
            (void) (ptr_option->callback_change) ();
        if (rc == 0)
            return 0;
        return 1;
    }
    
    /* failed to set config option */
    return 0;
}

/*
 * plugin_api_plugin_config_get: get value of a plugin config option
 */

char *
plugin_api_plugin_config_get (struct t_weechat_plugin *plugin, char *option_name)
{
    struct t_config_option *ptr_option;
    
    if (!option_name)
        return NULL;
    
    ptr_option = plugin_config_search (plugin->name, option_name);
    if (ptr_option)
        return (ptr_option->value) ? strdup (ptr_option->value) : NULL;
    
    /* option not found */
    return NULL;
}

/*
 * plugin_api_plugin_config_set: set value of a plugin config option
 */

int
plugin_api_plugin_config_set (struct t_weechat_plugin *plugin,
                              char *option_name, char *value)
{
    if (!option_name)
        return 0;
    
    if (plugin_config_set (plugin->name, option_name, value))
        return 1;
    
    return 0;
}

/*
 * plugin_api_prefix: return a prefix for display with printf
 */

char *
plugin_api_prefix (struct t_weechat_plugin *plugin, char *prefix)
{
    if (!plugin || !prefix)
        return gui_chat_prefix_empty;
    
    if (string_strcasecmp (prefix, "info") == 0)
        return gui_chat_prefix[GUI_CHAT_PREFIX_INFO];
    if (string_strcasecmp (prefix, "error") == 0)
        return gui_chat_prefix[GUI_CHAT_PREFIX_ERROR];
    if (string_strcasecmp (prefix, "network") == 0)
        return gui_chat_prefix[GUI_CHAT_PREFIX_NETWORK];
    if (string_strcasecmp (prefix, "action") == 0)
        return gui_chat_prefix[GUI_CHAT_PREFIX_ACTION];
    if (string_strcasecmp (prefix, "join") == 0)
        return gui_chat_prefix[GUI_CHAT_PREFIX_JOIN];
    if (string_strcasecmp (prefix, "quit") == 0)
        return gui_chat_prefix[GUI_CHAT_PREFIX_QUIT];
    
    return gui_chat_prefix_empty;
}

/*
 * plugin_api_color: return a WeeChat color for display with printf
 */

char *
plugin_api_color (struct t_weechat_plugin *plugin, char *color_name)
{
    int num_color;
    
    if (!plugin || !color_name)
        return GUI_NO_COLOR;

    num_color = gui_color_search_config (color_name);
    if (num_color >= 0)
        return GUI_COLOR(num_color);
    
    return GUI_NO_COLOR;
}

/*
 * plugin_api_printf: print a message on a buffer
 */

void
plugin_api_printf (struct t_weechat_plugin *plugin,
                   void *buffer, char *format, ...)
{
    va_list argptr;
    char buf[8192];
    
    if (!plugin || !format
        || !gui_buffer_valid ((struct t_gui_buffer *)buffer))
        return;
    
    va_start (argptr, format);
    vsnprintf (buf, sizeof (buf) - 1, format, argptr);
    va_end (argptr);
    
    gui_chat_printf ((struct t_gui_buffer *)buffer, "%s", buf);
}

/*
 * plugin_api_printf_date: print a message on a buffer with a specific date
 */

void
plugin_api_printf_date (struct t_weechat_plugin *plugin,
                        void *buffer, time_t date, char *format, ...)
{
    va_list argptr;
    char buf[8192];
    
    if (!plugin || !format
        || !gui_buffer_valid ((struct t_gui_buffer *)buffer))
        return;
    
    va_start (argptr, format);
    vsnprintf (buf, sizeof (buf) - 1, format, argptr);
    va_end (argptr);
    
    gui_chat_printf_date ((struct t_gui_buffer *)buffer, date, buf);
}

/*
 * plugin_api_log_printf: print a message in WeeChat log file
 */

void
plugin_api_log_printf (struct t_weechat_plugin *plugin, char *format, ...)
{
    va_list argptr;
    char buf[8192];
    
    if (!plugin || !format)
        return;
    
    va_start (argptr, format);
    vsnprintf (buf, sizeof (buf) - 1, format, argptr);
    va_end (argptr);
    
    log_printf ("%s", buf);
}

/*
 * plugin_api_infobar_printf: print a message in infobar
 */

void
plugin_api_infobar_printf (struct t_weechat_plugin *plugin, int time_displayed,
                           char *color_name, char *format, ...)
{
    va_list argptr;
    static char buf[1024];
    char *buf2;
    int num_color;
    
    if (!plugin || (time_displayed < 0) || !format)
        return;
    
    va_start (argptr, format);
    vsnprintf (buf, sizeof (buf) - 1, format, argptr);
    va_end (argptr);
    
    buf2 = string_iconv_to_internal (plugin->charset, buf);
    if (color_name && color_name[0])
    {
        num_color = gui_color_search_config (color_name);
        if (num_color < 0)
            num_color = GUI_COLOR_INFOBAR;
    }
    else
        num_color = GUI_COLOR_INFOBAR;
    
    gui_infobar_printf (time_displayed,
                        num_color,
                        "%s",
                        (buf2) ? buf2 : buf);
    if (buf2)
        free (buf2);
}

/*
 * plugin_api_infobar_remove: remove message(s) in infobar
 */

void
plugin_api_infobar_remove (struct t_weechat_plugin *plugin, int how_many)
{
    if (!plugin)
        return;
    
    if (how_many <= 0)
        gui_infobar_remove_all ();
    else
    {
        while ((gui_infobar) && (how_many > 0))
        {
            gui_infobar_remove ();
            how_many--;
        }
    }
    //gui_infobar_draw (gui_current_window->buffer, 1);
}

/*
 * plugin_api_hook_command: hook a command
 */

struct t_hook *
plugin_api_hook_command (struct t_weechat_plugin *plugin, char *command,
                         char *description, char *args,
                         char *args_desc, char *completion,
                         int (*callback)(void *, void *, int, char **, char **),
                         void *data)
{
    if (plugin && callback)
        return hook_command (plugin, command, description, args,
                             args_desc, completion,
                             callback, data);
    
    return NULL;
}

/*
 * plugin_api_hook_timer: hook a timer
 */

struct t_hook *
plugin_api_hook_timer (struct t_weechat_plugin *plugin, long interval,
                       int align_second, int max_calls,
                       int (*callback)(void *), void *data)
{
    if (plugin && (interval > 0) && callback)
        return hook_timer (plugin, interval, align_second, max_calls,
                           callback, data);
    
    return NULL;
}

/*
 * plugin_api_hook_fd: hook a file descriptor
 */

struct t_hook *
plugin_api_hook_fd (struct t_weechat_plugin *plugin, int fd,
                    int flag_read, int flag_write, int flag_exception,
                    int (*callback)(void *), void *data)
{
    int flags;
    
    if (plugin && (fd >= 0) && callback)
    {
        flags = 0;
        if (flag_read)
            flags |= HOOK_FD_FLAG_READ;
        if (flag_write)
            flags |= HOOK_FD_FLAG_WRITE;
        if (flag_exception)
            flags |= HOOK_FD_FLAG_EXCEPTION;
        return hook_fd (plugin, fd, flags, callback, data);
    }
    
    return NULL;
}

/*
 * plugin_api_hook_print: hook a printed message
 */

struct t_hook *
plugin_api_hook_print (struct t_weechat_plugin *plugin, void *buffer,
                       char *message, int strip_colors,
                       int (*callback)(void *, void *, time_t, char *, char *),
                       void *data)
{
    if (plugin && gui_buffer_valid ((struct t_gui_buffer *)buffer)
        && callback)
        return hook_print (plugin, buffer, message, strip_colors,
                           callback, data);
    
    return NULL;
}

/*
 * plugin_api_hook_signal: hook a signal
 */

struct t_hook *
plugin_api_hook_signal (struct t_weechat_plugin *plugin, char *signal,
                        int (*callback)(void *, char *, void *),
                        void *data)
{
    if (plugin && signal && signal[0] && callback)
        return hook_signal (plugin, signal, callback, data);
    
    return NULL;
}

/*
 * plugin_api_hook_signal_send: send a signal
 */

void
plugin_api_hook_signal_send (struct t_weechat_plugin *plugin, char *signal,
                             void *pointer)
{
    if (plugin && signal && signal[0])
        hook_signal_exec (signal, pointer);
}

/*
 * plugin_api_hook_config: hook a config option
 */

struct t_hook *
plugin_api_hook_config (struct t_weechat_plugin *plugin, char *config_type,
                        char *config_option,
                        int (*callback)(void *, char *, char *, char *),
                        void *data)
{
    if (plugin && callback)
        return hook_config (plugin, config_type, config_option,
                            callback, data);
    
    return NULL;
}

/*
 * plugin_api_hook_completion: hook a completion
 */

struct t_hook *
plugin_api_hook_completion (struct t_weechat_plugin *plugin, char *completion,
                            int (*callback)(void *, char *, void *, void *),
                            void *data)
{
    if (plugin && callback)
        return hook_completion (plugin, completion, callback, data);
    
    return NULL;
}

/*
 * plugin_api_unhook: unhook something
 */

void
plugin_api_unhook (struct t_weechat_plugin *plugin, void *hook)
{
    if (plugin && hook
        && (hook_valid_for_plugin (plugin, (struct t_hook *)hook)))
        unhook ((struct t_hook *)hook);
}

/*
 * plugin_api_unhook_all: unhook all for a plugin
 */

void
plugin_api_unhook_all (struct t_weechat_plugin *plugin)
{
    if (plugin)
        unhook_all (plugin);
}

/*
 * plugin_api_buffer_new: create a new buffer
 */

struct t_gui_buffer *
plugin_api_buffer_new (struct t_weechat_plugin *plugin, char *category,
                       char *name,
                       void (*input_data_cb)(struct t_gui_buffer *, char *))
{
    if (plugin && name && name[0])
        return gui_buffer_new (plugin, category, name, input_data_cb);
    
    return NULL;
}

/*
 * plugin_api_buffer_search: search a buffer
 */

struct t_gui_buffer *
plugin_api_buffer_search (struct t_weechat_plugin *plugin, char *category,
                          char *name)
{
    struct t_gui_buffer *ptr_buffer;
    
    if (plugin)
    {
        ptr_buffer = gui_buffer_search_by_category_name (category, name);
        if (ptr_buffer)
            return ptr_buffer;
        return gui_current_window->buffer;
    }
    
    return NULL;
}

/*
 * plugin_api_buffer_close: close a buffer
 */

void
plugin_api_buffer_close (struct t_weechat_plugin *plugin, void *buffer)
{
    if (plugin && buffer
        && gui_buffer_valid ((struct t_gui_buffer *)buffer))
        gui_buffer_free ((struct t_gui_buffer *)buffer, 1);
}

/*
 * plugin_api_buffer_get: get a buffer property
 */

void *
plugin_api_buffer_get (struct t_weechat_plugin *plugin, void *buffer,
                       char *property)
{
    if (plugin && buffer && property && property[0])
        return gui_buffer_get ((struct t_gui_buffer *)buffer, property);
    
    return NULL;
}

/*
 * plugin_api_buffer_set: set a buffer property
 */

void
plugin_api_buffer_set (struct t_weechat_plugin *plugin, void *buffer,
                       char *property, char *value)
{
    if (plugin && buffer && property && property[0])
        gui_buffer_set ((struct t_gui_buffer *)buffer, property, value);
}

/*
 * plugin_api_buffer_nick_add: add a nick to a buffer nicklist
 */

void
plugin_api_buffer_nick_add (struct t_weechat_plugin *plugin, void *buffer,
                            char *nick, int sort_index, char *color_nick,
                            char prefix, char *color_prefix)
{
    int num_color_nick, num_color_prefix;
    struct t_gui_nick *ptr_nick;

    if (plugin && buffer && gui_buffer_valid ((struct t_gui_buffer *)buffer)
        && nick && nick[0])
    {
        num_color_nick = gui_color_search_config (color_nick);
        if (num_color_nick < 0)
            num_color_nick = GUI_COLOR_NICKLIST;
        
        num_color_prefix = gui_color_search_config (color_prefix);
        if (num_color_prefix < 0)
            num_color_prefix = GUI_COLOR_NICKLIST;
        
        ptr_nick = gui_nicklist_search ((struct t_gui_buffer *)buffer, nick);
        if (ptr_nick)
            gui_nicklist_update ((struct t_gui_buffer *)buffer,
                                 ptr_nick, nick, sort_index, num_color_nick,
                                 prefix, num_color_prefix);
        else
            gui_nicklist_add ((struct t_gui_buffer *)buffer,
                              nick, sort_index, num_color_nick,
                              prefix, num_color_prefix);
    }
}

/*
 * plugin_api_buffer_nick_remove: remove a nick from a buffer nicklist
 */

void
plugin_api_buffer_nick_remove (struct t_weechat_plugin *plugin, void *buffer,
                               char *nick)
{
    if (plugin && buffer && gui_buffer_valid ((struct t_gui_buffer *)buffer)
        && nick && nick[0])
    {
        if (gui_nicklist_remove ((struct t_gui_buffer *)buffer, nick))
            gui_nicklist_draw ((struct t_gui_buffer *)buffer, 0);
        //gui_nicklist_remove ((struct t_gui_buffer *)buffer, nick);
    }
}

/*
 * plugin_api_command: execute a command (simulate user entry)
 */

void
plugin_api_command (struct t_weechat_plugin *plugin, void *buffer,
                    char *command)
{
    char *command2;
    
    if (!plugin || !command)
        return;
    
    command2 = string_iconv_to_internal (plugin->charset, command);
    if (!buffer)
        buffer = gui_current_window->buffer;
    input_data (buffer, (command2) ? command2 : command, 0);
    if (command2)
        free (command2);
}

/*
 * plugin_api_info_get: get info about WeeChat
 *                      WARNING: caller has to free string returned
 *                      by this function after use
 */

char *
plugin_api_info_get (struct t_weechat_plugin *plugin, char *info)
{
    //t_irc_server *ptr_server;
    //t_irc_channel *ptr_channel;
    time_t inactivity;
    char *return_str;
    
    if (!plugin || !info)
        return NULL;
    
    /* below are infos that do NOT need server to return info */
    
    if (string_strcasecmp (info, "version") == 0)
    {
        return strdup (PACKAGE_VERSION);
    }
    else if (string_strcasecmp (info, "dir_separator") == 0)
    {
        return strdup (DIR_SEPARATOR);
    }
    else if (string_strcasecmp (info, "weechat_dir") == 0)
    {
        return strdup (weechat_home);
    }
    else if (string_strcasecmp (info, "weechat_libdir") == 0)
    {
        return strdup (WEECHAT_LIBDIR);
    }
    else if (string_strcasecmp (info, "weechat_sharedir") == 0)
    {
        return strdup (WEECHAT_SHAREDIR);
    }
    else if (string_strcasecmp (info, "charset_terminal") == 0)
    {
        return strdup (local_charset);
    }
    else if (string_strcasecmp (info, "charset_internal") == 0)
    {
        return strdup (WEECHAT_INTERNAL_CHARSET);
    }
    else if (string_strcasecmp (info, "inactivity") == 0)
    {
        if (gui_keyboard_last_activity_time == 0)
            inactivity = 0;
        else
            inactivity = time (NULL) - gui_keyboard_last_activity_time;
        return_str = (char *) malloc (32);
        if (!return_str)
            return NULL;
        snprintf (return_str, 32, "%ld", (long int)inactivity);
        return return_str;
    }
    else if (string_strcasecmp (info, "input") == 0)
    {
        if (gui_current_window->buffer->input)
        {
            return_str = string_iconv_from_internal (plugin->charset,
                                                     gui_current_window->buffer->input_buffer);
            return (return_str) ? return_str : strdup ("");
        }
        else
            return strdup ("");
    }
    else if (string_strcasecmp (info, "input_mask") == 0)
    {
        if (gui_current_window->buffer->input)
            return strdup (gui_current_window->buffer->input_buffer_color_mask);
        else
            return strdup ("");
    }
    else if (string_strcasecmp (info, "input_pos") == 0)
    {
        if (gui_current_window->buffer->input)
        {
            return_str = (char *) malloc (32);
            if (!return_str)
                return NULL;
            snprintf (return_str, 32, "%d",
                      gui_current_window->buffer->input_buffer_pos);
            return return_str;
        }
        else
            return strdup ("");
    }
    
    /* below are infos that need server to return value */
    
    /*plugin_find_server_channel (server, NULL, &ptr_server, &ptr_channel);
    
    if (string_strcasecmp (info, "nick") == 0)
    {
        if (ptr_server && ptr_server->is_connected && ptr_server->nick)
            return strdup (ptr_server->nick);
    }
    else if (string_strcasecmp (info, "channel") == 0)
    {
        if (GUI_BUFFER_IS_CHANNEL(gui_current_window->buffer)
            || GUI_BUFFER_IS_PRIVATE(gui_current_window->buffer))
            return strdup (GUI_CHANNEL(gui_current_window->buffer)->name);
    }
    else if (string_strcasecmp (info, "server") == 0)
    {
        if (ptr_server && ptr_server->is_connected && ptr_server->name)
            return strdup (ptr_server->name);
    }
    else if (string_strcasecmp (info, "type") == 0)
    {
        return_str = (char *) malloc (32);
        if (!return_str)
            return NULL;
        snprintf (return_str, 32, "%d",
                  gui_current_window->buffer->type);
        return return_str;
    }
    else if (string_strcasecmp (info, "away") == 0)
    {
        if (ptr_server && ptr_server->is_connected && ptr_server->is_away)
            return strdup ("1");
        else
            return strdup ("0");
            }*/
    
    /* info not found */
    return NULL;
}

/*
 * plugin_api_infolist_get_add_buffer: add a buffer in a list
 *                                     return 1 if ok, 0 if error
 */

int
plugin_api_infolist_get_add_buffer (struct t_plugin_infolist *infolist,
                                    struct t_gui_buffer *buffer)
{
    struct t_plugin_infolist_item *ptr_item;
    
    if (!infolist || !buffer)
        return 0;
    
    ptr_item = plugin_infolist_new_item (infolist);
    if (!ptr_item)
        return 0;

    if (!plugin_infolist_new_var_pointer (ptr_item, "pointer", buffer))
        return 0;
    if (!plugin_infolist_new_var_integer (ptr_item, "number", buffer->number))
        return 0;
    if (!plugin_infolist_new_var_string (ptr_item, "category", buffer->category))
        return 0;
    if (!plugin_infolist_new_var_string (ptr_item, "name", buffer->name))
        return 0;
    if (!plugin_infolist_new_var_integer (ptr_item, "type", buffer->type))
        return 0;
    if (!plugin_infolist_new_var_integer (ptr_item, "notify_level", buffer->notify_level))
        return 0;
    if (!plugin_infolist_new_var_integer (ptr_item, "num_displayed", buffer->num_displayed))
        return 0;
    if (!plugin_infolist_new_var_string (ptr_item, "title", buffer->title))
        return 0;
    if (!plugin_infolist_new_var_integer (ptr_item, "input", buffer->input))
        return 0;
    if (!plugin_infolist_new_var_string (ptr_item, "input_nick", buffer->input_nick))
        return 0;
    if (!plugin_infolist_new_var_string (ptr_item, "input_string", buffer->input_buffer))
        return 0;
    
    return 1;
}

/*
 * plugin_api_infolist_get_add_buffer_line: add a buffer line in a list
 *                                          return 1 if ok, 0 if error
 */

int
plugin_api_infolist_get_add_buffer_line (struct t_plugin_infolist *infolist,
                                         struct t_gui_line *line)
{
    struct t_plugin_infolist_item *ptr_item;
    
    if (!infolist || !line)
        return 0;
    
    ptr_item = plugin_infolist_new_item (infolist);
    if (!ptr_item)
        return 0;
    
    if (!plugin_infolist_new_var_time (ptr_item, "date", line->date))
        return 0;
    if (!plugin_infolist_new_var_time (ptr_item, "date_printed", line->date))
        return 0;
    if (!plugin_infolist_new_var_string (ptr_item, "str_time", line->str_time))
        return 0;
    if (!plugin_infolist_new_var_string (ptr_item, "prefix", line->prefix))
        return 0;
    if (!plugin_infolist_new_var_string (ptr_item, "message", line->message))
        return 0;
    
    return 1;
}

/*
 * plugin_api_infolist_get: get list with infos about WeeChat structures
 *                          WARNING: caller has to free string returned
 *                          by this function after use, with weechat_list_free()
 */

struct t_plugin_infolist *
plugin_api_infolist_get (struct t_weechat_plugin *plugin, char *name,
                         void *pointer)
{
    struct t_plugin_infolist *ptr_infolist;
    struct t_gui_buffer *ptr_buffer;
    struct t_gui_line *ptr_line;
    
    if (!plugin || !name || !name[0])
        return NULL;
    
    if (string_strcasecmp (name, "buffer") == 0)
    {
        /* invalid buffer pointer ? */
        if (pointer && (!gui_buffer_valid (pointer)))
            return NULL;
        
        ptr_infolist = plugin_infolist_new ();
        if (ptr_infolist)
        {
            if (pointer)
            {
                /* build list with only one buffer */
                if (!plugin_api_infolist_get_add_buffer (ptr_infolist, pointer))
                {
                    plugin_infolist_free (ptr_infolist);
                    return NULL;
                }
                return ptr_infolist;
            }
            else
            {
                /* build list with all buffers */
                for (ptr_buffer = gui_buffers; ptr_buffer;
                     ptr_buffer = ptr_buffer->next_buffer)
                {
                    if (!plugin_api_infolist_get_add_buffer (ptr_infolist,
                                                             ptr_buffer))
                    {
                        plugin_infolist_free (ptr_infolist);
                        return NULL;
                    }
                }
                return ptr_infolist;
            }
        }
    }
    else if (string_strcasecmp (name, "buffer_lines") == 0)
    {
        if (!pointer)
            pointer = gui_buffers;
        else
        {
            /* invalid buffer pointer ? */
            if (!gui_buffer_valid (pointer))
                return NULL;
        }
        
        ptr_infolist = plugin_infolist_new ();
        if (ptr_infolist)
        {
            for (ptr_line = ((struct t_gui_buffer *)pointer)->lines; ptr_line;
                 ptr_line = ptr_line->next_line)
            {
                if (!plugin_api_infolist_get_add_buffer_line (ptr_infolist,
                                                              ptr_line))
                {
                    plugin_infolist_free (ptr_infolist);
                    return NULL;
                }
            }
            return ptr_infolist;
        }
    }
    
    /* list not found */
    return NULL;
}

/*
 * plugin_api_infolist_next: move item pointer to next item in a list
 *                           return 1 if pointer is still ok
 *                                  0 if end of list was reached
 */

int
plugin_api_infolist_next (struct t_weechat_plugin *plugin, void *infolist)
{
    if (!plugin || !infolist || !plugin_infolist_valid (infolist))
        return 0;
    
    return (plugin_infolist_next_item (infolist)) ? 1 : 0;
}

/*
 * plugin_api_infolist_prev: move item pointer to previous item in a list
 *                           return 1 if pointer is still ok
 *                                  0 if beginning of list was reached
 */

int
plugin_api_infolist_prev (struct t_weechat_plugin *plugin, void *infolist)
{
    if (!plugin || !infolist || !plugin_infolist_valid (infolist))
        return 0;

    return (plugin_infolist_prev_item (infolist)) ? 1 : 0;
}

/*
 * plugin_api_infolist_fields: get list of fields for current list item
 */

char *
plugin_api_infolist_fields (struct t_weechat_plugin *plugin, void *infolist)
{
    if (!plugin || !infolist || !plugin_infolist_valid (infolist))
        return NULL;
    
    return plugin_infolist_get_fields (infolist);
}

/*
 * plugin_api_infolist_integer: get an integer variable value in current list item
 */

int
plugin_api_infolist_integer (struct t_weechat_plugin *plugin, void *infolist,
                             char *var)
{
    if (!plugin || !infolist || !plugin_infolist_valid (infolist)
        || !((struct t_plugin_infolist *)infolist)->ptr_item)
        return 0;
    
    return plugin_infolist_get_integer (infolist, var);
}

/*
 * plugin_api_infolist_string: get a string variable value in current list item
 */

char *
plugin_api_infolist_string (struct t_weechat_plugin *plugin, void *infolist,
                            char *var)
{
    if (!plugin || !infolist || !plugin_infolist_valid (infolist)
        || !((struct t_plugin_infolist *)infolist)->ptr_item)
        return NULL;
    
    return plugin_infolist_get_string (infolist, var);
}

/*
 * plugin_api_infolist_pointer: get a pointer variable value in current list item
 */

void *
plugin_api_infolist_pointer (struct t_weechat_plugin *plugin, void *infolist,
                             char *var)
{
    if (!plugin || !infolist || !plugin_infolist_valid (infolist)
        || !((struct t_plugin_infolist *)infolist)->ptr_item)
        return NULL;
    
    return plugin_infolist_get_pointer (infolist, var);
}

/*
 * plugin_api_infolist_time: get a time variable value in current list item
 */

time_t
plugin_api_infolist_time (struct t_weechat_plugin *plugin, void *infolist,
                          char *var)
{
    if (!plugin || !infolist || !plugin_infolist_valid (infolist)
        || !((struct t_plugin_infolist *)infolist)->ptr_item)
        return 0;
    
    return plugin_infolist_get_time (infolist, var);
}

/*
 * plugin_api_infolist_free: free an infolist
 */

void
plugin_api_infolist_free (struct t_weechat_plugin *plugin, void *infolist)
{
    if (plugin && infolist && plugin_infolist_valid (infolist))
        plugin_infolist_free (infolist);
}

/*
 * plugin_api_log: add a message in buffer log file
 */

void
plugin_api_log (struct t_weechat_plugin *plugin,
                char *server, char *channel, char *message, ...)
{
    (void) plugin;
    (void) server;
    (void) channel;
    (void) message;
    
    /*t_gui_buffer *ptr_buffer;
    va_list argptr;
    static char buf[8192];
    char *buf2;
    
    if (!plugin || !message)
        return;
    
    ptr_buffer = gui_buffer_search (server, channel);
    if (ptr_buffer)
    {
	va_start (argptr, message);
	vsnprintf (buf, sizeof (buf) - 1, message, argptr);
	va_end (argptr);

        buf2 = string_iconv_to_internal (plugin->charset, buf);
	gui_log_write_line (ptr_buffer, (buf2) ? buf2 : buf);
        if (buf2)
            free (buf2);
    }*/
}
