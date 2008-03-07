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

/* script-api.c: script API */


#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "../weechat-plugin.h"
#include "script.h"
#include "script-callback.h"


/*
 * script_api_charset_set: set charset for script
 */

void
script_api_charset_set (struct t_plugin_script *script,
                        char *charset)
{
    if (script->charset)
        free (script->charset);
    
    script->charset = (charset) ? strdup (charset) : NULL;
}

/*
 * script_api_config_new: create a new configuration file
 *                        return new configuration file, NULL if error
 */

struct t_config_file *
script_api_config_new (struct t_weechat_plugin *weechat_plugin,
                       struct t_plugin_script *script,
                       char *filename,
                       int (*callback_reload)(void *data,
                                              struct t_config_file *config_file),
                       char *function)
{
    struct t_script_callback *new_script_callback;
    struct t_config_file *new_config_file;

    if (function && function[0])
    {
        new_script_callback = script_callback_alloc ();
        if (!new_script_callback)
            return NULL;
        
        new_config_file = weechat_config_new (filename, callback_reload,
                                              new_script_callback);
        if (!new_config_file)
        {
            free (new_script_callback);
            return NULL;
        }
        
        new_script_callback->script = script;
        new_script_callback->function = strdup (function);
        new_script_callback->config_file = new_config_file;
        
        script_callback_add (script, new_script_callback);
    }
    else
    {
        new_config_file = weechat_config_new (filename, NULL, NULL);
    }
    
    return new_config_file;
}

/*
 * script_api_config_new_section: create a new section in configuration file
 *                                return new section, NULL if error
 */

struct t_config_section *
script_api_config_new_section (struct t_weechat_plugin *weechat_plugin,
                               struct t_plugin_script *script,
                               struct t_config_file *config_file,
                               char *name,
                               void (*callback_read)(void *data,
                                                     struct t_config_file *config_file,
                                                     char *option_name,
                                                     char *value),
                               char *function_read,
                               void (*callback_write)(void *data,
                                                      struct t_config_file *config_file,
                                                      char *section_name),
                               char *function_write,
                               void (*callback_write_default)(void *data,
                                                              struct t_config_file *config_file,
                                                              char *section_name),
                               char *function_write_default)
{
    struct t_script_callback *new_script_callback1, *new_script_callback2;
    struct t_script_callback *new_script_callback3;
    struct t_config_section *new_section;
    void *callback1, *callback2, *callback3;

    new_script_callback1 = NULL;
    new_script_callback2 = NULL;
    new_script_callback3 = NULL;
    callback1 = NULL;
    callback2 = NULL;
    callback3 = NULL;
    
    if (function_read && function_read[0])
    {
        new_script_callback1 = script_callback_alloc ();
        if (!new_script_callback1)
            return NULL;
        callback1 = callback_read;
    }

    if (function_write && function_write[0])
    {
        new_script_callback2 = script_callback_alloc ();
        if (!new_script_callback2)
        {
            if (new_script_callback1)
                free (new_script_callback1);
            return NULL;
        }
        callback2 = callback_write;
    }

    if (function_write_default && function_write_default[0])
    {
        new_script_callback3 = script_callback_alloc ();
        if (!new_script_callback3)
        {
            if (new_script_callback1)
                free (new_script_callback1);
            if (new_script_callback2)
                free (new_script_callback2);
            return NULL;
        }
        callback3 = callback_write_default;
    }
    
    new_section = weechat_config_new_section (config_file,
                                              name,
                                              callback1,
                                              new_script_callback1,
                                              callback2,
                                              new_script_callback2,
                                              callback3,
                                              new_script_callback3);
    if (!new_section)
    {
        if (new_script_callback1)
            free (new_script_callback1);
        if (new_script_callback2)
            free (new_script_callback2);
        if (new_script_callback3)
            free (new_script_callback3);
        return NULL;
    }

    if (new_script_callback1)
    {
        new_script_callback1->script = script;
        new_script_callback1->function = strdup (function_read);
        new_script_callback1->config_file = config_file;
        new_script_callback1->config_section = new_section;
        script_callback_add (script, new_script_callback1);
    }

    if (new_script_callback2)
    {
        new_script_callback2->script = script;
        new_script_callback2->function = strdup (function_write);
        new_script_callback2->config_file = config_file;
        new_script_callback2->config_section = new_section;
        script_callback_add (script, new_script_callback2);
    }

    if (new_script_callback3)
    {
        new_script_callback3->script = script;
        new_script_callback3->function = strdup (function_write_default);
        new_script_callback3->config_file = config_file;
        new_script_callback3->config_section = new_section;
        script_callback_add (script, new_script_callback3);
    }
    
    return new_section;
}

/*
 * script_api_config_new_option: create a new option in section
 *                               return new option, NULL if error
 */

struct t_config_option *
script_api_config_new_option (struct t_weechat_plugin *weechat_plugin,
                              struct t_plugin_script *script,
                              struct t_config_file *config_file,
                              struct t_config_section *section,
                              char *name, char *type,
                              char *description, char *string_values,
                              int min, int max, char *default_value,
                              void (*callback_change)(void *data),
                              char *function)
{
    struct t_script_callback *new_script_callback;
    struct t_config_option *new_option;

    if (function && function[0])
    {
        new_script_callback = script_callback_alloc ();
        if (!new_script_callback)
            return NULL;
        
        new_option = weechat_config_new_option (config_file, section, name, type,
                                                description, string_values, min,
                                                max, default_value,
                                                callback_change,
                                                new_script_callback);
        if (!new_option)
        {
            free (new_script_callback);
            return NULL;
        }
        
        new_script_callback->script = script;
        new_script_callback->function = strdup (function);
        new_script_callback->config_file = config_file;
        new_script_callback->config_section = section;
        new_script_callback->config_option = new_option;
        
        script_callback_add (script, new_script_callback);
    }
    else
    {
        new_option = weechat_config_new_option (config_file, section, name, type,
                                                description, string_values, min,
                                                max, default_value, NULL, NULL);
    }
    
    return new_option;
}

/*
 * script_api_config_free: free configuration file
 */

void
script_api_config_free (struct t_weechat_plugin *weechat_plugin,
                        struct t_plugin_script *script,
                        struct t_config_file *config_file)
{
    struct t_script_callback *ptr_script_callback;
    
    if (!weechat_plugin || !script || !config_file)
        return;
    
    weechat_config_free (config_file);
    
    for (ptr_script_callback = script->callbacks; ptr_script_callback;
         ptr_script_callback = ptr_script_callback->next_callback)
    {
        if (ptr_script_callback->config_file == config_file)
            script_callback_remove (script, ptr_script_callback);
    }
}

/*
 * script_api_printf: print a message
 */

void
script_api_printf (struct t_weechat_plugin *weechat_plugin,
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
 * script_api_infobar_printf: print a message in infobar
 */

void
script_api_infobar_printf (struct t_weechat_plugin *weechat_plugin,
                           struct t_plugin_script *script,
                           int delay, char *color_name,
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
    weechat_infobar_printf (delay, color_name, "%s", (buf2) ? buf2 : buf);
    if (buf2)
        free (buf2);
}

/*
 * script_api_log_printf: add a message in WeeChat log file
 */

void
script_api_log_printf (struct t_weechat_plugin *weechat_plugin,
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
 * script_api_hook_command: hook a command
 *                          return new hook, NULL if error
 */

struct t_hook *
script_api_hook_command (struct t_weechat_plugin *weechat_plugin,
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
    struct t_script_callback *new_script_callback;
    struct t_hook *new_hook;

    new_script_callback = script_callback_alloc ();
    if (!new_script_callback)
        return NULL;
    
    new_hook = weechat_hook_command (command, description, args,
                                     args_description, completion,
                                     callback, new_script_callback);
    if (!new_hook)
    {
        free (new_script_callback);
        return NULL;
    }
    
    new_script_callback->script = script;
    new_script_callback->function = strdup (function);
    new_script_callback->hook = new_hook;
    
    script_callback_add (script, new_script_callback);
    
    return new_hook;
}

/*
 * script_api_hook_timer: hook a timer
 *                        return new hook, NULL if error
 */

struct t_hook *
script_api_hook_timer (struct t_weechat_plugin *weechat_plugin,
                       struct t_plugin_script *script,
                       int interval, int align_second, int max_calls,
                       int (*callback)(void *data),
                       char *function)
{
    struct t_script_callback *new_script_callback;
    struct t_hook *new_hook;
    
    new_script_callback = script_callback_alloc ();
    if (!new_script_callback)
        return NULL;
    
    new_hook = weechat_hook_timer (interval, align_second, max_calls,
                                   callback, new_script_callback);
    if (!new_hook)
    {
        free (new_script_callback);
        return NULL;
    }
    
    new_script_callback->script = script;
    new_script_callback->function = strdup (function);
    new_script_callback->hook = new_hook;

    script_callback_add (script, new_script_callback);
    
    return new_hook;
}

/*
 * script_api_hook_fd: hook a fd
 *                     return new hook, NULL if error
 */

struct t_hook *
script_api_hook_fd (struct t_weechat_plugin *weechat_plugin,
                    struct t_plugin_script *script,
                    int fd, int flag_read, int flag_write,
                    int flag_exception,
                    int (*callback)(void *data),
                    char *function)
{
    struct t_script_callback *new_script_callback;
    struct t_hook *new_hook;

    new_script_callback = script_callback_alloc ();
    if (!new_script_callback)
        return NULL;
    
    new_hook = weechat_hook_fd (fd, flag_read, flag_write, flag_exception,
                                callback, new_script_callback);
    if (!new_hook)
    {
        free (new_script_callback);
        return NULL;
    }
    
    new_script_callback->script = script;
    new_script_callback->function = strdup (function);
    new_script_callback->hook = new_hook;
    
    script_callback_add (script, new_script_callback);
    
    return new_hook;
}

/*
 * script_api_hook_print: hook a print
 *                        return new hook, NULL if error
 */

struct t_hook *
script_api_hook_print (struct t_weechat_plugin *weechat_plugin,
                       struct t_plugin_script *script,
                       struct t_gui_buffer *buffer,
                       char *message, int strip_colors,
                       int (*callback)(void *data,
                                       struct t_gui_buffer *buffer,
                                       time_t date, char *prefix,
                                       char *message),
                       char *function)
{
    struct t_script_callback *new_script_callback;
    struct t_hook *new_hook;
    
    new_script_callback = script_callback_alloc ();
    if (!new_script_callback)
        return NULL;
    
    new_hook = weechat_hook_print (buffer, message, strip_colors,
                                   callback, new_script_callback);
    if (!new_hook)
    {
        free (new_script_callback);
        return NULL;
    }
    
    new_script_callback->script = script;
    new_script_callback->function = strdup (function);
    new_script_callback->hook = new_hook;
    
    script_callback_add (script, new_script_callback);
    
    return new_hook;
}

/*
 * script_api_hook_signal: hook a signal
 *                         return new hook, NULL if error
 */

struct t_hook *
script_api_hook_signal (struct t_weechat_plugin *weechat_plugin,
                        struct t_plugin_script *script,
                        char *signal,
                        int (*callback)(void *data, char *signal,
                                        char *type_data,
                                        void *signal_data),
                        char *function)
{
    struct t_script_callback *new_script_callback;
    struct t_hook *new_hook;
    
    new_script_callback = script_callback_alloc ();
    if (!new_script_callback)
        return NULL;
    
    new_hook = weechat_hook_signal (signal, callback, new_script_callback);
    if (!new_hook)
    {
        free (new_script_callback);
        return NULL;
    }
    
    new_script_callback->script = script;
    new_script_callback->function = strdup (function);
    new_script_callback->hook = new_hook;
    
    script_callback_add (script, new_script_callback);
    
    return new_hook;
}

/*
 * script_api_hook_config: hook a config option
 *                         return new hook, NULL if error
 */

struct t_hook *
script_api_hook_config (struct t_weechat_plugin *weechat_plugin,
                        struct t_plugin_script *script,
                        char *type, char *option,
                        int (*callback)(void *data, char *type,
                                        char *option, char *value),
                        char *function)
{
    struct t_script_callback *new_script_callback;
    struct t_hook *new_hook;
    
    new_script_callback = script_callback_alloc ();
    if (!new_script_callback)
        return NULL;
    
    new_hook = weechat_hook_config (type, option, callback, new_script_callback);
    if (!new_hook)
    {
        free (new_script_callback);
        return NULL;
    }
    
    new_script_callback->script = script;
    new_script_callback->function = strdup (function);
    new_script_callback->hook = new_hook;
    
    script_callback_add (script, new_script_callback);
    
    return new_hook;
}

/*
 * script_api_hook_completion: hook a completion
 *                             return new hook, NULL if error
 */

struct t_hook *
script_api_hook_completion (struct t_weechat_plugin *weechat_plugin,
                            struct t_plugin_script *script,
                            char *completion,
                            int (*callback)(void *data, char *completion,
                                            struct t_gui_buffer *buffer,
                                            struct t_weelist *list),
                            char *function)
{
    struct t_script_callback *new_script_callback;
    struct t_hook *new_hook;
    
    new_script_callback = script_callback_alloc ();
    if (!new_script_callback)
        return NULL;
    
    new_hook = weechat_hook_completion (completion, callback, new_script_callback);
    if (!new_hook)
    {
        free (new_script_callback);
        return NULL;
    }
    
    new_script_callback->script = script;
    new_script_callback->function = strdup (function);
    new_script_callback->hook = new_hook;
    
    script_callback_add (script, new_script_callback);
    
    return new_hook;
}

/*
 * script_api_hook_modifier: hook a modifier
 *                           return new hook, NULL if error
 */

struct t_hook *
script_api_hook_modifier (struct t_weechat_plugin *weechat_plugin,
                          struct t_plugin_script *script,
                          char *modifier,
                          char *(*callback)(void *data, char *modifier,
                                            char *modifier_data, char *string),
                          char *function)
{
    struct t_script_callback *new_script_callback;
    struct t_hook *new_hook;
    
    new_script_callback = script_callback_alloc ();
    if (!new_script_callback)
        return NULL;
    
    new_hook = weechat_hook_modifier (modifier, callback, new_script_callback);
    if (!new_hook)
    {
        free (new_script_callback);
        return NULL;
    }
    
    new_script_callback->script = script;
    new_script_callback->function = strdup (function);
    new_script_callback->hook = new_hook;
    
    script_callback_add (script, new_script_callback);
    
    return new_hook;
}

/*
 * script_api_unhook: unhook something
 */

void
script_api_unhook (struct t_weechat_plugin *weechat_plugin,
                   struct t_plugin_script *script,
                   struct t_hook *hook)
{
    struct t_script_callback *ptr_script_callback;
    
    if (!weechat_plugin || !script || !hook)
        return;
    
    weechat_unhook (hook);
    
    for (ptr_script_callback = script->callbacks; ptr_script_callback;
         ptr_script_callback = ptr_script_callback->next_callback)
    {
        if (ptr_script_callback->hook == hook)
            script_callback_remove (script, ptr_script_callback);
    }
}

/*
 * script_api_unhook_all: remove all hooks from a script
 */

void
script_api_unhook_all (struct t_plugin_script *script)
{
    struct t_script_callback *ptr_callback, *next_callback;
    
    ptr_callback = script->callbacks;
    while (ptr_callback)
    {
        next_callback = ptr_callback->next_callback;
        
        script_callback_remove (script, ptr_callback);
        
        ptr_callback = next_callback;
    }
}

/*
 * script_api_buffer_new: create a new buffer
 */

struct t_gui_buffer *
script_api_buffer_new (struct t_weechat_plugin *weechat_plugin,
                       struct t_plugin_script *script,
                       char *category, char *name,
                       int (*input_callback)(void *data,
                                             struct t_gui_buffer *buffer,
                                             char *input_data),
                       char *function_input,
                       int (*close_callback)(void *data,
                                             struct t_gui_buffer *buffer),
                       char *function_close)
{
    struct t_script_callback *new_script_callback_input;
    struct t_script_callback *new_script_callback_close;
    struct t_gui_buffer *new_buffer;
    
    if ((!function_input || !function_input[0])
        && (!function_close || !function_close[0]))
        return weechat_buffer_new (category, name, NULL, NULL, NULL, NULL);
    
    new_script_callback_input = NULL;
    new_script_callback_close = NULL;
    
    if (function_input && function_input[0])
    {
        new_script_callback_input = script_callback_alloc ();
        if (!new_script_callback_input)
            return NULL;
    }
    
    if (function_close && function_close[0])
    {
        new_script_callback_close = script_callback_alloc ();
        if (!new_script_callback_close)
        {
            if (new_script_callback_input)
                free (new_script_callback_input);
            return NULL;
        }
    }
    
    new_buffer = weechat_buffer_new (category, name,
                                     (new_script_callback_input) ?
                                     input_callback : NULL,
                                     (new_script_callback_input) ?
                                     function_input : NULL,
                                     (new_script_callback_close) ?
                                     close_callback : NULL,
                                     (new_script_callback_close) ?
                                     function_close : NULL);
    if (!new_buffer)
    {
        if (new_script_callback_input)
            free (new_script_callback_input);
        if (new_script_callback_close)
            free (new_script_callback_close);
        return NULL;
    }

    if (new_script_callback_input)
    {
        new_script_callback_input->script = script;
        new_script_callback_input->function = strdup (function_input);
        new_script_callback_input->buffer = new_buffer;
        script_callback_add (script, new_script_callback_input);
    }

    if (new_script_callback_close)
    {
        new_script_callback_close->script = script;
        new_script_callback_close->function = strdup (function_close);
        new_script_callback_close->buffer = new_buffer;
        script_callback_add (script, new_script_callback_close);
    }
    
    return new_buffer;
}

/*
 * script_api_buffer_close: close a buffer
 */

void
script_api_buffer_close (struct t_weechat_plugin *weechat_plugin,
                         struct t_plugin_script *script,
                         struct t_gui_buffer *buffer,
                         int switch_to_another)
{
    struct t_script_callback *ptr_script_callback;
    
    if (!weechat_plugin || !script || !buffer)
        return;
    
    weechat_buffer_close (buffer, switch_to_another);
    
    for (ptr_script_callback = script->callbacks; ptr_script_callback;
         ptr_script_callback = ptr_script_callback->next_callback)
    {
        if (ptr_script_callback->buffer == buffer)
            break;
    }
    
    if (ptr_script_callback)
    {
        script_callback_remove (script, ptr_script_callback);
    }
}

/*
 * script_api_bar_item_new: add a new bar item
 */

struct t_gui_bar_item *
script_api_bar_item_new (struct t_weechat_plugin *weechat_plugin,
                         struct t_plugin_script *script,
                         char *name,
                         char *(*build_callback)(void *data,
                                                 struct t_gui_bar_item *item,
                                                 struct t_gui_window *window,
                                                 int max_width,
                                                 int max_height),
                         char *function_build)
{
    struct t_script_callback *new_script_callback;
    struct t_gui_bar_item *new_item;
    
    new_script_callback = script_callback_alloc ();
    if (!new_script_callback)
        return NULL;
    
    new_item = weechat_bar_item_new (name,
                                     (function_build && function_build[0]) ?
                                     build_callback : NULL,
                                     (function_build && function_build[0]) ?
                                     new_script_callback : NULL);
    if (!new_item)
    {
        free (new_script_callback);
        return NULL;
    }
    
    new_script_callback->script = script;
    new_script_callback->function = (function_build && function_build[0]) ?
        strdup (function_build) : NULL;
    new_script_callback->bar_item = new_item;
    script_callback_add (script, new_script_callback);
    
    return new_item;
}

/*
 * script_api_bar_item_remove: remove a bar item
 */

void
script_api_bar_item_remove (struct t_weechat_plugin *weechat_plugin,
                            struct t_plugin_script *script,
                            struct t_gui_bar_item *item)
{
    struct t_script_callback *ptr_script_callback;
    
    if (!weechat_plugin || !script || !item)
        return;
    
    weechat_bar_item_remove (item);
    
    for (ptr_script_callback = script->callbacks; ptr_script_callback;
         ptr_script_callback = ptr_script_callback->next_callback)
    {
        if (ptr_script_callback->bar_item == item)
            script_callback_remove (script, ptr_script_callback);
    }
}

/*
 * script_api_command: execute a command (simulate user entry)
 */

void
script_api_command (struct t_weechat_plugin *weechat_plugin,
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
 * script_api_config_get_plugin: get a value of a script option
 *                               format in file is: plugin.script.option=value
 */

char *
script_api_config_get_plugin (struct t_weechat_plugin *weechat_plugin,
                              struct t_plugin_script *script,
                              char *option)
{
    char *option_fullname, *return_value;
    
    option_fullname = (char *)malloc ((strlen (script->name) +
                                       strlen (option) + 2) * sizeof (char));
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
 * script_api_config_set_plugin: set value of a script config option
 *                               format in file is: plugin.script.option=value
 */

int
script_api_config_set_plugin (struct t_weechat_plugin *weechat_plugin,
                              struct t_plugin_script *script,
                              char *option, char *value)
{
    char *option_fullname;
    int return_code;
    
    option_fullname = (char *)malloc ((strlen (script->name) +
                                       strlen (option) + 2) * sizeof (char));
    if (!option_fullname)
        return 0;
    
    strcpy (option_fullname, script->name);
    strcat (option_fullname, ".");
    strcat (option_fullname, option);
    
    return_code = weechat_config_set_plugin (option_fullname, value);
    free (option_fullname);
    
    return return_code;
}
