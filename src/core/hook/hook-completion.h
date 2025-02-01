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

#ifndef WEECHAT_HOOK_COMPLETION_H
#define WEECHAT_HOOK_COMPLETION_H

struct t_weechat_plugin;
struct t_infolist_item;
struct t_gui_buffer;
struct t_gui_completion;

#define HOOK_COMPLETION(hook, var) (((struct t_hook_completion *)hook->hook_data)->var)

typedef int (t_hook_callback_completion)(const void *pointer, void *data,
                                         const char *completion_item,
                                         struct t_gui_buffer *buffer,
                                         struct t_gui_completion *completion);

struct t_hook_completion
{
    t_hook_callback_completion *callback; /* completion callback            */
    char *completion_item;                /* name of completion             */
    char *description;                    /* description                    */
};

extern char *hook_completion_get_description (struct t_hook *hook);
extern struct t_hook *hook_completion (struct t_weechat_plugin *plugin,
                                       const char *completion_item,
                                       const char *description,
                                       t_hook_callback_completion *callback,
                                       const void *callback_pointer,
                                       void *callback_data);
extern void hook_completion_exec (struct t_weechat_plugin *plugin,
                                  const char *completion_item,
                                  struct t_gui_buffer *buffer,
                                  struct t_gui_completion *completion);
extern void hook_completion_free_data (struct t_hook *hook);
extern int hook_completion_add_to_infolist (struct t_infolist_item *item,
                                            struct t_hook *hook);
extern void hook_completion_print_log (struct t_hook *hook);

#endif /* WEECHAT_HOOK_COMPLETION_H */
