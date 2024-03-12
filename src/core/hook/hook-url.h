/*
 * Copyright (C) 2023-2024 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_HOOK_URL_H
#define WEECHAT_HOOK_URL_H

struct t_weechat_plugin;
struct t_infolist_item;
struct t_hashtable;

#define HOOK_URL(hook, var) (((struct t_hook_url *)hook->hook_data)->var)

typedef int (t_hook_callback_url)(const void *pointer, void *data,
                                  const char *url,
                                  struct t_hashtable *options,
                                  struct t_hashtable *output);

struct t_hook_url
{
    t_hook_callback_url *callback;     /* URL callback                      */
    char *url;                         /* URL                               */
    struct t_hashtable *options;       /* URL options (see doc)             */
    long timeout;                      /* timeout (ms) (0 = no timeout)     */
    pthread_t thread_id;               /* thread id                         */
    int thread_created;                /* thread created                    */
    int thread_running;                /* 1 if thread is running            */
    struct t_hook *hook_timer;         /* timer to check if thread has ended*/
    struct t_hashtable *output;        /* URL transfer output data          */
};

extern char *hook_url_get_description (struct t_hook *hook);
extern struct t_hook *hook_url (struct t_weechat_plugin *plugin,
                                const char *url,
                                struct t_hashtable *options,
                                int timeout,
                                t_hook_callback_url *callback,
                                const void *callback_pointer,
                                void *callback_data);
extern void hook_url_free_data (struct t_hook *hook);
extern int hook_url_add_to_infolist (struct t_infolist_item *item,
                                         struct t_hook *hook);
extern void hook_url_print_log (struct t_hook *hook);

#endif /* WEECHAT_HOOK_URL_H */
