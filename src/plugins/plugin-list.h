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


#ifndef __WEECHAT_PLUGIN_LIST_H
#define __WEECHAT_PLUGIN_LIST_H 1

/* list structures */

enum t_plugin_var_type
{
    PLUGIN_LIST_VAR_INTEGER = 0,
    PLUGIN_LIST_VAR_STRING,
    PLUGIN_LIST_VAR_POINTER,
    PLUGIN_LIST_VAR_TIME,
};

struct t_plugin_list_var
{
    char *name;                           /* variable name                  */
    enum t_plugin_var_type type;          /* type: integer, string, time    */
    void *value;                          /* pointer to value               */
    struct t_plugin_list_var *prev_var;   /* link to previous variable      */
    struct t_plugin_list_var *next_var;   /* link to next variable          */
};

struct t_plugin_list_item
{
    struct t_plugin_list_var *vars;       /* item variables                 */
    struct t_plugin_list_var *last_var;   /* last variable                  */
    char *fields;                         /* fields list (NULL if never     */
                                          /*              asked)            */
    struct t_plugin_list_item *prev_item; /* link to previous item          */
    struct t_plugin_list_item *next_item; /* link to next item              */
};

struct t_plugin_list
{
    struct t_plugin_list_item *items;     /* link to items                  */
    struct t_plugin_list_item *last_item; /* last variable                  */
    struct t_plugin_list_item *ptr_item;  /* pointer to current item        */
    struct t_plugin_list *prev_list;      /* link to previous list          */
    struct t_plugin_list *next_list;      /* link to next list              */
};

/* list variables */

extern struct t_plugin_list *plugin_lists;
extern struct t_plugin_list *last_plugin_list;

/* list functions */

extern struct t_plugin_list *plugin_list_new ();
extern struct t_plugin_list_item *plugin_list_new_item (struct t_plugin_list *);
extern struct t_plugin_list_var *plugin_list_new_var_int (struct t_plugin_list_item *,
                                                          char *, int);
extern struct t_plugin_list_var *plugin_list_new_var_string (struct t_plugin_list_item *,
                                                             char *, char *);
extern struct t_plugin_list_var *plugin_list_new_var_pointer (struct t_plugin_list_item *,
                                                              char *, void *);
extern struct t_plugin_list_var *plugin_list_new_var_time (struct t_plugin_list_item *,
                                                           char *, time_t);
extern int plugin_list_valid (struct t_plugin_list *);
extern struct t_plugin_list_item *plugin_list_next_item (struct t_plugin_list *);
extern struct t_plugin_list_item *plugin_list_prev_item (struct t_plugin_list *);
extern char *plugin_list_get_fields (struct t_plugin_list *);
extern int plugin_list_get_int (struct t_plugin_list *, char *);
extern char *plugin_list_get_string (struct t_plugin_list *, char *);
extern void *plugin_list_get_pointer (struct t_plugin_list *, char *);
extern time_t plugin_list_get_time (struct t_plugin_list *, char *);
extern void plugin_list_free (struct t_plugin_list *);
extern void plugin_list_print_log ();

#endif /* plugin-list.h */
