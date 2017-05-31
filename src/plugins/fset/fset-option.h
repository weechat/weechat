/*
 * Copyright (C) 2003-2017 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_FSET_OPTION_H
#define WEECHAT_FSET_OPTION_H 1

#define FSET_OPTION_VALUE_NULL "null"

struct t_fset_option
{
    char *name;                          /* option name                     */
    char *parent_name;                   /* parent option name              */
    char *type;                          /* option type                     */
    char *default_value;                 /* option default value            */
    char *value;                         /* option value                    */
    char *parent_value;                  /* parent option value             */
    char *min;                           /* min value                       */
    char *max;                           /* max value                       */
    char *description;                   /* option description              */
    char *string_values;                 /* string values for option        */
    struct t_fset_option *prev_option;   /* link to previous option         */
    struct t_fset_option *next_option;   /* link to next option             */
};

extern struct t_arraylist *fset_options;
extern struct t_hashtable *fset_option_max_length_field;
extern char *fset_option_filter;

extern int fset_option_valid (struct t_fset_option *option);
extern struct t_fset_option *fset_option_search_by_name (const char *name,
                                                         int *line);
extern int fset_option_value_different_from_default (struct t_fset_option *option);
extern void fset_option_free (struct t_fset_option *fset_option);
extern struct t_arraylist *fset_option_get_arraylist_options ();
extern struct t_hashtable *fset_option_get_hashtable_max_length_field ();
extern void fset_option_get_options ();
extern void fset_option_set_filter (const char *filter);
extern void fset_option_filter_options (const char *filter);
extern int fset_option_config_cb (const void *pointer,
                                  void *data,
                                  const char *option,
                                  const char *value);
extern struct t_hdata *fset_option_hdata_option_cb (const void *pointer,
                                                    void *data,
                                                    const char *hdata_name);
extern int fset_option_add_to_infolist (struct t_infolist *infolist,
                                        struct t_fset_option *option);
extern void fset_option_print_log ();
extern int fset_option_init ();
extern void fset_option_end ();

#endif /* WEECHAT_FSET_OPTION_H */
