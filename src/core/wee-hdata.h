/*
 * Copyright (C) 2011-2013 Sebastien Helleu <flashcode@flashtux.org>
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

#ifndef __WEECHAT_HDATA_H
#define __WEECHAT_HDATA_H 1

#define HDATA_VAR(__struct, __name, __type, __update_allowed,           \
                  __array_size, __hdata_name)                           \
    hdata_new_var (hdata, #__name, offsetof (__struct, __name),         \
                   WEECHAT_HDATA_##__type, __update_allowed,            \
                   __array_size, __hdata_name)
#define HDATA_LIST(__name) hdata_new_list (hdata, #__name, &(__name));

struct t_hdata_var
{
    int offset;                        /* offset                            */
    char type;                         /* type                              */
    char update_allowed;               /* update allowed?                   */
    char *array_size;                  /* array size                        */
    char *hdata_name;                  /* hdata name                        */
};

struct t_hdata
{
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
                            void *pointer);
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
extern char hdata_char (struct t_hdata *hdata, void *pointer,
                        const char *name);
extern int hdata_integer (struct t_hdata *hdata, void *pointer,
                          const char *name);
extern long hdata_long (struct t_hdata *hdata, void *pointer,
                        const char *name);
extern const char *hdata_string (struct t_hdata *hdata, void *pointer,
                                 const char *name);
extern void *hdata_pointer (struct t_hdata *hdata, void *pointer,
                            const char *name);
extern time_t hdata_time (struct t_hdata *hdata, void *pointer,
                          const char *name);
extern struct t_hashtable *hdata_hashtable (struct t_hdata *hdata,
                                            void *pointer, const char *name);
extern int hdata_set (struct t_hdata *hdata, void *pointer, const char *name,
                      const char *value);
extern int hdata_update (struct t_hdata *hdata, void *pointer,
                         struct t_hashtable *hashtable);
extern const char *hdata_get_string (struct t_hdata *hdata,
                                     const char *property);
extern void hdata_free_all_plugin (struct t_weechat_plugin *plugin);
extern void hdata_free_all ();
extern void hdata_print_log ();
extern void hdata_init ();
extern void hdata_end ();

#endif /* __WEECHAT_HDATA_H */
