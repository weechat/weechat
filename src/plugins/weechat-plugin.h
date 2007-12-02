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

/* This header is designed to be distributed with WeeChat plugins */

#ifndef __WEECHAT_WEECHAT_PLUGIN_H
#define __WEECHAT_WEECHAT_PLUGIN_H 1

#include <sys/types.h>

/* return codes for init function and handlers */
#define PLUGIN_RC_FAILED             -1  /* function/handler failed         */
#define PLUGIN_RC_SUCCESS             0  /* function/handler ok             */

/* return codes specific to message handlers: messages can be discarded for
   WeeChat, for plugins, or both */
#define PLUGIN_RC_IGNORE_WEECHAT      1  /* ignore WeeChat for this message */
#define PLUGIN_RC_IGNORE_PLUGINS      2  /* ignore other plugins for msg    */
#define PLUGIN_RC_IGNORE_ALL          (PLUGIN_RC_OK_IGNORE_WEECHAT      \
                                       | PLUGIN_RC_OK_IGNORE_PLUGINS)
                                         /* ignore WeeChat and other plugins*/
#define PLUGIN_RC_WITH_HIGHLIGHT      4  /* ok and ask for highlight        */
                                         /* (for message handler only)      */

struct t_weechat_plugin
{
    /* plugin variables */
    char *filename;                    /* name of plugin on disk            */
    void *handle;                      /* handle of plugin (given by dlopen)*/
    char *name;                        /* plugin name                       */
    char *description;                 /* plugin description                */
    char *version;                     /* plugin version                    */
    char *charset;                     /* charset used by plugin            */
    struct t_weechat_plugin *prev_plugin; /* link to previous plugin        */
    struct t_weechat_plugin *next_plugin; /* link to next plugin            */
    
    /* plugin functions (API) */
    
    /* IMPORTANT NOTE for WeeChat developers: always add new API functions
       at the END of functions, for keeping backward compatibility with
       existing plugins */
    
    /* strings */
    void (*charset_set) (struct t_weechat_plugin *, char *);
    char *(*iconv_to_internal) (struct t_weechat_plugin *, char *, char *);
    char *(*iconv_from_internal) (struct t_weechat_plugin *, char *, char *);
    char *(*gettext) (struct t_weechat_plugin *, char *);
    char *(*ngettext) (struct t_weechat_plugin *, char *, char *, int);
    int (*strcasecmp) (struct t_weechat_plugin *, char *, char *);
    int (*strncasecmp) (struct t_weechat_plugin *, char *, char *, int);
    char *(*string_replace) (struct t_weechat_plugin *, char *, char *, char *);
    char **(*string_explode) (struct t_weechat_plugin *, char *, char *, int,
                              int, int *);
    void (*string_free_exploded) (struct t_weechat_plugin *, char **);
    
    /* directories */
    int (*mkdir_home) (struct t_weechat_plugin *, char *);
    void (*exec_on_files) (struct t_weechat_plugin *, char *,
                           int (*)(char *));

    /* config files */
    struct t_config_file *(*config_new) (struct t_weechat_plugin *, char *);
    struct t_config_section *(*config_new_section) (struct t_weechat_plugin *,
                                                    void *, char *,
                                                    void (*)(void *, char *, char *),
                                                    void (*)(void *),
                                                    void (*)(void *));
    struct t_config_option *(*config_new_option) (struct t_weechat_plugin *,
                                                  void *, char *, char *,
                                                  char *, char *, int, int,
                                                  char *, void (*)());
    char (*config_boolean) (struct t_weechat_plugin *, void *);
    int (*config_integer) (struct t_weechat_plugin *, void *);
    char *(*config_string) (struct t_weechat_plugin *, void *);
    int (*config_color) (struct t_weechat_plugin *, void *);
    char *(*config_get) (struct t_weechat_plugin *, char *);
    int (*config_set) (struct t_weechat_plugin *, char *, char *);
    char *(*plugin_config_get) (struct t_weechat_plugin *, char *);
    int (*plugin_config_set) (struct t_weechat_plugin *, char *, char *);

    
    /* display */
    char *(*prefix) (struct t_weechat_plugin *, char *);
    char *(*color) (struct t_weechat_plugin *, char *);
    void (*printf) (struct t_weechat_plugin *, void *, char *, ...);
    void (*printf_date) (struct t_weechat_plugin *, void *, time_t,
                         char *, ...);
    void (*log_printf) (struct t_weechat_plugin *, char *, ...);
    void (*print_infobar) (struct t_weechat_plugin *, int, char *, ...);
    void (*infobar_remove) (struct t_weechat_plugin *, int);
    
    /* hooks */
    struct t_hook *(*hook_command) (struct t_weechat_plugin *, char *, char *,
                                    char *, char *, char *,
                                    int (*)(void *, void *, int, char **, char **),
                                    void *);
    struct t_hook *(*hook_timer) (struct t_weechat_plugin *, long, int,
                                  int (*)(void *), void *);
    struct t_hook *(*hook_fd) (struct t_weechat_plugin *, int, int, int, int,
                               int (*)(void *), void *);
    struct t_hook *(*hook_print) (struct t_weechat_plugin *, void *, char *,
                                  int,
                                  int (*)(void *, void *, time_t, char *, char *),
                                  void *);
    struct t_hook *(*hook_event) (struct t_weechat_plugin *, char *,
                                  int (*)(void *, char *, void *), void *);
    struct t_hook *(*hook_config) (struct t_weechat_plugin *, char *, char *,
                                   int (*)(void *, char *, char *, char *),
                                   void *);
    void (*unhook) (struct t_weechat_plugin *, void *);
    void (*unhook_all) (struct t_weechat_plugin *);
    
    /* buffers */
    struct t_gui_buffer *(*buffer_new) (struct t_weechat_plugin *,
                                        char *, char *,
                                        void (*)(struct t_gui_buffer *, char *));
    struct t_gui_buffer *(*buffer_search) (struct t_weechat_plugin *,
                                           char *, char *);
    void (*buffer_close) (struct t_weechat_plugin *, void *);
    void (*buffer_set) (struct t_weechat_plugin *, void *, char *, char *);
    void (*buffer_nick_add) (struct t_weechat_plugin *, void *, char *, int,
                             char *, char, char *);
    void (*buffer_nick_remove) (struct t_weechat_plugin *, char *);
    
    /* command */
    void (*command) (struct t_weechat_plugin *, void *, char *);
    
    /* infos */
    char *(*info_get) (struct t_weechat_plugin *, char *);
    
    /* lists */
    struct t_plugin_list *(*list_get) (struct t_weechat_plugin *, char *,
                                       void *);
    int (*list_next) (struct t_weechat_plugin *, void *);
    int (*list_prev) (struct t_weechat_plugin *, void *);
    char *(*list_fields) (struct t_weechat_plugin *, void *);
    int (*list_integer) (struct t_weechat_plugin *, void *, char *);
    char *(*list_string) (struct t_weechat_plugin *, void *, char *);
    void *(*list_pointer) (struct t_weechat_plugin *, void *, char *);
    time_t (*list_time) (struct t_weechat_plugin *, void *, char *);
    void (*list_free) (struct t_weechat_plugin *, void *);
    
    /* log */
    void (*log) (struct t_weechat_plugin *, char *, char *, char *, ...);
    
    /* WeeChat developers: ALWAYS add new functions at the end */
};

/* macros for easy call to plugin API */

/* strings */
#define weechat_charset_set(__charset)                          \
    weechat_plugin->charset_set(weechat_plugin, __charset)
#define weechat_iconv_to_internal(__charset, __string)                  \
    weechat_plugin->iconv_to_internal(weechat_plugin,                   \
                                      __charset, __string)
#define weechat_iconv_from_internal(__charset, __string)                \
    weechat_plugin->iconv_from_internal(weechat_plugin,                 \
                                        __charset, __string)
#ifndef __WEECHAT_H
#define _(string) weechat_plugin->gettext(weechat_plugin, string)
#define N_(string) (string)
#define NG_(single,plural,number)                                       \
    weechat_plugin->ngettext(weechat_plugin, single, plural, number)
#endif
#define weechat_strcasecmp(__string1, __string2)                        \
    weechat_plugin->strcasecmp(weechat_plugin, __string1, __string2)
#define weechat_strncasecmp(__string1, __string2, __max)                \
    weechat_plugin->strncasecmp(weechat_plugin, __string1,              \
                                __string2, __max)
#define weechat_string_replace(__string1, __search1, __replace1)        \
    weechat_plugin->string_replace(weechat_plugin, __string1,           \
                                   __search1, __replace1)
#define weechat_string_explode(__string1, __separator, __eol, __max,    \
                               __num_items)                             \
    weechat_plugin->string_explode(weechat_plugin, __string1,           \
                                   __separator, __eol, __max,           \
                                   __num_items)
#define weechat_string_free_exploded(__array_str)                       \
    weechat_plugin->string_free_exploded(weechat_plugin, __array_str)

/* directories */
#define weechat_mkdir_home(__directory)                         \
    weechat_plugin->mkdir_home(weechat_plugin, __directory)
#define weechat_exec_on_files(__directory, __callback)                  \
    weechat_plugin->exec_on_files(weechat_plugin, __directory,          \
                                  __callback)

/* config files */
#define weechat_config_new(__filename)                          \
    weechat_plugin->config_new(weechat_plugin, __filename)
#define weechat_config_new_section(__config, __name, __cb_read,         \
                                   __cb_write_std, __cb_write_def)      \
    weechat_plugin->config_new_section(weechat_plugin,                  \
                                       __config, __name, __cb_read,     \
                                       __cb_write_std, __cb_write_def)
#define weechat_config_new_option(__section, __name, __desc,            \
                                  __string_values, __min, __max,        \
                                  __default, __callback)                \
    weechat_plugin->config_new_option(weechat_plugin,                   \
                                      __section, __name, __desc,        \
                                      __string_values, __min, __max,    \
                                      __default, __callback)
#define weechat_config_boolean(__option)                        \
    weechat_plugin->config_boolean(weechat_plugin, __option)
#define weechat_config_integer(__option)                            \
    weechat_plugin->config_integer(weechat_plugin, __option)
#define weechat_config_string(__option)                         \
    weechat_plugin->config_string(weechat_plugin, __option)
#define weechat_config_color(__option)                          \
    weechat_plugin->config_color(weechat_plugin, __option)
#define weechat_config_get(__option)                            \
    weechat_plugin->config_get(weechat_plugin, __option)
#define weechat_config_set(__option, __value)                           \
    weechat_plugin->config_set(weechat_plugin, __option, __value)
#define weechat_plugin_config_get(__option)                     \
    weechat_plugin->plugin_config_get(weechat_plugin, __option)
#define weechat_plugin_config_set(__option, __value)                    \
    weechat_plugin->plugin_config_set(weechat_plugin, __option, __value)

/* display */
#define weechat_prefix(__prefix_name)                           \
    weechat_plugin->prefix(weechat_plugin, __prefix_name)
#define weechat_color(__color_name)                     \
    weechat_plugin->color(weechat_plugin, __color_name)
#define weechat_printf(__buffer, __argz...)                     \
    weechat_plugin->printf(weechat_plugin, __buffer, ##__argz)
#define weechat_printf_date(__buffer, __datetime, __argz...)            \
    weechat_plugin->printf_date(weechat_plugin, __buffer, __datetime,   \
                                ##__argz)
#define weechat2_log_printf(__argz...)                          \
    weechat_plugin->log_printf(weechat_plugin, ##__argz)

/* hooks */
#define weechat_hook_command(__command, __description, __args,          \
                             __args_desc, __completion, __callback,     \
                             __data)                                    \
    weechat_plugin->hook_command(weechat_plugin, __command, __description, \
                                 __args, __args_desc, __completion,     \
                                 __callback, __data)
#define weechat_hook_timer(__interval, __max_calls, __callback, __data) \
    weechat_plugin->hook_timer(weechat_plugin, __interval, __max_calls, \
                               __callback, __data)
#define weechat_hook_fd(__fd, __flag_read, __flag_write,                \
                        __flag_exception, __callback, __data)           \
    weechat_plugin->hook_fd(weechat_plugin, __fd, __flag_read,          \
                            __flag_write, __flag_exception, __callback, \
                            __data)
#define weechat_hook_print(__buffer, __msg, __stri__colors, __callback, \
                           __data)                                      \
    weechat_plugin->hook_print(weechat_plugin, __buffer, __msg,         \
                               __stri__colors, __callback, __data)
#define weechat_hook_event(__event, __callback, __data)                 \
    weechat_plugin->hook_event(weechat_plugin, __event, __callback, __data)
#define weechat_hook_config(__type, __option, __callback, __data)       \
    weechat_plugin->hook_config(weechat_plugin, __type, __option,       \
                                __callback, __data)
#define weechat_unhook(__hook)                          \
    weechat_plugin->unhook(weechat_plugin, __hook)
#define weechat_unhook_all()                    \
    weechat_plugin->unhook(weechat_plugin)

/* buffers */
#define weechat_buffer_new(__category, __name, __input_data_cb)         \
    weechat_plugin->buffer_new(weechat_plugin, __category, __name,      \
                               __input_data_cb)
#define weechat_buffer_search(__category, __name)                       \
    weechat_plugin->buffer_search(weechat_plugin, __category, __name)
#define weechat_current_buffer                                  \
    weechat_plugin->buffer_search(weechat_plugin, NULL, NULL)
#define weechat_buffer_close(__buffer)                          \
    weechat_plugin->buffer_close(weechat_plugin, __buffer)
#define weechat_buffer_set(__buffer, __property, __value)             \
    weechat_plugin->buffer_set(weechat_plugin, __buffer,              \
                               __property, __value)

/* command */
#define weechat_command(__buffer, __command)                            \
    weechat_plugin->command(weechat_plugin, __buffer, __command)

/* infos */
#define weechat_info_get(__name)                        \
    weechat_plugin->info_get(weechat_plugin, __name)

/* lists */
#define weechat_list_get(__name, __pointer)                     \
    weechat_plugin->list_get(weechat_plugin, __name, __pointer)
#define weechat_list_next(__list)                       \
    weechat_plugin->list_next(weechat_plugin, __list)
#define weechat_list_prev(__list)                       \
    weechat_plugin->list_prev(weechat_plugin, __list)
#define weechat_list_fields(__list)                     \
    weechat_plugin->list_fields(weechat_plugin, __list)
#define weechat_list_integer(__item, __var)                         \
    weechat_plugin->list_integer(weechat_plugin, __item, __var)
#define weechat_list_string(__item, __var)                      \
    weechat_plugin->list_string(weechat_plugin, __item, __var)
#define weechat_list_pointer(__item, __var)                     \
    weechat_plugin->list_pointer(weechat_plugin, __item, __var)
#define weechat_list_time(__item, __var)                        \
    weechat_plugin->list_time(weechat_plugin, __item, __var)
#define weechat_list_free(__list)                       \
    weechat_plugin->list_free(weechat_plugin, __list)

#endif /* weechat-plugin.h */
