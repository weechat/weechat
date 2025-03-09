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

#ifndef WEECHAT_HOOK_FD_H
#define WEECHAT_HOOK_FD_H

struct t_weechat_plugin;
struct t_infolist_item;

#define HOOK_FD(hook, var) (((struct t_hook_fd *)hook->hook_data)->var)

/* flags for fd hooks */
#define HOOK_FD_FLAG_READ      (1 << 0)
#define HOOK_FD_FLAG_WRITE     (1 << 1)
#define HOOK_FD_FLAG_EXCEPTION (1 << 2)

typedef int (t_hook_callback_fd)(const void *pointer, void *data, int fd);

struct t_hook_fd
{
    t_hook_callback_fd *callback;      /* fd callback                       */
    int fd;                            /* socket or file descriptor         */
    int flags;                         /* fd flags (read,write,..)          */
    int error;                         /* contains errno if error occurred  */
                                       /* with fd                           */
};

extern char *hook_fd_get_description (struct t_hook *hook);
extern void hook_fd_add_cb (struct t_hook *hook);
extern void hook_fd_remove_cb (struct t_hook *hook);
extern struct t_hook *hook_fd (struct t_weechat_plugin *plugin, int fd,
                               int flag_read, int flag_write,
                               int flag_exception,
                               t_hook_callback_fd *callback,
                               const void *callback_pointer,
                               void *callback_data);
extern void hook_fd_exec (void);
extern void hook_fd_free_data (struct t_hook *hook);
extern int hook_fd_add_to_infolist (struct t_infolist_item *item,
                                    struct t_hook *hook);
extern void hook_fd_print_log (struct t_hook *hook);

#endif /* WEECHAT_HOOK_FD_H */
