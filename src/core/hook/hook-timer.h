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

#ifndef WEECHAT_HOOK_TIMER_H
#define WEECHAT_HOOK_TIMER_H

#include <time.h>
#include <sys/time.h>

struct t_weechat_plugin;
struct t_infolist_item;

#define HOOK_TIMER(hook, var) (((struct t_hook_timer *)hook->hook_data)->var)

typedef int (t_hook_callback_timer)(const void *pointer, void *data,
                                    int remaining_calls);

struct t_hook_timer
{
    t_hook_callback_timer *callback;   /* timer callback                    */
    long interval;                     /* timer interval (milliseconds)     */
    int align_second;                  /* alignment on a second             */
                                       /* for ex.: 60 = each min. at 0 sec  */
    int remaining_calls;               /* calls remaining (0 = unlimited)   */
    struct timeval last_exec;          /* last time hook was executed       */
    struct timeval next_exec;          /* next scheduled execution          */
};

extern time_t hook_last_system_time;

extern char *hook_timer_get_description (struct t_hook *hook);
extern struct t_hook *hook_timer (struct t_weechat_plugin *plugin,
                                  long interval, int align_second,
                                  int max_calls,
                                  t_hook_callback_timer *callback,
                                  const void *callback_pointer,
                                  void *callback_data);
extern int hook_timer_get_time_to_next (void);
extern void hook_timer_exec (void);
extern void hook_timer_free_data (struct t_hook *hook);
extern int hook_timer_add_to_infolist (struct t_infolist_item *item,
                                       struct t_hook *hook);
extern void hook_timer_print_log (struct t_hook *hook);

#endif /* WEECHAT_HOOK_TIMER_H */
