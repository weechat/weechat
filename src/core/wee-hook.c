/*
 * Copyright (c) 2003-2008 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* wee-hook.c: WeeChat hooks management */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#include "weechat.h"
#include "wee-hook.h"
#include "wee-infolist.h"
#include "wee-log.h"
#include "wee-network.h"
#include "wee-string.h"
#include "wee-util.h"
#include "../gui/gui-buffer.h"
#include "../gui/gui-chat.h"
#include "../gui/gui-color.h"
#include "../gui/gui-completion.h"
#include "../plugins/plugin.h"


char *hook_type_string[HOOK_NUM_TYPES] =
{ "command", "timer", "fd", "connect", "print", "signal", "config",
  "completion", "modifier", "info", "infolist" };
struct t_hook *weechat_hooks[HOOK_NUM_TYPES];     /* list of hooks          */
struct t_hook *last_weechat_hook[HOOK_NUM_TYPES]; /* last hook              */
int hook_exec_recursion = 0;           /* 1 when a hook is executed         */
time_t hook_last_system_time = 0;      /* used to detect system clock skew  */
int real_delete_pending = 0;           /* 1 if some hooks must be deleted   */


/*
 * hook_init: init hooks lists
 */

void
hook_init ()
{
    int type;
    
    for (type = 0; type < HOOK_NUM_TYPES; type++)
    {
        weechat_hooks[type] = NULL;
        last_weechat_hook[type] = NULL;
    }
    hook_last_system_time = time (NULL);
}

/*
 * hook_search_type: search type string and return integer (-1 if not found)
 */

int
hook_search_type (const char *type)
{
    int i;
    
    if (!type)
        return -1;
    
    for (i = 0; i < HOOK_NUM_TYPES; i++)
    {
        if (strcmp (hook_type_string[i], type) == 0)
            return i;
    }
    
    /* type not found */
    return -1;
}

/*
 * hook_find_pos: find position for new hook (keeping command list sorted)
 */

struct t_hook *
hook_find_pos (struct t_hook *hook)
{
    struct t_hook *ptr_hook;
    
    /* if it's not command hook, then add to the end of list */
    if (hook->type != HOOK_TYPE_COMMAND)
        return NULL;
    
    /* for command hook, keep list sorted */
    for (ptr_hook = weechat_hooks[hook->type]; ptr_hook;
         ptr_hook = ptr_hook->next_hook)
    {
        if (!ptr_hook->deleted
            && (string_strcasecmp (HOOK_COMMAND(hook, command),
                                   HOOK_COMMAND(ptr_hook, command)) <= 0))
            return ptr_hook;
    }
    
    /* position not found, best position is at the end */
    return NULL;
}

/*
 * hook_add_to_list: add a hook to list
 */

void
hook_add_to_list (struct t_hook *new_hook)
{
    struct t_hook *pos_hook;

    if (weechat_hooks[new_hook->type])
    {
        pos_hook = hook_find_pos (new_hook);
        if (pos_hook)
        {
            /* add hook somewhere in the list */
            new_hook->prev_hook = pos_hook->prev_hook;
            new_hook->next_hook = pos_hook;
            if (pos_hook->prev_hook)
                (pos_hook->prev_hook)->next_hook = new_hook;
            else
                weechat_hooks[new_hook->type] = new_hook;
            pos_hook->prev_hook = new_hook;
        }
        else
        {
            /* add hook to end of list */
            new_hook->prev_hook = last_weechat_hook[new_hook->type];
            new_hook->next_hook = NULL;
            last_weechat_hook[new_hook->type]->next_hook = new_hook;
            last_weechat_hook[new_hook->type] = new_hook;
        }
    }
    else
    {
        new_hook->prev_hook = NULL;
        new_hook->next_hook = NULL;
        weechat_hooks[new_hook->type] = new_hook;
        last_weechat_hook[new_hook->type] = new_hook;
    }
}

/*
 * hook_remove_from_list: remove a hook from list
 */

void
hook_remove_from_list (struct t_hook *hook)
{
    struct t_hook *new_hooks;
    int type;
    
    type = hook->type;
    
    if (last_weechat_hook[hook->type] == hook)
        last_weechat_hook[hook->type] = hook->prev_hook;
    if (hook->prev_hook)
    {
        (hook->prev_hook)->next_hook = hook->next_hook;
        new_hooks = weechat_hooks[hook->type];
    }
    else
        new_hooks = hook->next_hook;
    
    if (hook->next_hook)
        (hook->next_hook)->prev_hook = hook->prev_hook;
    
    free (hook);
    
    weechat_hooks[type] = new_hooks;
}

/*
 * hook_remove_deleted: remove deleted hooks from list
 */

void
hook_remove_deleted ()
{
    int type;
    struct t_hook *ptr_hook, *next_hook;

    if (real_delete_pending)
    {
        for (type = 0; type < HOOK_NUM_TYPES; type++)
        {
            ptr_hook = weechat_hooks[type];
            while (ptr_hook)
            {
                next_hook = ptr_hook->next_hook;
                
                if (ptr_hook->deleted)
                    hook_remove_from_list (ptr_hook);
                
                ptr_hook = next_hook;
            }
        }
        real_delete_pending = 0;
    }
}

/*
 * hook_init_data: init data a new hook with default values
 */

void
hook_init_data (struct t_hook *hook, struct t_weechat_plugin *plugin,
                int type, void *callback_data)
{
    hook->plugin = plugin;
    hook->type = type;
    hook->deleted = 0;
    hook->running = 0;
    hook->callback_data = callback_data;
    hook->hook_data = NULL;

    if (weechat_debug_core >= 2)
    {
        gui_chat_printf (NULL,
                         "debug: adding hook: type=%d (%s), plugin=%lx (%s)",
                         hook->type, hook_type_string[hook->type],
                         hook->plugin, plugin_get_name (hook->plugin));
    }
}

/*
 * hook_valid: check if a hook pointer exists
 *                   return 1 if hook exists
 *                          0 if hook is not found
 */

int
hook_valid (struct t_hook *hook)
{
    int type;
    struct t_hook *ptr_hook;

    for (type = 0; type < HOOK_NUM_TYPES; type++)
    {
        for (ptr_hook = weechat_hooks[type]; ptr_hook;
             ptr_hook = ptr_hook->next_hook)
        {
            if (!ptr_hook->deleted && (ptr_hook == hook))
                return 1;
        }
    }
    
    /* hook not found */
    return 0;
}

/*
 * hook_valid_for_plugin: check if a hook pointer exists for a plugin
 *                        return 1 if hook exists for plugin
 *                               0 if hook is not found for plugin
 */

int
hook_valid_for_plugin (struct t_weechat_plugin *plugin, struct t_hook *hook)
{
    int type;
    struct t_hook *ptr_hook;

    for (type = 0; type < HOOK_NUM_TYPES; type++)
    {
        for (ptr_hook = weechat_hooks[type]; ptr_hook;
             ptr_hook = ptr_hook->next_hook)
        {
            if (!ptr_hook->deleted && (ptr_hook == hook)
                && (ptr_hook->plugin == plugin))
                return 1;
        }
    }
    
    /* hook not found */
    return 0;
}

/*
 * hook_exec_start: code executed before a hook exec
 */

void
hook_exec_start ()
{
    hook_exec_recursion++;
}

/*
 * hook_exec_end: code executed after a hook exec
 */

void
hook_exec_end ()
{
    if (hook_exec_recursion > 0)
        hook_exec_recursion--;
    
    if (hook_exec_recursion == 0)
        hook_remove_deleted ();
}

/*
 * hook_search_command: search command hook in list
 */

struct t_hook *
hook_search_command (const char *command)
{
    struct t_hook *ptr_hook;
    
    for (ptr_hook = weechat_hooks[HOOK_TYPE_COMMAND]; ptr_hook;
         ptr_hook = ptr_hook->next_hook)
    {
        if (!ptr_hook->deleted
            && (string_strcasecmp (HOOK_COMMAND(ptr_hook, command), command) == 0))
            return ptr_hook;
    }
    
    /* command hook not found */
    return NULL;
}

/*
 * hook_command: hook a command
 */

struct t_hook *
hook_command (struct t_weechat_plugin *plugin, const char *command,
              const char *description,
              const char *args, const char *args_description,
              const char *completion,
              t_hook_callback_command *callback, void *callback_data)
{
    struct t_hook *ptr_hook,*new_hook;
    struct t_hook_command *new_hook_command;

    if ((string_strcasecmp (command, "builtin") == 0)
        && hook_search_command (command))
        return NULL;

    /* increase level for command hooks with same command name
       so that these commands will not be used any more, until this
       one is removed */
    for (ptr_hook = weechat_hooks[HOOK_TYPE_COMMAND]; ptr_hook;
         ptr_hook = ptr_hook->next_hook)
    {
        if (!ptr_hook->deleted
            && (string_strcasecmp (HOOK_COMMAND(ptr_hook, command), command) == 0))
        {
            HOOK_COMMAND(ptr_hook, level)++;
        }
    }
    
    new_hook = malloc (sizeof (*new_hook));
    if (!new_hook)
        return NULL;
    new_hook_command = malloc (sizeof (*new_hook_command));
    if (!new_hook_command)
    {
        free (new_hook);
        return NULL;
    }
    
    hook_init_data (new_hook, plugin, HOOK_TYPE_COMMAND, callback_data);
    
    new_hook->hook_data = new_hook_command;
    new_hook_command->callback = callback;
    new_hook_command->command = (command) ?
        strdup (command) : strdup ("");
    new_hook_command->level = 0;
    new_hook_command->description = (description) ?
        strdup (description) : strdup ("");
    new_hook_command->args = (args) ?
        strdup (args) : strdup ("");
    new_hook_command->args_description = (args_description) ?
        strdup (args_description) : strdup ("");
    new_hook_command->completion = (completion) ?
        strdup (completion) : strdup ("");
    
    hook_add_to_list (new_hook);
    
    return new_hook;
}

/*
 * hook_command_exec: execute command hook
 *                    return:  0 if command executed and failed
 *                             1 if command executed successfully
 *                            -1 if command not found
 *                            -2 if command is ambigous (same command exists
 *                               for another plugin, and we don't know which
 *                               one to run)
 *                            -3 if command is already running
 */

int
hook_command_exec (struct t_gui_buffer *buffer, int any_plugin,
                   struct t_weechat_plugin *plugin, const char *string)
{
    struct t_hook *ptr_hook, *next_hook;
    struct t_hook *hook_for_plugin, *hook_for_other_plugin;
    char **argv, **argv_eol;
    int argc, rc, command_is_running;
    
    rc = -1;
    
    if (!buffer || !string || !string[0])
        return -1;
    
    argv = string_explode (string, " ", 0, 0, &argc);
    if (argc == 0)
    {
        string_free_exploded (argv);
        return -1;
    }
    argv_eol = string_explode (string, " ", 1, 0, NULL);
    
    hook_exec_start ();
    
    hook_for_plugin = NULL;
    hook_for_other_plugin = NULL;
    command_is_running = 0;
    ptr_hook = weechat_hooks[HOOK_TYPE_COMMAND];
    while (ptr_hook)
    {
        next_hook = ptr_hook->next_hook;
        
        if (!ptr_hook->deleted
            && ((!any_plugin || HOOK_COMMAND(ptr_hook, level) == 0))
            && ((argv[0][0] == '/') && (string_strcasecmp (argv[0] + 1,
                                                           HOOK_COMMAND(ptr_hook, command)) == 0)))
        {
            if (ptr_hook->running > 0)
                command_is_running = ptr_hook->running;

            if (ptr_hook->running < HOOK_COMMAND_MAX_CALLS)
            {
                if (ptr_hook->plugin == plugin)
                {
                    if (!hook_for_plugin)
                        hook_for_plugin = ptr_hook;
                }
                else
                {
                    if (!hook_for_other_plugin)
                        hook_for_other_plugin = ptr_hook;
                }
            }
        }
        
        ptr_hook = next_hook;
    }
    
    /* ambiguous: command found for current plugin and other one, we don't know
       which one to run! */
    if (any_plugin && hook_for_plugin && hook_for_other_plugin)
    {
        rc = -2;
    }
    else
    {
        if (any_plugin || hook_for_plugin)
        {
            ptr_hook = (hook_for_plugin) ?
                hook_for_plugin : hook_for_other_plugin;
            
            if (ptr_hook)
            {
                ptr_hook->running++;
                rc = (int) (HOOK_COMMAND(ptr_hook, callback))
                    (ptr_hook->callback_data, buffer, argc, argv, argv_eol);
                ptr_hook->running--;
                if (rc == WEECHAT_RC_ERROR)
                    rc = 0;
                else
                    rc = 1;
            }
            else
            {
                if (command_is_running)
                    rc = -3;
            }
        }
        else
        {
            if (command_is_running)
                rc = -3;
        }
    }
    
    string_free_exploded (argv);
    string_free_exploded (argv_eol);
    
    hook_exec_end ();
    
    return rc;
}

/*
 * hook_timer_init: init a timer hook
 */

void
hook_timer_init (struct t_hook *hook)
{
    time_t time_now;
    struct tm *local_time, *gm_time;
    int local_hour, gm_hour, diff_hour;
    
    gettimeofday (&HOOK_TIMER(hook, last_exec), NULL);
    time_now = time (NULL);
    local_time = localtime(&time_now);
    local_hour = local_time->tm_hour;
    gm_time = gmtime(&time_now);
    gm_hour = gm_time->tm_hour;
    if ((local_time->tm_year > gm_time->tm_year)
        || (local_time->tm_mon > gm_time->tm_mon)
        || (local_time->tm_mday > gm_time->tm_mday))
    {
        diff_hour = (24 - gm_hour) + local_hour;
    }
    else if ((gm_time->tm_year > local_time->tm_year)
             || (gm_time->tm_mon > local_time->tm_mon)
             || (gm_time->tm_mday > local_time->tm_mday))
    {
        diff_hour = (-1) * ((24 - local_hour) + gm_hour);
    }
    else
        diff_hour = local_hour - gm_hour;
    
    if ((HOOK_TIMER(hook, interval) >= 1000)
        && (HOOK_TIMER(hook, align_second) > 0))
    {
        /* here we should use 0, but with this value timer is sometimes called
           before second has changed, so for displaying time, it may display
           2 times the same second, that's why we use 1000 micro seconds */
        HOOK_TIMER(hook, last_exec).tv_usec = 1000;
        HOOK_TIMER(hook, last_exec).tv_sec =
            HOOK_TIMER(hook, last_exec).tv_sec -
            ((HOOK_TIMER(hook, last_exec).tv_sec + (diff_hour * 3600)) %
             HOOK_TIMER(hook, align_second));
    }
    
    /* init next call with date of last call */
    HOOK_TIMER(hook, next_exec).tv_sec = HOOK_TIMER(hook, last_exec).tv_sec;
    HOOK_TIMER(hook, next_exec).tv_usec = HOOK_TIMER(hook, last_exec).tv_usec;
    
    /* add interval to next call date */
    util_timeval_add (&HOOK_TIMER(hook, next_exec), HOOK_TIMER(hook, interval));
}

/*
 * hook_timer: hook a timer
 */

struct t_hook *
hook_timer (struct t_weechat_plugin *plugin, long interval, int align_second,
            int max_calls, t_hook_callback_timer *callback,
            void *callback_data)
{
    struct t_hook *new_hook;
    struct t_hook_timer *new_hook_timer;
    
    if (interval <= 0)
        return NULL;
    
    new_hook = malloc (sizeof (*new_hook));
    if (!new_hook)
        return NULL;
    new_hook_timer = malloc (sizeof (*new_hook_timer));
    if (!new_hook_timer)
    {
        free (new_hook);
        return NULL;
    }
    
    hook_init_data (new_hook, plugin, HOOK_TYPE_TIMER, callback_data);
    
    new_hook->hook_data = new_hook_timer;
    new_hook_timer->callback = callback;
    new_hook_timer->interval = interval;
    new_hook_timer->align_second = align_second;
    new_hook_timer->remaining_calls = max_calls;
    
    hook_timer_init (new_hook);
    
    hook_add_to_list (new_hook);
    
    return new_hook;
}

/*
 * hook_timer_check_system_clock: check if system clock is older than previous
 *                                call to this function (that means new time
 *                                is lower that in past). If yes, then adjust
 *                                all timers to current time
 */

void
hook_timer_check_system_clock ()
{
    time_t now;
    long diff_time;
    struct t_hook *ptr_hook;
    
    now = time (NULL);
    
    /* check if difference with previous time is more than 10 seconds
       If it is, then consider it's clock skew and reinitialize all timers */
    diff_time = now - hook_last_system_time;
    if ((diff_time <= -10) || (diff_time >= 10))
    {
        gui_chat_printf (NULL,
                         _("System clock skew detected (%+ld seconds), "
                           "reinitializing all timers"),
                         diff_time);
        
        /* reinitialize all timers */
        for (ptr_hook = weechat_hooks[HOOK_TYPE_TIMER]; ptr_hook;
             ptr_hook = ptr_hook->next_hook)
        {
            if (!ptr_hook->deleted)
                hook_timer_init (ptr_hook);
        }
    }
    
    hook_last_system_time = now;
}

/*
 * hook_timer_time_to_next: get time to next timeout
 *                          return 1 if timeout is set with next timeout
 *                                 0 if there's no timeout
 */

void
hook_timer_time_to_next (struct timeval *tv_timeout)
{
    struct t_hook *ptr_hook;
    int found;
    struct timeval tv_now;
    long diff_usec;
    
    hook_timer_check_system_clock ();
    
    found = 0;
    tv_timeout->tv_sec = 0;
    tv_timeout->tv_usec = 0;
    
    for (ptr_hook = weechat_hooks[HOOK_TYPE_TIMER]; ptr_hook;
         ptr_hook = ptr_hook->next_hook)
    {
        if (!ptr_hook->deleted
            && (!found
                || (util_timeval_cmp (&HOOK_TIMER(ptr_hook, next_exec), tv_timeout) < 0)))
        {
            found = 1;
            tv_timeout->tv_sec = HOOK_TIMER(ptr_hook, next_exec).tv_sec;
            tv_timeout->tv_usec = HOOK_TIMER(ptr_hook, next_exec).tv_usec;
        }
    }
    
    /* no timeout found, return 2 seconds by default */
    if (!found)
    {
        tv_timeout->tv_sec = 2;
        tv_timeout->tv_usec = 0;
        return;
    }
    
    gettimeofday (&tv_now, NULL);
    
    /* next timeout is past date! */
    if (util_timeval_cmp (tv_timeout, &tv_now) < 0)
    {
        tv_timeout->tv_sec = 0;
        tv_timeout->tv_usec = 0;
        return;
    }
    
    tv_timeout->tv_sec = tv_timeout->tv_sec - tv_now.tv_sec;
    diff_usec = tv_timeout->tv_usec - tv_now.tv_usec;
    if (diff_usec >= 0)
        tv_timeout->tv_usec = diff_usec;
    else
    {
        tv_timeout->tv_sec--;
        tv_timeout->tv_usec = 1000000 + diff_usec;
    }
    
    /* to detect clock skew, we ensure there's a call to timers every
       2 seconds max */
    if (tv_timeout->tv_sec > 2)
    {
        tv_timeout->tv_sec = 2;
        tv_timeout->tv_usec = 0;
    }
}

/*
 * hook_timer_exec: execute timer hooks
 */

void
hook_timer_exec ()
{
    struct timeval tv_time;
    struct t_hook *ptr_hook, *next_hook;
    
    hook_timer_check_system_clock ();
    
    gettimeofday (&tv_time, NULL);
    
    hook_exec_start ();
    
    ptr_hook = weechat_hooks[HOOK_TYPE_TIMER];
    while (ptr_hook)
    {
        next_hook = ptr_hook->next_hook;
        
        if (!ptr_hook->deleted
            && !ptr_hook->running
            && (util_timeval_cmp (&HOOK_TIMER(ptr_hook, next_exec),
                                  &tv_time) <= 0))
        {
            ptr_hook->running = 1;
            (void) (HOOK_TIMER(ptr_hook, callback))
                (ptr_hook->callback_data);
            ptr_hook->running = 0;
            if (!ptr_hook->deleted)
            {
                HOOK_TIMER(ptr_hook, last_exec).tv_sec = tv_time.tv_sec;
                HOOK_TIMER(ptr_hook, last_exec).tv_usec = tv_time.tv_usec;
                
                util_timeval_add (&HOOK_TIMER(ptr_hook, next_exec),
                                  HOOK_TIMER(ptr_hook, interval));
                
                if (HOOK_TIMER(ptr_hook, remaining_calls) > 0)
                {
                    HOOK_TIMER(ptr_hook, remaining_calls)--;
                    if (HOOK_TIMER(ptr_hook, remaining_calls) == 0)
                        unhook (ptr_hook);
                }
            }
        }
        
        ptr_hook = next_hook;
    }
    
    hook_exec_end ();
}

/*
 * hook_search_fd: search fd hook in list
 */

struct t_hook *
hook_search_fd (int fd)
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
 * hook_fd: hook a fd event
 */

struct t_hook *
hook_fd (struct t_weechat_plugin *plugin, int fd, int flag_read,
         int flag_write, int flag_exception,
         t_hook_callback_fd *callback, void *callback_data)
{
    struct t_hook *new_hook;
    struct t_hook_fd *new_hook_fd;
    
    if ((fd < 0) || hook_search_fd (fd))
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
    
    hook_init_data (new_hook, plugin, HOOK_TYPE_FD, callback_data);
    
    new_hook->hook_data = new_hook_fd;
    new_hook_fd->callback = callback;
    new_hook_fd->fd = fd;
    new_hook_fd->flags = 0;
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
 * hook_fd_set: fill sets according to hd hooked
 *              return highest fd set
 */

int
hook_fd_set (fd_set *read_fds, fd_set *write_fds, fd_set *exception_fds)
{
    struct t_hook *ptr_hook;
    int max_fd;

    max_fd = 0;
    for (ptr_hook = weechat_hooks[HOOK_TYPE_FD]; ptr_hook;
         ptr_hook = ptr_hook->next_hook)
    {
        if (!ptr_hook->deleted)
        {
            if (HOOK_FD(ptr_hook, flags) & HOOK_FD_FLAG_READ)
            {
                FD_SET (HOOK_FD(ptr_hook, fd), read_fds);
                if (HOOK_FD(ptr_hook, fd) > max_fd)
                    max_fd = HOOK_FD(ptr_hook, fd);
            }
            if (HOOK_FD(ptr_hook, flags) & HOOK_FD_FLAG_WRITE)
            {
                FD_SET (HOOK_FD(ptr_hook, fd), write_fds);
                if (HOOK_FD(ptr_hook, fd) > max_fd)
                    max_fd = HOOK_FD(ptr_hook, fd);
            }
            if (HOOK_FD(ptr_hook, flags) & HOOK_FD_FLAG_EXCEPTION)
            {
                FD_SET (HOOK_FD(ptr_hook, fd), exception_fds);
                if (HOOK_FD(ptr_hook, fd) > max_fd)
                    max_fd = HOOK_FD(ptr_hook, fd);
            }
        }
    }
    
    return max_fd;
}

/*
 * hook_fd_exec: execute fd callbacks with sets
 */

void
hook_fd_exec (fd_set *read_fds, fd_set *write_fds, fd_set *exception_fds)
{
    struct t_hook *ptr_hook, *next_hook;
    
    hook_exec_start ();
    
    ptr_hook = weechat_hooks[HOOK_TYPE_FD];
    while (ptr_hook)
    {
        next_hook = ptr_hook->next_hook;
        
        if (!ptr_hook->deleted
            && !ptr_hook->running
            && (((HOOK_FD(ptr_hook, flags) & HOOK_FD_FLAG_READ)
                 && (FD_ISSET(HOOK_FD(ptr_hook, fd), read_fds)))
                || ((HOOK_FD(ptr_hook, flags) & HOOK_FD_FLAG_WRITE)
                    && (FD_ISSET(HOOK_FD(ptr_hook, fd), write_fds)))
                || ((HOOK_FD(ptr_hook, flags) & HOOK_FD_FLAG_EXCEPTION)
                    && (FD_ISSET(HOOK_FD(ptr_hook, fd), exception_fds)))))
        {
            ptr_hook->running = 1;
            (void) (HOOK_FD(ptr_hook, callback)) (ptr_hook->callback_data);
            ptr_hook->running = 0;
        }
        
        ptr_hook = next_hook;
    }
    
    hook_exec_end ();
}

/*
 * hook_connect: hook a connection to peer (using fork)
 */

struct t_hook *
hook_connect (struct t_weechat_plugin *plugin, const char *proxy,
              const char *address, int port, int sock, int ipv6,
              void *gnutls_sess, const char *local_hostname,
              t_hook_callback_connect *callback, void *callback_data)
{
    struct t_hook *new_hook;
    struct t_hook_connect *new_hook_connect;
    
#ifndef HAVE_GNUTLS
    /* make C compiler happy */
    (void) gnutls_sess;
#endif
    
    if ((sock < 0) || !address || (port <= 0))
        return NULL;
    
    new_hook = malloc (sizeof (*new_hook));
    if (!new_hook)
        return NULL;
    new_hook_connect = malloc (sizeof (*new_hook_connect));
    if (!new_hook_connect)
    {
        free (new_hook);
        return NULL;
    }
    
    hook_init_data (new_hook, plugin, HOOK_TYPE_CONNECT, callback_data);
    
    new_hook->hook_data = new_hook_connect;
    new_hook_connect->callback = callback;
    new_hook_connect->proxy = (proxy) ? strdup (proxy) : NULL;
    new_hook_connect->address = strdup (address);
    new_hook_connect->port = port;
    new_hook_connect->sock = sock;
    new_hook_connect->ipv6 = ipv6;
#ifdef HAVE_GNUTLS
    new_hook_connect->gnutls_sess = gnutls_sess;
#endif
    new_hook_connect->local_hostname = (local_hostname) ?
        strdup (local_hostname) : NULL;
    new_hook_connect->child_read = -1;
    new_hook_connect->child_write = -1;
    new_hook_connect->child_pid = 0;
    new_hook_connect->hook_fd = NULL;
    
    hook_add_to_list (new_hook);
    
    network_connect_with_fork (new_hook);
    
    return new_hook;
}

/*
 * hook_print: hook a message printed by WeeChat
 */

struct t_hook *
hook_print (struct t_weechat_plugin *plugin, struct t_gui_buffer *buffer,
            const char *tags, const char *message, int strip_colors,
            t_hook_callback_print *callback, void *callback_data)
{
    struct t_hook *new_hook;
    struct t_hook_print *new_hook_print;
    
    new_hook = malloc (sizeof (*new_hook));
    if (!new_hook)
        return NULL;
    new_hook_print = malloc (sizeof (*new_hook_print));
    if (!new_hook_print)
    {
        free (new_hook);
        return NULL;
    }
    
    hook_init_data (new_hook, plugin, HOOK_TYPE_PRINT, callback_data);
    
    new_hook->hook_data = new_hook_print;
    new_hook_print->callback = callback;
    new_hook_print->buffer = buffer;
    if (tags)
    {
        new_hook_print->tags_array = string_explode (tags, ",", 0, 0,
                                                     &new_hook_print->tags_count);
    }
    else
    {
        new_hook_print->tags_count = 0;
        new_hook_print->tags_array = NULL;
    }
    new_hook_print->message = (message) ? strdup (message) : NULL;
    new_hook_print->strip_colors = strip_colors;
    
    hook_add_to_list (new_hook);
    
    return new_hook;
}

/*
 * hook_print_exec: execute print hook
 */

void
hook_print_exec (struct t_gui_buffer *buffer, struct t_gui_line *line)
{
    struct t_hook *ptr_hook, *next_hook;
    char *prefix_no_color, *message_no_color;
    int tags_match, tag_found, i, j;
    
    if (!line->message || !line->message[0])
        return;
    
    prefix_no_color = (line->prefix) ?
        (char *)gui_color_decode ((unsigned char *)line->prefix) : NULL;
    
    message_no_color = (char *)gui_color_decode ((unsigned char *)line->message);
    if (!message_no_color)
    {
        free (prefix_no_color);
        return;
    }
    
    hook_exec_start ();
    
    ptr_hook = weechat_hooks[HOOK_TYPE_PRINT];
    while (ptr_hook)
    {
        next_hook = ptr_hook->next_hook;
        
        if (!ptr_hook->deleted
            && !ptr_hook->running
            && (!HOOK_PRINT(ptr_hook, buffer)
                || (buffer == HOOK_PRINT(ptr_hook, buffer)))
            && (!HOOK_PRINT(ptr_hook, message)
                || string_strcasestr (prefix_no_color, HOOK_PRINT(ptr_hook, message))
                || string_strcasestr (message_no_color, HOOK_PRINT(ptr_hook, message))))
        {
            /* check if tags match */
            if (HOOK_PRINT(ptr_hook, tags_array))
            {
                /* if there are tags in message printed */
                if (line->tags_array)
                {
                    tags_match = 1;
                    for (i = 0; i < HOOK_PRINT(ptr_hook, tags_count); i++)
                    {
                        /* search for tag in message */
                        tag_found = 0;
                        for (j = 0; j < line->tags_count; j++)
                        {
                            if (string_strcasecmp (HOOK_PRINT(ptr_hook, tags_array)[i],
                                                   line->tags_array[j]) != 0)
                            {
                                tag_found = 1;
                                break;
                            }
                        }
                        /* tag was asked by hook but not found in message? */
                        if (!tag_found)
                        {
                            tags_match = 0;
                            break;
                        }
                    }
                }
                else
                    tags_match = 0;
            }
            else
                tags_match = 1;
            
            /* run callback */
            if (tags_match)
            {
                ptr_hook->running = 1;
                (void) (HOOK_PRINT(ptr_hook, callback))
                    (ptr_hook->callback_data, buffer, line->date,
                     line->tags_count, (const char **)line->tags_array,
                     (int)line->displayed, (int)line->highlight,
                     (HOOK_PRINT(ptr_hook, strip_colors)) ? prefix_no_color : line->prefix,
                     (HOOK_PRINT(ptr_hook, strip_colors)) ? message_no_color : line->message);
                ptr_hook->running = 0;
            }
        }
        
        ptr_hook = next_hook;
    }
    
    free (prefix_no_color);
    free (message_no_color);
    
    hook_exec_end ();
}

/*
 * hook_signal: hook a signal
 */

struct t_hook *
hook_signal (struct t_weechat_plugin *plugin, const char *signal,
             t_hook_callback_signal *callback, void *callback_data)
{
    struct t_hook *new_hook;
    struct t_hook_signal *new_hook_signal;

    if (!signal || !signal[0])
        return NULL;
    
    new_hook = malloc (sizeof (*new_hook));
    if (!new_hook)
        return NULL;
    new_hook_signal = malloc (sizeof (*new_hook_signal));
    if (!new_hook_signal)
    {
        free (new_hook);
        return NULL;
    }
    
    hook_init_data (new_hook, plugin, HOOK_TYPE_SIGNAL, callback_data);
    
    new_hook->hook_data = new_hook_signal;
    new_hook_signal->callback = callback;
    new_hook_signal->signal = strdup (signal);
    
    hook_add_to_list (new_hook);
    
    return new_hook;
}

/*
 * hook_signal_send: send a signal
 */

void
hook_signal_send (const char *signal, const char *type_data, void *signal_data)
{
    struct t_hook *ptr_hook, *next_hook;
    
    hook_exec_start ();
    
    ptr_hook = weechat_hooks[HOOK_TYPE_SIGNAL];
    while (ptr_hook)
    {
        next_hook = ptr_hook->next_hook;
        
        if (!ptr_hook->deleted
            && !ptr_hook->running
            && (string_match (signal, HOOK_SIGNAL(ptr_hook, signal), 0)))
        {
            ptr_hook->running = 1;
            (void) (HOOK_SIGNAL(ptr_hook, callback))
                (ptr_hook->callback_data, signal, type_data, signal_data);
            ptr_hook->running = 0;
        }
        
        ptr_hook = next_hook;
    }
    
    hook_exec_end ();
}

/*
 * hook_config: hook a config option
 */

struct t_hook *
hook_config (struct t_weechat_plugin *plugin, const char *option,
             t_hook_callback_config *callback, void *callback_data)
{
    struct t_hook *new_hook;
    struct t_hook_config *new_hook_config;
    
    new_hook = malloc (sizeof (*new_hook));
    if (!new_hook)
        return NULL;
    new_hook_config = malloc (sizeof (*new_hook_config));
    if (!new_hook_config)
    {
        free (new_hook);
        return NULL;
    }
    
    hook_init_data (new_hook, plugin, HOOK_TYPE_CONFIG, callback_data);
    
    new_hook->hook_data = new_hook_config;
    new_hook_config->callback = callback;
    new_hook_config->option = (option) ? strdup (option) : strdup ("");
    
    hook_add_to_list (new_hook);
    
    return new_hook;
}

/*
 * hook_config_exec: execute config hooks
 */

void
hook_config_exec (const char *option, const char *value)
{
    struct t_hook *ptr_hook, *next_hook;
    
    hook_exec_start ();
    
    ptr_hook = weechat_hooks[HOOK_TYPE_CONFIG];
    while (ptr_hook)
    {
        next_hook = ptr_hook->next_hook;
        
        if (!ptr_hook->deleted
            && !ptr_hook->running
            && (!HOOK_CONFIG(ptr_hook, option)
                || (string_match (option, HOOK_CONFIG(ptr_hook, option), 0))))
        {
            ptr_hook->running = 1;
            (void) (HOOK_CONFIG(ptr_hook, callback))
                (ptr_hook->callback_data, option, value);
            ptr_hook->running = 0;
        }
        
        ptr_hook = next_hook;
    }
    
    hook_exec_end ();
}

/*
 * hook_completion: hook a completion
 */

struct t_hook *
hook_completion (struct t_weechat_plugin *plugin, const char *completion_item,
                 t_hook_callback_completion *callback, void *callback_data)
{
    struct t_hook *new_hook;
    struct t_hook_completion *new_hook_completion;
    
    if (!completion_item || !completion_item[0] || strchr (completion_item, ' '))
        return NULL;
    
    new_hook = malloc (sizeof (*new_hook));
    if (!new_hook)
        return NULL;
    new_hook_completion = malloc (sizeof (*new_hook_completion));
    if (!new_hook_completion)
    {
        free (new_hook);
        return NULL;
    }
    
    hook_init_data (new_hook, plugin, HOOK_TYPE_COMPLETION, callback_data);
    
    new_hook->hook_data = new_hook_completion;
    new_hook_completion->callback = callback;
    new_hook_completion->completion_item = strdup (completion_item);
    
    hook_add_to_list (new_hook);
    
    return new_hook;
}

/*
 * hook_completion_list_add: add a word for a completion (called by plugins)
 */

void
hook_completion_list_add (struct t_gui_completion *completion,
                          const char *word, int nick_completion,
                          const char *where)
{
    gui_completion_list_add (completion, word, nick_completion, where);
}

/*
 * hook_completion_exec: execute completion hook
 */

void
hook_completion_exec (struct t_weechat_plugin *plugin,
                      const char *completion_item,
                      struct t_gui_buffer *buffer,
                      struct t_gui_completion *completion)
{
    struct t_hook *ptr_hook, *next_hook;
    
    /* make C compiler happy */
    (void) plugin;
    
    hook_exec_start ();
    
    ptr_hook = weechat_hooks[HOOK_TYPE_COMPLETION];
    while (ptr_hook)
    {
        next_hook = ptr_hook->next_hook;
        
        if (!ptr_hook->deleted
            && !ptr_hook->running
            && (string_strcasecmp (HOOK_COMPLETION(ptr_hook, completion_item),
                                   completion_item) == 0))
        {
            ptr_hook->running = 1;
            (void) (HOOK_COMPLETION(ptr_hook, callback))
                (ptr_hook->callback_data, completion_item, buffer, completion);
            ptr_hook->running = 0;
        }
        
        ptr_hook = next_hook;
    }
    
    hook_exec_end ();
}

/*
 * hook_modifier: hook a modifier
 */

struct t_hook *
hook_modifier (struct t_weechat_plugin *plugin, const char *modifier,
               t_hook_callback_modifier *callback, void *callback_data)
{
    struct t_hook *new_hook;
    struct t_hook_modifier *new_hook_modifier;
    
    if (!modifier || !modifier[0])
        return NULL;
    
    new_hook = malloc (sizeof (*new_hook));
    if (!new_hook)
        return NULL;
    new_hook_modifier = malloc (sizeof (*new_hook_modifier));
    if (!new_hook_modifier)
    {
        free (new_hook);
        return NULL;
    }
    
    hook_init_data (new_hook, plugin, HOOK_TYPE_MODIFIER, callback_data);
    
    new_hook->hook_data = new_hook_modifier;
    new_hook_modifier->callback = callback;
    new_hook_modifier->modifier = strdup (modifier);
    
    hook_add_to_list (new_hook);
    
    return new_hook;
}

/*
 * hook_modifier_exec: execute modifier hook
 */

char *
hook_modifier_exec (struct t_weechat_plugin *plugin, const char *modifier,
                    const char *modifier_data, const char *string)
{
    struct t_hook *ptr_hook, *next_hook;
    char *new_msg, *message_modified;
    
    /* make C compiler happy */
    (void) plugin;
    
    if (!modifier || !modifier[0])
        return NULL;
    
    new_msg = NULL;
    message_modified = strdup (string);
    if (!message_modified)
        return NULL;
    
    hook_exec_start ();
    
    ptr_hook = weechat_hooks[HOOK_TYPE_MODIFIER];
    while (ptr_hook)
    {
        next_hook = ptr_hook->next_hook;
        
        if (!ptr_hook->deleted
            && !ptr_hook->running
            && (string_strcasecmp (HOOK_MODIFIER(ptr_hook, modifier),
                                   modifier) == 0))
        {
            ptr_hook->running = 1;
            new_msg = (HOOK_MODIFIER(ptr_hook, callback))
                (ptr_hook->callback_data, modifier, modifier_data,
                 message_modified);
            ptr_hook->running = 0;
            
            /* empty string returned => message dropped */
            if (new_msg && !new_msg[0])
            {
                free (message_modified);
                hook_exec_end ();
                return new_msg;
            }
            
            /* new message => keep it as base for next modifier */
            if (new_msg)
            {
                free (message_modified);
                message_modified = new_msg;
            }
        }
        
        ptr_hook = next_hook;
    }
    
    hook_exec_end ();
    
    return message_modified;
}

/*
 * hook_info: hook an info
 */

struct t_hook *
hook_info (struct t_weechat_plugin *plugin, const char *info_name,
           const char *description,
           t_hook_callback_info *callback, void *callback_data)
{
    struct t_hook *new_hook;
    struct t_hook_info *new_hook_info;
    
    if (!info_name || !info_name[0])
        return NULL;
    
    new_hook = malloc (sizeof (*new_hook));
    if (!new_hook)
        return NULL;
    new_hook_info = malloc (sizeof (*new_hook_info));
    if (!new_hook_info)
    {
        free (new_hook);
        return NULL;
    }
    
    hook_init_data (new_hook, plugin, HOOK_TYPE_INFO, callback_data);
    
    new_hook->hook_data = new_hook_info;
    new_hook_info->callback = callback;
    new_hook_info->info_name = strdup (info_name);
    new_hook_info->description = (description) ?
        strdup (description) : strdup ("");
    
    hook_add_to_list (new_hook);
    
    return new_hook;
}

/*
 * hook_info_get: get info via info hook
 */

const char *
hook_info_get (struct t_weechat_plugin *plugin, const char *info_name,
               const char *arguments)
{
    struct t_hook *ptr_hook, *next_hook;
    const char *value;
    
    /* make C compiler happy */
    (void) plugin;
    
    if (!info_name || !info_name[0])
        return NULL;
    
    hook_exec_start ();
    
    ptr_hook = weechat_hooks[HOOK_TYPE_INFO];
    while (ptr_hook)
    {
        next_hook = ptr_hook->next_hook;
        
        if (!ptr_hook->deleted
            && !ptr_hook->running
            && (string_strcasecmp (HOOK_INFO(ptr_hook, info_name),
                                   info_name) == 0))
        {
            ptr_hook->running = 1;
            value = (HOOK_INFO(ptr_hook, callback))
                (ptr_hook->callback_data, info_name, arguments);
            ptr_hook->running = 0;
            
            hook_exec_end ();
            return value;
        }
        
        ptr_hook = next_hook;
    }
    
    hook_exec_end ();
    
    /* info not found */
    return NULL;
}

/*
 * hook_infolist: hook an infolist
 */

struct t_hook *
hook_infolist (struct t_weechat_plugin *plugin, const char *infolist_name,
               const char *description,
               t_hook_callback_infolist *callback, void *callback_data)
{
    struct t_hook *new_hook;
    struct t_hook_infolist *new_hook_infolist;
    
    if (!infolist_name || !infolist_name[0])
        return NULL;
    
    new_hook = malloc (sizeof (*new_hook));
    if (!new_hook)
        return NULL;
    new_hook_infolist = malloc (sizeof (*new_hook_infolist));
    if (!new_hook_infolist)
    {
        free (new_hook);
        return NULL;
    }
    
    hook_init_data (new_hook, plugin, HOOK_TYPE_INFOLIST, callback_data);
    
    new_hook->hook_data = new_hook_infolist;
    new_hook_infolist->callback = callback;
    new_hook_infolist->infolist_name = strdup (infolist_name);
    new_hook_infolist->description = (description) ?
        strdup (description) : strdup ("");
    
    hook_add_to_list (new_hook);
    
    return new_hook;
}

/*
 * hook_infolist_get: get info via info hook
 */

struct t_infolist *
hook_infolist_get (struct t_weechat_plugin *plugin, const char *infolist_name,
                   void *pointer, const char *arguments)
{
    struct t_hook *ptr_hook, *next_hook;
    struct t_infolist *value;
    
    /* make C compiler happy */
    (void) plugin;
    
    if (!infolist_name || !infolist_name[0])
        return NULL;
    
    hook_exec_start ();
    
    ptr_hook = weechat_hooks[HOOK_TYPE_INFOLIST];
    while (ptr_hook)
    {
        next_hook = ptr_hook->next_hook;
        
        if (!ptr_hook->deleted
            && !ptr_hook->running
            && (string_strcasecmp (HOOK_INFOLIST(ptr_hook, infolist_name),
                                   infolist_name) == 0))
        {
            ptr_hook->running = 1;
            value = (HOOK_INFOLIST(ptr_hook, callback))
                (ptr_hook->callback_data, infolist_name, pointer, arguments);
            ptr_hook->running = 0;
            
            hook_exec_end ();
            return value;
        }
        
        ptr_hook = next_hook;
    }
    
    hook_exec_end ();
    
    /* infolist not found */
    return NULL;
}

/*
 * unhook: unhook something
 */

void
unhook (struct t_hook *hook)
{
    struct t_hook *ptr_hook;
    
    /* invalid hook? */
    if (!hook_valid (hook))
        return;
    
    /* hook already deleted? */
    if (hook->deleted)
        return;

    if (weechat_debug_core >= 2)
    {
        gui_chat_printf (NULL,
                         "debug: removing hook: type=%d (%s), plugin=%lx (%s)",
                         hook->type, hook_type_string[hook->type],
                         hook->plugin, plugin_get_name (hook->plugin));
    }
    
    /* free data */
    if (hook->hook_data)
    {
        switch (hook->type)
        {
            case HOOK_TYPE_COMMAND:
                /* decrease level for command hooks with same command name
                   and level higher than this one */
                for (ptr_hook = weechat_hooks[HOOK_TYPE_COMMAND]; ptr_hook;
                     ptr_hook = ptr_hook->next_hook)
                {
                    if (!ptr_hook->deleted
                        && (ptr_hook != hook)
                        && (string_strcasecmp (HOOK_COMMAND(ptr_hook, command),
                                               HOOK_COMMAND(hook, command)) == 0)
                        && (HOOK_COMMAND(ptr_hook, level) > HOOK_COMMAND(hook, level)))
                    {
                        HOOK_COMMAND(ptr_hook, level)--;
                    }
                }
                if (HOOK_COMMAND(hook, command))
                    free (HOOK_COMMAND(hook, command));
                if (HOOK_COMMAND(hook, description))
                    free (HOOK_COMMAND(hook, description));
                if (HOOK_COMMAND(hook, args))
                    free (HOOK_COMMAND(hook, args));
                if (HOOK_COMMAND(hook, args_description))
                    free (HOOK_COMMAND(hook, args_description));
                if (HOOK_COMMAND(hook, completion))
                    free (HOOK_COMMAND(hook, completion));
                free ((struct t_hook_command *)hook->hook_data);
                break;
            case HOOK_TYPE_TIMER:
                free ((struct t_hook_timer *)hook->hook_data);
                break;
            case HOOK_TYPE_FD:
                free ((struct t_hook_fd *)hook->hook_data);
                break;
            case HOOK_TYPE_CONNECT:
                if (HOOK_CONNECT(hook, address))
                    free (HOOK_CONNECT(hook, address));
                if (HOOK_CONNECT(hook, hook_fd))
                    unhook (HOOK_CONNECT(hook, hook_fd));
                if (HOOK_CONNECT(hook, child_pid) > 0)
                {
                    kill (HOOK_CONNECT(hook, child_pid), SIGKILL);
                    waitpid (HOOK_CONNECT(hook, child_pid), NULL, 0);
                }
                if (HOOK_CONNECT(hook, child_read) != -1)
                    close (HOOK_CONNECT(hook, child_read));
                if (HOOK_CONNECT(hook, child_write) != -1)
                    close (HOOK_CONNECT(hook, child_write));
                free ((struct t_hook_connect *)hook->hook_data);
                break;
            case HOOK_TYPE_PRINT:
                if (HOOK_PRINT(hook, message))
                    free (HOOK_PRINT(hook, message));
                free ((struct t_hook_print *)hook->hook_data);
                break;
            case HOOK_TYPE_SIGNAL:
                if (HOOK_SIGNAL(hook, signal))
                    free (HOOK_SIGNAL(hook, signal));
                free ((struct t_hook_signal *)hook->hook_data);
                break;
            case HOOK_TYPE_CONFIG:
                if (HOOK_CONFIG(hook, option))
                    free (HOOK_CONFIG(hook, option));
                free ((struct t_hook_config *)hook->hook_data);
                break;
            case HOOK_TYPE_COMPLETION:
                if (HOOK_COMPLETION(hook, completion_item))
                    free (HOOK_COMPLETION(hook, completion_item));
                free ((struct t_hook_completion *)hook->hook_data);
                break;
            case HOOK_TYPE_MODIFIER:
                if (HOOK_MODIFIER(hook, modifier))
                    free (HOOK_MODIFIER(hook, modifier));
                free ((struct t_hook_modifier *)hook->hook_data);
                break;
            case HOOK_TYPE_INFO:
                if (HOOK_INFO(hook, info_name))
                    free (HOOK_INFO(hook, info_name));
                if (HOOK_INFO(hook, description))
                    free (HOOK_INFO(hook, description));
                free ((struct t_hook_info *)hook->hook_data);
                break;
            case HOOK_TYPE_INFOLIST:
                if (HOOK_INFOLIST(hook, infolist_name))
                    free (HOOK_INFOLIST(hook, infolist_name));
                if (HOOK_INFOLIST(hook, description))
                    free (HOOK_INFOLIST(hook, description));
                free ((struct t_hook_infolist *)hook->hook_data);
                break;
            case HOOK_NUM_TYPES:
                /* this constant is used to count types only,
                   it is never used as type */
                break;
        }
        hook->hook_data = NULL;
    }           
    
    /* remove hook from list (if there's no hook exec pending) */
    if (hook_exec_recursion == 0)
    {
        hook_remove_from_list (hook);
    }
    else
    {
        /* there is one or more hook exec, then delete later */
        hook->deleted = 1;
        real_delete_pending = 1;
    }
}

/*
 * unhook_all_plugin: unhook all for a plugin
 */

void
unhook_all_plugin (struct t_weechat_plugin *plugin)
{
    int type;
    struct t_hook *ptr_hook, *next_hook;
    
    for (type = 0; type < HOOK_NUM_TYPES; type++)
    {
        ptr_hook = weechat_hooks[type];
        while (ptr_hook)
        {
            next_hook = ptr_hook->next_hook;
            if (ptr_hook->plugin == plugin)
                unhook (ptr_hook);
            ptr_hook = next_hook;
        }
    }
}

/*
 * unhook_all: unhook all
 */

void
unhook_all ()
{
    int type;
    struct t_hook *ptr_hook, *next_hook;
    
    for (type = 0; type < HOOK_NUM_TYPES; type++)
    {
        ptr_hook = weechat_hooks[type];
        while (ptr_hook)
        {
            next_hook = ptr_hook->next_hook;
            unhook (ptr_hook);
            ptr_hook = next_hook;
        }
    }
}

/*
 * hook_add_to_infolist_type: add hooks of a type in an infolist
 *                            return 1 if ok, 0 if error
 */

int
hook_add_to_infolist_type (struct t_infolist *infolist,
                           int type)
{
    struct t_hook *ptr_hook;
    struct t_infolist_item *ptr_item;
    char value[64];

    for (ptr_hook = weechat_hooks[type]; ptr_hook;
         ptr_hook = ptr_hook->next_hook)
    {
        ptr_item = infolist_new_item (infolist);
        if (!ptr_item)
            return 0;
        
        if (!infolist_new_var_pointer (ptr_item, "pointer", ptr_hook))
            return 0;
        if (!infolist_new_var_pointer (ptr_item, "plugin", ptr_hook->plugin))
            return 0;
        if (!infolist_new_var_string (ptr_item, "plugin_name",
                                      (ptr_hook->plugin) ?
                                      ptr_hook->plugin->name : NULL))
            return 0;
        if (!infolist_new_var_string (ptr_item, "type", hook_type_string[ptr_hook->type]))
            return 0;
        if (!infolist_new_var_integer (ptr_item, "deleted", ptr_hook->deleted))
            return 0;
        if (!infolist_new_var_integer (ptr_item, "running", ptr_hook->running))
            return 0;
        switch (ptr_hook->type)
        {
            case HOOK_TYPE_COMMAND:
                if (!ptr_hook->deleted)
                {
                    if (!infolist_new_var_pointer (ptr_item, "callback", HOOK_COMMAND(ptr_hook, callback)))
                        return 0;
                    if (!infolist_new_var_string (ptr_item, "command", HOOK_COMMAND(ptr_hook, command)))
                        return 0;
                    if (!infolist_new_var_integer (ptr_item, "level", HOOK_COMMAND(ptr_hook, level)))
                        return 0;
                    if (!infolist_new_var_string (ptr_item, "description",
                                                  HOOK_COMMAND(ptr_hook, description)))
                        return 0;
                    if (!infolist_new_var_string (ptr_item, "description_nls",
                                                  (HOOK_COMMAND(ptr_hook, description)
                                                   && HOOK_COMMAND(ptr_hook, description)[0]) ?
                                                  _(HOOK_COMMAND(ptr_hook, description)) : ""))
                        return 0;
                    if (!infolist_new_var_string (ptr_item, "args",
                                                  HOOK_COMMAND(ptr_hook, args)))
                        return 0;
                    if (!infolist_new_var_string (ptr_item, "args_nls",
                                                  (HOOK_COMMAND(ptr_hook, args)
                                                   && HOOK_COMMAND(ptr_hook, args)[0]) ?
                                                  _(HOOK_COMMAND(ptr_hook, args)) : ""))
                        return 0;
                    if (!infolist_new_var_string (ptr_item, "args_description",
                                                  HOOK_COMMAND(ptr_hook, args_description)))
                        return 0;
                    if (!infolist_new_var_string (ptr_item, "args_description_nls",
                                                  (HOOK_COMMAND(ptr_hook, args_description)
                                                   && HOOK_COMMAND(ptr_hook, args_description)[0]) ?
                                                  _(HOOK_COMMAND(ptr_hook, args_description)) : ""))
                        return 0;
                    if (!infolist_new_var_string (ptr_item, "completion", HOOK_COMMAND(ptr_hook, completion)))
                        return 0;
                }
                break;
            case HOOK_TYPE_TIMER:
                if (!ptr_hook->deleted)
                {
                    if (!infolist_new_var_pointer (ptr_item, "callback", HOOK_TIMER(ptr_hook, callback)))
                        return 0;
                    snprintf (value, sizeof (value), "%ld", HOOK_TIMER(ptr_hook, interval));
                    if (!infolist_new_var_string (ptr_item, "interval", value))
                        return 0;
                    if (!infolist_new_var_integer (ptr_item, "align_second", HOOK_TIMER(ptr_hook, align_second)))
                        return 0;
                    if (!infolist_new_var_integer (ptr_item, "remaining_calls", HOOK_TIMER(ptr_hook, remaining_calls)))
                        return 0;
                    if (!infolist_new_var_buffer (ptr_item, "last_exec",
                                                  &(HOOK_TIMER(ptr_hook, last_exec)),
                                                  sizeof (HOOK_TIMER(ptr_hook, last_exec))))
                        return 0;
                    if (!infolist_new_var_buffer (ptr_item, "next_exec",
                                                  &(HOOK_TIMER(ptr_hook, next_exec)),
                                                  sizeof (HOOK_TIMER(ptr_hook, next_exec))))
                        return 0;
                }
                break;
            case HOOK_TYPE_FD:
                if (!ptr_hook->deleted)
                {
                    if (!infolist_new_var_pointer (ptr_item, "callback", HOOK_FD(ptr_hook, callback)))
                        return 0;
                    if (!infolist_new_var_integer (ptr_item, "fd", HOOK_FD(ptr_hook, fd)))
                        return 0;
                    if (!infolist_new_var_integer (ptr_item, "flags", HOOK_FD(ptr_hook, flags)))
                        return 0;
                }
                break;
            case HOOK_TYPE_CONNECT:
                if (!ptr_hook->deleted)
                {
                    if (!infolist_new_var_pointer (ptr_item, "callback", HOOK_CONNECT(ptr_hook, callback)))
                        return 0;
                    if (!infolist_new_var_string (ptr_item, "address", HOOK_CONNECT(ptr_hook, address)))
                        return 0;
                    if (!infolist_new_var_integer (ptr_item, "port", HOOK_CONNECT(ptr_hook, port)))
                        return 0;
                    if (!infolist_new_var_integer (ptr_item, "sock", HOOK_CONNECT(ptr_hook, sock)))
                        return 0;
                    if (!infolist_new_var_integer (ptr_item, "ipv6", HOOK_CONNECT(ptr_hook, ipv6)))
                        return 0;
#ifdef HAVE_GNUTLS
                    if (!infolist_new_var_pointer (ptr_item, "gnutls_sess", HOOK_CONNECT(ptr_hook, gnutls_sess)))
                        return 0;
#endif
                    if (!infolist_new_var_string (ptr_item, "local_hostname", HOOK_CONNECT(ptr_hook, local_hostname)))
                        return 0;
                    if (!infolist_new_var_integer (ptr_item, "child_read", HOOK_CONNECT(ptr_hook, child_read)))
                        return 0;
                    if (!infolist_new_var_integer (ptr_item, "child_write", HOOK_CONNECT(ptr_hook, child_write)))
                        return 0;
                    if (!infolist_new_var_integer (ptr_item, "child_pid", HOOK_CONNECT(ptr_hook, child_pid)))
                        return 0;
                    if (!infolist_new_var_pointer (ptr_item, "hook_fd", HOOK_CONNECT(ptr_hook, hook_fd)))
                        return 0;
                }
                break;
            case HOOK_TYPE_PRINT:
                if (!ptr_hook->deleted)
                {
                    if (!infolist_new_var_pointer (ptr_item, "callback", HOOK_PRINT(ptr_hook, callback)))
                        return 0;
                    if (!infolist_new_var_pointer (ptr_item, "buffer", HOOK_PRINT(ptr_hook, buffer)))
                        return 0;
                    if (!infolist_new_var_integer (ptr_item, "tags_count", HOOK_PRINT(ptr_hook, tags_count)))
                        return 0;
                    if (!infolist_new_var_pointer (ptr_item, "tags_array", HOOK_PRINT(ptr_hook, tags_array)))
                        return 0;
                    if (!infolist_new_var_string (ptr_item, "message", HOOK_PRINT(ptr_hook, message)))
                        return 0;
                    if (!infolist_new_var_integer (ptr_item, "strip_colors", HOOK_PRINT(ptr_hook, strip_colors)))
                        return 0;
                }
                break;
            case HOOK_TYPE_SIGNAL:
                if (!ptr_hook->deleted)
                {
                    if (!infolist_new_var_pointer (ptr_item, "callback", HOOK_SIGNAL(ptr_hook, callback)))
                        return 0;
                    if (!infolist_new_var_string (ptr_item, "signal", HOOK_SIGNAL(ptr_hook, signal)))
                        return 0;
                }
                break;
            case HOOK_TYPE_CONFIG:
                if (!ptr_hook->deleted)
                {
                    if (!infolist_new_var_pointer (ptr_item, "callback", HOOK_CONFIG(ptr_hook, callback)))
                        return 0;
                    if (!infolist_new_var_string (ptr_item, "option", HOOK_CONFIG(ptr_hook, option)))
                        return 0;
                }
                break;
            case HOOK_TYPE_COMPLETION:
                if (!ptr_hook->deleted)
                {
                    if (!infolist_new_var_pointer (ptr_item, "callback", HOOK_COMPLETION(ptr_hook, callback)))
                        return 0;
                    if (!infolist_new_var_string (ptr_item, "completion_item", HOOK_COMPLETION(ptr_hook, completion_item)))
                        return 0;
                }
                break;
            case HOOK_TYPE_MODIFIER:
                if (!ptr_hook->deleted)
                {
                    if (!infolist_new_var_pointer (ptr_item, "callback", HOOK_MODIFIER(ptr_hook, callback)))
                        return 0;
                    if (!infolist_new_var_string (ptr_item, "modifier", HOOK_MODIFIER(ptr_hook, modifier)))
                        return 0;
                }
                break;
            case HOOK_TYPE_INFO:
                if (!ptr_hook->deleted)
                {
                    if (!infolist_new_var_pointer (ptr_item, "callback", HOOK_INFO(ptr_hook, callback)))
                        return 0;
                    if (!infolist_new_var_string (ptr_item, "info_name", HOOK_INFO(ptr_hook, info_name)))
                        return 0;
                    if (!infolist_new_var_string (ptr_item, "description", HOOK_INFO(ptr_hook, description)))
                        return 0;
                    if (!infolist_new_var_string (ptr_item, "description_nls",
                                                  (HOOK_INFO(ptr_hook, description)
                                                   && HOOK_INFO(ptr_hook, description)[0]) ?
                                                  _(HOOK_INFO(ptr_hook, description)) : ""))
                        return 0;
                }
                break;
            case HOOK_TYPE_INFOLIST:
                if (!ptr_hook->deleted)
                {
                    if (!infolist_new_var_pointer (ptr_item, "callback", HOOK_INFOLIST(ptr_hook, callback)))
                        return 0;
                    if (!infolist_new_var_string (ptr_item, "infolist_name", HOOK_INFOLIST(ptr_hook, infolist_name)))
                        return 0;
                    if (!infolist_new_var_string (ptr_item, "description", HOOK_INFOLIST(ptr_hook, description)))
                        return 0;
                    if (!infolist_new_var_string (ptr_item, "description_nls",
                                                  (HOOK_INFOLIST(ptr_hook, description)
                                                   && HOOK_INFOLIST(ptr_hook, description)[0]) ?
                                                  _(HOOK_INFOLIST(ptr_hook, description)) : ""))
                        return 0;
                }
                break;
            case HOOK_NUM_TYPES:
                /* this constant is used to count types only,
                   it is never used as type */
                break;
        }
    }
    
    return 1;
}

/*
 * hook_add_to_infolist: add hooks in an infolist
 *                       if type == NULL or is not found, all types are returned
 *                       return 1 if ok, 0 if error
 */

int
hook_add_to_infolist (struct t_infolist *infolist,
                      const char *type)
{
    int i, type_int;
    
    if (!infolist)
        return 0;
    
    type_int = (type) ? hook_search_type (type) : -1;
    
    for (i = 0; i < HOOK_NUM_TYPES; i++)
    {
        if ((type_int < 0) || (type_int == i))
            hook_add_to_infolist_type (infolist, i);
    }
    
    return 1;
}

/*
 * hook_print_log: print hooks in log (usually for crash dump)
 */

void
hook_print_log ()
{
    int type;
    struct t_hook *ptr_hook;
    struct tm *local_time;
    char text_time[1024];
    
    for (type = 0; type < HOOK_NUM_TYPES; type++)
    {
        for (ptr_hook = weechat_hooks[type]; ptr_hook;
             ptr_hook = ptr_hook->next_hook)
        {
            log_printf ("");
            log_printf ("[hook (addr:0x%lx)]", ptr_hook);
            log_printf ("  plugin . . . . . . . . : 0x%lx ('%s')",
                        ptr_hook->plugin, plugin_get_name (ptr_hook->plugin));
            log_printf ("  deleted. . . . . . . . : %d",    ptr_hook->deleted);
            log_printf ("  running. . . . . . . . : %d",    ptr_hook->running);
            log_printf ("  type . . . . . . . . . : %d (%s)",
                        ptr_hook->type, hook_type_string[ptr_hook->type]);
            log_printf ("  callback_data. . . . . : 0x%lx", ptr_hook->callback_data);
            switch (ptr_hook->type)
            {
                case HOOK_TYPE_COMMAND:
                    if (!ptr_hook->deleted)
                    {
                        log_printf ("  command data:");
                        log_printf ("    callback . . . . . . : 0x%lx", HOOK_COMMAND(ptr_hook, callback));
                        log_printf ("    command. . . . . . . : '%s'",  HOOK_COMMAND(ptr_hook, command));
                        log_printf ("    level. . . . . . . . : %d",    HOOK_COMMAND(ptr_hook, level));
                        log_printf ("    description. . . . . : '%s'",  HOOK_COMMAND(ptr_hook, description));
                        log_printf ("    args . . . . . . . . : '%s'",  HOOK_COMMAND(ptr_hook, args));
                        log_printf ("    args_description . . : '%s'",  HOOK_COMMAND(ptr_hook, args_description));
                        log_printf ("    completion . . . . . : '%s'",  HOOK_COMMAND(ptr_hook, completion));
                    }
                    break;
                case HOOK_TYPE_TIMER:
                    if (!ptr_hook->deleted)
                    {
                        log_printf ("  timer data:");
                        log_printf ("    callback . . . . . . : 0x%lx", HOOK_TIMER(ptr_hook, callback));
                        log_printf ("    interval . . . . . . : %ld",   HOOK_TIMER(ptr_hook, interval));
                        log_printf ("    align_second . . . . : %d",    HOOK_TIMER(ptr_hook, align_second));
                        log_printf ("    remaining_calls. . . : %d",    HOOK_TIMER(ptr_hook, remaining_calls));
                        local_time = localtime (&HOOK_TIMER(ptr_hook, last_exec).tv_sec);
                        strftime (text_time, sizeof (text_time),
                                  "%d/%m/%Y %H:%M:%S", local_time);
                        log_printf ("    last_exec.tv_sec . . : %ld (%s)",
                                    HOOK_TIMER(ptr_hook, last_exec.tv_sec),
                                    text_time);
                        log_printf ("    last_exec.tv_usec. . : %ld",   HOOK_TIMER(ptr_hook, last_exec.tv_usec));
                        local_time = localtime (&HOOK_TIMER(ptr_hook, next_exec).tv_sec);
                        strftime (text_time, sizeof (text_time),
                                  "%d/%m/%Y %H:%M:%S", local_time);
                        log_printf ("    next_exec.tv_sec . . : %ld (%s)",
                                    HOOK_TIMER(ptr_hook, next_exec.tv_sec),
                                    text_time);
                        log_printf ("    next_exec.tv_usec. . : %ld",   HOOK_TIMER(ptr_hook, next_exec.tv_usec));
                    }
                    break;
                case HOOK_TYPE_FD:
                    if (!ptr_hook->deleted)
                    {
                        log_printf ("  fd data:");
                        log_printf ("    callback . . . . . . : 0x%lx", HOOK_FD(ptr_hook, callback));
                        log_printf ("    fd . . . . . . . . . : %d",    HOOK_FD(ptr_hook, fd));
                        log_printf ("    flags. . . . . . . . : %d",    HOOK_FD(ptr_hook, flags));
                    }
                    break;
                case HOOK_TYPE_CONNECT:
                    if (!ptr_hook->deleted)
                    {
                        log_printf ("  connect data:");
                        log_printf ("    callback . . . . . . : 0x%lx", HOOK_CONNECT(ptr_hook, callback));
                        log_printf ("    address. . . . . . . : '%s'",  HOOK_CONNECT(ptr_hook, address));
                        log_printf ("    port . . . . . . . . : %d",    HOOK_CONNECT(ptr_hook, port));
                        log_printf ("    sock . . . . . . . . : %d",    HOOK_CONNECT(ptr_hook, sock));
                        log_printf ("    ipv6 . . . . . . . . : %d",    HOOK_CONNECT(ptr_hook, ipv6));
#ifdef HAVE_GNUTLS
                        log_printf ("    gnutls_sess. . . . . : 0x%lx", HOOK_CONNECT(ptr_hook, gnutls_sess));
#endif
                        log_printf ("    local_hostname . . . : '%s'",  HOOK_CONNECT(ptr_hook, local_hostname));
                        log_printf ("    child_read . . . . . : %d",    HOOK_CONNECT(ptr_hook, child_read));
                        log_printf ("    child_write. . . . . : %d",    HOOK_CONNECT(ptr_hook, child_write));
                        log_printf ("    hook_fd. . . . . . . : 0x%lx", HOOK_CONNECT(ptr_hook, hook_fd));
                    }
                    break;
                case HOOK_TYPE_PRINT:
                    if (!ptr_hook->deleted)
                    {
                        log_printf ("  print data:");
                        log_printf ("    callback . . . . . . : 0x%lx", HOOK_PRINT(ptr_hook, callback));
                        log_printf ("    buffer . . . . . . . : 0x%lx", HOOK_PRINT(ptr_hook, buffer));
                        log_printf ("    tags_count . . . . . : %d",    HOOK_PRINT(ptr_hook, tags_count));
                        log_printf ("    tags_array . . . . . : 0x%lx", HOOK_PRINT(ptr_hook, tags_array));
                        log_printf ("    message. . . . . . . : '%s'",  HOOK_PRINT(ptr_hook, message));
                        log_printf ("    strip_colors . . . . : %d",    HOOK_PRINT(ptr_hook, strip_colors));
                    }
                    break;
                case HOOK_TYPE_SIGNAL:
                    if (!ptr_hook->deleted)
                    {
                        log_printf ("  signal data:");
                        log_printf ("    callback . . . . . . : 0x%lx", HOOK_SIGNAL(ptr_hook, callback));
                        log_printf ("    signal . . . . . . . : '%s'",  HOOK_SIGNAL(ptr_hook, signal));
                    }
                    break;
                case HOOK_TYPE_CONFIG:
                    if (!ptr_hook->deleted)
                    {
                        log_printf ("  config data:");
                        log_printf ("    callback . . . . . . : 0x%lx", HOOK_CONFIG(ptr_hook, callback));
                        log_printf ("    option . . . . . . . : '%s'",  HOOK_CONFIG(ptr_hook, option));
                    }
                    break;
                case HOOK_TYPE_COMPLETION:
                    if (!ptr_hook->deleted)
                    {
                        log_printf ("  completion data:");
                        log_printf ("    callback . . . . . . : 0x%lx", HOOK_COMPLETION(ptr_hook, callback));
                        log_printf ("    completion_item. . . : '%s'",  HOOK_COMPLETION(ptr_hook, completion_item));
                    }
                    break;
                case HOOK_TYPE_MODIFIER:
                    if (!ptr_hook->deleted)
                    {
                        log_printf ("  modifier data:");
                        log_printf ("    callback . . . . . . : 0x%lx", HOOK_MODIFIER(ptr_hook, callback));
                        log_printf ("    modifier . . . . . . : '%s'",  HOOK_MODIFIER(ptr_hook, modifier));
                    }
                    break;
                case HOOK_TYPE_INFO:
                    if (!ptr_hook->deleted)
                    {
                        log_printf ("  info data:");
                        log_printf ("    callback . . . . . . : 0x%lx", HOOK_INFO(ptr_hook, callback));
                        log_printf ("    info_name. . . . . . : '%s'",  HOOK_INFO(ptr_hook, info_name));
                        log_printf ("    description. . . . . : '%s'",  HOOK_INFO(ptr_hook, description));
                    }
                    break;
                case HOOK_TYPE_INFOLIST:
                    if (!ptr_hook->deleted)
                    {
                        log_printf ("  infolist data:");
                        log_printf ("    callback . . . . . . : 0x%lx", HOOK_INFOLIST(ptr_hook, callback));
                        log_printf ("    infolist_name. . . . : '%s'",  HOOK_INFOLIST(ptr_hook, infolist_name));
                        log_printf ("    description. . . . . : '%s'",  HOOK_INFOLIST(ptr_hook, description));
                    }
                    break;
                case HOOK_NUM_TYPES:
                    /* this constant is used to count types only,
                       it is never used as type */
                    break;
            }
            log_printf ("  prev_hook. . . . . . . : 0x%lx", ptr_hook->prev_hook);
            log_printf ("  next_hook. . . . . . . : 0x%lx", ptr_hook->next_hook);
        }
    }
}
