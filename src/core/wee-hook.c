/*
 * wee-hook.c - WeeChat hooks management
 *
 * Copyright (C) 2003-2012 Sebastien Helleu <flashcode@flashtux.org>
 * Copyright (C) 2012 Simon Arlott
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
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>

#include "weechat.h"
#include "wee-hook.h"
#include "wee-hashtable.h"
#include "wee-hdata.h"
#include "wee-infolist.h"
#include "wee-list.h"
#include "wee-log.h"
#include "wee-network.h"
#include "wee-string.h"
#include "wee-url.h"
#include "wee-utf8.h"
#include "wee-util.h"
#include "../gui/gui-chat.h"
#include "../gui/gui-bar.h"
#include "../gui/gui-bar-window.h"
#include "../gui/gui-buffer.h"
#include "../gui/gui-color.h"
#include "../gui/gui-completion.h"
#include "../gui/gui-focus.h"
#include "../gui/gui-line.h"
#include "../gui/gui-window.h"
#include "../plugins/plugin.h"


char *hook_type_string[HOOK_NUM_TYPES] =
{ "command", "command_run", "timer", "fd", "process", "connect", "print",
  "signal", "hsignal", "config", "completion", "modifier",
  "info", "info_hashtable", "infolist", "hdata", "focus" };
struct t_hook *weechat_hooks[HOOK_NUM_TYPES];     /* list of hooks          */
struct t_hook *last_weechat_hook[HOOK_NUM_TYPES]; /* last hook              */
int hook_exec_recursion = 0;           /* 1 when a hook is executed         */
time_t hook_last_system_time = 0;      /* used to detect system clock skew  */
int real_delete_pending = 0;           /* 1 if some hooks must be deleted   */


void hook_process_run (struct t_hook *hook_process);


/*
 * Initializes lists of hooks.
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
        /* for command hook, sort on command name */
        for (ptr_hook = weechat_hooks[hook->type]; ptr_hook;
             ptr_hook = ptr_hook->next_hook)
        {
            if (!ptr_hook->deleted)
            {
                rc_cmp = string_strcasecmp (HOOK_COMMAND(hook, command),
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

    free (hook);

    weechat_hooks[type] = new_hooks;
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
 * Extracts priority and name from a string.
 *
 * String can be:
 *   - a simple name like "test":
 *       => priority = 1000 (default), name = "test"
 *   - a priority + "|" + name, like "500|test":
 *       => priority = 500, name = "test"
 */

void
hook_get_priority_and_name (const char *string,
                            int *priority, const char **name)
{
    char *pos, *str_priority, *error;
    long number;

    if (priority)
        *priority = HOOK_PRIORITY_DEFAULT;
    if (name)
        *name = string;

    pos = strchr (string, '|');
    if (pos)
    {
        str_priority = string_strndup (string, pos - string);
        if (str_priority)
        {
            error = NULL;
            number = strtol (str_priority, &error, 10);
            if (error && !error[0])
            {
                if (priority)
                    *priority = number;
                if (name)
                    *name = pos + 1;
            }
            free (str_priority);
        }
    }
}

/*
 * Initializes a new hook with default values.
 */

void
hook_init_data (struct t_hook *hook, struct t_weechat_plugin *plugin,
                int type, int priority, void *callback_data)
{
    hook->plugin = plugin;
    hook->subplugin = NULL;
    hook->type = type;
    hook->deleted = 0;
    hook->running = 0;
    hook->priority = priority;
    hook->callback_data = callback_data;
    hook->hook_data = NULL;

    if (weechat_debug_core >= 2)
    {
        gui_chat_printf (NULL,
                         "debug: adding hook: type=%d (%s), plugin=%lx (%s), "
                         "priority=%d",
                         hook->type, hook_type_string[hook->type],
                         hook->plugin, plugin_get_name (hook->plugin),
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
 * Searches for a command hook in list.
 *
 * Returns pointer to hook found, NULL if not found.
 */

struct t_hook *
hook_search_command (struct t_weechat_plugin *plugin, const char *command)
{
    struct t_hook *ptr_hook;

    for (ptr_hook = weechat_hooks[HOOK_TYPE_COMMAND]; ptr_hook;
         ptr_hook = ptr_hook->next_hook)
    {
        if (!ptr_hook->deleted
            && (ptr_hook->plugin == plugin)
            && (string_strcasecmp (HOOK_COMMAND(ptr_hook, command), command) == 0))
            return ptr_hook;
    }

    /* command hook not found for plugin */
    return NULL;
}

/*
 * Builds variables/arrays that will be used for completion of commands
 * arguments.
 */

void
hook_command_build_completion (struct t_hook_command *hook_command)
{
    int i, j, k, length, num_items;
    struct t_weelist *list;
    char *pos_completion, *pos_double_pipe, *pos_start, *pos_end;
    char **items, *last_space, *ptr_template;

    /* split templates using "||" as separator */
    hook_command->cplt_num_templates = 1;
    pos_completion = hook_command->completion;
    while ((pos_double_pipe = strstr (pos_completion, "||")) != NULL)
    {
        hook_command->cplt_num_templates++;
        pos_completion = pos_double_pipe + 2;
    }
    hook_command->cplt_templates = malloc (hook_command->cplt_num_templates *
                                           sizeof (*hook_command->cplt_templates));
    for (i = 0; i < hook_command->cplt_num_templates; i++)
    {
        hook_command->cplt_templates[i] = NULL;
    }
    pos_completion = hook_command->completion;
    i = 0;
    while (pos_completion)
    {
        pos_double_pipe = strstr (pos_completion, "||");
        if (!pos_double_pipe)
            pos_double_pipe = pos_completion + strlen (pos_completion);
        pos_start = pos_completion;
        pos_end = pos_double_pipe - 1;
        if (pos_end < pos_start)
        {
            hook_command->cplt_templates[i] = strdup ("");
        }
        else
        {
            while (pos_start[0] == ' ')
            {
                pos_start++;
            }
            pos_end = pos_double_pipe - 1;
            while ((pos_end > pos_start) && (pos_end[0] == ' '))
            {
                pos_end--;
            }
            hook_command->cplt_templates[i] = string_strndup (pos_start,
                                                              pos_end - pos_start + 1);
        }
        i++;
        if (!pos_double_pipe[0])
            pos_completion = NULL;
        else
            pos_completion = pos_double_pipe + 2;
    }

    /* for each template, split/count args */
    hook_command->cplt_templates_static = malloc (hook_command->cplt_num_templates *
                                                  sizeof (*hook_command->cplt_templates_static));
    hook_command->cplt_template_num_args = malloc (hook_command->cplt_num_templates *
                                                   sizeof (*hook_command->cplt_template_num_args));
    hook_command->cplt_template_args = malloc (hook_command->cplt_num_templates *
                                               sizeof (*hook_command->cplt_template_args));
    hook_command->cplt_template_num_args_concat = 0;
    for (i = 0; i < hook_command->cplt_num_templates; i++)
    {
        /*
         * build static part of template: it's first argument(s) which does not
         * contain "%" or "|"
         */
        last_space = NULL;
        ptr_template = hook_command->cplt_templates[i];
        while (ptr_template && ptr_template[0])
        {
            if (ptr_template[0] == ' ')
            {
                last_space = ptr_template;
                break;
            }
            else if ((ptr_template[0] == '%') || (ptr_template[0] == '|'))
                break;
            ptr_template = utf8_next_char (ptr_template);
        }
        if (last_space)
        {
            last_space--;
            while (last_space > hook_command->cplt_templates[i])
            {
                if (last_space[0] != ' ')
                    break;
            }
            if (last_space < hook_command->cplt_templates[i])
                last_space = NULL;
            else
                last_space++;
        }
        if (last_space)
            hook_command->cplt_templates_static[i] = string_strndup (hook_command->cplt_templates[i],
                                                                            last_space - hook_command->cplt_templates[i]);
        else
            hook_command->cplt_templates_static[i] = strdup (hook_command->cplt_templates[i]);

        /* build arguments for each template */
        hook_command->cplt_template_args[i] = string_split (hook_command->cplt_templates[i],
                                                            " ", 0, 0,
                                                            &(hook_command->cplt_template_num_args[i]));
        if (hook_command->cplt_template_num_args[i] > hook_command->cplt_template_num_args_concat)
            hook_command->cplt_template_num_args_concat = hook_command->cplt_template_num_args[i];
    }

    /*
     * build strings with concatentaion of items from different templates
     * for each argument: these strings will be used when completing argument
     * if we can't find which template to use (for example for first argument)
     */
    if (hook_command->cplt_template_num_args_concat == 0)
        hook_command->cplt_template_args_concat = NULL;
    else
    {
        hook_command->cplt_template_args_concat = malloc (hook_command->cplt_template_num_args_concat *
                                                          sizeof (*hook_command->cplt_template_args_concat));
        list = weelist_new ();
        for (i = 0; i < hook_command->cplt_template_num_args_concat; i++)
        {
            /* first compute length */
            length = 1;
            for (j = 0; j < hook_command->cplt_num_templates; j++)
            {
                if (i < hook_command->cplt_template_num_args[j])
                    length += strlen (hook_command->cplt_template_args[j][i]) + 1;
            }
            /* alloc memory */
            hook_command->cplt_template_args_concat[i] = malloc (length);
            if (hook_command->cplt_template_args_concat[i])
            {
                /* concatene items with "|" as separator */
                weelist_remove_all (list);
                hook_command->cplt_template_args_concat[i][0] = '\0';
                for (j = 0; j < hook_command->cplt_num_templates; j++)
                {
                    if (i < hook_command->cplt_template_num_args[j])
                    {
                        items = string_split (hook_command->cplt_template_args[j][i],
                                              "|", 0, 0, &num_items);
                        for (k = 0; k < num_items; k++)
                        {
                            if (!weelist_search (list, items[k]))
                            {
                                if (hook_command->cplt_template_args_concat[i][0])
                                    strcat (hook_command->cplt_template_args_concat[i], "|");
                                strcat (hook_command->cplt_template_args_concat[i],
                                        items[k]);
                                weelist_add (list, items[k], WEECHAT_LIST_POS_END,
                                             NULL);
                            }
                        }
                        string_free_split (items);
                    }
                }
            }
        }
        weelist_free (list);
    }
}

/*
 * Hooks a command.
 *
 * Returns pointer to new hook, NULL if error.
 */

struct t_hook *
hook_command (struct t_weechat_plugin *plugin, const char *command,
              const char *description,
              const char *args, const char *args_description,
              const char *completion,
              t_hook_callback_command *callback, void *callback_data)
{
    struct t_hook *new_hook;
    struct t_hook_command *new_hook_command;
    int priority;
    const char *ptr_command;

    if (!callback)
        return NULL;

    if (hook_search_command (plugin, command))
    {
        gui_chat_printf (NULL,
                         _("%sError: another command \"%s\" already exists "
                           "for plugin \"%s\""),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         command,
                         plugin_get_name (plugin));
        return NULL;
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

    hook_get_priority_and_name (command, &priority, &ptr_command);
    hook_init_data (new_hook, plugin, HOOK_TYPE_COMMAND, priority,
                    callback_data);

    new_hook->hook_data = new_hook_command;
    new_hook_command->callback = callback;
    new_hook_command->command = strdup ((ptr_command) ? ptr_command :
                                        ((command) ? command : ""));
    new_hook_command->description = strdup ((description) ? description : "");
    new_hook_command->args = strdup ((args) ? args : "");
    new_hook_command->args_description = strdup ((args_description) ?
                                                 args_description : "");
    new_hook_command->completion = strdup ((completion) ? completion : "");

    /* build completion variables for command */
    new_hook_command->cplt_num_templates = 0;
    new_hook_command->cplt_templates = NULL;
    new_hook_command->cplt_templates_static = NULL;
    new_hook_command->cplt_template_num_args = NULL;
    new_hook_command->cplt_template_args = NULL;
    new_hook_command->cplt_template_num_args_concat = 0;
    new_hook_command->cplt_template_args_concat = NULL;
    hook_command_build_completion (new_hook_command);

    hook_add_to_list (new_hook);

    return new_hook;
}

/*
 * Executes a command hook.
 *
 * Returns:
 *    0: command executed and failed
 *    1: command executed successfully
 *   -1: command not found
 *   -2: command is ambiguous (same command exists for another plugin, and we
 *       don't know which one to run)
 *   -3: command is already running
 */

int
hook_command_exec (struct t_gui_buffer *buffer, int any_plugin,
                   struct t_weechat_plugin *plugin, const char *string)
{
    struct t_hook *ptr_hook, *next_hook;
    struct t_hook *hook_plugin, *hook_other_plugin, *hook_other_plugin2;
    char **argv, **argv_eol, *ptr_command_name;
    int argc, rc, count_other_plugin;

    if (!buffer || !string || !string[0])
        return -1;

    rc = hook_command_run_exec (buffer, string);
    if (rc == WEECHAT_RC_OK_EAT)
        return 1;

    rc = -1;

    argv = string_split (string, " ", 0, 0, &argc);
    if (argc == 0)
    {
        string_free_split (argv);
        return -1;
    }
    argv_eol = string_split (string, " ", 1, 0, NULL);

    ptr_command_name = utf8_next_char (argv[0]);

    hook_exec_start ();

    hook_plugin = NULL;
    hook_other_plugin = NULL;
    hook_other_plugin2 = NULL;
    count_other_plugin = 0;
    ptr_hook = weechat_hooks[HOOK_TYPE_COMMAND];
    while (ptr_hook)
    {
        next_hook = ptr_hook->next_hook;

        if (!ptr_hook->deleted
            && (string_strcasecmp (ptr_command_name,
                                   HOOK_COMMAND(ptr_hook, command)) == 0))
        {
            if (ptr_hook->plugin == plugin)
            {
                if (!hook_plugin)
                    hook_plugin = ptr_hook;
            }
            else
            {
                if (any_plugin)
                {
                    if (!hook_other_plugin)
                        hook_other_plugin = ptr_hook;
                    else if (!hook_other_plugin2)
                        hook_other_plugin2 = ptr_hook;
                    count_other_plugin++;
                }
            }
        }

        ptr_hook = next_hook;
    }

    if (!hook_plugin && !hook_other_plugin)
    {
        /* command not found at all */
        rc = -1;
    }
    else
    {
        if (!hook_plugin && (count_other_plugin > 1)
            && (hook_other_plugin->priority == hook_other_plugin2->priority))
        {
            /*
             * ambiguous: no command for current plugin, but more than one
             * command was found for other plugins with the same priority
             * => we don't know which one to run!
             */
            rc = -2;
        }
        else
        {
            if (hook_plugin && hook_other_plugin)
            {
                /*
                 * if we have a command in current plugin and another plugin,
                 * choose the command with the higher priority (if priority
                 * is the same, always choose the command for the current
                 * plugin)
                 */
                ptr_hook = (hook_other_plugin->priority > hook_plugin->priority) ?
                    hook_other_plugin : hook_plugin;
            }
            else
            {
                /*
                 * choose the command for current plugin, if found, otherwise
                 * use command found in another plugin
                 */
                ptr_hook = (hook_plugin) ? hook_plugin : hook_other_plugin;
            }

            if (ptr_hook->running >= HOOK_COMMAND_MAX_CALLS)
            {
                /* loop in execution of command => do NOT execute again */
                rc = -3;
            }
            else
            {
                /* execute the command! */
                ptr_hook->running++;
                rc = (int) (HOOK_COMMAND(ptr_hook, callback))
                    (ptr_hook->callback_data, buffer, argc, argv, argv_eol);
                ptr_hook->running--;
                if (rc == WEECHAT_RC_ERROR)
                    rc = 0;
                else
                    rc = 1;
            }
        }
    }

    string_free_split (argv);
    string_free_split (argv_eol);

    hook_exec_end ();

    return rc;
}

/*
 * Hooks a command when it's run by WeeChat.
 *
 * Returns pointer to new hook, NULL if error.
 */

struct t_hook *
hook_command_run (struct t_weechat_plugin *plugin, const char *command,
                  t_hook_callback_command_run *callback, void *callback_data)
{
    struct t_hook *new_hook;
    struct t_hook_command_run *new_hook_command_run;
    int priority;
    const char *ptr_command;

    if (!callback)
        return NULL;

    new_hook = malloc (sizeof (*new_hook));
    if (!new_hook)
        return NULL;
    new_hook_command_run = malloc (sizeof (*new_hook_command_run));
    if (!new_hook_command_run)
    {
        free (new_hook);
        return NULL;
    }

    hook_get_priority_and_name (command, &priority, &ptr_command);
    hook_init_data (new_hook, plugin, HOOK_TYPE_COMMAND_RUN, priority,
                    callback_data);

    new_hook->hook_data = new_hook_command_run;
    new_hook_command_run->callback = callback;
    new_hook_command_run->command = strdup ((ptr_command) ? ptr_command :
                                            ((command) ? command : ""));

    hook_add_to_list (new_hook);

    return new_hook;
}

/*
 * Executes a command_run hook.
 */

int
hook_command_run_exec (struct t_gui_buffer *buffer, const char *command)
{
    struct t_hook *ptr_hook, *next_hook;
    int rc, hook_matching, length;
    char *command2;
    const char *ptr_command;

    ptr_command = command;
    command2 = NULL;

    if (command[0] != '/')
    {
        length = strlen (command) + 1;
        command2 = malloc (length);
        if (command2)
        {
            snprintf (command2, length, "/%s", command + 1);
            ptr_command = command2;
        }
    }

    ptr_hook = weechat_hooks[HOOK_TYPE_COMMAND_RUN];
    while (ptr_hook)
    {
        next_hook = ptr_hook->next_hook;

        if (!ptr_hook->deleted
            && !ptr_hook->running
            && HOOK_COMMAND_RUN(ptr_hook, command))
        {
            hook_matching = string_match (ptr_command,
                                          HOOK_COMMAND_RUN(ptr_hook, command),
                                          0);

            if (!hook_matching
                && !strchr (HOOK_COMMAND_RUN(ptr_hook, command), ' '))
            {
                hook_matching = (string_strncasecmp (ptr_command,
                                                     HOOK_COMMAND_RUN(ptr_hook, command),
                                                     utf8_strlen (HOOK_COMMAND_RUN(ptr_hook, command))) == 0);
            }

            if (hook_matching)
            {
                ptr_hook->running = 1;
                rc = (HOOK_COMMAND_RUN(ptr_hook, callback)) (ptr_hook->callback_data,
                                                             buffer,
                                                             ptr_command);
                ptr_hook->running = 0;
                if (rc == WEECHAT_RC_OK_EAT)
                {
                    if (command2)
                        free (command2);
                    return rc;
                }
            }
        }

        ptr_hook = next_hook;
    }

    if (command2)
        free (command2);

    return WEECHAT_RC_OK;
}

/*
 * Initializes a timer hook.
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
        /*
         * here we should use 0, but with this value timer is sometimes called
         * before second has changed, so for displaying time, it may display
         * 2 times the same second, that's why we use 1000 micro seconds
         */
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
 * Hooks a timer.
 *
 * Returns pointer to new hook, NULL if error.
 */

struct t_hook *
hook_timer (struct t_weechat_plugin *plugin, long interval, int align_second,
            int max_calls, t_hook_callback_timer *callback,
            void *callback_data)
{
    struct t_hook *new_hook;
    struct t_hook_timer *new_hook_timer;

    if ((interval <= 0) || !callback)
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

    hook_init_data (new_hook, plugin, HOOK_TYPE_TIMER, HOOK_PRIORITY_DEFAULT,
                    callback_data);

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
 * Checks if system clock is older than previous call to this function (that
 * means new time is lower than in past). If yes, adjusts all timers to current
 * time.
 */

void
hook_timer_check_system_clock ()
{
    time_t now;
    long diff_time;
    struct t_hook *ptr_hook;

    now = time (NULL);

    /*
     * check if difference with previous time is more than 10 seconds:
     * if it is, then consider it's clock skew and reinitialize all timers
     */
    diff_time = now - hook_last_system_time;
    if ((diff_time <= -10) || (diff_time >= 10))
    {
        if (weechat_debug_core >= 1)
        {
            gui_chat_printf (NULL,
                             _("System clock skew detected (%+ld seconds), "
                               "reinitializing all timers"),
                             diff_time);
        }

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
 * Sets time until next timeout.
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

    /*
     * to detect clock skew, we ensure there's a call to timers every
     * 2 seconds max
     */
    if (tv_timeout->tv_sec > 2)
    {
        tv_timeout->tv_sec = 2;
        tv_timeout->tv_usec = 0;
    }
}

/*
 * Executes timer hooks.
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
                (ptr_hook->callback_data,
                 (HOOK_TIMER(ptr_hook, remaining_calls) > 0) ?
                  HOOK_TIMER(ptr_hook, remaining_calls) - 1 : -1);
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
 * Searches for a fd hook in list.
 *
 * Returns pointer to hook found, NULL if not found.
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
 * Hooks a fd event.
 *
 * Returns pointer to new hook, NULL if error.
 */

struct t_hook *
hook_fd (struct t_weechat_plugin *plugin, int fd, int flag_read,
         int flag_write, int flag_exception,
         t_hook_callback_fd *callback, void *callback_data)
{
    struct t_hook *new_hook;
    struct t_hook_fd *new_hook_fd;

    if ((fd < 0) || hook_search_fd (fd) || !callback)
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
                    callback_data);

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
 * Fills sets according to fd hooked.
 *
 * Returns highest fd set.
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
            /* skip invalid file descriptors */
            if ((fcntl (HOOK_FD(ptr_hook,fd), F_GETFD) == -1)
                && (errno == EBADF))
            {
                if (HOOK_FD(ptr_hook, error) == 0)
                {
                    HOOK_FD(ptr_hook, error) = errno;
                    gui_chat_printf (NULL,
                                     _("%sError: bad file descriptor (%d) "
                                       "used in hook_fd"),
                                     gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                     HOOK_FD(ptr_hook, fd));
                }
            }
            else
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
    }

    return max_fd;
}

/*
 * Executes fd callbacks with sets.
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
            (void) (HOOK_FD(ptr_hook, callback)) (ptr_hook->callback_data,
                                                  HOOK_FD(ptr_hook, fd));
            ptr_hook->running = 0;
        }

        ptr_hook = next_hook;
    }

    hook_exec_end ();
}

/*
 * Hooks a process (using fork) with options in hashtable.
 *
 * Returns pointer to new hook, NULL if error.
 */

struct t_hook *
hook_process_hashtable (struct t_weechat_plugin *plugin,
                        const char *command, struct t_hashtable *options,
                        int timeout,
                        t_hook_callback_process *callback, void *callback_data)
{
    struct t_hook *new_hook;
    struct t_hook_process *new_hook_process;
    char *stdout_buffer, *stderr_buffer;

    if (!command || !command[0] || !callback)
        return NULL;

    stdout_buffer = malloc (HOOK_PROCESS_BUFFER_SIZE + 1);
    if (!stdout_buffer)
        return NULL;

    stderr_buffer = malloc (HOOK_PROCESS_BUFFER_SIZE + 1);
    if (!stderr_buffer)
    {
        free (stdout_buffer);
        return NULL;
    }

    new_hook = malloc (sizeof (*new_hook));
    if (!new_hook)
    {
        free (stdout_buffer);
        free (stderr_buffer);
        return NULL;
    }
    new_hook_process = malloc (sizeof (*new_hook_process));
    if (!new_hook_process)
    {
        free (stdout_buffer);
        free (stderr_buffer);
        free (new_hook);
        return NULL;
    }

    hook_init_data (new_hook, plugin, HOOK_TYPE_PROCESS, HOOK_PRIORITY_DEFAULT,
                    callback_data);

    new_hook->hook_data = new_hook_process;
    new_hook_process->callback = callback;
    new_hook_process->command = strdup (command);
    new_hook_process->options = (options) ? hashtable_dup (options) : NULL;
    new_hook_process->timeout = timeout;
    new_hook_process->child_read[HOOK_PROCESS_STDOUT] = -1;
    new_hook_process->child_read[HOOK_PROCESS_STDERR] = -1;
    new_hook_process->child_write[HOOK_PROCESS_STDOUT] = -1;
    new_hook_process->child_write[HOOK_PROCESS_STDERR] = -1;
    new_hook_process->child_pid = 0;
    new_hook_process->hook_fd[HOOK_PROCESS_STDOUT] = NULL;
    new_hook_process->hook_fd[HOOK_PROCESS_STDERR] = NULL;
    new_hook_process->hook_timer = NULL;
    new_hook_process->buffer[HOOK_PROCESS_STDOUT] = stdout_buffer;
    new_hook_process->buffer[HOOK_PROCESS_STDERR] = stderr_buffer;
    new_hook_process->buffer_size[HOOK_PROCESS_STDOUT] = 0;
    new_hook_process->buffer_size[HOOK_PROCESS_STDERR] = 0;

    hook_add_to_list (new_hook);

    hook_process_run (new_hook);

    return new_hook;
}

/*
 * Hooks a process (using fork).
 *
 * Returns pointer to new hook, NULL if error.
 */

struct t_hook *
hook_process (struct t_weechat_plugin *plugin,
              const char *command, int timeout,
              t_hook_callback_process *callback, void *callback_data)
{
    return hook_process_hashtable (plugin, command, NULL, timeout,
                                   callback, callback_data);
}

/*
 * Child process for hook process: executes command and returns string result
 * into pipe for WeeChat process.
 */

void
hook_process_child (struct t_hook *hook_process)
{
    char **exec_args, *arg0, str_arg[64];
    const char *ptr_url, *ptr_arg;
    int rc, i, num_args;

    /*
     * close stdin, so that process will fail to read stdin (process reading
     * stdin should not be run inside WeeChat!)
     */
    close (STDIN_FILENO);

    /* redirect stdout/stderr to pipe (so that father process can read them) */
    close (HOOK_PROCESS(hook_process, child_read[HOOK_PROCESS_STDOUT]));
    close (HOOK_PROCESS(hook_process, child_read[HOOK_PROCESS_STDERR]));
    if (dup2 (HOOK_PROCESS(hook_process, child_write[HOOK_PROCESS_STDOUT]),
              STDOUT_FILENO) < 0)
    {
        _exit (EXIT_FAILURE);
    }
    if (dup2 (HOOK_PROCESS(hook_process, child_write[HOOK_PROCESS_STDERR]),
              STDERR_FILENO) < 0)
    {
        _exit (EXIT_FAILURE);
    }

    rc = EXIT_SUCCESS;

    if (strncmp (HOOK_PROCESS(hook_process, command), "url:", 4) == 0)
    {
        /* get URL output (on stdout or file, depending on options) */
        ptr_url = HOOK_PROCESS(hook_process, command) + 4;
        while (ptr_url[0] == ' ')
        {
            ptr_url++;
        }
        rc = weeurl_download (ptr_url, HOOK_PROCESS(hook_process, options));
        if (rc != 0)
            fprintf (stderr, "Error with URL '%s'\n", ptr_url);
    }
    else
    {
        /* launch command */
        num_args = 0;
        if (HOOK_PROCESS(hook_process, options))
        {
            /*
             * count number of arguments given in the hashable options,
             * keys are: "arg1", "arg2", ...
             */
            while (1)
            {
                snprintf (str_arg, sizeof (str_arg), "arg%d", num_args + 1);
                ptr_arg = hashtable_get (HOOK_PROCESS(hook_process, options),
                                         str_arg);
                if (!ptr_arg)
                    break;
                num_args++;
            }
        }
        if (num_args > 0)
        {
            /*
             * if at least one argument was found in hashtable option, the
             * "command" contains only path to binary (without arguments), and
             * the arguments are in hashtable
             */
            exec_args = malloc ((num_args + 2) * sizeof (exec_args[0]));
            if (exec_args)
            {
                exec_args[0] = strdup (HOOK_PROCESS(hook_process, command));
                for (i = 1; i <= num_args; i++)
                {
                    snprintf (str_arg, sizeof (str_arg), "arg%d", i);
                    ptr_arg = hashtable_get (HOOK_PROCESS(hook_process, options),
                                             str_arg);
                    exec_args[i] = (ptr_arg) ? strdup (ptr_arg) : NULL;
                }
                exec_args[num_args + 1] = NULL;
            }
        }
        else
        {
            /*
             * if no arguments were found in hashtable, make an automatic split
             * of command, like the shell does
             */
            exec_args = string_split_shell (HOOK_PROCESS(hook_process, command));
        }

        if (exec_args)
        {
            arg0 = string_expand_home (exec_args[0]);
            if (arg0)
            {
                free (exec_args[0]);
                exec_args[0] = arg0;
            }
            if (weechat_debug_core >= 1)
            {
                log_printf ("hook_process, command='%s'",
                            HOOK_PROCESS(hook_process, command));
                for (i = 0; exec_args[i]; i++)
                {
                    log_printf ("  args[%02d] == '%s'", i, exec_args[i]);
                }
            }
            execvp (exec_args[0], exec_args);
        }

        /* should not be executed if execvp was ok */
        if (exec_args)
            string_free_split (exec_args);
        fprintf (stderr, "Error with command '%s'\n",
                 HOOK_PROCESS(hook_process, command));
        rc = EXIT_FAILURE;
    }

    fflush (stdout);
    fflush (stderr);

    _exit (rc);
}

/*
 * Sends buffers (stdout/stderr) to callback.
 */

void
hook_process_send_buffers (struct t_hook *hook_process, int callback_rc)
{
    int i, size;

    /* add '\0' at end of stdout and stderr */
    for (i = 0; i < 2; i++)
    {
        size = HOOK_PROCESS(hook_process, buffer_size[i]);
        if (size > 0)
            HOOK_PROCESS(hook_process, buffer[i])[size] = '\0';
    }

    /* send buffers to callback */
    (void) (HOOK_PROCESS(hook_process, callback))
        (hook_process->callback_data,
         HOOK_PROCESS(hook_process, command),
         callback_rc,
         (HOOK_PROCESS(hook_process, buffer_size[HOOK_PROCESS_STDOUT]) > 0) ?
         HOOK_PROCESS(hook_process, buffer[HOOK_PROCESS_STDOUT]) : NULL,
         (HOOK_PROCESS(hook_process, buffer_size[HOOK_PROCESS_STDERR]) > 0) ?
         HOOK_PROCESS(hook_process, buffer[HOOK_PROCESS_STDERR]) : NULL);

    /* reset size for stdout and stderr */
    HOOK_PROCESS(hook_process, buffer_size[HOOK_PROCESS_STDOUT]) = 0;
    HOOK_PROCESS(hook_process, buffer_size[HOOK_PROCESS_STDERR]) = 0;
}

/*
 * Adds some data to buffer (stdout or stderr).
 */

void
hook_process_add_to_buffer (struct t_hook *hook_process, int index_buffer,
                            const char *buffer, int size)
{
    if (HOOK_PROCESS(hook_process, buffer_size[index_buffer]) + size > HOOK_PROCESS_BUFFER_SIZE)
        hook_process_send_buffers (hook_process, WEECHAT_HOOK_PROCESS_RUNNING);

    memcpy (HOOK_PROCESS(hook_process, buffer[index_buffer]) +
            HOOK_PROCESS(hook_process, buffer_size[index_buffer]),
            buffer, size);
    HOOK_PROCESS(hook_process, buffer_size[index_buffer]) += size;
}

/*
 * Reads process output (stdout or stderr) from child process.
 */

void
hook_process_child_read (struct t_hook *hook_process, int fd,
                         int index_buffer, struct t_hook **hook_fd)
{
    char buffer[4096];
    int num_read;

    if (hook_process->deleted)
        return;

    num_read = read (fd, buffer, sizeof (buffer) - 1);
    if (num_read > 0)
    {
        hook_process_add_to_buffer (hook_process, index_buffer,
                                    buffer, num_read);
    }
    else if (num_read == 0)
    {
        unhook (*hook_fd);
        *hook_fd = NULL;
    }
}

/*
 * Reads process output (stdout) from child process.
 */

int
hook_process_child_read_stdout_cb (void *arg_hook_process, int fd)
{
    struct t_hook *hook_process;

    hook_process = (struct t_hook *)arg_hook_process;
    hook_process_child_read (hook_process, fd, HOOK_PROCESS_STDOUT,
                             &(HOOK_PROCESS(hook_process, hook_fd[HOOK_PROCESS_STDOUT])));
    return WEECHAT_RC_OK;
}

/*
 * Reads process output (stderr) from child process.
 */

int
hook_process_child_read_stderr_cb (void *arg_hook_process, int fd)
{
    struct t_hook *hook_process;

    hook_process = (struct t_hook *)arg_hook_process;
    hook_process_child_read (hook_process, fd, HOOK_PROCESS_STDERR,
                             &(HOOK_PROCESS(hook_process, hook_fd[HOOK_PROCESS_STDERR])));
    return WEECHAT_RC_OK;
}

/*
 * Checks if child process is still alive.
 */

int
hook_process_timer_cb (void *arg_hook_process, int remaining_calls)
{
    struct t_hook *hook_process;
    int status, rc;

    /* make C compiler happy */
    (void) remaining_calls;

    hook_process = (struct t_hook *)arg_hook_process;

    if (hook_process->deleted)
        return WEECHAT_RC_OK;

    if (remaining_calls == 0)
    {
        hook_process_send_buffers (hook_process, WEECHAT_HOOK_PROCESS_ERROR);
        if (weechat_debug_core >= 1)
        {
            gui_chat_printf (NULL,
                             _("End of command '%s', timeout reached (%.1fs)"),
                             HOOK_PROCESS(hook_process, command),
                             ((float)HOOK_PROCESS(hook_process, timeout)) / 1000);
        }
        kill (HOOK_PROCESS(hook_process, child_pid), SIGKILL);
        usleep (1000);
        unhook (hook_process);
    }
    else
    {
        if (!HOOK_PROCESS(hook_process, hook_fd[HOOK_PROCESS_STDOUT])
            && !HOOK_PROCESS(hook_process, hook_fd[HOOK_PROCESS_STDERR]))
        {
            if (waitpid (HOOK_PROCESS(hook_process, child_pid), &status, WNOHANG) > 0)
            {
                rc = WEXITSTATUS(status);
                hook_process_send_buffers (hook_process, rc);
                unhook (hook_process);
            }
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Executes process command in child, and read data in current process,
 * with fd hook.
 */

void
hook_process_run (struct t_hook *hook_process)
{
    int pipe_stdout[2], pipe_stderr[2], timeout, max_calls;
    long interval;
    pid_t pid;

    /* create pipe for child process (stdout) */
    if (pipe (pipe_stdout) < 0)
    {
        (void) (HOOK_PROCESS(hook_process, callback))
            (hook_process->callback_data,
             HOOK_PROCESS(hook_process, command),
             WEECHAT_HOOK_PROCESS_ERROR,
             NULL, NULL);
        unhook (hook_process);
        return;
    }
    if (pipe (pipe_stderr) < 0)
    {
        close (pipe_stdout[0]);
        close (pipe_stdout[1]);
        (void) (HOOK_PROCESS(hook_process, callback))
            (hook_process->callback_data,
             HOOK_PROCESS(hook_process, command),
             WEECHAT_HOOK_PROCESS_ERROR,
             NULL, NULL);
        unhook (hook_process);
        return;
    }

    HOOK_PROCESS(hook_process, child_read[HOOK_PROCESS_STDOUT]) = pipe_stdout[0];
    HOOK_PROCESS(hook_process, child_write[HOOK_PROCESS_STDOUT]) = pipe_stdout[1];

    HOOK_PROCESS(hook_process, child_read[HOOK_PROCESS_STDERR]) = pipe_stderr[0];
    HOOK_PROCESS(hook_process, child_write[HOOK_PROCESS_STDERR]) = pipe_stderr[1];

    switch (pid = fork ())
    {
        /* fork failed */
        case -1:
            (void) (HOOK_PROCESS(hook_process, callback))
                (hook_process->callback_data,
                 HOOK_PROCESS(hook_process, command),
                 WEECHAT_HOOK_PROCESS_ERROR,
                 NULL, NULL);
            unhook (hook_process);
            return;
        /* child process */
        case 0:
            setuid (getuid ());
            hook_process_child (hook_process);
            /* never executed */
            _exit (EXIT_SUCCESS);
            break;
    }
    /* parent process */
    HOOK_PROCESS(hook_process, child_pid) = pid;
    close (HOOK_PROCESS(hook_process, child_write[HOOK_PROCESS_STDOUT]));
    HOOK_PROCESS(hook_process, child_write[HOOK_PROCESS_STDOUT]) = -1;
    close (HOOK_PROCESS(hook_process, child_write[HOOK_PROCESS_STDERR]));
    HOOK_PROCESS(hook_process, child_write[HOOK_PROCESS_STDERR]) = -1;

    HOOK_PROCESS(hook_process, hook_fd[HOOK_PROCESS_STDOUT]) =
        hook_fd (hook_process->plugin,
                 HOOK_PROCESS(hook_process, child_read[HOOK_PROCESS_STDOUT]),
                 1, 0, 0,
                 &hook_process_child_read_stdout_cb,
                 hook_process);

    HOOK_PROCESS(hook_process, hook_fd[HOOK_PROCESS_STDERR]) =
        hook_fd (hook_process->plugin,
                 HOOK_PROCESS(hook_process, child_read[HOOK_PROCESS_STDERR]),
                 1, 0, 0,
                 &hook_process_child_read_stderr_cb,
                 hook_process);

    timeout = HOOK_PROCESS(hook_process, timeout);
    interval = 100;
    max_calls = 0;
    if (timeout > 0)
    {
        if (timeout <= 100)
        {
            interval = timeout;
            max_calls = 1;
        }
        else
        {
            interval = 100;
            max_calls = timeout / 100;
            if (timeout % 100 == 0)
                max_calls++;
        }
    }
    HOOK_PROCESS(hook_process, hook_timer) = hook_timer (hook_process->plugin,
                                                         interval, 0, max_calls,
                                                         &hook_process_timer_cb,
                                                         hook_process);
}

/*
 * Hooks a connection to a peer (using fork).
 *
 * Returns pointer to new hook, NULL if error.
 */

struct t_hook *
hook_connect (struct t_weechat_plugin *plugin, const char *proxy,
              const char *address, int port, int ipv6, int retry,
              void *gnutls_sess, void *gnutls_cb, int gnutls_dhkey_size,
              const char *gnutls_priorities, const char *local_hostname,
              t_hook_callback_connect *callback, void *callback_data)
{
    struct t_hook *new_hook;
    struct t_hook_connect *new_hook_connect;
#ifdef HOOK_CONNECT_MAX_SOCKETS
    int i;
#endif

#ifndef HAVE_GNUTLS
    /* make C compiler happy */
    (void) gnutls_sess;
    (void) gnutls_cb;
    (void) gnutls_dhkey_size;
    (void) gnutls_priorities;
#endif

    if (!address || (port <= 0) || !callback)
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

    hook_init_data (new_hook, plugin, HOOK_TYPE_CONNECT, HOOK_PRIORITY_DEFAULT,
                    callback_data);

    new_hook->hook_data = new_hook_connect;
    new_hook_connect->callback = callback;
    new_hook_connect->proxy = (proxy) ? strdup (proxy) : NULL;
    new_hook_connect->address = strdup (address);
    new_hook_connect->port = port;
    new_hook_connect->sock = -1;
    new_hook_connect->ipv6 = ipv6;
    new_hook_connect->retry = retry;
#ifdef HAVE_GNUTLS
    new_hook_connect->gnutls_sess = gnutls_sess;
    new_hook_connect->gnutls_cb = gnutls_cb;
    new_hook_connect->gnutls_dhkey_size = gnutls_dhkey_size;
    new_hook_connect->gnutls_priorities = (gnutls_priorities) ?
        strdup (gnutls_priorities) : NULL;
#endif
    new_hook_connect->local_hostname = (local_hostname) ?
        strdup (local_hostname) : NULL;
    new_hook_connect->child_read = -1;
    new_hook_connect->child_write = -1;
    new_hook_connect->child_recv = -1;
    new_hook_connect->child_send = -1;
    new_hook_connect->child_pid = 0;
    new_hook_connect->hook_child_timer = NULL;
    new_hook_connect->hook_fd = NULL;
    new_hook_connect->handshake_hook_fd = NULL;
    new_hook_connect->handshake_hook_timer = NULL;
    new_hook_connect->handshake_fd_flags = 0;
    new_hook_connect->handshake_ip_address = NULL;
#ifdef HOOK_CONNECT_MAX_SOCKETS
    for (i = 0; i < HOOK_CONNECT_MAX_SOCKETS; i++)
    {
        new_hook_connect->sock_v4[i] = -1;
        new_hook_connect->sock_v6[i] = -1;
    }
#endif

    hook_add_to_list (new_hook);

    network_connect_with_fork (new_hook);

    return new_hook;
}

/*
 * Verifies certificates.
 */

#ifdef HAVE_GNUTLS
int
hook_connect_gnutls_verify_certificates (gnutls_session_t tls_session)
{
    struct t_hook *ptr_hook;
    int rc;

    rc = -1;
    ptr_hook = weechat_hooks[HOOK_TYPE_CONNECT];
    while (ptr_hook)
    {
        /* looking for the right hook using to the gnutls session pointer */
        if (!ptr_hook->deleted
            && HOOK_CONNECT(ptr_hook, gnutls_sess)
            && (*(HOOK_CONNECT(ptr_hook, gnutls_sess)) == tls_session))
        {
            rc = (int) (HOOK_CONNECT(ptr_hook, gnutls_cb))
                (ptr_hook->callback_data, tls_session, NULL, 0,
                 NULL, 0, NULL,
                 WEECHAT_HOOK_CONNECT_GNUTLS_CB_VERIFY_CERT);
            break;
        }
        ptr_hook = ptr_hook->next_hook;
    }

    return rc;
}
#endif

/*
 * Sets certificates.
 */

#ifdef HAVE_GNUTLS
int
hook_connect_gnutls_set_certificates (gnutls_session_t tls_session,
                                      const gnutls_datum_t *req_ca, int nreq,
                                      const gnutls_pk_algorithm_t *pk_algos,
                                      int pk_algos_len,
#if LIBGNUTLS_VERSION_NUMBER >= 0x020b00
                                      gnutls_retr2_st *answer)
#else
                                      gnutls_retr_st *answer)
#endif
{
    struct t_hook *ptr_hook;
    int rc;

    rc = -1;
    ptr_hook = weechat_hooks[HOOK_TYPE_CONNECT];
    while (ptr_hook)
    {
        /* looking for the right hook using to the gnutls session pointer */
        if (!ptr_hook->deleted
            && HOOK_CONNECT(ptr_hook, gnutls_sess)
            && (*(HOOK_CONNECT(ptr_hook, gnutls_sess)) == tls_session))
        {
            rc = (int) (HOOK_CONNECT(ptr_hook, gnutls_cb))
                (ptr_hook->callback_data, tls_session, req_ca, nreq,
                 pk_algos, pk_algos_len, answer,
                 WEECHAT_HOOK_CONNECT_GNUTLS_CB_SET_CERT);
            break;
        }
        ptr_hook = ptr_hook->next_hook;
    }

    return rc;
}
#endif

/*
 * Hooks a message printed by WeeChat.
 *
 * Returns pointer to new hook, NULL if error.
 */

struct t_hook *
hook_print (struct t_weechat_plugin *plugin, struct t_gui_buffer *buffer,
            const char *tags, const char *message, int strip_colors,
            t_hook_callback_print *callback, void *callback_data)
{
    struct t_hook *new_hook;
    struct t_hook_print *new_hook_print;

    if (!callback)
        return NULL;

    new_hook = malloc (sizeof (*new_hook));
    if (!new_hook)
        return NULL;
    new_hook_print = malloc (sizeof (*new_hook_print));
    if (!new_hook_print)
    {
        free (new_hook);
        return NULL;
    }

    hook_init_data (new_hook, plugin, HOOK_TYPE_PRINT, HOOK_PRIORITY_DEFAULT,
                    callback_data);

    new_hook->hook_data = new_hook_print;
    new_hook_print->callback = callback;
    new_hook_print->buffer = buffer;
    if (tags)
    {
        new_hook_print->tags_array = string_split (tags, ",", 0, 0,
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
 * Executes a print hook.
 */

void
hook_print_exec (struct t_gui_buffer *buffer, struct t_gui_line *line)
{
    struct t_hook *ptr_hook, *next_hook;
    char *prefix_no_color, *message_no_color;
    int tags_match, tag_found, i, j;

    if (!line->data->message || !line->data->message[0])
        return;

    prefix_no_color = (line->data->prefix) ?
        gui_color_decode (line->data->prefix, NULL) : NULL;

    message_no_color = gui_color_decode (line->data->message, NULL);
    if (!message_no_color)
    {
        if (prefix_no_color)
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
                || !HOOK_PRINT(ptr_hook, message)[0]
                || string_strcasestr (prefix_no_color, HOOK_PRINT(ptr_hook, message))
                || string_strcasestr (message_no_color, HOOK_PRINT(ptr_hook, message))))
        {
            /* check if tags match */
            if (HOOK_PRINT(ptr_hook, tags_array))
            {
                /* if there are tags in message printed */
                if (line->data->tags_array)
                {
                    tags_match = 1;
                    for (i = 0; i < HOOK_PRINT(ptr_hook, tags_count); i++)
                    {
                        /* search for tag in message */
                        tag_found = 0;
                        for (j = 0; j < line->data->tags_count; j++)
                        {
                            if (string_strcasecmp (HOOK_PRINT(ptr_hook, tags_array)[i],
                                                   line->data->tags_array[j]) == 0)
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
                    (ptr_hook->callback_data, buffer, line->data->date,
                     line->data->tags_count,
                     (const char **)line->data->tags_array,
                     (int)line->data->displayed, (int)line->data->highlight,
                     (HOOK_PRINT(ptr_hook, strip_colors)) ? prefix_no_color : line->data->prefix,
                     (HOOK_PRINT(ptr_hook, strip_colors)) ? message_no_color : line->data->message);
                ptr_hook->running = 0;
            }
        }

        ptr_hook = next_hook;
    }

    if (prefix_no_color)
        free (prefix_no_color);
    if (message_no_color)
        free (message_no_color);

    hook_exec_end ();
}

/*
 * Hooks a signal.
 *
 * Returns pointer to new hook, NULL if error.
 */

struct t_hook *
hook_signal (struct t_weechat_plugin *plugin, const char *signal,
             t_hook_callback_signal *callback, void *callback_data)
{
    struct t_hook *new_hook;
    struct t_hook_signal *new_hook_signal;
    int priority;
    const char *ptr_signal;

    if (!signal || !signal[0] || !callback)
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

    hook_get_priority_and_name (signal, &priority, &ptr_signal);
    hook_init_data (new_hook, plugin, HOOK_TYPE_SIGNAL, priority,
                    callback_data);

    new_hook->hook_data = new_hook_signal;
    new_hook_signal->callback = callback;
    new_hook_signal->signal = strdup ((ptr_signal) ? ptr_signal : signal);

    hook_add_to_list (new_hook);

    return new_hook;
}

/*
 * Sends a signal.
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
 * Hooks a hsignal (signal with hashtable).
 *
 * Returns pointer to new hook, NULL if error.
 */

struct t_hook *
hook_hsignal (struct t_weechat_plugin *plugin, const char *signal,
              t_hook_callback_hsignal *callback, void *callback_data)
{
    struct t_hook *new_hook;
    struct t_hook_hsignal *new_hook_hsignal;
    int priority;
    const char *ptr_signal;

    if (!signal || !signal[0] || !callback)
        return NULL;

    new_hook = malloc (sizeof (*new_hook));
    if (!new_hook)
        return NULL;
    new_hook_hsignal = malloc (sizeof (*new_hook_hsignal));
    if (!new_hook_hsignal)
    {
        free (new_hook);
        return NULL;
    }

    hook_get_priority_and_name (signal, &priority, &ptr_signal);
    hook_init_data (new_hook, plugin, HOOK_TYPE_HSIGNAL, priority,
                    callback_data);

    new_hook->hook_data = new_hook_hsignal;
    new_hook_hsignal->callback = callback;
    new_hook_hsignal->signal = strdup ((ptr_signal) ? ptr_signal : signal);

    hook_add_to_list (new_hook);

    return new_hook;
}

/*
 * Sends a hsignal (signal with hashtable).
 */

void
hook_hsignal_send (const char *signal, struct t_hashtable *hashtable)
{
    struct t_hook *ptr_hook, *next_hook;

    hook_exec_start ();

    ptr_hook = weechat_hooks[HOOK_TYPE_HSIGNAL];
    while (ptr_hook)
    {
        next_hook = ptr_hook->next_hook;

        if (!ptr_hook->deleted
            && !ptr_hook->running
            && (string_match (signal, HOOK_HSIGNAL(ptr_hook, signal), 0)))
        {
            ptr_hook->running = 1;
            (void) (HOOK_HSIGNAL(ptr_hook, callback))
                (ptr_hook->callback_data, signal, hashtable);
            ptr_hook->running = 0;
        }

        ptr_hook = next_hook;
    }

    hook_exec_end ();
}

/*
 * Hooks a configuration option.
 *
 * Returns pointer to new hook, NULL if error.
 */

struct t_hook *
hook_config (struct t_weechat_plugin *plugin, const char *option,
             t_hook_callback_config *callback, void *callback_data)
{
    struct t_hook *new_hook;
    struct t_hook_config *new_hook_config;
    int priority;
    const char *ptr_option;

    if (!callback)
        return NULL;

    new_hook = malloc (sizeof (*new_hook));
    if (!new_hook)
        return NULL;
    new_hook_config = malloc (sizeof (*new_hook_config));
    if (!new_hook_config)
    {
        free (new_hook);
        return NULL;
    }

    hook_get_priority_and_name (option, &priority, &ptr_option);
    hook_init_data (new_hook, plugin, HOOK_TYPE_CONFIG, priority,
                    callback_data);

    new_hook->hook_data = new_hook_config;
    new_hook_config->callback = callback;
    new_hook_config->option = strdup ((ptr_option) ? ptr_option :
                                      ((option) ? option : ""));

    hook_add_to_list (new_hook);

    return new_hook;
}

/*
 * Executes a config hook.
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
 * Hooks a completion.
 *
 * Returns pointer to new hook, NULL if error.
 */

struct t_hook *
hook_completion (struct t_weechat_plugin *plugin, const char *completion_item,
                 const char *description,
                 t_hook_callback_completion *callback, void *callback_data)
{
    struct t_hook *new_hook;
    struct t_hook_completion *new_hook_completion;
    int priority;
    const char *ptr_completion_item;

    if (!completion_item || !completion_item[0]
        || strchr (completion_item, ' ') || !callback)
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

    hook_get_priority_and_name (completion_item, &priority, &ptr_completion_item);
    hook_init_data (new_hook, plugin, HOOK_TYPE_COMPLETION, priority,
                    callback_data);

    new_hook->hook_data = new_hook_completion;
    new_hook_completion->callback = callback;
    new_hook_completion->completion_item = strdup ((ptr_completion_item) ?
                                                   ptr_completion_item : completion_item);
    new_hook_completion->description = strdup ((description) ?
                                               description : "");

    hook_add_to_list (new_hook);

    return new_hook;
}

/*
 * Gets a completion property as string.
 */

const char *
hook_completion_get_string (struct t_gui_completion *completion,
                            const char *property)
{
    return gui_completion_get_string (completion, property);
}

/*
 * Adds a word for a completion.
 */

void
hook_completion_list_add (struct t_gui_completion *completion,
                          const char *word, int nick_completion,
                          const char *where)
{
    gui_completion_list_add (completion, word, nick_completion, where);
}

/*
 * Executes a completion hook.
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
 * Hooks a modifier.
 *
 * Returns pointer to new hook, NULL if error.
 */

struct t_hook *
hook_modifier (struct t_weechat_plugin *plugin, const char *modifier,
               t_hook_callback_modifier *callback, void *callback_data)
{
    struct t_hook *new_hook;
    struct t_hook_modifier *new_hook_modifier;
    int priority;
    const char *ptr_modifier;

    if (!modifier || !modifier[0] || !callback)
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

    hook_get_priority_and_name (modifier, &priority, &ptr_modifier);
    hook_init_data (new_hook, plugin, HOOK_TYPE_MODIFIER, priority,
                    callback_data);

    new_hook->hook_data = new_hook_modifier;
    new_hook_modifier->callback = callback;
    new_hook_modifier->modifier = strdup ((ptr_modifier) ? ptr_modifier : modifier);

    hook_add_to_list (new_hook);

    return new_hook;
}

/*
 * Executes a modifier hook.
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
 * Hooks an info.
 *
 * Returns pointer to new hook, NULL if error.
 */

struct t_hook *
hook_info (struct t_weechat_plugin *plugin, const char *info_name,
           const char *description, const char *args_description,
           t_hook_callback_info *callback, void *callback_data)
{
    struct t_hook *new_hook;
    struct t_hook_info *new_hook_info;
    int priority;
    const char *ptr_info_name;

    if (!info_name || !info_name[0] || !callback)
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

    hook_get_priority_and_name (info_name, &priority, &ptr_info_name);
    hook_init_data (new_hook, plugin, HOOK_TYPE_INFO, priority, callback_data);

    new_hook->hook_data = new_hook_info;
    new_hook_info->callback = callback;
    new_hook_info->info_name = strdup ((ptr_info_name) ?
                                       ptr_info_name : info_name);
    new_hook_info->description = strdup ((description) ? description : "");
    new_hook_info->args_description = strdup ((args_description) ?
                                              args_description : "");

    hook_add_to_list (new_hook);

    return new_hook;
}

/*
 * Gets info (as string) via info hook.
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
 * Hooks an info using hashtable.
 *
 * Returns pointer to new hook, NULL if error.
 */

struct t_hook *
hook_info_hashtable (struct t_weechat_plugin *plugin, const char *info_name,
                     const char *description, const char *args_description,
                     const char *output_description,
                     t_hook_callback_info_hashtable *callback,
                     void *callback_data)
{
    struct t_hook *new_hook;
    struct t_hook_info_hashtable *new_hook_info_hashtable;
    int priority;
    const char *ptr_info_name;

    if (!info_name || !info_name[0] || !callback)
        return NULL;

    new_hook = malloc (sizeof (*new_hook));
    if (!new_hook)
        return NULL;
    new_hook_info_hashtable = malloc (sizeof (*new_hook_info_hashtable));
    if (!new_hook_info_hashtable)
    {
        free (new_hook);
        return NULL;
    }

    hook_get_priority_and_name (info_name, &priority, &ptr_info_name);
    hook_init_data (new_hook, plugin, HOOK_TYPE_INFO_HASHTABLE, priority,
                    callback_data);

    new_hook->hook_data = new_hook_info_hashtable;
    new_hook_info_hashtable->callback = callback;
    new_hook_info_hashtable->info_name = strdup ((ptr_info_name) ?
                                                 ptr_info_name : info_name);
    new_hook_info_hashtable->description = strdup ((description) ? description : "");
    new_hook_info_hashtable->args_description = strdup ((args_description) ?
                                                        args_description : "");
    new_hook_info_hashtable->output_description = strdup ((output_description) ?
                                                          output_description : "");

    hook_add_to_list (new_hook);

    return new_hook;
}

/*
 * Gets info (as hashtable) via info hook.
 */

struct t_hashtable *
hook_info_get_hashtable (struct t_weechat_plugin *plugin, const char *info_name,
                         struct t_hashtable *hashtable)
{
    struct t_hook *ptr_hook, *next_hook;
    struct t_hashtable *value;

    /* make C compiler happy */
    (void) plugin;

    if (!info_name || !info_name[0])
        return NULL;

    hook_exec_start ();

    ptr_hook = weechat_hooks[HOOK_TYPE_INFO_HASHTABLE];
    while (ptr_hook)
    {
        next_hook = ptr_hook->next_hook;

        if (!ptr_hook->deleted
            && !ptr_hook->running
            && (string_strcasecmp (HOOK_INFO_HASHTABLE(ptr_hook, info_name),
                                   info_name) == 0))
        {
            ptr_hook->running = 1;
            value = (HOOK_INFO_HASHTABLE(ptr_hook, callback))
                (ptr_hook->callback_data, info_name, hashtable);
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
 * Hooks an infolist.
 *
 * Returns pointer to new hook, NULL if error.
 */

struct t_hook *
hook_infolist (struct t_weechat_plugin *plugin, const char *infolist_name,
               const char *description, const char *pointer_description,
               const char *args_description,
               t_hook_callback_infolist *callback, void *callback_data)
{
    struct t_hook *new_hook;
    struct t_hook_infolist *new_hook_infolist;
    int priority;
    const char *ptr_infolist_name;

    if (!infolist_name || !infolist_name[0] || !callback)
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

    hook_get_priority_and_name (infolist_name, &priority, &ptr_infolist_name);
    hook_init_data (new_hook, plugin, HOOK_TYPE_INFOLIST, priority,
                    callback_data);

    new_hook->hook_data = new_hook_infolist;
    new_hook_infolist->callback = callback;
    new_hook_infolist->infolist_name = strdup ((ptr_infolist_name) ?
                                               ptr_infolist_name : infolist_name);
    new_hook_infolist->description = strdup ((description) ? description : "");
    new_hook_infolist->pointer_description = strdup ((pointer_description) ?
                                                     pointer_description : "");
    new_hook_infolist->args_description = strdup ((args_description) ?
                                                  args_description : "");

    hook_add_to_list (new_hook);

    return new_hook;
}

/*
 * Gets an infolist via infolist hook.
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
 * Hooks a hdata.
 *
 * Returns pointer to new hook, NULL if error.
 */

struct t_hook *
hook_hdata (struct t_weechat_plugin *plugin, const char *hdata_name,
            const char *description,
            t_hook_callback_hdata *callback, void *callback_data)
{
    struct t_hook *new_hook;
    struct t_hook_hdata *new_hook_hdata;
    int priority;
    const char *ptr_hdata_name;

    if (!hdata_name || !hdata_name[0] || !callback)
        return NULL;

    new_hook = malloc (sizeof (*new_hook));
    if (!new_hook)
        return NULL;
    new_hook_hdata = malloc (sizeof (*new_hook_hdata));
    if (!new_hook_hdata)
    {
        free (new_hook);
        return NULL;
    }

    hook_get_priority_and_name (hdata_name, &priority, &ptr_hdata_name);
    hook_init_data (new_hook, plugin, HOOK_TYPE_HDATA, priority,
                    callback_data);

    new_hook->hook_data = new_hook_hdata;
    new_hook_hdata->callback = callback;
    new_hook_hdata->hdata_name = strdup ((ptr_hdata_name) ?
                                         ptr_hdata_name : hdata_name);
    new_hook_hdata->description = strdup ((description) ? description : "");

    hook_add_to_list (new_hook);

    return new_hook;
}

/*
 * Gets hdata via hdata hook.
 */

struct t_hdata *
hook_hdata_get (struct t_weechat_plugin *plugin, const char *hdata_name)
{
    struct t_hook *ptr_hook, *next_hook;
    struct t_hdata *value;

    /* make C compiler happy */
    (void) plugin;

    if (!hdata_name || !hdata_name[0])
        return NULL;

    if (weechat_hdata)
    {
        value = hashtable_get (weechat_hdata, hdata_name);
        if (value)
            return value;
    }

    hook_exec_start ();

    ptr_hook = weechat_hooks[HOOK_TYPE_HDATA];
    while (ptr_hook)
    {
        next_hook = ptr_hook->next_hook;

        if (!ptr_hook->deleted
            && !ptr_hook->running
            && (strcmp (HOOK_HDATA(ptr_hook, hdata_name), hdata_name) == 0))
        {
            ptr_hook->running = 1;
            value = (HOOK_HDATA(ptr_hook, callback))
                (ptr_hook->callback_data,
                 HOOK_HDATA(ptr_hook, hdata_name));
            ptr_hook->running = 0;

            hook_exec_end ();
            return value;
        }

        ptr_hook = next_hook;
    }

    hook_exec_end ();

    /* hdata not found */
    return NULL;
}

/*
 * Hooks a focus.
 *
 * Returns pointer to new hook, NULL if error.
 */

struct t_hook *
hook_focus (struct t_weechat_plugin *plugin, const char *area,
            t_hook_callback_focus *callback, void *callback_data)
{
    struct t_hook *new_hook;
    struct t_hook_focus *new_hook_focus;
    int priority;
    const char *ptr_area;

    if (!area || !area[0] || !callback)
        return NULL;

    new_hook = malloc (sizeof (*new_hook));
    if (!new_hook)
        return NULL;
    new_hook_focus = malloc (sizeof (*new_hook_focus));
    if (!new_hook_focus)
    {
        free (new_hook);
        return NULL;
    }

    hook_get_priority_and_name (area, &priority, &ptr_area);
    hook_init_data (new_hook, plugin, HOOK_TYPE_FOCUS, priority,
                    callback_data);

    new_hook->hook_data = new_hook_focus;
    new_hook_focus->callback = callback;
    new_hook_focus->area = strdup ((ptr_area) ? ptr_area : area);

    hook_add_to_list (new_hook);

    return new_hook;
}

/*
 * Adds keys of a hashtable into another.
 */

void
hook_focus_hashtable_map_cb (void *data, struct t_hashtable *hashtable,
                             const void *key, const void *value)
{
    struct t_hashtable *hashtable1;

    /* make C compiler happy */
    (void) hashtable;

    hashtable1 = (struct t_hashtable *)data;

    if (hashtable1 && key && value)
        hashtable_set (hashtable1, (const char *)key, (const char *)value);
}

/*
 * Adds keys of a hashtable into another (adding suffix "2" to keys).
 */

void
hook_focus_hashtable_map2_cb (void *data, struct t_hashtable *hashtable,
                              const void *key, const void *value)
{
    struct t_hashtable *hashtable1;
    int length;
    char *key2;

    /* make C compiler happy */
    (void) hashtable;

    hashtable1 = (struct t_hashtable *)data;

    length = strlen ((const char *)key) + 1 + 1;
    key2 = malloc (length);
    if (key2)
    {
        snprintf (key2, length, "%s2", (const char *)key);
        if (hashtable1 && key && value)
            hashtable_set (hashtable1, key2, (const char *)value);
        free (key2);
    }
}

/*
 * Gets data for focus on (x,y) on screen.
 *
 * Argument hashtable_focus2 is not NULL only for a mouse gesture (it's for
 * point where mouse button has been released).
 */

struct t_hashtable *
hook_focus_get_data (struct t_hashtable *hashtable_focus1,
                     struct t_hashtable *hashtable_focus2)
{
    struct t_hook *ptr_hook, *next_hook;
    struct t_hashtable *hashtable1, *hashtable2, *hashtable_ret;
    const char *focus1_chat, *focus1_bar_item_name, *keys;
    char **list_keys, *new_key;
    int num_keys, i, length, focus1_is_chat;

    if (!hashtable_focus1)
        return NULL;

    focus1_chat = hashtable_get (hashtable_focus1, "_chat");
    focus1_is_chat = (focus1_chat && (strcmp (focus1_chat, "1") == 0));
    focus1_bar_item_name = hashtable_get (hashtable_focus1, "_bar_item_name");

    hashtable1 = hashtable_dup (hashtable_focus1);
    if (!hashtable1)
        return NULL;
    hashtable2 = (hashtable_focus2) ? hashtable_dup (hashtable_focus2) : NULL;

    hook_exec_start ();

    ptr_hook = weechat_hooks[HOOK_TYPE_FOCUS];
    while (ptr_hook)
    {
        next_hook = ptr_hook->next_hook;

        if (!ptr_hook->deleted
            && !ptr_hook->running
            && ((focus1_is_chat
                 && (strcmp (HOOK_FOCUS(ptr_hook, area), "chat") == 0))
                || (focus1_bar_item_name && focus1_bar_item_name[0]
                    && (strcmp (HOOK_FOCUS(ptr_hook, area), focus1_bar_item_name) == 0))))
        {
            /* run callback for focus #1 */
            ptr_hook->running = 1;
            hashtable_ret = (HOOK_FOCUS(ptr_hook, callback))
                (ptr_hook->callback_data, hashtable1);
            ptr_hook->running = 0;
            if (hashtable_ret)
            {
                if (hashtable_ret != hashtable1)
                {
                    /*
                     * add keys of hashtable_ret into hashtable1
                     * and destroy it
                     */
                    hashtable_map (hashtable_ret,
                                   &hook_focus_hashtable_map_cb,
                                   hashtable1);
                    hashtable_free (hashtable_ret);
                }
            }

            /* run callback for focus #2 */
            if (hashtable2)
            {
                ptr_hook->running = 1;
                hashtable_ret = (HOOK_FOCUS(ptr_hook, callback))
                    (ptr_hook->callback_data, hashtable2);
                ptr_hook->running = 0;
                if (hashtable_ret)
                {
                    if (hashtable_ret != hashtable2)
                    {
                        /*
                         * add keys of hashtable_ret into hashtable2
                         * and destroy it
                         */
                        hashtable_map (hashtable_ret,
                                       &hook_focus_hashtable_map_cb,
                                       hashtable2);
                        hashtable_free (hashtable_ret);
                    }
                }
            }
        }

        ptr_hook = next_hook;
    }

    if (hashtable2)
    {
        hashtable_map (hashtable2, &hook_focus_hashtable_map2_cb, hashtable1);
        hashtable_free (hashtable2);
    }
    else
    {
        keys = hashtable_get_string (hashtable1, "keys");
        if (keys)
        {
            list_keys = string_split (keys, ",", 0, 0, &num_keys);
            if (list_keys)
            {
                for (i = 0; i < num_keys; i++)
                {
                    length = strlen (list_keys[i]) + 1 + 1;
                    new_key = malloc (length);
                    if (new_key)
                    {
                        snprintf (new_key, length, "%s2", list_keys[i]);
                        hashtable_set (hashtable1, new_key,
                                       hashtable_get (hashtable1,
                                                      list_keys[i]));
                        free (new_key);
                    }
                }
                string_free_split (list_keys);
            }
        }
    }

    hook_exec_end ();

    return hashtable1;
}

/*
 * Sets a hook property (string).
 */

void
hook_set (struct t_hook *hook, const char *property, const char *value)
{
    if (string_strcasecmp (property, "subplugin") == 0)
    {
        if (hook->subplugin)
            free(hook->subplugin);
        hook->subplugin = strdup (value);
    }
}

/*
 * Unhooks something.
 */

void
unhook (struct t_hook *hook)
{
    int i;

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
    if (hook->subplugin)
        free (hook->subplugin);
    if (hook->hook_data)
    {
        switch (hook->type)
        {
            case HOOK_TYPE_COMMAND:
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
                if (HOOK_COMMAND(hook, cplt_templates))
                {
                    for (i = 0; i < HOOK_COMMAND(hook, cplt_num_templates); i++)
                    {
                        if (HOOK_COMMAND(hook, cplt_templates)[i])
                            free (HOOK_COMMAND(hook, cplt_templates)[i]);
                        if (HOOK_COMMAND(hook, cplt_templates_static)[i])
                            free (HOOK_COMMAND(hook, cplt_templates_static)[i]);
                        string_free_split (HOOK_COMMAND(hook, cplt_template_args)[i]);
                    }
                    free (HOOK_COMMAND(hook, cplt_templates));
                }
                if (HOOK_COMMAND(hook, cplt_templates_static))
                    free (HOOK_COMMAND(hook, cplt_templates_static));
                if (HOOK_COMMAND(hook, cplt_template_num_args))
                    free (HOOK_COMMAND(hook, cplt_template_num_args));
                if (HOOK_COMMAND(hook, cplt_template_args))
                    free (HOOK_COMMAND(hook, cplt_template_args));
                if (HOOK_COMMAND(hook, cplt_template_args_concat))
                {
                    for (i = 0;
                         i < HOOK_COMMAND(hook, cplt_template_num_args_concat);
                         i++)
                    {
                        free (HOOK_COMMAND(hook, cplt_template_args_concat[i]));
                    }
                    free (HOOK_COMMAND(hook, cplt_template_args_concat));
                }
                break;
            case HOOK_TYPE_COMMAND_RUN:
                if (HOOK_COMMAND_RUN(hook, command))
                    free (HOOK_COMMAND_RUN(hook, command));
                break;
            case HOOK_TYPE_TIMER:
                break;
            case HOOK_TYPE_FD:
                break;
            case HOOK_TYPE_PROCESS:
                if (HOOK_PROCESS(hook, command))
                    free (HOOK_PROCESS(hook, command));
                if (HOOK_PROCESS(hook, options))
                    hashtable_free (HOOK_PROCESS(hook, options));
                if (HOOK_PROCESS(hook, hook_fd[HOOK_PROCESS_STDOUT]))
                    unhook (HOOK_PROCESS(hook, hook_fd[HOOK_PROCESS_STDOUT]));
                if (HOOK_PROCESS(hook, hook_fd[HOOK_PROCESS_STDERR]))
                    unhook (HOOK_PROCESS(hook, hook_fd[HOOK_PROCESS_STDERR]));
                if (HOOK_PROCESS(hook, hook_timer))
                    unhook (HOOK_PROCESS(hook, hook_timer));
                if (HOOK_PROCESS(hook, child_pid) > 0)
                {
                    kill (HOOK_PROCESS(hook, child_pid), SIGKILL);
                    waitpid (HOOK_PROCESS(hook, child_pid), NULL, 0);
                }
                if (HOOK_PROCESS(hook, child_read[HOOK_PROCESS_STDOUT]) != -1)
                    close (HOOK_PROCESS(hook, child_read[HOOK_PROCESS_STDOUT]));
                if (HOOK_PROCESS(hook, child_write[HOOK_PROCESS_STDOUT]) != -1)
                    close (HOOK_PROCESS(hook, child_write[HOOK_PROCESS_STDOUT]));
                if (HOOK_PROCESS(hook, child_read[HOOK_PROCESS_STDERR]) != -1)
                    close (HOOK_PROCESS(hook, child_read[HOOK_PROCESS_STDERR]));
                if (HOOK_PROCESS(hook, child_write[HOOK_PROCESS_STDERR]) != -1)
                    close (HOOK_PROCESS(hook, child_write[HOOK_PROCESS_STDERR]));
                if (HOOK_PROCESS(hook, buffer[HOOK_PROCESS_STDOUT]))
                    free (HOOK_PROCESS(hook, buffer[HOOK_PROCESS_STDOUT]));
                if (HOOK_PROCESS(hook, buffer[HOOK_PROCESS_STDERR]))
                    free (HOOK_PROCESS(hook, buffer[HOOK_PROCESS_STDERR]));
                break;
            case HOOK_TYPE_CONNECT:
                if (HOOK_CONNECT(hook, proxy))
                    free (HOOK_CONNECT(hook, proxy));
                if (HOOK_CONNECT(hook, address))
                    free (HOOK_CONNECT(hook, address));
#ifdef HAVE_GNUTLS
                if (HOOK_CONNECT(hook, gnutls_priorities))
                    free (HOOK_CONNECT(hook, gnutls_priorities));
#endif
                if (HOOK_CONNECT(hook, local_hostname))
                    free (HOOK_CONNECT(hook, local_hostname));
                if (HOOK_CONNECT(hook, hook_child_timer))
                    unhook (HOOK_CONNECT(hook, hook_child_timer));
                if (HOOK_CONNECT(hook, hook_fd))
                    unhook (HOOK_CONNECT(hook, hook_fd));
                if (HOOK_CONNECT(hook, handshake_hook_fd))
                    unhook (HOOK_CONNECT(hook, handshake_hook_fd));
                if (HOOK_CONNECT(hook, handshake_hook_timer))
                    unhook (HOOK_CONNECT(hook, handshake_hook_timer));
                if (HOOK_CONNECT(hook, handshake_ip_address))
                    free (HOOK_CONNECT(hook, handshake_ip_address));
                if (HOOK_CONNECT(hook, child_pid) > 0)
                {
                    kill (HOOK_CONNECT(hook, child_pid), SIGKILL);
                    waitpid (HOOK_CONNECT(hook, child_pid), NULL, 0);
                }
                if (HOOK_CONNECT(hook, child_read) != -1)
                    close (HOOK_CONNECT(hook, child_read));
                if (HOOK_CONNECT(hook, child_write) != -1)
                    close (HOOK_CONNECT(hook, child_write));
                if (HOOK_CONNECT(hook, child_recv) != -1)
                    close (HOOK_CONNECT(hook, child_recv));
                if (HOOK_CONNECT(hook, child_send) != -1)
                    close (HOOK_CONNECT(hook, child_send));
#ifdef HOOK_CONNECT_MAX_SOCKETS
                for (i = 0; i < HOOK_CONNECT_MAX_SOCKETS; i++)
                {
                    if (HOOK_CONNECT(hook, sock_v4[i]) != -1)
                        close (HOOK_CONNECT(hook, sock_v4[i]));
                    if (HOOK_CONNECT(hook, sock_v6[i]) != -1)
                        close (HOOK_CONNECT(hook, sock_v6[i]));
                }
#endif
                break;
            case HOOK_TYPE_PRINT:
                if (HOOK_PRINT(hook, message))
                    free (HOOK_PRINT(hook, message));
                break;
            case HOOK_TYPE_SIGNAL:
                if (HOOK_SIGNAL(hook, signal))
                    free (HOOK_SIGNAL(hook, signal));
                break;
            case HOOK_TYPE_HSIGNAL:
                if (HOOK_HSIGNAL(hook, signal))
                    free (HOOK_HSIGNAL(hook, signal));
                break;
            case HOOK_TYPE_CONFIG:
                if (HOOK_CONFIG(hook, option))
                    free (HOOK_CONFIG(hook, option));
                break;
            case HOOK_TYPE_COMPLETION:
                if (HOOK_COMPLETION(hook, completion_item))
                    free (HOOK_COMPLETION(hook, completion_item));
                if (HOOK_COMPLETION(hook, description))
                    free (HOOK_COMPLETION(hook, description));
                break;
            case HOOK_TYPE_MODIFIER:
                if (HOOK_MODIFIER(hook, modifier))
                    free (HOOK_MODIFIER(hook, modifier));
                break;
            case HOOK_TYPE_INFO:
                if (HOOK_INFO(hook, info_name))
                    free (HOOK_INFO(hook, info_name));
                if (HOOK_INFO(hook, description))
                    free (HOOK_INFO(hook, description));
                if (HOOK_INFO(hook, args_description))
                    free (HOOK_INFO(hook, args_description));
                break;
            case HOOK_TYPE_INFO_HASHTABLE:
                if (HOOK_INFO_HASHTABLE(hook, info_name))
                    free (HOOK_INFO_HASHTABLE(hook, info_name));
                if (HOOK_INFO_HASHTABLE(hook, description))
                    free (HOOK_INFO_HASHTABLE(hook, description));
                if (HOOK_INFO_HASHTABLE(hook, args_description))
                    free (HOOK_INFO_HASHTABLE(hook, args_description));
                if (HOOK_INFO_HASHTABLE(hook, output_description))
                    free (HOOK_INFO_HASHTABLE(hook, output_description));
                break;
            case HOOK_TYPE_INFOLIST:
                if (HOOK_INFOLIST(hook, infolist_name))
                    free (HOOK_INFOLIST(hook, infolist_name));
                if (HOOK_INFOLIST(hook, description))
                    free (HOOK_INFOLIST(hook, description));
                if (HOOK_INFOLIST(hook, pointer_description))
                    free (HOOK_INFOLIST(hook, pointer_description));
                if (HOOK_INFOLIST(hook, args_description))
                    free (HOOK_INFOLIST(hook, args_description));
                break;
            case HOOK_TYPE_HDATA:
                if (HOOK_HDATA(hook, hdata_name))
                    free (HOOK_HDATA(hook, hdata_name));
                if (HOOK_HDATA(hook, description))
                    free (HOOK_HDATA(hook, description));
                break;
            case HOOK_TYPE_FOCUS:
                if (HOOK_FOCUS(hook, area))
                    free (HOOK_FOCUS(hook, area));
                break;
            case HOOK_NUM_TYPES:
                /*
                 * this constant is used to count types only,
                 * it is never used as type
                 */
                break;
        }
        free (hook->hook_data);
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
 * Unhooks everything for a plugin.
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
    struct t_infolist_item *ptr_item;
    char value[64];
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
                    match = string_match (HOOK_COMMAND(ptr_hook, command), arguments, 0);
                    break;
                default:
                    break;
            }
        }

        if (!match)
            continue;

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
        if (!infolist_new_var_string (ptr_item, "subplugin", ptr_hook->subplugin))
            return 0;
        if (!infolist_new_var_string (ptr_item, "type", hook_type_string[ptr_hook->type]))
            return 0;
        if (!infolist_new_var_integer (ptr_item, "deleted", ptr_hook->deleted))
            return 0;
        if (!infolist_new_var_integer (ptr_item, "running", ptr_hook->running))
            return 0;
        if (!infolist_new_var_integer (ptr_item, "priority", ptr_hook->priority))
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
            case HOOK_TYPE_COMMAND_RUN:
                if (!ptr_hook->deleted)
                {
                    if (!infolist_new_var_pointer (ptr_item, "callback", HOOK_COMMAND_RUN(ptr_hook, callback)))
                        return 0;
                    if (!infolist_new_var_string (ptr_item, "command", HOOK_COMMAND_RUN(ptr_hook, command)))
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
                    if (!infolist_new_var_integer (ptr_item, "error", HOOK_FD(ptr_hook, error)))
                        return 0;
                }
                break;
            case HOOK_TYPE_PROCESS:
                if (!ptr_hook->deleted)
                {
                    if (!infolist_new_var_pointer (ptr_item, "callback", HOOK_PROCESS(ptr_hook, callback)))
                        return 0;
                    if (!infolist_new_var_string (ptr_item, "command", HOOK_PROCESS(ptr_hook, command)))
                        return 0;
                    if (!infolist_new_var_string (ptr_item, "options", hashtable_get_string (HOOK_PROCESS(ptr_hook, options), "keys_values")))
                        return 0;
                    if (!infolist_new_var_integer (ptr_item, "timeout", HOOK_PROCESS(ptr_hook, timeout)))
                        return 0;
                    if (!infolist_new_var_integer (ptr_item, "child_read_stdout", HOOK_PROCESS(ptr_hook, child_read[HOOK_PROCESS_STDOUT])))
                        return 0;
                    if (!infolist_new_var_integer (ptr_item, "child_write_stdout", HOOK_PROCESS(ptr_hook, child_write[HOOK_PROCESS_STDOUT])))
                        return 0;
                    if (!infolist_new_var_integer (ptr_item, "child_read_stderr", HOOK_PROCESS(ptr_hook, child_read[HOOK_PROCESS_STDERR])))
                        return 0;
                    if (!infolist_new_var_integer (ptr_item, "child_write_stderr", HOOK_PROCESS(ptr_hook, child_write[HOOK_PROCESS_STDERR])))
                        return 0;
                    if (!infolist_new_var_integer (ptr_item, "child_pid", HOOK_PROCESS(ptr_hook, child_pid)))
                        return 0;
                    if (!infolist_new_var_pointer (ptr_item, "hook_fd_stdout", HOOK_PROCESS(ptr_hook, hook_fd[HOOK_PROCESS_STDOUT])))
                        return 0;
                    if (!infolist_new_var_pointer (ptr_item, "hook_fd_stderr", HOOK_PROCESS(ptr_hook, hook_fd[HOOK_PROCESS_STDERR])))
                        return 0;
                    if (!infolist_new_var_pointer (ptr_item, "hook_timer", HOOK_PROCESS(ptr_hook, hook_timer)))
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
                    if (!infolist_new_var_integer (ptr_item, "retry", HOOK_CONNECT(ptr_hook, retry)))
                        return 0;
#ifdef HAVE_GNUTLS
                    if (!infolist_new_var_pointer (ptr_item, "gnutls_sess", HOOK_CONNECT(ptr_hook, gnutls_sess)))
                        return 0;
                    if (!infolist_new_var_pointer (ptr_item, "gnutls_cb", HOOK_CONNECT(ptr_hook, gnutls_cb)))
                        return 0;
                    if (!infolist_new_var_integer (ptr_item, "gnutls_dhkey_size", HOOK_CONNECT(ptr_hook, gnutls_dhkey_size)))
                        return 0;
#endif
                    if (!infolist_new_var_string (ptr_item, "local_hostname", HOOK_CONNECT(ptr_hook, local_hostname)))
                        return 0;
                    if (!infolist_new_var_integer (ptr_item, "child_read", HOOK_CONNECT(ptr_hook, child_read)))
                        return 0;
                    if (!infolist_new_var_integer (ptr_item, "child_write", HOOK_CONNECT(ptr_hook, child_write)))
                        return 0;
                    if (!infolist_new_var_integer (ptr_item, "child_recv", HOOK_CONNECT(ptr_hook, child_recv)))
                        return 0;
                    if (!infolist_new_var_integer (ptr_item, "child_send", HOOK_CONNECT(ptr_hook, child_send)))
                        return 0;
                    if (!infolist_new_var_integer (ptr_item, "child_pid", HOOK_CONNECT(ptr_hook, child_pid)))
                        return 0;
                    if (!infolist_new_var_pointer (ptr_item, "hook_child_timer", HOOK_CONNECT(ptr_hook, hook_child_timer)))
                        return 0;
                    if (!infolist_new_var_pointer (ptr_item, "hook_fd", HOOK_CONNECT(ptr_hook, hook_fd)))
                        return 0;
                    if (!infolist_new_var_pointer (ptr_item, "handshake_hook_fd", HOOK_CONNECT(ptr_hook, handshake_hook_fd)))
                        return 0;
                    if (!infolist_new_var_pointer (ptr_item, "handshake_hook_timer", HOOK_CONNECT(ptr_hook, handshake_hook_timer)))
                        return 0;
                    if (!infolist_new_var_integer (ptr_item, "handshake_fd_flags", HOOK_CONNECT(ptr_hook, handshake_fd_flags)))
                        return 0;
                    if (!infolist_new_var_string (ptr_item, "handshake_ip_address", HOOK_CONNECT(ptr_hook, handshake_ip_address)))
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
            case HOOK_TYPE_HSIGNAL:
                if (!ptr_hook->deleted)
                {
                    if (!infolist_new_var_pointer (ptr_item, "callback", HOOK_HSIGNAL(ptr_hook, callback)))
                        return 0;
                    if (!infolist_new_var_string (ptr_item, "signal", HOOK_HSIGNAL(ptr_hook, signal)))
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
                    if (!infolist_new_var_string (ptr_item, "description", HOOK_COMPLETION(ptr_hook, description)))
                        return 0;
                    if (!infolist_new_var_string (ptr_item, "description_nls",
                                                  (HOOK_COMPLETION(ptr_hook, description)
                                                   && HOOK_COMPLETION(ptr_hook, description)[0]) ?
                                                  _(HOOK_COMPLETION(ptr_hook, description)) : ""))
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
                    if (!infolist_new_var_string (ptr_item, "args_description", HOOK_INFO(ptr_hook, args_description)))
                        return 0;
                    if (!infolist_new_var_string (ptr_item, "args_description_nls",
                                                  (HOOK_INFO(ptr_hook, args_description)
                                                   && HOOK_INFO(ptr_hook, args_description)[0]) ?
                                                  _(HOOK_INFO(ptr_hook, args_description)) : ""))
                        return 0;
                }
                break;
            case HOOK_TYPE_INFO_HASHTABLE:
                if (!ptr_hook->deleted)
                {
                    if (!infolist_new_var_pointer (ptr_item, "callback", HOOK_INFO_HASHTABLE(ptr_hook, callback)))
                        return 0;
                    if (!infolist_new_var_string (ptr_item, "info_name", HOOK_INFO_HASHTABLE(ptr_hook, info_name)))
                        return 0;
                    if (!infolist_new_var_string (ptr_item, "description", HOOK_INFO_HASHTABLE(ptr_hook, description)))
                        return 0;
                    if (!infolist_new_var_string (ptr_item, "description_nls",
                                                  (HOOK_INFO_HASHTABLE(ptr_hook, description)
                                                   && HOOK_INFO_HASHTABLE(ptr_hook, description)[0]) ?
                                                  _(HOOK_INFO_HASHTABLE(ptr_hook, description)) : ""))
                        return 0;
                    if (!infolist_new_var_string (ptr_item, "args_description", HOOK_INFO_HASHTABLE(ptr_hook, args_description)))
                        return 0;
                    if (!infolist_new_var_string (ptr_item, "args_description_nls",
                                                  (HOOK_INFO_HASHTABLE(ptr_hook, args_description)
                                                   && HOOK_INFO_HASHTABLE(ptr_hook, args_description)[0]) ?
                                                  _(HOOK_INFO_HASHTABLE(ptr_hook, args_description)) : ""))
                        return 0;
                    if (!infolist_new_var_string (ptr_item, "output_description", HOOK_INFO_HASHTABLE(ptr_hook, output_description)))
                        return 0;
                    if (!infolist_new_var_string (ptr_item, "output_description_nls",
                                                  (HOOK_INFO_HASHTABLE(ptr_hook, output_description)
                                                   && HOOK_INFO_HASHTABLE(ptr_hook, output_description)[0]) ?
                                                  _(HOOK_INFO_HASHTABLE(ptr_hook, output_description)) : ""))
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
                    if (!infolist_new_var_string (ptr_item, "pointer_description", HOOK_INFOLIST(ptr_hook, pointer_description)))
                        return 0;
                    if (!infolist_new_var_string (ptr_item, "pointer_description_nls",
                                                  (HOOK_INFOLIST(ptr_hook, pointer_description)
                                                   && HOOK_INFOLIST(ptr_hook, pointer_description)[0]) ?
                                                  _(HOOK_INFOLIST(ptr_hook, pointer_description)) : ""))
                        return 0;
                    if (!infolist_new_var_string (ptr_item, "args_description", HOOK_INFOLIST(ptr_hook, args_description)))
                        return 0;
                    if (!infolist_new_var_string (ptr_item, "args_description_nls",
                                                  (HOOK_INFOLIST(ptr_hook, args_description)
                                                   && HOOK_INFOLIST(ptr_hook, args_description)[0]) ?
                                                  _(HOOK_INFOLIST(ptr_hook, args_description)) : ""))
                        return 0;
                }
                break;
            case HOOK_TYPE_HDATA:
                if (!ptr_hook->deleted)
                {
                    if (!infolist_new_var_pointer (ptr_item, "callback", HOOK_HDATA(ptr_hook, callback)))
                        return 0;
                    if (!infolist_new_var_string (ptr_item, "hdata_name", HOOK_HDATA(ptr_hook, hdata_name)))
                        return 0;
                    if (!infolist_new_var_string (ptr_item, "description", HOOK_HDATA(ptr_hook, description)))
                        return 0;
                    if (!infolist_new_var_string (ptr_item, "description_nls",
                                                  (HOOK_HDATA(ptr_hook, description)
                                                   && HOOK_HDATA(ptr_hook, description)[0]) ?
                                                  _(HOOK_HDATA(ptr_hook, description)) : ""))
                        return 0;
                }
                break;
            case HOOK_TYPE_FOCUS:
                if (!ptr_hook->deleted)
                {
                    if (!infolist_new_var_pointer (ptr_item, "callback", HOOK_FOCUS(ptr_hook, callback)))
                        return 0;
                    if (!infolist_new_var_string (ptr_item, "area", HOOK_FOCUS(ptr_hook, area)))
                        return 0;
                }
                break;
            case HOOK_NUM_TYPES:
                /*
                 * this constant is used to count types only,
                 * it is never used as type
                 */
                break;
        }
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
hook_add_to_infolist (struct t_infolist *infolist, const char *arguments)
{
    const char *pos_arguments;
    char *type;
    int i, type_int;

    if (!infolist)
        return 0;

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
    int type, i, j;
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
            log_printf ("  plugin. . . . . . . . . : 0x%lx ('%s')",
                        ptr_hook->plugin, plugin_get_name (ptr_hook->plugin));
            log_printf ("  subplugin . . . . . . . : '%s'",  ptr_hook->subplugin);
            log_printf ("  type. . . . . . . . . . : %d (%s)",
                        ptr_hook->type, hook_type_string[ptr_hook->type]);
            log_printf ("  deleted . . . . . . . . : %d",    ptr_hook->deleted);
            log_printf ("  running . . . . . . . . : %d",    ptr_hook->running);
            log_printf ("  priority. . . . . . . . : %d",    ptr_hook->priority);
            log_printf ("  callback_data . . . . . : 0x%lx", ptr_hook->callback_data);
            switch (ptr_hook->type)
            {
                case HOOK_TYPE_COMMAND:
                    if (!ptr_hook->deleted)
                    {
                        log_printf ("  command data:");
                        log_printf ("    callback. . . . . . . : 0x%lx", HOOK_COMMAND(ptr_hook, callback));
                        log_printf ("    command . . . . . . . : '%s'",  HOOK_COMMAND(ptr_hook, command));
                        log_printf ("    description . . . . . : '%s'",  HOOK_COMMAND(ptr_hook, description));
                        log_printf ("    args. . . . . . . . . : '%s'",  HOOK_COMMAND(ptr_hook, args));
                        log_printf ("    args_description. . . : '%s'",  HOOK_COMMAND(ptr_hook, args_description));
                        log_printf ("    completion. . . . . . : '%s'",  HOOK_COMMAND(ptr_hook, completion));
                        log_printf ("    cplt_num_templates. . : %d",    HOOK_COMMAND(ptr_hook, cplt_num_templates));
                        for (i = 0; i < HOOK_COMMAND(ptr_hook, cplt_num_templates); i++)
                        {
                            log_printf ("    cplt_templates[%04d] . . . : '%s'",
                                        i, HOOK_COMMAND(ptr_hook, cplt_templates)[i]);
                            log_printf ("    cplt_templates_static[%04d]: '%s'",
                                        i, HOOK_COMMAND(ptr_hook, cplt_templates_static)[i]);
                            log_printf ("      num_args. . . . . . : %d",
                                        HOOK_COMMAND(ptr_hook, cplt_template_num_args)[i]);
                            for (j = 0; j < HOOK_COMMAND(ptr_hook, cplt_template_num_args)[i]; j++)
                            {
                                log_printf ("      args[%04d]. . . . . : '%s'",
                                            j, HOOK_COMMAND(ptr_hook, cplt_template_args)[i][j]);
                            }
                        }
                        log_printf ("    num_args_concat . . . : %d", HOOK_COMMAND(ptr_hook, cplt_template_num_args_concat));
                        for (i = 0; i < HOOK_COMMAND(ptr_hook, cplt_template_num_args_concat); i++)
                        {
                            log_printf ("    args_concat[%04d] . . : '%s'",
                                        i, HOOK_COMMAND(ptr_hook, cplt_template_args_concat)[i]);
                        }
                    }
                    break;
                case HOOK_TYPE_COMMAND_RUN:
                    if (!ptr_hook->deleted)
                    {
                        log_printf ("  command_run data:");
                        log_printf ("    callback. . . . . . . : 0x%lx", HOOK_COMMAND_RUN(ptr_hook, callback));
                        log_printf ("    command . . . . . . . : '%s'",  HOOK_COMMAND_RUN(ptr_hook, command));
                    }
                    break;
                case HOOK_TYPE_TIMER:
                    if (!ptr_hook->deleted)
                    {
                        log_printf ("  timer data:");
                        log_printf ("    callback. . . . . . . : 0x%lx", HOOK_TIMER(ptr_hook, callback));
                        log_printf ("    interval. . . . . . . : %ld",   HOOK_TIMER(ptr_hook, interval));
                        log_printf ("    align_second. . . . . : %d",    HOOK_TIMER(ptr_hook, align_second));
                        log_printf ("    remaining_calls . . . : %d",    HOOK_TIMER(ptr_hook, remaining_calls));
                        text_time[0] = '\0';
                        local_time = localtime (&HOOK_TIMER(ptr_hook, last_exec).tv_sec);
                        if (local_time)
                        {
                            strftime (text_time, sizeof (text_time),
                                      "%d/%m/%Y %H:%M:%S", local_time);
                        }
                        log_printf ("    last_exec.tv_sec. . . : %ld (%s)",
                                    HOOK_TIMER(ptr_hook, last_exec.tv_sec),
                                    text_time);
                        log_printf ("    last_exec.tv_usec . . : %ld",   HOOK_TIMER(ptr_hook, last_exec.tv_usec));
                        text_time[0] = '\0';
                        local_time = localtime (&HOOK_TIMER(ptr_hook, next_exec).tv_sec);
                        if (local_time)
                        {
                            strftime (text_time, sizeof (text_time),
                                      "%d/%m/%Y %H:%M:%S", local_time);
                        }
                        log_printf ("    next_exec.tv_sec. . . : %ld (%s)",
                                    HOOK_TIMER(ptr_hook, next_exec.tv_sec),
                                    text_time);
                        log_printf ("    next_exec.tv_usec . . : %ld",   HOOK_TIMER(ptr_hook, next_exec.tv_usec));
                    }
                    break;
                case HOOK_TYPE_FD:
                    if (!ptr_hook->deleted)
                    {
                        log_printf ("  fd data:");
                        log_printf ("    callback. . . . . . . : 0x%lx", HOOK_FD(ptr_hook, callback));
                        log_printf ("    fd. . . . . . . . . . : %d",    HOOK_FD(ptr_hook, fd));
                        log_printf ("    flags . . . . . . . . : %d",    HOOK_FD(ptr_hook, flags));
                        log_printf ("    error . . . . . . . . : %d",    HOOK_FD(ptr_hook, error));
                    }
                    break;
                case HOOK_TYPE_PROCESS:
                    if (!ptr_hook->deleted)
                    {
                        log_printf ("  process data:");
                        log_printf ("    callback. . . . . . . : 0x%lx", HOOK_PROCESS(ptr_hook, callback));
                        log_printf ("    command . . . . . . . : '%s'",  HOOK_PROCESS(ptr_hook, command));
                        log_printf ("    options . . . . . . . : 0x%lx (hashtable: '%s')",
                                    HOOK_PROCESS(ptr_hook, options),
                                    hashtable_get_string (HOOK_PROCESS(ptr_hook, options),
                                                          "keys_values"));
                        log_printf ("    timeout . . . . . . . : %d",    HOOK_PROCESS(ptr_hook, timeout));
                        log_printf ("    child_read[stdout]. . : %d",    HOOK_PROCESS(ptr_hook, child_read[HOOK_PROCESS_STDOUT]));
                        log_printf ("    child_write[stdout] . : %d",    HOOK_PROCESS(ptr_hook, child_write[HOOK_PROCESS_STDOUT]));
                        log_printf ("    child_read[stderr]. . : %d",    HOOK_PROCESS(ptr_hook, child_read[HOOK_PROCESS_STDERR]));
                        log_printf ("    child_write[stderr] . : %d",    HOOK_PROCESS(ptr_hook, child_write[HOOK_PROCESS_STDERR]));
                        log_printf ("    child_pid . . . . . . : %d",    HOOK_PROCESS(ptr_hook, child_pid));
                        log_printf ("    hook_fd[stdout] . . . : 0x%lx", HOOK_PROCESS(ptr_hook, hook_fd[HOOK_PROCESS_STDOUT]));
                        log_printf ("    hook_fd[stderr] . . . : 0x%lx", HOOK_PROCESS(ptr_hook, hook_fd[HOOK_PROCESS_STDERR]));
                        log_printf ("    hook_timer. . . . . . : 0x%lx", HOOK_PROCESS(ptr_hook, hook_timer));
                    }
                    break;
                case HOOK_TYPE_CONNECT:
                    if (!ptr_hook->deleted)
                    {
                        log_printf ("  connect data:");
                        log_printf ("    callback. . . . . . . : 0x%lx", HOOK_CONNECT(ptr_hook, callback));
                        log_printf ("    address . . . . . . . : '%s'",  HOOK_CONNECT(ptr_hook, address));
                        log_printf ("    port. . . . . . . . . : %d",    HOOK_CONNECT(ptr_hook, port));
                        log_printf ("    sock. . . . . . . . . : %d",    HOOK_CONNECT(ptr_hook, sock));
                        log_printf ("    ipv6. . . . . . . . . : %d",    HOOK_CONNECT(ptr_hook, ipv6));
                        log_printf ("    retry . . . . . . . . : %d",    HOOK_CONNECT(ptr_hook, retry));
#ifdef HAVE_GNUTLS
                        log_printf ("    gnutls_sess . . . . . : 0x%lx", HOOK_CONNECT(ptr_hook, gnutls_sess));
                        log_printf ("    gnutls_cb . . . . . . : 0x%lx", HOOK_CONNECT(ptr_hook, gnutls_cb));
                        log_printf ("    gnutls_dhkey_size . . : %d",    HOOK_CONNECT(ptr_hook, gnutls_dhkey_size));
                        log_printf ("    gnutls_priorities . . : '%s'",  HOOK_CONNECT(ptr_hook, gnutls_priorities));
#endif
                        log_printf ("    local_hostname. . . . : '%s'",  HOOK_CONNECT(ptr_hook, local_hostname));
                        log_printf ("    child_read. . . . . . : %d",    HOOK_CONNECT(ptr_hook, child_read));
                        log_printf ("    child_write . . . . . : %d",    HOOK_CONNECT(ptr_hook, child_write));
                        log_printf ("    child_recv. . . . . . : %d",    HOOK_CONNECT(ptr_hook, child_recv));
                        log_printf ("    child_send. . . . . . : %d",    HOOK_CONNECT(ptr_hook, child_send));
                        log_printf ("    child_pid . . . . . . : %d",    HOOK_CONNECT(ptr_hook, child_pid));
                        log_printf ("    hook_child_timer. . . : 0x%lx", HOOK_CONNECT(ptr_hook, hook_child_timer));
                        log_printf ("    hook_fd . . . . . . . : 0x%lx", HOOK_CONNECT(ptr_hook, hook_fd));
                        log_printf ("    handshake_hook_fd . . : 0x%lx", HOOK_CONNECT(ptr_hook, handshake_hook_fd));
                        log_printf ("    handshake_hook_timer. : 0x%lx", HOOK_CONNECT(ptr_hook, handshake_hook_timer));
                        log_printf ("    handshake_fd_flags. . : %d",    HOOK_CONNECT(ptr_hook, handshake_fd_flags));
                        log_printf ("    handshake_ip_address. : '%s'",  HOOK_CONNECT(ptr_hook, handshake_ip_address));
#ifdef HOOK_CONNECT_MAX_SOCKETS
                        for (i = 0; i < HOOK_CONNECT_MAX_SOCKETS; i++)
                        {
                            log_printf ("    sock_v4[%d]. . . . . . : '%d'", HOOK_CONNECT(ptr_hook, sock_v4[i]));
                            log_printf ("    sock_v6[%d]. . . . . . : '%d'", HOOK_CONNECT(ptr_hook, sock_v6[i]));
                        }
#endif
                    }
                    break;
                case HOOK_TYPE_PRINT:
                    if (!ptr_hook->deleted)
                    {
                        log_printf ("  print data:");
                        log_printf ("    callback. . . . . . . : 0x%lx", HOOK_PRINT(ptr_hook, callback));
                        log_printf ("    buffer. . . . . . . . : 0x%lx", HOOK_PRINT(ptr_hook, buffer));
                        log_printf ("    tags_count. . . . . . : %d",    HOOK_PRINT(ptr_hook, tags_count));
                        log_printf ("    tags_array. . . . . . : 0x%lx", HOOK_PRINT(ptr_hook, tags_array));
                        log_printf ("    message . . . . . . . : '%s'",  HOOK_PRINT(ptr_hook, message));
                        log_printf ("    strip_colors. . . . . : %d",    HOOK_PRINT(ptr_hook, strip_colors));
                    }
                    break;
                case HOOK_TYPE_SIGNAL:
                    if (!ptr_hook->deleted)
                    {
                        log_printf ("  signal data:");
                        log_printf ("    callback. . . . . . . : 0x%lx", HOOK_SIGNAL(ptr_hook, callback));
                        log_printf ("    signal. . . . . . . . : '%s'",  HOOK_SIGNAL(ptr_hook, signal));
                    }
                    break;
                case HOOK_TYPE_HSIGNAL:
                    if (!ptr_hook->deleted)
                    {
                        log_printf ("  signal data:");
                        log_printf ("    callback. . . . . . . : 0x%lx", HOOK_HSIGNAL(ptr_hook, callback));
                        log_printf ("    signal. . . . . . . . : '%s'",  HOOK_HSIGNAL(ptr_hook, signal));
                    }
                    break;
                case HOOK_TYPE_CONFIG:
                    if (!ptr_hook->deleted)
                    {
                        log_printf ("  config data:");
                        log_printf ("    callback. . . . . . . : 0x%lx", HOOK_CONFIG(ptr_hook, callback));
                        log_printf ("    option. . . . . . . . : '%s'",  HOOK_CONFIG(ptr_hook, option));
                    }
                    break;
                case HOOK_TYPE_COMPLETION:
                    if (!ptr_hook->deleted)
                    {
                        log_printf ("  completion data:");
                        log_printf ("    callback. . . . . . . : 0x%lx", HOOK_COMPLETION(ptr_hook, callback));
                        log_printf ("    completion_item . . . : '%s'",  HOOK_COMPLETION(ptr_hook, completion_item));
                        log_printf ("    description . . . . . : '%s'",  HOOK_COMPLETION(ptr_hook, description));
                    }
                    break;
                case HOOK_TYPE_MODIFIER:
                    if (!ptr_hook->deleted)
                    {
                        log_printf ("  modifier data:");
                        log_printf ("    callback. . . . . . . : 0x%lx", HOOK_MODIFIER(ptr_hook, callback));
                        log_printf ("    modifier. . . . . . . : '%s'",  HOOK_MODIFIER(ptr_hook, modifier));
                    }
                    break;
                case HOOK_TYPE_INFO:
                    if (!ptr_hook->deleted)
                    {
                        log_printf ("  info data:");
                        log_printf ("    callback. . . . . . . : 0x%lx", HOOK_INFO(ptr_hook, callback));
                        log_printf ("    info_name . . . . . . : '%s'",  HOOK_INFO(ptr_hook, info_name));
                        log_printf ("    description . . . . . : '%s'",  HOOK_INFO(ptr_hook, description));
                        log_printf ("    args_description. . . : '%s'",  HOOK_INFO(ptr_hook, args_description));
                    }
                    break;
                case HOOK_TYPE_INFO_HASHTABLE:
                    if (!ptr_hook->deleted)
                    {
                        log_printf ("  info_hashtable data:");
                        log_printf ("    callback. . . . . . . : 0x%lx", HOOK_INFO_HASHTABLE(ptr_hook, callback));
                        log_printf ("    info_name . . . . . . : '%s'",  HOOK_INFO_HASHTABLE(ptr_hook, info_name));
                        log_printf ("    description . . . . . : '%s'",  HOOK_INFO_HASHTABLE(ptr_hook, description));
                        log_printf ("    args_description. . . : '%s'",  HOOK_INFO_HASHTABLE(ptr_hook, args_description));
                        log_printf ("    output_description. . : '%s'",  HOOK_INFO_HASHTABLE(ptr_hook, output_description));
                    }
                    break;
                case HOOK_TYPE_INFOLIST:
                    if (!ptr_hook->deleted)
                    {
                        log_printf ("  infolist data:");
                        log_printf ("    callback. . . . . . . : 0x%lx", HOOK_INFOLIST(ptr_hook, callback));
                        log_printf ("    infolist_name . . . . : '%s'",  HOOK_INFOLIST(ptr_hook, infolist_name));
                        log_printf ("    description . . . . . : '%s'",  HOOK_INFOLIST(ptr_hook, description));
                        log_printf ("    pointer_description . : '%s'",  HOOK_INFOLIST(ptr_hook, pointer_description));
                        log_printf ("    args_description. . . : '%s'",  HOOK_INFOLIST(ptr_hook, args_description));
                    }
                    break;
                case HOOK_TYPE_HDATA:
                    if (!ptr_hook->deleted)
                    {
                        log_printf ("  hdata data:");
                        log_printf ("    callback. . . . . . . : 0x%lx", HOOK_HDATA(ptr_hook, callback));
                        log_printf ("    hdata_name. . . . . . : '%s'",  HOOK_HDATA(ptr_hook, hdata_name));
                        log_printf ("    description . . . . . : '%s'",  HOOK_HDATA(ptr_hook, description));
                    }
                    break;
                case HOOK_TYPE_FOCUS:
                    if (!ptr_hook->deleted)
                    {
                        log_printf ("  focus data:");
                        log_printf ("    callback. . . . . . . : 0x%lx", HOOK_FOCUS(ptr_hook, callback));
                        log_printf ("    area. . . . . . . . . : '%s'",  HOOK_FOCUS(ptr_hook, area));
                    }
                    break;
                case HOOK_NUM_TYPES:
                    /*
                     * this constant is used to count types only,
                     * it is never used as type
                     */
                    break;
            }
            log_printf ("  prev_hook . . . . . . . : 0x%lx", ptr_hook->prev_hook);
            log_printf ("  next_hook . . . . . . . : 0x%lx", ptr_hook->next_hook);
        }
    }
}
