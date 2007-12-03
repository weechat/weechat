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


#ifndef __WEECHAT_CONFIG_FILE_H
#define __WEECHAT_CONFIG_FILE_H 1

#define CONFIG_BOOLEAN(option) (*((char *)((option)->value)))
#define CONFIG_BOOLEAN_DEFAULT(option) (*((char *)((option)->default_value)))

#define CONFIG_INTEGER(option) (*((int *)((option)->value)))
#define CONFIG_INTEGER_DEFAULT(option) (*((int *)((option)->default_value)))

#define CONFIG_STRING(option) ((char *)((option)->value))
#define CONFIG_STRING_DEFAULT(option) ((char *)((option)->default_value))

#define CONFIG_COLOR(option) (*((int *)((option)->value)))
#define CONFIG_COLOR_DEFAULT(option) (*((int *)((option)->default_value)))

#define CONFIG_BOOLEAN_FALSE  0
#define CONFIG_BOOLEAN_TRUE   1

struct t_config_file
{
    struct t_weechat_plugin *plugin;       /* plugin which created this cfg */
    char *filename;                        /* config filename (without path)*/
    FILE *file;                            /* file pointer                  */
    struct t_config_section *sections;     /* config sections               */
    struct t_config_section *last_section; /* last config section           */
    struct t_config_file *prev_config;     /* link to previous config file  */
    struct t_config_file *next_config;     /* link to next config file      */
};

struct t_config_section
{
    char *name;                            /* section name                  */
    void (*callback_read)                  /* called when unknown option    */
    (void *, char *, char *);              /* is read from config file      */
    void (*callback_write)                 /* called to write special       */
    (void *);                              /* options in config file        */
    void (*callback_write_default)         /* called to write default       */
    (void *);                              /* options in config file        */
    struct t_config_option *options;       /* options in section            */
    struct t_config_option *last_option;   /* last option in section        */
    struct t_config_section *prev_section; /* link to previous section      */
    struct t_config_section *next_section; /* link to next section          */
};

enum t_config_option_type
{
    CONFIG_OPTION_BOOLEAN = 0,
    CONFIG_OPTION_INTEGER,
    CONFIG_OPTION_STRING,
    CONFIG_OPTION_COLOR,
};

struct t_config_option
{
    char *name;                            /* name                          */
    enum t_config_option_type type;        /* type                          */
    char *description;                     /* description                   */
    char **string_values;                  /* allowed string values         */
    int min, max;                          /* min and max for value         */
    void *default_value;                   /* default value                 */
    void *value;                           /* value                         */
    void (*callback_change)();             /* called when value is changed  */
    int loaded;                            /* 1 if opt was found in config  */
    struct t_config_option *prev_option;   /* link to previous option       */
    struct t_config_option *next_option;   /* link to next option           */
};

extern struct t_config_file *config_file_new (void *, char *);
extern int config_file_valid_for_plugin (void *, struct t_config_file *);
extern struct t_config_section *config_file_new_section (struct t_config_file *,
                                                         char *,
                                                         void (*)(void *, char *, char *),
                                                         void (*)(void *),
                                                         void (*)(void *));
extern struct t_config_section *config_file_search_section (struct t_config_file *,
                                                            char *);
extern struct t_config_option *config_file_new_option_boolean (struct t_config_section *,
                                                               char *, char *,
                                                               int,
                                                               void (*)());
extern struct t_config_option *config_file_new_option_integer (struct t_config_section *,
                                                               char *, char *,
                                                               int, int, int,
                                                               void (*)());
extern struct t_config_option *config_file_new_option_integer_with_string (struct t_config_section *,
                                                                           char *,
                                                                           char *,
                                                                           char *,
                                                                           int,
                                                                           void (*)());
extern struct t_config_option *config_file_new_option_string (struct t_config_section *,
                                                              char *, char *,
                                                              int, int,
                                                              char *, void (*)());
extern struct t_config_option *config_file_new_option_color (struct t_config_section *,
                                                             char *, char *,
                                                             int, char *,
                                                             void (*)());
extern struct t_config_option *config_file_search_option (struct t_config_file *,
                                                          struct t_config_section *,
                                                          char *);
extern int config_file_option_valid_for_plugin (void *, struct t_config_option *);
extern int config_file_string_boolean_value (char *);
extern int config_file_option_set (struct t_config_option *, char *);
extern int config_file_option_reset (struct t_config_option *);

extern int config_file_read (struct t_config_file *);
extern int config_file_reload (struct t_config_file *);
extern void config_file_write_line (struct t_config_file *, char *, char *);
extern int config_file_write (struct t_config_file *, int);
extern void config_file_option_free (struct t_config_section *,
                                     struct t_config_option *);
extern void config_file_section_free (struct t_config_file *,
                                      struct t_config_section *);
extern void config_file_free (struct t_config_file *);
extern void config_file_free_all ();
extern void config_file_print_stdout (struct t_config_file *);
extern void config_file_print_log ();

#endif /* wee-config-file.h */
