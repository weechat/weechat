/*
 * Copyright (C) 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_HOOK_PRINT_H
#define WEECHAT_HOOK_PRINT_H

#include <time.h>

struct t_weechat_plugin;
struct t_infolist_item;
struct t_gui_buffer;
struct t_gui_line;

#define HOOK_PRINT(hook, var) (((struct t_hook_print *)hook->hook_data)->var)

typedef int (t_hook_callback_print)(const void *pointer, void *data,
                                    struct t_gui_buffer *buffer,
                                    time_t date, int date_usec,
                                    int tags_count, const char **tags,
                                    int displayed, int highlight,
                                    const char *prefix, const char *message);

struct t_hook_print
{
    t_hook_callback_print *callback;   /* print callback                    */
    struct t_gui_buffer *buffer;       /* buffer selected (NULL = all)      */
    int tags_count;                    /* number of tags selected           */
    char ***tags_array;                /* tags selected (NULL = any)        */
    char *message;                     /* part of message (NULL/empty = all)*/
    int strip_colors;                  /* strip colors in msg for callback? */
};

extern char *hook_print_get_description (struct t_hook *hook);
extern struct t_hook *hook_print (struct t_weechat_plugin *plugin,
                                  struct t_gui_buffer *buffer,
                                  const char *tags, const char *message,
                                  int strip_colors,
                                  t_hook_callback_print *callback,
                                  const void *callback_pointer,
                                  void *callback_data);
extern void hook_print_exec (struct t_gui_buffer *buffer,
                             struct t_gui_line *line);
extern void hook_print_free_data (struct t_hook *hook);
extern int hook_print_add_to_infolist (struct t_infolist_item *item,
                                       struct t_hook *hook);
extern void hook_print_print_log (struct t_hook *hook);

#endif /* WEECHAT_HOOK_PRINT_H */
