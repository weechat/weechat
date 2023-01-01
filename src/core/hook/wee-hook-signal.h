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

#ifndef WEECHAT_HOOK_SIGNAL_H
#define WEECHAT_HOOK_SIGNAL_H

struct t_weechat_plugin;
struct t_infolist_item;

#define HOOK_SIGNAL(hook, var) (((struct t_hook_signal *)hook->hook_data)->var)

typedef int (t_hook_callback_signal)(const void *pointer, void *data,
                                     const char *signal, const char *type_data,
                                     void *signal_data);

struct t_hook_signal
{
    t_hook_callback_signal *callback;  /* signal callback                   */
    char **signals;                    /* signals selected; each one may    */
                                       /* begin or end with "*",            */
                                       /* "*" == any signal                 */
    int num_signals;                   /* number of signals                 */
};

extern char *hook_signal_get_description (struct t_hook *hook);
extern struct t_hook *hook_signal (struct t_weechat_plugin *plugin,
                                   const char *signal,
                                   t_hook_callback_signal *callback,
                                   const void *callback_pointer,
                                   void *callback_data);
extern int hook_signal_send (const char *signal, const char *type_data,
                             void *signal_data);
extern void hook_signal_free_data (struct t_hook *hook);
extern int hook_signal_add_to_infolist (struct t_infolist_item *item,
                                        struct t_hook *hook);
extern void hook_signal_print_log (struct t_hook *hook);

#endif /* WEECHAT_HOOK_SIGNAL_H */
