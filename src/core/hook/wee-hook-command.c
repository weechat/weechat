/*
 * wee-hook-command.c - WeeChat command hook
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

#include "../weechat.h"
#include "../wee-arraylist.h"
#include "../wee-config.h"
#include "../wee-hook.h"
#include "../wee-infolist.h"
#include "../wee-list.h"
#include "../wee-log.h"
#include "../wee-string.h"
#include "../wee-utf8.h"
#include "../../gui/gui-chat.h"
#include "../../gui/gui-filter.h"
#include "../../plugins/plugin.h"


/*
 * Returns description of hook.
 *
 * Note: result must be freed after use.
 */

char *
hook_command_get_description (struct t_hook *hook)
{
    return strdup (HOOK_COMMAND(hook, command));
}

/*
 * Searches for a command hook in list.
 *
 * Returns pointer to hook found, NULL if not found.
 */

struct t_hook *
hook_command_search (struct t_weechat_plugin *plugin, const char *command)
{
    struct t_hook *ptr_hook;

    if (!command)
        return NULL;

    for (ptr_hook = weechat_hooks[HOOK_TYPE_COMMAND]; ptr_hook;
         ptr_hook = ptr_hook->next_hook)
    {
        if (!ptr_hook->deleted
            && (ptr_hook->plugin == plugin)
            && (strcmp (HOOK_COMMAND(ptr_hook, command), command) == 0))
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
    char *completion, *pos_completion, *pos_double_pipe, *pos_start, *pos_end;
    char **items;
    const char *last_space, *ptr_template;

    completion = string_replace (hook_command->completion, "\n", " ");
    if (!completion)
        return;

    /* split templates using "||" as separator */
    hook_command->cplt_num_templates = 1;
    pos_completion = completion;
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
    pos_completion = completion;
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
         * contain "%"
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
            else if (ptr_template[0] == '%')
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
        hook_command->cplt_template_args[i] = string_split (
            hook_command->cplt_templates[i],
            " ",
            NULL,
            WEECHAT_STRING_SPLIT_STRIP_LEFT
            | WEECHAT_STRING_SPLIT_STRIP_RIGHT
            | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
            0,
            &(hook_command->cplt_template_num_args[i]));
        if (hook_command->cplt_template_num_args[i] > hook_command->cplt_template_num_args_concat)
            hook_command->cplt_template_num_args_concat = hook_command->cplt_template_num_args[i];
    }

    /*
     * build strings with concatenation of items from different templates
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
                /* concatenate items with "|" as separator */
                weelist_remove_all (list);
                hook_command->cplt_template_args_concat[i][0] = '\0';
                for (j = 0; j < hook_command->cplt_num_templates; j++)
                {
                    if (i < hook_command->cplt_template_num_args[j])
                    {
                        items = string_split (
                            hook_command->cplt_template_args[j][i],
                            "|",
                            NULL,
                            WEECHAT_STRING_SPLIT_STRIP_LEFT
                            | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                            | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                            0,
                            &num_items);
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

    free (completion);
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
              t_hook_callback_command *callback,
              const void *callback_pointer,
              void *callback_data)
{
    struct t_hook *new_hook;
    struct t_hook_command *new_hook_command;
    int priority;
    const char *ptr_command;

    if (!callback)
        return NULL;

    if (hook_command_search (plugin, command))
    {
        gui_chat_printf (NULL,
                         _("%sAnother command \"%s\" already exists for "
                           "plugin \"%s\""),
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

    string_get_priority_and_name (command, &priority, &ptr_command,
                                  HOOK_PRIORITY_DEFAULT);
    hook_init_data (new_hook, plugin, HOOK_TYPE_COMMAND, priority,
                    callback_pointer, callback_data);

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
 *   HOOK_COMMAND_EXEC_OK: command executed successfully
 *   HOOK_COMMAND_EXEC_ERROR: command executed and failed
 *   HOOK_COMMAND_EXEC_NOT_FOUND: command not found
 *   HOOK_COMMAND_EXEC_AMBIGUOUS_PLUGINS: command is ambiguous (same command
 *     exists for another plugin, and we don't know which one to run)
 *   HOOK_COMMAND_EXEC_AMBIGUOUS_INCOMPLETE: command is ambiguous (incomplete
 *     command and multiple commands start with this name)
 *   HOOK_COMMAND_EXEC_RUNNING: command is already running
 */

int
hook_command_exec (struct t_gui_buffer *buffer, int any_plugin,
                   struct t_weechat_plugin *plugin, const char *string)
{
    struct t_hook *ptr_hook, *next_hook;
    struct t_hook *hook_plugin, *hook_other_plugin, *hook_other_plugin2;
    struct t_hook *hook_incomplete_command;
    char **argv, **argv_eol;
    const char *ptr_command_name;
    int argc, rc, length_command_name, allow_incomplete_commands;
    int count_other_plugin, count_incomplete_commands;

    if (!buffer || !string || !string[0])
        return HOOK_COMMAND_EXEC_NOT_FOUND;

    if (hook_command_run_exec (buffer, string) == WEECHAT_RC_OK_EAT)
        return HOOK_COMMAND_EXEC_OK;

    argv = string_split (string, " ", NULL,
                         WEECHAT_STRING_SPLIT_STRIP_LEFT
                         | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                         | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                         0, &argc);
    if (argc == 0)
    {
        string_free_split (argv);
        return HOOK_COMMAND_EXEC_NOT_FOUND;
    }
    argv_eol = string_split (string, " ", NULL,
                             WEECHAT_STRING_SPLIT_STRIP_LEFT
                             | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                             | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS
                             | WEECHAT_STRING_SPLIT_KEEP_EOL,
                             0, NULL);

    ptr_command_name = utf8_next_char (argv[0]);
    length_command_name = utf8_strlen (ptr_command_name);

    hook_exec_start ();

    hook_plugin = NULL;
    hook_other_plugin = NULL;
    hook_other_plugin2 = NULL;
    hook_incomplete_command = NULL;
    count_other_plugin = 0;
    allow_incomplete_commands = CONFIG_BOOLEAN(config_look_command_incomplete);
    count_incomplete_commands = 0;
    ptr_hook = weechat_hooks[HOOK_TYPE_COMMAND];
    while (ptr_hook)
    {
        next_hook = ptr_hook->next_hook;

        if (!ptr_hook->deleted)
        {
            if (strcmp (ptr_command_name, HOOK_COMMAND(ptr_hook, command)) == 0)
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
            else if (allow_incomplete_commands
                     && (string_strncmp (ptr_command_name,
                                         HOOK_COMMAND(ptr_hook, command),
                                         length_command_name) == 0))
            {
                hook_incomplete_command = ptr_hook;
                count_incomplete_commands++;
            }
        }

        ptr_hook = next_hook;
    }

    rc = HOOK_COMMAND_EXEC_NOT_FOUND;
    ptr_hook = NULL;

    if (hook_plugin || hook_other_plugin)
    {
        if (!hook_plugin && (count_other_plugin > 1)
            && (hook_other_plugin->priority == hook_other_plugin2->priority))
        {
            /*
             * ambiguous: no command for current plugin, but more than one
             * command was found for other plugins with the same priority
             * => we don't know which one to run!
             */
            rc = HOOK_COMMAND_EXEC_AMBIGUOUS_PLUGINS;
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
        }
    }
    else if (hook_incomplete_command)
    {
        if (count_incomplete_commands == 1)
            ptr_hook = hook_incomplete_command;
        else
            rc = HOOK_COMMAND_EXEC_AMBIGUOUS_INCOMPLETE;
    }

    /* execute the command for the hook found */
    if (ptr_hook)
    {
        if (ptr_hook->running >= HOOK_COMMAND_MAX_CALLS)
        {
            /* loop in execution of command => do NOT execute again */
            rc = HOOK_COMMAND_EXEC_RUNNING;
        }
        else
        {
            /* execute the command! */
            ptr_hook->running++;
            rc = (int) (HOOK_COMMAND(ptr_hook, callback))
                (ptr_hook->callback_pointer,
                 ptr_hook->callback_data,
                 buffer,
                 argc,
                 argv,
                 argv_eol);
            ptr_hook->running--;
            if (rc == WEECHAT_RC_ERROR)
                rc = HOOK_COMMAND_EXEC_ERROR;
            else
                rc = HOOK_COMMAND_EXEC_OK;
        }
    }

    string_free_split (argv);
    string_free_split (argv_eol);

    hook_exec_end ();

    return rc;
}

/*
 * Gets relevance for cmd2 (existing command) compared to cmd1 (non-existing
 * command).
 *
 * Both commands are in lower case.
 *
 * Returns a number based on the Levenshtein distance between two commands,
 * lower is better.
 */

int
hook_command_similar_get_relevance (const char *cmd1, int length_cmd1,
                                    const char *cmd2, int length_cmd2)
{
    const char *pos;
    int relevance;

    /* perfect match if commands are the same (different case) */
    if (strcmp (cmd1, cmd2) == 0)
        return HOOK_COMMAND_SIMILAR_DIFF_CASE_ONLY;

    /* init relevance with Levenshtein distance (lower is better) */
    relevance = string_levenshtein (cmd1, cmd2, 1);

    /* bonus if one command includes the other */
    pos = (length_cmd1 < length_cmd2) ?
        strstr (cmd2, cmd1) : strstr (cmd1, cmd2);
    if (pos)
    {
        relevance /= 4;
        /* extra bonus if match is at beginning */
        if ((pos == cmd1) || (pos == cmd2))
            relevance -= 2;
    }
    else
    {
        /* malus if no chars in common between two words */
        if (string_get_common_bytes_count (cmd1, cmd2) == 0)
            relevance *= 2;
    }

    return relevance;
}

/*
 * Compares similar commands to sort them by relevance (lower number first:
 * best relevance).
 */

int
hook_command_similar_cmp_cb (void *data, struct t_arraylist *arraylist,
                             void *pointer1, void *pointer2)
{
    struct t_hook_command_similar *ptr_cmd1, *ptr_cmd2;

    /* make C compiler happy */
    (void) data;
    (void) arraylist;

    ptr_cmd1 = (struct t_hook_command_similar *)pointer1;
    ptr_cmd2 = (struct t_hook_command_similar *)pointer2;

    if (ptr_cmd1->relevance < ptr_cmd2->relevance)
        return -1;

    if (ptr_cmd1->relevance > ptr_cmd2->relevance)
        return 1;

    return string_strcasecmp (ptr_cmd1->command, ptr_cmd2->command);
}

/*
 * Frees a similar command.
 */

void
hook_command_similar_free_cb (void *data, struct t_arraylist *arraylist,
                              void *pointer)
{
    /* make C compiler happy */
    (void) data;
    (void) arraylist;

    free (pointer);
}

/*
 * Builds an arraylist with similar commands.
 *
 * Note: result must be freed after use.
 */

struct t_arraylist *
hook_command_build_list_similar_commands (const char *command)
{
    struct t_arraylist *list_commands;
    struct t_hook *ptr_hook;
    struct t_hook_command_similar *cmd_similar;
    char *cmd1, *cmd2;
    int length_cmd1, length_cmd2, relevance;

    cmd1 = string_tolower (command);
    if (!cmd1)
        return NULL;

    length_cmd1 = strlen (cmd1);

    list_commands = arraylist_new (64, 1, 0,
                                   &hook_command_similar_cmp_cb, NULL,
                                   &hook_command_similar_free_cb, NULL);

    for (ptr_hook = weechat_hooks[HOOK_TYPE_COMMAND]; ptr_hook;
         ptr_hook = ptr_hook->next_hook)
    {
        if (ptr_hook->deleted)
            continue;
        cmd2 = string_tolower (HOOK_COMMAND(ptr_hook, command));
        if (!cmd2)
            continue;
        length_cmd2 = strlen (cmd2);
        relevance = hook_command_similar_get_relevance (cmd1, length_cmd1,
                                                        cmd2, length_cmd2);
        cmd_similar = (struct t_hook_command_similar *)malloc (
            sizeof (*cmd_similar));
        if (cmd_similar)
        {
            cmd_similar->command = HOOK_COMMAND(ptr_hook, command);
            cmd_similar->relevance = relevance;
        }
        arraylist_add (list_commands, cmd_similar);
        free (cmd2);
    }

    free (cmd1);

    return list_commands;
}

/*
 * Displays an error on unknown command, with a list of existing similar
 * command names.
 */

void
hook_command_display_error_unknown (const char *command)
{
    struct t_arraylist *list_commands;
    struct t_hook_command_similar *cmd_similar;
    char **str_commands;
    int i, list_size, found, found_diff_case_only;

    if (!command || !command[0])
        return;

    list_commands = hook_command_build_list_similar_commands (command);
    if (!list_commands)
        return;

    str_commands = string_dyn_alloc (256);
    if (!str_commands)
    {
        arraylist_free (list_commands);
        return;
    }

    found = 0;
    found_diff_case_only = 0;
    list_size = arraylist_size (list_commands);
    for (i = 0; i < list_size; i++)
    {
        cmd_similar = (struct t_hook_command_similar *)arraylist_get (
            list_commands, i);
        if (cmd_similar->relevance >= 3)
            break;
        if (found > 0)
            string_dyn_concat (str_commands, ", ", -1);
        string_dyn_concat (str_commands, cmd_similar->command, -1);
        found++;
        if (cmd_similar->relevance == HOOK_COMMAND_SIMILAR_DIFF_CASE_ONLY)
            found_diff_case_only++;
        if (found >= 5)
            break;
    }
    if (found == 0)
        string_dyn_concat (str_commands, "-", -1);

    gui_chat_printf_date_tags (
        NULL,
        0, GUI_FILTER_TAG_NO_FILTER,
        (found_diff_case_only > 0) ?
        _("%sUnknown command \"%s\" (commands are case sensitive, "
          "type /help for help), "
          "commands with similar name: %s") :
        _("%sUnknown command \"%s\" (type /help for help), "
          "commands with similar name: %s"),
        gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
        command,
        *str_commands);

    string_dyn_free (str_commands, 1);
    arraylist_free (list_commands);
}

/*
 * Frees data in a command hook.
 */

void
hook_command_free_data (struct t_hook *hook)
{
    int i;

    if (!hook || !hook->hook_data)
        return;

    if (HOOK_COMMAND(hook, command))
    {
        free (HOOK_COMMAND(hook, command));
        HOOK_COMMAND(hook, command) = NULL;
    }
    if (HOOK_COMMAND(hook, description))
    {
        free (HOOK_COMMAND(hook, description));
        HOOK_COMMAND(hook, description) = NULL;
    }
    if (HOOK_COMMAND(hook, args))
    {
        free (HOOK_COMMAND(hook, args));
        HOOK_COMMAND(hook, args) = NULL;
    }
    if (HOOK_COMMAND(hook, args_description))
    {
        free (HOOK_COMMAND(hook, args_description));
        HOOK_COMMAND(hook, args_description) = NULL;
    }
    if (HOOK_COMMAND(hook, completion))
    {
        free (HOOK_COMMAND(hook, completion));
        HOOK_COMMAND(hook, completion) = NULL;
    }
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
    {
        free (HOOK_COMMAND(hook, cplt_templates_static));
        HOOK_COMMAND(hook, cplt_templates_static) = NULL;
    }
    if (HOOK_COMMAND(hook, cplt_template_num_args))
    {
        free (HOOK_COMMAND(hook, cplt_template_num_args));
        HOOK_COMMAND(hook, cplt_template_num_args) = NULL;
    }
    if (HOOK_COMMAND(hook, cplt_template_args))
    {
        free (HOOK_COMMAND(hook, cplt_template_args));
        HOOK_COMMAND(hook, cplt_template_args) = NULL;
    }
    if (HOOK_COMMAND(hook, cplt_template_args_concat))
    {
        for (i = 0;
             i < HOOK_COMMAND(hook, cplt_template_num_args_concat);
             i++)
        {
            free (HOOK_COMMAND(hook, cplt_template_args_concat[i]));
        }
        free (HOOK_COMMAND(hook, cplt_template_args_concat));
        HOOK_COMMAND(hook, cplt_template_args_concat) = NULL;
    }

    free (hook->hook_data);
    hook->hook_data = NULL;
}

/*
 * Adds command hook data in the infolist item.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
hook_command_add_to_infolist (struct t_infolist_item *item,
                              struct t_hook *hook)
{
    if (!item || !hook || !hook->hook_data)
        return 0;

    if (!infolist_new_var_pointer (item, "callback", HOOK_COMMAND(hook, callback)))
        return 0;
    if (!infolist_new_var_string (item, "command", HOOK_COMMAND(hook, command)))
        return 0;
    if (!infolist_new_var_string (item, "description",
                                  HOOK_COMMAND(hook, description)))
        return 0;
    if (!infolist_new_var_string (item, "description_nls",
                                  (HOOK_COMMAND(hook, description)
                                   && HOOK_COMMAND(hook, description)[0]) ?
                                  _(HOOK_COMMAND(hook, description)) : ""))
        return 0;
    if (!infolist_new_var_string (item, "args",
                                  HOOK_COMMAND(hook, args)))
        return 0;
    if (!infolist_new_var_string (item, "args_nls",
                                  (HOOK_COMMAND(hook, args)
                                   && HOOK_COMMAND(hook, args)[0]) ?
                                  _(HOOK_COMMAND(hook, args)) : ""))
        return 0;
    if (!infolist_new_var_string (item, "args_description",
                                  HOOK_COMMAND(hook, args_description)))
        return 0;
    if (!infolist_new_var_string (item, "args_description_nls",
                                  (HOOK_COMMAND(hook, args_description)
                                   && HOOK_COMMAND(hook, args_description)[0]) ?
                                  _(HOOK_COMMAND(hook, args_description)) : ""))
        return 0;
    if (!infolist_new_var_string (item, "completion", HOOK_COMMAND(hook, completion)))
        return 0;

    return 1;
}

/*
 * Prints command hook data in WeeChat log file (usually for crash dump).
 */

void
hook_command_print_log (struct t_hook *hook)
{
    int i, j;

    if (!hook || !hook->hook_data)
        return;

    log_printf ("  command data:");
    log_printf ("    callback. . . . . . . : 0x%lx", HOOK_COMMAND(hook, callback));
    log_printf ("    command . . . . . . . : '%s'",  HOOK_COMMAND(hook, command));
    log_printf ("    description . . . . . : '%s'",  HOOK_COMMAND(hook, description));
    log_printf ("    args. . . . . . . . . : '%s'",  HOOK_COMMAND(hook, args));
    log_printf ("    args_description. . . : '%s'",  HOOK_COMMAND(hook, args_description));
    log_printf ("    completion. . . . . . : '%s'",  HOOK_COMMAND(hook, completion));
    log_printf ("    cplt_num_templates. . : %d",    HOOK_COMMAND(hook, cplt_num_templates));
    for (i = 0; i < HOOK_COMMAND(hook, cplt_num_templates); i++)
    {
        log_printf ("    cplt_templates[%04d] . . . : '%s'",
                    i, HOOK_COMMAND(hook, cplt_templates)[i]);
        log_printf ("    cplt_templates_static[%04d]: '%s'",
                    i, HOOK_COMMAND(hook, cplt_templates_static)[i]);
        log_printf ("      num_args. . . . . . : %d",
                    HOOK_COMMAND(hook, cplt_template_num_args)[i]);
        for (j = 0; j < HOOK_COMMAND(hook, cplt_template_num_args)[i]; j++)
        {
            log_printf ("      args[%04d]. . . . . : '%s'",
                        j, HOOK_COMMAND(hook, cplt_template_args)[i][j]);
        }
    }
    log_printf ("    num_args_concat . . . : %d", HOOK_COMMAND(hook, cplt_template_num_args_concat));
    for (i = 0; i < HOOK_COMMAND(hook, cplt_template_num_args_concat); i++)
    {
        log_printf ("    args_concat[%04d] . . : '%s'",
                    i, HOOK_COMMAND(hook, cplt_template_args_concat)[i]);
    }
}
