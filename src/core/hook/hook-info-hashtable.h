/*
 * Copyright (C) 2003-2024 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_HOOK_INFO_HASHTABLE_H
#define WEECHAT_HOOK_INFO_HASHTABLE_H

struct t_weechat_plugin;
struct t_infolist_item;

#define HOOK_INFO_HASHTABLE(hook, var) (((struct t_hook_info_hashtable *)hook->hook_data)->var)

typedef struct t_hashtable *(t_hook_callback_info_hashtable)(const void *pointer,
                                                             void *data,
                                                             const char *info_name,
                                                             struct t_hashtable *hashtable);

struct t_hook_info_hashtable
{
    t_hook_callback_info_hashtable *callback; /* info_hashtable callback    */
    char *info_name;                   /* name of info returned             */
    char *description;                 /* description                       */
    char *args_description;            /* description of arguments          */
    char *output_description;          /* description of output (hashtable) */
};

extern char *hook_info_hashtable_get_description (struct t_hook *hook);
extern struct t_hook *hook_info_hashtable (struct t_weechat_plugin *plugin,
                                           const char *info_name,
                                           const char *description,
                                           const char *args_description,
                                           const char *output_description,
                                           t_hook_callback_info_hashtable *callback,
                                           const void *callback_pointer,
                                           void *callback_data);
extern struct t_hashtable *hook_info_get_hashtable (struct t_weechat_plugin *plugin,
                                                    const char *info_name,
                                                    struct t_hashtable *hashtable);
extern void hook_info_hashtable_free_data (struct t_hook *hook);
extern int hook_info_hashtable_add_to_infolist (struct t_infolist_item *item,
                                                struct t_hook *hook);
extern void hook_info_hashtable_print_log (struct t_hook *hook);

#endif /* WEECHAT_HOOK_INFO_HASHTABLE_H */
