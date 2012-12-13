/*
 * Copyright (C) 2003-2012 Sebastien Helleu <flashcode@flashtux.org>
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
 * script-api.c: script API functions, used by script plugins (perl/python/..)
 */

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "weechat-plugin.h"
#include "plugin-script.h"
#include "plugin-script-api.h"
#include "plugin-script-callback.h"


/*
 * Sets charset for script.
 */

void
plugin_script_api_charset_set (struct t_plugin_script *script,
                               const char *charset)
{
    if (script->charset)
        free (script->charset);

    script->charset = (charset) ? strdup (charset) : NULL;
}

/*
 * Creates a new configuration file.
 *
 * Returns pointer to new configuration file, NULL if error.
 */

struct t_config_file *
plugin_script_api_config_new (struct t_weechat_plugin *weechat_plugin,
                              struct t_plugin_script *script,
                              const char *name,
                              int (*callback_reload)(void *data,
                                                     struct t_config_file *config_file),
                              const char *function,
                              const char *data)
{
    struct t_plugin_script_cb *script_cb;
    struct t_config_file *new_config_file;

    script_cb = plugin_script_callback_add (script, function, data);
    if (!script_cb)
        return NULL;

    new_config_file = weechat_config_new (name, callback_reload,
                                          (function && function[0]) ? script_cb : NULL);
    if (new_config_file)
        script_cb->config_file = new_config_file;
    else
        plugin_script_callback_remove (script, script_cb);

    return new_config_file;
}

/*
 * Creates a new section in configuration file.
 *
 * Returns pointer to new section, NULL if error.
 */

struct t_config_section *
plugin_script_api_config_new_section (struct t_weechat_plugin *weechat_plugin,
                                      struct t_plugin_script *script,
                                      struct t_config_file *config_file,
                                      const char *name,
                                      int user_can_add_options,
                                      int user_can_delete_options,
                                      int (*callback_read)(void *data,
                                                           struct t_config_file *config_file,
                                                           struct t_config_section *section,
                                                           const char *option_name,
                                                           const char *value),
                                      const char *function_read,
                                      const char *data_read,
                                      int (*callback_write)(void *data,
                                                            struct t_config_file *config_file,
                                                            const char *section_name),
                                      const char *function_write,
                                      const char *data_write,
                                      int (*callback_write_default)(void *data,
                                                                    struct t_config_file *config_file,
                                                                    const char *section_name),
                                      const char *function_write_default,
                                      const char *data_write_default,
                                      int (*callback_create_option)(void *data,
                                                                    struct t_config_file *config_file,
                                                                    struct t_config_section *section,
                                                                    const char *option_name,
                                                                    const char *value),
                                      const char *function_create_option,
                                      const char *data_create_option,
                                      int (*callback_delete_option)(void *data,
                                                                    struct t_config_file *config_file,
                                                                    struct t_config_section *section,
                                                                    struct t_config_option *option),
                                      const char *function_delete_option,
                                      const char *data_delete_option)
{
    struct t_plugin_script_cb *script_cb_read, *script_cb_write;
    struct t_plugin_script_cb *script_cb_write_default, *script_cb_create_option;
    struct t_plugin_script_cb *script_cb_delete_option;
    struct t_config_section *new_section;

    script_cb_read = plugin_script_callback_add (script, function_read, data_read);
    script_cb_write = plugin_script_callback_add (script, function_write, data_write);
    script_cb_write_default = plugin_script_callback_add (script, function_write_default, data_write_default);
    script_cb_create_option = plugin_script_callback_add (script, function_create_option, data_create_option);
    script_cb_delete_option = plugin_script_callback_add (script, function_delete_option, data_delete_option);
    if (!script_cb_read || !script_cb_write || !script_cb_write_default
        || !script_cb_create_option || !script_cb_delete_option)
    {
        if (script_cb_read)
            plugin_script_callback_remove (script, script_cb_read);
        if (script_cb_write)
            plugin_script_callback_remove (script, script_cb_write);
        if (script_cb_write_default)
            plugin_script_callback_remove (script, script_cb_write_default);
        if (script_cb_create_option)
            plugin_script_callback_remove (script, script_cb_create_option);
        if (script_cb_delete_option)
            plugin_script_callback_remove (script, script_cb_delete_option);
        return NULL;
    }

    new_section = weechat_config_new_section (config_file,
                                              name,
                                              user_can_add_options,
                                              user_can_delete_options,
                                              (function_read && function_read[0]) ? callback_read : NULL,
                                              (function_read && function_read[0]) ? script_cb_read : NULL,
                                              (function_write && function_write[0]) ? callback_write : NULL,
                                              (function_write && function_write[0]) ? script_cb_write : NULL,
                                              (function_write_default && function_write_default[0]) ? callback_write_default : NULL,
                                              (function_write_default && function_write_default[0]) ? script_cb_write_default : NULL,
                                              (function_create_option && function_create_option[0]) ? callback_create_option : NULL,
                                              (function_create_option && function_create_option[0]) ? script_cb_create_option : NULL,
                                              (function_delete_option && function_delete_option[0]) ? callback_delete_option : NULL,
                                              (function_delete_option && function_delete_option[0]) ? script_cb_delete_option : NULL);
    if (new_section)
    {
        script_cb_read->config_file = config_file;
        script_cb_read->config_section = new_section;
        script_cb_write->config_file = config_file;
        script_cb_write->config_section = new_section;
        script_cb_write_default->config_file = config_file;
        script_cb_write_default->config_section = new_section;
        script_cb_create_option->config_file = config_file;
        script_cb_create_option->config_section = new_section;
        script_cb_delete_option->config_file = config_file;
        script_cb_delete_option->config_section = new_section;
    }
    else
    {
        plugin_script_callback_remove (script, script_cb_read);
        plugin_script_callback_remove (script, script_cb_write);
        plugin_script_callback_remove (script, script_cb_write_default);
        plugin_script_callback_remove (script, script_cb_create_option);
        plugin_script_callback_remove (script, script_cb_delete_option);
    }

    return new_section;
}

/*
 * Creates a new option in section.
 *
 * Returns pointer to new option, NULL if error.
 */

struct t_config_option *
plugin_script_api_config_new_option (struct t_weechat_plugin *weechat_plugin,
                                     struct t_plugin_script *script,
                                     struct t_config_file *config_file,
                                     struct t_config_section *section,
                                     const char *name, const char *type,
                                     const char *description, const char *string_values,
                                     int min, int max,
                                     const char *default_value,
                                     const char *value,
                                     int null_value_allowed,
                                     int (*callback_check_value)(void *data,
                                                                 struct t_config_option *option,
                                                                 const char *value),
                                     const char *function_check_value,
                                     const char *data_check_value,
                                     void (*callback_change)(void *data,
                                                             struct t_config_option *option),
                                     const char *function_change,
                                     const char *data_change,
                                     void (*callback_delete)(void *data,
                                                             struct t_config_option *option),
                                     const char *function_delete,
                                     const char *data_delete)
{
    struct t_plugin_script_cb *script_cb_check_value, *script_cb_change;
    struct t_plugin_script_cb *script_cb_delete;
    struct t_config_option *new_option;

    script_cb_check_value = plugin_script_callback_add (script, function_check_value, data_check_value);
    script_cb_change = plugin_script_callback_add (script, function_change, data_change);
    script_cb_delete = plugin_script_callback_add (script, function_delete, data_delete);
    if (!script_cb_check_value || !script_cb_change || !script_cb_delete)
    {
        if (script_cb_check_value)
            plugin_script_callback_remove (script, script_cb_check_value);
        if (script_cb_change)
            plugin_script_callback_remove (script, script_cb_change);
        if (script_cb_delete)
            plugin_script_callback_remove (script, script_cb_delete);
        return NULL;
    }

    new_option = weechat_config_new_option (config_file, section, name, type,
                                            description, string_values, min,
                                            max, default_value, value,
                                            null_value_allowed,
                                            (function_check_value && function_check_value[0]) ? callback_check_value : NULL,
                                            (function_check_value && function_check_value[0]) ? script_cb_check_value : NULL,
                                            (function_change && function_change[0]) ? callback_change : NULL,
                                            (function_change && function_change[0]) ? script_cb_change : NULL,
                                            (function_delete && function_delete[0]) ? callback_delete : NULL,
                                            (function_delete && function_delete[0]) ? script_cb_delete : NULL);
    if (new_option)
    {
        script_cb_check_value->config_file = config_file;
        script_cb_check_value->config_section = section;
        script_cb_check_value->config_option = new_option;
        script_cb_change->config_file = config_file;
        script_cb_change->config_section = section;
        script_cb_change->config_option = new_option;
        script_cb_delete->config_file = config_file;
        script_cb_delete->config_section = section;
        script_cb_delete->config_option = new_option;
    }
    else
    {
        plugin_script_callback_remove (script, script_cb_check_value);
        plugin_script_callback_remove (script, script_cb_change);
        plugin_script_callback_remove (script, script_cb_delete);
    }

    return new_option;
}

/*
 * Frees an option in configuration file.
 */

void
plugin_script_api_config_option_free (struct t_weechat_plugin *weechat_plugin,
                                      struct t_plugin_script *script,
                                      struct t_config_option *option)
{
    struct t_plugin_script_cb *ptr_script_cb, *next_callback;

    if (!weechat_plugin || !script || !option)
        return;

    weechat_config_option_free (option);

    ptr_script_cb = script->callbacks;
    while (ptr_script_cb)
    {
        next_callback = ptr_script_cb->next_callback;

        if (ptr_script_cb->config_option == option)
            plugin_script_callback_remove (script, ptr_script_cb);

        ptr_script_cb = next_callback;
    }
}

/*
 * Frees all option of a section in configuration file.
 */

void
plugin_script_api_config_section_free_options (struct t_weechat_plugin *weechat_plugin,
                                               struct t_plugin_script *script,
                                               struct t_config_section *section)
{
    struct t_plugin_script_cb *ptr_script_cb, *next_callback;

    if (!weechat_plugin || !script || !section)
        return;

    weechat_config_section_free_options (section);

    ptr_script_cb = script->callbacks;
    while (ptr_script_cb)
    {
        next_callback = ptr_script_cb->next_callback;

        if ((ptr_script_cb->config_section == section)
            && ptr_script_cb->config_option)
            plugin_script_callback_remove (script, ptr_script_cb);

        ptr_script_cb = next_callback;
    }
}

/*
 * Frees a section in configuration file.
 */

void
plugin_script_api_config_section_free (struct t_weechat_plugin *weechat_plugin,
                                       struct t_plugin_script *script,
                                       struct t_config_section *section)
{
    struct t_plugin_script_cb *ptr_script_cb, *next_callback;

    if (!weechat_plugin || !script || !section)
        return;

    weechat_config_section_free (section);

    ptr_script_cb = script->callbacks;
    while (ptr_script_cb)
    {
        next_callback = ptr_script_cb->next_callback;

        if (ptr_script_cb->config_section == section)
            plugin_script_callback_remove (script, ptr_script_cb);

        ptr_script_cb = next_callback;
    }
}

/*
 * Frees a configuration file.
 */

void
plugin_script_api_config_free (struct t_weechat_plugin *weechat_plugin,
                               struct t_plugin_script *script,
                               struct t_config_file *config_file)
{
    struct t_plugin_script_cb *ptr_script_cb, *next_callback;

    if (!weechat_plugin || !script || !config_file)
        return;

    weechat_config_free (config_file);

    ptr_script_cb = script->callbacks;
    while (ptr_script_cb)
    {
        next_callback = ptr_script_cb->next_callback;

        if (ptr_script_cb->config_file == config_file)
            plugin_script_callback_remove (script, ptr_script_cb);

        ptr_script_cb = next_callback;
    }
}

/*
 * Prints a message.
 */

void
plugin_script_api_printf (struct t_weechat_plugin *weechat_plugin,
                          struct t_plugin_script *script,
                          struct t_gui_buffer *buffer, const char *format, ...)
{
    char *buf2;

    weechat_va_format (format);
    if (!vbuffer)
        return;

    buf2 = (script && script->charset && script->charset[0]) ?
        weechat_iconv_to_internal (script->charset, vbuffer) : NULL;
    weechat_printf (buffer, "%s", (buf2) ? buf2 : vbuffer);
    if (buf2)
        free (buf2);

    free (vbuffer);
}

/*
 * Prints a message with optional date and tags.
 */

void
plugin_script_api_printf_date_tags (struct t_weechat_plugin *weechat_plugin,
                                    struct t_plugin_script *script,
                                    struct t_gui_buffer *buffer,
                                    time_t date, const char *tags,
                                    const char *format, ...)
{
    char *buf2;

    weechat_va_format (format);
    if (!vbuffer)
        return;

    buf2 = (script->charset && script->charset[0]) ?
        weechat_iconv_to_internal (script->charset, vbuffer) : NULL;
    weechat_printf_date_tags (buffer, date, tags,
                              "%s", (buf2) ? buf2 : vbuffer);
    if (buf2)
        free (buf2);

    free (vbuffer);
}

/*
 * Prints a message on a buffer with free content.
 */

void
plugin_script_api_printf_y (struct t_weechat_plugin *weechat_plugin,
                            struct t_plugin_script *script,
                            struct t_gui_buffer *buffer, int y,
                            const char *format, ...)
{
    char *buf2;

    weechat_va_format (format);
    if (!vbuffer)
        return;

    buf2 = (script->charset && script->charset[0]) ?
        weechat_iconv_to_internal (script->charset, vbuffer) : NULL;
    weechat_printf_y (buffer, y, "%s", (buf2) ? buf2 : vbuffer);
    if (buf2)
        free (buf2);

    free (vbuffer);
}

/*
 * Prints a message in WeeChat log file.
 */

void
plugin_script_api_log_printf (struct t_weechat_plugin *weechat_plugin,
                              struct t_plugin_script *script,
                              const char *format, ...)
{
    char *buf2;

    weechat_va_format (format);
    if (!vbuffer)
        return;

    buf2 = (script->charset && script->charset[0]) ?
        weechat_iconv_to_internal (script->charset, vbuffer) : NULL;
    weechat_log_printf ("%s", (buf2) ? buf2 : vbuffer);
    if (buf2)
        free (buf2);

    free (vbuffer);
}

/*
 * Hooks a command.
 *
 * Returns pointer to new hook, NULL if error.
 */

struct t_hook *
plugin_script_api_hook_command (struct t_weechat_plugin *weechat_plugin,
                                struct t_plugin_script *script,
                                const char *command, const char *description,
                                const char *args, const char *args_description,
                                const char *completion,
                                int (*callback)(void *data,
                                                struct t_gui_buffer *buffer,
                                                int argc, char **argv,
                                                char **argv_eol),
                                const char *function,
                                const char *data)
{
    struct t_plugin_script_cb *script_cb;
    struct t_hook *new_hook;

    script_cb = plugin_script_callback_add (script, function, data);
    if (!script_cb)
        return NULL;

    new_hook = weechat_hook_command (command, description, args,
                                     args_description, completion,
                                     callback, script_cb);
    if (new_hook)
    {
        weechat_hook_set (new_hook, "subplugin", script->name);
        script_cb->hook = new_hook;
    }
    else
        plugin_script_callback_remove (script, script_cb);

    return new_hook;
}

/*
 * Hooks a command when it's run by WeeChat.
 *
 * Returns pointer to new hook, NULL if error.
 */

struct t_hook *
plugin_script_api_hook_command_run (struct t_weechat_plugin *weechat_plugin,
                                    struct t_plugin_script *script,
                                    const char *command,
                                    int (*callback)(void *data,
                                                    struct t_gui_buffer *buffer,
                                                    const char *command),
                                    const char *function,
                                    const char *data)
{
    struct t_plugin_script_cb *script_cb;
    struct t_hook *new_hook;

    script_cb = plugin_script_callback_add (script, function, data);
    if (!script_cb)
        return NULL;

    new_hook = weechat_hook_command_run (command,
                                         callback, script_cb);
    if (new_hook)
    {
        weechat_hook_set (new_hook, "subplugin", script->name);
        script_cb->hook = new_hook;
    }
    else
        plugin_script_callback_remove (script, script_cb);

    return new_hook;
}

/*
 * Hooks a timer.
 *
 * Returns pointer to new hook, NULL if error.
 */

struct t_hook *
plugin_script_api_hook_timer (struct t_weechat_plugin *weechat_plugin,
                              struct t_plugin_script *script,
                              int interval, int align_second, int max_calls,
                              int (*callback)(void *data,
                                              int remaining_calls),
                              const char *function,
                              const char *data)
{
    struct t_plugin_script_cb *script_cb;
    struct t_hook *new_hook;

    script_cb = plugin_script_callback_add (script, function, data);
    if (!script_cb)
        return NULL;

    new_hook = weechat_hook_timer (interval, align_second, max_calls,
                                   callback, script_cb);
    if (new_hook)
    {
        weechat_hook_set (new_hook, "subplugin", script->name);
        script_cb->hook = new_hook;
    }
    else
        plugin_script_callback_remove (script, script_cb);

    return new_hook;
}

/*
 * Hooks a fd event.
 *
 * Returns pointer to new hook, NULL if error.
 */

struct t_hook *
plugin_script_api_hook_fd (struct t_weechat_plugin *weechat_plugin,
                           struct t_plugin_script *script,
                           int fd, int flag_read, int flag_write,
                           int flag_exception,
                           int (*callback)(void *data, int fd),
                           const char *function,
                           const char *data)
{
    struct t_plugin_script_cb *script_cb;
    struct t_hook *new_hook;

    script_cb = plugin_script_callback_add (script, function, data);
    if (!script_cb)
        return NULL;

    new_hook = weechat_hook_fd (fd, flag_read, flag_write, flag_exception,
                                callback, script_cb);
    if (new_hook)
    {
        weechat_hook_set (new_hook, "subplugin", script->name);
        script_cb->hook = new_hook;
    }
    else
        plugin_script_callback_remove (script, script_cb);

    return new_hook;
}

/*
 * Hooks a process (using fork) with options in hashtable.
 *
 * Returns pointer to new hook, NULL if error.
 */

struct t_hook *
plugin_script_api_hook_process_hashtable (struct t_weechat_plugin *weechat_plugin,
                                          struct t_plugin_script *script,
                                          const char *command,
                                          struct t_hashtable *options,
                                          int timeout,
                                          int (*callback)(void *data,
                                                          const char *command,
                                                          int return_code,
                                                          const char *out,
                                                          const char *err),
                                          const char *function,
                                          const char *data)
{
    struct t_plugin_script_cb *script_cb;
    struct t_hook *new_hook;

    script_cb = plugin_script_callback_add (script, function, data);
    if (!script_cb)
        return NULL;

    new_hook = weechat_hook_process_hashtable (command, options, timeout,
                                               callback, script_cb);
    if (new_hook)
    {
        weechat_hook_set (new_hook, "subplugin", script->name);
        script_cb->hook = new_hook;
    }
    else
        plugin_script_callback_remove (script, script_cb);

    return new_hook;
}

/*
 * Hooks a process (using fork).
 *
 * Returns pointer to new hook, NULL if error.
 */

struct t_hook *
plugin_script_api_hook_process (struct t_weechat_plugin *weechat_plugin,
                                struct t_plugin_script *script,
                                const char *command,
                                int timeout,
                                int (*callback)(void *data,
                                                const char *command,
                                                int return_code,
                                                const char *out,
                                                const char *err),
                                const char *function,
                                const char *data)
{
    return plugin_script_api_hook_process_hashtable (weechat_plugin, script,
                                                     command, NULL, timeout,
                                                     callback, function, data);
}

/*
 * Hooks a connection to a peer (using fork).
 *
 * Returns pointer to new hook, NULL if error.
 */

struct t_hook *
plugin_script_api_hook_connect (struct t_weechat_plugin *weechat_plugin,
                                struct t_plugin_script *script,
                                const char *proxy, const char *address, int port,
                                int ipv6, int retry,
                                void *gnutls_sess, void *gnutls_cb,
                                int gnutls_dhkey_size,
                                const char *gnutls_priorities,
                                const char *local_hostname,
                                int (*callback)(void *data, int status,
                                                int gnutls_rc, int sock,
                                                const char *error,
                                                const char *ip_address),
                                const char *function,
                                const char *data)
{
    struct t_plugin_script_cb *script_cb;
    struct t_hook *new_hook;

    script_cb = plugin_script_callback_add (script, function, data);
    if (!script_cb)
        return NULL;

    new_hook = weechat_hook_connect (proxy, address, port, ipv6, retry,
                                     gnutls_sess, gnutls_cb, gnutls_dhkey_size,
                                     gnutls_priorities, local_hostname,
                                     callback, script_cb);
    if (new_hook)
    {
        weechat_hook_set (new_hook, "subplugin", script->name);
        script_cb->hook = new_hook;
    }
    else
        plugin_script_callback_remove (script, script_cb);

    return new_hook;
}

/*
 * Hooks a message printed by WeeChat.
 *
 * Returns pointer to new hook, NULL if error.
 */

struct t_hook *
plugin_script_api_hook_print (struct t_weechat_plugin *weechat_plugin,
                              struct t_plugin_script *script,
                              struct t_gui_buffer *buffer,
                              const char *tags, const char *message,
                              int strip_colors,
                              int (*callback)(void *data,
                                              struct t_gui_buffer *buffer,
                                              time_t date,
                                              int tags_count, const char **tags,
                                              int displayed, int highlight,
                                              const char *prefix,
                                              const char *message),
                              const char *function,
                              const char *data)
{
    struct t_plugin_script_cb *script_cb;
    struct t_hook *new_hook;

    script_cb = plugin_script_callback_add (script, function, data);
    if (!script_cb)
        return NULL;

    new_hook = weechat_hook_print (buffer, tags, message, strip_colors,
                                   callback, script_cb);
    if (new_hook)
    {
        weechat_hook_set (new_hook, "subplugin", script->name);
        script_cb->hook = new_hook;
    }
    else
        plugin_script_callback_remove (script, script_cb);

    return new_hook;
}

/*
 * Hooks a signal.
 *
 * Returns pointer to new hook, NULL if error.
 */

struct t_hook *
plugin_script_api_hook_signal (struct t_weechat_plugin *weechat_plugin,
                               struct t_plugin_script *script,
                               const char *signal,
                               int (*callback)(void *data, const char *signal,
                                               const char *type_data,
                                               void *signal_data),
                               const char *function,
                               const char *data)
{
    struct t_plugin_script_cb *script_cb;
    struct t_hook *new_hook;

    script_cb = plugin_script_callback_add (script, function, data);
    if (!script_cb)
        return NULL;

    new_hook = weechat_hook_signal (signal, callback, script_cb);
    if (new_hook)
    {
        weechat_hook_set (new_hook, "subplugin", script->name);
        script_cb->hook = new_hook;
    }
    else
        plugin_script_callback_remove (script, script_cb);

    return new_hook;
}

/*
 * Hooks a hsignal (signal with hashtable).
 *
 * Returns pointer to new hook, NULL if error.
 */

struct t_hook *
plugin_script_api_hook_hsignal (struct t_weechat_plugin *weechat_plugin,
                                struct t_plugin_script *script,
                                const char *signal,
                                int (*callback)(void *data, const char *signal,
                                                struct t_hashtable *hashtable),
                                const char *function,
                                const char *data)
{
    struct t_plugin_script_cb *script_cb;
    struct t_hook *new_hook;

    script_cb = plugin_script_callback_add (script, function, data);
    if (!script_cb)
        return NULL;

    new_hook = weechat_hook_hsignal (signal, callback, script_cb);
    if (new_hook)
    {
        weechat_hook_set (new_hook, "subplugin", script->name);
        script_cb->hook = new_hook;
    }
    else
        plugin_script_callback_remove (script, script_cb);

    return new_hook;
}

/*
 * Hooks a configuration option.
 *
 * Returns pointer to new hook, NULL if error.
 */

struct t_hook *
plugin_script_api_hook_config (struct t_weechat_plugin *weechat_plugin,
                               struct t_plugin_script *script,
                               const char *option,
                               int (*callback)(void *data, const char *option,
                                               const char *value),
                               const char *function,
                               const char *data)
{
    struct t_plugin_script_cb *script_cb;
    struct t_hook *new_hook;

    script_cb = plugin_script_callback_add (script, function, data);
    if (!script_cb)
        return NULL;

    new_hook = weechat_hook_config (option, callback, script_cb);
    if (new_hook)
    {
        weechat_hook_set (new_hook, "subplugin", script->name);
        script_cb->hook = new_hook;
    }
    else
        plugin_script_callback_remove (script, script_cb);

    return new_hook;
}

/*
 * Hooks a completion.
 *
 * Returns pointer to new hook, NULL if error.
 */

struct t_hook *
plugin_script_api_hook_completion (struct t_weechat_plugin *weechat_plugin,
                                   struct t_plugin_script *script,
                                   const char *completion,
                                   const char *description,
                                   int (*callback)(void *data,
                                                   const char *completion_item,
                                                   struct t_gui_buffer *buffer,
                                                   struct t_gui_completion *completion),
                                   const char *function,
                                   const char *data)
{
    struct t_plugin_script_cb *script_cb;
    struct t_hook *new_hook;

    script_cb = plugin_script_callback_add (script, function, data);
    if (!script_cb)
        return NULL;

    new_hook = weechat_hook_completion (completion, description,
                                        callback, script_cb);
    if (new_hook)
    {
        weechat_hook_set (new_hook, "subplugin", script->name);
        script_cb->hook = new_hook;
    }
    else
        plugin_script_callback_remove (script, script_cb);

    return new_hook;
}

/*
 * Hooks a modifier.
 *
 * Returns pointer to new hook, NULL if error.
 */

struct t_hook *
plugin_script_api_hook_modifier (struct t_weechat_plugin *weechat_plugin,
                                 struct t_plugin_script *script,
                                 const char *modifier,
                                 char *(*callback)(void *data, const char *modifier,
                                                   const char *modifier_data,
                                                   const char *string),
                                 const char *function,
                                 const char *data)
{
    struct t_plugin_script_cb *script_cb;
    struct t_hook *new_hook;

    script_cb = plugin_script_callback_add (script, function, data);
    if (!script_cb)
        return NULL;

    new_hook = weechat_hook_modifier (modifier, callback, script_cb);
    if (new_hook)
    {
        weechat_hook_set (new_hook, "subplugin", script->name);
        script_cb->hook = new_hook;
    }
    else
        plugin_script_callback_remove (script, script_cb);

    return new_hook;
}

/*
 * Hooks an info.
 *
 * Returns pointer to new hook, NULL if error.
 */

struct t_hook *
plugin_script_api_hook_info (struct t_weechat_plugin *weechat_plugin,
                             struct t_plugin_script *script,
                             const char *info_name,
                             const char *description,
                             const char *args_description,
                             const char *(*callback)(void *data,
                                                     const char *info_name,
                                                     const char *arguments),
                             const char *function,
                             const char *data)
{
    struct t_plugin_script_cb *script_cb;
    struct t_hook *new_hook;

    script_cb = plugin_script_callback_add (script, function, data);
    if (!script_cb)
        return NULL;

    new_hook = weechat_hook_info (info_name, description, args_description,
                                  callback, script_cb);
    if (new_hook)
    {
        weechat_hook_set (new_hook, "subplugin", script->name);
        script_cb->hook = new_hook;
    }
    else
        plugin_script_callback_remove (script, script_cb);

    return new_hook;
}

/*
 * Hooks an info using hashtable.
 *
 * Returns pointer to new hook, NULL if error.
 */

struct t_hook *
plugin_script_api_hook_info_hashtable (struct t_weechat_plugin *weechat_plugin,
                                       struct t_plugin_script *script,
                                       const char *info_name,
                                       const char *description,
                                       const char *args_description,
                                       const char *output_description,
                                       struct t_hashtable *(*callback)(void *data,
                                                                       const char *info_name,
                                                                       struct t_hashtable *hashtable),
                                       const char *function,
                                       const char *data)
{
    struct t_plugin_script_cb *script_cb;
    struct t_hook *new_hook;

    script_cb = plugin_script_callback_add (script, function, data);
    if (!script_cb)
        return NULL;

    new_hook = weechat_hook_info_hashtable (info_name, description,
                                            args_description,
                                            output_description,
                                            callback, script_cb);
    if (new_hook)
    {
        weechat_hook_set (new_hook, "subplugin", script->name);
        script_cb->hook = new_hook;
    }
    else
        plugin_script_callback_remove (script, script_cb);

    return new_hook;
}

/*
 * Hooks an infolist.
 *
 * Returns pointer to new hook, NULL if error.
 */

struct t_hook *
plugin_script_api_hook_infolist (struct t_weechat_plugin *weechat_plugin,
                                 struct t_plugin_script *script,
                                 const char *infolist_name,
                                 const char *description,
                                 const char *pointer_description,
                                 const char *args_description,
                                 struct t_infolist *(*callback)(void *data,
                                                                const char *infolist_name,
                                                                void *pointer,
                                                                const char *arguments),
                                 const char *function,
                                 const char *data)
{
    struct t_plugin_script_cb *script_cb;
    struct t_hook *new_hook;

    script_cb = plugin_script_callback_add (script, function, data);
    if (!script_cb)
        return NULL;

    new_hook = weechat_hook_infolist (infolist_name, description,
                                      pointer_description, args_description,
                                      callback, script_cb);
    if (new_hook)
    {
        weechat_hook_set (new_hook, "subplugin", script->name);
        script_cb->hook = new_hook;
    }
    else
        plugin_script_callback_remove (script, script_cb);

    return new_hook;
}

/*
 * Hooks a focus.
 *
 * Returns pointer to new hook, NULL if error.
 */

struct t_hook *
plugin_script_api_hook_focus (struct t_weechat_plugin *weechat_plugin,
                              struct t_plugin_script *script,
                              const char *area,
                              struct t_hashtable *(*callback)(void *data,
                                                              struct t_hashtable *info),
                              const char *function,
                              const char *data)
{
    struct t_plugin_script_cb *script_cb;
    struct t_hook *new_hook;

    script_cb = plugin_script_callback_add (script, function, data);
    if (!script_cb)
        return NULL;

    new_hook = weechat_hook_focus (area, callback, script_cb);
    if (new_hook)
    {
        weechat_hook_set (new_hook, "subplugin", script->name);
        script_cb->hook = new_hook;
    }
    else
        plugin_script_callback_remove (script, script_cb);

    return new_hook;
}

/*
 * Unhooks something.
 */

void
plugin_script_api_unhook (struct t_weechat_plugin *weechat_plugin,
                          struct t_plugin_script *script,
                          struct t_hook *hook)
{
    struct t_plugin_script_cb *ptr_script_cb, *next_callback;

    if (!weechat_plugin || !script || !hook)
        return;

    weechat_unhook (hook);

    ptr_script_cb = script->callbacks;
    while (ptr_script_cb)
    {
        next_callback = ptr_script_cb->next_callback;

        if (ptr_script_cb->hook == hook)
            plugin_script_callback_remove (script, ptr_script_cb);

        ptr_script_cb = next_callback;
    }
}

/*
 * Unhooks everything for a script.
 */

void
plugin_script_api_unhook_all (struct t_weechat_plugin *weechat_plugin,
                              struct t_plugin_script *script)
{
    struct t_plugin_script_cb *ptr_script_cb, *next_callback;

    ptr_script_cb = script->callbacks;
    while (ptr_script_cb)
    {
        next_callback = ptr_script_cb->next_callback;

        if (ptr_script_cb->hook)
        {
            weechat_unhook (ptr_script_cb->hook);
            plugin_script_callback_remove (script, ptr_script_cb);
        }

        ptr_script_cb = next_callback;
    }
}

/*
 * Creates a new buffer.
 */

struct t_gui_buffer *
plugin_script_api_buffer_new (struct t_weechat_plugin *weechat_plugin,
                              struct t_plugin_script *script,
                              const char *name,
                              int (*input_callback)(void *data,
                                                    struct t_gui_buffer *buffer,
                                                    const char *input_data),
                              const char *function_input,
                              const char *data_input,
                              int (*close_callback)(void *data,
                                                    struct t_gui_buffer *buffer),
                              const char *function_close,
                              const char *data_close)
{
    struct t_plugin_script_cb *script_cb_input;
    struct t_plugin_script_cb *script_cb_close;
    struct t_gui_buffer *new_buffer;

    script_cb_input = plugin_script_callback_add (script, function_input, data_input);
    script_cb_close = plugin_script_callback_add (script, function_close, data_close);
    if (!script_cb_input || !script_cb_close)
    {
        if (script_cb_input)
            plugin_script_callback_remove (script, script_cb_input);
        if (script_cb_close)
            plugin_script_callback_remove (script, script_cb_close);
        return NULL;
    }

    new_buffer = weechat_buffer_new (name,
                                     (function_input && function_input[0]) ? input_callback : NULL,
                                     (function_input && function_input[0]) ? script_cb_input : NULL,
                                     (function_close && function_close[0]) ? close_callback : NULL,
                                     (function_close && function_close[0]) ? script_cb_close : NULL);
    if (new_buffer)
    {
        script_cb_input->buffer = new_buffer;
        script_cb_close->buffer = new_buffer;

        /* used when upgrading weechat, to set callbacks */
        weechat_buffer_set (new_buffer, "localvar_set_script_name", script->name);
        weechat_buffer_set (new_buffer, "localvar_set_script_input_cb", function_input);
        weechat_buffer_set (new_buffer, "localvar_set_script_input_cb_data", data_input);
        weechat_buffer_set (new_buffer, "localvar_set_script_close_cb", function_close);
        weechat_buffer_set (new_buffer, "localvar_set_script_close_cb_data", data_close);
    }
    else
    {
        plugin_script_callback_remove (script, script_cb_input);
        plugin_script_callback_remove (script, script_cb_close);
    }

    return new_buffer;
}

/*
 * Closes a buffer.
 */

void
plugin_script_api_buffer_close (struct t_weechat_plugin *weechat_plugin,
                                struct t_plugin_script *script,
                                struct t_gui_buffer *buffer)
{
    struct t_plugin_script_cb *ptr_script_cb, *next_callback;

    if (!weechat_plugin || !script || !buffer)
        return;

    weechat_buffer_close (buffer);

    ptr_script_cb = script->callbacks;
    while (ptr_script_cb)
    {
        next_callback = ptr_script_cb->next_callback;

        if (ptr_script_cb->buffer == buffer)
            plugin_script_callback_remove (script, ptr_script_cb);

        ptr_script_cb = next_callback;
    }
}

/*
 * Adds a new bar item.
 *
 * Returns pointer to new bar item, NULL if error.
 */

struct t_gui_bar_item *
plugin_script_api_bar_item_new (struct t_weechat_plugin *weechat_plugin,
                                struct t_plugin_script *script,
                                const char *name,
                                char *(*build_callback)(void *data,
                                                        struct t_gui_bar_item *item,
                                                        struct t_gui_window *window),
                                const char *function,
                                const char *data)
{
    struct t_plugin_script_cb *script_cb;
    struct t_gui_bar_item *new_item;

    script_cb = plugin_script_callback_add (script, function, data);
    if (!script_cb)
        return NULL;

    new_item = weechat_bar_item_new (name,
                                     (function && function[0]) ? build_callback : NULL,
                                     (function && function[0]) ? script_cb : NULL);
    if (new_item)
        script_cb->bar_item = new_item;
    else
        plugin_script_callback_remove (script, script_cb);

    return new_item;
}

/*
 * Removes a bar item.
 */

void
plugin_script_api_bar_item_remove (struct t_weechat_plugin *weechat_plugin,
                                   struct t_plugin_script *script,
                                   struct t_gui_bar_item *item)
{
    struct t_plugin_script_cb *ptr_script_cb, *next_callback;

    if (!weechat_plugin || !script || !item)
        return;

    weechat_bar_item_remove (item);

    ptr_script_cb = script->callbacks;
    while (ptr_script_cb)
    {
        next_callback = ptr_script_cb->next_callback;

        if (ptr_script_cb->bar_item == item)
            plugin_script_callback_remove (script, ptr_script_cb);

        ptr_script_cb = next_callback;
    }
}

/*
 * Executes a command on a buffer (simulates user entry).
 */

void
plugin_script_api_command (struct t_weechat_plugin *weechat_plugin,
                           struct t_plugin_script *script,
                           struct t_gui_buffer *buffer, const char *command)
{
    char *command2;

    command2 = (script->charset && script->charset[0]) ?
        weechat_iconv_to_internal (script->charset, command) : NULL;

    weechat_command (buffer, (command2) ? command2 : command);

    if (command2)
        free (command2);
}

/*
 * Gets value of a script option (format in file is "plugin.script.option").
 */

const char *
plugin_script_api_config_get_plugin (struct t_weechat_plugin *weechat_plugin,
                                     struct t_plugin_script *script,
                                     const char *option)
{
    char *option_fullname;
    const char *return_value;

    option_fullname = malloc ((strlen (script->name) +
                               strlen (option) + 2));
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
 * Checks if a script option is set.
 *
 * Returns:
 *   1: script option is set
 *   0: script option does not exist
 */

int
plugin_script_api_config_is_set_plugin (struct t_weechat_plugin *weechat_plugin,
                                        struct t_plugin_script *script,
                                        const char *option)
{
    char *option_fullname;
    int return_code;

    option_fullname = malloc ((strlen (script->name) +
                               strlen (option) + 2));
    if (!option_fullname)
        return 0;

    strcpy (option_fullname, script->name);
    strcat (option_fullname, ".");
    strcat (option_fullname, option);

    return_code = weechat_config_is_set_plugin (option_fullname);
    free (option_fullname);

    return return_code;
}

/*
 * Sets value of a script option (format in file is "plugin.script.option").
 */

int
plugin_script_api_config_set_plugin (struct t_weechat_plugin *weechat_plugin,
                                     struct t_plugin_script *script,
                                     const char *option, const char *value)
{
    char *option_fullname;
    int return_code;

    option_fullname = malloc ((strlen (script->name) +
                               strlen (option) + 2));
    if (!option_fullname)
        return 0;

    strcpy (option_fullname, script->name);
    strcat (option_fullname, ".");
    strcat (option_fullname, option);

    return_code = weechat_config_set_plugin (option_fullname, value);
    free (option_fullname);

    return return_code;
}

/*
 * Sets description of a script option.
 */

void
plugin_script_api_config_set_desc_plugin (struct t_weechat_plugin *weechat_plugin,
                                          struct t_plugin_script *script,
                                          const char *option, const char *description)
{
    char *option_fullname;

    option_fullname = malloc ((strlen (script->name) +
                               strlen (option) + 2));
    if (!option_fullname)
        return;

    strcpy (option_fullname, script->name);
    strcat (option_fullname, ".");
    strcat (option_fullname, option);

    weechat_config_set_desc_plugin (option_fullname, description);
    free (option_fullname);
}

/*
 * Unsets a script option.
 */

int
plugin_script_api_config_unset_plugin (struct t_weechat_plugin *weechat_plugin,
                                       struct t_plugin_script *script,
                                       const char *option)
{
    char *option_fullname;
    int return_code;

    option_fullname = malloc ((strlen (script->name) +
                               strlen (option) + 2));
    if (!option_fullname)
        return 0;

    strcpy (option_fullname, script->name);
    strcat (option_fullname, ".");
    strcat (option_fullname, option);

    return_code = weechat_config_unset_plugin (option_fullname);
    free (option_fullname);

    return return_code;
}

/*
 * Reads upgrade file.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
plugin_script_api_upgrade_read (struct t_weechat_plugin *weechat_plugin,
                                struct t_plugin_script *script,
                                struct t_upgrade_file *upgrade_file,
                                int (*callback_read)(void *data,
                                                     struct t_upgrade_file *upgrade_file,
                                                     int object_id,
                                                     struct t_infolist *infolist),
                                const char *function,
                                const char *data)
{
    struct t_plugin_script_cb *script_cb;
    int rc;

    if (!function || !function[0])
        return 0;

    script_cb = plugin_script_callback_add (script, function, data);
    if (!script_cb)
        return 0;
    script_cb->upgrade_file = upgrade_file;

    rc = weechat_upgrade_read (upgrade_file,
                               callback_read,
                               script_cb);

    plugin_script_callback_remove (script, script_cb);

    return rc;
}
