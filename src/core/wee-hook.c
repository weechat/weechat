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

#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "weechat.h"
#include "wee-hook.h"
#include "wee-log.h"
#include "wee-string.h"
#include "wee-util.h"
#include "../gui/gui-buffer.h"
#include "../gui/gui-color.h"
#include "../plugins/plugin.h"


struct t_hook *weechat_hooks[HOOK_NUM_TYPES];
struct t_hook *last_weechat_hook[HOOK_NUM_TYPES];
int hook_exec_recursion = 0;
int real_delete_pending = 0;


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
        hook->prev_hook->next_hook = hook->next_hook;
        new_hooks = weechat_hooks[hook->type];
    }
    else
        new_hooks = hook->next_hook;
    
    if (hook->next_hook)
        hook->next_hook->prev_hook = hook->prev_hook;
    
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
hook_search_command (char *command)
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
hook_command (struct t_weechat_plugin *plugin, char *command, char *description,
              char *args, char *args_description, char *completion,
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
                   struct t_weechat_plugin *plugin, char *string)
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
            if (ptr_hook->running)
                command_is_running = 1;
            else
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
                ptr_hook->running = 1;
                rc = (int) (HOOK_COMMAND(ptr_hook, callback))
                    (ptr_hook->callback_data, buffer, argc, argv, argv_eol);
                ptr_hook->running = 0;
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
 * hook_timer: hook a timer
 */

struct t_hook *
hook_timer (struct t_weechat_plugin *plugin, long interval, int align_second,
            int max_calls, t_hook_callback_timer *callback,
            void *callback_data)
{
    struct t_hook *new_hook;
    struct t_hook_timer *new_hook_timer;
    struct timezone tz;
    
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
    new_hook_timer->remaining_calls = max_calls;

    tz.tz_minuteswest = 0;
    gettimeofday (&new_hook_timer->last_exec, &tz);
    
    if ((interval >= 1000) && (align_second > 0))
    {
        /* here we should use 0, but with this value timer is sometimes called
           before second has changed, so for displaying time, it may display
           2 times the same second, that's why we use 1000 micro seconds */
        new_hook_timer->last_exec.tv_usec = 1000;
        new_hook_timer->last_exec.tv_sec =
            new_hook_timer->last_exec.tv_sec -
            ((new_hook_timer->last_exec.tv_sec - (tz.tz_minuteswest * 60)) %
             align_second);
    }
    
    new_hook_timer->next_exec.tv_sec = new_hook_timer->last_exec.tv_sec;
    new_hook_timer->next_exec.tv_usec = new_hook_timer->last_exec.tv_usec;
    util_timeval_add (&new_hook_timer->next_exec, interval);
    
    hook_add_to_list (new_hook);
    
    return new_hook;
}

/*
 * hook_timer_time_to_next: get time to next timeout
 *                          return 1 if timeout is set with next timeout
 *                                 0 if there's no timeout
 */

int
hook_timer_time_to_next (struct timeval *tv_timeout)
{
    struct t_hook *ptr_hook;
    int found;
    struct timeval tv_now;
    long diff_usec;
    
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
    
    /* no timeout found */
    if (!found)
        return 0;
    
    gettimeofday (&tv_now, NULL);
    
    /* next timeout is past date! */
    if (util_timeval_cmp (tv_timeout, &tv_now) < 0)
    {
        tv_timeout->tv_sec = 0;
        tv_timeout->tv_usec = 0;
        return 1;
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
    
    return 1;
}

/*
 * hook_timer_exec: execute timer hooks
 */

void
hook_timer_exec ()
{
    struct timeval tv_time;
    struct t_hook *ptr_hook, *next_hook;
    
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
 * hook_print: hook a message printed by WeeChat
 */

struct t_hook *
hook_print (struct t_weechat_plugin *plugin, struct t_gui_buffer *buffer,
            char *tags, char *message, int strip_colors,
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
hook_print_exec (struct t_gui_buffer *buffer, time_t date, int tags_count,
                 char **tags_array, char *prefix, char *message)
{
    struct t_hook *ptr_hook, *next_hook;
    char *prefix_no_color, *message_no_color;
    int tags_match, tag_found, i, j;
    
    if (!message || !message[0])
        return;
    
    prefix_no_color = (prefix) ?
        (char *)gui_color_decode ((unsigned char *)prefix) : NULL;
    
    message_no_color = (char *)gui_color_decode ((unsigned char *)message);
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
                if (tags_array)
                {
                    tags_match = 1;
                    for (i = 0; i < HOOK_PRINT(ptr_hook, tags_count); i++)
                    {
                        /* search for tag in message */
                        tag_found = 0;
                        for (j = 0; j < tags_count; j++)
                        {
                            if (string_strcasecmp (HOOK_PRINT(ptr_hook, tags_array)[i],
                                                   tags_array[j]) != 0)
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
                    (ptr_hook->callback_data, buffer, date,
                     tags_count, tags_array,
                     (HOOK_PRINT(ptr_hook, strip_colors)) ? prefix_no_color : prefix,
                     (HOOK_PRINT(ptr_hook, strip_colors)) ? message_no_color : message);
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
hook_signal (struct t_weechat_plugin *plugin, char *signal,
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
hook_signal_send (char *signal, char *type_data, void *signal_data)
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
hook_config (struct t_weechat_plugin *plugin, char *option,
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
hook_config_exec (char *option, char *value)
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
hook_completion (struct t_weechat_plugin *plugin, char *completion,
                 t_hook_callback_completion *callback, void *callback_data)
{
    struct t_hook *new_hook;
    struct t_hook_completion *new_hook_completion;
    
    if (!completion || !completion[0] || strchr (completion, ' '))
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
    new_hook_completion->completion = strdup (completion);
    
    hook_add_to_list (new_hook);
    
    return new_hook;
}

/*
 * hook_completion_exec: execute completion hook
 */

void
hook_completion_exec (struct t_weechat_plugin *plugin, char *completion,
                      struct t_gui_buffer *buffer, struct t_weelist *list)
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
            && (string_strcasecmp (HOOK_COMPLETION(ptr_hook, completion),
                                   completion) == 0))
        {
            ptr_hook->running = 1;
            (void) (HOOK_COMPLETION(ptr_hook, callback))
                (ptr_hook->callback_data, completion, buffer, list);
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
hook_modifier (struct t_weechat_plugin *plugin, char *modifier,
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
hook_modifier_exec (struct t_weechat_plugin *plugin, char *modifier,
                    char *modifier_data, char *string)
{
    struct t_hook *ptr_hook, *next_hook;
    char *new_msg, *message_modified;
    
    /* make C compiler happy */
    (void) plugin;
    
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
 * unhook: unhook something
 */

void
unhook (struct t_hook *hook)
{
    struct t_hook *ptr_hook;
    
    /* hook already deleted? */
    if (hook->deleted)
        return;
    
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
                if (HOOK_COMPLETION(hook, completion))
                    free (HOOK_COMPLETION(hook, completion));
                free ((struct t_hook_completion *)hook->hook_data);
                break;
            case HOOK_TYPE_MODIFIER:
                if (HOOK_MODIFIER(hook, modifier))
                    free (HOOK_MODIFIER(hook, modifier));
                free ((struct t_hook_modifier *)hook->hook_data);
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
            log_printf ("[hook (addr:0x%x)]", ptr_hook);
            log_printf ("  plugin . . . . . . . . : 0x%x ('%s')",
                        ptr_hook->plugin,
                        (ptr_hook->plugin) ? ptr_hook->plugin->name : "");
            log_printf ("  deleted. . . . . . . . : %d",   ptr_hook->deleted);
            log_printf ("  running. . . . . . . . : %d",   ptr_hook->running);
            switch (ptr_hook->type)
            {
                case HOOK_TYPE_COMMAND:
                    log_printf ("  type . . . . . . . . . : %d (command)", ptr_hook->type);
                    log_printf ("  callback_data. . . . . : 0x%x", ptr_hook->callback_data);
                    if (!ptr_hook->deleted)
                    {
                        log_printf ("  command data:");
                        log_printf ("    callback . . . . . . : 0x%x", HOOK_COMMAND(ptr_hook, callback));
                        log_printf ("    command. . . . . . . : '%s'", HOOK_COMMAND(ptr_hook, command));
                        log_printf ("    level. . . . . . . . : %d",   HOOK_COMMAND(ptr_hook, level));
                        log_printf ("    command_desc . . . . : '%s'", HOOK_COMMAND(ptr_hook, description));
                        log_printf ("    command_args . . . . : '%s'", HOOK_COMMAND(ptr_hook, args));
                        log_printf ("    command_args_desc. . : '%s'", HOOK_COMMAND(ptr_hook, args_description));
                        log_printf ("    command_completion . : '%s'", HOOK_COMMAND(ptr_hook, completion));
                    }
                    break;
                case HOOK_TYPE_TIMER:
                    log_printf ("  type . . . . . . . . . : %d (timer)", ptr_hook->type);
                    log_printf ("  callback_data. . . . . : 0x%x", ptr_hook->callback_data);
                    if (!ptr_hook->deleted)
                    {
                        log_printf ("  timer data:");
                        log_printf ("    callback . . . . . . : 0x%x", HOOK_TIMER(ptr_hook, callback));
                        log_printf ("    interval . . . . . . : %ld",  HOOK_TIMER(ptr_hook, interval));
                        local_time = localtime (&HOOK_TIMER(ptr_hook, last_exec).tv_sec);
                        strftime (text_time, sizeof (text_time),
                                  "%d/%m/%Y %H:%M:%S", local_time);
                        log_printf ("    last_exec.tv_sec . . : %ld (%s)",
                                    HOOK_TIMER(ptr_hook, last_exec.tv_sec),
                                    text_time);
                        log_printf ("    last_exec.tv_usec. . : %ld",  HOOK_TIMER(ptr_hook, last_exec.tv_usec));
                        local_time = localtime (&HOOK_TIMER(ptr_hook, next_exec).tv_sec);
                        strftime (text_time, sizeof (text_time),
                                  "%d/%m/%Y %H:%M:%S", local_time);
                        log_printf ("    next_exec.tv_sec . . : %ld (%s)",
                                    HOOK_TIMER(ptr_hook, next_exec.tv_sec),
                                    text_time);
                        log_printf ("    next_exec.tv_usec. . : %ld",  HOOK_TIMER(ptr_hook, next_exec.tv_usec));
                    }
                    break;
                case HOOK_TYPE_FD:
                    log_printf ("  type . . . . . . . . . : %d (fd)", ptr_hook->type);
                    log_printf ("  callback_data. . . . . : 0x%x", ptr_hook->callback_data);
                    if (!ptr_hook->deleted)
                    {
                        log_printf ("  fd data:");
                        log_printf ("    callback . . . . . . : 0x%x", HOOK_FD(ptr_hook, callback));
                        log_printf ("    fd . . . . . . . . . : %ld",  HOOK_FD(ptr_hook, fd));
                        log_printf ("    flags. . . . . . . . : %ld",  HOOK_FD(ptr_hook, flags));
                    }
                    break;
                case HOOK_TYPE_PRINT:
                    log_printf ("  type . . . . . . . . . : %d (print)", ptr_hook->type);
                    log_printf ("  callback_data. . . . . : 0x%x", ptr_hook->callback_data);
                    if (!ptr_hook->deleted)
                    {
                        log_printf ("  print data:");
                        log_printf ("    callback . . . . . . : 0x%x", HOOK_PRINT(ptr_hook, callback));
                        log_printf ("    buffer . . . . . . . : 0x%x", HOOK_PRINT(ptr_hook, buffer));
                        log_printf ("    message. . . . . . . : '%s'", HOOK_PRINT(ptr_hook, message));
                    }
                    break;
                case HOOK_TYPE_SIGNAL:
                    log_printf ("  type . . . . . . . . . : %d (signal)", ptr_hook->type);
                    log_printf ("  callback_data. . . . . : 0x%x", ptr_hook->callback_data);
                    if (!ptr_hook->deleted)
                    {
                        log_printf ("  signal data:");
                        log_printf ("    callback . . . . . . : 0x%x", HOOK_SIGNAL(ptr_hook, callback));
                        log_printf ("    signal . . . . . . . : '%s'", HOOK_SIGNAL(ptr_hook, signal));
                    }
                    break;
                case HOOK_TYPE_CONFIG:
                    log_printf ("  type . . . . . . . . . : %d (config)", ptr_hook->type);
                    log_printf ("  callback_data. . . . . : 0x%x", ptr_hook->callback_data);
                    if (!ptr_hook->deleted)
                    {
                        log_printf ("  config data:");
                        log_printf ("    callback . . . . . . : 0x%x", HOOK_CONFIG(ptr_hook, callback));
                        log_printf ("    option . . . . . . . : '%s'", HOOK_CONFIG(ptr_hook, option));
                    }
                    break;
                case HOOK_TYPE_COMPLETION:
                    log_printf ("  type . . . . . . . . . : %d (completion)", ptr_hook->type);
                    log_printf ("  callback_data. . . . . : 0x%x", ptr_hook->callback_data);
                    if (!ptr_hook->deleted)
                    {
                        log_printf ("  completion data:");
                        log_printf ("    callback . . . . . . : 0x%x", HOOK_COMPLETION(ptr_hook, callback));
                        log_printf ("    completion . . . . . : '%s'", HOOK_COMPLETION(ptr_hook, completion));
                    }
                    break;
                case HOOK_TYPE_MODIFIER:
                    log_printf ("  type . . . . . . . . . : %d (modifier)", ptr_hook->type);
                    log_printf ("  callback_data. . . . . : 0x%x", ptr_hook->callback_data);
                    if (!ptr_hook->deleted)
                    {
                        log_printf ("  modifier data:");
                        log_printf ("    callback . . . . . . : 0x%x", HOOK_MODIFIER(ptr_hook, callback));
                        log_printf ("    modifier . . . . . . : '%s'", HOOK_MODIFIER(ptr_hook, modifier));
                    }
                    break;
                case HOOK_NUM_TYPES:
                    /* this constant is used to count types only,
                       it is never used as type */
                    break;
            }
            log_printf ("  prev_hook. . . . . . . : 0x%x", ptr_hook->prev_hook);
            log_printf ("  next_hook. . . . . . . : 0x%x", ptr_hook->next_hook);
        }
    }
}
