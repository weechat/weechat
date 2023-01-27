/*
 * wee-hook.c - WeeChat hooks management
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
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <errno.h>

#include "weechat.h"
#include "wee-hook.h"
#include "wee-hashtable.h"
#include "wee-infolist.h"
#include "wee-log.h"
#include "wee-signal.h"
#include "wee-string.h"
#include "../gui/gui-chat.h"
#include "../plugins/plugin.h"


char *hook_type_string[HOOK_NUM_TYPES] =
{ "command", "command_run", "timer", "fd", "process", "connect", "line",
  "print", "signal", "hsignal", "config", "completion", "modifier",
  "info", "info_hashtable", "infolist", "hdata", "focus" };
struct t_hook *weechat_hooks[HOOK_NUM_TYPES];     /* list of hooks          */
struct t_hook *last_weechat_hook[HOOK_NUM_TYPES]; /* last hook              */
int hooks_count[HOOK_NUM_TYPES];                  /* number of hooks        */
int hooks_count_total = 0;                        /* total number of hooks  */
int hook_exec_recursion = 0;           /* 1 when a hook is executed         */
int real_delete_pending = 0;           /* 1 if some hooks must be deleted   */

int hook_socketpair_ok = 0;            /* 1 if socketpair() is OK           */

/* hook callbacks */
t_callback_hook *hook_callback_add[HOOK_NUM_TYPES] =
{ NULL, NULL, NULL, &hook_fd_add_cb, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
t_callback_hook *hook_callback_remove[HOOK_NUM_TYPES] =
{ NULL, NULL, NULL, &hook_fd_remove_cb, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
t_callback_hook *hook_callback_free_data[HOOK_NUM_TYPES] =
{ &hook_command_free_data, &hook_command_run_free_data,
  &hook_timer_free_data, &hook_fd_free_data,
  &hook_process_free_data, &hook_connect_free_data,
  &hook_line_free_data, &hook_print_free_data,
  &hook_signal_free_data, &hook_hsignal_free_data,
  &hook_config_free_data, &hook_completion_free_data,
  &hook_modifier_free_data, &hook_info_free_data,
  &hook_info_hashtable_free_data, &hook_infolist_free_data,
  &hook_hdata_free_data, &hook_focus_free_data };
t_callback_hook_get_desc *hook_callback_get_desc[HOOK_NUM_TYPES] =
{ &hook_command_get_description, &hook_command_run_get_description,
  &hook_timer_get_description, &hook_fd_get_description,
  &hook_process_get_description, &hook_connect_get_description,
  &hook_line_get_description, &hook_print_get_description,
  &hook_signal_get_description, &hook_hsignal_get_description,
  &hook_config_get_description, &hook_completion_get_description,
  &hook_modifier_get_description, &hook_info_get_description,
  &hook_info_hashtable_get_description, &hook_infolist_get_description,
  &hook_hdata_get_description, &hook_focus_get_description };
t_callback_hook_infolist *hook_callback_add_to_infolist[HOOK_NUM_TYPES] =
{ &hook_command_add_to_infolist, &hook_command_run_add_to_infolist,
  &hook_timer_add_to_infolist, &hook_fd_add_to_infolist,
  &hook_process_add_to_infolist, &hook_connect_add_to_infolist,
  &hook_line_add_to_infolist, &hook_print_add_to_infolist,
  &hook_signal_add_to_infolist, &hook_hsignal_add_to_infolist,
  &hook_config_add_to_infolist, &hook_completion_add_to_infolist,
  &hook_modifier_add_to_infolist, &hook_info_add_to_infolist,
  &hook_info_hashtable_add_to_infolist, &hook_infolist_add_to_infolist,
  &hook_hdata_add_to_infolist, &hook_focus_add_to_infolist };
t_callback_hook *hook_callback_print_log[HOOK_NUM_TYPES] =
{ &hook_command_print_log, &hook_command_run_print_log,
  &hook_timer_print_log, &hook_fd_print_log,
  &hook_process_print_log, &hook_connect_print_log,
  &hook_line_print_log, &hook_print_print_log,
  &hook_signal_print_log, &hook_hsignal_print_log,
  &hook_config_print_log, &hook_completion_print_log,
  &hook_modifier_print_log, &hook_info_print_log,
  &hook_info_hashtable_print_log, &hook_infolist_print_log,
  &hook_hdata_print_log, &hook_focus_print_log };


/*
 * Initializes hooks.
 */

void
hook_init ()
{
    int type, sock[2], rc;

    /* initialize list of hooks and callbacks */
    for (type = 0; type < HOOK_NUM_TYPES; type++)
    {
        weechat_hooks[type] = NULL;
        last_weechat_hook[type] = NULL;
        hooks_count[type] = 0;
    }
    hooks_count_total = 0;
    hook_last_system_time = time (NULL);

    /*
     * Set a flag to 0 if socketpair() function is not available.
     *
     * For the connect hook, when this is defined an array of sockets will
     * be passed from the parent process to the child process instead of using
     * SCM_RIGHTS to pass a socket back from the child process to parent
     * process.
     *
     * This allows connections to work on Windows but it limits the number of
     * IPs that can be attempted each time.
     */
    hook_socketpair_ok = 1;

#if defined(__CYGWIN__) || defined(__APPLE__) || defined(__MACH__)
    hook_socketpair_ok = 0;
    (void) sock;
    (void) rc;
#else
    /*
     * Test if socketpair() function is working fine: this is NOT the case
     * on Windows with Ubuntu bash
     * (errno == 94: ESOCKTNOSUPPORT: socket type not supported)
     */
    rc = socketpair (AF_LOCAL, SOCK_DGRAM, 0, sock);
    if (rc < 0)
    {
        /* Windows/Ubuntu */
        hook_socketpair_ok = 0;
    }
    else
    {
        close (sock[0]);
        close (sock[1]);
    }
#endif
}

/*
 * Searches for a hook type.
 *
 * Returns index of type in enum t_hook_type, -1 if type is not found.
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
 * Searches for position of hook in list (to keep hooks sorted).
 *
 * Hooks are sorted by priority, except commands which are sorted by command
 * name, and then priority.
 */

struct t_hook *
hook_find_pos (struct t_hook *hook)
{
    struct t_hook *ptr_hook;
    int rc_cmp;

    if (hook->type == HOOK_TYPE_COMMAND)
    {
        /* for command hook, sort on command name + priority */
        for (ptr_hook = weechat_hooks[hook->type]; ptr_hook;
             ptr_hook = ptr_hook->next_hook)
        {
            if (!ptr_hook->deleted)
            {
                rc_cmp = string_strcmp (HOOK_COMMAND(hook, command),
                                        HOOK_COMMAND(ptr_hook, command));
                if (rc_cmp < 0)
                    return ptr_hook;
                if ((rc_cmp == 0) && (hook->priority > ptr_hook->priority))
                    return ptr_hook;
            }
        }
    }
    else
    {
        /* for other types, sort on priority */
        for (ptr_hook = weechat_hooks[hook->type]; ptr_hook;
             ptr_hook = ptr_hook->next_hook)
        {
            if (!ptr_hook->deleted && (hook->priority > ptr_hook->priority))
                return ptr_hook;
        }
    }

    /* position not found, add at the end */
    return NULL;
}

/*
 * Adds a hook to list.
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
            /* add hook before "pos_hook" */
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

    hooks_count[new_hook->type]++;
    hooks_count_total++;

    if (hook_callback_add[new_hook->type])
        (hook_callback_add[new_hook->type]) (new_hook);
}

/*
 * Removes a hook from list.
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

    weechat_hooks[type] = new_hooks;

    hooks_count[type]--;
    hooks_count_total--;

    if (hook_callback_remove[hook->type])
        (hook_callback_remove[hook->type]) (hook);

    free (hook);
}

/*
 * Removes hooks marked as "deleted" from list.
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
 * Initializes a new hook with default values.
 */

void
hook_init_data (struct t_hook *hook, struct t_weechat_plugin *plugin,
                int type, int priority,
                const void *callback_pointer, void *callback_data)
{
    hook->plugin = plugin;
    hook->subplugin = NULL;
    hook->type = type;
    hook->deleted = 0;
    hook->running = 0;
    hook->priority = priority;
    hook->callback_pointer = callback_pointer;
    hook->callback_data = callback_data;
    hook->hook_data = NULL;

    if (weechat_debug_core >= 2)
    {
        gui_chat_printf (NULL,
                         "debug: adding hook: type=%d (%s), plugin=\"%s\", "
                         "priority=%d",
                         hook->type,
                         hook_type_string[hook->type],
                         plugin_get_name (hook->plugin),
                         hook->priority);
    }
}

/*
 * Checks if a hook pointer is valid.
 *
 * Returns:
 *   1: hook exists
 *   0: hook does not exist
 */

int
hook_valid (struct t_hook *hook)
{
    int type;
    struct t_hook *ptr_hook;

    if (!hook)
        return 0;

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
 * Starts a hook exec.
 */

void
hook_exec_start ()
{
    hook_exec_recursion++;
}

/*
 * Ends a hook_exec.
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
 * Returns description of hook.
 *
 * Note: result must be freed after use.
 */

char *
hook_get_description (struct t_hook *hook)
{
    return (hook_callback_get_desc[hook->type]) (hook);
}

/*
 * Sets a hook property (string).
 */

void
hook_set (struct t_hook *hook, const char *property, const char *value)
{
    ssize_t num_written;
    char *error;
    long number;
    int rc;

    /* invalid hook? */
    if (!hook_valid (hook))
        return;

    if (!property)
        return;

    if (strcmp (property, "subplugin") == 0)
    {
        if (hook->subplugin)
            free (hook->subplugin);
        hook->subplugin = strdup (value);
    }
    else if (strcmp (property, "stdin") == 0)
    {
        if (!hook->deleted
            && (hook->type == HOOK_TYPE_PROCESS)
            && (HOOK_PROCESS(hook, child_write[HOOK_PROCESS_STDIN]) >= 0))
        {
            /* send data on child's stdin */
            num_written = write (HOOK_PROCESS(hook, child_write[HOOK_PROCESS_STDIN]),
                                 value, strlen (value));
            (void) num_written;
        }
    }
    else if (strcmp (property, "stdin_close") == 0)
    {
        if (!hook->deleted
            && (hook->type == HOOK_TYPE_PROCESS)
            && (HOOK_PROCESS(hook, child_write[HOOK_PROCESS_STDIN]) >= 0))
        {
            /* close stdin pipe */
            close (HOOK_PROCESS(hook, child_write[HOOK_PROCESS_STDIN]));
            HOOK_PROCESS(hook, child_write[HOOK_PROCESS_STDIN]) = -1;
        }
    }
    else if (strcmp (property, "signal") == 0)
    {
        if (!hook->deleted
            && (hook->type == HOOK_TYPE_PROCESS)
            && (HOOK_PROCESS(hook, child_pid) > 0))
        {
            error = NULL;
            number = strtol (value, &error, 10);
            if (!error || error[0])
            {
                /* not a number? look for signal by name */
                number = signal_search_name (value);
            }
            if (number >= 0)
            {
                rc = kill (HOOK_PROCESS(hook, child_pid), (int)number);
                if (rc < 0)
                {
                    gui_chat_printf (NULL,
                                     _("%sError sending signal %d to pid %d: %s"),
                                     gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                     (int)number,
                                     HOOK_PROCESS(hook, child_pid),
                                     strerror (errno));
                }
            }
        }
    }
}

/*
 * Unhooks something.
 */

void
unhook (struct t_hook *hook)
{
    /* invalid hook? */
    if (!hook_valid (hook))
        return;

    /* hook already deleted? */
    if (hook->deleted)
        return;

    if (weechat_debug_core >= 2)
    {
        gui_chat_printf (NULL,
                         "debug: removing hook: type=%d (%s), plugin=\"%s\"",
                         hook->type,
                         hook_type_string[hook->type],
                         plugin_get_name (hook->plugin));
    }

    /* free data specific to the hook */
    (hook_callback_free_data[hook->type]) (hook);

    /* free data common to all hooks */
    if (hook->subplugin)
    {
        free (hook->subplugin);
        hook->subplugin = NULL;
    }
    if (hook->callback_data)
    {
        free (hook->callback_data);
        hook->callback_data = NULL;
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
 * Unhooks everything for a plugin/subplugin.
 */

void
unhook_all_plugin (struct t_weechat_plugin *plugin, const char *subplugin)
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
            {
                if (!subplugin
                    || (ptr_hook->subplugin &&
                        strcmp (ptr_hook->subplugin, subplugin) == 0))
                {
                    unhook (ptr_hook);
                }
            }
            ptr_hook = next_hook;
        }
    }
}

/*
 * Unhooks everything.
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
 * Adds a hook in an infolist.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
hook_add_to_infolist_pointer (struct t_infolist *infolist, struct t_hook *hook)
{
    struct t_infolist_item *ptr_item;

    ptr_item = infolist_new_item (infolist);
    if (!ptr_item)
        return 0;

    if (!infolist_new_var_pointer (ptr_item, "pointer", hook))
        return 0;
    if (!infolist_new_var_pointer (ptr_item, "plugin", hook->plugin))
        return 0;
    if (!infolist_new_var_string (ptr_item, "plugin_name",
                                  (hook->plugin) ?
                                  hook->plugin->name : NULL))
        return 0;
    if (!infolist_new_var_string (ptr_item, "subplugin", hook->subplugin))
        return 0;
    if (!infolist_new_var_string (ptr_item, "type", hook_type_string[hook->type]))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "deleted", hook->deleted))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "running", hook->running))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "priority", hook->priority))
        return 0;
    if (!infolist_new_var_pointer (ptr_item, "callback_pointer", (void *)hook->callback_pointer))
        return 0;
    if (!infolist_new_var_pointer (ptr_item, "callback_data", (void *)hook->callback_data))
        return 0;

    /* hook deleted? return only hook info above */
    if (hook->deleted)
        return 1;

    /* hook not deleted: add extra hook info */
    if (!(hook_callback_add_to_infolist[hook->type]) (ptr_item, hook))
        return 0;

    return 1;
}

/*
 * Adds hooks of a type in an infolist.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
hook_add_to_infolist_type (struct t_infolist *infolist, int type,
                           const char *arguments)
{
    struct t_hook *ptr_hook;
    int match;

    for (ptr_hook = weechat_hooks[type]; ptr_hook;
         ptr_hook = ptr_hook->next_hook)
    {
        match = 1;
        if (arguments && !ptr_hook->deleted)
        {
            switch (ptr_hook->type)
            {
                case HOOK_TYPE_COMMAND:
                    match = string_match (HOOK_COMMAND(ptr_hook, command), arguments, 1);
                    break;
                default:
                    break;
            }
        }

        if (!match)
            continue;

        hook_add_to_infolist_pointer (infolist, ptr_hook);
    }

    return 1;
}

/*
 * Adds hooks in an infolist.
 *
 * Argument "arguments" can be a hook type with optional comma + name after.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
hook_add_to_infolist (struct t_infolist *infolist, struct t_hook *pointer,
                      const char *arguments)
{
    const char *pos_arguments;
    char *type;
    int i, type_int;

    if (!infolist)
        return 0;

    if (pointer)
        return hook_add_to_infolist_pointer (infolist, pointer);

    type = NULL;
    pos_arguments = NULL;

    if (arguments && arguments[0])
    {
        pos_arguments = strchr (arguments, ',');
        if (pos_arguments)
        {
            type = string_strndup (arguments, pos_arguments - arguments);
            pos_arguments++;
        }
        else
            type = strdup (arguments);
    }

    type_int = (type) ? hook_search_type (type) : -1;

    for (i = 0; i < HOOK_NUM_TYPES; i++)
    {
        if ((type_int < 0) || (type_int == i))
            hook_add_to_infolist_type (infolist, i, pos_arguments);
    }

    if (type)
        free (type);

    return 1;
}

/*
 * Prints hooks in WeeChat log file (usually for crash dump).
 */

void
hook_print_log ()
{
    int type;
    struct t_hook *ptr_hook;

    for (type = 0; type < HOOK_NUM_TYPES; type++)
    {
        for (ptr_hook = weechat_hooks[type]; ptr_hook;
             ptr_hook = ptr_hook->next_hook)
        {
            log_printf ("");
            log_printf ("[hook (addr:0x%lx)]", ptr_hook);
            log_printf ("  plugin. . . . . . . . . : 0x%lx ('%s')",
                        ptr_hook->plugin, plugin_get_name (ptr_hook->plugin));
            log_printf ("  subplugin . . . . . . . : '%s'",  ptr_hook->subplugin);
            log_printf ("  type. . . . . . . . . . : %d (%s)",
                        ptr_hook->type, hook_type_string[ptr_hook->type]);
            log_printf ("  deleted . . . . . . . . : %d",    ptr_hook->deleted);
            log_printf ("  running . . . . . . . . : %d",    ptr_hook->running);
            log_printf ("  priority. . . . . . . . : %d",    ptr_hook->priority);
            log_printf ("  callback_pointer. . . . : 0x%lx", ptr_hook->callback_pointer);
            log_printf ("  callback_data . . . . . : 0x%lx", ptr_hook->callback_data);
            if (ptr_hook->deleted)
                continue;

            (hook_callback_print_log[ptr_hook->type]) (ptr_hook);

            log_printf ("  prev_hook . . . . . . . : 0x%lx", ptr_hook->prev_hook);
            log_printf ("  next_hook . . . . . . . : 0x%lx", ptr_hook->next_hook);
        }
    }
}
