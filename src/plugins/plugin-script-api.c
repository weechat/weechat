/*
 * script-api.c - script API functions, used by script plugins
 *
 * Copyright (C) 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "weechat-plugin.h"
#include "plugin-script.h"
#include "plugin-script-api.h"


/*
 * Sets charset for script.
 */

void
plugin_script_api_charset_set (struct t_plugin_script *script,
                               const char *charset)
{
    if (!script)
        return;

    free (script->charset);
    script->charset = (charset) ? strdup (charset) : NULL;
}

/*
 * Checks if a string matches a list of masks.
 *
 * Returns:
 *   1: string matches list of masks
 *   0: string does not match list of masks
 */

int
plugin_script_api_string_match_list (struct t_weechat_plugin *weechat_plugin,
                                     const char *string, const char *masks,
                                     int case_sensitive)
{
    char **list_masks;
    int match;

    list_masks = (masks && masks[0]) ?
        weechat_string_split (masks, ",", NULL,
                              WEECHAT_STRING_SPLIT_STRIP_LEFT
                              | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                              | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                              0, NULL) : NULL;

    match = weechat_string_match_list (string,
                                       (const char **)list_masks,
                                       case_sensitive);

    weechat_string_free_split (list_masks);

    return match;
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
                              int (*callback_reload)(const void *pointer,
                                                     void *data,
                                                     struct t_config_file *config_file),
                              const char *function,
                              const char *data)
{
    char *function_and_data;
    struct t_config_file *new_config_file;

    if (!script)
        return NULL;

    function_and_data = plugin_script_build_function_and_data (function, data);

    new_config_file = weechat_config_new (
        name,
        (function_and_data) ? callback_reload : NULL,
        script,
        function_and_data);

    if (!new_config_file && function_and_data)
        free (function_and_data);

    return new_config_file;
}

/*
 * Sets configuration file version and a callback to update config
 * sections/options on-the-fly when the config is read.
 *
 * Returns pointer to new configuration file, NULL if error.
 */

int
plugin_script_api_config_set_version (struct t_weechat_plugin *weechat_plugin,
                                      struct t_plugin_script *script,
                                      struct t_config_file *config_file,
                                      int version,
                                      struct t_hashtable *(*callback_update)(const void *pointer,
                                                                             void *data,
                                                                             struct t_config_file *config_file,
                                                                             int version_read,
                                                                             struct t_hashtable *data_read),
                                      const char *function,
                                      const char *data)
{
    char *function_and_data;
    int rc;

    if (!script)
        return 0;

    function_and_data = plugin_script_build_function_and_data (function, data);

    rc = weechat_config_set_version (
        config_file,
        version,
        (function_and_data) ? callback_update : NULL,
        script,
        function_and_data);

    if (!rc && function_and_data)
        free (function_and_data);

    return rc;
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
                                      int (*callback_read)(const void *pointer,
                                                           void *data,
                                                           struct t_config_file *config_file,
                                                           struct t_config_section *section,
                                                           const char *option_name,
                                                           const char *value),
                                      const char *function_read,
                                      const char *data_read,
                                      int (*callback_write)(const void *pointer,
                                                            void *data,
                                                            struct t_config_file *config_file,
                                                            const char *section_name),
                                      const char *function_write,
                                      const char *data_write,
                                      int (*callback_write_default)(const void *pointer,
                                                                    void *data,
                                                                    struct t_config_file *config_file,
                                                                    const char *section_name),
                                      const char *function_write_default,
                                      const char *data_write_default,
                                      int (*callback_create_option)(const void *pointer,
                                                                    void *data,
                                                                    struct t_config_file *config_file,
                                                                    struct t_config_section *section,
                                                                    const char *option_name,
                                                                    const char *value),
                                      const char *function_create_option,
                                      const char *data_create_option,
                                      int (*callback_delete_option)(const void *pointer,
                                                                    void *data,
                                                                    struct t_config_file *config_file,
                                                                    struct t_config_section *section,
                                                                    struct t_config_option *option),
                                      const char *function_delete_option,
                                      const char *data_delete_option)
{
    char *function_and_data_read, *function_and_data_write;
    char *function_and_data_write_default, *function_and_data_create_option;
    char *function_and_data_delete_option;
    struct t_config_section *new_section;

    if (!script)
        return NULL;

    function_and_data_read = plugin_script_build_function_and_data (
        function_read, data_read);
    function_and_data_write = plugin_script_build_function_and_data (
        function_write, data_write);
    function_and_data_write_default = plugin_script_build_function_and_data (
        function_write_default, data_write_default);
    function_and_data_create_option = plugin_script_build_function_and_data (
        function_create_option, data_create_option);
    function_and_data_delete_option = plugin_script_build_function_and_data (
        function_delete_option, data_delete_option);

    new_section = weechat_config_new_section (
        config_file,
        name,
        user_can_add_options,
        user_can_delete_options,
        (function_and_data_read) ? callback_read : NULL,
        script,
        function_and_data_read,
        (function_and_data_write) ? callback_write : NULL,
        script,
        function_and_data_write,
        (function_and_data_write_default) ? callback_write_default : NULL,
        script,
        function_and_data_write_default,
        (function_and_data_create_option) ? callback_create_option : NULL,
        script,
        function_and_data_create_option,
        (function_and_data_delete_option) ? callback_delete_option : NULL,
        script,
        function_and_data_delete_option);;

    if (!new_section)
    {
        free (function_and_data_read);
        free (function_and_data_write);
        free (function_and_data_write_default);
        free (function_and_data_create_option);
        free (function_and_data_delete_option);
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
                                     int (*callback_check_value)(const void *pointer,
                                                                 void *data,
                                                                 struct t_config_option *option,
                                                                 const char *value),
                                     const char *function_check_value,
                                     const char *data_check_value,
                                     void (*callback_change)(const void *pointer,
                                                             void *data,
                                                             struct t_config_option *option),
                                     const char *function_change,
                                     const char *data_change,
                                     void (*callback_delete)(const void *pointer,
                                                             void *data,
                                                             struct t_config_option *option),
                                     const char *function_delete,
                                     const char *data_delete)
{
    char *function_and_data_check_value, *function_and_data_change;
    char *function_and_data_delete;
    struct t_config_option *new_option;

    if (!script)
        return NULL;

    function_and_data_check_value = plugin_script_build_function_and_data (
        function_check_value, data_check_value);
    function_and_data_change = plugin_script_build_function_and_data (
        function_change, data_change);
    function_and_data_delete = plugin_script_build_function_and_data (
        function_delete, data_delete);

    new_option = weechat_config_new_option (
        config_file, section, name, type,
        description, string_values, min,
        max, default_value, value,
        null_value_allowed,
        (function_and_data_check_value) ? callback_check_value : NULL,
        script,
        function_and_data_check_value,
        (function_and_data_change) ? callback_change : NULL,
        script,
        function_and_data_change,
        (function_and_data_delete) ? callback_delete : NULL,
        script,
        function_and_data_delete);

    if (!new_option)
    {
        free (function_and_data_check_value);
        free (function_and_data_change);
        free (function_and_data_delete);
    }

    return new_option;
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
    free (buf2);

    free (vbuffer);
}

/*
 * Prints a message, with optional date and tags.
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

    buf2 = (script && script->charset && script->charset[0]) ?
        weechat_iconv_to_internal (script->charset, vbuffer) : NULL;
    weechat_printf_date_tags (buffer, date, tags,
                              "%s", (buf2) ? buf2 : vbuffer);
    free (buf2);

    free (vbuffer);
}

/*
 * Prints a message, with optional date/time (with microseconds) and tags.
 */

void
plugin_script_api_printf_datetime_tags (struct t_weechat_plugin *weechat_plugin,
                                        struct t_plugin_script *script,
                                        struct t_gui_buffer *buffer,
                                        time_t date, int date_usec,
                                        const char *tags,
                                        const char *format, ...)
{
    char *buf2;

    weechat_va_format (format);
    if (!vbuffer)
        return;

    buf2 = (script && script->charset && script->charset[0]) ?
        weechat_iconv_to_internal (script->charset, vbuffer) : NULL;
    weechat_printf_datetime_tags (buffer, date, date_usec, tags,
                                  "%s", (buf2) ? buf2 : vbuffer);
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

    buf2 = (script && script->charset && script->charset[0]) ?
        weechat_iconv_to_internal (script->charset, vbuffer) : NULL;
    weechat_printf_y (buffer, y, "%s", (buf2) ? buf2 : vbuffer);
    free (buf2);

    free (vbuffer);
}

/*
 * Prints a message on a buffer with free content, with optional date and tags.
 */

void
plugin_script_api_printf_y_date_tags (struct t_weechat_plugin *weechat_plugin,
                                      struct t_plugin_script *script,
                                      struct t_gui_buffer *buffer, int y,
                                      time_t date, const char *tags,
                                      const char *format, ...)
{
    char *buf2;

    weechat_va_format (format);
    if (!vbuffer)
        return;

    buf2 = (script && script->charset && script->charset[0]) ?
        weechat_iconv_to_internal (script->charset, vbuffer) : NULL;
    weechat_printf_y_date_tags (buffer, y, date, tags,
                                "%s", (buf2) ? buf2 : vbuffer);
    free (buf2);

    free (vbuffer);
}

/*
 * Prints a message on a buffer with free content, with optional date/time
 * (with microseconds) and tags.
 */

void
plugin_script_api_printf_y_datetime_tags (struct t_weechat_plugin *weechat_plugin,
                                          struct t_plugin_script *script,
                                          struct t_gui_buffer *buffer, int y,
                                          time_t date, int date_usec,
                                          const char *tags,
                                          const char *format, ...)
{
    char *buf2;

    weechat_va_format (format);
    if (!vbuffer)
        return;

    buf2 = (script && script->charset && script->charset[0]) ?
        weechat_iconv_to_internal (script->charset, vbuffer) : NULL;
    weechat_printf_y_datetime_tags (buffer, y, date, date_usec, tags,
                                    "%s", (buf2) ? buf2 : vbuffer);
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

    buf2 = (script && script->charset && script->charset[0]) ?
        weechat_iconv_to_internal (script->charset, vbuffer) : NULL;
    weechat_log_printf ("%s", (buf2) ? buf2 : vbuffer);
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
                                int (*callback)(const void *pointer,
                                                void *data,
                                                struct t_gui_buffer *buffer,
                                                int argc, char **argv,
                                                char **argv_eol),
                                const char *function,
                                const char *data)
{
    char *function_and_data;
    struct t_hook *new_hook;

    if (!script)
        return NULL;

    function_and_data = plugin_script_build_function_and_data (function, data);

    new_hook = weechat_hook_command (command, description, args,
                                     args_description, completion,
                                     callback, script, function_and_data);

    if (new_hook)
        weechat_hook_set (new_hook, "subplugin", script->name);
    else
        free (function_and_data);

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
                                    int (*callback)(const void *pointer,
                                                    void *data,
                                                    struct t_gui_buffer *buffer,
                                                    const char *command),
                                    const char *function,
                                    const char *data)
{
    char *function_and_data;
    struct t_hook *new_hook;

    if (!script)
        return NULL;

    function_and_data = plugin_script_build_function_and_data (function, data);

    new_hook = weechat_hook_command_run (command,
                                         callback, script, function_and_data);

    if (new_hook)
        weechat_hook_set (new_hook, "subplugin", script->name);
    else
        free (function_and_data);

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
                              long interval, int align_second, int max_calls,
                              int (*callback)(const void *pointer,
                                              void *data,
                                              int remaining_calls),
                              const char *function,
                              const char *data)
{
    char *function_and_data;
    struct t_hook *new_hook;

    if (!script)
        return NULL;

    function_and_data = plugin_script_build_function_and_data (function, data);

    new_hook = weechat_hook_timer (interval, align_second, max_calls,
                                   callback, script, function_and_data);

    if (new_hook)
        weechat_hook_set (new_hook, "subplugin", script->name);
    else
        free (function_and_data);

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
                           int (*callback)(const void *pointer,
                                           void *data,
                                           int fd),
                           const char *function,
                           const char *data)
{
    char *function_and_data;
    struct t_hook *new_hook;

    if (!script)
        return NULL;

    function_and_data = plugin_script_build_function_and_data (function, data);

    new_hook = weechat_hook_fd (fd, flag_read, flag_write, flag_exception,
                                callback, script, function_and_data);

    if (new_hook)
        weechat_hook_set (new_hook, "subplugin", script->name);
    else
        free (function_and_data);

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
                                          int (*callback)(const void *pointer,
                                                          void *data,
                                                          const char *command,
                                                          int return_code,
                                                          const char *out,
                                                          const char *err),
                                          const char *function,
                                          const char *data)
{
    char *function_and_data;
    struct t_hook *new_hook;

    if (!script)
        return NULL;

    function_and_data = plugin_script_build_function_and_data (function, data);

    new_hook = weechat_hook_process_hashtable (command, options, timeout,
                                               callback, script,
                                               function_and_data);

    if (new_hook)
        weechat_hook_set (new_hook, "subplugin", script->name);
    else
        free (function_and_data);

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
                                int (*callback)(const void *pointer,
                                                void *data,
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
 * Hooks a URL.
 *
 * Returns pointer to new hook, NULL if error.
 */

struct t_hook *
plugin_script_api_hook_url (struct t_weechat_plugin *weechat_plugin,
                            struct t_plugin_script *script,
                            const char *url,
                            struct t_hashtable *options,
                            int timeout,
                            int (*callback)(const void *pointer,
                                            void *data,
                                            const char *url,
                                            struct t_hashtable *options,
                                            struct t_hashtable *output),
                            const char *function,
                            const char *data)
{
    char *function_and_data;
    struct t_hook *new_hook;

    if (!script)
        return NULL;

    function_and_data = plugin_script_build_function_and_data (function, data);

    new_hook = weechat_hook_url (url, options, timeout,
                                 callback, script, function_and_data);

    if (new_hook)
        weechat_hook_set (new_hook, "subplugin", script->name);
    else
        free (function_and_data);

    return new_hook;
}

/*
 * Hooks a connection to a peer (using fork).
 *
 * Returns pointer to new hook, NULL if error.
 */

struct t_hook *
plugin_script_api_hook_connect (struct t_weechat_plugin *weechat_plugin,
                                struct t_plugin_script *script,
                                const char *proxy,
                                const char *address, int port,
                                int ipv6, int retry,
                                void *gnutls_sess, void *gnutls_cb,
                                int gnutls_dhkey_size,
                                const char *gnutls_priorities,
                                const char *local_hostname,
                                int (*callback)(const void *pointer,
                                                void *data,
                                                int status, int gnutls_rc,
                                                int sock,
                                                const char *error,
                                                const char *ip_address),
                                const char *function,
                                const char *data)
{
    char *function_and_data;
    struct t_hook *new_hook;

    if (!script)
        return NULL;

    function_and_data = plugin_script_build_function_and_data (function, data);

    new_hook = weechat_hook_connect (proxy, address, port, ipv6, retry,
                                     gnutls_sess, gnutls_cb, gnutls_dhkey_size,
                                     gnutls_priorities, local_hostname,
                                     callback, script, function_and_data);

    if (new_hook)
        weechat_hook_set (new_hook, "subplugin", script->name);
    else
        free (function_and_data);

    return new_hook;
}

/*
 * Hooks a line.
 *
 * Returns pointer to new hook, NULL if error.
 */

struct t_hook *
plugin_script_api_hook_line (struct t_weechat_plugin *weechat_plugin,
                             struct t_plugin_script *script,
                             const char *buffer_type,
                             const char *buffer_name,
                             const char *tags,
                             struct t_hashtable *(*callback)(const void *pointer,
                                                             void *data,
                                                             struct t_hashtable *line),
                             const char *function,
                             const char *data)
{
    char *function_and_data;
    struct t_hook *new_hook;

    if (!script)
        return NULL;

    function_and_data = plugin_script_build_function_and_data (function, data);

    new_hook = weechat_hook_line (buffer_type, buffer_name, tags, callback,
                                  script, function_and_data);

    if (new_hook)
        weechat_hook_set (new_hook, "subplugin", script->name);
    else
        free (function_and_data);

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
                              int (*callback)(const void *pointer,
                                              void *data,
                                              struct t_gui_buffer *buffer,
                                              time_t date,
                                              int date_usec,
                                              int tags_count,
                                              const char **tags,
                                              int displayed, int highlight,
                                              const char *prefix,
                                              const char *message),
                              const char *function,
                              const char *data)
{
    char *function_and_data;
    struct t_hook *new_hook;

    if (!script)
        return NULL;

    function_and_data = plugin_script_build_function_and_data (function, data);

    new_hook = weechat_hook_print (buffer, tags, message, strip_colors,
                                   callback, script, function_and_data);

    if (new_hook)
        weechat_hook_set (new_hook, "subplugin", script->name);
    else
        free (function_and_data);

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
                               int (*callback)(const void *pointer,
                                               void *data,
                                               const char *signal,
                                               const char *type_data,
                                               void *signal_data),
                               const char *function,
                               const char *data)
{
    char *function_and_data;
    struct t_hook *new_hook;

    if (!script)
        return NULL;

    function_and_data = plugin_script_build_function_and_data (function, data);

    new_hook = weechat_hook_signal (signal, callback, script,
                                    function_and_data);

    if (new_hook)
        weechat_hook_set (new_hook, "subplugin", script->name);
    else
        free (function_and_data);

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
                                int (*callback)(const void *pointer,
                                                void *data,
                                                const char *signal,
                                                struct t_hashtable *hashtable),
                                const char *function,
                                const char *data)
{
    char *function_and_data;
    struct t_hook *new_hook;

    if (!script)
        return NULL;

    function_and_data = plugin_script_build_function_and_data (function, data);

    new_hook = weechat_hook_hsignal (signal, callback, script,
                                     function_and_data);

    if (new_hook)
        weechat_hook_set (new_hook, "subplugin", script->name);
    else
        free (function_and_data);

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
                               int (*callback)(const void *pointer,
                                               void *data,
                                               const char *option,
                                               const char *value),
                               const char *function,
                               const char *data)
{
    char *function_and_data;
    struct t_hook *new_hook;

    if (!script)
        return NULL;

    function_and_data = plugin_script_build_function_and_data (function, data);

    new_hook = weechat_hook_config (option, callback, script,
                                    function_and_data);

    if (new_hook)
        weechat_hook_set (new_hook, "subplugin", script->name);
    else
        free (function_and_data);

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
                                   int (*callback)(const void *pointer,
                                                   void *data,
                                                   const char *completion_item,
                                                   struct t_gui_buffer *buffer,
                                                   struct t_gui_completion *completion),
                                   const char *function,
                                   const char *data)
{
    char *function_and_data;
    struct t_hook *new_hook;

    if (!script)
        return NULL;

    function_and_data = plugin_script_build_function_and_data (function, data);

    new_hook = weechat_hook_completion (completion, description,
                                        callback, script, function_and_data);

    if (new_hook)
        weechat_hook_set (new_hook, "subplugin", script->name);
    else
        free (function_and_data);

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
                                 char *(*callback)(const void *pointer,
                                                   void *data,
                                                   const char *modifier,
                                                   const char *modifier_data,
                                                   const char *string),
                                 const char *function,
                                 const char *data)
{
    char *function_and_data;
    struct t_hook *new_hook;

    if (!script)
        return NULL;

    function_and_data = plugin_script_build_function_and_data (function, data);

    new_hook = weechat_hook_modifier (modifier,
                                      callback, script, function_and_data);

    if (new_hook)
        weechat_hook_set (new_hook, "subplugin", script->name);
    else
        free (function_and_data);

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
                             char *(*callback)(const void *pointer,
                                               void *data,
                                               const char *info_name,
                                               const char *arguments),
                             const char *function,
                             const char *data)
{
    char *function_and_data;
    struct t_hook *new_hook;

    if (!script)
        return NULL;

    function_and_data = plugin_script_build_function_and_data (function, data);

    new_hook = weechat_hook_info (info_name, description, args_description,
                                  callback, script, function_and_data);

    if (new_hook)
        weechat_hook_set (new_hook, "subplugin", script->name);
    else
        free (function_and_data);

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
                                       struct t_hashtable *(*callback)(const void *pointer,
                                                                       void *data,
                                                                       const char *info_name,
                                                                       struct t_hashtable *hashtable),
                                       const char *function,
                                       const char *data)
{
    char *function_and_data;
    struct t_hook *new_hook;

    if (!script)
        return NULL;

    function_and_data = plugin_script_build_function_and_data (function, data);

    new_hook = weechat_hook_info_hashtable (info_name, description,
                                            args_description,
                                            output_description,
                                            callback, script,
                                            function_and_data);

    if (new_hook)
        weechat_hook_set (new_hook, "subplugin", script->name);
    else
        free (function_and_data);

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
                                 struct t_infolist *(*callback)(const void *pointer,
                                                                void *data,
                                                                const char *infolist_name,
                                                                void *obj_pointer,
                                                                const char *arguments),
                                 const char *function,
                                 const char *data)
{
    char *function_and_data;
    struct t_hook *new_hook;

    if (!script)
        return NULL;

    function_and_data = plugin_script_build_function_and_data (function, data);

    new_hook = weechat_hook_infolist (infolist_name, description,
                                      pointer_description, args_description,
                                      callback, script, function_and_data);

    if (new_hook)
        weechat_hook_set (new_hook, "subplugin", script->name);
    else
        free (function_and_data);

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
                              struct t_hashtable *(*callback)(const void *pointer,
                                                              void *data,
                                                              struct t_hashtable *info),
                              const char *function,
                              const char *data)
{
    char *function_and_data;
    struct t_hook *new_hook;

    if (!script)
        return NULL;

    function_and_data = plugin_script_build_function_and_data (function, data);

    new_hook = weechat_hook_focus (area, callback, script, function_and_data);

    if (new_hook)
        weechat_hook_set (new_hook, "subplugin", script->name);
    else
        free (function_and_data);

    return new_hook;
}

/*
 * Creates a new buffer with optional properties.
 */

struct t_gui_buffer *
plugin_script_api_buffer_new_props (struct t_weechat_plugin *weechat_plugin,
                                    struct t_plugin_script *script,
                                    const char *name,
                                    struct t_hashtable *properties,
                                    int (*input_callback)(const void *pointer,
                                                          void *data,
                                                          struct t_gui_buffer *buffer,
                                                          const char *input_data),
                                    const char *function_input,
                                    const char *data_input,
                                    int (*close_callback)(const void *pointer,
                                                          void *data,
                                                          struct t_gui_buffer *buffer),
                                    const char *function_close,
                                    const char *data_close)
{
    char *function_and_data_input, *function_and_data_close;
    struct t_gui_buffer *new_buffer;

    if (!script)
        return NULL;

    function_and_data_input = plugin_script_build_function_and_data (
        function_input, data_input);
    function_and_data_close = plugin_script_build_function_and_data (
        function_close, data_close);

    new_buffer = weechat_buffer_new_props (
        name,
        properties,
        (function_and_data_input) ? input_callback : NULL,
        script,
        function_and_data_input,
        (function_and_data_close) ? close_callback : NULL,
        script,
        function_and_data_close);

    if (new_buffer)
    {
        /* used when upgrading weechat, to set callbacks */
        weechat_buffer_set (new_buffer,
                            "localvar_set_script_name", script->name);
        weechat_buffer_set (new_buffer,
                            "localvar_set_script_input_cb", function_input);
        weechat_buffer_set (new_buffer,
                            "localvar_set_script_input_cb_data", data_input);
        weechat_buffer_set (new_buffer,
                            "localvar_set_script_close_cb", function_close);
        weechat_buffer_set (new_buffer,
                            "localvar_set_script_close_cb_data", data_close);
    }
    else
    {
        free (function_and_data_input);
        free (function_and_data_close);
    }

    return new_buffer;
}

/*
 * Creates a new buffer.
 */

struct t_gui_buffer *
plugin_script_api_buffer_new (struct t_weechat_plugin *weechat_plugin,
                              struct t_plugin_script *script,
                              const char *name,
                              int (*input_callback)(const void *pointer,
                                                    void *data,
                                                    struct t_gui_buffer *buffer,
                                                    const char *input_data),
                              const char *function_input,
                              const char *data_input,
                              int (*close_callback)(const void *pointer,
                                                    void *data,
                                                    struct t_gui_buffer *buffer),
                              const char *function_close,
                              const char *data_close)
{
    return plugin_script_api_buffer_new_props (
        weechat_plugin,
        script,
        name,
        NULL,  /* properties */
        input_callback,
        function_input,
        data_input,
        close_callback,
        function_close,
        data_close);
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
                                char *(*build_callback)(const void *pointer,
                                                        void *data,
                                                        struct t_gui_bar_item *item,
                                                        struct t_gui_window *window,
                                                        struct t_gui_buffer *buffer,
                                                        struct t_hashtable *extra_info),
                                const char *function,
                                const char *data)
{
    struct t_gui_bar_item *new_item;
    char str_function[1024], *function_and_data;;
    int new_callback;

    if (!script)
        return NULL;

    new_callback = 0;
    if (strncmp (name, "(extra)", 7) == 0)
    {
        name += 7;
        new_callback = 1;
    }
    str_function[0] = '\0';
    if (function && function[0])
    {
        snprintf (str_function, sizeof (str_function),
                  "%s%s",
                  (new_callback) ? "(extra)" : "",
                  function);
    }

    function_and_data = plugin_script_build_function_and_data (str_function,
                                                               data);

    new_item = weechat_bar_item_new (name, build_callback, script,
                                     function_and_data);

    if (!new_item && function_and_data)
        free (function_and_data);

    return new_item;
}

/*
 * Executes a command on a buffer (simulates user entry) with options.
 */

int
plugin_script_api_command_options (struct t_weechat_plugin *weechat_plugin,
                                   struct t_plugin_script *script,
                                   struct t_gui_buffer *buffer,
                                   const char *command,
                                   struct t_hashtable *options)
{
    char *command2;
    int rc;

    command2 = (script && script->charset && script->charset[0]) ?
        weechat_iconv_to_internal (script->charset, command) : NULL;

    rc = weechat_command_options (buffer,
                                  (command2) ? command2 : command,
                                  options);

    free (command2);

    return rc;
}

/*
 * Executes a command on a buffer (simulates user entry).
 */

int
plugin_script_api_command (struct t_weechat_plugin *weechat_plugin,
                           struct t_plugin_script *script,
                           struct t_gui_buffer *buffer, const char *command)
{
    return plugin_script_api_command_options (weechat_plugin, script, buffer,
                                              command, NULL);
}

/*
 * Builds full name of option: "script.option".
 *
 * Note: result must be freed after use.
 */

char *
plugin_script_api_build_option_full_name (struct t_weechat_plugin *weechat_plugin,
                                          struct t_plugin_script *script,
                                          const char *option)
{
    char *option_full_name;

    weechat_asprintf (&option_full_name, "%s.%s", script->name, option);
    return option_full_name;
}

/*
 * Gets value of a script option (format in file is "plugin.script.option").
 */

const char *
plugin_script_api_config_get_plugin (struct t_weechat_plugin *weechat_plugin,
                                     struct t_plugin_script *script,
                                     const char *option)
{
    char *option_full_name;
    const char *return_value;

    if (!script)
        return NULL;

    option_full_name = plugin_script_api_build_option_full_name (
        weechat_plugin, script, option);
    if (!option_full_name)
        return NULL;

    return_value = weechat_config_get_plugin (option_full_name);

    free (option_full_name);

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
    char *option_full_name;
    int return_code;

    if (!script)
        return 0;

    option_full_name = plugin_script_api_build_option_full_name (
        weechat_plugin, script, option);
    if (!option_full_name)
        return 0;

    return_code = weechat_config_is_set_plugin (option_full_name);

    free (option_full_name);

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
    char *option_full_name;
    int return_code;

    if (!script)
        return 0;

    option_full_name = plugin_script_api_build_option_full_name (
        weechat_plugin, script, option);
    if (!option_full_name)
        return 0;

    return_code = weechat_config_set_plugin (option_full_name, value);

    free (option_full_name);

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
    char *option_full_name;

    if (!script)
        return;

    option_full_name = plugin_script_api_build_option_full_name (
        weechat_plugin, script, option);
    if (!option_full_name)
        return;

    weechat_config_set_desc_plugin (option_full_name, description);

    free (option_full_name);
}

/*
 * Unsets a script option.
 */

int
plugin_script_api_config_unset_plugin (struct t_weechat_plugin *weechat_plugin,
                                       struct t_plugin_script *script,
                                       const char *option)
{
    char *option_full_name;
    int return_code;

    if (!script)
        return 0;

    option_full_name = plugin_script_api_build_option_full_name (
        weechat_plugin, script, option);
    if (!option_full_name)
        return 0;

    return_code = weechat_config_unset_plugin (option_full_name);

    free (option_full_name);

    return return_code;
}

/*
 * Creates an upgrade file.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

struct t_upgrade_file *
plugin_script_api_upgrade_new (struct t_weechat_plugin *weechat_plugin,
                               struct t_plugin_script *script,
                               const char *filename,
                               int (*callback_read)(const void *pointer,
                                       void *data,
                                       struct t_upgrade_file *upgrade_file,
                                       int object_id,
                                       struct t_infolist *infolist),
                               const char *function,
                               const char *data)
{
    char *function_and_data;
    struct t_upgrade_file *new_upgrade_file;

    if (!script)
        return NULL;

    function_and_data = plugin_script_build_function_and_data (function, data);

    new_upgrade_file = weechat_upgrade_new (
        filename,
        (function_and_data) ? callback_read : NULL,
        script,
        function_and_data);

    if (!new_upgrade_file && function_and_data)
        free (function_and_data);

    return new_upgrade_file;
}
