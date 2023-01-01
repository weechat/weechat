/*
 * wee-hook-fd.c - WeeChat fd hook
 *
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <fcntl.h>
#include <errno.h>

#include "../weechat.h"
#include "../wee-hook.h"
#include "../wee-infolist.h"
#include "../wee-log.h"
#include "../../gui/gui-chat.h"


struct pollfd *hook_fd_pollfd = NULL;  /* file descriptors for poll()       */
int hook_fd_pollfd_count = 0;          /* number of file descriptors        */


/*
 * Returns description of hook.
 *
 * Note: result must be freed after use.
 */

char *
hook_fd_get_description (struct t_hook *hook)
{
    char str_desc[512];

    snprintf (str_desc, sizeof (str_desc),
              "%d (flags: 0x%x:%s%s%s)",
              HOOK_FD(hook, fd),
              HOOK_FD(hook, flags),
              (HOOK_FD(hook, flags) & HOOK_FD_FLAG_READ) ? " read" : "",
              (HOOK_FD(hook, flags) & HOOK_FD_FLAG_WRITE) ? " write" : "",
              (HOOK_FD(hook, flags) & HOOK_FD_FLAG_EXCEPTION) ? " exception" : "");

    return strdup (str_desc);
}

/*
 * Searches for a fd hook in list.
 *
 * Returns pointer to hook found, NULL if not found.
 */

struct t_hook *
hook_fd_search (int fd)
{
    struct t_hook *ptr_hook;

    for (ptr_hook = weechat_hooks[HOOK_TYPE_FD]; ptr_hook;
         ptr_hook = ptr_hook->next_hook)
    {
        if (!ptr_hook->deleted && (HOOK_FD(ptr_hook, fd) == fd))
            return ptr_hook;
    }

    /* fd hook not found */
    return NULL;
}

/*
 * Reallocates the "struct pollfd" array for poll().
 */

void
hook_fd_realloc_pollfd ()
{
    struct pollfd *ptr_pollfd;
    int count;

    if (hooks_count[HOOK_TYPE_FD] == hook_fd_pollfd_count)
        return;

    count = hooks_count[HOOK_TYPE_FD];

    if (count == 0)
    {
        if (hook_fd_pollfd)
        {
            free (hook_fd_pollfd);
            hook_fd_pollfd = NULL;
        }
    }
    else
    {
        ptr_pollfd = realloc (hook_fd_pollfd,
                              count * sizeof (struct pollfd));
        if (!ptr_pollfd)
            return;
        hook_fd_pollfd = ptr_pollfd;
    }

    hook_fd_pollfd_count = count;
}

/*
 * Callback called when a fd hook is added in the list of hooks.
 */

void
hook_fd_add_cb (struct t_hook *hook)
{
    /* make C compiler happy */
    (void) hook;

    hook_fd_realloc_pollfd ();
}

/*
 * Callback called when a fd hook is removed from the list of hooks.
 */

void
hook_fd_remove_cb (struct t_hook *hook)
{
    /* make C compiler happy */
    (void) hook;

    hook_fd_realloc_pollfd ();
}

/*
 * Hooks a fd event.
 *
 * Returns pointer to new hook, NULL if error.
 */

struct t_hook *
hook_fd (struct t_weechat_plugin *plugin, int fd, int flag_read,
         int flag_write, int flag_exception,
         t_hook_callback_fd *callback,
         const void *callback_pointer,
         void *callback_data)
{
    struct t_hook *new_hook;
    struct t_hook_fd *new_hook_fd;

    if ((fd < 0) || hook_fd_search (fd) || !callback)
        return NULL;

    new_hook = malloc (sizeof (*new_hook));
    if (!new_hook)
        return NULL;
    new_hook_fd = malloc (sizeof (*new_hook_fd));
    if (!new_hook_fd)
    {
        free (new_hook);
        return NULL;
    }

    hook_init_data (new_hook, plugin, HOOK_TYPE_FD, HOOK_PRIORITY_DEFAULT,
                    callback_pointer, callback_data);

    new_hook->hook_data = new_hook_fd;
    new_hook_fd->callback = callback;
    new_hook_fd->fd = fd;
    new_hook_fd->flags = 0;
    new_hook_fd->error = 0;
    if (flag_read)
        new_hook_fd->flags |= HOOK_FD_FLAG_READ;
    if (flag_write)
        new_hook_fd->flags |= HOOK_FD_FLAG_WRITE;
    if (flag_exception)
        new_hook_fd->flags |= HOOK_FD_FLAG_EXCEPTION;

    hook_add_to_list (new_hook);

    return new_hook;
}

/*
 * Executes fd hooks:
 * - poll() on fie descriptors
 * - call of hook fd callbacks if needed.
 */

void
hook_fd_exec ()
{
    int i, num_fd, timeout, ready, found;
    struct t_hook *ptr_hook, *next_hook;

    if (!weechat_hooks[HOOK_TYPE_FD])
        return;

    /* build an array of "struct pollfd" for poll() */
    num_fd = 0;
    for (ptr_hook = weechat_hooks[HOOK_TYPE_FD]; ptr_hook;
         ptr_hook = ptr_hook->next_hook)
    {
        if (!ptr_hook->deleted)
        {
            /* skip invalid file descriptors */
            if ((fcntl (HOOK_FD(ptr_hook,fd), F_GETFD) == -1)
                && (errno == EBADF))
            {
                if (HOOK_FD(ptr_hook, error) == 0)
                {
                    HOOK_FD(ptr_hook, error) = errno;
                    gui_chat_printf (NULL,
                                     _("%sBad file descriptor (%d) used in "
                                       "hook_fd"),
                                     gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                     HOOK_FD(ptr_hook, fd));
                }
            }
            else
            {
                if (num_fd > hook_fd_pollfd_count)
                    break;

                hook_fd_pollfd[num_fd].fd = HOOK_FD(ptr_hook, fd);
                hook_fd_pollfd[num_fd].events = 0;
                hook_fd_pollfd[num_fd].revents = 0;
                if (HOOK_FD(ptr_hook, flags) & HOOK_FD_FLAG_READ)
                    hook_fd_pollfd[num_fd].events |= POLLIN;
                if (HOOK_FD(ptr_hook, flags) & HOOK_FD_FLAG_WRITE)
                    hook_fd_pollfd[num_fd].events |= POLLOUT;

                num_fd++;
            }
        }
    }

    /* perform the poll() */
    timeout = hook_timer_get_time_to_next ();
    if (hook_process_pending)
        timeout = 0;
    ready = poll (hook_fd_pollfd, num_fd, timeout);
    if (ready <= 0)
        return;

    /* execute callbacks for file descriptors with activity */
    hook_exec_start ();

    ptr_hook = weechat_hooks[HOOK_TYPE_FD];
    while (ptr_hook)
    {
        next_hook = ptr_hook->next_hook;

        if (!ptr_hook->deleted
            && !ptr_hook->running)
        {
            found = 0;
            for (i = 0; i < num_fd; i++)
            {
                if (hook_fd_pollfd[i].fd == HOOK_FD(ptr_hook, fd)
                    && hook_fd_pollfd[i].revents)
                {
                    found = 1;
                    break;
                }
            }
            if (found)
            {
                ptr_hook->running = 1;
                (void) (HOOK_FD(ptr_hook, callback)) (
                    ptr_hook->callback_pointer,
                    ptr_hook->callback_data,
                    HOOK_FD(ptr_hook, fd));
                ptr_hook->running = 0;
            }
        }

        ptr_hook = next_hook;
    }

    hook_exec_end ();
}

/*
 * Frees data in a fd hook.
 */

void
hook_fd_free_data (struct t_hook *hook)
{
    if (!hook || !hook->hook_data)
        return;

    free (hook->hook_data);
    hook->hook_data = NULL;
}

/*
 * Adds fd hook data in the infolist item.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
hook_fd_add_to_infolist (struct t_infolist_item *item,
                         struct t_hook *hook)
{
    if (!item || !hook || !hook->hook_data)
        return 0;

    if (!infolist_new_var_pointer (item, "callback", HOOK_FD(hook, callback)))
        return 0;
    if (!infolist_new_var_integer (item, "fd", HOOK_FD(hook, fd)))
        return 0;
    if (!infolist_new_var_integer (item, "flags", HOOK_FD(hook, flags)))
        return 0;
    if (!infolist_new_var_integer (item, "error", HOOK_FD(hook, error)))
        return 0;

    return 1;
}

/*
 * Prints fd hook data in WeeChat log file (usually for crash dump).
 */

void
hook_fd_print_log (struct t_hook *hook)
{
    if (!hook || !hook->hook_data)
        return;

    log_printf ("  fd data:");
    log_printf ("    callback. . . . . . . : 0x%lx", HOOK_FD(hook, callback));
    log_printf ("    fd. . . . . . . . . . : %d", HOOK_FD(hook, fd));
    log_printf ("    flags . . . . . . . . : %d", HOOK_FD(hook, flags));
    log_printf ("    error . . . . . . . . : %d", HOOK_FD(hook, error));
}
