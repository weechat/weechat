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


#ifndef __WEECHAT_CONFIG_OPTION_H
#define __WEECHAT_CONFIG_OPTION_H 1

#define OPTION_TYPE_BOOLEAN         1   /* values: on/off                   */
#define OPTION_TYPE_INT             2   /* values: from min to max          */
#define OPTION_TYPE_INT_WITH_STRING 3   /* values: one from **array_values  */
#define OPTION_TYPE_STRING          4   /* values: any string, may be empty */
#define OPTION_TYPE_COLOR           5   /* values: a color struct           */


#define BOOL_FALSE 0
#define BOOL_TRUE  1

struct t_config_option
{
    char *name;
    char *description;
    int type;
    int min, max;
    int default_int;
    char *default_string;
    char **array_values;
    int *ptr_int;
    char **ptr_string;
    void (*handler_change)();
};

extern int config_option_get_pos_array_values (char **, char *);
extern void config_option_list_remove (char **, char *);
extern void config_option_list_set (char **, char *, char *);
extern void config_option_list_get_value (char **, char *, char **, int *);
extern int config_option_option_get_boolean_value (char *);
extern int config_option_set (struct t_config_option *, char *);
extern struct t_config_option *config_option_search (struct t_config_option *, char *);
extern struct t_config_option *config_option_section_option_search (char **,
                                                                    struct t_config_option **,
                                                                    char *);
extern void config_option_section_option_search_get_value (char **, struct t_config_option **,
                                                           char *, struct t_config_option **,
                                                           void **);
extern int config_option_section_option_set_value_by_name (char **,
                                                           struct t_config_option **,
                                                           char *, char *);
extern int config_option_section_get_index (char **, char *);
extern void config_option_section_option_set_default_values (char **,
                                                             struct t_config_option **);
extern void config_option_print_stdout (char **, struct t_config_option **);

#endif /* wee-config-option.h */
