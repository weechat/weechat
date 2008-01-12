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

/* script.c: script interface for WeeChat plugins */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include "../weechat-plugin.h"
#include "script.h"
#include "script-callback.h"


#define SCRIPT_OPTION_CHECK_LICENSE "check_license"

int script_option_check_license = 0;


/*
 * script_config_read: read config options
 */

void
script_config_read (struct t_weechat_plugin *weechat_plugin)
{
    char *string;
    
    string = weechat_config_get_plugin (SCRIPT_OPTION_CHECK_LICENSE);
    if (!string)
    {
        weechat_config_set_plugin (SCRIPT_OPTION_CHECK_LICENSE, "on");
        string = weechat_config_get_plugin (SCRIPT_OPTION_CHECK_LICENSE);
    }
    if (string && (weechat_config_string_to_boolean (string) > 0))
        script_option_check_license = 1;
    else
        script_option_check_license = 0;
}

/*
 * script_config_cb: callback called when config option is changed
 */

int
script_config_cb (void *data, char *type, char *option, char *value)
{
    (void) type;
    (void) option;
    (void) value;
    
    script_config_read (data);
    
    return WEECHAT_RC_OK;
}

/*
 * script_init: initialize script
 */

void
script_init (struct t_weechat_plugin *weechat_plugin)
{
    char *option;
    int length;
    
    script_config_read (weechat_plugin);
    
    length = strlen (weechat_plugin->name) + 32;
    option= (char *)malloc (length);
    if (option)
    {
        snprintf (option, length - 1, "%s.%s",
                  weechat_plugin->name, SCRIPT_OPTION_CHECK_LICENSE);
        weechat_hook_config ("plugin", option,
                             &script_config_cb, (void *)weechat_plugin);
    }
}

/*
 * script_pointer_to_string: convert pointer to string for usage
 *                           in a script (any language)
 *                           WARNING: result has to be free() after use
 */

char *
script_pointer_to_string (void *pointer)
{
    char pointer_str[128];

    if (!pointer)
        return strdup ("");
    
    snprintf (pointer_str, sizeof (pointer_str) - 1,
              "0x%x", (unsigned int)pointer);
    
    return strdup (pointer_str);
}

/*
 * script_string_to_pointer: convert stirng to pointer for usage
 *                           outside script
 */

void *
script_string_to_pointer (char *pointer_str)
{
    unsigned int value;
    
    if (!pointer_str || (pointer_str[0] != '0') || (pointer_str[1] != 'x'))
        return NULL;
    
    sscanf (pointer_str + 2, "%x", &value);
    
    return (void *)value;
}

/*
 * script_auto_load: auto-load all scripts in a directory
 */

void
script_auto_load (struct t_weechat_plugin *weechat_plugin,
                  char *language,
                  int (*callback)(void *data, char *filename))
{
    char *dir_home, *dir_name;
    int dir_length;
    
    /* build directory, adding WeeChat home */
    dir_home = weechat_info_get ("weechat_dir");
    if (!dir_home)
        return;
    dir_length = strlen (dir_home) + strlen (language) + 16;
    dir_name = (char *)malloc (dir_length * sizeof (char));
    if (!dir_name)
        return;
    
    snprintf (dir_name, dir_length, "%s/%s/autoload", dir_home, language);
    weechat_exec_on_files (dir_name, NULL, callback);
    
    free (dir_name);
}

/*
 * script_search: search a script in list
 */

struct t_plugin_script *
script_search (struct t_weechat_plugin *weechat_plugin,
               struct t_plugin_script **list, char *name)
{
    struct t_plugin_script *ptr_script;
    
    for (ptr_script = *list; ptr_script;
         ptr_script = ptr_script->next_script)
    {
        if (weechat_strcasecmp (ptr_script->name, name) == 0)
            return ptr_script;
    }
    
    /* script not found */
    return NULL;
}

/*
 * script_search_full_name: search the full path name of a script
 */

char *
script_search_full_name (struct t_weechat_plugin *weechat_plugin,
                         char *language, char *filename)
{
    char *final_name, *dir_home, *dir_system;
    int length;
    struct stat st;
    
    if (filename[0] == '~')
    {
        dir_home = getenv ("HOME");
        if (!dir_home)
            return NULL;
        length = strlen (dir_home) + strlen (filename + 1) + 1;
        final_name = (char *)malloc (length);
        if (final_name)
        {
            snprintf (final_name, length, "%s%s", dir_home, filename + 1);
            return final_name;
        }
        return NULL;
    }
    
    dir_home = weechat_info_get ("weechat_dir");
    if (dir_home)
    {
        /* try WeeChat user's autoload dir */
        length = strlen (dir_home) + strlen (language) + 8 + strlen (filename) + 16;
        final_name = (char *)malloc (length);
        if (final_name)
        {
            snprintf (final_name, length,
                      "%s/%s/autoload/%s", dir_home, language, filename);
            if ((stat (final_name, &st) == 0) && (st.st_size > 0))
                return final_name;
            free (final_name);
        }

        /* try WeeChat language user's dir */
        length = strlen (dir_home) + strlen (language) + strlen (filename) + 16;
        final_name = (char *)malloc (length);
        if (final_name)
        {
            snprintf (final_name, length,
                      "%s/%s/%s", dir_home, language, filename);
            if ((stat (final_name, &st) == 0) && (st.st_size > 0))
                return final_name;
            free (final_name);
        }
        
        /* try WeeChat user's dir */
        length = strlen (dir_home) + strlen (filename) + 16;
        final_name = (char *)malloc (length);
        if (final_name)
        {
            snprintf (final_name, length,
                      "%s/%s", dir_home, filename);
            if ((stat (final_name, &st) == 0) && (st.st_size > 0))
                return final_name;
            free (final_name);
        }
    }
    
    /* try WeeChat system dir */
    dir_system = weechat_info_get ("weechat_sharedir");
    if (dir_system)
    {
        length = strlen (dir_system) + strlen (dir_system) + strlen (filename) + 16;
        final_name = (char *)malloc (length);
        if (final_name)
        {
            snprintf (final_name,length,
                      "%s/%s/%s", dir_system, language, filename);
            if ((stat (final_name, &st) == 0) && (st.st_size > 0))
                return final_name;
            free (final_name);
        }
    }
    
    return strdup (filename);
}

/*
 * script_add: add a script to list of scripts
 */

struct t_plugin_script *
script_add (struct t_weechat_plugin *weechat_plugin,
            struct t_plugin_script **script_list,
            char *filename,
            char *name, char *author, char *version, char *license,
            char *shutdown_func, char *description, char *charset)
{
    struct t_plugin_script *new_script;
    
    if (strchr (name, ' '))
    {
        weechat_printf (NULL,
                        _("%s: error loading script \"%s\" (bad name, spaces "
                          "are forbidden)"),
                        weechat_plugin->name, name);
        return NULL;
    }
    
    if (script_option_check_license
        && (weechat_strcmp_ignore_chars (weechat_plugin->license, license,
                                         "0123456789-.,/\\()[]{}", 0) != 0))
    {
        weechat_printf (NULL,
                        _("%s%s: warning, license \"%s\" for script \"%s\" "
                          "differs from plugin license (\"%s\")"),
                        weechat_prefix ("error"), weechat_plugin->name,
                        license, name, weechat_plugin->license);
    }
    
    new_script = (struct t_plugin_script *)malloc (sizeof (struct t_plugin_script));
    if (new_script)
    {
        new_script->filename = strdup (filename);
        new_script->interpreter = NULL;
        new_script->name = strdup (name);
        new_script->author = strdup (author);
        new_script->version = strdup (version);
        new_script->license = strdup (license);
        new_script->description = strdup (description);
        new_script->shutdown_func = (shutdown_func) ?
            strdup (shutdown_func) : NULL;
        new_script->charset = (charset) ? strdup (charset) : NULL;
        
        new_script->callbacks = NULL;
        
        /* add new script to list */
        if ((*script_list))
            (*script_list)->prev_script = new_script;
        new_script->prev_script = NULL;
        new_script->next_script = (*script_list);
        (*script_list) = new_script;
        
        return new_script;
    }
    
    weechat_printf (NULL,
                    _("%s: error loading script \"%s\" (not enough memory)"),
                    weechat_plugin->name, name);
    
    return NULL;
}

/*
 * script_remove: remove a script from list of scripts
 */

void
script_remove (struct t_weechat_plugin *weechat_plugin,
               struct t_plugin_script **script_list,
               struct t_plugin_script *script)
{
    /* remove all callbacks created by this script */
    script_callback_remove_all (weechat_plugin, script);
    
    /* free data */
    if (script->filename)
        free (script->filename);
    if (script->name)
        free (script->name);
    if (script->description)
        free (script->description);
    if (script->version)
        free (script->version);
    if (script->shutdown_func)
        free (script->shutdown_func);
    if (script->charset)
        free (script->charset);
    
    /* remove script from list */
    if (script->prev_script)
        (script->prev_script)->next_script = script->next_script;
    else
        (*script_list) = script->next_script;
    if (script->next_script)
        (script->next_script)->prev_script = script->prev_script;
    
    /* free script */
    free (script);
}
