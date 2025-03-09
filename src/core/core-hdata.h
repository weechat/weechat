/*
 * Copyright (C) 2011-2025 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_HDATA_H
#define WEECHAT_HDATA_H

#include <time.h>

/* create a hdata variable (name is the same as the struct field */
#define HDATA_VAR(__struct, __field, __type, __update_allowed,          \
                  __array_size, __hdata_name)                           \
    hdata_new_var (hdata, #__field, offsetof (__struct, __field),       \
                   WEECHAT_HDATA_##__type, __update_allowed,            \
                   __array_size, __hdata_name)
/* create a hdata variable with a custom name */
#define HDATA_VAR_NAME(__struct, __field, __name, __type,               \
                       __update_allowed, __array_size, __hdata_name)    \
    hdata_new_var (hdata, __name, offsetof (__struct, __field),         \
                   WEECHAT_HDATA_##__type, __update_allowed,            \
                   __array_size, __hdata_name)
/* create a hdata list */
#define HDATA_LIST(__name, __flags)                                     \
    hdata_new_list (hdata, #__name, &(__name), __flags);

struct t_hdata_var
{
    int offset;                        /* offset                            */
    char type;                         /* type                              */
    char update_allowed;               /* update allowed?                   */
    char *array_size;                  /* array size                        */
    char array_pointer;                /* pointer to dynamically allocated  */
                                       /* array?                            */
    char *hdata_name;                  /* hdata name                        */
};

struct t_hdata_list
{
    void *pointer;                     /* list pointer                      */
    int flags;                         /* flags for list                    */
};

struct t_hdata
{
    char *name;                        /* name of hdata                     */
    struct t_weechat_plugin *plugin;   /* plugin which created this hdata   */
                                       /* (NULL if created by WeeChat)      */
    char *var_prev;                    /* name of var with pointer to       */
                                       /* previous element in list          */
    char *var_next;                    /* name of var with pointer to       */
                                       /* next element in list              */
    struct t_hashtable *hash_var;      /* hash with type & offset of vars   */
    struct t_hashtable *hash_list;     /* hashtable with pointers on lists  */
                                       /* (used to search objects)          */

    char create_allowed;               /* create allowed?                   */
    char delete_allowed;               /* delete allowed?                   */
    int (*callback_update)             /* update callback                   */
    (void *data,
     struct t_hdata *hdata,
     void *pointer,
     struct t_hashtable *hashtable);
    void *callback_update_data;        /* data sent to update callback      */

    /* internal vars */
    char update_pending;               /* update pending: hdata_set allowed */
};

extern struct t_hashtable *weechat_hdata;

extern char *hdata_type_string[];

extern struct t_hdata *hdata_new (struct t_weechat_plugin *plugin,
                                  const char *hdata_name, const char *var_prev,
                                  const char *var_next,
                                  int create_allowed, int delete_allowed,
                                  int (*callback_update)(void *data,
                                                         struct t_hdata *hdata,
                                                         void *pointer,
                                                         struct t_hashtable *hashtable),
                                  void *callback_update_data);
extern void hdata_new_var (struct t_hdata *hdata, const char *name, int offset,
                           int type, int update_allowed, const char *array_size,
                           const char *hdata_name);
extern void hdata_new_list (struct t_hdata *hdata, const char *name,
                            void *pointer, int flags);
extern int hdata_get_var_offset (struct t_hdata *hdata, const char *name);
extern int hdata_get_var_type (struct t_hdata *hdata, const char *name);
extern const char *hdata_get_var_type_string (struct t_hdata *hdata,
                                              const char *name);
extern int hdata_get_var_array_size (struct t_hdata *hdata, void *pointer,
                                     const char *name);
extern const char *hdata_get_var_array_size_string (struct t_hdata *hdata,
                                                    void *pointer,
                                                    const char *name);
extern const char *hdata_get_var_hdata (struct t_hdata *hdata,
                                        const char *name);
extern void *hdata_get_var (struct t_hdata *hdata, void *pointer,
                            const char *name);
extern void *hdata_get_var_at_offset (struct t_hdata *hdata, void *pointer,
                                      int offset);
extern void *hdata_get_list (struct t_hdata *hdata, const char *name);
extern int hdata_check_pointer (struct t_hdata *hdata, void *list,
                                void *pointer);
extern void *hdata_move (struct t_hdata *hdata, void *pointer, int count);
extern void *hdata_search (struct t_hdata *hdata, void *pointer,
                           const char *search, struct t_hashtable *pointers,
                           struct t_hashtable *extra_vars,
                           struct t_hashtable *options, int move);
extern int hdata_count (struct t_hdata *hdata, void *pointer);
extern void hdata_get_index_and_name (const char *name, int *index,
                                      const char **ptr_name);
extern char hdata_char (struct t_hdata *hdata, void *pointer,
                        const char *name);
extern int hdata_integer (struct t_hdata *hdata, void *pointer,
                          const char *name);
extern long hdata_long (struct t_hdata *hdata, void *pointer,
                        const char *name);
extern long long hdata_longlong (struct t_hdata *hdata, void *pointer,
                                 const char *name);
extern const char *hdata_string (struct t_hdata *hdata, void *pointer,
                                 const char *name);
extern void *hdata_pointer (struct t_hdata *hdata, void *pointer,
                            const char *name);
extern time_t hdata_time (struct t_hdata *hdata, void *pointer,
                          const char *name);
extern struct t_hashtable *hdata_hashtable (struct t_hdata *hdata,
                                            void *pointer, const char *name);
extern int hdata_compare (struct t_hdata *hdata, void *pointer1,
                          void *pointer2, const char *name,
                          int case_sensitive);
extern int hdata_set (struct t_hdata *hdata, void *pointer, const char *name,
                      const char *value);
extern int hdata_update (struct t_hdata *hdata, void *pointer,
                         struct t_hashtable *hashtable);
extern const char *hdata_get_string (struct t_hdata *hdata,
                                     const char *property);
extern void hdata_free_all_plugin (struct t_weechat_plugin *plugin);
extern void hdata_free_all (void);
extern void hdata_print_log (void);
extern void hdata_init (void);
extern void hdata_end (void);

#endif /* WEECHAT_HDATA_H */
