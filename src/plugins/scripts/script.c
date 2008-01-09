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
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include "../weechat-plugin.h"
#include "script.h"


/*
 * weechat_script_pointer_to_string: convert pointer to string for usage
 *                                   in a script (any language)
 *                                   WARNING: result has to be free() after use
 */

char *
weechat_script_pointer_to_string (void *pointer)
{
    char pointer_str[128];

    if (!pointer)
        return strdup ("");
    
    snprintf (pointer_str, sizeof (pointer_str) - 1,
              "0x%x", (unsigned int)pointer);
    
    return strdup (pointer_str);
}

/*
 * weechat_script_string_to_pointer: convert stirng to pointer for usage
 *                                   outside script
 */

void *
weechat_script_string_to_pointer (char *pointer_str)
{
    unsigned int value;
    
    if (!pointer_str || (pointer_str[0] != '0') || (pointer_str[0] != 'x'))
        return NULL;
    
    sscanf (pointer_str + 2, "%x", &value);
    
    return (void *)value;
}

/*
 * weechat_script_auto_load: auto-load all scripts in a directory
 */

void
weechat_script_auto_load (struct t_weechat_plugin *weechat_plugin,
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
 * weechat_script_search: search a script in list
 */

struct t_plugin_script *
weechat_script_search (struct t_weechat_plugin *weechat_plugin,
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
 * weechat_script_search_full_name: search the full path name of a script
 */

char *
weechat_script_search_full_name (struct t_weechat_plugin *weechat_plugin,
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
 * weechat_script_add: add a script to list of scripts
 */

struct t_plugin_script *
weechat_script_add (struct t_weechat_plugin *weechat_plugin,
                    struct t_plugin_script **script_list,
                    char *filename,
                    char *name, char *version,
                    char *shutdown_func, char *description,
                    char *charset)
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
    
    new_script = (struct t_plugin_script *)malloc (sizeof (struct t_plugin_script));
    if (new_script)
    {
        new_script->filename = strdup (filename);
        new_script->interpreter = NULL;
        new_script->name = strdup (name);
        new_script->version = strdup (version);
        new_script->shutdown_func = strdup (shutdown_func);
        new_script->description = strdup (description);
        new_script->charset = (charset) ? strdup (charset) : NULL;
        
        new_script->hooks = NULL;
        
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
 * weechat_script_remove_script_hook: remove a script_hook from script
 */

void
weechat_script_remove_script_hook (struct t_weechat_plugin *weechat_plugin,
                                   struct t_plugin_script *script,
                                   struct t_script_hook *script_hook)
{
    /* remove script_hook from list */
    if (script_hook->prev_hook)
        script_hook->prev_hook->next_hook = script_hook->next_hook;
    if (script_hook->next_hook)
        script_hook->next_hook->prev_hook = script_hook->prev_hook;
    if (script->hooks == script_hook)
        script->hooks = script_hook->next_hook;
    
    /* unhook and free data */
    if (script_hook->hook)
        weechat_unhook (script_hook->hook);
    if (script_hook->function)
        free (script_hook->function);
}

/*
 * weechat_script_remove: remove a script from list of scripts
 */

void
weechat_script_remove (struct t_weechat_plugin *weechat_plugin,
                       struct t_plugin_script **script_list,
                       struct t_plugin_script *script)
{
    /* remove all hooks created by this script */
    while (script->hooks)
    {
        weechat_script_remove_script_hook (weechat_plugin,
                                           script,
                                           script->hooks);
    }
    
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

/*
 * weechat_script_charset_set: set charset for script
 */

void
weechat_script_charset_set (struct t_plugin_script *script,
                            char *charset)
{
    if (script->charset)
        free (script->charset);
    
    script->charset = (charset) ? strdup (charset) : NULL;
}

/*
 * weechat_script_printf: print a message
 */

void
weechat_script_printf (struct t_weechat_plugin *weechat_plugin,
                       struct t_plugin_script *script,
                       struct t_gui_buffer *buffer, char *format, ...)
{
    va_list argptr;
    static char buf[8192];
    char *buf2;
    
    va_start (argptr, format);
    vsnprintf (buf, sizeof (buf) - 1, format, argptr);
    va_end (argptr);
    
    buf2 = (script->charset && script->charset[0]) ?
        weechat_iconv_to_internal (script->charset, buf) : NULL;
    weechat_printf (buffer, "%s", (buf2) ? buf2 : buf);
    if (buf2)
        free (buf2);
}

/*
 * weechat_script_infobar_printf: print a message in infobar
 */

void
weechat_script_infobar_printf (struct t_weechat_plugin *weechat_plugin,
                               struct t_plugin_script *script,
                               int delay, char *color_name, char *format, ...)
{
    va_list argptr;
    static char buf[1024];
    char *buf2;
    
    va_start (argptr, format);
    vsnprintf (buf, sizeof (buf) - 1, format, argptr);
    va_end (argptr);
    
    buf2 = (script->charset && script->charset[0]) ?
        weechat_iconv_to_internal (script->charset, buf) : NULL;
    weechat_infobar_printf (delay, color_name, "%s", (buf2) ? buf2 : buf);
    if (buf2)
        free (buf2);
}

/*
 * weechat_script_log_printf: add a message in WeeChat log file
 */

void
weechat_script_log_printf (struct t_weechat_plugin *weechat_plugin,
                           struct t_plugin_script *script,
                           char *format, ...)
{
    va_list argptr;
    static char buf[1024];
    char *buf2;
    
    va_start (argptr, format);
    vsnprintf (buf, sizeof (buf) - 1, format, argptr);
    va_end (argptr);
    
    buf2 = (script->charset && script->charset[0]) ?
        weechat_iconv_to_internal (script->charset, buf) : NULL;
    weechat_log_printf ("%s", (buf2) ? buf2 : buf);
    if (buf2)
        free (buf2);
}

/*
 * weechat_script_hook_command: hook a command
 *                              return 1 if ok, 0 if error
 */

int
weechat_script_hook_command (struct t_weechat_plugin *weechat_plugin,
                             struct t_plugin_script *script,
                             char *command, char *description,
                             char *args, char *args_description,
                             char *completion,
                             int (*callback)(void *data,
                                             struct t_gui_buffer *buffer,
                                             int argc, char **argv,
                                             char **argv_eol),
                             char *function)
{
    struct t_script_hook *new_script_hook;
    struct t_hook *new_hook;

    new_script_hook = (struct t_script_hook *)malloc (sizeof (struct t_script_hook));
    if (!new_script_hook)
        return 0;

    new_script_hook->hook = NULL;
    new_script_hook->function = NULL;
    new_script_hook->script = NULL;
    
    new_hook = weechat_hook_command (command, description, args,
                                     args_description, completion,
                                     callback, new_script_hook);
    if (!new_hook)
    {
        free (new_script_hook);
        return 0;
    }
    
    new_script_hook->hook = new_hook;
    new_script_hook->function = strdup (function);
    new_script_hook->script = script;
    
    /* add script_hook to list of hooks for current script */
    if (script->hooks)
        script->hooks->prev_hook = new_script_hook;
    new_script_hook->prev_hook = NULL;
    new_script_hook->next_hook = script->hooks;
    script->hooks = new_script_hook;
    
    return 1;
}

/*
 * weechat_script_hook_timer: hook a timer
 *                            return 1 if ok, 0 if error
 */

int
weechat_script_hook_timer (struct t_weechat_plugin *weechat_plugin,
                           struct t_plugin_script *script,
                           int interval, int align_second, int max_calls,
                           int (*callback)(void *data),
                           char *function)
{
    struct t_script_hook *new_script_hook;
    struct t_hook *new_hook;

    new_script_hook = (struct t_script_hook *)malloc (sizeof (struct t_script_hook));
    if (!new_script_hook)
        return 0;

    new_script_hook->hook = NULL;
    new_script_hook->function = NULL;
    new_script_hook->script = NULL;
    
    new_hook = weechat_hook_timer (interval, align_second, max_calls,
                                   callback, new_script_hook);
    if (!new_hook)
    {
        free (new_script_hook);
        return 0;
    }
    
    new_script_hook->hook = new_hook;
    new_script_hook->function = strdup (function);
    new_script_hook->script = script;
    
    /* add script_hook to list of hooks for current script */
    if (script->hooks)
        script->hooks->prev_hook = new_script_hook;
    new_script_hook->prev_hook = NULL;
    new_script_hook->next_hook = script->hooks;
    script->hooks = new_script_hook;
    
    return 1;
}

/*
 * weechat_script_hook_fd: hook a fd
 *                         return 1 if ok, 0 if error
 */

int
weechat_script_hook_fd (struct t_weechat_plugin *weechat_plugin,
                        struct t_plugin_script *script,
                        int fd, int flag_read, int flag_write,
                        int flag_exception,
                        int (*callback)(void *data),
                        char *function)
{
    struct t_script_hook *new_script_hook;
    struct t_hook *new_hook;

    new_script_hook = (struct t_script_hook *)malloc (sizeof (struct t_script_hook));
    if (!new_script_hook)
        return 0;

    new_script_hook->hook = NULL;
    new_script_hook->function = NULL;
    new_script_hook->script = NULL;
    
    new_hook = weechat_hook_fd (fd, flag_read, flag_write, flag_exception,
                                callback, new_script_hook);
    if (!new_hook)
    {
        free (new_script_hook);
        return 0;
    }
    
    new_script_hook->hook = new_hook;
    new_script_hook->function = strdup (function);
    new_script_hook->script = script;
    
    /* add script_hook to list of hooks for current script */
    if (script->hooks)
        script->hooks->prev_hook = new_script_hook;
    new_script_hook->prev_hook = NULL;
    new_script_hook->next_hook = script->hooks;
    script->hooks = new_script_hook;
    
    return 1;
}

/*
 * weechat_script_hook_print: hook a print
 *                            return 1 if ok, 0 if error
 */

int
weechat_script_hook_print (struct t_weechat_plugin *weechat_plugin,
                           struct t_plugin_script *script,
                           struct t_gui_buffer *buffer,
                           char *message, int strip_colors,
                           int (*callback)(void *data,
                                           struct t_gui_buffer *buffer,
                                           time_t date, char *prefix,
                                           char *message),
                           char *function)
{
    struct t_script_hook *new_script_hook;
    struct t_hook *new_hook;

    new_script_hook = (struct t_script_hook *)malloc (sizeof (struct t_script_hook));
    if (!new_script_hook)
        return 0;

    new_script_hook->hook = NULL;
    new_script_hook->function = NULL;
    new_script_hook->script = NULL;
    
    new_hook = weechat_hook_print (buffer, message, strip_colors,
                                   callback, new_script_hook);
    if (!new_hook)
    {
        free (new_script_hook);
        return 0;
    }
    
    new_script_hook->hook = new_hook;
    new_script_hook->function = strdup (function);
    new_script_hook->script = script;
    
    /* add script_hook to list of hooks for current script */
    if (script->hooks)
        script->hooks->prev_hook = new_script_hook;
    new_script_hook->prev_hook = NULL;
    new_script_hook->next_hook = script->hooks;
    script->hooks = new_script_hook;
    
    return 1;
}

/*
 * weechat_script_hook_signal: hook a signal
 *                             return 1 if ok, 0 if error
 */

int
weechat_script_hook_signal (struct t_weechat_plugin *weechat_plugin,
                            struct t_plugin_script *script,
                            char *signal,
                            int (*callback)(void *data, char *signal,
                                            char *type_data,
                                            void *signal_data),
                            char *function)
{
    struct t_script_hook *new_script_hook;
    struct t_hook *new_hook;

    new_script_hook = (struct t_script_hook *)malloc (sizeof (struct t_script_hook));
    if (!new_script_hook)
        return 0;

    new_script_hook->hook = NULL;
    new_script_hook->function = NULL;
    new_script_hook->script = NULL;
    
    new_hook = weechat_hook_signal (signal, callback, new_script_hook);
    if (!new_hook)
    {
        free (new_script_hook);
        return 0;
    }
    
    new_script_hook->hook = new_hook;
    new_script_hook->function = strdup (function);
    new_script_hook->script = script;
    
    /* add script_hook to list of hooks for current script */
    if (script->hooks)
        script->hooks->prev_hook = new_script_hook;
    new_script_hook->prev_hook = NULL;
    new_script_hook->next_hook = script->hooks;
    script->hooks = new_script_hook;
    
    return 1;
}

/*
 * weechat_script_hook_config: hook a config option
 *                             return 1 if ok, 0 if error
 */

int
weechat_script_hook_config (struct t_weechat_plugin *weechat_plugin,
                            struct t_plugin_script *script,
                            char *type, char *option,
                            int (*callback)(void *data, char *type,
                                            char *option, char *value),
                            char *function)
{
    struct t_script_hook *new_script_hook;
    struct t_hook *new_hook;

    new_script_hook = (struct t_script_hook *)malloc (sizeof (struct t_script_hook));
    if (!new_script_hook)
        return 0;

    new_script_hook->hook = NULL;
    new_script_hook->function = NULL;
    new_script_hook->script = NULL;
    
    new_hook = weechat_hook_config (type, option, callback, new_script_hook);
    if (!new_hook)
    {
        free (new_script_hook);
        return 0;
    }
    
    new_script_hook->hook = new_hook;
    new_script_hook->function = strdup (function);
    new_script_hook->script = script;
    
    /* add script_hook to list of hooks for current script */
    if (script->hooks)
        script->hooks->prev_hook = new_script_hook;
    new_script_hook->prev_hook = NULL;
    new_script_hook->next_hook = script->hooks;
    script->hooks = new_script_hook;
    
    return 1;
}

/*
 * weechat_script_hook_completion: hook a completion
 *                                 return 1 if ok, 0 if error
 */

int
weechat_script_hook_completion (struct t_weechat_plugin *weechat_plugin,
                                struct t_plugin_script *script,
                                char *completion,
                                int (*callback)(void *data, char *completion,
                                                struct t_gui_buffer *buffer,
                                                struct t_weelist *list),
                                char *function)
{
    struct t_script_hook *new_script_hook;
    struct t_hook *new_hook;

    new_script_hook = (struct t_script_hook *)malloc (sizeof (struct t_script_hook));
    if (!new_script_hook)
        return 0;

    new_script_hook->hook = NULL;
    new_script_hook->function = NULL;
    new_script_hook->script = NULL;
    
    new_hook = weechat_hook_completion (completion, callback, new_script_hook);
    if (!new_hook)
    {
        free (new_script_hook);
        return 0;
    }
    
    new_script_hook->hook = new_hook;
    new_script_hook->function = strdup (function);
    new_script_hook->script = script;
    
    /* add script_hook to list of hooks for current script */
    if (script->hooks)
        script->hooks->prev_hook = new_script_hook;
    new_script_hook->prev_hook = NULL;
    new_script_hook->next_hook = script->hooks;
    script->hooks = new_script_hook;
    
    return 1;
}

/*
 * weechat_script_unhook: unhook something
 */

void
weechat_script_unhook (struct t_weechat_plugin *weechat_plugin,
                       struct t_plugin_script *script,
                       struct t_hook *hook)
{
    struct t_script_hook *ptr_script_hook;

    for (ptr_script_hook = script->hooks; ptr_script_hook;
         ptr_script_hook = ptr_script_hook->next_hook)
    {
        if (ptr_script_hook->hook == hook)
            break;
    }
    
    if (ptr_script_hook)
        weechat_script_remove_script_hook (weechat_plugin,
                                           script,
                                           ptr_script_hook);
}

/*
 * weechat_script_command: execute a command (simulate user entry)
 */

void
weechat_script_command (struct t_weechat_plugin *weechat_plugin,
                        struct t_plugin_script *script,
                        struct t_gui_buffer *buffer, char *command)
{
    char *command2;
    
    command2 = (script->charset && script->charset[0]) ?
        weechat_iconv_to_internal (script->charset, command) : NULL;
    weechat_command (buffer, (command2) ? command2 : command);
    if (command2)
        free (command2);
}

/*
 * weechat_script_config_get_plugin: get a value of a script option
 *                                   format in file is: plugin.script.option=value
 */

char *
weechat_script_config_get_plugin (struct t_weechat_plugin *weechat_plugin,
                                  struct t_plugin_script *script,
                                  char *option)
{
    char *option_fullname, *return_value;
    
    option_fullname = (char *)malloc (strlen (script->name) +
                                      strlen (option) + 2);
    if (!option_fullname)
        return NULL;
    
    strcpy (option_fullname, script->name);
    strcat (option_fullname, ".");
    strcat (option_fullname, option);
    
    return_value = weechat_config_get_plugin (option_fullname);
    free (option_fullname);
    
    return return_value;
}

/*
 * weechat_script_config_set_plugin: set value of a script config option
 *                                   format in file is: plugin.script.option=value
 */

int
weechat_script_config_set_plugin (struct t_weechat_plugin *weechat_plugin,
                                  struct t_plugin_script *script,
                                  char *option, char *value)
{
    char *option_fullname;
    int return_code;
    
    option_fullname = (char *)malloc (strlen (script->name) +
                                      strlen (option) + 2);
    if (!option_fullname)
        return 0;
    
    strcpy (option_fullname, script->name);
    strcat (option_fullname, ".");
    strcat (option_fullname, option);
    
    return_code = weechat_config_set_plugin (option_fullname, value);
    free (option_fullname);
    
    return return_code;
}
