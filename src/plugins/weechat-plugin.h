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
    char **(*string_explode) (struct t_weechat_plugin *, char *, char *, int,
                              int, int *);
    void (*string_free_exploded) (struct t_weechat_plugin *, char **);
    
    /* directories */
    int (*mkdir_home) (struct t_weechat_plugin *, char *);
    void (*exec_on_files) (struct t_weechat_plugin *, char *,
                           int (*)(char *));
    
    /* display */
    void (*printf) (struct t_weechat_plugin *, void *, char *, ...);
    void (*printf_date) (struct t_weechat_plugin *, void *, time_t,
                         char *, ...);
    char *(*prefix) (struct t_weechat_plugin *, char *);
    char *(*color) (struct t_weechat_plugin *, char *);
    void (*print_infobar) (struct t_weechat_plugin *, int, char *, ...);
    void (*infobar_remove) (struct t_weechat_plugin *, int);
    
    /* hooks */
    struct t_hook *(*hook_command) (struct t_weechat_plugin *, char *, char *,
                                    char *, char *, char *,
                                    int (*)(void *, int, char **, char **),
                                    void *);
    struct t_hook *(*hook_print) (struct t_weechat_plugin *, void *, char *,
                                  int (*)(void *, void *, time_t, char *, char *),
                                  void *);
    struct t_hook *(*hook_config) (struct t_weechat_plugin *, char *, char *,
                                   int (*)(void *, char *, char *, char *),
                                   void *);
    struct t_hook *(*hook_timer) (struct t_weechat_plugin *, long, int,
                                  int (*)(void *), void *);
    struct t_hook *(*hook_fd) (struct t_weechat_plugin *, int, int, int, int,
                               int (*)(void *), void *);
    void (*unhook) (struct t_weechat_plugin *, void *);
    void (*unhook_all) (struct t_weechat_plugin *);
    
    /* buffers */
    struct t_gui_buffer *(*buffer_new) (struct t_weechat_plugin *,
                                        char *, char *);
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
    int (*list_int) (struct t_weechat_plugin *, void *, char *);
    char *(*list_string) (struct t_weechat_plugin *, void *, char *);
    void *(*list_pointer) (struct t_weechat_plugin *, void *, char *);
    time_t (*list_time) (struct t_weechat_plugin *, void *, char *);
    void (*list_free) (struct t_weechat_plugin *, void *);
    
    /* config */
    char *(*config_get) (struct t_weechat_plugin *, char *);
    int (*config_set) (struct t_weechat_plugin *, char *, char *);
    char *(*plugin_config_get) (struct t_weechat_plugin *, char *);
    int (*plugin_config_set) (struct t_weechat_plugin *, char *, char *);
    
    /* log */
    void (*log) (struct t_weechat_plugin *, char *, char *, char *, ...);
    
    /* WeeChat developers: ALWAYS add new functions at the end */
};

/* macros for easy call to plugin API */

#ifndef __WEECHAT_H
#define _(string) weechat_plugin->gettext(weechat_plugin, string)
#define N_(string) (string)
#define NG_(single,plural,number)                                       \
    weechat_plugin->ngettext(weechat_plugin, single, plural, number)
#endif
#define weechat_strcasecmp(string1, string2)                            \
    weechat_plugin->strcasecmp(weechat_plugin, string1, string2)
#define weechat_strncasecmp(string1, string2, max)                      \
    weechat_plugin->strncasecmp(weechat_plugin, string1, string2, max)
#define weechat_string_explode(string1, separator, eol, max,            \
                               num_items)                               \
    weechat_plugin->string_explode(weechat_plugin, string1, separator,  \
                                   eol, max, num_items)
#define weechat_string_free_exploded(array_str)                         \
    weechat_plugin->string_free_exploded(weechat_plugin, array_str)

#define weechat_printf(buffer, argz...)                         \
    weechat_plugin->printf(weechat_plugin, buffer, ##argz)
#define weechat_printf_date(buffer, datetime, argz...)                  \
    weechat_plugin->printf_date(weechat_plugin, buffer, datetime,       \
                                ##argz)
#define weechat_prefix(prefix_name)                     \
    weechat_plugin->prefix(weechat_plugin, prefix_name)
#define weechat_color(color_name)                       \
    weechat_plugin->color(weechat_plugin, color_name)

#define weechat_hook_command(command, description, args, args_desc,     \
                             completion, callback, data)                \
    weechat_plugin->hook_command(weechat_plugin, command, description,  \
                                 args, args_desc, completion, callback, \
                                 data)
#define weechat_hook_print(buffer, msg, callback, data)                 \
    weechat_plugin->hook_print(weechat_plugin, buffer, msg,             \
                               callback, data)
#define weechat_hook_config(type, option, callback, data)               \
    weechat_plugin->hook_config(weechat_plugin, type, option,           \
                                callback, data)
#define weechat_hook_timer(interval, max_calls, callback, data)         \
    weechat_plugin->hook_timer(weechat_plugin, interval, max_calls,     \
                               callback, data)
#define weechat_hook_fd(fd, flag_read, flag_write, flag_exception,      \
                        callback, data)                                 \
    weechat_plugin->hook_fd(weechat_plugin, fd, flag_read, flag_write,  \
                            flag_exception, callback, data)
#define weechat_unhook(hook)                            \
    weechat_plugin->unhook(weechat_plugin, hook)
#define weechat_unhook_all()                    \
    weechat_plugin->unhook(weechat_plugin)

#define weechat_buffer_new(category, name)                      \
    weechat_plugin->buffer_new(weechat_plugin, category, name)
#define weechat_buffer_search(category, name)                           \
    weechat_plugin->buffer_search(weechat_plugin, category, name)
#define weechat_current_buffer                                  \
    weechat_plugin->buffer_search(weechat_plugin, NULL, NULL)
#define weechat_buffer_close(ptr_buffer)                        \
    weechat_plugin->buffer_close(weechat_plugin, ptr_buffer)
#define weechat_buffer_set(ptr_buffer, property, value)                 \
    weechat_plugin->buffer_set(weechat_plugin, ptr_buffer,              \
                               property, value)

#define weechat_command(buffer, cmd)                            \
    weechat_plugin->command(weechat_plugin, buffer, cmd)

#define weechat_info_get(infoname)                      \
    weechat_plugin->info_get(weechat_plugin, infoname)

#define weechat_list_get(name, pointer)                                 \
    weechat_plugin->list_get(weechat_plugin, name, pointer)
#define weechat_list_next(ptrlist)                      \
    weechat_plugin->list_next(weechat_plugin, ptrlist)
#define weechat_list_prev(ptrlist)                      \
    weechat_plugin->list_prev(weechat_plugin, ptrlist)
#define weechat_list_fields(ptrlist)                            \
    weechat_plugin->list_fields(weechat_plugin, ptrlist)
#define weechat_list_int(ptritem, var)                          \
    weechat_plugin->list_int(weechat_plugin, ptritem, var)
#define weechat_list_string(ptritem, var)                       \
    weechat_plugin->list_string(weechat_plugin, ptritem, var)
#define weechat_list_pointer(ptritem, var)                      \
    weechat_plugin->list_pointer(weechat_plugin, ptritem, var)
#define weechat_list_time(ptritem, var)                         \
    weechat_plugin->list_time(weechat_plugin, ptritem, var)
#define weechat_list_free(ptrlist)                      \
    weechat_plugin->list_free(weechat_plugin, ptrlist)

#define weechat_config_get(option)                      \
    weechat_plugin->config_get(weechat_plugin, option)
#define weechat_config_set(option, value)                       \
    weechat_plugin->config_set(weechat_plugin, option, value)
#define weechat_plugin_config_get(option)                       \
    weechat_plugin->plugin_config_get(weechat_plugin, option)
#define weechat_plugin_config_set(option, value)                        \
    weechat_plugin->plugin_config_set(weechat_plugin, option, value)

#endif /* weechat-plugin.h */
