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

#ifndef WEECHAT_HOOK_MODIFIER_H
#define WEECHAT_HOOK_MODIFIER_H

struct t_weechat_plugin;
struct t_infolist_item;

#define HOOK_MODIFIER(hook, var) (((struct t_hook_modifier *)hook->hook_data)->var)

typedef char *(t_hook_callback_modifier)(const void *pointer, void *data,
                                         const char *modifier,
                                         const char *modifier_data,
                                         const char *string);

struct t_hook_modifier
{
    t_hook_callback_modifier *callback; /* modifier callback                */
    char *modifier;                     /* name of modifier                 */
};

extern char *hook_modifier_get_description (struct t_hook *hook);
extern struct t_hook *hook_modifier (struct t_weechat_plugin *plugin,
                                     const char *modifier,
                                     t_hook_callback_modifier *callback,
                                     const void *callback_pointer,
                                     void *callback_data);
extern char *hook_modifier_exec (struct t_weechat_plugin *plugin,
                                 const char *modifier,
                                 const char *modifier_data,
                                 const char *string);
extern void hook_modifier_free_data (struct t_hook *hook);
extern int hook_modifier_add_to_infolist (struct t_infolist_item *item,
                                          struct t_hook *hook);
extern void hook_modifier_print_log (struct t_hook *hook);

#endif /* WEECHAT_HOOK_MODIFIER_H */
