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

#ifndef WEECHAT_HOOK_FOCUS_H
#define WEECHAT_HOOK_FOCUS_H

struct t_weechat_plugin;
struct t_hashtable;
struct t_infolist_item;

#define HOOK_FOCUS(hook, var) (((struct t_hook_focus *)hook->hook_data)->var)

typedef struct t_hashtable *(t_hook_callback_focus)(const void *pointer,
                                                    void *data,
                                                    struct t_hashtable *info);

struct t_hook_focus
{
    t_hook_callback_focus *callback;    /* focus callback                   */
    char *area;                         /* "chat" or bar item name          */
};

extern char *hook_focus_get_description (struct t_hook *hook);
extern struct t_hook *hook_focus (struct t_weechat_plugin *plugin,
                                  const char *area,
                                  t_hook_callback_focus *callback,
                                  const void *callback_pointer,
                                  void *callback_data);
extern struct t_hashtable *hook_focus_get_data (struct t_hashtable *hashtable_focus1,
                                                struct t_hashtable *hashtable_focus2);
extern void hook_focus_free_data (struct t_hook *hook);
extern int hook_focus_add_to_infolist (struct t_infolist_item *item,
                                       struct t_hook *hook);
extern void hook_focus_print_log (struct t_hook *hook);

#endif /* WEECHAT_HOOK_FOCUS_H */
