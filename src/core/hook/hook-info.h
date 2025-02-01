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

#ifndef WEECHAT_HOOK_INFO_H
#define WEECHAT_HOOK_INFO_H

struct t_weechat_plugin;
struct t_infolist_item;

#define HOOK_INFO(hook, var) (((struct t_hook_info *)hook->hook_data)->var)

typedef char *(t_hook_callback_info)(const void *pointer, void *data,
                                     const char *info_name,
                                     const char *arguments);

struct t_hook_info
{
    t_hook_callback_info *callback;    /* info callback                     */
    char *info_name;                   /* name of info returned             */
    char *description;                 /* description                       */
    char *args_description;            /* description of arguments          */
};

extern char *hook_info_get_description (struct t_hook *hook);
extern struct t_hook *hook_info (struct t_weechat_plugin *plugin,
                                 const char *info_name,
                                 const char *description,
                                 const char *args_description,
                                 t_hook_callback_info *callback,
                                 const void *callback_pointer,
                                 void *callback_data);
extern char *hook_info_get (struct t_weechat_plugin *plugin,
                            const char *info_name,
                            const char *arguments);
extern void hook_info_free_data (struct t_hook *hook);
extern int hook_info_add_to_infolist (struct t_infolist_item *item,
                                      struct t_hook *hook);
extern void hook_info_print_log (struct t_hook *hook);

#endif /* WEECHAT_HOOK_INFO_H */
