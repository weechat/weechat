/*
 * Copyright (c) 2003-2007 by FlashCode <flashcode@flashtux.org>
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
#include "wee-command.h"
#include "wee-log.h"
#include "wee-string.h"
#include "wee-util.h"
#include "../plugins/plugin.h"


struct t_hook *weechat_hooks = NULL;
struct t_hook *last_weechat_hook = NULL;


/*
 * hook_add_to_list: add a hook to list
 */

void
hook_add_to_list (struct t_hook *new_hook)
{
    new_hook->prev_hook = last_weechat_hook;
    if (weechat_hooks)
        last_weechat_hook->next_hook = new_hook;
    else
        weechat_hooks = new_hook;
    last_weechat_hook = new_hook;
    new_hook->next_hook = NULL;
}

/*
 * hook_init: init a new hook with default values
 */

void
hook_init (struct t_hook *hook, void *plugin, int type, void *callback_data)
{
    hook->plugin = plugin;
    hook->type = type;
    hook->callback_data = callback_data;
    
    hook->hook_data = NULL;
    
    hook->running = 0;
}

/*
 * hook_valid_for_plugin: check if a hook pointer exists for a plugin
 *                        return 1 if hook exists for plugin
 *                               0 if hook is not found for plugin
 */

int
hook_valid_for_plugin (void *plugin, struct t_hook *hook)
{
    struct t_hook *ptr_hook;
    
    for (ptr_hook = weechat_hooks; ptr_hook;
         ptr_hook = ptr_hook->next_hook)
    {
        if ((ptr_hook == hook)
            && (ptr_hook->plugin == (struct t_weechat_plugin *)plugin))
            return 1;
    }
    
    /* hook not found */
    return 0;
}

/*
 * hook_command: hook a command
 */

struct t_hook *
hook_command (void *plugin, char *command, char *description,
              char *args, char *args_description, char *completion,
              t_hook_callback_command *callback,
              void *callback_data)
{
    struct t_hook *new_hook;
    struct t_hook_command *new_hook_command;
    
    new_hook = (struct t_hook *)malloc (sizeof (struct t_hook));
    if (!new_hook)
        return NULL;
    new_hook_command = (struct t_hook_command *)malloc (sizeof (struct t_hook_command));
    if (!new_hook_command)
    {
        free (new_hook);
        return NULL;
    }
    
    hook_init (new_hook, plugin, HOOK_TYPE_COMMAND, callback_data);

    new_hook->hook_data = new_hook_command;
    new_hook_command->callback = callback;
    new_hook_command->command = (command) ?
        strdup (command) : strdup ("");
    new_hook_command->description = (description) ?
        strdup (description) : strdup ("");
    new_hook_command->args = (args) ?
        strdup (args) : strdup ("");
    new_hook_command->args_description = (args_description) ?
        strdup (args_description) : strdup ("");
    new_hook_command->completion = (completion) ?
        strdup (completion) : strdup ("");
    
    hook_add_to_list (new_hook);
    
    if (command && command[0])
        command_index_add (command);
    
    return new_hook;
}

/*
 * hook_command_exec: execute command hook
 *                    return:  0 if command executed and failed
 *                             1 if command executed successfully
 *                            -1 if command not found
 */

int
hook_command_exec (void *plugin, char *string)
{
    struct t_hook *ptr_hook;
    char **argv, **argv_eol;
    int argc;

    if (!string || !string[0])
        return -1;
    
    argv = string_explode (string, " ", 0, 0, &argc);
    if (argc == 0)
    {
        string_free_exploded (argv);
        return -1;
    }
    argv_eol = string_explode (string, " ", 1, 0, NULL);
    
    for (ptr_hook = weechat_hooks; ptr_hook;
         ptr_hook = ptr_hook->next_hook)
    {
        if ((ptr_hook->type == HOOK_TYPE_COMMAND)
            && (!plugin || (plugin == ptr_hook->plugin))
            && (string_strcasecmp (argv[0] + 1,
                                   HOOK_COMMAND(ptr_hook, command)) == 0))
        {
            if ((int) (HOOK_COMMAND(ptr_hook, callback))
                (ptr_hook->callback_data, argc, argv, argv_eol) == PLUGIN_RC_FAILED)
                return 0;
            else
                return 1;
        }
    }
    
    string_free_exploded (argv);
    string_free_exploded (argv_eol);
    
    /* no hook found */
    return -1;
}

/*
 * hook_message: hook a message
 */

struct t_hook *
hook_message (void *plugin, char *message,
              t_hook_callback_message *callback, void *callback_data)
{
    struct t_hook *new_hook;
    struct t_hook_message *new_hook_message;
    
    new_hook = (struct t_hook *)malloc (sizeof (struct t_hook));
    if (!new_hook)
        return NULL;
    new_hook_message = (struct t_hook_message *)malloc (sizeof (struct t_hook_message));
    if (!new_hook_message)
    {
        free (new_hook);
        return NULL;
    }
    
    hook_init (new_hook, plugin, HOOK_TYPE_MESSAGE, callback_data);

    new_hook->hook_data = new_hook_message;
    new_hook_message->callback = callback;
    new_hook_message->message = (message) ? strdup (message) : strdup ("");
    
    hook_add_to_list (new_hook);
    
    return new_hook;
}

/*
 * hook_config: hook a config option
 */

struct t_hook *
hook_config (void *plugin, char *type, char *option,
             t_hook_callback_config *callback, void *callback_data)
{
    struct t_hook *new_hook;
    struct t_hook_config *new_hook_config;
    
    new_hook = (struct t_hook *)malloc (sizeof (struct t_hook));
    if (!new_hook)
        return NULL;
    new_hook_config = (struct t_hook_config *)malloc (sizeof (struct t_hook_config));
    if (!new_hook_config)
    {
        free (new_hook);
        return NULL;
    }
    
    hook_init (new_hook, plugin, HOOK_TYPE_CONFIG, callback_data);
    
    new_hook->hook_data = new_hook_config;
    new_hook_config->callback = callback;
    new_hook_config->type = (type) ? strdup (type) : strdup ("");
    new_hook_config->option = (option) ? strdup (option) : strdup ("");
    
    hook_add_to_list (new_hook);
    
    return new_hook;
}

/*
 * hook_config_exec: execute config hooks
 */

void
hook_config_exec (char *type, char *option, char *value)
{
    struct t_hook *ptr_hook, *next_hook;
    
    ptr_hook = weechat_hooks;
    while (ptr_hook)
    {
        next_hook = ptr_hook->next_hook;
        
        if ((ptr_hook->type == HOOK_TYPE_CONFIG)
            && (!HOOK_CONFIG(ptr_hook, type)
                || (string_strcasecmp (HOOK_CONFIG(ptr_hook, type),
                                       type) == 0))
            && (!HOOK_CONFIG(ptr_hook, option)
                || (string_strcasecmp (HOOK_CONFIG(ptr_hook, option),
                                       option) == 0)))
        {
            (void) (HOOK_CONFIG(ptr_hook, callback))
                (ptr_hook->callback_data, type, option, value);
        }
        
        ptr_hook = next_hook;
    }
}

/*
 * hook_timer: hook a timer
 */

struct t_hook *
hook_timer (void *plugin, long interval, int max_calls,
            t_hook_callback_timer *callback, void *callback_data)
{
    struct t_hook *new_hook;
    struct t_hook_timer *new_hook_timer;
    
    new_hook = (struct t_hook *)malloc (sizeof (struct t_hook));
    if (!new_hook)
        return NULL;
    new_hook_timer = (struct t_hook_timer *)malloc (sizeof (struct t_hook_timer));
    if (!new_hook_timer)
    {
        free (new_hook);
        return NULL;
    }
    
    hook_init (new_hook, plugin, HOOK_TYPE_TIMER, callback_data);
    
    new_hook->hook_data = new_hook_timer;
    new_hook_timer->callback = callback;
    new_hook_timer->interval = interval;
    new_hook_timer->remaining_calls = max_calls;
    gettimeofday (&new_hook_timer->last_exec, NULL);
    
    hook_add_to_list (new_hook);
    
    return new_hook;
}

/*
 * hook_timer_exec: execute timer hooks
 */

void
hook_timer_exec (struct timeval *tv_time)
{
    struct t_hook *ptr_hook, *next_hook;
    long time_diff;
    
    ptr_hook = weechat_hooks;
    while (ptr_hook)
    {
        next_hook = ptr_hook->next_hook;
        
        if (ptr_hook->type == HOOK_TYPE_TIMER)
        {
            time_diff = util_get_timeval_diff (&HOOK_TIMER(ptr_hook, last_exec),
                                               tv_time);
            if (time_diff >= HOOK_TIMER(ptr_hook, interval))
            {
                (void) (HOOK_TIMER(ptr_hook, callback))
                    (ptr_hook->callback_data);
                HOOK_TIMER(ptr_hook, last_exec).tv_sec = tv_time->tv_sec;
                HOOK_TIMER(ptr_hook, last_exec).tv_usec = tv_time->tv_usec;
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
}

/*
 * hook_search_fd: search fd hook in list
 */

struct t_hook *
hook_search_fd (int fd)
{
    struct t_hook *ptr_hook;
    
    for (ptr_hook = weechat_hooks; ptr_hook;
         ptr_hook = ptr_hook->next_hook)
    {
        if ((ptr_hook->type == HOOK_TYPE_FD)
            && (HOOK_FD(ptr_hook, fd) == fd))
            return ptr_hook;
    }
    
    /* fd hook not found */
    return NULL;
}

/*
 * hook_fd: hook a fd event
 */

struct t_hook *
hook_fd (void *plugin, int fd, int flags,
         t_hook_callback_fd *callback, void *callback_data)
{
    struct t_hook *new_hook;
    struct t_hook_fd *new_hook_fd;
    
    if (hook_search_fd (fd))
        return NULL;
    
    new_hook = (struct t_hook *)malloc (sizeof (struct t_hook));
    if (!new_hook)
        return NULL;
    new_hook_fd = (struct t_hook_fd *)malloc (sizeof (struct t_hook_fd));
    if (!new_hook_fd)
    {
        free (new_hook);
        return NULL;
    }
    
    hook_init (new_hook, plugin, HOOK_TYPE_FD, callback_data);
    
    new_hook->hook_data = new_hook_fd;
    new_hook_fd->callback = callback;
    new_hook_fd->fd = fd;
    new_hook_fd->flags = flags;
    
    hook_add_to_list (new_hook);
    
    return new_hook;
}

/*
 * hook_fd_set: fill sets according to hd hooked
 */

void
hook_fd_set (fd_set *read_fds, fd_set *write_fds, fd_set *except_fds)
{
    struct t_hook *ptr_hook;
    
    FD_ZERO (read_fds);
    FD_ZERO (write_fds);
    FD_ZERO (except_fds);
    
    for (ptr_hook = weechat_hooks; ptr_hook;
         ptr_hook = ptr_hook->next_hook)
    {
        if (ptr_hook->type == HOOK_TYPE_FD)
        {
            if (HOOK_FD(ptr_hook, flags) & HOOK_FD_FLAG_READ)
                FD_SET (HOOK_FD(ptr_hook, fd), read_fds);
            if (HOOK_FD(ptr_hook, flags) & HOOK_FD_FLAG_WRITE)
                FD_SET (HOOK_FD(ptr_hook, fd), write_fds);
            if (HOOK_FD(ptr_hook, flags) & HOOK_FD_FLAG_EXCEPTION)
                FD_SET (HOOK_FD(ptr_hook, fd), except_fds);
        }
    }
}

/*
 * hook_fd_exec: execute fd callbacks with sets
 */

void
hook_fd_exec (fd_set *read_fds, fd_set *write_fds, fd_set *except_fds)
{
    struct t_hook *ptr_hook, *next_hook;

    ptr_hook = weechat_hooks;
    while (ptr_hook)
    {
        next_hook = ptr_hook->next_hook;
        
        if ((ptr_hook->type == HOOK_TYPE_FD)
            && (((HOOK_FD(ptr_hook, flags)& HOOK_FD_FLAG_READ)
                 && (FD_ISSET(HOOK_FD(ptr_hook, fd), read_fds)))
                || ((HOOK_FD(ptr_hook, flags) & HOOK_FD_FLAG_WRITE)
                    && (FD_ISSET(HOOK_FD(ptr_hook, fd), write_fds)))
                || ((HOOK_FD(ptr_hook, flags) & HOOK_FD_FLAG_EXCEPTION)
                    && (FD_ISSET(HOOK_FD(ptr_hook, fd), except_fds)))))
            (HOOK_FD(ptr_hook, callback)) (ptr_hook->callback_data);
        
        ptr_hook = next_hook;
    }
}

/*
 * unhook: unhook something
 */

void
unhook (struct t_hook *hook)
{
    struct t_hook *new_hooks;
    
    /* free data */
    if (hook->hook_data)
    {
        switch (hook->type)
        {
            case HOOK_TYPE_COMMAND:
                if (HOOK_COMMAND(hook, command)
                    && HOOK_COMMAND(hook, command)[0])
                    command_index_remove (HOOK_COMMAND(hook, command));
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
            case HOOK_TYPE_MESSAGE:
                if (HOOK_MESSAGE(hook, message))
                    free (HOOK_MESSAGE(hook, message));
                free ((struct t_hook_message *)hook->hook_data);
                break;
            case HOOK_TYPE_CONFIG:
                if (HOOK_CONFIG(hook, type))
                    free (HOOK_CONFIG(hook, type));
                if (HOOK_CONFIG(hook, option))
                    free (HOOK_CONFIG(hook, option));
                free ((struct t_hook_config *)hook->hook_data);
                break;
            case HOOK_TYPE_TIMER:
                free ((struct t_hook_timer *)hook->hook_data);
                break;
            case HOOK_TYPE_FD:
                free ((struct t_hook_fd *)hook->hook_data);
                break;
            case HOOK_TYPE_KEYBOARD:
                break;
        }
    }           
    
    /* remove hook from list */
    if (last_weechat_hook == hook)
        last_weechat_hook = hook->prev_hook;
    if (hook->prev_hook)
    {
        hook->prev_hook->next_hook = hook->next_hook;
        new_hooks = weechat_hooks;
    }
    else
        new_hooks = hook->next_hook;
    
    if (hook->next_hook)
        hook->next_hook->prev_hook = hook->prev_hook;
    
    free (hook);
    weechat_hooks = new_hooks;
}

/*
 * unhook_all_plugin: unhook all for a plugin
 */

void
unhook_all_plugin (void *plugin)
{
    struct t_hook *ptr_hook, *next_hook;
    
    ptr_hook = weechat_hooks;
    while (ptr_hook)
    {
        next_hook = ptr_hook->next_hook;
        
        if (ptr_hook->plugin == plugin)
            unhook (ptr_hook);
        
        ptr_hook = next_hook;
    }
}

/*
 * unhook_all: unhook all
 */

void
unhook_all ()
{
    while (weechat_hooks)
    {
        unhook (weechat_hooks);
    }
}

/*
 * hook_print_log: print hooks in log (usually for crash dump)
 */

void
hook_print_log ()
{
    struct t_hook *ptr_hook;
    
    for (ptr_hook = weechat_hooks; ptr_hook;
         ptr_hook = ptr_hook->next_hook)
    {
        weechat_log_printf ("\n");
        weechat_log_printf ("[hook (addr:0x%X)]\n", ptr_hook);
        weechat_log_printf ("  type . . . . . . . . . : %d\n",   ptr_hook->type);
        weechat_log_printf ("  callback_data. . . . . : 0x%X\n", ptr_hook->callback_data);
        switch (ptr_hook->type)
        {
            case HOOK_TYPE_COMMAND:
                weechat_log_printf ("  command data:\n");
                weechat_log_printf ("    callback . . . . . . : 0x%X\n", HOOK_COMMAND(ptr_hook, callback));
                weechat_log_printf ("    command. . . . . . . : '%s'\n", HOOK_COMMAND(ptr_hook, command));
                weechat_log_printf ("    command_desc . . . . : '%s'\n", HOOK_COMMAND(ptr_hook, description));
                weechat_log_printf ("    command_args . . . . : '%s'\n", HOOK_COMMAND(ptr_hook, args));
                weechat_log_printf ("    command_args_desc. . : '%s'\n", HOOK_COMMAND(ptr_hook, args_description));
                weechat_log_printf ("    command_completion . : '%s'\n", HOOK_COMMAND(ptr_hook, completion));
                break;
            case HOOK_TYPE_MESSAGE:
                weechat_log_printf ("  message data:\n");
                weechat_log_printf ("    message. . . . . . . : '%s'\n", HOOK_MESSAGE(ptr_hook, message));
                break;
            case HOOK_TYPE_CONFIG:
                weechat_log_printf ("  config data:\n");
                weechat_log_printf ("    type . . . . . . . . : '%s'\n", HOOK_CONFIG(ptr_hook, type));
                weechat_log_printf ("    option . . . . . . . : '%s'\n", HOOK_CONFIG(ptr_hook, option));
                break;
            case HOOK_TYPE_TIMER:
                weechat_log_printf ("  timer data:\n");
                weechat_log_printf ("    interval . . . . . . : %ld\n",  HOOK_TIMER(ptr_hook, interval));
                weechat_log_printf ("    last_exec.tv_sec . . : %ld\n",  HOOK_TIMER(ptr_hook, last_exec.tv_sec));
                weechat_log_printf ("    last_exec.tv_usec. . : %ld\n",  HOOK_TIMER(ptr_hook, last_exec.tv_usec));
                break;
            case HOOK_TYPE_FD:
                weechat_log_printf ("  fd data:\n");
                weechat_log_printf ("    fd . . . . . . . . . : %ld\n",  HOOK_FD(ptr_hook, fd));
                weechat_log_printf ("    flags. . . . . . . . : %ld\n",  HOOK_FD(ptr_hook, flags));
                break;
            case HOOK_TYPE_KEYBOARD:
                weechat_log_printf ("  keyboard data:\n");
                break;
        }        
        weechat_log_printf ("  running. . . . . . . . : %d\n",   ptr_hook->running);
        weechat_log_printf ("  prev_hook. . . . . . . : 0x%X\n", ptr_hook->prev_hook);
        weechat_log_printf ("  next_hook. . . . . . . : 0x%X\n", ptr_hook->next_hook);
    }
}
