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

#ifndef WEECHAT_HOOK_HSIGNAL_H
#define WEECHAT_HOOK_HSIGNAL_H

struct t_weechat_plugin;
struct t_infolist_item;

#define HOOK_HSIGNAL(hook, var) (((struct t_hook_hsignal *)hook->hook_data)->var)

typedef int (t_hook_callback_hsignal)(const void *pointer, void *data,
                                      const char *signal,
                                      struct t_hashtable *hashtable);

struct t_hook_hsignal
{
    t_hook_callback_hsignal *callback; /* signal callback                   */
    char **signals;                    /* signals selected; each one may    */
                                       /* begin or end with "*",            */
                                       /* "*" == any signal                 */
    int num_signals;                   /* number of signals                 */
};

extern char *hook_hsignal_get_description (struct t_hook *hook);
extern struct t_hook *hook_hsignal (struct t_weechat_plugin *plugin,
                                    const char *signal,
                                    t_hook_callback_hsignal *callback,
                                    const void *callback_pointer,
                                    void *callback_data);
extern int hook_hsignal_send (const char *signal,
                              struct t_hashtable *hashtable);
extern void hook_hsignal_free_data (struct t_hook *hook);
extern int hook_hsignal_add_to_infolist (struct t_infolist_item *item,
                                         struct t_hook *hook);
extern void hook_hsignal_print_log (struct t_hook *hook);

#endif /* WEECHAT_HOOK_HSIGNAL_H */
