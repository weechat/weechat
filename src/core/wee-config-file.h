/*
 * Copyright (C) 2003-2023 Sébastien Helleu <flashcode@flashtux.org>
 * Copyright (C) 2005-2006 Emmanuel Bouthenot <kolter@openics.org>
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

#ifndef WEECHAT_CONFIG_FILE_H
#define WEECHAT_CONFIG_FILE_H

#include <stdio.h>

#define CONFIG_VERSION_OPTION  "config_version"

#define CONFIG_PRIORITY_DEFAULT 1000

#define CONFIG_BOOLEAN(option) (*((int *)((option)->value)))
#define CONFIG_BOOLEAN_DEFAULT(option) (*((int *)((option)->default_value)))

#define CONFIG_INTEGER(option) (*((int *)((option)->value)))
#define CONFIG_INTEGER_DEFAULT(option) (*((int *)((option)->default_value)))

#define CONFIG_STRING(option) ((char *)((option)->value))
#define CONFIG_STRING_DEFAULT(option) ((char *)((option)->default_value))

#define CONFIG_COLOR(option) (*((int *)((option)->value)))
#define CONFIG_COLOR_DEFAULT(option) (*((int *)((option)->default_value)))

#define CONFIG_BOOLEAN_FALSE  0
#define CONFIG_BOOLEAN_TRUE   1

struct t_weelist;
struct t_infolist;

struct t_config_option;

struct t_config_file
{
    struct t_weechat_plugin *plugin;       /* plugin which created this cfg */
    int priority;                          /* config files are sorted by    */
                                           /* priority then name            */
    char *name;                            /* name (example: "weechat")     */
    char *filename;                        /* filename (without path)       */
                                           /* (example: "weechat.conf")     */
    FILE *file;                            /* file pointer                  */
    int version;                           /* config version (default=1)    */
    int version_read;                      /* config version read in file   */
    struct t_hashtable *(*callback_update) /* callback for version update   */
    (const void *pointer,
     void *data,
     struct t_config_file *config_file,
     int version_read,
     struct t_hashtable *data_read);
    const void *callback_update_pointer;   /* pointer sent to callback      */
    void *callback_update_data;            /* data sent to callback         */
    int (*callback_reload)                 /* callback for config reload    */
    (const void *pointer,
     void *data,
     struct t_config_file *config_file);
    const void *callback_reload_pointer;   /* pointer sent to callback      */
    void *callback_reload_data;            /* data sent to callback         */
    struct t_config_section *sections;     /* config sections               */
    struct t_config_section *last_section; /* last config section           */
    struct t_config_file *prev_config;     /* link to previous config file  */
    struct t_config_file *next_config;     /* link to next config file      */
};

struct t_config_section
{
    struct t_config_file *config_file;     /* configuration file            */
    char *name;                            /* section name                  */
    int user_can_add_options;              /* user can add with /set ?      */
    int user_can_delete_options;           /* user can del with /unset ?    */
    int (*callback_read)                   /* called to read a line from    */
    (const void *pointer,                  /* config file (only for some    */
     void *data,                           /* special sections)             */
     struct t_config_file *config_file,
     struct t_config_section *section,
     const char *option_name,
     const char *value);
    const void *callback_read_pointer;     /* pointer sent to read callback */
    void *callback_read_data;              /* data sent to read callback    */
    int (*callback_write)                  /* called to write options       */
    (const void *pointer,                  /* in config file (only for some */
     void *data,                           /* special sections)             */
     struct t_config_file *config_file,
     const char *section_name);
    const void *callback_write_pointer;    /* pointer sent to write cb      */
    void *callback_write_data;             /* data sent to write callback   */
    int (*callback_write_default)          /* called to write default       */
    (const void *pointer,                  /* options in config file        */
     void *data,
     struct t_config_file *config_file,
     const char *section_name);
    const void *callback_write_default_pointer; /* ptr sent to write def.cb */
    void *callback_write_default_data;     /* data sent to write def. callb.*/
    int (*callback_create_option)          /* called to create option in    */
    (const void *pointer,                  /* section                       */
     void *data,
     struct t_config_file *config_file,
     struct t_config_section *section,
     const char *option_name,
     const char *value);
    const void *callback_create_option_pointer; /* ptr sent to create cb    */
    void *callback_create_option_data;     /* data sent to create callback  */
    int (*callback_delete_option)          /* called to delete option in    */
    (const void *pointer,                  /* section                       */
     void *data,
     struct t_config_file *config_file,
     struct t_config_section *section,
     struct t_config_option *option);
    const void *callback_delete_option_pointer; /* ptr sent to delete cb    */
    void *callback_delete_option_data;     /* data sent to delete callback  */
    struct t_config_option *options;       /* options in section            */
    struct t_config_option *last_option;   /* last option in section        */
    struct t_config_section *prev_section; /* link to previous section      */
    struct t_config_section *next_section; /* link to next section          */
};

enum t_config_option_type
{
    CONFIG_OPTION_TYPE_BOOLEAN = 0,
    CONFIG_OPTION_TYPE_INTEGER,
    CONFIG_OPTION_TYPE_STRING,
    CONFIG_OPTION_TYPE_COLOR,
    /* number of option types */
    CONFIG_NUM_OPTION_TYPES,
};

struct t_config_option
{
    struct t_config_file *config_file;     /* configuration file            */
    struct t_config_section *section;      /* section                       */
    char *name;                            /* name                          */
    char *parent_name;                     /* parent name (to inherit the   */
                                           /* value from another option)    */
    enum t_config_option_type type;        /* type                          */
    char *description;                     /* description                   */
    char **string_values;                  /* allowed string values         */
    int min, max;                          /* min and max for value         */
    void *default_value;                   /* default value                 */
    void *value;                           /* value                         */
    int null_value_allowed;                /* null value allowed ?          */
    int (*callback_check_value)            /* called to check value before  */
    (const void *pointer,                  /* assigning new value           */
     void *data,
     struct t_config_option *option,
     const char *value);
    const void *callback_check_value_pointer; /* pointer sent to check cb   */
    void *callback_check_value_data;       /* data sent to check callback   */
    void (*callback_change)                /* called when value is changed  */
    (const void *pointer,
     void *data,
     struct t_config_option *option);
    const void *callback_change_pointer;   /* pointer sent to change cb     */
    void *callback_change_data;            /* data sent to change callback  */
    void (*callback_delete)                /* called when option is deleted */
    (const void *pointer,
     void *data,
     struct t_config_option *option);
    const void *callback_delete_pointer;   /* pointer sent to delete cb     */
    void *callback_delete_data;            /* data sent to delete callback  */
    int loaded;                            /* 1 if opt was found in config  */
    struct t_config_option *prev_option;   /* link to previous option       */
    struct t_config_option *next_option;   /* link to next option           */
};

extern struct t_config_file *config_files;
extern struct t_config_file *last_config_file;

extern char *config_option_type_string[];

extern int config_file_valid (struct t_config_file *config_file);
extern struct t_config_file *config_file_search (const char *name);
extern struct t_config_file *config_file_new (struct t_weechat_plugin *plugin,
                                              const char *name,
                                              int (*callback_reload)(const void *pointer,
                                                                     void *data,
                                                                     struct t_config_file *config_file),
                                              const void *callback_reload_pointer,
                                              void *callback_reload_data);
extern int config_file_set_version (struct t_config_file *config_file,
                                    int version,
                                    struct t_hashtable *(*callback_update)(const void *pointer,
                                                                           void *data,
                                                                           struct t_config_file *config_file,
                                                                           int version_read,
                                                                           struct t_hashtable *data_read),
                                    const void *callback_update_pointer,
                                    void *callback_update_data);
extern struct t_arraylist *config_file_get_configs_by_priority ();
extern struct t_config_section *config_file_new_section (struct t_config_file *config_file,
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
extern struct t_config_section *config_file_search_section (struct t_config_file *config_file,
                                                            const char *name);
extern struct t_config_option *config_file_new_option (struct t_config_file *config_file,
                                                       struct t_config_section *section,
                                                       const char *name, const char *type,
                                                       const char *description,
                                                       const char *string_values,
                                                       int min, int max,
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
extern struct t_config_option *config_file_search_option (struct t_config_file *config_file,
                                                          struct t_config_section *section,
                                                          const char *option_name);
extern void config_file_search_section_option (struct t_config_file *config_file,
                                               struct t_config_section *section,
                                               const char *option_name,
                                               struct t_config_section **section_found,
                                               struct t_config_option **option_found);
extern void config_file_search_with_string (const char *option_name,
                                            struct t_config_file **config_file,
                                            struct t_config_section **section,
                                            struct t_config_option **option,
                                            char **pos_option_name);
extern int config_file_string_to_boolean (const char *text);
extern int config_file_option_reset (struct t_config_option *option,
                                     int run_callback);
extern int config_file_option_set (struct t_config_option *option,
                                   const char *value, int run_callback);
extern int config_file_option_set_null (struct t_config_option *option,
                                        int run_callback);
extern int config_file_option_set_default (struct t_config_option *option,
                                           const char *value,
                                           int run_callback);
extern int config_file_option_toggle (struct t_config_option *option,
                                      const char **values, int num_values,
                                      int run_callback);
extern int config_file_option_unset (struct t_config_option *option);
extern void config_file_option_rename (struct t_config_option *option,
                                       const char *new_name);
extern char *config_file_option_value_to_string (struct t_config_option *option,
                                                 int default_value,
                                                 int add_delimiters,
                                                 int use_colors);
extern const char *config_file_option_get_string (struct t_config_option *option,
                                                  const char *property);
extern void *config_file_option_get_pointer (struct t_config_option *option,
                                             const char *property);
extern int config_file_option_is_null (struct t_config_option *option);
extern int config_file_option_default_is_null (struct t_config_option *option);
extern int config_file_option_has_changed (struct t_config_option *option);
extern int config_file_option_set_with_string (const char *option_name, const char *value);
extern int config_file_option_boolean (struct t_config_option *option);
extern int config_file_option_boolean_default (struct t_config_option *option);
extern int config_file_option_integer (struct t_config_option *option);
extern int config_file_option_integer_default (struct t_config_option *option);
extern const char *config_file_option_string (struct t_config_option *option);
extern const char *config_file_option_string_default (struct t_config_option *option);
extern const char *config_file_option_color (struct t_config_option *option);
extern const char *config_file_option_color_default (struct t_config_option *option);
extern int config_file_write_option (struct t_config_file *config_file,
                                     struct t_config_option *option);
extern int config_file_write_line (struct t_config_file *config_file,
                                   const char *option_name, const char *value, ...);
extern int config_file_write (struct t_config_file *config_files);
extern int config_file_read (struct t_config_file *config_file);
extern int config_file_reload (struct t_config_file *config_file);
extern void config_file_option_free (struct t_config_option *option,
                                     int run_callback);
extern void config_file_section_free_options (struct t_config_section *section);
extern void config_file_section_free (struct t_config_section *section);
extern void config_file_free (struct t_config_file *config_file);
extern void config_file_free_all ();
extern void config_file_free_all_plugin (struct t_weechat_plugin *plugin);
extern struct t_hdata *config_file_hdata_config_file_cb (const void *pointer,
                                                         void *data,
                                                         const char *hdata_name);
extern struct t_hdata *config_file_hdata_config_section_cb (const void *pointer,
                                                            void *data,
                                                            const char *hdata_name);
extern struct t_hdata *config_file_hdata_config_option_cb (const void *pointer,
                                                           void *data,
                                                           const char *hdata_name);
extern int config_file_add_to_infolist (struct t_infolist *infolist,
                                        const char *option_name);
extern void config_file_print_log ();

#endif /* WEECHAT_CONFIG_FILE_H */
