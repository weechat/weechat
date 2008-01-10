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
 *                          return 1 if ok, 0 if error
 */

int
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
        return 0;
    
    new_script_callback->script = NULL;
    new_script_callback->function = NULL;
    new_script_callback->hook = NULL;
    
    new_hook = weechat_hook_command (command, description, args,
                                     args_description, completion,
                                     callback, new_script_callback);
    if (!new_hook)
    {
        free (new_script_callback);
        return 0;
    }
    
    new_script_callback->script = script;
    new_script_callback->function = strdup (function);
    new_script_callback->hook = new_hook;
    
    script_callback_add (script, new_script_callback);
    
    return 1;
}

/*
 * script_api_hook_timer: hook a timer
 *                        return 1 if ok, 0 if error
 */

int
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
        return 0;
    
    new_hook = weechat_hook_timer (interval, align_second, max_calls,
                                   callback, new_script_callback);
    if (!new_hook)
    {
        free (new_script_callback);
        return 0;
    }
    
    new_script_callback->script = script;
    new_script_callback->function = strdup (function);
    new_script_callback->hook = new_hook;

    script_callback_add (script, new_script_callback);
    
    return 1;
}

/*
 * script_api_hook_fd: hook a fd
 *                     return 1 if ok, 0 if error
 */

int
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
        return 0;
    
    new_hook = weechat_hook_fd (fd, flag_read, flag_write, flag_exception,
                                callback, new_script_callback);
    if (!new_hook)
    {
        free (new_script_callback);
        return 0;
    }
    
    new_script_callback->script = script;
    new_script_callback->function = strdup (function);
    new_script_callback->hook = new_hook;
    
    script_callback_add (script, new_script_callback);
    
    return 1;
}

/*
 * script_api_hook_print: hook a print
 *                        return 1 if ok, 0 if error
 */

int
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
        return 0;
    
    new_hook = weechat_hook_print (buffer, message, strip_colors,
                                   callback, new_script_callback);
    if (!new_hook)
    {
        free (new_script_callback);
        return 0;
    }
    
    new_script_callback->script = script;
    new_script_callback->function = strdup (function);
    new_script_callback->hook = new_hook;
    
    script_callback_add (script, new_script_callback);
    
    return 1;
}

/*
 * script_api_hook_signal: hook a signal
 *                         return 1 if ok, 0 if error
 */

int
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
        return 0;
    
    new_hook = weechat_hook_signal (signal, callback, new_script_callback);
    if (!new_hook)
    {
        free (new_script_callback);
        return 0;
    }
    
    new_script_callback->script = script;
    new_script_callback->function = strdup (function);
    new_script_callback->hook = new_hook;
    
    script_callback_add (script, new_script_callback);
    
    return 1;
}

/*
 * script_api_hook_config: hook a config option
 *                         return 1 if ok, 0 if error
 */

int
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
        return 0;
    
    new_hook = weechat_hook_config (type, option, callback, new_script_callback);
    if (!new_hook)
    {
        free (new_script_callback);
        return 0;
    }
    
    new_script_callback->script = script;
    new_script_callback->function = strdup (function);
    new_script_callback->hook = new_hook;
    
    script_callback_add (script, new_script_callback);
    
    return 1;
}

/*
 * script_api_hook_completion: hook a completion
 *                             return 1 if ok, 0 if error
 */

int
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
        return 0;
    
    new_hook = weechat_hook_completion (completion, callback, new_script_callback);
    if (!new_hook)
    {
        free (new_script_callback);
        return 0;
    }
    
    new_script_callback->script = script;
    new_script_callback->function = strdup (function);
    new_script_callback->hook = new_hook;
    
    script_callback_add (script, new_script_callback);
    
    return 1;
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
    
    for (ptr_script_callback = script->callbacks; ptr_script_callback;
         ptr_script_callback = ptr_script_callback->next_callback)
    {
        if (ptr_script_callback->hook == hook)
            break;
    }
    
    if (ptr_script_callback)
    {
        script_callback_remove (weechat_plugin, script, ptr_script_callback);
    }
}

/*
 * script_api_unhook_all: remove all hooks from a script
 */

void
script_api_unhook_all (struct t_weechat_plugin *weechat_plugin,
                       struct t_plugin_script *script)
{
    struct t_script_callback *ptr_callback, *next_callback;
    
    ptr_callback = script->callbacks;
    while (ptr_callback)
    {
        next_callback = ptr_callback->next_callback;
        
        script_callback_remove (weechat_plugin, script, ptr_callback);
        
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
                       int (*callback)(void *data,
                                       struct t_gui_buffer *buffer,
                                       char *input_data),
                       char *function)
{
    struct t_script_callback *new_script_callback;
    struct t_gui_buffer *new_buffer;
    
    new_script_callback = script_callback_alloc ();
    if (!new_script_callback)
        return NULL;
    
    new_buffer = weechat_buffer_new (category, name,
                                     callback, new_script_callback);
    if (!new_buffer)
    {
        free (new_script_callback);
        return NULL;
    }
    
    new_script_callback->script = script;
    new_script_callback->function = strdup (function);
    new_script_callback->buffer = new_buffer;
    
    script_callback_add (script, new_script_callback);
    
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
        script_callback_remove (weechat_plugin, script, ptr_script_callback);
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
