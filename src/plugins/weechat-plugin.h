/*
 * weechat-plugin.h - header to compile WeeChat plugins
 *
 * Copyright (C) 2003-2023 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_WEECHAT_PLUGIN_H
#define WEECHAT_WEECHAT_PLUGIN_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stddef.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>

/* some systems like GNU/Hurd do not define PATH_MAX */
#ifndef PATH_MAX
    #define PATH_MAX 4096
#endif /* PATH_MAX */

struct t_config_option;
struct t_config_section;
struct t_config_file;
struct t_gui_window;
struct t_gui_buffer;
struct t_gui_bar;
struct t_gui_bar_item;
struct t_gui_bar_window;
struct t_gui_completion;
struct t_gui_nick;
struct t_gui_nick_group;
struct t_infolist;
struct t_infolist_item;
struct t_upgrade_file;
struct t_weelist;
struct t_weelist_item;
struct t_arraylist;
struct t_hashtable;
struct t_hdata;
struct timeval;

/*
 * IMPORTANT NOTE for WeeChat developers: if you update, add or remove
 * some functions in this file, then please update API version below.
 */

/*
 * API version (used to check that plugin has same API and can be loaded):
 * please change the date with current one; for a second change at same
 * date, increment the 01, otherwise please keep 01.
 */
#define WEECHAT_PLUGIN_API_VERSION "20230220-01"

/* macros for defining plugin infos */
#define WEECHAT_PLUGIN_NAME(__name)                                     \
    char weechat_plugin_name[] = __name;                                \
    char weechat_plugin_api_version[] = WEECHAT_PLUGIN_API_VERSION;
#define WEECHAT_PLUGIN_AUTHOR(__author)         \
    char weechat_plugin_author[] = __author;
#define WEECHAT_PLUGIN_DESCRIPTION(__desc)      \
    char weechat_plugin_description[] = __desc;
#define WEECHAT_PLUGIN_VERSION(__version)       \
    char weechat_plugin_version[] = __version;
#define WEECHAT_PLUGIN_LICENSE(__license)       \
    char weechat_plugin_license[] = __license;
#define WEECHAT_PLUGIN_PRIORITY(__priority)     \
    int weechat_plugin_priority = __priority;

/* return codes for plugin functions */
#define WEECHAT_RC_OK                               0
#define WEECHAT_RC_OK_EAT                           1
#define WEECHAT_RC_ERROR                           -1

/* flags for string_split function */
#define WEECHAT_STRING_SPLIT_STRIP_LEFT            (1 << 0)
#define WEECHAT_STRING_SPLIT_STRIP_RIGHT           (1 << 1)
#define WEECHAT_STRING_SPLIT_COLLAPSE_SEPS         (1 << 2)
#define WEECHAT_STRING_SPLIT_KEEP_EOL              (1 << 3)

/* return codes for config read functions/callbacks */
#define WEECHAT_CONFIG_READ_OK                      0
#define WEECHAT_CONFIG_READ_MEMORY_ERROR           -1
#define WEECHAT_CONFIG_READ_FILE_NOT_FOUND         -2

/* return codes for config write functions/callbacks */
#define WEECHAT_CONFIG_WRITE_OK                     0
#define WEECHAT_CONFIG_WRITE_ERROR                 -1
#define WEECHAT_CONFIG_WRITE_MEMORY_ERROR          -2

/* null value for option */
#define WEECHAT_CONFIG_OPTION_NULL                 "null"

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

/* type for keys and values in hashtable */
#define WEECHAT_HASHTABLE_INTEGER                   "integer"
#define WEECHAT_HASHTABLE_STRING                    "string"
#define WEECHAT_HASHTABLE_POINTER                   "pointer"
#define WEECHAT_HASHTABLE_BUFFER                    "buffer"
#define WEECHAT_HASHTABLE_TIME                      "time"

/* types for hdata */
#define WEECHAT_HDATA_OTHER                         0
#define WEECHAT_HDATA_CHAR                          1
#define WEECHAT_HDATA_INTEGER                       2
#define WEECHAT_HDATA_LONG                          3
#define WEECHAT_HDATA_STRING                        4
#define WEECHAT_HDATA_POINTER                       5
#define WEECHAT_HDATA_TIME                          6
#define WEECHAT_HDATA_HASHTABLE                     7
#define WEECHAT_HDATA_SHARED_STRING                 8

/* flags for hdata lists */
#define WEECHAT_HDATA_LIST_CHECK_POINTERS           1

/* buffer hotlist */
#define WEECHAT_HOTLIST_LOW                         "0"
#define WEECHAT_HOTLIST_MESSAGE                     "1"
#define WEECHAT_HOTLIST_PRIVATE                     "2"
#define WEECHAT_HOTLIST_HIGHLIGHT                   "3"

/*
 * process return code (for callback):
 *   if >= 0, the process ended and it's return code of command
 *   if -1, the process is still running
 *   if -2, the process ended with an error
 *   if -3, the callback is called in the child process (exec of function)
 *          (note: the return code -3 is NEVER sent to script plugins,
 *           it can be used only in C API)
 */
#define WEECHAT_HOOK_PROCESS_RUNNING                -1
#define WEECHAT_HOOK_PROCESS_ERROR                  -2
#define WEECHAT_HOOK_PROCESS_CHILD                  -3

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
#define WEECHAT_HOOK_CONNECT_TIMEOUT                9
#define WEECHAT_HOOK_CONNECT_SOCKET_ERROR           10

/* action for gnutls callback: verify or set certificate */
#define WEECHAT_HOOK_CONNECT_GNUTLS_CB_VERIFY_CERT  0
#define WEECHAT_HOOK_CONNECT_GNUTLS_CB_SET_CERT     1

/* type of data for signal hooked */
#define WEECHAT_HOOK_SIGNAL_STRING                  "string"
#define WEECHAT_HOOK_SIGNAL_INT                     "int"
#define WEECHAT_HOOK_SIGNAL_POINTER                 "pointer"

/* macro to format string with variable args, using dynamic buffer size */
#define weechat_va_format(__format)                                     \
    va_list argptr;                                                     \
    int vaa_size, vaa_num;                                              \
    char *vbuffer, *vaa_buffer2;                                        \
    vaa_size = 1024;                                                    \
    vbuffer = malloc (vaa_size);                                        \
    if (vbuffer)                                                        \
    {                                                                   \
        while (1)                                                       \
        {                                                               \
            va_start (argptr, __format);                                \
            vaa_num = vsnprintf (vbuffer, vaa_size, __format, argptr);  \
            va_end (argptr);                                            \
            if ((vaa_num >= 0) && (vaa_num < vaa_size))                 \
                break;                                                  \
            vaa_size = (vaa_num >= 0) ? vaa_num + 1 : vaa_size * 2;     \
            vaa_buffer2 = realloc (vbuffer, vaa_size);                  \
            if (!vaa_buffer2)                                           \
            {                                                           \
                free (vbuffer);                                         \
                vbuffer = NULL;                                         \
                break;                                                  \
            }                                                           \
            vbuffer = vaa_buffer2;                                      \
        }                                                               \
    }

/*
 * macro to return error in case of missing arguments in callback of
 * hook_command
 */
#define WEECHAT_COMMAND_MIN_ARGS(__min_args, __option)                  \
    if (argc < __min_args)                                              \
    {                                                                   \
        weechat_printf_date_tags (                                      \
            NULL, 0, "no_filter",                                       \
            _("%sToo few arguments for command \"%s%s%s\" "             \
              "(help on command: /help %s)"),                           \
            weechat_prefix ("error"),                                   \
            argv[0],                                                    \
            (__option && __option[0]) ? " " : "",                       \
            (__option && __option[0]) ? __option : "",                  \
            argv[0] + 1);                                               \
        return WEECHAT_RC_ERROR;                                        \
    }

/* macro to return error in callback of hook_command */
#define WEECHAT_COMMAND_ERROR                                           \
    {                                                                   \
        weechat_printf_date_tags (                                      \
            NULL, 0, "no_filter",                                       \
            _("%sError with command \"%s\" "                            \
              "(help on command: /help %s)"),                           \
            weechat_prefix ("error"),                                   \
            argv_eol[0],                                                \
            argv[0] + 1);                                               \
        return WEECHAT_RC_ERROR;                                        \
    }

/* macro to convert integer to string */
#define TO_STR_HELPER(x) #x
#define TO_STR(x) TO_STR_HELPER(x)

struct t_weechat_plugin
{
    /* plugin variables */
    char *filename;                    /* name of plugin on disk            */
    void *handle;                      /* handle of plugin (given by dlopen)*/
    char *name;                        /* short name                        */
    char *description;                 /* description                       */
    char *author;                      /* author                            */
    char *version;                     /* plugin version                    */
    char *license;                     /* license                           */
    char *charset;                     /* charset used by plugin            */
    int priority;                      /* plugin priority (default is 1000) */
    int initialized;                   /* plugin initialized? (init called) */
    int debug;                         /* debug level for plugin (0=off)    */
    int upgrading;                     /* 1 if the plugin must load upgrade */
                                       /* info on startup (if weechat is    */
                                       /* run with --upgrade)               */
    struct t_hashtable *variables;     /* plugin custom variables           */
    struct t_weechat_plugin *prev_plugin; /* link to previous plugin        */
    struct t_weechat_plugin *next_plugin; /* link to next plugin            */

    /*
     * plugin functions (API)
     * WeeChat developers: if you add functions in API, update value of
     * constant WEECHAT_PLUGIN_API_VERSION
     */

    /* plugins */
    const char *(*plugin_get_name) (struct t_weechat_plugin *plugin);

    /* strings */
    void (*charset_set) (struct t_weechat_plugin *plugin, const char *charset);
    char *(*iconv_to_internal) (const char *charset, const char *string);
    char *(*iconv_from_internal) (const char *charset, const char *string);
    const char *(*gettext) (const char *string);
    const char *(*ngettext) (const char *single, const char *plural, int count);
    char *(*strndup) (const char *string, int bytes);
    char *(*string_cut) (const char *string, int length, int count_suffix,
                         int screen, const char *cut_suffix);
    char *(*string_tolower) (const char *string);
    char *(*string_toupper) (const char *string);
    int (*string_charcmp) (const char *string1, const char *string2);
    int (*string_charcasecmp) (const char *string1, const char *string2);
    int (*strcmp) (const char *string1, const char *string2);
    int (*strncmp) (const char *string1, const char *string2, int max);
    int (*strcasecmp) (const char *string1, const char *string2);
    int (*strcasecmp_range) (const char *string1, const char *string2,
                             int range);
    int (*strncasecmp) (const char *string1, const char *string2, int max);
    int (*strncasecmp_range) (const char *string1, const char *string2,
                              int max, int range);
    int (*strcmp_ignore_chars) (const char *string1, const char *string2,
                                const char *chars_ignored, int case_sensitive);
    const char *(*strcasestr) (const char *string, const char *search);
    int (*strlen_screen) (const char *string);
    int (*string_match) (const char *string, const char *mask,
                         int case_sensitive);
    int (*string_match_list) (const char *string, const char **masks,
                              int case_sensitive);
    char *(*string_replace) (const char *string, const char *search,
                             const char *replace);
    char *(*string_expand_home) (const char *path);
    char *(*string_eval_path_home) (const char *path,
                                    struct t_hashtable *pointers,
                                    struct t_hashtable *extra_vars,
                                    struct t_hashtable *options);
    char *(*string_remove_quotes) (const char *string, const char *quotes);
    char *(*string_strip) (const char *string, int left, int right,
                           const char *chars);
    char *(*string_convert_escaped_chars) (const char *string);
    char *(*string_mask_to_regex) (const char *mask);
    const char *(*string_regex_flags) (const char *regex, int default_flags,
                                       int *flags);
    int (*string_regcomp) (void *preg, const char *regex, int default_flags);
    int (*string_has_highlight) (const char *string,
                                 const char *highlight_words);
    int (*string_has_highlight_regex) (const char *string, const char *regex);
    char *(*string_replace_regex) (const char *string, void *regex,
                                   const char *replace,
                                   const char reference_char,
                                   char *(*callback)(void *data,
                                                     const char *text),
                                   void *callback_data);
    char *(*string_translate_chars) (const char *string, const char *chars1,
                                     const char *chars2);
    char **(*string_split) (const char *string, const char *separators,
                            const char *strip_items, int flags,
                            int num_items_max, int *num_items);
    char **(*string_split_shell) (const char *string, int *num_items);
    void (*string_free_split) (char **split_string);
    char *(*string_rebuild_split_string) (const char **split_string,
                                          const char *separator,
                                          int index_start, int index_end);
    char **(*string_split_command) (const char *command, char separator);
    void (*string_free_split_command) (char **split_command);
    char *(*string_format_size) (unsigned long long size);
    unsigned long long (*string_parse_size) (const char *size);
    int (*string_color_code_size) (const char *string);
    char *(*string_remove_color) (const char *string, const char *replacement);
    int (*string_base_encode) (int base, const char *from, int length,
                               char *to);
    int (*string_base_decode) (int base, const char *from, char *to);
    char *(*string_hex_dump) (const char *data, int data_size,
                              int bytes_per_line, const char *prefix,
                              const char *suffix);
    int (*string_is_command_char) (const char *string);
    const char *(*string_input_for_buffer) (const char *string);
    char *(*string_eval_expression )(const char *expr,
                                     struct t_hashtable *pointers,
                                     struct t_hashtable *extra_vars,
                                     struct t_hashtable *options);
    char **(*string_dyn_alloc) (int size_alloc);
    int (*string_dyn_copy) (char **string, const char *new_string);
    int (*string_dyn_concat) (char **string, const char *add, int bytes);
    char *(*string_dyn_free) (char **string, int free_string);

    /* UTF-8 strings */
    int (*utf8_has_8bits) (const char *string);
    int (*utf8_is_valid) (const char *string, int length, char **error);
    void (*utf8_normalize) (char *string, char replacement);
    const char *(*utf8_prev_char) (const char *string_start,
                                   const char *string);
    const char *(*utf8_next_char) (const char *string);
    int (*utf8_char_int) (const char *string);
    int (*utf8_char_size) (const char *string);
    int (*utf8_strlen) (const char *string);
    int (*utf8_strnlen) (const char *string, int bytes);
    int (*utf8_strlen_screen) (const char *string);
    int (*utf8_char_size_screen) (const char *string);
    const char *(*utf8_add_offset) (const char *string, int offset);
    int (*utf8_real_pos) (const char *string, int pos);
    int (*utf8_pos) (const char *string, int real_pos);
    char *(*utf8_strndup) (const char *string, int length);
    void (*utf8_strncpy) (char *dest, const char *string, int length);

    /* crypto */
    int (*crypto_hash) (const void *data, int data_size,
                        const char *hash_algo, void *hash, int *hash_size);
    int (*crypto_hash_file) (const char *fliename,
                             const char *hash_algo, void *hash, int *hash_size);
    int (*crypto_hash_pbkdf2) (const void *data, int data_size,
                               const char *hash_algo,
                               const void *salt, int salt_size,
                               int iterations,
                               void *hash, int *hash_size);
    int (*crypto_hmac) (const void *key, int key_size,
                        const void *message, int message_size,
                        const char *hash_algo, void *hash, int *hash_size);

    /* directories/files */
    int (*mkdir_home) (const char *directory, int mode);
    int (*mkdir) (const char *directory, int mode);
    int (*mkdir_parents) (const char *directory, int mode);
    void (*exec_on_files) (const char *directory, int recurse_subdirs,
                           int hidden_files,
                           void (*callback)(void *data, const char *filename),
                           void *callback_data);
    char *(*file_get_content) (const char *filename);
    int (*file_copy) (const char *from, const char *to);
    int (*file_compress) (const char *from, const char *to,
                          const char *compressor, int compression_level);

    /* util */
    int (*util_timeval_cmp) (struct timeval *tv1, struct timeval *tv2);
    long long (*util_timeval_diff) (struct timeval *tv1, struct timeval *tv2);
    void (*util_timeval_add) (struct timeval *tv, long long interval);
    const char *(*util_get_time_string) (const time_t *date);
    int (*util_version_number) (const char *version);

    /* sorted lists */
    struct t_weelist *(*list_new) ();
    struct t_weelist_item *(*list_add) (struct t_weelist *weelist,
                                        const char *data,
                                        const char *where,
                                        void *user_data);
    struct t_weelist_item *(*list_search) (struct t_weelist *weelist,
                                           const char *data);
    int (*list_search_pos) (struct t_weelist *weelist,
                            const char *data);
    struct t_weelist_item *(*list_casesearch) (struct t_weelist *weelist,
                                               const char *data);
    int (*list_casesearch_pos) (struct t_weelist *weelist,
                                const char *data);
    struct t_weelist_item *(*list_get) (struct t_weelist *weelist,
                                        int position);
    void (*list_set) (struct t_weelist_item *item, const char *value);
    struct t_weelist_item *(*list_next) (struct t_weelist_item *item);
    struct t_weelist_item *(*list_prev) (struct t_weelist_item *item);
    const char *(*list_string) (struct t_weelist_item *item);
    void *(*list_user_data) (struct t_weelist_item *item);
    int (*list_size) (struct t_weelist *weelist);
    void (*list_remove) (struct t_weelist *weelist,
                         struct t_weelist_item *item);
    void (*list_remove_all) (struct t_weelist *weelist);
    void (*list_free) (struct t_weelist *weelist);

    /* array lists */
    struct t_arraylist *(*arraylist_new) (int initial_size,
                                          int sorted,
                                          int allow_duplicates,
                                          int (*callback_cmp)(void *data,
                                                              struct t_arraylist *arraylist,
                                                              void *pointer1,
                                                              void *pointer2),
                                          void *callback_cmp_data,
                                          void (*callback_free)(void *data,
                                                                struct t_arraylist *arraylist,
                                                                void *pointer),
                                          void *callback_free_data);
    int (*arraylist_size) (struct t_arraylist *arraylist);
    void *(*arraylist_get) (struct t_arraylist *arraylist, int index);
    void *(*arraylist_search) (struct t_arraylist *arraylist, void *pointer,
                               int *index, int *index_insert);
    int (*arraylist_insert) (struct t_arraylist *arraylist, int index,
                             void *pointer);
    int (*arraylist_add) (struct t_arraylist *arraylist, void *pointer);
    int (*arraylist_remove) (struct t_arraylist *arraylist, int index);
    int (*arraylist_clear) (struct t_arraylist *arraylist);
    void (*arraylist_free) (struct t_arraylist *arraylist);

    /* hash tables */
    struct t_hashtable *(*hashtable_new) (int size,
                                          const char *type_keys,
                                          const char *type_values,
                                          unsigned long long (*callback_hash_key)(struct t_hashtable *hashtable,
                                                                                  const void *key),
                                          int (*callback_keycmp)(struct t_hashtable *hashtable,
                                                                 const void *key1,
                                                                 const void *key2));
    struct t_hashtable_item *(*hashtable_set_with_size) (struct t_hashtable *hashtable,
                                                         const void *key,
                                                         int key_size,
                                                         const void *value,
                                                         int value_size);
    struct t_hashtable_item *(*hashtable_set) (struct t_hashtable *hashtable,
                                               const void *key,
                                               const void *value);
    void *(*hashtable_get) (struct t_hashtable *hashtable, const void *key);
    int (*hashtable_has_key) (struct t_hashtable *hashtable, const void *key);
    void (*hashtable_map) (struct t_hashtable *hashtable,
                           void (*callback_map) (void *data,
                                                 struct t_hashtable *hashtable,
                                                 const void *key,
                                                 const void *value),
                           void *callback_map_data);
    void (*hashtable_map_string) (struct t_hashtable *hashtable,
                                  void (*callback_map) (void *data,
                                                        struct t_hashtable *hashtable,
                                                        const char *key,
                                                        const char *value),
                                  void *callback_map_data);
    struct t_hashtable *(*hashtable_dup) (struct t_hashtable *hashtable);
    int (*hashtable_get_integer) (struct t_hashtable *hashtable,
                                  const char *property);
    const char *(*hashtable_get_string) (struct t_hashtable *hashtable,
                                         const char *property);
    void (*hashtable_set_pointer) (struct t_hashtable *hashtable,
                                   const char *property,
                                   void *pointer);
    int (*hashtable_add_to_infolist) (struct t_hashtable *hashtable,
                                      struct t_infolist_item *infolist_item,
                                      const char *prefix);
    int (*hashtable_add_from_infolist) (struct t_hashtable *hashtable,
                                        struct t_infolist *infolist,
                                        const char *prefix);
    void (*hashtable_remove) (struct t_hashtable *hashtable, const void *key);
    void (*hashtable_remove_all) (struct t_hashtable *hashtable);
    void (*hashtable_free) (struct t_hashtable *hashtable);

    /* config files */
    struct t_config_file *(*config_new) (struct t_weechat_plugin *plugin,
                                         const char *name,
                                         int (*callback_reload)(const void *pointer,
                                                                void *data,
                                                                struct t_config_file *config_file),
                                         const void *callback_reload_pointer,
                                         void *callback_reload_data);
    int (*config_set_version) (struct t_config_file *config_file,
                               int version,
                               struct t_hashtable *(*callback_update)(const void *pointer,
                                                                      void *data,
                                                                      struct t_config_file *config_file,
                                                                      int version_read,
                                                                      struct t_hashtable *data_read),
                               const void *callback_update_pointer,
                               void *callback_update_data);
    struct t_config_section *(*config_new_section) (struct t_config_file *config_file,
                                                    const char *name,
                                                    int user_can_add_options,
                                                    int user_can_delete_options,
                                                    int (*callback_read)(const void *pointer,
                                                                         void *data,
                                                                         struct t_config_file *config_file,
                                                                         struct t_config_section *section,
                                                                         const char *option_name,
                                                                         const char *value),
                                                    const void *callback_read_pointer,
                                                    void *callback_read_data,
                                                    int (*callback_write)(const void *pointer,
                                                                          void *data,
                                                                          struct t_config_file *config_file,
                                                                          const char *section_name),
                                                    const void *callback_write_pointer,
                                                    void *callback_write_data,
                                                    int (*callback_write_default)(const void *pointer,
                                                                                  void *data,
                                                                                  struct t_config_file *config_file,
                                                                                  const char *section_name),
                                                    const void *callback_write_default_pointer,
                                                    void *callback_write_default_data,
                                                    int (*callback_create_option)(const void *pointer,
                                                                                  void *data,
                                                                                  struct t_config_file *config_file,
                                                                                  struct t_config_section *section,
                                                                                  const char *option_name,
                                                                                  const char *value),
                                                    const void *callback_create_option_pointer,
                                                    void *callback_create_option_data,
                                                    int (*callback_delete_option)(const void *pointer,
                                                                                  void *data,
                                                                                  struct t_config_file *config_file,
                                                                                  struct t_config_section *section,
                                                                                  struct t_config_option *option),
                                                    const void *callback_delete_option_pointer,
                                                    void *callback_delete_option_data);
    struct t_config_section *(*config_search_section) (struct t_config_file *config_file,
                                                       const char *section_name);
    struct t_config_option *(*config_new_option) (struct t_config_file *config_file,
                                                  struct t_config_section *section,
                                                  const char *name,
                                                  const char *type,
                                                  const char *description,
                                                  const char *string_values,
                                                  int min,
                                                  int max,
                                                  const char *default_value,
                                                  const char *value,
                                                  int null_value_allowed,
                                                  int (*callback_check_value)(const void *pointer,
                                                                              void *data,
                                                                              struct t_config_option *option,
                                                                              const char *value),
                                                  const void *callback_check_value_pointer,
                                                  void *callback_check_value_data,
                                                  void (*callback_change)(const void *pointer,
                                                                          void *data,
                                                                          struct t_config_option *option),
                                                  const void *callback_change_pointer,
                                                  void *callback_change_data,
                                                  void (*callback_delete)(const void *pointer,
                                                                          void *data,
                                                                          struct t_config_option *option),
                                                  const void *callback_delete_pointer,
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
    int (*config_option_set) (struct t_config_option *option,
                              const char *value, int run_callback);
    int (*config_option_set_null) (struct t_config_option *option,
                                   int run_callback);
    int (*config_option_unset) (struct t_config_option *option);
    void (*config_option_rename) (struct t_config_option *option,
                                  const char *new_name);
    const char *(*config_option_get_string) (struct t_config_option *option,
                                             const char *property);
    void *(*config_option_get_pointer) (struct t_config_option *option,
                                        const char *property);
    int (*config_option_is_null) (struct t_config_option *option);
    int (*config_option_default_is_null) (struct t_config_option *option);
    int (*config_boolean) (struct t_config_option *option);
    int (*config_boolean_default) (struct t_config_option *option);
    int (*config_integer) (struct t_config_option *option);
    int (*config_integer_default) (struct t_config_option *option);
    const char *(*config_string) (struct t_config_option *option);
    const char *(*config_string_default) (struct t_config_option *option);
    const char *(*config_color) (struct t_config_option *option);
    const char *(*config_color_default) (struct t_config_option *option);
    int (*config_write_option) (struct t_config_file *config_file,
                                struct t_config_option *option);
    int (*config_write_line) (struct t_config_file *config_file,
                              const char *option_name,
                              const char *value, ...);
    int (*config_write) (struct t_config_file *config_file);
    int (*config_read) (struct t_config_file *config_file);
    int (*config_reload) (struct t_config_file *config_file);
    void (*config_option_free) (struct t_config_option *option);
    void (*config_section_free_options) (struct t_config_section *section);
    void (*config_section_free) (struct t_config_section *section);
    void (*config_free) (struct t_config_file *config_file);
    struct t_config_option *(*config_get) (const char *option_name);
    const char *(*config_get_plugin) (struct t_weechat_plugin *plugin,
                                      const char *option_name);
    int (*config_is_set_plugin) (struct t_weechat_plugin *plugin,
                                 const char *option_name);
    int (*config_set_plugin) (struct t_weechat_plugin *plugin,
                              const char *option_name, const char *value);
    void (*config_set_desc_plugin) (struct t_weechat_plugin *plugin,
                                    const char *option_name,
                                    const char *description);
    int (*config_unset_plugin) (struct t_weechat_plugin *plugin,
                                const char *option_name);

    /* key bindings */
    int (*key_bind) (const char *context, struct t_hashtable *keys);
    int (*key_unbind) (const char *context, const char *key);

    /* display */
    const char *(*prefix) (const char *prefix);
    const char *(*color) (const char *color_name);
    void (*printf_date_tags) (struct t_gui_buffer *buffer, time_t date,
                              const char *tags, const char *message, ...);
    void (*printf_y_date_tags) (struct t_gui_buffer *buffer, int y,
                                time_t date, const char *tags,
                                const char *message, ...);
    void (*log_printf) (const char *message, ...);

    /* hooks */
    struct t_hook *(*hook_command) (struct t_weechat_plugin *plugin,
                                    const char *command,
                                    const char *description,
                                    const char *args,
                                    const char *args_description,
                                    const char *completion,
                                    int (*callback)(const void *pointer,
                                                    void *data,
                                                    struct t_gui_buffer *buffer,
                                                    int argc, char **argv,
                                                    char **argv_eol),
                                    const void *callback_pointer,
                                    void *callback_data);
    struct t_hook *(*hook_command_run) (struct t_weechat_plugin *plugin,
                                        const char *command,
                                        int (*callback)(const void *pointer,
                                                        void *data,
                                                        struct t_gui_buffer *buffer,
                                                        const char *command),
                                        const void *callback_pointer,
                                        void *callback_data);
    struct t_hook *(*hook_timer) (struct t_weechat_plugin *plugin,
                                  long interval,
                                  int align_second,
                                  int max_calls,
                                  int (*callback)(const void *pointer,
                                                  void *data,
                                                  int remaining_calls),
                                  const void *callback_pointer,
                                  void *callback_data);
    struct t_hook *(*hook_fd) (struct t_weechat_plugin *plugin,
                               int fd,
                               int flag_read,
                               int flag_write,
                               int flag_exception,
                               int (*callback)(const void *pointer,
                                               void *data,
                                               int fd),
                               const void *callback_pointer,
                               void *callback_data);
    struct t_hook *(*hook_process) (struct t_weechat_plugin *plugin,
                                    const char *command,
                                    int timeout,
                                    int (*callback)(const void *pointer,
                                                    void *data,
                                                    const char *command,
                                                    int return_code,
                                                    const char *out,
                                                    const char *err),
                                    const void *callback_pointer,
                                    void *callback_data);
    struct t_hook *(*hook_process_hashtable) (struct t_weechat_plugin *plugin,
                                              const char *command,
                                              struct t_hashtable *options,
                                              int timeout,
                                              int (*callback)(const void *pointer,
                                                              void *data,
                                                              const char *command,
                                                              int return_code,
                                                              const char *out,
                                                              const char *err),
                                              const void *callback_pointer,
                                              void *callback_data);
    struct t_hook *(*hook_connect) (struct t_weechat_plugin *plugin,
                                    const char *proxy,
                                    const char *address,
                                    int port,
                                    int ipv6,
                                    int retry,
                                    void *gnutls_sess, void *gnutls_cb,
                                    int gnutls_dhkey_size,
                                    const char *gnutls_priorities,
                                    const char *local_hostname,
                                    int (*callback)(const void *pointer,
                                                    void *data,
                                                    int status,
                                                    int gnutls_rc,
                                                    int sock,
                                                    const char *error,
                                                    const char *ip_address),
                                    const void *callback_pointer,
                                    void *callback_data);
    struct t_hook *(*hook_line) (struct t_weechat_plugin *plugin,
                                 const char *buffer_type,
                                 const char *buffer_name,
                                 const char *tags,
                                 struct t_hashtable *(*callback)(const void *pointer,
                                                                 void *data,
                                                                 struct t_hashtable *line),
                                 const void *callback_pointer,
                                 void *callback_data);
    struct t_hook *(*hook_print) (struct t_weechat_plugin *plugin,
                                  struct t_gui_buffer *buffer,
                                  const char *tags,
                                  const char *message,
                                  int strip_colors,
                                  int (*callback)(const void *pointer,
                                                  void *data,
                                                  struct t_gui_buffer *buffer,
                                                  time_t date,
                                                  int tags_count,
                                                  const char **tags,
                                                  int displayed,
                                                  int highlight,
                                                  const char *prefix,
                                                  const char *message),
                                  const void *callback_pointer,
                                  void *callback_data);
    struct t_hook *(*hook_signal) (struct t_weechat_plugin *plugin,
                                   const char *signal,
                                   int (*callback)(const void *pointer,
                                                   void *data,
                                                   const char *signal,
                                                   const char *type_data,
                                                   void *signal_data),
                                   const void *callback_pointer,
                                   void *callback_data);
    int (*hook_signal_send) (const char *signal, const char *type_data,
                             void *signal_data);
    struct t_hook *(*hook_hsignal) (struct t_weechat_plugin *plugin,
                                    const char *signal,
                                    int (*callback)(const void *pointer,
                                                    void *data,
                                                    const char *signal,
                                                    struct t_hashtable *hashtable),
                                    const void *callback_pointer,
                                    void *callback_data);
    int (*hook_hsignal_send) (const char *signal,
                              struct t_hashtable *hashtable);
    struct t_hook *(*hook_config) (struct t_weechat_plugin *plugin,
                                   const char *option,
                                   int (*callback)(const void *pointer,
                                                   void *data,
                                                   const char *option,
                                                   const char *value),
                                   const void *callback_pointer,
                                   void *callback_data);
    struct t_hook *(*hook_completion) (struct t_weechat_plugin *plugin,
                                       const char *completion_item,
                                       const char *description,
                                       int (*callback)(const void *pointer,
                                                       void *data,
                                                       const char *completion_item,
                                                       struct t_gui_buffer *buffer,
                                                       struct t_gui_completion *completion),
                                       const void *callback_pointer,
                                       void *callback_data);
    const char *(*hook_completion_get_string) (struct t_gui_completion *completion,
                                               const char *property);
    void (*hook_completion_list_add) (struct t_gui_completion *completion,
                                      const char *word,
                                      int nick_completion,
                                      const char *where);
    struct t_hook *(*hook_modifier) (struct t_weechat_plugin *plugin,
                                     const char *modifier,
                                     char *(*callback)(const void *pointer,
                                                       void *data,
                                                       const char *modifier,
                                                       const char *modifier_data,
                                                       const char *string),
                                     const void *callback_pointer,
                                     void *callback_data);
    char *(*hook_modifier_exec) (struct t_weechat_plugin *plugin,
                                 const char *modifier,
                                 const char *modifier_data,
                                 const char *string);
    struct t_hook *(*hook_info) (struct t_weechat_plugin *plugin,
                                 const char *info_name,
                                 const char *description,
                                 const char *args_description,
                                 char *(*callback)(const void *pointer,
                                                   void *data,
                                                   const char *info_name,
                                                   const char *arguments),
                                 const void *callback_pointer,
                                 void *callback_data);
    struct t_hook *(*hook_info_hashtable) (struct t_weechat_plugin *plugin,
                                           const char *info_name,
                                           const char *description,
                                           const char *args_description,
                                           const char *output_description,
                                           struct t_hashtable *(*callback)(const void *pointer,
                                                                           void *data,
                                                                           const char *info_name,
                                                                           struct t_hashtable *hashtable),
                                           const void *callback_pointer,
                                           void *callback_data);
    struct t_hook *(*hook_infolist) (struct t_weechat_plugin *plugin,
                                     const char *infolist_name,
                                     const char *description,
                                     const char *pointer_description,
                                     const char *args_description,
                                     struct t_infolist *(*callback)(const void *cb_pointer,
                                                                    void *data,
                                                                    const char *infolist_name,
                                                                    void *obj_pointer,
                                                                    const char *arguments),
                                     const void *callback_pointer,
                                     void *callback_data);
    struct t_hook *(*hook_hdata) (struct t_weechat_plugin *plugin,
                                  const char *hdata_name,
                                  const char *description,
                                  struct t_hdata *(*callback)(const void *pointer,
                                                              void *data,
                                                              const char *hdata_name),
                                  const void *callback_pointer,
                                  void *callback_data);
    struct t_hook *(*hook_focus) (struct t_weechat_plugin *plugin,
                                  const char *area,
                                  struct t_hashtable *(*callback)(const void *pointer,
                                                                  void *data,
                                                                  struct t_hashtable *info),
                                  const void *callback_pointer,
                                  void *callback_data);
    void (*hook_set) (struct t_hook *hook, const char *property,
                      const char *value);
    void (*unhook) (struct t_hook *hook);
    void (*unhook_all) (struct t_weechat_plugin *plugin,
                        const char *subplugin);

    /* buffers */
    struct t_gui_buffer *(*buffer_new) (struct t_weechat_plugin *plugin,
                                        const char *name,
                                        int (*input_callback)(const void *pointer,
                                                              void *data,
                                                              struct t_gui_buffer *buffer,
                                                              const char *input_data),
                                        const void *input_callback_pointer,
                                        void *input_callback_data,
                                        int (*close_callback)(const void *pointer,
                                                              void *data,
                                                              struct t_gui_buffer *buffer),
                                        const void *close_callback_pointer,
                                        void *close_callback_data);
    struct t_gui_buffer *(*buffer_new_props) (struct t_weechat_plugin *plugin,
                                              const char *name,
                                              struct t_hashtable *properties,
                                              int (*input_callback)(const void *pointer,
                                                                    void *data,
                                                                    struct t_gui_buffer *buffer,
                                                                    const char *input_data),
                                              const void *input_callback_pointer,
                                              void *input_callback_data,
                                              int (*close_callback)(const void *pointer,
                                                                    void *data,
                                                                    struct t_gui_buffer *buffer),
                                              const void *close_callback_pointer,
                                              void *close_callback_data);
    struct t_gui_buffer *(*buffer_search) (const char *plugin, const char *name);
    struct t_gui_buffer *(*buffer_search_main) ();
    void (*buffer_clear) (struct t_gui_buffer *buffer);
    void (*buffer_close) (struct t_gui_buffer *buffer);
    void (*buffer_merge) (struct t_gui_buffer *buffer,
                           struct t_gui_buffer *target_buffer);
    void (*buffer_unmerge) (struct t_gui_buffer *buffer, int number);
    int (*buffer_get_integer) (struct t_gui_buffer *buffer,
                               const char *property);
    const char *(*buffer_get_string) (struct t_gui_buffer *buffer,
                                      const char *property);
    void *(*buffer_get_pointer) (struct t_gui_buffer *buffer,
                                 const char *property);
    void (*buffer_set) (struct t_gui_buffer *buffer, const char *property,
                        const char *value);
    void (*buffer_set_pointer) (struct t_gui_buffer *buffer,
                                const char *property, void *pointer);
    char *(*buffer_string_replace_local_var) (struct t_gui_buffer *buffer,
                                              const char *string);
    int (*buffer_match_list) (struct t_gui_buffer *buffer, const char *string);

    /* windows */
    struct t_gui_window *(*window_search_with_buffer) (struct t_gui_buffer *buffer);
    int (*window_get_integer) (struct t_gui_window *window,
                               const char *property);
    const char *(*window_get_string) (struct t_gui_window *window,
                                      const char *property);
    void *(*window_get_pointer) (struct t_gui_window *window,
                                 const char *property);
    void (*window_set_title) (const char *title);

    /* nicklist */
    struct t_gui_nick_group *(*nicklist_add_group) (struct t_gui_buffer *buffer,
                                                    struct t_gui_nick_group *parent_group,
                                                    const char *name,
                                                    const char *color,
                                                    int visible);
    struct t_gui_nick_group *(*nicklist_search_group) (struct t_gui_buffer *buffer,
                                                       struct t_gui_nick_group *from_group,
                                                       const char *name);
    struct t_gui_nick *(*nicklist_add_nick) (struct t_gui_buffer *buffer,
                                             struct t_gui_nick_group *group,
                                             const char *name,
                                             const char *color,
                                             const char *prefix,
                                             const char *prefix_color,
                                             int visible);
    struct t_gui_nick *(*nicklist_search_nick) (struct t_gui_buffer *buffer,
                                                struct t_gui_nick_group *from_group,
                                                const char *name);
    void (*nicklist_remove_group) (struct t_gui_buffer *buffer,
                                   struct t_gui_nick_group *group);
    void (*nicklist_remove_nick) (struct t_gui_buffer *buffer,
                                  struct t_gui_nick *nick);
    void (*nicklist_remove_all) (struct t_gui_buffer *buffer);
    void (*nicklist_get_next_item) (struct t_gui_buffer *buffer,
                                    struct t_gui_nick_group **group,
                                    struct t_gui_nick **nick);
    int (*nicklist_group_get_integer) (struct t_gui_buffer *buffer,
                                       struct t_gui_nick_group *group,
                                       const char *property);
    const char *(*nicklist_group_get_string) (struct t_gui_buffer *buffer,
                                              struct t_gui_nick_group *group,
                                              const char *property);
    void *(*nicklist_group_get_pointer) (struct t_gui_buffer *buffer,
                                         struct t_gui_nick_group *group,
                                         const char *property);
    void (*nicklist_group_set) (struct t_gui_buffer *buffer,
                                struct t_gui_nick_group *group,
                                const char *property, const char *value);
    int (*nicklist_nick_get_integer) (struct t_gui_buffer *buffer,
                                      struct t_gui_nick *nick,
                                      const char *property);
    const char *(*nicklist_nick_get_string) (struct t_gui_buffer *buffer,
                                             struct t_gui_nick *nick,
                                             const char *property);
    void *(*nicklist_nick_get_pointer) (struct t_gui_buffer *buffer,
                                        struct t_gui_nick *nick,
                                        const char *property);
    void (*nicklist_nick_set) (struct t_gui_buffer *buffer,
                               struct t_gui_nick *nick,
                               const char *property, const char *value);

    /* bars */
    struct t_gui_bar_item *(*bar_item_search) (const char *name);
    struct t_gui_bar_item *(*bar_item_new) (struct t_weechat_plugin *plugin,
                                            const char *name,
                                            char *(*build_callback)(const void *pointer,
                                                                    void *data,
                                                                    struct t_gui_bar_item *item,
                                                                    struct t_gui_window *window,
                                                                    struct t_gui_buffer *buffer,
                                                                    struct t_hashtable *extra_info),
                                            const void *build_callback_pointer,
                                            void *build_callback_data);
    void (*bar_item_update) (const char *name);
    void (*bar_item_remove) (struct t_gui_bar_item *item);
    struct t_gui_bar *(*bar_search) (const char *name);
    struct t_gui_bar *(*bar_new) (const char *name,
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
                                  const char *color_bg_inactive,
                                  const char *separator,
                                  const char *items);
    int (*bar_set) (struct t_gui_bar *bar, const char *property,
                    const char *value);
    void (*bar_update) (const char *name);
    void (*bar_remove) (struct t_gui_bar *bar);

    /* command */
    int (*command) (struct t_weechat_plugin *plugin,
                    struct t_gui_buffer *buffer, const char *command);
    int (*command_options) (struct t_weechat_plugin *plugin,
                            struct t_gui_buffer *buffer, const char *command,
                            struct t_hashtable *options);

    /* completion */
    struct t_gui_completion *(*completion_new) (struct t_weechat_plugin *plugin,
                                                struct t_gui_buffer *buffer);
    int (*completion_search) (struct t_gui_completion *completion,
                              const char *data, int position, int direction);
    const char *(*completion_get_string) (struct t_gui_completion *completion,
                                          const char *property);
    void (*completion_list_add) (struct t_gui_completion *completion,
                                 const char *word,
                                 int nick_completion,
                                 const char *where);
    void (*completion_free) (struct t_gui_completion *completion);

    /* network */
    int (*network_pass_proxy) (const char *proxy, int sock,
                               const char *address, int port);
    int (*network_connect_to) (const char *proxy,
                               struct sockaddr *address,
                               socklen_t address_length);

    /* infos */
    char *(*info_get) (struct t_weechat_plugin *plugin, const char *info_name,
                       const char *arguments);
    struct t_hashtable *(*info_get_hashtable) (struct t_weechat_plugin *plugin,
                                               const char *info_name,
                                               struct t_hashtable *hashtable);

    /* infolists */
    struct t_infolist *(*infolist_new) (struct t_weechat_plugin *plugin);
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
    struct t_infolist_var *(*infolist_search_var) (struct t_infolist *infolist,
                                                   const char *name);
    struct t_infolist *(*infolist_get) (struct t_weechat_plugin *plugin,
                                        const char *infolist_name,
                                        void *pointer,
                                        const char *arguments);
    int (*infolist_next) (struct t_infolist *infolist);
    int (*infolist_prev) (struct t_infolist *infolist);
    void (*infolist_reset_item_cursor) (struct t_infolist *infolist);
    const char *(*infolist_fields) (struct t_infolist *infolist);
    int (*infolist_integer) (struct t_infolist *infolist, const char *var);
    const char *(*infolist_string) (struct t_infolist *infolist, const char *var);
    void *(*infolist_pointer) (struct t_infolist *infolist, const char *var);
    void *(*infolist_buffer) (struct t_infolist *infolist, const char *var,
                              int *size);
    time_t (*infolist_time) (struct t_infolist *infolist, const char *var);
    void (*infolist_free) (struct t_infolist *infolist);

    /* hdata */
    struct t_hdata *(*hdata_new) (struct t_weechat_plugin *plugin,
                                  const char *hdata_name, const char *var_prev,
                                  const char *var_next,
                                  int create_allowed, int delete_allowed,
                                  int (*callback_update)(void *data,
                                                         struct t_hdata *hdata,
                                                         void *pointer,
                                                         struct t_hashtable *hashtable),
                                  void *callback_update_data);
    void (*hdata_new_var) (struct t_hdata *hdata, const char *name, int offset,
                           int type, int update_allowed, const char *array_size,
                           const char *hdata_name);
    void (*hdata_new_list) (struct t_hdata *hdata, const char *name,
                            void *pointer, int flags);
    struct t_hdata *(*hdata_get) (struct t_weechat_plugin *plugin,
                                  const char *hdata_name);
    int (*hdata_get_var_offset) (struct t_hdata *hdata, const char *name);
    int (*hdata_get_var_type) (struct t_hdata *hdata, const char *name);
    const char *(*hdata_get_var_type_string) (struct t_hdata *hdata,
                                              const char *name);
    int (*hdata_get_var_array_size) (struct t_hdata *hdata, void *pointer,
                                     const char *name);
    const char *(*hdata_get_var_array_size_string) (struct t_hdata *hdata,
                                                    void *pointer,
                                                    const char *name);
    const char *(*hdata_get_var_hdata) (struct t_hdata *hdata,
                                        const char *name);
    void *(*hdata_get_var) (struct t_hdata *hdata, void *pointer,
                            const char *name);
    void *(*hdata_get_var_at_offset) (struct t_hdata *hdata, void *pointer,
                                      int offset);
    void *(*hdata_get_list) (struct t_hdata *hdata, const char *name);
    int (*hdata_check_pointer) (struct t_hdata *hdata, void *list,
                                void *pointer);
    void *(*hdata_move) (struct t_hdata *hdata, void *pointer, int count);
    void *(*hdata_search) (struct t_hdata *hdata, void *pointer,
                           const char *search, struct t_hashtable *pointers,
                           struct t_hashtable *extra_vars,
                           struct t_hashtable *options,
                           int move);
    char (*hdata_char) (struct t_hdata *hdata, void *pointer,
                        const char *name);
    int (*hdata_integer) (struct t_hdata *hdata, void *pointer,
                          const char *name);
    long (*hdata_long) (struct t_hdata *hdata, void *pointer,
                        const char *name);
    const char *(*hdata_string) (struct t_hdata *hdata, void *pointer,
                                 const char *name);
    void *(*hdata_pointer) (struct t_hdata *hdata, void *pointer,
                            const char *name);
    time_t (*hdata_time) (struct t_hdata *hdata, void *pointer,
                          const char *name);
    struct t_hashtable *(*hdata_hashtable) (struct t_hdata *hdata,
                                            void *pointer, const char *name);
    int (*hdata_compare) (struct t_hdata *hdata,
                          void *pointer1, void *pointer2, const char *name,
                          int case_sensitive);
    int (*hdata_set) (struct t_hdata *hdata, void *pointer, const char *name,
                      const char *value);
    int (*hdata_update) (struct t_hdata *hdata, void *pointer,
                         struct t_hashtable *hashtable);
    const char *(*hdata_get_string) (struct t_hdata *hdata,
                                     const char *property);

    /* upgrade */
    struct t_upgrade_file *(*upgrade_new) (const char *filename,
                                           int (*callback_read)(const void *pointer,
                                                                void *data,
                                                                struct t_upgrade_file *upgrade_file,
                                                                int object_id,
                                                                struct t_infolist *infolist),
                                           const void *callback_read_pointer,
                                           void *callback_read_data);
    int (*upgrade_write_object) (struct t_upgrade_file *upgrade_file,
                                 int object_id,
                                 struct t_infolist *infolist);
    int (*upgrade_read) (struct t_upgrade_file *upgrade_file);
    void (*upgrade_close) (struct t_upgrade_file *upgrade_file);
};

extern int weechat_plugin_init (struct t_weechat_plugin *plugin,
                                int argc, char *argv[]);
extern int weechat_plugin_end (struct t_weechat_plugin *plugin);

/* macros for easy call to plugin API */

/* plugins */
#define weechat_plugin_get_name(__plugin)                               \
    (weechat_plugin->plugin_get_name)(__plugin)

/* strings */
#define weechat_charset_set(__charset)                                  \
    (weechat_plugin->charset_set)(weechat_plugin, __charset)
#define weechat_iconv_to_internal(__charset, __string)                  \
    (weechat_plugin->iconv_to_internal)(__charset, __string)
#define weechat_iconv_from_internal(__charset, __string)                \
    (weechat_plugin->iconv_from_internal)(__charset, __string)
#ifndef WEECHAT_H
#ifndef _
#define _(string) (weechat_plugin->gettext)(string)
#endif /* _ */
#ifndef N_
#define N_(string) (string)
#endif /* N_ */
#ifndef NG_
#define NG_(single,plural,number)                                       \
    (weechat_plugin->ngettext)(single, plural, number)
#endif /* NG_ */
#endif /* WEECHAT_H */
#define weechat_gettext(string) (weechat_plugin->gettext)(string)
#define weechat_ngettext(single,plural,number)                          \
    (weechat_plugin->ngettext)(single, plural, number)
#define weechat_strndup(__string, __bytes)                              \
    (weechat_plugin->strndup)(__string, __bytes)
#define weechat_string_cut(__string, __length, __count_suffix,          \
                           __screen, __cut_suffix)                      \
    (weechat_plugin->string_cut)(__string, __length, __count_suffix,    \
                                 __screen, __cut_suffix)
#define weechat_string_tolower(__string)                                \
    (weechat_plugin->string_tolower)(__string)
#define weechat_string_toupper(__string)                                \
    (weechat_plugin->string_toupper)(__string)
#define weechat_string_charcmp(__string1, __string2)                    \
    (weechat_plugin->string_charcmp)(__string1, __string2)
#define weechat_string_charcasecmp(__string1, __string2)                \
    (weechat_plugin->string_charcasecmp)(__string1, __string2)
#define weechat_strcmp(__string1, __string2)                            \
    (weechat_plugin->strcmp)(__string1, __string2)
#define weechat_strncmp(__string1, __string2, __max)                    \
    (weechat_plugin->strncmp)(__string1, __string2, __max)
#define weechat_strcasecmp(__string1, __string2)                        \
    (weechat_plugin->strcasecmp)(__string1, __string2)
#define weechat_strcasecmp_range(__string1, __string2, __range)         \
    (weechat_plugin->strcasecmp_range)(__string1, __string2, __range)
#define weechat_strncasecmp(__string1, __string2, __max)                \
    (weechat_plugin->strncasecmp)(__string1, __string2, __max)
#define weechat_strncasecmp_range(__string1, __string2, __max, __range) \
    (weechat_plugin->strncasecmp_range)(__string1, __string2, __max,    \
                                        __range)
#define weechat_strcmp_ignore_chars(__string1, __string2,               \
                                    __chars_ignored, __case_sensitive)  \
    (weechat_plugin->strcmp_ignore_chars)(__string1, __string2,         \
                                          __chars_ignored,              \
                                          __case_sensitive)
#define weechat_strcasestr(__string, __search)                          \
    (weechat_plugin->strcasestr)(__string, __search)
#define weechat_strlen_screen(__string)                                 \
    (weechat_plugin->strlen_screen)(__string)
#define weechat_string_match(__string, __mask, __case_sensitive)        \
    (weechat_plugin->string_match)(__string, __mask, __case_sensitive)
#define weechat_string_match_list(__string, __masks, __case_sensitive)  \
    (weechat_plugin->string_match_list)(__string, __masks,              \
                                        __case_sensitive)
#define weechat_string_replace(__string, __search, __replace)           \
    (weechat_plugin->string_replace)(__string, __search, __replace)
#define weechat_string_expand_home(__path)                              \
    (weechat_plugin->string_expand_home)(__path)
#define weechat_string_eval_path_home(__path, __pointers,               \
                                      __extra_vars, __options)          \
    (weechat_plugin->string_eval_path_home)(__path, __pointers,         \
                                            __extra_vars, __options)
#define weechat_string_remove_quotes(__string, __quotes)                \
    (weechat_plugin->string_remove_quotes)(__string, __quotes)
#define weechat_string_strip(__string, __left, __right, __chars)        \
    (weechat_plugin->string_strip)(__string, __left, __right, __chars)
#define weechat_string_convert_escaped_chars(__string)                  \
    (weechat_plugin->string_convert_escaped_chars)(__string)
#define weechat_string_mask_to_regex(__mask)                            \
    (weechat_plugin->string_mask_to_regex)(__mask)
#define weechat_string_regex_flags(__regex, __default_flags, __flags)   \
    (weechat_plugin->string_regex_flags)(__regex, __default_flags,      \
                                         __flags)
#define weechat_string_regcomp(__preg, __regex, __default_flags)        \
    (weechat_plugin->string_regcomp)(__preg, __regex, __default_flags)
#define weechat_string_has_highlight(__string, __highlight_words)       \
    (weechat_plugin->string_has_highlight)(__string, __highlight_words)
#define weechat_string_has_highlight_regex(__string, __regex)           \
    (weechat_plugin->string_has_highlight_regex)(__string, __regex)
#define weechat_string_replace_regex(__string, __regex, __replace,      \
                                     __reference_char, __callback,      \
                                     __callback_data)                   \
    (weechat_plugin->string_replace_regex)(__string, __regex,           \
                                           __replace,                   \
                                           __reference_char,            \
                                           __callback,                  \
                                           __callback_data)
#define weechat_string_translate_chars(__string, __chars1, __chars2)    \
    (weechat_plugin->string_translate_chars)(__string, __chars1,        \
                                             __chars2);
#define weechat_string_split(__string, __separators, __strip_items,     \
                             __flags, __max, __num_items)               \
    (weechat_plugin->string_split)(__string, __separators,              \
                                   __strip_items, __flags,              \
                                   __max, __num_items)
#define weechat_string_split_shell(__string, __num_items)               \
    (weechat_plugin->string_split_shell)(__string, __num_items)
#define weechat_string_free_split(__split_string)                       \
    (weechat_plugin->string_free_split)(__split_string)
#define weechat_string_rebuild_split_string(__split_string,             \
                                            __separator,                \
                                            __index_start,              \
                                            __index_end)                \
    (weechat_plugin->string_rebuild_split_string)(__split_string,       \
                                                  __separator,          \
                                                  __index_start,        \
                                                  __index_end)
#define weechat_string_split_command(__command, __separator)            \
    (weechat_plugin->string_split_command)(__command, __separator)
#define weechat_string_free_split_command(__split_command)              \
    (weechat_plugin->string_free_split_command)(__split_command)
#define weechat_string_format_size(__size)                              \
    (weechat_plugin->string_format_size)(__size)
#define weechat_string_parse_size(__size)                               \
    (weechat_plugin->string_parse_size)(__size)
#define weechat_string_color_code_size(__string)                        \
    (weechat_plugin->string_color_code_size)(__string)
#define weechat_string_remove_color(__string, __replacement)            \
    (weechat_plugin->string_remove_color)(__string, __replacement)
#define weechat_string_base_encode(__base, __from, __length, __to)      \
    (weechat_plugin->string_base_encode)(__base, __from, __length,      \
                                         __to)
#define weechat_string_base_decode(__base, __from, __to)                \
    (weechat_plugin->string_base_decode)(__base, __from, __to)
#define weechat_string_hex_dump(__data, __data_size, __bytes_per_line,  \
                                __prefix, __suffix)                     \
    (weechat_plugin->string_hex_dump)(__data, __data_size,              \
                                      __bytes_per_line, __prefix,       \
                                      __suffix)
#define weechat_string_is_command_char(__string)                        \
    (weechat_plugin->string_is_command_char)(__string)
#define weechat_string_input_for_buffer(__string)                       \
    (weechat_plugin->string_input_for_buffer)(__string)
#define weechat_string_eval_expression(__expr, __pointers,              \
                                       __extra_vars, __options)         \
    (weechat_plugin->string_eval_expression)(__expr, __pointers,        \
                                             __extra_vars, __options)
#define weechat_string_dyn_alloc(__size_alloc)                          \
    (weechat_plugin->string_dyn_alloc)(__size_alloc)
#define weechat_string_dyn_copy(__string, __new_string)                 \
    (weechat_plugin->string_dyn_copy)(__string, __new_string)
#define weechat_string_dyn_concat(__string, __add, __bytes)             \
    (weechat_plugin->string_dyn_concat)(__string, __add, __bytes)
#define weechat_string_dyn_free(__string, __free_string)                \
    (weechat_plugin->string_dyn_free)(__string, __free_string)

/* UTF-8 strings */
#define weechat_utf8_has_8bits(__string)                                \
    (weechat_plugin->utf8_has_8bits)(__string)
#define weechat_utf8_is_valid(__string, __length, __error)              \
    (weechat_plugin->utf8_is_valid)(__string, __length, __error)
#define weechat_utf8_normalize(__string, __char)                        \
    (weechat_plugin->utf8_normalize)(__string, __char)
#define weechat_utf8_prev_char(__start, __string)                       \
    (weechat_plugin->utf8_prev_char)(__start, __string)
#define weechat_utf8_next_char(__string)                                \
    (weechat_plugin->utf8_next_char)(__string)
#define weechat_utf8_char_int(__string)                                 \
    (weechat_plugin->utf8_char_int)(__string)
#define weechat_utf8_char_size(__string)                                \
    (weechat_plugin->utf8_char_size)(__string)
#define weechat_utf8_strlen(__string)                                   \
    (weechat_plugin->utf8_strlen)(__string)
#define weechat_utf8_strnlen(__string, __bytes)                         \
    (weechat_plugin->utf8_strnlen)(__string, __bytes)
#define weechat_utf8_strlen_screen(__string)                            \
    (weechat_plugin->utf8_strlen_screen)(__string)
#define weechat_utf8_char_size_screen(__string)                         \
    (weechat_plugin->utf8_char_size_screen)(__string)
#define weechat_utf8_add_offset(__string, __offset)                     \
    (weechat_plugin->utf8_add_offset)(__string, __offset)
#define weechat_utf8_real_pos(__string, __pos)                          \
    (weechat_plugin->utf8_real_pos)(__string, __pos)
#define weechat_utf8_pos(__string, __real_pos)                          \
    (weechat_plugin->utf8_pos)(__string, __real_pos)
#define weechat_utf8_strndup(__string, __length)                        \
    (weechat_plugin->utf8_strndup)(__string, __length)
#define weechat_utf8_strncpy(__dest, __string, __length)                \
    (weechat_plugin->utf8_strncpy)(__dest, __string, __length)

/* crypto */
#define weechat_crypto_hash(__data, __data_size, __hash_algo,           \
                            __hash, __hash_size)                        \
    (weechat_plugin->crypto_hash)(__data, __data_size, __hash_algo,     \
                                  __hash, __hash_size)
#define weechat_crypto_hash_file(__filename, __hash_algo, __hash,       \
                                 __hash_size)                           \
    (weechat_plugin->crypto_hash_file)(__filename, __hash_algo, __hash, \
                                       __hash_size)
#define weechat_crypto_hash_pbkdf2(__data, __data_size, __hash_algo,    \
                                   __salt, __salt_size, __iterations,   \
                                   __hash, __hash_size)                 \
    (weechat_plugin->crypto_hash_pbkdf2)(__data, __data_size,           \
                                         __hash_algo,                   \
                                         __salt, __salt_size,           \
                                         __iterations,                  \
                                         __hash, __hash_size)
#define weechat_crypto_hmac(__key, __key_size,                          \
                            __message, __message_size,                  \
                            __hash_algo,                                \
                            __hash, __hash_size)                        \
    (weechat_plugin->crypto_hmac)(__key, __key_size,                    \
                                  __message, __message_size,            \
                                  __hash_algo,                          \
                                  __hash, __hash_size)

/* directories */
#define weechat_mkdir_home(__directory, __mode)                         \
    (weechat_plugin->mkdir_home)(__directory, __mode)
#define weechat_mkdir(__directory, __mode)                              \
    (weechat_plugin->mkdir)(__directory, __mode)
#define weechat_mkdir_parents(__directory, __mode)                      \
    (weechat_plugin->mkdir_parents)(__directory, __mode)
#define weechat_exec_on_files(__directory, __recurse_subdirs,           \
                              __hidden_files, __callback,               \
                              __callback_data)                          \
    (weechat_plugin->exec_on_files)(__directory, __recurse_subdirs,     \
                                    __hidden_files,                     \
                                    __callback, __callback_data)
#define weechat_file_get_content(__filename)                            \
    (weechat_plugin->file_get_content)(__filename)
#define weechat_file_copy(__from, __to)                                 \
    (weechat_plugin->file_copy)(__from, __to)
#define weechat_file_compress(__from, __to, __compressor,               \
                              __compression_level)                      \
    (weechat_plugin->file_compress)(__from, __to, __compressor,         \
                                    __compression_level)

/* util */
#define weechat_util_timeval_cmp(__time1, __time2)                      \
    (weechat_plugin->util_timeval_cmp)(__time1, __time2)
#define weechat_util_timeval_diff(__time1, __time2)                     \
    (weechat_plugin->util_timeval_diff)(__time1, __time2)
#define weechat_util_timeval_add(__time, __interval)                    \
    (weechat_plugin->util_timeval_add)(__time, __interval)
#define weechat_util_get_time_string(__date)                            \
    (weechat_plugin->util_get_time_string)(__date)
#define weechat_util_version_number(__version)                          \
    (weechat_plugin->util_version_number)(__version)

/* sorted list */
#define weechat_list_new()                                              \
    (weechat_plugin->list_new)()
#define weechat_list_add(__list, __string, __where, __user_data)        \
    (weechat_plugin->list_add)(__list, __string, __where, __user_data)
#define weechat_list_search(__list, __string)                           \
    (weechat_plugin->list_search)(__list, __string)
#define weechat_list_search_pos(__list, __string)                       \
    (weechat_plugin->list_search_pos)(__list, __string)
#define weechat_list_casesearch(__list, __string)                       \
    (weechat_plugin->list_casesearch)(__list, __string)
#define weechat_list_casesearch_pos(__list, __string)                   \
    (weechat_plugin->list_casesearch_pos)(__list, __string)
#define weechat_list_get(__list, __index)                               \
    (weechat_plugin->list_get)(__list, __index)
#define weechat_list_set(__item, __value)                               \
    (weechat_plugin->list_set)(__item, __value)
#define weechat_list_next(__item)                                       \
    (weechat_plugin->list_next)(__item)
#define weechat_list_prev(__item)                                       \
    (weechat_plugin->list_prev)(__item)
#define weechat_list_string(__item)                                     \
    (weechat_plugin->list_string)(__item)
#define weechat_list_user_data(__item)                                  \
    (weechat_plugin->list_user_data)(__item)
#define weechat_list_size(__list)                                       \
    (weechat_plugin->list_size)(__list)
#define weechat_list_remove(__list, __item)                             \
    (weechat_plugin->list_remove)(__list, __item)
#define weechat_list_remove_all(__list)                                 \
    (weechat_plugin->list_remove_all)(__list)
#define weechat_list_free(__list)                                       \
    (weechat_plugin->list_free)(__list)

/* array lists */
#define weechat_arraylist_new(__initial_size, __sorted,                 \
                              __allow_duplicates, __callback_cmp,       \
                              __callback_cmp_data, __callback_free,     \
                              __callback_free_data)                     \
    (weechat_plugin->arraylist_new)(__initial_size, __sorted,           \
                              __allow_duplicates, __callback_cmp,       \
                              __callback_cmp_data, __callback_free,     \
                              __callback_free_data)
#define weechat_arraylist_size(__arraylist)                             \
    (weechat_plugin->arraylist_size)(__arraylist)
#define weechat_arraylist_get(__arraylist, __index)                     \
    (weechat_plugin->arraylist_get)(__arraylist, __index)
#define weechat_arraylist_search(__arraylist, __pointer, __index,       \
                                 __index_insert)                        \
    (weechat_plugin->arraylist_search)(__arraylist, __pointer, __index, \
                                       __index_insert)
#define weechat_arraylist_insert(__arraylist, __index, __pointer)       \
    (weechat_plugin->arraylist_insert)(__arraylist, __index, __pointer)
#define weechat_arraylist_add(__arraylist, __pointer)                   \
    (weechat_plugin->arraylist_add)(__arraylist, __pointer)
#define weechat_arraylist_remove(__arraylist, __index)                  \
    (weechat_plugin->arraylist_remove)(__arraylist, __index)
#define weechat_arraylist_clear(__arraylist)                            \
    (weechat_plugin->arraylist_clear)(__arraylist)
#define weechat_arraylist_free(__arraylist)                             \
    (weechat_plugin->arraylist_free)(__arraylist)

/* hash tables */
#define weechat_hashtable_new(__size, __type_keys, __type_values,       \
                              __callback_hash_key, __callback_keycmp)   \
    (weechat_plugin->hashtable_new)(__size, __type_keys, __type_values, \
                                    __callback_hash_key,                \
                                    __callback_keycmp)
#define weechat_hashtable_set_with_size(__hashtable, __key, __key_size, \
                                        __value, __value_size)          \
    (weechat_plugin->hashtable_set_with_size)(__hashtable, __key,       \
                                              __key_size, __value,      \
                                              __value_size)
#define weechat_hashtable_set(__hashtable, __key, __value)              \
    (weechat_plugin->hashtable_set)(__hashtable, __key, __value)
#define weechat_hashtable_get(__hashtable, __key)                       \
    (weechat_plugin->hashtable_get)(__hashtable, __key)
#define weechat_hashtable_has_key(__hashtable, __key)                   \
    (weechat_plugin->hashtable_has_key)(__hashtable, __key)
#define weechat_hashtable_map(__hashtable, __cb_map, __cb_map_data)     \
    (weechat_plugin->hashtable_map)(__hashtable, __cb_map,              \
                                    __cb_map_data)
#define weechat_hashtable_map_string(__hashtable, __cb_map,             \
                                     __cb_map_data)                     \
    (weechat_plugin->hashtable_map_string)(__hashtable, __cb_map,       \
                                           __cb_map_data)
#define weechat_hashtable_dup(__hashtable)                              \
    (weechat_plugin->hashtable_dup)(__hashtable)
#define weechat_hashtable_get_integer(__hashtable, __property)          \
    (weechat_plugin->hashtable_get_integer)(__hashtable, __property)
#define weechat_hashtable_get_string(__hashtable, __property)           \
    (weechat_plugin->hashtable_get_string)(__hashtable, __property)
#define weechat_hashtable_set_pointer(__hashtable, __property,          \
                                      __pointer)                        \
    (weechat_plugin->hashtable_set_pointer)(__hashtable, __property,    \
                                            __pointer)
#define weechat_hashtable_add_to_infolist(__hashtable, __infolist_item, \
                                          __prefix)                     \
    (weechat_plugin->hashtable_add_to_infolist)(__hashtable,            \
                                                __infolist_item,        \
                                                __prefix)
#define weechat_hashtable_add_from_infolist(__hashtable, __infolist,    \
                                            __prefix)                   \
    (weechat_plugin->hashtable_add_from_infolist)(__hashtable,          \
                                                  __infolist,           \
                                                  __prefix)
#define weechat_hashtable_remove(__hashtable, __key)                    \
    (weechat_plugin->hashtable_remove)(__hashtable, __key)
#define weechat_hashtable_remove_all(__hashtable)                       \
    (weechat_plugin->hashtable_remove_all)(__hashtable)
#define weechat_hashtable_free(__hashtable)                             \
    (weechat_plugin->hashtable_free)(__hashtable)

/* config files */
#define weechat_config_new(__name, __callback_reload,                   \
                           __callback_reload_pointer,                   \
                           __callback_reload_data)                      \
    (weechat_plugin->config_new)(weechat_plugin, __name,                \
                                 __callback_reload,                     \
                                 __callback_reload_pointer,             \
                                 __callback_reload_data)
#define weechat_config_set_version(__config, __version,                 \
                                   __callback_update,                   \
                                   __callback_update_pointer,           \
                                   __callback_update_data)              \
    (weechat_plugin->config_set_version)(__config, __version,           \
                                         __callback_update,             \
                                         __callback_update_pointer,     \
                                         __callback_update_data)
#define weechat_config_new_section(__config, __name,                    \
                                   __user_can_add_options,              \
                                   __user_can_delete_options,           \
                                   __cb_read,                           \
                                   __cb_read_pointer,                   \
                                   __cb_read_data,                      \
                                   __cb_write_std,                      \
                                   __cb_write_std_pointer,              \
                                   __cb_write_std_data,                 \
                                   __cb_write_def,                      \
                                   __cb_write_def_pointer,              \
                                   __cb_write_def_data,                 \
                                   __cb_create_option,                  \
                                   __cb_create_option_pointer,          \
                                   __cb_create_option_data,             \
                                   __cb_delete_option,                  \
                                   __cb_delete_option_pointer,          \
                                   __cb_delete_option_data)             \
    (weechat_plugin->config_new_section)(__config, __name,              \
                                         __user_can_add_options,        \
                                         __user_can_delete_options,     \
                                         __cb_read,                     \
                                         __cb_read_pointer,             \
                                         __cb_read_data,                \
                                         __cb_write_std,                \
                                         __cb_write_std_pointer,        \
                                         __cb_write_std_data,           \
                                         __cb_write_def,                \
                                         __cb_write_def_pointer,        \
                                         __cb_write_def_data,           \
                                         __cb_create_option,            \
                                         __cb_create_option_pointer,    \
                                         __cb_create_option_data,       \
                                         __cb_delete_option,            \
                                         __cb_delete_option_pointer,    \
                                         __cb_delete_option_data)
#define weechat_config_search_section(__config, __name)                 \
    (weechat_plugin->config_search_section)(__config, __name)
#define weechat_config_new_option(__config, __section, __name, __type,  \
                                  __desc, __string_values, __min,       \
                                  __max, __default, __value,            \
                                  __null_value_allowed,                 \
                                  __callback_check,                     \
                                  __callback_check_pointer,             \
                                  __callback_check_data,                \
                                  __callback_change,                    \
                                  __callback_change_pointer,            \
                                  __callback_change_data,               \
                                  __callback_delete,                    \
                                  __callback_delete_pointer,            \
                                  __callback_delete_data)               \
    (weechat_plugin->config_new_option)(__config, __section, __name,    \
                                        __type, __desc,                 \
                                        __string_values,                \
                                        __min, __max, __default,        \
                                        __value,                        \
                                        __null_value_allowed,           \
                                        __callback_check,               \
                                        __callback_check_pointer,       \
                                        __callback_check_data,          \
                                        __callback_change,              \
                                        __callback_change_pointer,      \
                                        __callback_change_data,         \
                                        __callback_delete,              \
                                        __callback_delete_pointer,      \
                                        __callback_delete_data)
#define weechat_config_search_option(__config, __section, __name)       \
    (weechat_plugin->config_search_option)(__config, __section, __name)
#define weechat_config_search_section_option(__config, __section,       \
                                             __name, __section_found,   \
                                             __option_found)            \
    (weechat_plugin->config_search_section_option)(__config, __section, \
                                                   __name,              \
                                                   __section_found,     \
                                                   __option_found);
#define weechat_config_search_with_string(__name, __config, __section,  \
                                          __option, __pos_option)       \
    (weechat_plugin->config_search_with_string)(__name, __config,       \
                                                __section, __option,    \
                                                __pos_option);
#define weechat_config_string_to_boolean(__string)                      \
    (weechat_plugin->config_string_to_boolean)(__string)
#define weechat_config_option_reset(__option, __run_callback)           \
    (weechat_plugin->config_option_reset)(__option, __run_callback)
#define weechat_config_option_set(__option, __value, __run_callback)    \
    (weechat_plugin->config_option_set)(__option, __value,              \
                                        __run_callback)
#define weechat_config_option_set_null(__option, __run_callback)        \
    (weechat_plugin->config_option_set_null)(__option, __run_callback)
#define weechat_config_option_unset(__option)                           \
    (weechat_plugin->config_option_unset)(__option)
#define weechat_config_option_rename(__option, __new_name)              \
    (weechat_plugin->config_option_rename)(__option, __new_name)
#define weechat_config_option_get_string(__option, __property)         \
    (weechat_plugin->config_option_get_string)(__option, __property)
#define weechat_config_option_get_pointer(__option, __property)         \
    (weechat_plugin->config_option_get_pointer)(__option, __property)
#define weechat_config_option_is_null(__option)                         \
    (weechat_plugin->config_option_is_null)(__option)
#define weechat_config_option_default_is_null(__option)                 \
    (weechat_plugin->config_option_default_is_null)(__option)
#define weechat_config_boolean(__option)                                \
    (weechat_plugin->config_boolean)(__option)
#define weechat_config_boolean_default(__option)                        \
    (weechat_plugin->config_boolean_default)(__option)
#define weechat_config_integer(__option)                                \
    (weechat_plugin->config_integer)(__option)
#define weechat_config_integer_default(__option)                        \
    (weechat_plugin->config_integer_default)(__option)
#define weechat_config_string(__option)                                 \
    (weechat_plugin->config_string)(__option)
#define weechat_config_string_default(__option)                         \
    (weechat_plugin->config_string_default)(__option)
#define weechat_config_color(__option)                                  \
    (weechat_plugin->config_color)(__option)
#define weechat_config_color_default(__option)                          \
    (weechat_plugin->config_color_default)(__option)
#define weechat_config_write_option(__config, __option)                 \
    (weechat_plugin->config_write_option)(__config, __option)
#define weechat_config_write_line(__config, __option, __value...)       \
    (weechat_plugin->config_write_line)(__config, __option, ##__value)
#define weechat_config_write(__config)                                  \
    (weechat_plugin->config_write)(__config)
#define weechat_config_read(__config)                                   \
    (weechat_plugin->config_read)(__config)
#define weechat_config_reload(__config)                                 \
    (weechat_plugin->config_reload)(__config)
#define weechat_config_option_free(__option)                            \
    (weechat_plugin->config_option_free)(__option)
#define weechat_config_section_free_options(__section)                  \
    (weechat_plugin->config_section_free_options)(__section)
#define weechat_config_section_free(__section)                          \
    (weechat_plugin->config_section_free)(__section)
#define weechat_config_free(__config)                                   \
    (weechat_plugin->config_free)(__config)
#define weechat_config_get(__option)                                    \
    (weechat_plugin->config_get)(__option)
#define weechat_config_get_plugin(__option)                             \
    (weechat_plugin->config_get_plugin)(weechat_plugin, __option)
#define weechat_config_is_set_plugin(__option)                          \
    (weechat_plugin->config_is_set_plugin)(weechat_plugin, __option)
#define weechat_config_set_plugin(__option, __value)                    \
    (weechat_plugin->config_set_plugin)(weechat_plugin, __option,       \
                                        __value)
#define weechat_config_set_desc_plugin(__option, __description)         \
    (weechat_plugin->config_set_desc_plugin)(weechat_plugin, __option,  \
                                             __description)
#define weechat_config_unset_plugin(__option)                           \
    (weechat_plugin->config_unset_plugin)(weechat_plugin, __option)

/* key bindings */
#define weechat_key_bind(__context, __keys)                             \
    (weechat_plugin->key_bind)(__context, __keys)
#define weechat_key_unbind(__context, __key)                            \
    (weechat_plugin->key_unbind)(__context, __key)

/* display */
#define weechat_prefix(__prefix)                                        \
    (weechat_plugin->prefix)(__prefix)
#define weechat_color(__color_name)                                     \
    (weechat_plugin->color)(__color_name)
#define weechat_printf(__buffer, __message, __argz...)                  \
    (weechat_plugin->printf_date_tags)(__buffer, 0, NULL, __message,    \
                                       ##__argz)
#define weechat_printf_date_tags(__buffer, __date, __tags, __message,   \
                                 __argz...)                             \
    (weechat_plugin->printf_date_tags)(__buffer, __date, __tags,        \
                                       __message, ##__argz)
#define weechat_printf_y(__buffer, __y, __message, __argz...)           \
    (weechat_plugin->printf_y_date_tags)(__buffer, __y, 0, NULL,        \
                                         __message, ##__argz)
#define weechat_printf_y_date_tags(__buffer, __y, __date, __tags,       \
                                   __message, __argz...)                \
    (weechat_plugin->printf_y_date_tags)(__buffer, __y, __date, __tags, \
                                         __message, ##__argz)
#define weechat_log_printf(__message, __argz...)                        \
    (weechat_plugin->log_printf)(__message, ##__argz)

/* hooks */
#define weechat_hook_command(__command, __description, __args,          \
                             __args_desc, __completion, __callback,     \
                             __pointer, __data)                         \
    (weechat_plugin->hook_command)(weechat_plugin, __command,           \
                                   __description, __args, __args_desc,  \
                                   __completion, __callback, __pointer, \
                                   __data)
#define weechat_hook_command_run(__command, __callback, __pointer,      \
                                 __data)                                \
    (weechat_plugin->hook_command_run)(weechat_plugin, __command,       \
                                       __callback, __pointer, __data)
#define weechat_hook_timer(__interval, __align_second, __max_calls,     \
                           __callback, __pointer, __data)               \
    (weechat_plugin->hook_timer)(weechat_plugin, __interval,            \
                                 __align_second, __max_calls,           \
                                 __callback, __pointer, __data)
#define weechat_hook_fd(__fd, __flag_read, __flag_write,                \
                        __flag_exception, __callback, __pointer,        \
                        __data)                                         \
    (weechat_plugin->hook_fd)(weechat_plugin, __fd, __flag_read,        \
                              __flag_write, __flag_exception,           \
                              __callback, __pointer, __data)
#define weechat_hook_process(__command, __timeout, __callback,          \
                             __callback_pointer, __callback_data)       \
    (weechat_plugin->hook_process)(weechat_plugin, __command,           \
                                   __timeout, __callback,               \
                                   __callback_pointer, __callback_data)
#define weechat_hook_process_hashtable(__command, __options, __timeout, \
                                       __callback, __callback_pointer,  \
                                       __callback_data)                 \
    (weechat_plugin->hook_process_hashtable)(weechat_plugin, __command, \
                                             __options, __timeout,      \
                                             __callback,                \
                                             __callback_pointer,        \
                                             __callback_data)
#define weechat_hook_connect(__proxy, __address, __port, __ipv6,        \
                             __retry, __gnutls_sess, __gnutls_cb,       \
                             __gnutls_dhkey_size, __gnutls_priorities,  \
                             __local_hostname, __callback, __pointer,   \
                             __data)                                    \
    (weechat_plugin->hook_connect)(weechat_plugin, __proxy, __address,  \
                                   __port, __ipv6, __retry,             \
                                   __gnutls_sess, __gnutls_cb,          \
                                   __gnutls_dhkey_size,                 \
                                   __gnutls_priorities,                 \
                                   __local_hostname,                    \
                                   __callback, __pointer, __data)
#define weechat_hook_line(_buffer_type, __buffer_name, __tags,          \
                          __callback, __pointer, __data)                \
    (weechat_plugin->hook_line)(weechat_plugin, _buffer_type,           \
                                __buffer_name, __tags, __callback,      \
                                __pointer, __data)
#define weechat_hook_print(__buffer, __tags, __msg, __strip__colors,    \
                           __callback, __pointer, __data)               \
    (weechat_plugin->hook_print)(weechat_plugin, __buffer, __tags,      \
                                 __msg, __strip__colors, __callback,    \
                                 __pointer, __data)
#define weechat_hook_signal(__signal, __callback, __pointer, __data)    \
    (weechat_plugin->hook_signal)(weechat_plugin, __signal, __callback, \
                                  __pointer, __data)
#define weechat_hook_signal_send(__signal, __type_data, __signal_data)  \
    (weechat_plugin->hook_signal_send)(__signal, __type_data,           \
                                       __signal_data)
#define weechat_hook_hsignal(__signal, __callback, __pointer, __data)   \
    (weechat_plugin->hook_hsignal)(weechat_plugin, __signal,            \
                                   __callback, __pointer, __data)
#define weechat_hook_hsignal_send(__signal, __hashtable)                \
    (weechat_plugin->hook_hsignal_send)(__signal, __hashtable)
#define weechat_hook_config(__option, __callback, __pointer, __data)    \
    (weechat_plugin->hook_config)(weechat_plugin, __option, __callback, \
                                  __pointer, __data)
#define weechat_hook_completion(__completion, __description,            \
                                __callback, __pointer, __data)          \
    (weechat_plugin->hook_completion)(weechat_plugin, __completion,     \
                                      __description, __callback,        \
                                      __pointer, __data)
#define weechat_hook_completion_get_string(__completion, __property)    \
    (weechat_plugin->hook_completion_get_string)(__completion,          \
                                                 __property)
#define weechat_hook_completion_list_add(__completion, __word,          \
                                         __nick_completion, __where)    \
    (weechat_plugin->hook_completion_list_add)(__completion, __word,    \
                                               __nick_completion,       \
                                               __where)
#define weechat_hook_modifier(__modifier, __callback, __pointer,        \
                              __data)                                   \
    (weechat_plugin->hook_modifier)(weechat_plugin, __modifier,         \
                                    __callback, __pointer, __data)
#define weechat_hook_modifier_exec(__modifier, __modifier_data,         \
                                   __string)                            \
    (weechat_plugin->hook_modifier_exec)(weechat_plugin, __modifier,    \
                                         __modifier_data, __string)
#define weechat_hook_info(__info_name, __description,                   \
                          __args_description, __callback, __pointer,    \
                          __data)                                       \
    (weechat_plugin->hook_info)(weechat_plugin, __info_name,            \
                                __description, __args_description,      \
                                __callback, __pointer, __data)
#define weechat_hook_info_hashtable(__info_name, __description,         \
                                    __args_description,                 \
                                    __output_description,               \
                                    __callback,                         \
                                    __pointer,                          \
                                    __data)                             \
    (weechat_plugin->hook_info_hashtable)(weechat_plugin, __info_name,  \
                                          __description,                \
                                          __args_description,           \
                                          __output_description,         \
                                          __callback, __pointer,        \
                                          __data)
#define weechat_hook_infolist(__infolist_name, __description,           \
                              __pointer_description,                    \
                              __args_description, __callback,           \
                              __pointer, __data)                        \
    (weechat_plugin->hook_infolist)(weechat_plugin, __infolist_name,    \
                                    __description,                      \
                                    __pointer_description,              \
                                    __args_description, __callback,     \
                                    __pointer, __data)
#define weechat_hook_hdata(__hdata_name, __description, __callback,     \
                           __pointer, __data)                           \
    (weechat_plugin->hook_hdata)(weechat_plugin, __hdata_name,          \
                                 __description, __callback, __pointer,  \
                                 __data)
#define weechat_hook_focus(__area, __callback, __pointer, __data)       \
    (weechat_plugin->hook_focus)(weechat_plugin, __area, __callback,    \
                                 __pointer, __data)
#define weechat_hook_set(__hook, __property, __value)                   \
    (weechat_plugin->hook_set)(__hook, __property, __value)
#define weechat_unhook(__hook)                                          \
    (weechat_plugin->unhook)( __hook)
#define weechat_unhook_all(__subplugin)                                 \
    (weechat_plugin->unhook_all)(weechat_plugin, __subplugin)

/* buffers */
#define weechat_buffer_new(__name,                                      \
                           __input_callback,                            \
                           __input_callback_pointer,                    \
                           __input_callback_data,                       \
                           __close_callback,                            \
                           __close_callback_pointer,                    \
                           __close_callback_data)                       \
    (weechat_plugin->buffer_new)(weechat_plugin,                        \
                                 __name,                                \
                                 __input_callback,                      \
                                 __input_callback_pointer,              \
                                 __input_callback_data,                 \
                                 __close_callback,                      \
                                 __close_callback_pointer,              \
                                 __close_callback_data)
#define weechat_buffer_new_props(__name,                                \
                                 __properties,                          \
                                 __input_callback,                      \
                                 __input_callback_pointer,              \
                                 __input_callback_data,                 \
                                 __close_callback,                      \
                                 __close_callback_pointer,              \
                                 __close_callback_data)                 \
    (weechat_plugin->buffer_new_props)(weechat_plugin,                  \
                                       __name,                          \
                                       __properties,                    \
                                       __input_callback,                \
                                       __input_callback_pointer,        \
                                       __input_callback_data,           \
                                       __close_callback,                \
                                       __close_callback_pointer,        \
                                       __close_callback_data)
#define weechat_buffer_search(__plugin, __name)                         \
    (weechat_plugin->buffer_search)(__plugin, __name)
#define weechat_buffer_search_main()                                    \
    (weechat_plugin->buffer_search_main)()
#define weechat_current_buffer()                                        \
    (weechat_plugin->buffer_search)(NULL, NULL)
#define weechat_buffer_clear(__buffer)                                  \
    (weechat_plugin->buffer_clear)(__buffer)
#define weechat_buffer_close(__buffer)                                  \
    (weechat_plugin->buffer_close)(__buffer)
#define weechat_buffer_merge(__buffer, __target_buffer)                 \
    (weechat_plugin->buffer_merge)(__buffer, __target_buffer)
#define weechat_buffer_unmerge(__buffer, __number)                      \
    (weechat_plugin->buffer_unmerge)(__buffer, __number)
#define weechat_buffer_get_integer(__buffer, __property)                \
    (weechat_plugin->buffer_get_integer)(__buffer, __property)
#define weechat_buffer_get_string(__buffer, __property)                 \
    (weechat_plugin->buffer_get_string)(__buffer, __property)
#define weechat_buffer_get_pointer(__buffer, __property)                \
    (weechat_plugin->buffer_get_pointer)(__buffer, __property)
#define weechat_buffer_set(__buffer, __property, __value)               \
    (weechat_plugin->buffer_set)(__buffer, __property, __value)
#define weechat_buffer_set_pointer(__buffer, __property, __pointer)     \
    (weechat_plugin->buffer_set_pointer)(__buffer, __property,          \
                                         __pointer)
#define weechat_buffer_string_replace_local_var(__buffer, __string)     \
    (weechat_plugin->buffer_string_replace_local_var)(__buffer,         \
                                                      __string)
#define weechat_buffer_match_list(__buffer, __string)                   \
    (weechat_plugin->buffer_match_list)(__buffer, __string)

/* windows */
#define weechat_window_search_with_buffer(__buffer)                     \
    (weechat_plugin->window_search_with_buffer)(__buffer)
#define weechat_window_get_integer(__window, __property)                \
    (weechat_plugin->window_get_integer)(__window, __property)
#define weechat_window_get_string(__window, __property)                 \
    (weechat_plugin->window_get_string)(__window, __property)
#define weechat_window_get_pointer(__window, __property)                \
    (weechat_plugin->window_get_pointer)(__window, __property)
#define weechat_current_window()                                        \
    (weechat_plugin->window_get_pointer)(NULL, "current")
#define weechat_window_set_title(__title)                               \
    (weechat_plugin->window_set_title)(__title)

/* nicklist */
#define weechat_nicklist_add_group(__buffer, __parent_group, __name,    \
                                   __color, __visible)                  \
    (weechat_plugin->nicklist_add_group)(__buffer, __parent_group,      \
                                         __name, __color, __visible)
#define weechat_nicklist_search_group(__buffer, __from_group, __name)   \
    (weechat_plugin->nicklist_search_group)(__buffer, __from_group,     \
                                            __name)
#define weechat_nicklist_add_nick(__buffer, __group, __name, __color,   \
                                  __prefix, __prefix_color, __visible)  \
    (weechat_plugin->nicklist_add_nick)(__buffer, __group, __name,      \
                                        __color, __prefix,              \
                                        __prefix_color, __visible)
#define weechat_nicklist_search_nick(__buffer, __from_group, __name)    \
    (weechat_plugin->nicklist_search_nick)(__buffer, __from_group,      \
                                           __name)
#define weechat_nicklist_remove_group(__buffer, __group)                \
    (weechat_plugin->nicklist_remove_group)(__buffer, __group)
#define weechat_nicklist_remove_nick(__buffer, __nick)                  \
    (weechat_plugin->nicklist_remove_nick)(__buffer, __nick)
#define weechat_nicklist_remove_all(__buffer)                           \
    (weechat_plugin->nicklist_remove_all)(__buffer)
#define weechat_nicklist_get_next_item(__buffer, __group, __nick)       \
    (weechat_plugin->nicklist_get_next_item)(__buffer, __group, __nick)
#define weechat_nicklist_group_get_integer(__buffer, __group,           \
                                           __property)                  \
    (weechat_plugin->nicklist_group_get_integer)(__buffer, __group,     \
                                                 __property)
#define weechat_nicklist_group_get_string(__buffer, __group,            \
                                          __property)                   \
    (weechat_plugin->nicklist_group_get_string)(__buffer, __group,      \
                                                __property)
#define weechat_nicklist_group_get_pointer(__buffer, __group,           \
                                           __property)                  \
    (weechat_plugin->nicklist_group_get_pointer)(__buffer, __group,     \
                                                 __property)
#define weechat_nicklist_group_set(__buffer, __group, __property,       \
                                   __value)                             \
    (weechat_plugin->nicklist_group_set)(__buffer, __group, __property, \
                                         __value)
#define weechat_nicklist_nick_get_integer(__buffer, __nick, __property) \
    (weechat_plugin->nicklist_nick_get_integer)(__buffer, __nick,       \
                                                __property)
#define weechat_nicklist_nick_get_string(__buffer, __nick, __property)  \
    (weechat_plugin->nicklist_nick_get_string)(__buffer, __nick,        \
                                               __property)
#define weechat_nicklist_nick_get_pointer(__buffer, __nick, __property) \
    (weechat_plugin->nicklist_nick_get_pointer)(__buffer, __nick,       \
                                                __property)
#define weechat_nicklist_nick_set(__buffer, __nick, __property,         \
                                  __value)                              \
    (weechat_plugin->nicklist_nick_set)(__buffer, __nick, __property,   \
                                        __value)

/* bars */
#define weechat_bar_item_search(__name)                                 \
    (weechat_plugin->bar_item_search)(__name)
#define weechat_bar_item_new(__name, __build_callback,                  \
                             __build_callback_pointer,                  \
                             __build_callback_data)                     \
    (weechat_plugin->bar_item_new)(weechat_plugin, __name,              \
                                   __build_callback,                    \
                                   __build_callback_pointer,            \
                                   __build_callback_data)
#define weechat_bar_item_update(__name)                                 \
    (weechat_plugin->bar_item_update)(__name)
#define weechat_bar_item_remove(__item)                                 \
    (weechat_plugin->bar_item_remove)(__item)
#define weechat_bar_search(__name)                                      \
    (weechat_plugin->bar_search)(__name)
#define weechat_bar_new(__name, __hidden, __priority, __type,           \
                        __condition, __position, __filling_top_bottom,  \
                        __filling_left_right, __size, __size_max,       \
                        __color_fg, __color_delim, __color_bg,          \
                        __color_bg_inactive, __separator, __items)      \
    (weechat_plugin->bar_new)(__name, __hidden, __priority, __type,     \
                              __condition, __position,                  \
                              __filling_top_bottom,                     \
                              __filling_left_right,                     \
                              __size, __size_max, __color_fg,           \
                              __color_delim, __color_bg,                \
                              __color_bg_inactive, __separator,         \
                              __items)
#define weechat_bar_set(__bar, __property, __value)                     \
    (weechat_plugin->bar_set)(__bar, __property, __value)
#define weechat_bar_update(__name)                                      \
    (weechat_plugin->bar_update)(__name)
#define weechat_bar_remove(__bar)                                       \
    (weechat_plugin->bar_remove)(__bar)

/* command */
#define weechat_command(__buffer, __command)                            \
    (weechat_plugin->command)(weechat_plugin, __buffer, __command)
#define weechat_command_options(__buffer, __command, __options)         \
    (weechat_plugin->command_options)(weechat_plugin, __buffer,         \
                                      __command, __options)

/* completion */
#define weechat_completion_new(__buffer)                                \
    (weechat_plugin->completion_new)(weechat_plugin, __buffer)
#define weechat_completion_search(__completion, __data, __position,     \
                                  __direction)                          \
    (weechat_plugin->completion_search)(__completion, __data,           \
                                        __position, __direction)
#define weechat_completion_get_string(__completion, __property)         \
    (weechat_plugin->completion_get_string)(__completion, __property)
#define weechat_completion_list_add(__completion, __word,               \
                                    __nick_completion, __where)         \
    (weechat_plugin->completion_list_add)(__completion, __word,         \
                                          __nick_completion, __where)
#define weechat_completion_free(__completion)                           \
    (weechat_plugin->completion_free)(__completion)

/* network */
#define weechat_network_pass_proxy(__proxy, __sock, __address, __port)  \
    (weechat_plugin->network_pass_proxy)(__proxy, __sock, __address,    \
                                         __port)
#define weechat_network_connect_to(__proxy, __address,                  \
                                   __address_length)                    \
    (weechat_plugin->network_connect_to)(__proxy, __address,            \
                                         __address_length)

/* infos */
#define weechat_info_get(__info_name, __arguments)                      \
    (weechat_plugin->info_get)(weechat_plugin, __info_name,             \
                               __arguments)
#define weechat_info_get_hashtable(__info_name, __hashtable)            \
    (weechat_plugin->info_get_hashtable)(weechat_plugin, __info_name,   \
                                         __hashtable)

/* infolists */
#define weechat_infolist_new()                                          \
    (weechat_plugin->infolist_new)(weechat_plugin)
#define weechat_infolist_new_item(__list)                               \
    (weechat_plugin->infolist_new_item)(__list)
#define weechat_infolist_new_var_integer(__item, __name, __value)       \
    (weechat_plugin->infolist_new_var_integer)(__item, __name, __value)
#define weechat_infolist_new_var_string(__item, __name, __value)        \
    (weechat_plugin->infolist_new_var_string)(__item, __name, __value)
#define weechat_infolist_new_var_pointer(__item, __name, __pointer)     \
    (weechat_plugin->infolist_new_var_pointer)(__item, __name,          \
                                               __pointer)
#define weechat_infolist_new_var_buffer(__item, __name, __buffer,       \
                                        __size)                         \
    (weechat_plugin->infolist_new_var_buffer)(__item, __name, __buffer, \
                                              __size)
#define weechat_infolist_new_var_time(__item, __name, __time)           \
    (weechat_plugin->infolist_new_var_time)(__item, __name, __time)
#define weechat_infolist_search_var(__list, __name)                     \
    (weechat_plugin->infolist_search_var)(__list, __name)
#define weechat_infolist_get(__infolist_name, __pointer, __arguments)   \
    (weechat_plugin->infolist_get)(weechat_plugin, __infolist_name,     \
                                   __pointer, __arguments)
#define weechat_infolist_next(__list)                                   \
    (weechat_plugin->infolist_next)(__list)
#define weechat_infolist_prev(__list)                                   \
    (weechat_plugin->infolist_prev)(__list)
#define weechat_infolist_reset_item_cursor(__list)                      \
    (weechat_plugin->infolist_reset_item_cursor)(__list)
#define weechat_infolist_fields(__list)                                 \
    (weechat_plugin->infolist_fields)(__list)
#define weechat_infolist_integer(__item, __var)                         \
    (weechat_plugin->infolist_integer)(__item, __var)
#define weechat_infolist_string(__item, __var)                          \
    (weechat_plugin->infolist_string)(__item, __var)
#define weechat_infolist_pointer(__item, __var)                         \
    (weechat_plugin->infolist_pointer)(__item, __var)
#define weechat_infolist_buffer(__item, __var, __size)                  \
    (weechat_plugin->infolist_buffer)(__item, __var, __size)
#define weechat_infolist_time(__item, __var)                            \
    (weechat_plugin->infolist_time)(__item, __var)
#define weechat_infolist_free(__list)                                   \
    (weechat_plugin->infolist_free)(__list)

/* hdata */
#define weechat_hdata_new(__hdata_name, __var_prev, __var_next,         \
                          __create_allowed, __delete_allowed,           \
                          __callback_update, __callback_update_data)    \
    (weechat_plugin->hdata_new)(weechat_plugin, __hdata_name,           \
                                __var_prev, __var_next,                 \
                                __create_allowed, __delete_allowed,     \
                                __callback_update,                      \
                                __callback_update_data)
#define weechat_hdata_new_var(__hdata, __name, __offset, __type,        \
                              __update_allowed, __array_size,           \
                              __hdata_name)                             \
    (weechat_plugin->hdata_new_var)(__hdata, __name, __offset, __type,  \
                                    __update_allowed, __array_size,     \
                                    __hdata_name)
#define WEECHAT_HDATA_VAR(__struct, __name, __type, __update_allowed,   \
                          __array_size, __hdata_name)                   \
    weechat_hdata_new_var (hdata, #__name, offsetof (__struct, __name), \
                           WEECHAT_HDATA_##__type, __update_allowed,    \
                           __array_size, __hdata_name)
#define weechat_hdata_new_list(__hdata, __name, __pointer, __flags)     \
    (weechat_plugin->hdata_new_list)(__hdata, __name, __pointer,        \
                                     __flags)
#define WEECHAT_HDATA_LIST(__name, __flags)                             \
    weechat_hdata_new_list (hdata, #__name, &(__name), __flags);
#define weechat_hdata_get(__hdata_name)                                 \
    (weechat_plugin->hdata_get)(weechat_plugin, __hdata_name)
#define weechat_hdata_get_var_offset(__hdata, __name)                   \
    (weechat_plugin->hdata_get_var_offset)(__hdata, __name)
#define weechat_hdata_get_var_type(__hdata, __name)                     \
    (weechat_plugin->hdata_get_var_type)(__hdata, __name)
#define weechat_hdata_get_var_type_string(__hdata, __name)              \
    (weechat_plugin->hdata_get_var_type_string)(__hdata, __name)
#define weechat_hdata_get_var_array_size(__hdata, __pointer, __name)    \
    (weechat_plugin->hdata_get_var_array_size)(__hdata, __pointer,      \
                                               __name)
#define weechat_hdata_get_var_array_size_string(__hdata, __pointer,     \
                                                __name)                 \
    (weechat_plugin->hdata_get_var_array_size_string)(__hdata,          \
                                                      __pointer,        \
                                                      __name)
#define weechat_hdata_get_var_hdata(__hdata, __name)                    \
    (weechat_plugin->hdata_get_var_hdata)(__hdata, __name)
#define weechat_hdata_get_var(__hdata, __pointer, __name)               \
    (weechat_plugin->hdata_get_var)(__hdata, __pointer, __name)
#define weechat_hdata_get_var_at_offset(__hdata, __pointer, __offset)   \
    (weechat_plugin->hdata_get_var_at_offset)(__hdata, __pointer,       \
                                              __offset)
#define weechat_hdata_get_list(__hdata, __name)                         \
    (weechat_plugin->hdata_get_list)(__hdata, __name)
#define weechat_hdata_check_pointer(__hdata, __list, __pointer)         \
    (weechat_plugin->hdata_check_pointer)(__hdata, __list, __pointer)
#define weechat_hdata_move(__hdata, __pointer, __count)                 \
    (weechat_plugin->hdata_move)(__hdata, __pointer, __count)
#define weechat_hdata_search(__hdata, __pointer, __search, __pointers,  \
                             __extra_vars, __options, __move)           \
    (weechat_plugin->hdata_search)(__hdata, __pointer, __search,        \
                                   __pointers, __extra_vars, __options, \
                                   __move)
#define weechat_hdata_char(__hdata, __pointer, __name)                  \
    (weechat_plugin->hdata_char)(__hdata, __pointer, __name)
#define weechat_hdata_integer(__hdata, __pointer, __name)               \
    (weechat_plugin->hdata_integer)(__hdata, __pointer, __name)
#define weechat_hdata_long(__hdata, __pointer, __name)                  \
    (weechat_plugin->hdata_long)(__hdata, __pointer, __name)
#define weechat_hdata_string(__hdata, __pointer, __name)                \
    (weechat_plugin->hdata_string)(__hdata, __pointer, __name)
#define weechat_hdata_pointer(__hdata, __pointer, __name)               \
    (weechat_plugin->hdata_pointer)(__hdata, __pointer, __name)
#define weechat_hdata_time(__hdata, __pointer, __name)                  \
    (weechat_plugin->hdata_time)(__hdata, __pointer, __name)
#define weechat_hdata_hashtable(__hdata, __pointer, __name)             \
    (weechat_plugin->hdata_hashtable)(__hdata, __pointer, __name)
#define weechat_hdata_compare(__hdata, __pointer1, __pointer2, __name,  \
                              __case_sensitive)                         \
    (weechat_plugin->hdata_compare)(__hdata, __pointer1, __pointer2,    \
                                    __name, __case_sensitive)
#define weechat_hdata_set(__hdata, __pointer, __name, __value)          \
    (weechat_plugin->hdata_set)(__hdata, __pointer, __name, __value)
#define weechat_hdata_update(__hdata, __pointer, __hashtable)           \
    (weechat_plugin->hdata_update)(__hdata, __pointer, __hashtable)
#define weechat_hdata_get_string(__hdata, __property)                   \
    (weechat_plugin->hdata_get_string)(__hdata, __property)

/* upgrade */
#define weechat_upgrade_new(__filename, __callback_read,                \
                            __callback_read_pointer,                    \
                            __callback_read_data)                       \
    (weechat_plugin->upgrade_new)(__filename, __callback_read,          \
                                  __callback_read_pointer,              \
                                  __callback_read_data)
#define weechat_upgrade_write_object(__upgrade_file, __object_id,       \
                                     __infolist)                        \
    (weechat_plugin->upgrade_write_object)(__upgrade_file, __object_id, \
                                           __infolist)
#define weechat_upgrade_read(__upgrade_file)                            \
    (weechat_plugin->upgrade_read)(__upgrade_file)
#define weechat_upgrade_close(__upgrade_file)                           \
    (weechat_plugin->upgrade_close)(__upgrade_file)

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* WEECHAT_WEECHAT_PLUGIN_H */
