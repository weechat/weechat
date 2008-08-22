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

struct t_gui_window;
struct t_gui_buffer;
struct t_gui_bar;
struct t_gui_bar_item;
struct t_gui_completion;
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
#define WEECHAT_PLUGIN_WEECHAT_VERSION(__version)       \
    char weechat_plugin_weechat_version[] = __version;
#define WEECHAT_PLUGIN_LICENSE(__license)       \
    char weechat_plugin_license[] = __license;

/* return codes for plugin functions */
#define WEECHAT_RC_OK                               0
#define WEECHAT_RC_ERROR                           -1

/* return codes for config read functions/callbacks */
#define WEECHAT_CONFIG_READ_OK                      0
#define WEECHAT_CONFIG_READ_MEMORY_ERROR           -1
#define WEECHAT_CONFIG_READ_FILE_NOT_FOUND         -2

/* return codes for config write functions/callbacks */
#define WEECHAT_CONFIG_WRITE_OK                     0
#define WEECHAT_CONFIG_WRITE_ERROR                 -1
#define WEECHAT_CONFIG_WRITE_MEMORY_ERROR          -2

/* return codes for config option set */
#define WEECHAT_CONFIG_OPTION_SET_OK_CHANGED        2
#define WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE     1
#define WEECHAT_CONFIG_OPTION_SET_ERROR             0
#define WEECHAT_CONFIG_OPTION_SET_OPTION_NOT_FOUND -1

/* return codes for config option unset */
#define WEECHAT_CONFIG_OPTION_UNSET_OK_NO_RESET     0
#define WEECHAT_CONFIG_OPTION_UNSET_OK_RESET        1
#define WEECHAT_CONFIG_OPTION_UNSET_OK_REMOVED      2
#define WEECHAT_CONFIG_OPTION_UNSET_ERROR          -1

/* list management (order of elements) */
#define WEECHAT_LIST_POS_SORT                       "sort"
#define WEECHAT_LIST_POS_BEGINNING                  "beginning"
#define WEECHAT_LIST_POS_END                        "end"

/* buffer hotlist */
#define WEECHAT_HOTLIST_LOW                         "0"
#define WEECHAT_HOTLIST_MESSAGE                     "1"
#define WEECHAT_HOTLIST_PRIVATE                     "2"
#define WEECHAT_HOTLIST_HIGHLIGHT                   "3"

/* connect status for connection hooked */
#define WEECHAT_HOOK_CONNECT_OK                     0
#define WEECHAT_HOOK_CONNECT_ADDRESS_NOT_FOUND      1
#define WEECHAT_HOOK_CONNECT_IP_ADDRESS_NOT_FOUND   2
#define WEECHAT_HOOK_CONNECT_CONNECTION_REFUSED     3
#define WEECHAT_HOOK_CONNECT_PROXY_ERROR            4
#define WEECHAT_HOOK_CONNECT_LOCAL_HOSTNAME_ERROR   5
#define WEECHAT_HOOK_CONNECT_GNUTLS_INIT_ERROR      6
#define WEECHAT_HOOK_CONNECT_GNUTLS_HANDSHAKE_ERROR 7
#define WEECHAT_HOOK_CONNECT_MEMORY_ERROR           8

/* type of data for signal hooked */
#define WEECHAT_HOOK_SIGNAL_STRING                  "string"
#define WEECHAT_HOOK_SIGNAL_INT                     "int"
#define WEECHAT_HOOK_SIGNAL_POINTER                 "pointer"

struct t_weechat_plugin
{
    /* plugin variables */
    char *filename;                    /* name of plugin on disk            */
    void *handle;                      /* handle of plugin (given by dlopen)*/
    char *name;                        /* short name                        */
    char *description;                 /* description                       */
    char *author;                      /* author                            */
    char *version;                     /* plugin version                    */
    char *weechat_version;             /* weechat version required          */
    char *license;                     /* license                           */
    char *charset;                     /* charset used by plugin            */
    struct t_weechat_plugin *prev_plugin; /* link to previous plugin        */
    struct t_weechat_plugin *next_plugin; /* link to next plugin            */
    
    /* plugin functions (API) */
    
    /* IMPORTANT NOTE for WeeChat developers: always add new API functions
       at the END of functions, for keeping backward compatibility with
       existing plugins */
    
    /* strings */
    void (*charset_set) (struct t_weechat_plugin *plugin, const char *charset);
    char *(*iconv_to_internal) (const char *charset, const char *string);
    char *(*iconv_from_internal) (const char *charset, const char *string);
    char *(*gettext) (const char *string);
    char *(*ngettext) (const char *single, const char *plural, int count);
    char *(*strndup) (const char *string, int length);
    void (*string_tolower) (char *string);
    void (*string_toupper) (char *string);
    int (*strcasecmp) (const char *string1, const char *string2);
    int (*strncasecmp) (const char *string1, const char *string2, int max);
    int (*strcmp_ignore_chars) (const char *string1, const char *string2,
                                const char *chars_ignored, int case_sensitive);
    char *(*strcasestr) (const char *string1, const char *string2);
    int (*string_match) (const char *string, const char *mask,
                         int case_sensitive);
    char *(*string_replace) (const char *string, const char *search,
                             const char *replace);
    char *(*string_remove_quotes) (const char *string, const char *quotes);
    char *(*string_strip) (const char *string, int left, int right,
                           const char *chars);
    int (*string_has_highlight) (const char *string,
                                 const char *highlight_words);
    char **(*string_explode) (const char *string, const char *separators,
                              int keep_eol, int num_items_max, int *num_items);
    void (*string_free_exploded) (char **exploded_string);
    char *(*string_build_with_exploded) (char **exploded_string,
                                         const char *separator);
    char **(*string_split_command) (const char *command, char separator);
    void (*string_free_splitted_command) (char **splitted_command);
    
    /* UTF-8 strings */
    int (*utf8_has_8bits) (const char *string);
    int (*utf8_is_valid) (const char *string, char **error);
    void (*utf8_normalize) (const char *string, char replacement);
    char *(*utf8_prev_char) (const char *string_start, const char *string);
    char *(*utf8_next_char) (const char *string);
    int (*utf8_char_size) (const char *string);
    int (*utf8_strlen) (const char *string);
    int (*utf8_strnlen) (const char *string, int bytes);
    int (*utf8_strlen_screen) (const char *string);
    int (*utf8_charcasecmp) (const char *string1, const char *string2);
    int (*utf8_char_size_screen) (const char *string);
    char *(*utf8_add_offset) (const char *string, int offset);
    int (*utf8_real_pos) (const char *string, int pos);
    int (*utf8_pos) (const char *string, int real_pos);
    
    /* directories */
    int (*mkdir_home) (const char *directory, int mode);
    int (*mkdir) (const char *directory, int mode);
    void (*exec_on_files) (const char *directory, void *data,
                           int (*callback)(void *data, const char *filename));
    
    /* util */
    int (*timeval_cmp) (struct timeval *tv1, struct timeval *tv2);
    long (*timeval_diff) (struct timeval *tv1, struct timeval *tv2);
    void (*timeval_add) (struct timeval *tv, long interval);
    
    /* sorted list */
    struct t_weelist *(*list_new) ();
    struct t_weelist_item *(*list_add) (struct t_weelist *weelist, const char *data,
                                        const char *where);
    struct t_weelist_item *(*list_search) (struct t_weelist *weelist,
                                           const char *data);
    struct t_weelist_item *(*list_casesearch) (struct t_weelist *weelist,
                                               const char *data);
    struct t_weelist_item *(*list_get) (struct t_weelist *weelist,
                                        int position);
    void (*list_set) (struct t_weelist_item *item, const char *value);
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
                                         const char *name,
                                         int (*callback_reload)(void *data,
                                                                struct t_config_file *config_file),
                                         void *callback_reload_data);
    struct t_config_section *(*config_new_section) (struct t_config_file *config_file,
                                                    const char *name,
                                                    int user_can_add_options,
                                                    int user_can_delete_options,
                                                    int (*callback_read)(void *data,
                                                                         struct t_config_file *config_file,
                                                                         struct t_config_section *section,
                                                                         const char *option_name,
                                                                         const char *value),
                                                    void *callback_read_data,
                                                    void (*callback_write)(void *data,
                                                                           struct t_config_file *config_file,
                                                                           const char *section_name),
                                                    void *callback_write_data,
                                                    void (*callback_write_default)(void *data,
                                                                                   struct t_config_file *config_file,
                                                                                   const char *section_name),
                                                    void *callback_write_default_data,
                                                    int (*callback_create_option)(void *data,
                                                                                  struct t_config_file *config_file,
                                                                                  struct t_config_section *section,
                                                                                  const char *option_name,
                                                                                  const char *value),
                                                    void *callback_create_option_data);
    struct t_config_section *(*config_search_section) (struct t_config_file *config_file,
                                                       const char *section_name);
    struct t_config_option *(*config_new_option) (struct t_config_file *config_file,
                                                  struct t_config_section *section,
                                                  const char *name, const char *type,
                                                  const char *description,
                                                  const char *string_values,
                                                  int min, int max,
                                                  const char *default_value,
                                                  int (*callback_check_value)(void *data,
                                                                              struct t_config_option *option,
                                                                              const char *value),
                                                  void *callback_check_value_data,
                                                  void (*callback_change)(void *data,
                                                                          struct t_config_option *option),
                                                  void *callback_change_data,
                                                  void (*callback_delete)(void *data,
                                                                          struct t_config_option *option),
                                                  void *callback_delete_data);
    struct t_config_option *(*config_search_option) (struct t_config_file *config_file,
                                                     struct t_config_section *section,
                                                     const char *option_name);
    void (*config_search_section_option) (struct t_config_file *config_file,
                                          struct t_config_section *section,
                                          const char *option_name,
                                          struct t_config_section **section_found,
                                          struct t_config_option **option_found);
    void (*config_search_with_string) (const char *option_name,
                                       struct t_config_file **config_file,
                                       struct t_config_section **section,
                                       struct t_config_option **option,
                                       char **pos_option_name);
    int (*config_string_to_boolean) (const char *text);
    int (*config_option_reset) (struct t_config_option *option,
                                int run_callback);
    int (*config_option_set) (struct t_config_option *option, const char *value,
                              int run_callback);
    int (*config_option_unset) (struct t_config_option *option);
    void (*config_option_rename) (struct t_config_option *option,
                                  const char *new_name);
    void *(*config_option_get_pointer) (struct t_config_option *option,
                                        const char *property);
    int (*config_boolean) (struct t_config_option *option);
    int (*config_integer) (struct t_config_option *option);
    char *(*config_string) (struct t_config_option *option);
    int (*config_color) (struct t_config_option *option);
    void (*config_write_line) (struct t_config_file *config_file,
                               const char *option_name, const char *value, ...);
    int (*config_write) (struct t_config_file *config_file);
    int (*config_read) (struct t_config_file *config_file);
    int (*config_reload) (struct t_config_file *config_file);
    void (*config_option_free) (struct t_config_option *option);
    void (*config_section_free_options) (struct t_config_section *section);
    void (*config_section_free) (struct t_config_file *config_file,
                                 struct t_config_section *section);
    void (*config_free) (struct t_config_file *config_file);
    struct t_config_option *(*config_get) (const char *option_name);
    char *(*config_get_plugin) (struct t_weechat_plugin *plugin,
                                const char *option_name);
    int (*config_set_plugin) (struct t_weechat_plugin *plugin,
                              const char *option_name, const char *value);
    
    /* display */
    char *(*prefix) (const char *prefix);
    char *(*color) (const char *color_name);
    void (*printf_date_tags) (struct t_gui_buffer *buffer, time_t date,
                              const char *tags, const char *message, ...);
    void (*printf_y) (struct t_gui_buffer *buffer, int y,
                      const char *message, ...);
    void (*log_printf) (const char *message, ...);
    
    /* hooks */
    struct t_hook *(*hook_command) (struct t_weechat_plugin *plugin,
                                    const char *command, const char *description,
                                    const char *args, const char *args_description,
                                    const char *completion,
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
    struct t_hook *(*hook_connect) (struct t_weechat_plugin *plugin,
                                    const char *address, int port,
                                    int sock, int ipv6, void *gnutls_sess,
                                    const char *local_hostname,
                                    int (*callback)(void *data, int status),
                                    void *callback_data);
    struct t_hook *(*hook_print) (struct t_weechat_plugin *plugin,
                                  struct t_gui_buffer *buffer,
                                  const char *tags, const char *message,
                                  int strip_colors,
                                  int (*callback)(void *data,
                                                  struct t_gui_buffer *buffer,
                                                  time_t date,
                                                  int tags_count,
                                                  char **tags,
                                                  const char *prefix, const char *message),
                                  void *callback_data);
    struct t_hook *(*hook_signal) (struct t_weechat_plugin *plugin,
                                   const char *signal,
                                   int (*callback)(void *data, const char *signal,
                                                   const char *type_data,
                                                   void *signal_data),
                                   void *callback_data);
    void (*hook_signal_send) (const char *signal, const char *type_data,
                              void *signal_data);
    struct t_hook *(*hook_config) (struct t_weechat_plugin *plugin,
                                   const char *option,
                                   int (*callback)(void *data, const char *option,
                                                   const char *value),
                                   void *callback_data);
    struct t_hook *(*hook_completion) (struct t_weechat_plugin *plugin,
                                       const char *completion_item,
                                       int (*callback)(void *data,
                                                       const char *completion_item,
                                                       struct t_gui_buffer *buffer,
                                                       struct t_gui_completion *completion),
                                       void *callback_data);
    void (*hook_completion_list_add) (struct t_gui_completion *completion,
                                      const char *word, int nick_completion,
                                      const char *where);
    struct t_hook *(*hook_modifier) (struct t_weechat_plugin *plugin,
                                     const char *modifier,
                                     char *(*callback)(void *data,
                                                       const char *modifier,
                                                       const char *modifier_data,
                                                       const char *string),
                                     void *callback_data);
    char *(*hook_modifier_exec) (struct t_weechat_plugin *plugin,
                                 const char *modifier, const char *modifier_data,
                                 const char *string);
    void (*unhook) (struct t_hook *hook);
    void (*unhook_all) (struct t_weechat_plugin *plugin);
    
    /* buffers */
    struct t_gui_buffer *(*buffer_new) (struct t_weechat_plugin *plugin,
                                        const char *category, const char *name,
                                        int (*input_callback)(void *data,
                                                              struct t_gui_buffer *buffer,
                                                              const char *input_data),
                                        void *input_callback_data,
                                        int (*close_callback)(void *data,
                                                              struct t_gui_buffer *buffer),
                                        void *close_callback_data);
    struct t_gui_buffer *(*buffer_search) (const char *category, const char *name);
    void (*buffer_clear) (struct t_gui_buffer *buffer);
    void (*buffer_close) (struct t_gui_buffer *buffer, int switch_to_another);
    int (*buffer_get_integer) (struct t_gui_buffer *buffer, const char *property);
    char *(*buffer_get_string) (struct t_gui_buffer *buffer, const char *property);
    void *(*buffer_get_pointer) (struct t_gui_buffer *buffer, const char *property);
    void (*buffer_set) (struct t_gui_buffer *buffer, const char *property,
                        void *value);
    
    /* nicklist */
    struct t_gui_nick_group *(*nicklist_add_group) (struct t_gui_buffer *buffer,
                                                    struct t_gui_nick_group *parent_group,
                                                    const char *name, const char *color,
                                                    int visible);
    struct t_gui_nick_group *(*nicklist_search_group) (struct t_gui_buffer *buffer,
                                                       struct t_gui_nick_group *from_group,
                                                       const char *name);
    struct t_gui_nick *(*nicklist_add_nick) (struct t_gui_buffer *buffer,
                                             struct t_gui_nick_group *group,
                                             const char *name, const char *color,
                                             char prefix, const char *prefix_color,
                                             int visible);
    struct t_gui_nick *(*nicklist_search_nick) (struct t_gui_buffer *buffer,
                                                struct t_gui_nick_group *from_group,
                                                const char *name);
    void (*nicklist_remove_group) (struct t_gui_buffer *buffer,
                                   struct t_gui_nick_group *group);
    void (*nicklist_remove_nick) (struct t_gui_buffer *buffer,
                                  struct t_gui_nick *nick);
    void (*nicklist_remove_all) (struct t_gui_buffer *buffer);
    
    /* bars */
    struct t_gui_bar_item *(*bar_item_search) (const char *name);
    struct t_gui_bar_item *(*bar_item_new) (struct t_weechat_plugin *plugin,
                                            const char *name,
                                            char *(*build_callback)(void *data,
                                                                    struct t_gui_bar_item *item,
                                                                    struct t_gui_window *window,
                                                                    int max_width, int max_height),
                                            void *build_callback_data);
    void (*bar_item_update) (const char *name);
    void (*bar_item_remove) (struct t_gui_bar_item *item);
    struct t_gui_bar *(*bar_search) (const char *name);
    struct t_gui_bar *(*bar_new) (struct t_weechat_plugin *plugin,
                                  const char *name,
                                  const char *hidden,
                                  const char *priority,
                                  const char *type,
                                  const char *condition,
                                  const char *position,
                                  const char *filling_top_bottom,
                                  const char *filling_left_right,
                                  const char *size,
                                  const char *size_max,
                                  const char *color_fg,
                                  const char *color_delim,
                                  const char *color_bg,
                                  const char *separator,
                                  const char *items);
    int (*bar_set) (struct t_gui_bar *bar, const char *property,
                    const char *value);
    void (*bar_update) (const char *name);
    void (*bar_remove) (struct t_gui_bar *bar);
    
    /* command */
    void (*command) (struct t_weechat_plugin *plugin,
                     struct t_gui_buffer *buffer, const char *command);

    /* network */
    int (*network_pass_proxy) (int sock, const char *address, int port);
    int (*network_connect_to) (int sock, unsigned long address, int port);
    
    /* infos */
    char *(*info_get) (struct t_weechat_plugin *plugin, const char *info);
    
    /* infolists */
    struct t_infolist *(*infolist_new) ();
    struct t_infolist_item *(*infolist_new_item) (struct t_infolist *infolist);
    struct t_infolist_var *(*infolist_new_var_integer) (struct t_infolist_item *item,
                                                        const char *name,
                                                        int value);
    struct t_infolist_var *(*infolist_new_var_string) (struct t_infolist_item *item,
                                                       const char *name,
                                                       const char *value);
    struct t_infolist_var *(*infolist_new_var_pointer) (struct t_infolist_item *item,
                                                        const char *name,
                                                        void *pointer);
    struct t_infolist_var *(*infolist_new_var_buffer) (struct t_infolist_item *item,
                                                       const char *name,
                                                       void *pointer,
                                                       int size);
    struct t_infolist_var *(*infolist_new_var_time) (struct t_infolist_item *item,
                                                     const char *name,
                                                     time_t time);
    struct t_infolist *(*infolist_get) (const char *name, void *pointer,
                                        const char *arguments);
    int (*infolist_next) (struct t_infolist *infolist);
    int (*infolist_prev) (struct t_infolist *infolist);
    void (*infolist_reset_item_cursor) (struct t_infolist *infolist);
    char *(*infolist_fields) (struct t_infolist *infolist);
    int (*infolist_integer) (struct t_infolist *infolist, const char *var);
    char *(*infolist_string) (struct t_infolist *infolist, const char *var);
    void *(*infolist_pointer) (struct t_infolist *infolist, const char *var);
    void *(*infolist_buffer) (struct t_infolist *infolist, const char *var,
                              int *size);
    time_t (*infolist_time) (struct t_infolist *infolist, const char *var);
    void (*infolist_free) (struct t_infolist *infolist);

    /* upgrade */
    struct t_upgrade_file *(*upgrade_create) (const char *filename,
                                              int write);
    int (*upgrade_write_object) (struct t_upgrade_file *upgrade_file,
                                 int object_id,
                                 struct t_infolist *infolist);
    int (*upgrade_read) (struct t_upgrade_file *upgrade_file,
                         int (*callback_read)(int object_id,
                                              struct t_infolist *infolist));
    void (*upgrade_close) (struct t_upgrade_file *upgrade_file);
    
    /* WeeChat developers: ALWAYS add new functions at the end */
};

extern int weechat_plugin_init (struct t_weechat_plugin *plugin,
                                int argc, char *argv[]);
extern int weechat_plugin_end (struct t_weechat_plugin *plugin);

/* macros for easy call to plugin API */

/* strings */
#define weechat_charset_set(__charset)                          \
    weechat_plugin->charset_set(weechat_plugin, __charset)
#define weechat_iconv_to_internal(__charset, __string)          \
    weechat_plugin->iconv_to_internal(__charset, __string)
#define weechat_iconv_from_internal(__charset, __string)        \
    weechat_plugin->iconv_from_internal(__charset, __string)
#ifndef __WEECHAT_H
#ifndef _
#define _(string) weechat_plugin->gettext(string)
#define N_(string) (string)
#define NG_(single,plural,number)                       \
    weechat_plugin->ngettext(single, plural, number)
#endif
#endif
#define weechat_gettext(string) weechat_plugin->gettext(string)
#define weechat_ngettext(single,plural,number)          \
    weechat_plugin->ngettext(single, plural, number)
#define weechat_strndup(__string, __length)     \
    weechat_plugin->strndup(__string, __length)
#define weechat_string_tolower(__string)        \
    weechat_plugin->string_tolower(__string)
#define weechat_string_toupper(__string)        \
    weechat_plugin->string_toupper(__string)
#define weechat_strcasecmp(__string1, __string2)        \
    weechat_plugin->strcasecmp(__string1, __string2)
#define weechat_strncasecmp(__string1, __string2, __max)        \
    weechat_plugin->strncasecmp(__string1, __string2, __max)
#define weechat_strcmp_ignore_chars(__string1, __string2,               \
                                    __chars_ignored, __case_sensitive)  \
    weechat_plugin->strcmp_ignore_chars(__string1, __string2,           \
                                        __chars_ignored,                \
                                        __case_sensitive)
#define weechat_strcasestr(__string1, __string2)        \
    weechat_plugin->strcasestr(__string1, __string2)
#define weechat_string_match(__string, __mask, __case_sensitive)        \
    weechat_plugin->string_match(__string, __mask, __case_sensitive)
#define weechat_string_replace(__string, __search, __replace)           \
    weechat_plugin->string_replace(__string, __search, __replace)
#define weechat_string_remove_quotes(__string, __quotes)        \
    weechat_plugin->string_remove_quotes(__string, __quotes)
#define weechat_string_strip(__string, __left, __right, __chars)        \
    weechat_plugin->string_strip(__string, __left, __right, __chars)
#define weechat_string_has_highlight(__string, __highlight_words)       \
    weechat_plugin->string_has_highlight(__string, __highlight_words)
#define weechat_string_explode(__string, __separator, __eol, __max,     \
                               __num_items)                             \
    weechat_plugin->string_explode(__string, __separator, __eol,        \
                                   __max, __num_items)
#define weechat_string_free_exploded(__exploded_string)         \
    weechat_plugin->string_free_exploded(__exploded_string)
#define weechat_string_build_with_exploded(__exploded_string,           \
                                           __separator)                 \
    weechat_plugin->string_build_with_exploded(__exploded_string,       \
                                               __separator)
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
#define weechat_utf8_charcasecmp(__string1, __string2)          \
    weechat_plugin->utf8_charcasecmp(__string1, __string2)
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
#define weechat_list_set(__item, __value)               \
    weechat_plugin->list_set(__item, __value)
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
#define weechat_config_new(__name, __callback_reload,                   \
                           __callback_reload_data)                      \
    weechat_plugin->config_new(weechat_plugin, __name,                  \
                               __callback_reload,                       \
                               __callback_reload_data)
#define weechat_config_new_section(__config, __name,                    \
                                   __user_can_add_options,              \
                                   __user_can_delete_options,           \
                                   __cb_read, __cb_read_data,           \
                                   __cb_write_std, __cb_write_std_data, \
                                   __cb_write_def, __cb_write_def_data, \
                                   __cb_create_option,                  \
                                   __cb_create_option_data)             \
    weechat_plugin->config_new_section(__config, __name,                \
                                       __user_can_add_options,          \
                                       __user_can_delete_options,       \
                                       __cb_read, __cb_read_data,       \
                                       __cb_write_std,                  \
                                       __cb_write_std_data,             \
                                       __cb_write_def,                  \
                                       __cb_write_def_data,             \
                                       __cb_create_option,              \
                                       __cb_create_option_data)
#define weechat_config_search_section(__config, __name)         \
    weechat_plugin->config_search_section(__config, __name)
#define weechat_config_new_option(__config, __section, __name, __type,  \
                                  __desc, __string_values, __min,       \
                                  __max, __default, __callback_check,   \
                                  __callback_check_data,                \
                                  __callback_change,                    \
                                  __callback_change_data,               \
                                  __callback_delete,                    \
                                  __callback_delete_data)               \
    weechat_plugin->config_new_option(__config, __section, __name,      \
                                      __type, __desc, __string_values,  \
                                      __min, __max, __default,          \
                                      __callback_check,                 \
                                      __callback_check_data,            \
                                      __callback_change,                \
                                      __callback_change_data,           \
                                      __callback_delete,                \
                                      __callback_delete_data)
#define weechat_config_search_option(__config, __section, __name)       \
    weechat_plugin->config_search_option(__config, __section, __name)
#define weechat_config_search_section_option(__config, __section,       \
                                             __name, __section_found,   \
                                             __option_found)            \
    weechat_plugin->config_search_section_option(__config, __section,   \
                                                 __name,                \
                                                 __section_found,       \
                                                 __option_found);
#define weechat_config_search_with_string(__name, __config, __section,  \
                                          __option, __pos_option)       \
    weechat_plugin->config_search_with_string(__name, __config,         \
                                              __section, __option,      \
                                              __pos_option);
#define weechat_config_string_to_boolean(__string)      \
    weechat_plugin->config_string_to_boolean(__string)
#define weechat_config_option_reset(__option, __run_callback)           \
    weechat_plugin->config_option_reset(__option, __run_callback)
#define weechat_config_option_set(__option, __value, __run_callback)    \
    weechat_plugin->config_option_set(__option, __value,                \
                                      __run_callback)
#define weechat_config_option_unset(__option)                           \
    weechat_plugin->config_option_unset(__option)
#define weechat_config_option_rename(__option, __new_name)      \
    weechat_plugin->config_option_rename(__option, __new_name)
#define weechat_config_option_get_pointer(__option, __property)         \
    weechat_plugin->config_option_get_pointer(__option, __property)
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
#define weechat_config_option_free(__option)            \
    weechat_plugin->config_option_free(__option)
#define weechat_config_section_free_options(__section)          \
    weechat_plugin->config_section_free_options(__section)
#define weechat_config_section_free(__config, __section)        \
    weechat_plugin->config_section_free(__config, __section)
#define weechat_config_free(__config)           \
    weechat_plugin->config_free(__config)
#define weechat_config_get(__option)            \
    weechat_plugin->config_get(__option)
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
    weechat_plugin->printf_date_tags(__buffer, 0, NULL, __message,      \
                                     ##__argz)
#define weechat_printf_y(__buffer, __y, __message, __argz...)     \
    weechat_plugin->printf_y(__buffer, __y, __message, ##__argz)
#define weechat_printf_date(__buffer, __date, __message, __argz...)     \
    weechat_plugin->printf_date_tags(__buffer, __date, NULL,            \
                                     __message, ##__argz)
#define weechat_printf_tags(__buffer, __tags, __message, __argz...)     \
    weechat_plugin->printf_date_tags(__buffer, 0, __tags, __message,    \
                                     ##__argz)
#define weechat_printf_date_tags(__buffer, __date, __tags, __message,   \
                                 __argz...)                             \
    weechat_plugin->printf_date_tags(__buffer, __date, __tags,          \
                                     __message, ##__argz)
#define weechat_log_printf(__message, __argz...)        \
    weechat_plugin->log_printf(__message, ##__argz)

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
#define weechat_hook_connect(__address, __port, __sock, __ipv6,         \
                             __gnutls_sess, __local_hostname,           \
                             __callback, __data)                        \
    weechat_plugin->hook_connect(weechat_plugin, __address, __port,     \
                                 __sock, __ipv6, __gnutls_sess,         \
                                 __local_hostname, __callback, __data)
#define weechat_hook_print(__buffer, __tags, __msg, __strip__colors,    \
                           __callback, __data)                          \
    weechat_plugin->hook_print(weechat_plugin, __buffer, __tags,        \
                               __msg, __strip__colors, __callback,      \
                               __data)
#define weechat_hook_signal(__signal, __callback, __data)               \
    weechat_plugin->hook_signal(weechat_plugin, __signal, __callback,   \
                                __data)
#define weechat_hook_signal_send(__signal, __type_data, __signal_data)  \
    weechat_plugin->hook_signal_send(__signal, __type_data,             \
                                     __signal_data)
#define weechat_hook_config(__option, __callback, __data)               \
    weechat_plugin->hook_config(weechat_plugin, __option, __callback,   \
                                __data)
#define weechat_hook_completion(__completion, __callback, __data)       \
    weechat_plugin->hook_completion(weechat_plugin, __completion,       \
                                    __callback, __data)
#define weechat_hook_completion_list_add(__completion, __word,          \
                                         __nick_completion, __where)    \
    weechat_plugin->hook_completion_list_add(__completion, __word,      \
                                             __nick_completion,         \
                                             __where)
#define weechat_hook_modifier(__modifier, __callback, __data)   \
    weechat_plugin->hook_modifier(weechat_plugin, __modifier,   \
                                  __callback, __data)
#define weechat_hook_modifier_exec(__modifier, __modifier_data,         \
                                   __string)                            \
    weechat_plugin->hook_modifier_exec(weechat_plugin, __modifier,      \
                                       __modifier_data, __string)
#define weechat_unhook(__hook)                  \
    weechat_plugin->unhook( __hook)
#define weechat_unhook_all()                            \
    weechat_plugin->unhook_all_plugin(weechat_plugin)

/* buffers */
#define weechat_buffer_new(__category, __name, __input_callback,        \
                           __input_callback_data, __close_callback,     \
                           __close_callback_data)                       \
    weechat_plugin->buffer_new(weechat_plugin, __category, __name,      \
                               __input_callback, __input_callback_data, \
                               __close_callback, __close_callback_data)
#define weechat_buffer_search(__category, __name)       \
    weechat_plugin->buffer_search(__category, __name)
#define weechat_current_buffer                  \
    weechat_plugin->buffer_search(NULL, NULL)
#define weechat_buffer_clear(__buffer)          \
    weechat_plugin->buffer_clear(__buffer)
#define weechat_buffer_close(__buffer, __switch_to_another)     \
    weechat_plugin->buffer_close(__buffer, __switch_to_another)
#define weechat_buffer_get_integer(__buffer, __property)         \
    weechat_plugin->buffer_get_integer(__buffer, __property)
#define weechat_buffer_get_string(__buffer, __property)        \
    weechat_plugin->buffer_get_string(__buffer, __property)
#define weechat_buffer_get_pointer(__buffer, __property)        \
    weechat_plugin->buffer_get_pointer(__buffer, __property)
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

/* bars */
#define weechat_bar_item_search(__name)         \
    weechat_plugin->bar_item_search(__name)
#define weechat_bar_item_new(__name, __build_callback, __data)          \
    weechat_plugin->bar_item_new(weechat_plugin, __name,                \
                                 __build_callback, __data)
#define weechat_bar_item_update(__name)         \
    weechat_plugin->bar_item_update(__name)
#define weechat_bar_item_remove(__item)         \
    weechat_plugin->bar_item_remove(__item)
#define weechat_bar_search(__name)              \
    weechat_plugin->bar_search(__name)
#define weechat_bar_new(__name, __hidden, __priority, __type,          \
                        __condition, __position, __filling_top_bottom, \
                        __filling_left_right, __size, __size_max,      \
                        __color_fg, __color_delim, __color_bg,         \
                        __separator, __items)                          \
    weechat_plugin->bar_new(weechat_plugin, __name, __hidden,          \
                            __priority, __type, __condition,           \
                            __position, __filling_top_bottom,          \
                            __filling_left_right, __size, __size_max,  \
                            __color_fg, __color_delim, __color_bg,     \
                            __separator, __items)
#define weechat_bar_set(__bar, __property, __value)     \
    weechat_plugin->bar_set(__bar, __property, __value)
#define weechat_bar_update(__name)              \
    weechat_plugin->bar_update(__name)
#define weechat_bar_remove(__bar)               \
    weechat_plugin->bar_remove(__bar)

/* command */
#define weechat_command(__buffer, __command)                            \
    weechat_plugin->command(weechat_plugin, __buffer, __command)

/* network */
#define weechat_network_pass_proxy(__sock, __address, __port)           \
    weechat_plugin->network_pass_proxy(__sock, __address, __port)
#define weechat_network_connect_to(__sock, __address, __port)           \
    weechat_plugin->network_connect_to(__sock, __address, __port)

/* infos */
#define weechat_info_get(__name)                        \
    weechat_plugin->info_get(weechat_plugin, __name)

/* infolists */
#define weechat_infolist_new()                  \
    weechat_plugin->infolist_new()
#define weechat_infolist_new_item(__list)       \
    weechat_plugin->infolist_new_item(__list)
#define weechat_infolist_new_var_integer(__item, __name, __value)       \
    weechat_plugin->infolist_new_var_integer(__item, __name, __value)
#define weechat_infolist_new_var_string(__item, __name, __value)        \
    weechat_plugin->infolist_new_var_string(__item, __name, __value)
#define weechat_infolist_new_var_pointer(__item, __name, __pointer)     \
    weechat_plugin->infolist_new_var_pointer(__item, __name, __pointer)
#define weechat_infolist_new_var_buffer(__item, __name, __buffer,       \
                                        __size)                         \
    weechat_plugin->infolist_new_var_buffer(__item, __name, __buffer,   \
                                            __size)
#define weechat_infolist_new_var_time(__item, __name, __time)           \
    weechat_plugin->infolist_new_var_time(__item, __name, __time)
#define weechat_infolist_get(__name, __pointer, __arguments)            \
    weechat_plugin->infolist_get(__name, __pointer, __arguments)
#define weechat_infolist_next(__list)                                   \
    weechat_plugin->infolist_next(__list)
#define weechat_infolist_prev(__list)                                   \
    weechat_plugin->infolist_prev(__list)
#define weechat_infolist_reset_item_cursor(__list)                      \
    weechat_plugin->infolist_reset_item_cursor(__list)
#define weechat_infolist_fields(__list)                                 \
    weechat_plugin->infolist_fields(__list)
#define weechat_infolist_integer(__item, __var)                         \
    weechat_plugin->infolist_integer(__item, __var)
#define weechat_infolist_string(__item, __var)                          \
    weechat_plugin->infolist_string(__item, __var)
#define weechat_infolist_pointer(__item, __var)                         \
    weechat_plugin->infolist_pointer(__item, __var)
#define weechat_infolist_buffer(__item, __var, __size)                  \
    weechat_plugin->infolist_buffer(__item, __var, __size)
#define weechat_infolist_time(__item, __var)                            \
    weechat_plugin->infolist_time(__item, __var)
#define weechat_infolist_free(__list)                                   \
    weechat_plugin->infolist_free(__list)

/* upgrade */
#define weechat_upgrade_create(__filename, __write)                     \
    weechat_plugin->upgrade_create(__filename, __write)
#define weechat_upgrade_write_object(__upgrade_file, __object_id,       \
                                     __infolist)                        \
    weechat_plugin->upgrade_write_object(__upgrade_file, __object_id,   \
                                         __infolist)
#define weechat_upgrade_read(__upgrade_file, __callback)                \
    weechat_plugin->upgrade_read(__upgrade_file, __callback)
#define weechat_upgrade_close(__upgrade_file)                           \
    weechat_plugin->upgrade_close(__upgrade_file)

#endif /* weechat-plugin.h */
