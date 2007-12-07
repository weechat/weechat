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


#ifndef __WEECHAT_PLUGIN_INFOLIST_H
#define __WEECHAT_PLUGIN_INFOLIST_H 1

/* list structures */

enum t_plugin_infolist_type
{
    PLUGIN_INFOLIST_INTEGER = 0,
    PLUGIN_INFOLIST_STRING,
    PLUGIN_INFOLIST_POINTER,
    PLUGIN_INFOLIST_TIME,
};

struct t_plugin_infolist_var
{
    char *name;                             /* variable name                */
    enum t_plugin_infolist_type type;       /* type: integer, string, time  */
    void *value;                            /* pointer to value             */
    struct t_plugin_infolist_var *prev_var; /* link to previous variable    */
    struct t_plugin_infolist_var *next_var; /* link to next variable        */
};

struct t_plugin_infolist_item
{
    struct t_plugin_infolist_var *vars;       /* item variables             */
    struct t_plugin_infolist_var *last_var;   /* last variable              */
    char *fields;                             /* fields list (NULL if never */
                                              /*              asked)        */
    struct t_plugin_infolist_item *prev_item; /* link to previous item      */
    struct t_plugin_infolist_item *next_item; /* link to next item          */
};

struct t_plugin_infolist
{
    struct t_plugin_infolist_item *items;     /* link to items                  */
    struct t_plugin_infolist_item *last_item; /* last variable                  */
    struct t_plugin_infolist_item *ptr_item;  /* pointer to current item        */
    struct t_plugin_infolist *prev_infolist;  /* link to previous list          */
    struct t_plugin_infolist *next_infolist;  /* link to next list              */
};

/* list variables */

extern struct t_plugin_infolist *plugin_infolists;
extern struct t_plugin_infolist *last_plugin_infolist;

/* list functions */

extern struct t_plugin_infolist *plugin_infolist_new ();
extern struct t_plugin_infolist_item *plugin_infolist_new_item (struct t_plugin_infolist *);
extern struct t_plugin_infolist_var *plugin_infolist_new_var_integer (struct t_plugin_infolist_item *,
                                                                      char *, int);
extern struct t_plugin_infolist_var *plugin_infolist_new_var_string (struct t_plugin_infolist_item *,
                                                                     char *, char *);
extern struct t_plugin_infolist_var *plugin_infolist_new_var_pointer (struct t_plugin_infolist_item *,
                                                                      char *, void *);
extern struct t_plugin_infolist_var *plugin_infolist_new_var_time (struct t_plugin_infolist_item *,
                                                                   char *, time_t);
extern int plugin_infolist_valid (struct t_plugin_infolist *);
extern struct t_plugin_infolist_item *plugin_infolist_next_item (struct t_plugin_infolist *);
extern struct t_plugin_infolist_item *plugin_infolist_prev_item (struct t_plugin_infolist *);
extern char *plugin_infolist_get_fields (struct t_plugin_infolist *);
extern int plugin_infolist_get_integer (struct t_plugin_infolist *, char *);
extern char *plugin_infolist_get_string (struct t_plugin_infolist *, char *);
extern void *plugin_infolist_get_pointer (struct t_plugin_infolist *, char *);
extern time_t plugin_infolist_get_time (struct t_plugin_infolist *, char *);
extern void plugin_infolist_free (struct t_plugin_infolist *);
extern void plugin_infolist_print_log ();

#endif /* plugin-infolist.h */
