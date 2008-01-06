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

/* This header is designed to be distributed with WeeChat plugins */

#ifndef __WEECHAT_WEECHAT_PLUGIN_H
#define __WEECHAT_WEECHAT_PLUGIN_H 1

#include <sys/types.h>

struct t_gui_buffer;
struct t_weelist;

/* macros for defining plugin infos */
#define WEECHAT_PLUGIN_NAME(__name)             \
    char weechat_plugin_name[] = __name;
#define WEECHAT_PLUGIN_AUTHOR(__author)         \
    char weechat_plugin_author[] = __author;
#define WEECHAT_PLUGIN_DESCRIPTION(__desc)      \
    char weechat_plugin_description[] = __desc;
#define WEECHAT_PLUGIN_VERSION(__version)       \
    char weechat_plugin_version[] = __version;
#define WEECHAT_PLUGIN_LICENSE(__license)       \
    char weechat_plugin_license[] = __license;

/* return codes for plugin functions */
#define WEECHAT_RC_ERROR           -1  /* function failed with an error     */
#define WEECHAT_RC_OK               0  /* function ok                       */

/* return codes specific to message handlers: messages can be discarded for
   WeeChat, for plugins, or both */
#define WEECHAT_RC_IGNORE_WEECHAT   1  /* ignore WeeChat for this message   */
#define WEECHAT_RC_IGNORE_PLUGINS   2  /* ignore other plugins for msg      */
#define WEECHAT_RC_IGNORE_ALL       (PLUGIN_RC_OK_IGNORE_WEECHAT      \
                                    | PLUGIN_RC_OK_IGNORE_PLUGINS)
                                       /* ignore WeeChat and other plugins  */
#define WEECHAT_RC_WITH_HIGHLIGHT   4  /* ok and ask for highlight          */
                                       /* (for message handler only)        */

/* list management (order of elements) */
#define WEECHAT_LIST_POS_SORT      "sort"
#define WEECHAT_LIST_POS_BEGINNING "beginning"
#define WEECHAT_LIST_POS_END       "end"

/* buffer hotlist */
#define WEECHAT_HOTLIST_LOW       "0"
#define WEECHAT_HOTLIST_MESSAGE   "1"
#define WEECHAT_HOTLIST_PRIVATE   "2"
#define WEECHAT_HOTLIST_HIGHLIGHT "3"

struct t_weechat_plugin
{
    /* plugin variables */
    char *filename;                    /* name of plugin on disk            */
    void *handle;                      /* handle of plugin (given by dlopen)*/
    char *name;                        /* short name                        */
    char *description;                 /* description                       */
    char *author;                      /* author                            */
    char *version;                     /* version                           */
    char *license;                     /* license                           */
    char *charset;                     /* charset used by plugin            */
    struct t_weechat_plugin *prev_plugin; /* link to previous plugin        */
    struct t_weechat_plugin *next_plugin; /* link to next plugin            */
    
    /* plugin functions (API) */
    
    /* IMPORTANT NOTE for WeeChat developers: always add new API functions
       at the END of functions, for keeping backward compatibility with
       existing plugins */
    
    /* strings */
    void (*charset_set) (struct t_weechat_plugin *plugin, char *charset);
    char *(*iconv_to_internal) (char *charset, char *string);
    char *(*iconv_from_internal) (char *charset, char *string);
    char *(*gettext) (char *string);
    char *(*ngettext) (char *single, char *plural, int count);
    int (*strcasecmp) (char *string1, char *string2);
    int (*strncasecmp) (char *string1, char *string2, int max);
    char *(*strcasestr) (char *string1, char *string2);
    char *(*string_replace) (char *string, char *search, char *replace);
    char **(*string_explode) (char *string, char *separators, int keep_eol,
                              int num_items_max, int *num_items);
    void (*string_free_exploded) (char **exploded_string);
    char **(*string_split_command) (char *command, char separator);
    void (*string_free_splitted_command) (char **splitted_command);
    
    /* UTF-8 strings */
    int (*utf8_has_8bits) (char *string);
    int (*utf8_is_valid) (char *string, char **error);
    void (*utf8_normalize) (char *string, char replacement);
    char *(*utf8_prev_char) (char *string_start, char *string);
    char *(*utf8_next_char) (char *string);
    int (*utf8_char_size) (char *string);
    int (*utf8_strlen) (char *string);
    int (*utf8_strnlen) (char *string, int bytes);
    int (*utf8_strlen_screen) (char *string);
    int (*utf8_charcasecmp) (char *string1, char *string2);
    int (*utf8_char_size_screen) (char *string);
    char *(*utf8_add_offset) (char *string, int offset);
    int (*utf8_real_pos) (char *string, int pos);
    int (*utf8_pos) (char *string, int real_pos);
    
    /* directories */
    int (*mkdir_home) (char *directory, int mode);
    int (*mkdir) (char *directory, int mode);
    void (*exec_on_files) (char *directory, void *data,
                           int (*callback)(void *data, char *filename));
    
    /* util */
    int (*timeval_cmp) (struct timeval *tv1, struct timeval *tv2);
    long (*timeval_diff) (struct timeval *tv1, struct timeval *tv2);
    void (*timeval_add) (struct timeval *tv, long interval);
    
    /* sorted list */
    struct t_weelist *(*list_new) ();
    struct t_weelist_item *(*list_add) (struct t_weelist *weelist, char *data,
                                        char *where);
    struct t_weelist_item *(*list_search) (struct t_weelist *weelist,
                                           char *data);
    struct t_weelist_item *(*list_casesearch) (struct t_weelist *weelist,
                                               char *data);
    struct t_weelist_item *(*list_get) (struct t_weelist *weelist,
                                        int position);
    void (*list_set) (struct t_weelist_item *item, char *new_value);
    struct t_weelist_item *(*list_next) (struct t_weelist_item *item);
    struct t_weelist_item *(*list_prev) (struct t_weelist_item *item);
    char *(*list_string) (struct t_weelist_item *item);
    int (*list_size) (struct t_weelist *weelist);
    void (*list_remove) (struct t_weelist *weelist,
                         struct t_weelist_item *item);
    void (*list_remove_all) (struct t_weelist *weelist);
    void (*list_free) (struct t_weelist *weelist);

    /* config files */
    struct t_config_file *(*config_new) (struct t_weechat_plugin *plugin,
                                         char *filename);
    struct t_config_section *(*config_new_section) (struct t_config_file *config_file,
                                                    char *name,
                                                    void (*callback_read)
                                                    (struct t_config_file *config_file,
                                                     char *option_name,
                                                     char *value),
                                                    void (*callback_write)
                                                    (struct t_config_file *config_file,
                                                     char *section_name),
                                                    void (*callback_write_default)
                                                    (struct t_config_file *config_file,
                                                     char *section_name));
    struct t_config_section *(*config_search_section) (struct t_config_file *config_file,
                                                       char *section_name);
    struct t_config_option *(*config_new_option) (struct t_config_section *config_file,
                                                  char *name, char *type,
                                                  char *description,
                                                  char *string_values,
                                                  int min, int max,
                                                  char *default_value,
                                                  void (*callback_change)());
    struct t_config_option *(*config_search_option) (struct t_config_file *config_file,
                                                     struct t_config_section *section,
                                                     char *option_name);
    int (*config_string_to_boolean) (char *text);
    int (*config_option_set) (struct t_config_option *option, char *new_value,
                              int run_callback);
    int (*config_boolean) (struct t_config_option *option);
    int (*config_integer) (struct t_config_option *option);
    char *(*config_string) (struct t_config_option *option);
    int (*config_color) (struct t_config_option *option);
    void (*config_write_line) (struct t_config_file *config_file,
                               char *option_name, char *value, ...);
    int (*config_write) (struct t_config_file *config_file);
    int (*config_read) (struct t_config_file *config_file);
    int (*config_reload) (struct t_config_file *config_file);
    void (*config_free) (struct t_config_file *config_file);
    struct t_config_option *(*config_get_weechat) (char *option_name);
    char *(*config_get_plugin) (struct t_weechat_plugin *plugin,
                                char *option_name);
    int (*config_set_plugin) (struct t_weechat_plugin *plugin,
                              char *option_name, char *value);
    
    /* display */
    char *(*prefix) (char *prefix);
    char *(*color) (char *color_name);
    void (*printf_date) (struct t_gui_buffer *buffer, time_t date,
                         char *message, ...);
    void (*log_printf) (char *message, ...);
    void (*infobar_printf) (struct t_weechat_plugin *plugin, int delay,
                            char *color_name, char *format, ...);
    void (*infobar_remove) (int how_many);
    
    /* hooks */
    struct t_hook *(*hook_command) (struct t_weechat_plugin *plugin,
                                    char *command, char *description,
                                    char *args, char *args_description,
                                    char *completion,
                                    int (*callback)(void *data,
                                                    struct t_gui_buffer *buffer,
                                                    int argc, char **argv,
                                                    char **argv_eol),
                                    void *callback_data);
    struct t_hook *(*hook_timer) (struct t_weechat_plugin *plugin,
                                  long interval, int align_second,
                                  int max_calls,
                                  int (*callback)(void *data),
                                  void *callback_data);
    struct t_hook *(*hook_fd) (struct t_weechat_plugin *plugin,
                               int fd, int flag_read, int flag_write,
                               int flag_exception,
                               int (*callback)(void *data),
                               void *callback_data);
    struct t_hook *(*hook_print) (struct t_weechat_plugin *plugin,
                                  struct t_gui_buffer *buffer,
                                  char *message, int strip_colors,
                                  int (*callback)(void *data,
                                                  struct t_gui_buffer *buffer,
                                                  time_t date, char *prefix,
                                                  char *message),
                                  void *callback_data);
    struct t_hook *(*hook_signal) (struct t_weechat_plugin *plugin,
                                   char *signal,
                                   int (*callback)(void *data, char *signal,
                                                   void *signal_data),
                                   void *callback_data);
    void (*hook_signal_send) (char *signal, void *signal_data);
    struct t_hook *(*hook_config) (struct t_weechat_plugin *plugin,
                                   char *type, char *option,
                                   int (*callback)(void *data, char *type,
                                                   char *option, char *value),
                                   void *callback_data);
    struct t_hook *(*hook_completion) (struct t_weechat_plugin *plugin,
                                       char *completion,
                                       int (*callback)(void *data,
                                                       char *completion,
                                                       struct t_gui_buffer *buffer,
                                                       struct t_weelist *list),
                                       void *callback_data);
    void (*unhook) (struct t_hook *hook);
    void (*unhook_all) (struct t_weechat_plugin *plugin);
    
    /* buffers */
    struct t_gui_buffer *(*buffer_new) (struct t_weechat_plugin *plugin,
                                        char *category, char *name,
                                        void (*callback_input_data)(struct t_gui_buffer *buffer,
                                                                    char *data));
    struct t_gui_buffer *(*buffer_search) (char *category, char *name);
    void (*buffer_close) (struct t_gui_buffer *buffer, int switch_to_another);
    void *(*buffer_get) (struct t_gui_buffer *buffer, char *property);
    void (*buffer_set) (struct t_gui_buffer *buffer, char *property,
                        char *value);
    
    /* nicklist */
    struct t_gui_nick_group *(*nicklist_add_group) (struct t_gui_buffer *buffer,
                                                    struct t_gui_nick_group *parent_group,
                                                    char *name, char *color,
                                                    int visible);
    struct t_gui_nick_group *(*nicklist_search_group) (struct t_gui_buffer *buffer,
                                                       struct t_gui_nick_group *from_group,
                                                       char *name);
    struct t_gui_nick *(*nicklist_add_nick) (struct t_gui_buffer *buffer,
                                             struct t_gui_nick_group *group,
                                             char *name, char *color,
                                             char prefix, char *prefix_color,
                                             int visible);
    struct t_gui_nick *(*nicklist_search_nick) (struct t_gui_buffer *buffer,
                                                struct t_gui_nick_group *from_group,
                                                char *name);
    void (*nicklist_remove_group) (struct t_gui_buffer *buffer,
                                   struct t_gui_nick_group *group);
    void (*nicklist_remove_nick) (struct t_gui_buffer *buffer,
                                  struct t_gui_nick *nick);
    void (*nicklist_remove_all) (struct t_gui_buffer *buffer);
    
    /* command */
    void (*command) (struct t_weechat_plugin *plugin,
                     struct t_gui_buffer *buffer, char *command);
    
    /* infos */
    char *(*info_get) (struct t_weechat_plugin *plugin, char *info);
    
    /* infolists */
    struct t_plugin_infolist *(*infolist_get) (char *name, void *pointer);
    int (*infolist_next) (struct t_plugin_infolist *infolist);
    int (*infolist_prev) (struct t_plugin_infolist *infolist);
    char *(*infolist_fields) (struct t_plugin_infolist *infolist);
    int (*infolist_integer) (struct t_plugin_infolist *infolist, char *var);
    char *(*infolist_string) (struct t_plugin_infolist *infolist, char *var);
    void *(*infolist_pointer) (struct t_plugin_infolist *infolist, char *var);
    time_t (*infolist_time) (struct t_plugin_infolist *infolist, char *var);
    void (*infolist_free) (struct t_plugin_infolist *infolist);
    
    /* WeeChat developers: ALWAYS add new functions at the end */
};

/* macros for easy call to plugin API */

/* strings */
#define weechat_charset_set(__charset)                          \
    weechat_plugin->charset_set(weechat_plugin, __charset)
#define weechat_iconv_to_internal(__charset, __string)          \
    weechat_plugin->iconv_to_internal(__charset, __string)
#define weechat_iconv_from_internal(__charset, __string)        \
    weechat_plugin->iconv_from_internal(__charset, __string)
#ifndef __WEECHAT_H
#define _(string) weechat_plugin->gettext(string)
#define N_(string) (string)
#define NG_(single,plural,number)                       \
    weechat_plugin->ngettext(single, plural, number)
#endif
#define weechat_strcasecmp(__string1, __string2)        \
    weechat_plugin->strcasecmp(__string1, __string2)
#define weechat_strncasecmp(__string1, __string2, __max)        \
    weechat_plugin->strncasecmp(__string1, __string2, __max)
#define weechat_strcasestr(__string1, __string2)        \
    weechat_plugin->strcasestr(__string1, __string2)
#define weechat_string_replace(__string, __search, __replace)           \
    weechat_plugin->string_replace(__string, __search, __replace)
#define weechat_string_explode(__string, __separator, __eol, __max,     \
                               __num_items)                             \
    weechat_plugin->string_explode(__string, __separator, __eol,        \
                                   __max, __num_items)
#define weechat_string_free_exploded(__exploded_string)         \
    weechat_plugin->string_free_exploded(__exploded_string)
#define weechat_string_split_command(__command, __separator)            \
    weechat_plugin->string_split_command(__command, __separator)
#define weechat_string_free_splitted_command(__splitted_command)        \
    weechat_plugin->string_free_splitted_command(__splitted_command)

/* UTF-8 strings */
#define weechat_utf8_has_8bits(__string)        \
    weechat_plugin->utf8_has_8bits(__string)
#define weechat_utf8_is_valid(__string, __error)        \
    weechat_plugin->utf8_is_valid(__string, __error)
#define weechat_utf8_normalize(__string, __char)        \
    weechat_plugin->utf8_normalize(__string, __char)
#define weechat_utf8_prev_char(__start, __string)       \
    weechat_plugin->utf8_prev_char(__start, __string)
#define weechat_utf8_next_char(__string)        \
    weechat_plugin->utf8_next_char(__string)
#define weechat_utf8_char_size(__string)        \
    weechat_plugin->utf8_char_size(__string)
#define weechat_utf8_strlen(__string)           \
    weechat_plugin->utf8_strlen(__string)
#define weechat_utf8_strnlen(__string, __bytes)         \
    weechat_plugin->utf8_strnlen(__string, __bytes)
#define weechat_utf8_strlen_screen(__string)            \
    weechat_plugin->utf8_strlen_screen(__string)
#define weechat_utf8_charcasecmp(__string)      \
    weechat_plugin->utf8_charcasecmp(__string)
#define weechat_utf8_char_size_screen(__string)         \
    weechat_plugin->utf8_char_size_screen(__string)
#define weechat_utf8_add_offset(__string, __offset)     \
    weechat_plugin->utf8_add_offset(__string, __offset)
#define weechat_utf8_real_pos(__string, __pos)          \
    weechat_plugin->utf8_real_pos(__string, __pos)
#define weechat_utf8_pos(__string, __real_pos)          \
    weechat_plugin->utf8_pos(__string, __real_pos)

/* directories */
#define weechat_mkdir_home(__directory, __mode)         \
    weechat_plugin->mkdir_home(__directory, __mode)
#define weechat_mkdir(__directory, __mode)      \
    weechat_plugin->mkdir(__directory, __mode)
#define weechat_exec_on_files(__directory, __data, __callback)          \
    weechat_plugin->exec_on_files(__directory, __data, __callback)

/* util */
#define weechat_timeval_cmp(__time1, __time2)           \
    weechat_plugin->timeval_cmp(__time1, __time2)
#define weechat_timeval_diff(__time1, __time2)          \
    weechat_plugin->timeval_diff(__time1, __time2)
#define weechat_timeval_add(__time, __interval)         \
    weechat_plugin->timeval_add(__time, __interval)

/* sorted list */
#define weechat_list_new()                      \
    weechat_plugin->list_new()
#define weechat_list_add(__list, __string, __where)     \
    weechat_plugin->list_add(__list, __string, __where)
#define weechat_list_search(__list, __string)           \
    weechat_plugin->list_search(__list, __string)
#define weechat_list_casesearch(__list, __string)       \
    weechat_plugin->list_casesearch(__list, __string)
#define weechat_list_get(__list, __index)       \
    weechat_plugin->list_get(__list, __index)
#define weechat_list_set(__item, __new_value)           \
    weechat_plugin->list_set(__item, __new_value)
#define weechat_list_next(__item)               \
    weechat_plugin->list_next(__item)
#define weechat_list_prev(__item)               \
    weechat_plugin->list_prev(__item)
#define weechat_list_string(__item)             \
    weechat_plugin->list_string(__item)
#define weechat_list_size(__list)               \
    weechat_plugin->list_size(__list)
#define weechat_list_remove(__list, __item)     \
    weechat_plugin->list_remove(__list, __item)
#define weechat_list_remove_all(__list)         \
    weechat_plugin->list_remove_all(__list)
#define weechat_list_free(__list)               \
    weechat_plugin->list_free(__list)

/* config files */
#define weechat_config_new(__filename)                          \
    weechat_plugin->config_new(weechat_plugin, __filename)
#define weechat_config_new_section(__config, __name, __cb_read,         \
                                   __cb_write_std, __cb_write_def)      \
    weechat_plugin->config_new_section(__config, __name, __cb_read,     \
                                       __cb_write_std, __cb_write_def)
#define weechat_config_search_section(__config, __name)         \
    weechat_plugin->config_search_section(__config, __name)
#define weechat_config_new_option(__section, __name, __type, __desc,    \
                                  __string_values, __min, __max,        \
                                  __default, __callback)                \
    weechat_plugin->config_new_option(__section, __name, __type,        \
                                      __desc, __string_values,          \
                                      __min, __max, __default,          \
                                      __callback)
#define weechat_config_search_option(__config, __section, __name)       \
    weechat_plugin->config_search_option(__config, __section, __name)
#define weechat_config_string_to_boolean(__string)      \
    weechat_plugin->config_string_to_boolean(__string)
#define weechat_config_option_set(__option, __value, __run_callback)    \
    weechat_plugin->config_option_set(__option, __value,                \
                                      __run_callback)
#define weechat_config_boolean(__option)        \
    weechat_plugin->config_boolean(__option)
#define weechat_config_integer(__option)        \
    weechat_plugin->config_integer(__option)
#define weechat_config_string(__option)         \
    weechat_plugin->config_string(__option)
#define weechat_config_color(__option)          \
    weechat_plugin->config_color(__option)
#define weechat_config_write_line(__config, __option,           \
                                  __value...)                   \
    weechat_plugin->config_write_line(__config, __option,       \
                                      ##__value)
#define weechat_config_write(__config)          \
    weechat_plugin->config_write(__config)
#define weechat_config_read(__config)           \
    weechat_plugin->config_read(__config)
#define weechat_config_reload(__config)         \
    weechat_plugin->config_reload(__config)
#define weechat_config_free(__config)           \
    weechat_plugin->config_free(__config)
#define weechat_config_get_weechat(__option)            \
    weechat_plugin->config_get_weechat(__option)
#define weechat_config_get_plugin(__option)                     \
    weechat_plugin->config_get_plugin(weechat_plugin, __option)
#define weechat_config_set_plugin(__option, __value)                    \
    weechat_plugin->config_set_plugin(weechat_plugin, __option, __value)

/* display */
#define weechat_prefix(__prefix)                \
    weechat_plugin->prefix(__prefix)
#define weechat_color(__color_name)             \
    weechat_plugin->color(__color_name)
#define weechat_printf(__buffer, __message, __argz...)                  \
    weechat_plugin->printf_date(__buffer, 0, __message, ##__argz)
#define weechat_printf_date(__buffer, __date, __message, __argz...)     \
    weechat_plugin->printf_date(__buffer, __date, __message, ##__argz)
#define weechat_log_printf(__message, __argz...)        \
    weechat_plugin->log_printf(__message, ##__argz)
#define weechat_infobar_printf(__delay, __color_name, __message,        \
                               __argz...)                               \
    weechat_plugin->infobar_printf(weechat_plugin, __delay,             \
                                   __color_name, __message, ##__argz)
#define weechat_infobar_remove(__how_many)      \
    weechat_plugin->infobar_remove(__how_many)

/* hooks */
#define weechat_hook_command(__command, __description, __args,          \
                             __args_desc, __completion, __callback,     \
                             __data)                                    \
    weechat_plugin->hook_command(weechat_plugin, __command, __description, \
                                 __args, __args_desc, __completion,     \
                                 __callback, __data)
#define weechat_hook_timer(__interval, __align_second, __max_calls,     \
                           __callback, __data)                          \
    weechat_plugin->hook_timer(weechat_plugin, __interval,              \
                               __align_second, __max_calls,             \
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
#define weechat_hook_signal(__signal, __callback, __data)               \
    weechat_plugin->hook_signal(weechat_plugin, __signal, __callback,   \
                                __data)
#define weechat_hook_signal_send(__signal, __pointer)                   \
    weechat_plugin->hook_signal_send(__signal, __pointer)
#define weechat_hook_config(__type, __option, __callback, __data)       \
    weechat_plugin->hook_config(weechat_plugin, __type, __option,       \
                                __callback, __data)
#define weechat_hook_completion(__completion, __callback, __data)       \
    weechat_plugin->hook_completion(weechat_plugin, __completion,       \
                                    __callback, __data)
#define weechat_unhook(__hook)                  \
    weechat_plugin->unhook( __hook)
#define weechat_unhook_all()                            \
    weechat_plugin->unhook_all_plugin(weechat_plugin)

/* buffers */
#define weechat_buffer_new(__category, __name, __callback_input_data)   \
    weechat_plugin->buffer_new(weechat_plugin, __category, __name,      \
                               __callback_input_data)
#define weechat_buffer_search(__category, __name)       \
    weechat_plugin->buffer_search(__category, __name)
#define weechat_current_buffer                  \
    weechat_plugin->buffer_search(NULL, NULL)
#define weechat_buffer_close(__buffer, __switch_to_another)     \
    weechat_plugin->buffer_close(__buffer, __switch_to_another)
#define weechat_buffer_get(__buffer, __property)        \
    weechat_plugin->buffer_get(__buffer, __property)
#define weechat_buffer_set(__buffer, __property, __value)       \
    weechat_plugin->buffer_set(__buffer, __property, __value)

/* nicklist */
#define weechat_nicklist_add_group(__buffer, __parent_group, __name, \
                                   __color, __visible)               \
    weechat_plugin->nicklist_add_group(__buffer, __parent_group,     \
                                       __name, __color, __visible)
#define weechat_nicklist_search_group(__buffer, __from_group, __name)   \
    weechat_plugin->nicklist_search_group(__buffer, __from_group,       \
                                          __name)
#define weechat_nicklist_add_nick(__buffer, __group, __name, __color,   \
                                  __prefix, __prefix_color, __visible)  \
    weechat_plugin->nicklist_add_nick(__buffer, __group, __name,        \
                                      __color, __prefix, __prefix_color, \
                                      __visible)
#define weechat_nicklist_search_nick(__buffer, __from_group, __name)    \
    weechat_plugin->nicklist_search_nick(__buffer, __from_group, __name)
#define weechat_nicklist_remove_group(__buffer, __group)        \
    weechat_plugin->nicklist_remove_group(__buffer, __group)
#define weechat_nicklist_remove_nick(__buffer, __nick)          \
    weechat_plugin->nicklist_remove_nick(__buffer, __nick)
#define weechat_nicklist_remove_all(__buffer)           \
    weechat_plugin->nicklist_remove_all(__buffer)

/* command */
#define weechat_command(__buffer, __command)                            \
    weechat_plugin->command(weechat_plugin, __buffer, __command)

/* infos */
#define weechat_info_get(__name)                        \
    weechat_plugin->info_get(weechat_plugin, __name)

/* infolists */
#define weechat_infolist_get(__name, __pointer)         \
    weechat_plugin->infolist_get(__name, __pointer)
#define weechat_infolist_next(__list)           \
    weechat_plugin->infolist_next(__list)
#define weechat_infolist_prev(__list)           \
    weechat_plugin->infolist_prev(__list)
#define weechat_infolist_fields(__list)         \
    weechat_plugin->infolist_fields(__list)
#define weechat_infolist_integer(__item, __var)         \
    weechat_plugin->infolist_integer(__item, __var)
#define weechat_infolist_string(__item, __var)          \
    weechat_plugin->infolist_string(__item, __var)
#define weechat_infolist_pointer(__item, __var)         \
    weechat_plugin->infolist_pointer(__item, __var)
#define weechat_infolist_time(__item, __var)            \
    weechat_plugin->infolist_time(__item, __var)
#define weechat_infolist_free(__list)           \
    weechat_plugin->infolist_free(__list)

#endif /* weechat-plugin.h */
