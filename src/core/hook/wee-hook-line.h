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

#ifndef WEECHAT_HOOK_LINE_H
#define WEECHAT_HOOK_LINE_H

struct t_weechat_plugin;
struct t_infolist_item;
struct t_hashtable;
struct t_gui_line;

#define HOOK_LINE(hook, var) (((struct t_hook_line *)hook->hook_data)->var)

typedef struct t_hashtable *(t_hook_callback_line)(const void *pointer,
                                                   void *data,
                                                   struct t_hashtable *line);

struct t_hook_line
{
    t_hook_callback_line *callback;    /* line callback                     */
    int buffer_type;                   /* -1 = any type, ≥ 0: only this type*/
    char **buffers;                    /* list of buffer masks where the    */
                                       /* hook is executed (see the         */
                                       /* function "buffer_match_list")     */
    int num_buffers;                   /* number of buffers in list         */
    int tags_count;                    /* number of tags selected           */
    char ***tags_array;                /* tags selected (NULL = any)        */
};

extern char *hook_line_get_description (struct t_hook *hook);
extern struct t_hook *hook_line (struct t_weechat_plugin *plugin,
                                 const char *buffer_type,
                                 const char *buffer_name,
                                 const char *tags,
                                 t_hook_callback_line *callback,
                                 const void *callback_pointer,
                                 void *callback_data);
extern void hook_line_exec (struct t_gui_line *line);
extern void hook_line_free_data (struct t_hook *hook);
extern int hook_line_add_to_infolist (struct t_infolist_item *item,
                                      struct t_hook *hook);
extern void hook_line_print_log (struct t_hook *hook);

#endif /* WEECHAT_HOOK_LINE_H */
