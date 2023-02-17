/*
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

#ifndef WEECHAT_INFOLIST_H
#define WEECHAT_INFOLIST_H

#include <time.h>

struct t_weechat_plugin;

/* list structures */

enum t_infolist_type
{
    INFOLIST_INTEGER = 0,
    INFOLIST_STRING,
    INFOLIST_POINTER,
    INFOLIST_BUFFER,
    INFOLIST_TIME,
};

struct t_infolist_var
{
    char *name;                        /* variable name                     */
    enum t_infolist_type type;         /* type: int, string, ...            */
    void *value;                       /* pointer to value                  */
    int size;                          /* for type buffer                   */
    struct t_infolist_var *prev_var;   /* link to previous variable         */
    struct t_infolist_var *next_var;   /* link to next variable             */
};

struct t_infolist_item
{
    struct t_infolist_var *vars;       /* item variables                    */
    struct t_infolist_var *last_var;   /* last variable                     */
    char *fields;                      /* fields list (NULL if never asked) */
    struct t_infolist_item *prev_item; /* link to previous item             */
    struct t_infolist_item *next_item; /* link to next item                 */
};

struct t_infolist
{
    struct t_weechat_plugin *plugin;   /* plugin which created this infolist*/
                                       /* (NULL if created by WeeChat)      */
    struct t_infolist_item *items;     /* link to items                     */
    struct t_infolist_item *last_item; /* last variable                     */
    struct t_infolist_item *ptr_item;  /* pointer to current item           */
    struct t_infolist *prev_infolist;  /* link to previous list             */
    struct t_infolist *next_infolist;  /* link to next list                 */
};

/* list variables */

extern struct t_infolist *weechat_infolists;
extern struct t_infolist *last_weechat_infolist;

/* list functions */

extern struct t_infolist *infolist_new (struct t_weechat_plugin *plugin);
extern int infolist_valid (struct t_infolist *infolist);
extern struct t_infolist_item *infolist_new_item (struct t_infolist *infolist);
extern struct t_infolist_var *infolist_new_var_integer (struct t_infolist_item *item,
                                                        const char *name,
                                                        int value);
extern struct t_infolist_var *infolist_new_var_string (struct t_infolist_item *item,
                                                       const char *name,
                                                       const char *value);
extern struct t_infolist_var *infolist_new_var_pointer (struct t_infolist_item *item,
                                                        const char *name,
                                                        void *pointer);
extern struct t_infolist_var *infolist_new_var_buffer (struct t_infolist_item *item,
                                                       const char *name,
                                                       void *pointer,
                                                       int size);
extern struct t_infolist_var *infolist_new_var_time (struct t_infolist_item *item,
                                                     const char *name,
                                                     time_t time);
extern struct t_infolist_var *infolist_search_var (struct t_infolist *infolist,
                                                   const char *name);
extern struct t_infolist_item *infolist_next (struct t_infolist *infolist);
extern struct t_infolist_item *infolist_prev (struct t_infolist *infolist);
extern void infolist_reset_item_cursor (struct t_infolist *infolist);
extern const char *infolist_fields (struct t_infolist *infolist);
extern int infolist_integer (struct t_infolist *infolist,
                             const char *var);
extern const char *infolist_string (struct t_infolist *infolist,
                              const char *var);
extern void *infolist_pointer (struct t_infolist *infolist,
                               const char *var);
extern void *infolist_buffer (struct t_infolist *infolist,
                              const char *var, int *size);
extern time_t infolist_time (struct t_infolist *infolist,
                             const char *var);
extern void infolist_free (struct t_infolist *infolist);
extern void infolist_free_all_plugin (struct t_weechat_plugin *plugin);
extern void infolist_print_log ();

#endif /* WEECHAT_INFOLIST_H */
