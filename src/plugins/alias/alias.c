/*
 * alias.c - alias plugin for WeeChat: command aliases
 *
 * Copyright (C) 2003-2021 Sébastien Helleu <flashcode@flashtux.org>
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "alias.h"
#include "alias-command.h"
#include "alias-completion.h"
#include "alias-config.h"
#include "alias-info.h"


WEECHAT_PLUGIN_NAME(ALIAS_PLUGIN_NAME);
WEECHAT_PLUGIN_DESCRIPTION(N_("Alias commands"));
WEECHAT_PLUGIN_AUTHOR("Sébastien Helleu <flashcode@flashtux.org>");
WEECHAT_PLUGIN_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_LICENSE(WEECHAT_LICENSE);
WEECHAT_PLUGIN_PRIORITY(11000);

#define ALIAS_IS_ARG_NUMBER(number) ((number >= '1') && (number <= '9'))

struct t_weechat_plugin *weechat_alias_plugin = NULL;

struct t_alias *alias_list = NULL;
struct t_alias *last_alias = NULL;


/*
 * Checks if an alias pointer is valid.
 *
 * Returns:
 *   1: alias exists
 *   0; alias does not exist
 */

int
alias_valid (struct t_alias *alias)
{
    struct t_alias *ptr_alias;

    if (!alias)
        return 0;

    for (ptr_alias = alias_list; ptr_alias;
         ptr_alias = ptr_alias->next_alias)
    {
        if (ptr_alias == alias)
            return 1;
    }

    /* alias not found */
    return 0;
}

/*
 * Searches for an alias by name.
 *
 * Returns pointer to alias found, NULL if not found.
 */

struct t_alias *
alias_search (const char *alias_name)
{
    struct t_alias *ptr_alias;

    for (ptr_alias = alias_list; ptr_alias;
         ptr_alias = ptr_alias->next_alias)
    {
        if (weechat_strcasecmp (alias_name, ptr_alias->name) == 0)
            return ptr_alias;
    }
    return NULL;
}

/*
 * Adds a word to string and increments length.
 */

void
alias_string_add_word (char **alias, int *length, const char *word)
{
    int length_word;
    char *alias2;

    if (!word)
        return;

    length_word = strlen (word);
    if (length_word == 0)
        return;

    if (*alias == NULL)
    {
        *alias = malloc (length_word + 1);
        strcpy (*alias, word);
    }
    else
    {
        alias2 = realloc (*alias, strlen (*alias) + length_word + 1);
        if (!alias2)
        {
            if (*alias)
            {
                free (*alias);
                *alias = NULL;
            }
            return;
        }
        *alias = alias2;
        strcat (*alias, word);
    }
    *length += length_word;
}

/*
 * Adds word (in range) to string and increments length.
 */

void
alias_string_add_word_range (char **alias, int *length, const char *start,
                             const char *end)
{
    char *word;

    word = weechat_strndup (start, end - start);
    if (word)
    {
        alias_string_add_word (alias, length, word);
        free (word);
    }
}

/*
 * Adds some arguments to string and increments length.
 */

void
alias_string_add_arguments (char **alias, int *length, char **argv, int start,
                            int end)
{
    int i;

    for (i = start; i <= end; i++)
    {
        if (i != start)
            alias_string_add_word (alias, length, " ");
        alias_string_add_word (alias, length, argv[i]);
    }
}

/*
 * Replaces arguments in alias.
 *
 * Arguments replaced are (n and m in 1..9):
 *   $n   argument n
 *   $-m  arguments from 1 to m
 *   $n-  arguments from n to last
 *   $n-m arguments from n to m
 *   $*   all arguments
 *   $~   last argument
 *
 * Note: result must be freed after use.
 */

char *
alias_replace_args (const char *alias_args, const char *user_args)
{
    char **argv, *res;
    const char *start, *pos;
    int n, m, argc, length_res, args_count, offset;

    argv = weechat_string_split (user_args, " ", NULL,
                                 WEECHAT_STRING_SPLIT_STRIP_LEFT
                                 | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                 | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                 0, &argc);

    res = NULL;
    length_res = 0;
    args_count = 0;
    start = alias_args;
    pos = start;
    while (pos && pos[0])
    {
        offset = 0;

        if ((pos[0] == '\\') && (pos[1] == '$'))
        {
            offset = 2;
            alias_string_add_word_range (&res, &length_res, start, pos);
            alias_string_add_word (&res, &length_res, "$");
        }
        else
        {
            if (pos[0] == '$')
            {
                if (pos[1] == '*')
                {
                    /* replace with all arguments */
                    args_count++;
                    offset = 2;
                    if (pos > start)
                        alias_string_add_word_range (&res, &length_res, start, pos);
                    alias_string_add_word (&res, &length_res, user_args);
                }
                else if (pos[1] == '~')
                {
                    /* replace with last argument */
                    args_count++;
                    offset = 2;
                    if (pos > start)
                        alias_string_add_word_range (&res, &length_res, start, pos);
                    if (argc > 0)
                        alias_string_add_word (&res, &length_res, argv[argc - 1]);
                }
                else if ((pos[1] == '-') && ALIAS_IS_ARG_NUMBER(pos[2]))
                {
                    /* replace with arguments 1 to m */
                    args_count++;
                    offset = 3;
                    if (pos > start)
                        alias_string_add_word_range (&res, &length_res, start, pos);
                    if (pos[2] - '1' < argc)
                        m = pos[2] - '1';
                    else
                        m = argc - 1;
                    alias_string_add_arguments (&res, &length_res, argv, 0, m);
                }
                else if (ALIAS_IS_ARG_NUMBER(pos[1]))
                {
                    args_count++;
                    n = pos[1] - '1';
                    if (pos > start)
                        alias_string_add_word_range (&res, &length_res, start, pos);
                    if (pos[2] != '-')
                    {
                        /* replace with argument n */
                        offset = 2;
                        if (n < argc)
                            alias_string_add_word (&res, &length_res, argv[n]);
                    }
                    else
                    {
                        if (ALIAS_IS_ARG_NUMBER(pos[3]))
                        {
                            /* replace with arguments n to m */
                            offset = 4;
                            if (pos[3] - '1' < argc)
                                m = pos[3] - '1';
                            else
                                m = argc - 1;
                        }
                        else
                        {
                            /* replace with arguments n to last */
                            offset = 3;
                            m = argc - 1;
                        }
                        if (n < argc)
                        {
                            alias_string_add_arguments (&res, &length_res,
                                                        argv, n, m);
                        }
                    }
                }
            }
        }

        if (offset != 0)
        {
            pos += offset;
            start = pos;
        }
        else
            pos++;
    }

    if (pos > start)
        alias_string_add_word (&res, &length_res, start);

    if (argv)
        weechat_string_free_split (argv);

    return res;
}

/*
 * Replaces local buffer variables in string, then runs command on buffer.
 */

void
alias_run_command (struct t_gui_buffer **buffer, const char *command)
{
    char *string;
    struct t_gui_buffer *old_current_buffer, *new_current_buffer;

    /* save current buffer pointer */
    old_current_buffer = weechat_current_buffer ();

    /* execute command */
    string = weechat_buffer_string_replace_local_var (*buffer, command);
    weechat_command (*buffer,
                     (string) ? string : command);
    if (string)
        free (string);

    /* get new current buffer */
    new_current_buffer = weechat_current_buffer ();

    /*
     * if current buffer was changed by command, then we'll use this one for
     * next commands in alias
     */
    if (old_current_buffer != new_current_buffer)
        *buffer = new_current_buffer;
}

/*
 * Callback for alias: called when user uses an alias.
 */

int
alias_cb (const void *pointer, void *data,
          struct t_gui_buffer *buffer, int argc, char **argv,
          char **argv_eol)
{
    struct t_alias *ptr_alias;
    char **commands, **ptr_cmd, **ptr_next_cmd;
    char *args_replaced, *alias_command;
    int some_args_replaced, length1, length2;

    /* make C compiler happy */
    (void) data;
    (void) argv;

    ptr_alias = (struct t_alias *)pointer;

    if (ptr_alias->running)
    {
        weechat_printf (NULL,
                        _("%s%s: error, circular reference when calling "
                          "alias \"%s\""),
                        weechat_prefix ("error"), ALIAS_PLUGIN_NAME,
                        ptr_alias->name);
        return WEECHAT_RC_OK;
    }
    else
    {
        /* an alias can contain many commands separated by ';' */
        commands = weechat_string_split_command (ptr_alias->command, ';');
        if (commands)
        {
            some_args_replaced = 0;
            ptr_alias->running = 1;
            for (ptr_cmd = commands; *ptr_cmd; ptr_cmd++)
            {
                ptr_next_cmd = ptr_cmd;
                ptr_next_cmd++;

                args_replaced = alias_replace_args (*ptr_cmd,
                                                    (argc > 1) ? argv_eol[1] : "");
                if (args_replaced && (strcmp (args_replaced, *ptr_cmd) != 0))
                    some_args_replaced = 1;

                /*
                 * if alias has arguments, they are now
                 * arguments of the last command in the list (if no $1,$2,..$*)
                 * was found
                 */
                if ((*ptr_next_cmd == NULL) && argv_eol[1] && (!some_args_replaced))
                {
                    length1 = strlen (*ptr_cmd);
                    length2 = strlen (argv_eol[1]);

                    alias_command = malloc (1 + length1 + 1 + length2 + 1);
                    if (alias_command)
                    {
                        if (!weechat_string_is_command_char (*ptr_cmd))
                            strcpy (alias_command, "/");
                        else
                            alias_command[0] = '\0';

                        strcat (alias_command, *ptr_cmd);
                        strcat (alias_command, " ");
                        strcat (alias_command, argv_eol[1]);

                        alias_run_command (&buffer,
                                           alias_command);
                        free (alias_command);
                    }
                }
                else
                {
                    if (weechat_string_is_command_char (*ptr_cmd))
                    {
                        alias_run_command (&buffer,
                                           (args_replaced) ? args_replaced : *ptr_cmd);
                    }
                    else
                    {
                        alias_command = malloc (1 + strlen ((args_replaced) ? args_replaced : *ptr_cmd) + 1);
                        if (alias_command)
                        {
                            strcpy (alias_command, "/");
                            strcat (alias_command, (args_replaced) ? args_replaced : *ptr_cmd);
                            alias_run_command (&buffer,
                                               alias_command);
                            free (alias_command);
                        }
                    }
                }

                if (args_replaced)
                    free (args_replaced);
            }
            ptr_alias->running = 0;
            weechat_string_free_split_command (commands);
        }
    }
    return WEECHAT_RC_OK;
}

/*
 * Frees an alias and remove it from list.
 */

void
alias_free (struct t_alias *alias)
{
    struct t_alias *new_alias_list;

    if (!alias)
        return;

    /* remove alias from list */
    if (last_alias == alias)
        last_alias = alias->prev_alias;
    if (alias->prev_alias)
    {
        (alias->prev_alias)->next_alias = alias->next_alias;
        new_alias_list = alias_list;
    }
    else
        new_alias_list = alias->next_alias;
    if (alias->next_alias)
        (alias->next_alias)->prev_alias = alias->prev_alias;

    /* free data */
    if (alias->hook)
        weechat_unhook (alias->hook);
    if (alias->name)
        free (alias->name);
    if (alias->command)
        free (alias->command);
    if (alias->completion)
        free (alias->completion);
    free (alias);

    alias_list = new_alias_list;
}

/*
 * Frees all aliases.
 */

void
alias_free_all ()
{
    while (alias_list)
    {
        alias_free (alias_list);
    }
}

/*
 * Searches for position of alias (to keep aliases sorted by name).
 */

struct t_alias *
alias_find_pos (const char *name)
{
    struct t_alias *ptr_alias;

    for (ptr_alias = alias_list; ptr_alias; ptr_alias = ptr_alias->next_alias)
    {
        if (weechat_strcasecmp (name, ptr_alias->name) < 0)
            return ptr_alias;
    }

    /* position not found (we will add to the end of list) */
    return NULL;
}

/*
 * Hooks command for an alias.
 */

void
alias_hook_command (struct t_alias *alias)
{
    char *str_priority_name, *str_completion;
    int length;

    /*
     * build string with priority and name: the alias priority is 2000, which
     * is higher than default one (1000), so the alias is executed before a
     * command (if a command with same name exists in core or in another plugin)
     */
    length = strlen (alias->name) + 16 + 1;
    str_priority_name = malloc (length);
    if (str_priority_name)
        snprintf (str_priority_name, length, "2000|%s", alias->name);

    /*
     * if alias has no custom completion, then default is to complete with
     * completion template of target command, for example if alias is
     * "/alias add test /buffer", then str_completion will be "%%buffer"
     */
    str_completion = NULL;
    if (!alias->completion)
    {
        length = 2 + strlen (alias->command) + 1;
        str_completion = malloc (length);
        if (str_completion)
        {
            snprintf (str_completion, length, "%%%%%s",
                      (weechat_string_is_command_char (alias->command)) ?
                      weechat_utf8_next_char (alias->command) : alias->command);
        }
    }

    alias->hook = weechat_hook_command ((str_priority_name) ? str_priority_name : alias->name,
                                        alias->command,
                                        NULL, NULL,
                                        (str_completion) ? str_completion : alias->completion,
                                        &alias_cb, alias, NULL);

    if (str_priority_name)
        free (str_priority_name);
    if (str_completion)
        free (str_completion);
}

/*
 * Updates completion for an alias.
 */

void
alias_update_completion (struct t_alias *alias, const char *completion)
{
    /* update completion in alias */
    if (alias->completion)
        free (alias->completion);
    alias->completion = (completion) ? strdup (completion) : NULL;

    /* unhook and hook again command, with new completion */
    weechat_unhook (alias->hook);
    alias->hook = NULL;
    alias_hook_command (alias);
}

/*
 * Checks if an alias name is valid: it must not contain any slashes nor
 * any spaces.
 *
 * Returns:
 *   1: name is valid
 *   0: name is invalid
 */

int
alias_name_valid (const char *name)
{
    if (!name || !name[0])
        return 0;

    /* no spaces allowed */
    if (strchr (name, ' '))
        return 0;

    /* no slashes allowed */
    if (strchr (name, '/'))
        return 0;

    /* name is valid */
    return 1;
}

/*
 * Creates a new alias and adds it to alias list.
 *
 * Returns pointer to new alias, NULL if error.
 */

struct t_alias *
alias_new (const char *name, const char *command, const char *completion)
{
    struct t_alias *new_alias, *ptr_alias, *pos_alias;

    if (!alias_name_valid (name))
    {
        weechat_printf (NULL,
                        _("%s%s: invalid alias name: \"%s\""),
                        weechat_prefix ("error"), ALIAS_PLUGIN_NAME,
                        name);
        return NULL;
    }

    if (!command || !command[0])
        return NULL;

    while (weechat_string_is_command_char (name))
    {
        name = weechat_utf8_next_char (name);
    }

    ptr_alias = alias_search (name);
    if (ptr_alias)
        alias_free (ptr_alias);

    new_alias = malloc (sizeof (*new_alias));
    if (new_alias)
    {
        new_alias->hook = NULL;
        new_alias->name = strdup (name);
        new_alias->command = strdup (command);
        new_alias->completion = (completion) ? strdup (completion) : NULL;
        new_alias->running = 0;

        alias_hook_command (new_alias);

        if (alias_list)
        {
            pos_alias = alias_find_pos (name);
            if (pos_alias)
            {
                /* insert alias into the list (before alias found) */
                new_alias->prev_alias = pos_alias->prev_alias;
                new_alias->next_alias = pos_alias;
                if (pos_alias->prev_alias)
                    (pos_alias->prev_alias)->next_alias = new_alias;
                else
                    alias_list = new_alias;
                pos_alias->prev_alias = new_alias;
            }
            else
            {
                /* add alias to end of list */
                new_alias->prev_alias = last_alias;
                new_alias->next_alias = NULL;
                last_alias->next_alias = new_alias;
                last_alias = new_alias;
            }
        }
        else
        {
            new_alias->prev_alias = NULL;
            new_alias->next_alias = NULL;
            alias_list = new_alias;
            last_alias = new_alias;
        }
    }

    return new_alias;
}

/*
 * Adds an alias in an infolist.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
alias_add_to_infolist (struct t_infolist *infolist, struct t_alias *alias)
{
    struct t_infolist_item *ptr_item;

    if (!infolist || !alias)
        return 0;

    ptr_item = weechat_infolist_new_item (infolist);
    if (!ptr_item)
        return 0;

    if (!weechat_infolist_new_var_pointer (ptr_item, "hook", alias->hook))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "name", alias->name))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "command", alias->command))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "completion", alias->completion))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "running", alias->running))
        return 0;

    return 1;
}

/*
 * Initializes alias plugin.
 */

int
weechat_plugin_init (struct t_weechat_plugin *plugin, int argc, char *argv[])
{
    /* make C compiler happy */
    (void) argc;
    (void) argv;

    weechat_plugin = plugin;

    if (!alias_config_init ())
        return WEECHAT_RC_ERROR;

    alias_config_read ();

    alias_command_init ();

    alias_completion_init ();

    alias_info_init ();

    return WEECHAT_RC_OK;
}

/*
 * Ends alias plugin.
 */

int
weechat_plugin_end (struct t_weechat_plugin *plugin)
{
    /* make C compiler happy */
    (void) plugin;

    alias_config_write ();
    alias_free_all ();
    weechat_config_free (alias_config_file);

    return WEECHAT_RC_OK;
}
