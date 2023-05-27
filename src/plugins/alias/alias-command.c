/*
 * alias-command.c - alias commands
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "alias.h"
#include "alias-config.h"

/*
 * Adds a new alias.
 */

void
alias_command_add (const char *alias_name, const char *command,
                   const char *completion)
{
    struct t_config_option *ptr_option;

    /* define new alias */
    if (!alias_new (alias_name, command, completion))
    {
        weechat_printf (NULL,
                        _("%s%s: error creating alias \"%s\" "
                          "=> \"%s\""),
                        weechat_prefix ("error"), ALIAS_PLUGIN_NAME,
                        alias_name, command);
        return;
    }

    /* create configuration option for command */
    ptr_option = weechat_config_search_option (alias_config_file,
                                               alias_config_section_cmd,
                                               alias_name);
    if (ptr_option)
        weechat_config_option_free (ptr_option);
    alias_config_cmd_new_option (alias_name, command);

    /* create configuration option for completion */
    ptr_option = weechat_config_search_option (alias_config_file,
                                               alias_config_section_completion,
                                               alias_name);
    if (ptr_option)
        weechat_config_option_free (ptr_option);
    if (completion)
        alias_config_completion_new_option (alias_name, completion);

    /* display message */
    weechat_printf (NULL,
                    _("Alias \"%s\" => \"%s\" created"),
                    alias_name, command);
}

/*
 * Callback for command "/alias": displays or creates alias.
 */

int
alias_command_cb (const void *pointer, void *data,
                  struct t_gui_buffer *buffer, int argc,
                  char **argv, char **argv_eol)
{
    char *ptr_alias_name, *ptr_alias_name2;
    struct t_alias *ptr_alias, *ptr_alias2;
    struct t_config_option *ptr_option;
    int alias_found, i;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) buffer;

    /* List all aliases */
    if ((argc == 1) || (weechat_strcmp (argv[1], "list") == 0))
    {
        if (alias_list)
        {
            /* get pointer to alias name */
            ptr_alias_name = NULL;
            if (argc > 1)
            {
                ptr_alias_name = (weechat_string_is_command_char (argv[2])) ?
                    (char *)weechat_utf8_next_char (argv[2]) : argv[2];
            }

            /* display list of aliases */
            alias_found = 0;
            for (ptr_alias = alias_list; ptr_alias;
                 ptr_alias = ptr_alias->next_alias)
            {
                if (!ptr_alias_name
                    || strstr (ptr_alias->name, ptr_alias_name))
                {
                    if (!alias_found)
                    {
                        weechat_printf (NULL, "");
                        if (ptr_alias_name)
                        {
                            weechat_printf (NULL,
                                            _("Aliases with \"%s\":"),
                                            ptr_alias_name);
                        }
                        else
                        {
                            weechat_printf (NULL, _("All aliases:"));
                        }
                    }
                    ptr_option = weechat_config_search_option (
                        alias_config_file,
                        alias_config_section_completion,
                        ptr_alias->name);
                    if (ptr_option)
                    {
                        weechat_printf (
                            NULL,
                            "  %s %s=>%s %s  %s(%s%s %s%s)%s",
                            ptr_alias->name,
                            weechat_color ("chat_delimiters"),
                            weechat_color ("chat"),
                            ptr_alias->command,
                            weechat_color ("chat_delimiters"),
                            weechat_color ("chat"),
                            _("completion:"),
                            weechat_config_string (ptr_option),
                            weechat_color ("chat_delimiters"),
                            weechat_color ("chat"));
                    }
                    else
                    {
                        weechat_printf (
                            NULL,
                            "  %s %s=>%s %s",
                            ptr_alias->name,
                            weechat_color ("chat_delimiters"),
                            weechat_color ("chat"),
                            ptr_alias->command);
                    }
                    alias_found = 1;
                }
            }
            if (!alias_found)
            {
                weechat_printf (NULL, _("No alias found matching \"%s\""),
                                (ptr_alias_name) ? ptr_alias_name : "");
            }
        }
        else
        {
            weechat_printf (NULL, _("No alias defined"));
        }
        return WEECHAT_RC_OK;
    }

    if (weechat_strcmp (argv[1], "add") == 0)
    {
        WEECHAT_COMMAND_MIN_ARGS(4, "add");
        alias_command_add (
            (weechat_string_is_command_char (argv[2])) ?
            (char *)weechat_utf8_next_char (argv[2]) : argv[2],
            argv_eol[3],
            NULL);
        return WEECHAT_RC_OK;
    }

    if (weechat_strcmp (argv[1], "addcompletion") == 0)
    {
        WEECHAT_COMMAND_MIN_ARGS(5, "addcompletion");
        alias_command_add (
            (weechat_string_is_command_char (argv[3])) ?
            (char *)weechat_utf8_next_char (argv[3]) : argv[3],
            argv_eol[4],
            argv[2]);
        return WEECHAT_RC_OK;
    }

    if (weechat_strcmp (argv[1], "del") == 0)
    {
        WEECHAT_COMMAND_MIN_ARGS(3, "del");
        for (i = 2; i < argc; i++)
        {
            ptr_alias_name = (weechat_string_is_command_char (argv[i])) ?
                (char *)weechat_utf8_next_char (argv[i]) : argv[i];
            ptr_alias = alias_search (ptr_alias_name);
            if (!ptr_alias)
            {
                weechat_printf (NULL,
                                _("%sAlias \"%s\" not found"),
                                weechat_prefix ("error"),
                                ptr_alias_name);
            }
            else
            {
                /* remove alias */
                alias_free (ptr_alias);

                /* remove options */
                ptr_option = weechat_config_search_option (
                    alias_config_file,
                    alias_config_section_cmd,
                    ptr_alias_name);
                if (ptr_option)
                    weechat_config_option_free (ptr_option);
                ptr_option = weechat_config_search_option (
                    alias_config_file,
                    alias_config_section_completion,
                    ptr_alias_name);
                if (ptr_option)
                    weechat_config_option_free (ptr_option);

                weechat_printf (NULL,
                                _("Alias \"%s\" removed"),
                                ptr_alias_name);
            }
        }
        return WEECHAT_RC_OK;
    }

    if (weechat_strcmp (argv[1], "rename") == 0)
    {
        WEECHAT_COMMAND_MIN_ARGS(4, "rename");

        ptr_alias_name = (weechat_string_is_command_char (argv[2])) ?
            (char *)weechat_utf8_next_char (argv[2]) : argv[2];
        ptr_alias_name2 = (weechat_string_is_command_char (argv[3])) ?
            (char *)weechat_utf8_next_char (argv[3]) : argv[3];

        ptr_alias = alias_search (ptr_alias_name);
        if (!ptr_alias)
        {
            weechat_printf (NULL,
                            _("%sAlias \"%s\" not found"),
                            weechat_prefix ("error"),
                            ptr_alias_name);
        }
        else
        {
            /* check if target name already exists */
            ptr_alias2 = alias_search (ptr_alias_name2);
            if (ptr_alias2)
            {
                weechat_printf (NULL, _("%sAlias \"%s\" already exists"),
                                weechat_prefix ("error"), ptr_alias_name2);
            }
            else
            {
                /* rename alias */
                if (alias_rename (ptr_alias, ptr_alias_name2))
                {
                    weechat_printf (
                        NULL,
                        _("Alias \"%s\" has been renamed to \"%s\""),
                        ptr_alias_name,
                        ptr_alias_name2);
                }
            }
        }
        return WEECHAT_RC_OK;
    }

    if (weechat_strcmp (argv[1], "missing") == 0)
    {
        for (i = 0; alias_default[i][0]; i++)
        {
            if (!alias_search (alias_default[i][0]))
            {
                alias_command_add (alias_default[i][0],  /* name */
                                   alias_default[i][1],  /* command */
                                   alias_default[i][2]);  /* completion */
            }
        }
        return WEECHAT_RC_OK;
    }

    WEECHAT_COMMAND_ERROR;
}

/*
 * Hooks alias command.
 */

void
alias_command_init ()
{
    weechat_hook_command (
        "alias",
        N_("list, add or remove command aliases"),
        N_("list [<alias>]"
           " || add <alias> [<command>[;<command>...]]"
           " || addcompletion <completion> <alias> [<command>[;<command>...]]"
           " || del <alias> [<alias>...]"
           " || rename <alias> <new_alias>"
           " || missing"),
        /* xgettext:no-c-format */
        N_("         list: list aliases (without argument, this list is "
           "displayed)\n"
           "          add: add an alias\n"
           "addcompletion: add an alias with a custom completion\n"
           "          del: delete an alias\n"
           "       rename: rename an alias\n"
           "      missing: add missing aliases (using default aliases)\n"
           "   completion: completion for alias: by default completion is "
           "done with target command\n"
           "               note: you can use %%command to use completion of "
           "an existing command\n"
           "        alias: name of alias\n"
           "      command: command name with arguments (many commands can be "
           "separated by semicolons)\n"
           "\n"
           "Note: in command, special variables are replaced:\n"
           "        $n: argument 'n' (between 1 and 9)\n"
           "       $-m: arguments from 1 to 'm'\n"
           "       $n-: arguments from 'n' to last\n"
           "      $n-m: arguments from 'n' to 'm'\n"
           "        $*: all arguments\n"
           "        $&: all arguments, with \" replaced by \\\"\n"
           "        $~: last argument\n"
           "      $var: where \"var\" is a local variable of buffer (see "
           "/buffer listvar)\n"
           "            examples: $nick, $channel, $server, $plugin, $name\n"
           "\n"
           "Examples:\n"
           "  alias /split to split window horizontally:\n"
           "    /alias add split /window splith\n"
           "  alias /hello to say \"hello\" on all channels but not on "
           "#weechat:\n"
           "    /alias add hello /allchan -exclude=#weechat hello\n"
           "  rename alias \"hello\" to \"Hello\":\n"
           "    /alias rename hello Hello\n"
           "  alias /forcejoin to send IRC command \"forcejoin\" with "
           "completion of /sajoin:\n"
           "    /alias addcompletion %%sajoin forcejoin /quote forcejoin"),
        "list %(alias)"
        " || add %(alias) %(commands:/)|%(alias_value)"
        " || addcompletion %- %(alias) %(commands:/)|%(alias_value)"
        " || del %(alias)|%*"
        " || rename %(alias) %(alias)"
        " || missing",
        &alias_command_cb, NULL, NULL);
}
