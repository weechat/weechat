/*
 * Copyright (c) 2003-2005 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* command.c: WeeChat internal commands */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "weechat.h"
#include "command.h"
#include "weelist.h"
#include "weeconfig.h"
#include "../irc/irc.h"
#include "../gui/gui.h"
#include "../plugins/plugins.h"


/* WeeChat internal commands */

t_weechat_command weechat_commands[] =
{ { "alias", N_("create an alias for a command"),
    N_("[alias_name [command [arguments]]"),
    N_("alias_name: name of alias\ncommand: command name (WeeChat "
    "or IRC command, without first '/')\n" "arguments: arguments for command"),
    0, MAX_ARGS, NULL, weechat_cmd_alias },
  { "buffer", N_("manage buffers"),
    N_("[action | number]"),
    N_("action: action to do:\n"
    "  move    move buffer in the list (may be relative, for example -1)\n"
    "  close   close buffer (for channel: same as /part without part message)\n"
    "  list    list opened buffers (no parameter implies this list)\n"
    "  notify  set notify level for buffer (0=never, 1=highlight, 2=1+msg, 3=2+join/part)\n"
    "number: jump to buffer by number"),
    0, MAX_ARGS, weechat_cmd_buffer, NULL },
  { "clear", N_("clear window(s)"),
    N_("[-all]"),
    N_("-all: clear all windows"),
    0, 1, weechat_cmd_clear, NULL },
  { "connect", N_("connect to a server"),
    N_("[servername]"),
    N_("servername: server name to connect"),
    0, 1, weechat_cmd_connect, NULL },
  { "disconnect", N_("disconnect from a server"),
    N_("[servername]"),
    N_("servername: server name to disconnect"),
    0, 1, weechat_cmd_disconnect, NULL },
  { "help", N_("display help about commands"),
    N_("[command]"), N_("command: name of a WeeChat or IRC command"),
    0, 1, weechat_cmd_help, NULL },
  { "perl", N_("list/load/unload Perl scripts"),
    N_("[load filename] | [autoload] | [unload]"),
    N_("filename: Perl script (file) to load\n"
    "Without argument, /perl command lists all loaded Perl scripts."),
    0, 2, weechat_cmd_perl, NULL },
  { "server", N_("list, add or remove servers"),
    N_("[servername] | "
    "[servername hostname port [-auto | -noauto] [-pwd password] [-nicks nick1 "
    "[nick2 [nick3]]] [-username username] [-realname realname] "
    "[-command command] [-autojoin channel[,channel]] ] | "
    "[del servername]"),
    N_("servername: server name, for internal & display use\n"
    "hostname: name or IP address of server\n"
    "port: port for server (integer)\n"
    "password: password for server\n"
    "nick1: first nick for server\n"
    "nick2: alternate nick for server\n"
    "nick3: second alternate nick for server\n"
    "username: user name\n"
    "realname: real name of user"),
    0, MAX_ARGS, weechat_cmd_server, NULL },
  { "save", N_("save config to disk"),
    N_("[file]"), N_("file: filename for writing config"),
    0, 1, weechat_cmd_save, NULL },
  { "set", N_("set config parameters"),
    N_("[option[=value]]"), N_("option: name of an option\nvalue: value for option"),
    0, MAX_ARGS, NULL, weechat_cmd_set },
  { "unalias", N_("remove an alias"),
    N_("alias_name"), N_("alias_name: name of alias to remove"),
    1, 1, NULL, weechat_cmd_unalias },
  { "window", N_("manage windows"),
    N_("[list | splith | splitv | [merge [down | up | left | right | all]]]"),
    N_(
    "  list  list opened windows (no parameter implies this list)\n"
    "splith  split current window horizontally\n"
    "splitv  split current window vertically\n"
    " merge  merge window with another"),
    0, 2, weechat_cmd_window, NULL },
  { NULL, NULL, NULL, NULL, 0, 0, NULL, NULL }
};

t_weechat_alias *weechat_alias = NULL;
t_weechat_alias *weechat_last_alias = NULL;

t_weelist *index_commands;
t_weelist *last_index_command;


/*
 * command_index_build: build an index of commands (internal, irc and alias)
 *                      This list will be sorted, and used for completion
 */

void
command_index_build ()
{
    int i;
    
    index_commands = NULL;
    last_index_command = NULL;
    i = 0;
    while (weechat_commands[i].command_name)
    {
        (void) weelist_add (&index_commands, &last_index_command,
                            weechat_commands[i].command_name);
        i++;
    }
    i = 0;
    while (irc_commands[i].command_name)
    {
        if (irc_commands[i].cmd_function_args || irc_commands[i].cmd_function_1arg)
            (void) weelist_add (&index_commands, &last_index_command,
                                irc_commands[i].command_name);
        i++;
    }
}

/*
 * command_index_free: remove all commands in index
 */

void
command_index_free ()
{
    while (index_commands)
    {
        weelist_remove (&index_commands, &last_index_command, index_commands);
    }
}

/*
 * alias_search: search an alias
 */

t_weechat_alias *
alias_search (char *alias_name)
{
    t_weechat_alias *ptr_alias;
    
    for (ptr_alias = weechat_alias; ptr_alias; ptr_alias = ptr_alias->next_alias)
    {
        if (strcasecmp (alias_name, ptr_alias->alias_name) == 0)
            return ptr_alias;
    }
    return NULL;
}

/*
 * alias_find_pos: find position for an alias (for sorting aliases)
 */

t_weechat_alias *
alias_find_pos (char *alias_name)
{
    t_weechat_alias *ptr_alias;
    
    for (ptr_alias = weechat_alias; ptr_alias; ptr_alias = ptr_alias->next_alias)
    {
        if (strcasecmp (alias_name, ptr_alias->alias_name) < 0)
            return ptr_alias;
    }
    return NULL;
}

/*
 * alias_insert_sorted: insert alias into sorted list
 */

void
alias_insert_sorted (t_weechat_alias *alias)
{
    t_weechat_alias *pos_alias;
    
    pos_alias = alias_find_pos (alias->alias_name);
    
    if (weechat_alias)
    {
        if (pos_alias)
        {
            /* insert alias into the list (before alias found) */
            alias->prev_alias = pos_alias->prev_alias;
            alias->next_alias = pos_alias;
            if (pos_alias->prev_alias)
                pos_alias->prev_alias->next_alias = alias;
            else
                weechat_alias = alias;
            pos_alias->prev_alias = alias;
        }
        else
        {
            /* add alias to the end */
            alias->prev_alias = weechat_last_alias;
            alias->next_alias = NULL;
            weechat_last_alias->next_alias = alias;
            weechat_last_alias = alias;
        }
    }
    else
    {
        alias->prev_alias = NULL;
        alias->next_alias = NULL;
        weechat_alias = alias;
        weechat_last_alias = alias;
    }
}

/*
 * alias_new: create new alias and add it to alias list
 */

t_weechat_alias *
alias_new (char *alias_name, char *alias_command)
{
    char *pos;
    t_weechat_alias *new_alias;
    
    if (weelist_search (index_commands, alias_name))
    {
        irc_display_prefix (NULL, PREFIX_ERROR);
        gui_printf (NULL, _("%s alias or command \"%s\" already exists!\n"),
                    WEECHAT_ERROR, alias_name);
        return NULL;
    }
    pos = strchr (alias_command, ' ');
    if (pos)
        pos[0] = '\0';
    if (alias_search (alias_command))
    {
        irc_display_prefix (NULL, PREFIX_ERROR);
        gui_printf (NULL, _("%s alias cannot run another alias!\n"),
                    WEECHAT_ERROR);
        return NULL;
    }
    if (!weelist_search (index_commands, alias_command))
    {
        irc_display_prefix (NULL, PREFIX_ERROR);
        gui_printf (NULL, _("%s target command \"%s\" does not exist!\n"),
                    WEECHAT_ERROR, alias_command);
        return NULL;
    }
    if (pos)
        pos[0] = ' ';
    
    if ((new_alias = ((t_weechat_alias *) malloc (sizeof (t_weechat_alias)))))
    {
        new_alias->alias_name = strdup (alias_name);
        new_alias->alias_command = (char *)malloc (strlen (alias_command) + 2);
        if (new_alias->alias_command)
        {
            new_alias->alias_command[0] = '/';
            strcpy (new_alias->alias_command + 1, alias_command);
        }
        alias_insert_sorted (new_alias);
        return new_alias;
    }
    else
        return NULL;
}

/*
 * alias_free: free an alias and reomve it from list
 */

void
alias_free (t_weechat_alias *alias)
{
    t_weechat_alias *new_weechat_alias;

    /* remove alias from list */
    if (weechat_last_alias == alias)
        weechat_last_alias = alias->prev_alias;
    if (alias->prev_alias)
    {
        (alias->prev_alias)->next_alias = alias->next_alias;
        new_weechat_alias = weechat_alias;
    }
    else
        new_weechat_alias = alias->next_alias;
    
    if (alias->next_alias)
        (alias->next_alias)->prev_alias = alias->prev_alias;

    /* free data */
    if (alias->alias_name)
        free (alias->alias_name);
    if (alias->alias_command)
        free (alias->alias_command);
    free (alias);
    weechat_alias = new_weechat_alias;
}

/*
 * alias_free_all: free all alias
 */

void
alias_free_all ()
{
    while (weechat_alias)
        alias_free (weechat_alias);
}

/*
 * explode_string: explode a string according to separators
 */

char **
explode_string (/*@null@*/ char *string, char *separators, int num_items_max,
                int *num_items)
{
    int i, n_items;
    char **array;
    char *ptr, *ptr1, *ptr2;

    if (num_items != NULL)
        *num_items = 0;

    n_items = num_items_max;

    if (string == NULL)
        return NULL;

    if (num_items_max == 0)
    {
        /* calculate number of items */
        ptr = string;
        i = 1;
        while ((ptr = strpbrk (ptr, separators)))
        {
            while (strchr (separators, ptr[0]) != NULL)
                ptr++;
            i++;
        }
        n_items = i;
    }

    array =
        (char **) malloc ((num_items_max ? n_items : n_items + 1) *
                          sizeof (char *));

    ptr1 = string;
    ptr2 = string;

    for (i = 0; i < n_items; i++)
    {
        while (strchr (separators, ptr1[0]) != NULL)
            ptr1++;
        if (i == (n_items - 1) || (ptr2 = strpbrk (ptr1, separators)) == NULL)
            if ((ptr2 = strchr (ptr1, '\r')) == NULL)
                if ((ptr2 = strchr (ptr1, '\n')) == NULL)
                    ptr2 = strchr (ptr1, '\0');

        if ((ptr1 == NULL) || (ptr2 == NULL))
        {
            array[i] = NULL;
        }
        else
        {
            if (ptr2 - ptr1 > 0)
            {
                array[i] =
                    (char *) malloc ((ptr2 - ptr1 + 1) * sizeof (char));
                array[i] = strncpy (array[i], ptr1, ptr2 - ptr1);
                array[i][ptr2 - ptr1] = '\0';
                ptr1 = ++ptr2;
            }
            else
            {
                array[i] = NULL;
            }
        }
    }
    if (num_items_max == 0)
    {
        array[i] = NULL;
        if (num_items != NULL)
            *num_items = i;
    }
    else
    {
        if (num_items != NULL)
            *num_items = num_items_max;
    }

    return array;
}

/*
 * exec_weechat_command: executes a command (WeeChat internal or IRC)
 *                       returns: 1 if command was executed succesfully
 *                                0 if error (command not executed)
 */

int
exec_weechat_command (t_irc_server *server, char *string)
{
    int i, j, argc, return_code, length1, length2;
    char *command, *pos, *ptr_args, **argv, *alias_command;
    t_weechat_alias *ptr_alias;

    if ((!string) || (!string[0]) || (string[0] != '/'))
        return 0;

    command = strdup (string);
    
    /* look for end of command */
    ptr_args = NULL;
    
    pos = &command[strlen (command) - 1];
    if (pos[0] == ' ')
    {
        while ((pos > command) && (pos[0] == ' '))
            pos--;
        pos[1] = '\0';
    }
    
    pos = strchr (command, ' ');
    if (pos)
    {
        pos[0] = '\0';
        pos++;
        while (pos[0] == ' ')
            pos++;
        ptr_args = pos;
        if (!ptr_args[0])
            ptr_args = NULL;
    }
    
    if (!plugin_exec_command (command + 1, (server) ? server->name : "", ptr_args))
    {
        argv = explode_string (ptr_args, " ", 0, &argc);
        
        for (i = 0; weechat_commands[i].command_name; i++)
        {
            if (strcasecmp (weechat_commands[i].command_name, command + 1) == 0)
            {
                if ((argc < weechat_commands[i].min_arg)
                    || (argc > weechat_commands[i].max_arg))
                {
                    if (weechat_commands[i].min_arg ==
                        weechat_commands[i].max_arg)
                    {
                        irc_display_prefix (NULL, PREFIX_ERROR);
                        gui_printf (NULL,
                                    _("%s wrong argument count for %s command \"%s\" "
                                    "(expected: %d arg%s)\n"),
                                    WEECHAT_ERROR, PACKAGE_NAME,
                                    command + 1,
                                    weechat_commands[i].max_arg,
                                    (weechat_commands[i].max_arg >
                                     1) ? "s" : "");
                    }
                    else
                    {
                        irc_display_prefix (NULL, PREFIX_ERROR);
                        gui_printf (NULL,
                                    _("%s wrong argument count for %s command \"%s\" "
                                    "(expected: between %d and %d arg%s)\n"),
                                    WEECHAT_ERROR, PACKAGE_NAME,
                                    command + 1,
                                    weechat_commands[i].min_arg,
                                    weechat_commands[i].max_arg,
                                    (weechat_commands[i].max_arg >
                                     1) ? "s" : "");
                    }
                }
                else
                {
                    if (weechat_commands[i].cmd_function_args)
                        return_code = (int) (weechat_commands[i].cmd_function_args)
                                            (argc, argv);
                    else
                        return_code = (int) (weechat_commands[i].cmd_function_1arg)
                                            (ptr_args);
                    if (return_code < 0)
                    {
                        irc_display_prefix (NULL, PREFIX_ERROR);
                        gui_printf (NULL,
                                    _("%s command \"%s\" failed\n"),
                                    WEECHAT_ERROR, command + 1);
                    }
                }
                if (argv)
                {
                    for (j = 0; argv[j]; j++)
                        free (argv[j]);
                    free (argv);
                }
                free (command);
                return 1;
            }
        }
        for (i = 0; irc_commands[i].command_name; i++)
        {
            if ((strcasecmp (irc_commands[i].command_name, command + 1) == 0) &&
                ((irc_commands[i].cmd_function_args) ||
                (irc_commands[i].cmd_function_1arg)))
            {
                if ((argc < irc_commands[i].min_arg)
                    || (argc > irc_commands[i].max_arg))
                {
                    if (irc_commands[i].min_arg == irc_commands[i].max_arg)
                    {
                        irc_display_prefix (NULL, PREFIX_ERROR);
                        gui_printf
                            (NULL,
                             _("%s wrong argument count for IRC command \"%s\" "
                             "(expected: %d arg%s)\n"),
                             WEECHAT_ERROR,
                             command + 1,
                             irc_commands[i].max_arg,
                             (irc_commands[i].max_arg > 1) ? "s" : "");
                    }
                    else
                    {
                        irc_display_prefix (NULL, PREFIX_ERROR);
                        gui_printf
                            (NULL,
                             _("%s wrong argument count for IRC command \"%s\" "
                             "(expected: between %d and %d arg%s)\n"),
                             WEECHAT_ERROR,
                             command + 1,
                             irc_commands[i].min_arg, irc_commands[i].max_arg,
                             (irc_commands[i].max_arg > 1) ? "s" : "");
                    }
                }
                else
                {
                    if ((irc_commands[i].need_connection) &&
                        ((!server) || (!server->is_connected)))
                    {
                        irc_display_prefix (NULL, PREFIX_ERROR);
                        gui_printf (NULL,
                                    _("%s command \"%s\" needs a server connection!\n"),
                                    WEECHAT_ERROR, irc_commands[i].command_name);
                        free (command);
                        return 0;
                    }
                    if (irc_commands[i].cmd_function_args)
                        return_code = (int) (irc_commands[i].cmd_function_args)
                                      (server, argc, argv);
                    else
                        return_code = (int) (irc_commands[i].cmd_function_1arg)
                                      (server, ptr_args);
                    if (return_code < 0)
                    {
                        irc_display_prefix (NULL, PREFIX_ERROR);
                        gui_printf (NULL,
                                    _("%s command \"%s\" failed\n"),
                                    WEECHAT_ERROR, command + 1);
                    }
                }
                if (argv)
                {
                    for (j = 0; argv[j]; j++)
                        free (argv[j]);
                    free (argv);
                }
                free (command);
                return 1;
            }
        }
        for (ptr_alias = weechat_alias; ptr_alias;
             ptr_alias = ptr_alias->next_alias)
        {
            if (strcasecmp (ptr_alias->alias_name, command + 1) == 0)
            {
                if (ptr_args)
                {
                    length1 = strlen (ptr_alias->alias_command);
                    length2 = strlen (ptr_args);
                    alias_command = (char *)malloc (length1 + 1 + length2 + 1);
                    if (alias_command)
                    {
                        strcpy (alias_command, ptr_alias->alias_command);
                        alias_command[length1] = ' ';
                        strcpy (alias_command + length1 + 1, ptr_args);
                    }
                    (void) exec_weechat_command (server, alias_command);
                    if (alias_command)
                        free (alias_command);
                }
                else
                    (void) exec_weechat_command (server, ptr_alias->alias_command);
                
                if (argv)
                {
                    for (j = 0; argv[j]; j++)
                        free (argv[j]);
                    free (argv);
                }
                free (command);
                return 1;
            }
        }
        irc_display_prefix (NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("%s unknown command \"%s\" (type /help for help)\n"),
                    WEECHAT_ERROR,
                    command + 1);
        if (argv)
        {
            for (j = 0; argv[j]; j++)
                free (argv[j]);
            free (argv);
        }
    }
    free (command);
    return 0;
}

/*
 * user_command: interprets user command (if beginning with '/')
 *               any other text is sent to the server, if connected
 */

void
user_command (t_irc_server *server, char *command)
{
    t_irc_nick *ptr_nick;
    
    if ((!command) || (!command[0]) || (command[0] == '\r') || (command[0] == '\n'))
        return;
    if ((command[0] == '/') && (command[1] != '/'))
    {
        /* WeeChat internal command (or IRC command) */
        (void) exec_weechat_command (server, command);
    }
    else
    {
        if ((command[0] == '/') && (command[1] == '/'))
            command++;
        if (server && (!BUFFER_IS_SERVER(gui_current_window->buffer)))
        {
            server_sendf (server, "PRIVMSG %s :%s\r\n",
                          CHANNEL(gui_current_window->buffer)->name,
                          command);

            if (BUFFER_IS_PRIVATE(gui_current_window->buffer))
            {
                gui_printf_color_type (CHANNEL(gui_current_window->buffer)->buffer,
                                       MSG_TYPE_NICK,
                                       COLOR_WIN_CHAT_DARK, "<");
                gui_printf_color_type (CHANNEL(gui_current_window->buffer)->buffer,
                                       MSG_TYPE_NICK,
                                       COLOR_WIN_NICK_SELF,
                                       "%s", server->nick);
                gui_printf_color_type (CHANNEL(gui_current_window->buffer)->buffer,
                                       MSG_TYPE_NICK,
                                       COLOR_WIN_CHAT_DARK, "> ");
                gui_printf_color_type (CHANNEL(gui_current_window->buffer)->buffer,
                                       MSG_TYPE_MSG,
                                       COLOR_WIN_CHAT, "%s\n", command);
            }
            else
            {
                ptr_nick = nick_search (CHANNEL(gui_current_window->buffer), server->nick);
                if (ptr_nick)
                {
                    irc_display_nick (CHANNEL(gui_current_window->buffer)->buffer, ptr_nick,
                                      MSG_TYPE_NICK, 1, 1, 0);
                    gui_printf_color (CHANNEL(gui_current_window->buffer)->buffer,
                                      COLOR_WIN_CHAT, "%s\n", command);
                }
                else
                {
                    irc_display_prefix (server->buffer, PREFIX_ERROR);
                    gui_printf (server->buffer,
                                _("%s cannot find nick for sending message\n"),
                                WEECHAT_ERROR);
                }
            }
        }
        else
        {
            irc_display_prefix ((server) ? server->buffer : NULL, PREFIX_ERROR);
            gui_printf_nolog ((server) ? server->buffer : NULL,
                              _("This window is not a channel!\n"));
        }
    }
}

/*
 * weechat_cmd_alias: display or create alias
 */

int
weechat_cmd_alias (char *arguments)
{
    char *pos;
    t_weechat_alias *ptr_alias;
    
    if (arguments && arguments[0])
    {
        /* Define new alias */
        pos = strchr (arguments, ' ');
        if (pos)
        {
            pos[0] = '\0';
            pos++;
            while (pos[0] == ' ')
                pos++;
            if (!pos[0])
            {
                irc_display_prefix (NULL, PREFIX_ERROR);
                gui_printf (NULL, _("%s missing arguments for \"%s\" command\n"),
                            WEECHAT_ERROR, "alias");
                return -1;
            }
            if (!alias_new (arguments, pos))
                return -1;
            if (weelist_add (&index_commands, &last_index_command, arguments))
            {
                irc_display_prefix (NULL, PREFIX_INFO);
                gui_printf (NULL, _("Alias \"%s\" => \"%s\" created\n"),
                            arguments, pos);
            }
            else
            {
                irc_display_prefix (NULL, PREFIX_ERROR);
                gui_printf (NULL, _("Failed to create alias \"%s\" => \"%s\" "
                            "(not enough memory)\n"),
                            arguments, pos);
                return -1;
            }
        }
        else
        {
            irc_display_prefix (NULL, PREFIX_ERROR);
            gui_printf (NULL, _("%s missing arguments for \"%s\" command\n"),
                        WEECHAT_ERROR, "alias");
            return -1;
        }
    }
    else
    {
        /* List all aliases */
        if (weechat_alias)
        {
            gui_printf (NULL, "\n");
            gui_printf (NULL, _("List of aliases:\n"));
            for (ptr_alias = weechat_alias; ptr_alias;
                 ptr_alias = ptr_alias->next_alias)
            {
                gui_printf (NULL, "  %s => %s\n",
                            ptr_alias->alias_name,
                            ptr_alias->alias_command + 1);
            }
        }
        else
        {
            irc_display_prefix (NULL, PREFIX_INFO);
            gui_printf (NULL, _("No alias defined.\n"));
        }
    }
    return 0;
}

/*
 * weechat_cmd_buffer_display_info: display info about a buffer
 */

void
weechat_cmd_buffer_display_info (t_gui_buffer *buffer)
{
    if (buffer->dcc)
        gui_printf_color (NULL, COLOR_WIN_CHAT_CHANNEL, "DCC\n");
    else if (BUFFER_IS_SERVER (buffer))
    {
        gui_printf (NULL, _("Server: "));
        gui_printf_color (NULL, COLOR_WIN_CHAT_CHANNEL,
                          "%s\n", SERVER(buffer)->name);
    }
    else if (BUFFER_IS_CHANNEL (buffer))
    {
        gui_printf (NULL, _("Channel: "));
        gui_printf_color (NULL, COLOR_WIN_CHAT_CHANNEL,
                          "%s", CHANNEL(buffer)->name);
        gui_printf (NULL, _(" (server: "));
        gui_printf_color (NULL, COLOR_WIN_CHAT_CHANNEL,
                          "%s", SERVER(buffer)->name);
        gui_printf (NULL, ")\n");
    }
    else if (BUFFER_IS_PRIVATE (buffer))
    {
        gui_printf (NULL, _("Private with: "));
        gui_printf_color (NULL, COLOR_WIN_CHAT_NICK,
                          "%s", CHANNEL(buffer)->name);
        gui_printf (NULL, _(" (server: "));
        gui_printf_color (NULL, COLOR_WIN_CHAT_CHANNEL,
                          "%s", SERVER(buffer)->name);
        gui_printf (NULL, ")\n");
    }
}

/*
 * weechat_cmd_buffer: manage buffers
 */

int
weechat_cmd_buffer (int argc, char **argv)
{
    t_gui_buffer *ptr_buffer;
    t_irc_server *ptr_server;
    t_irc_channel *ptr_channel;
    long number;
    char *error;
    
    if ((argc == 0) || ((argc == 1) && (strcasecmp (argv[0], "list") == 0)))
    {
        /* list opened buffers */
        
        gui_printf (NULL, "\n");
        gui_printf (NULL, _("Opened buffers:\n"));
        
        for (ptr_buffer = gui_buffers; ptr_buffer; ptr_buffer = ptr_buffer->next_buffer)
        {
            gui_printf_color (NULL, COLOR_WIN_CHAT_DARK, "[");
            gui_printf (NULL, "%d", ptr_buffer->number);
            gui_printf_color (NULL, COLOR_WIN_CHAT_DARK, "] ");
            
            weechat_cmd_buffer_display_info (ptr_buffer);
        }
    }
    else
    {
        if (strcasecmp (argv[0], "move") == 0)
        {
            /* move buffer to another number in the list */
            
            if (argc < 2)
            {
                irc_display_prefix (NULL, PREFIX_ERROR);
                gui_printf (NULL, _("%s missing arguments for \"%s\" command\n"),
                            WEECHAT_ERROR, "buffer");
                return -1;
            }
            
            error = NULL;
            number = strtol (((argv[1][0] == '+') || (argv[1][0] == '-')) ? argv[1] + 1 : argv[1],
                             &error, 10);
            if ((error) && (error[0] == '\0'))
            {
                if (argv[1][0] == '+')
                    gui_move_buffer_to_number (gui_current_window,
                                               gui_current_window->buffer->number + ((int) number));
                else if (argv[1][0] == '-')
                    gui_move_buffer_to_number (gui_current_window,
                                               gui_current_window->buffer->number - ((int) number));
                else
                    gui_move_buffer_to_number (gui_current_window, (int) number);
            }
            else
            {
                /* invalid number */
                irc_display_prefix (NULL, PREFIX_ERROR);
                gui_printf (NULL, _("%s incorrect buffer number\n"),
                            WEECHAT_ERROR);
                return -1;
            }
        }
        else if (strcasecmp (argv[0], "close") == 0)
        {
            /* close buffer (server or channel/private) */
            
            if ((!gui_current_window->buffer->next_buffer)
                && (gui_current_window->buffer == gui_buffers))
            {
                irc_display_prefix (NULL, PREFIX_ERROR);
                gui_printf (NULL,
                            _("%s can not close the single buffer\n"),
                            WEECHAT_ERROR);
                return -1;
            }
            if (BUFFER_IS_SERVER(gui_current_window->buffer))
            {
                if (SERVER(gui_current_window->buffer)->channels)
                {
                    irc_display_prefix (NULL, PREFIX_ERROR);
                    gui_printf (NULL,
                                _("%s can not close server buffer while channels "
                                "are opened\n"),
                                WEECHAT_ERROR);
                    return -1;
                }
                server_disconnect (SERVER(gui_current_window->buffer), 0);
                ptr_server = SERVER(gui_current_window->buffer);
                gui_buffer_free (gui_current_window->buffer, 1);
                ptr_server->buffer = NULL;
            }
            else
            {
                if (SERVER(gui_current_window->buffer))
                {
                    if (SERVER(gui_current_window->buffer)->is_connected)
                        irc_cmd_send_part (SERVER(gui_current_window->buffer), NULL);
                    else
                    {
                        ptr_channel = channel_search (SERVER(gui_current_window->buffer),
                                                      CHANNEL(gui_current_window->buffer)->name);
                        if (ptr_channel)
                            channel_free (SERVER(gui_current_window->buffer),
                                          ptr_channel);
                        gui_buffer_free (gui_current_window->buffer, 1);
                    }
                }
                else
                    gui_buffer_free (gui_current_window->buffer, 1);
            }
            gui_draw_buffer_status (gui_current_window->buffer, 1);
        }
        else if (strcasecmp (argv[0], "notify") == 0)
        {
            /* set notify level for buffer */
            
            if (argc < 2)
            {
                /* display notify level for all buffers */
                gui_printf (NULL, "\n");
                gui_printf (NULL, _("Notify levels:  "));
                for (ptr_buffer = gui_buffers; ptr_buffer; ptr_buffer = ptr_buffer->next_buffer)
                {
                    gui_printf (NULL, "%d.%s:",
                                ptr_buffer->number,
                                (ptr_buffer->dcc) ? "DCC" :
                                    ((BUFFER_IS_SERVER(ptr_buffer)) ? SERVER(ptr_buffer)->name :
                                    CHANNEL(ptr_buffer)->name));
                    if (ptr_buffer->dcc)
                        gui_printf (NULL, "-");
                    else
                        gui_printf (NULL, "%d", ptr_buffer->notify_level);
                    if (ptr_buffer->next_buffer)
                        gui_printf (NULL, "  ");
                }
                gui_printf (NULL, "\n");
            }
            else
            {
                error = NULL;
                number = strtol (argv[1], &error, 10);
                if ((error) && (error[0] == '\0'))
                {
                    if ((number < 0) || (number > 3))
                    {
                        /* invalid highlight level */
                        irc_display_prefix (NULL, PREFIX_ERROR);
                        gui_printf (NULL, _("%s incorrect notify level (must be between 0 and 3)\n"),
                                    WEECHAT_ERROR);
                        return -1;
                    }
                    gui_current_window->buffer->notify_level = number;
                }
                else
                {
                    /* invalid number */
                    irc_display_prefix (NULL, PREFIX_ERROR);
                    gui_printf (NULL, _("%s incorrect notify level (must be between 0 and 3)\n"),
                                WEECHAT_ERROR);
                    return -1;
                }
            }
        }
        else
        {
            /* jump to buffer by number */
            
            error = NULL;
            number = strtol (argv[0], &error, 10);
            if ((error) && (error[0] == '\0'))
            {
                if (!gui_switch_to_buffer_by_number (gui_current_window, (int) number))
                {
                    irc_display_prefix (NULL, PREFIX_ERROR);
                    gui_printf (NULL,
                                _("%s buffer \"%s\" not found for \"%s\" command\n"),
                                WEECHAT_ERROR, argv[0], "buffer");
                    return -1;
                }
            }
            else
            {
                /* invalid number */
                irc_display_prefix (NULL, PREFIX_ERROR);
                gui_printf (NULL, _("%s incorrect buffer number\n"),
                            WEECHAT_ERROR);
                return -1;
            }
            
        }
    }
    return 0;
}

/*
 * weechat_cmd_clear: display or create alias
 */

int
weechat_cmd_clear (int argc, char **argv)
{
    if (argc == 1)
    {
        if (strcmp (argv[0], "-all") == 0)
            gui_buffer_clear_all ();
        else
        {
            irc_display_prefix (NULL, PREFIX_ERROR);
            gui_printf (NULL,
                        _("unknown parameter \"%s\" for \"%s\" command\n"),
                        argv[0], "clear");
            return -1;
        }
    }
    else
        gui_buffer_clear (gui_current_window->buffer);
    return 0;
}

/*
 * weechat_cmd_connect: connect to a server
 */

int
weechat_cmd_connect (int argc, char **argv)
{
    t_irc_server *ptr_server;
    
    if (argc == 1)
        ptr_server = server_search (argv[0]);
    else
        ptr_server = SERVER(gui_current_window->buffer);
    
    if (ptr_server)
    {
        if (ptr_server->is_connected)
        {
            irc_display_prefix (NULL, PREFIX_ERROR);
            gui_printf (NULL,
                        _("%s already connected to server \"%s\"!\n"),
                        WEECHAT_ERROR, ptr_server->name);
            return -1;
        }
        if (ptr_server->child_pid > 0)
        {
            irc_display_prefix (NULL, PREFIX_ERROR);
            gui_printf (NULL,
                        _("%s currently connecting to server \"%s\"!\n"),
                        WEECHAT_ERROR, ptr_server->name);
            return -1;
        }
        if (!ptr_server->buffer)
        {
            if (!gui_buffer_new (gui_current_window, ptr_server, NULL, 0, 1))
                return -1;
        }
        if (server_connect (ptr_server))
        {
            ptr_server->reconnect_start = 0;
            ptr_server->reconnect_join = (ptr_server->channels) ? 1 : 0;
        }
    }
    else
    {
        irc_display_prefix (NULL, PREFIX_ERROR);
        gui_printf (NULL, _("%s server not found\n"), WEECHAT_ERROR);
        return -1;
    }
    return 0;
}

/*
 * weechat_cmd_disconnect: disconnect from a server
 */

int
weechat_cmd_disconnect (int argc, char **argv)
{
    t_irc_server *ptr_server;
    
    if (argc == 1)
        ptr_server = server_search (argv[0]);
    else
        ptr_server = SERVER(gui_current_window->buffer);
    
    if (ptr_server)
    {
        if ((!ptr_server->is_connected) && (ptr_server->child_pid == 0)
            && (ptr_server->reconnect_start == 0))
        {
            irc_display_prefix (ptr_server->buffer, PREFIX_ERROR);
            gui_printf (ptr_server->buffer,
                        _("%s not connected to server \"%s\"!\n"),
                        WEECHAT_ERROR, ptr_server->name);
            return -1;
        }
        if (ptr_server->reconnect_start > 0)
        {
            irc_display_prefix (ptr_server->buffer, PREFIX_INFO);
            gui_printf (ptr_server->buffer,
                        _("Auto-reconnection is cancelled\n"));
        }
        server_disconnect (ptr_server, 0);
        gui_draw_buffer_status (gui_current_window->buffer, 1);
    }
    else
    {
        irc_display_prefix (NULL, PREFIX_ERROR);
        gui_printf (NULL, _("%s server not found\n"), WEECHAT_ERROR);
        return -1;
    }
    return 0;
}

/*
 * weechat_cmd_help: display help
 */

int
weechat_cmd_help (int argc, char **argv)
{
    int i;

    if (argc == 0)
    {
        gui_printf (NULL, "\n");
        gui_printf (NULL, _("%s internal commands:\n"), PACKAGE_NAME);
        for (i = 0; weechat_commands[i].command_name; i++)
        {
            gui_printf_color (NULL, COLOR_WIN_CHAT_CHANNEL, "   %s",
                              weechat_commands[i].command_name);
            gui_printf (NULL, " - %s\n",
                        _(weechat_commands[i].command_description));
        }
        gui_printf (NULL, "\n");
        gui_printf (NULL, _("IRC commands:\n"));
        for (i = 0; irc_commands[i].command_name; i++)
        {
            if (irc_commands[i].cmd_function_args || irc_commands[i].cmd_function_1arg)
            {
                gui_printf_color (NULL, COLOR_WIN_CHAT_CHANNEL, "   %s",
                                  irc_commands[i].command_name);
                gui_printf (NULL, " - %s\n",
                            _(irc_commands[i].command_description));
            }
        }
    }
    if (argc == 1)
    {
        for (i = 0; weechat_commands[i].command_name; i++)
        {
            if (strcasecmp (weechat_commands[i].command_name, argv[0]) == 0)
            {
                gui_printf (NULL, "\n");
                gui_printf (NULL, "[w]");
                gui_printf_color (NULL, COLOR_WIN_CHAT_CHANNEL, "  /%s",
                                  weechat_commands[i].command_name);
                if (weechat_commands[i].arguments &&
                    weechat_commands[i].arguments[0])
                    gui_printf (NULL, "  %s\n",
                                _(weechat_commands[i].arguments));
                else
                    gui_printf (NULL, "\n");
                if (weechat_commands[i].arguments_description &&
                    weechat_commands[i].arguments_description[0])
                    gui_printf (NULL, "\n%s\n",
                                _(weechat_commands[i].arguments_description));
                return 0;
            }
        }
        for (i = 0; irc_commands[i].command_name; i++)
        {
            if ((strcasecmp (irc_commands[i].command_name, argv[0]) == 0)
                && (irc_commands[i].cmd_function_args || irc_commands[i].cmd_function_1arg))
            {
                gui_printf (NULL, "\n");
                gui_printf (NULL, "[i]");
                gui_printf_color (NULL, COLOR_WIN_CHAT_CHANNEL, "  /%s",
                                  irc_commands[i].command_name);
                if (irc_commands[i].arguments &&
                    irc_commands[i].arguments[0])
                    gui_printf (NULL, "  %s\n",
                                _(irc_commands[i].arguments));
                else
                    gui_printf (NULL, "\n");
                if (irc_commands[i].arguments_description &&
                    irc_commands[i].arguments_description[0])
                    gui_printf (NULL, "\n%s\n",
                                _(irc_commands[i].arguments_description));
                return 0;
            }
        }
        irc_display_prefix (NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("No help available, \"%s\" is an unknown command\n"),
                    argv[0]);
    }
    return 0;
}

/*
 * weechat_cmd_perl: list/load/unload Perl scripts
 */

int
weechat_cmd_perl (int argc, char **argv)
{
    #ifdef PLUGINS
    t_plugin_script *ptr_plugin_script;
    t_plugin_handler *ptr_plugin_handler;
    int handler_found, path_length;
    char *path_script;
    
    #ifdef PLUGIN_PERL
    switch (argc)
    {
        case 0:
            /* list registered Perl scripts */
            gui_printf (NULL, "\n");
            gui_printf (NULL, _("Registered Perl scripts:\n"));
            if (perl_scripts)
            {
                for (ptr_plugin_script = perl_scripts; ptr_plugin_script;
                     ptr_plugin_script = ptr_plugin_script->next_script)
                {
                    irc_display_prefix (NULL, PREFIX_PLUGIN);
                    gui_printf (NULL, "  %s v%s%s%s\n",
                                ptr_plugin_script->name,
                                ptr_plugin_script->version,
                                (ptr_plugin_script->description[0]) ? " - " : "",
                                ptr_plugin_script->description);
                }
            }
            else
            {
                irc_display_prefix (NULL, PREFIX_PLUGIN);
                gui_printf (NULL, _("  (none)\n"));
            }
            
            /* list Perl message handlers */
            gui_printf (NULL, "\n");
            gui_printf (NULL, _("Perl message handlers:\n"));
            handler_found = 0;
            for (ptr_plugin_handler = plugin_msg_handlers; ptr_plugin_handler;
                 ptr_plugin_handler = ptr_plugin_handler->next_handler)
            {
                if (ptr_plugin_handler->plugin_type == PLUGIN_TYPE_PERL)
                {
                    handler_found = 1;
                    irc_display_prefix (NULL, PREFIX_PLUGIN);
                    gui_printf (NULL, _("  IRC(%s) => Perl(%s)\n"),
                                ptr_plugin_handler->name,
                                ptr_plugin_handler->function_name);
                }
            }
            if (!handler_found)
            {
                irc_display_prefix (NULL, PREFIX_PLUGIN);
                gui_printf (NULL, _("  (none)\n"));
            }
            
            /* list Perl command handlers */
            gui_printf (NULL, "\n");
            gui_printf (NULL, _("Perl command handlers:\n"));
            handler_found = 0;
            for (ptr_plugin_handler = plugin_cmd_handlers; ptr_plugin_handler;
                 ptr_plugin_handler = ptr_plugin_handler->next_handler)
            {
                if (ptr_plugin_handler->plugin_type == PLUGIN_TYPE_PERL)
                {
                    handler_found = 1;
                    irc_display_prefix (NULL, PREFIX_PLUGIN);
                    gui_printf (NULL, _("  Command /%s => Perl(%s)\n"),
                                ptr_plugin_handler->name,
                                ptr_plugin_handler->function_name);
                }
            }
            if (!handler_found)
            {
                irc_display_prefix (NULL, PREFIX_PLUGIN);
                gui_printf (NULL, _("  (none)\n"));
            }
            
            break;
        case 1:
            if (strcmp (argv[0], "autoload") == 0)
                plugin_auto_load (PLUGIN_TYPE_PERL, "perl/autoload");
            if (strcmp (argv[0], "unload") == 0)
            {
                /* unload all Perl scripts */
                plugin_unload (PLUGIN_TYPE_PERL, NULL);
                irc_display_prefix (NULL, PREFIX_PLUGIN);
                gui_printf (NULL, _("Perl scripts unloaded\n"));
            }
            break;
        case 2:
            if (strcmp (argv[0], "load") == 0)
            {
                /* load Perl script */
                if (strstr(argv[1], DIR_SEPARATOR))
                    path_script = NULL;
                else
                {
                    path_length = strlen (weechat_home) + strlen (argv[1]) + 7;
                    path_script = (char *) malloc (path_length * sizeof (char));
                    snprintf (path_script, path_length, "%s%s%s%s%s",
                              weechat_home, DIR_SEPARATOR, "perl",
                              DIR_SEPARATOR, argv[1]);
                }
                plugin_load (PLUGIN_TYPE_PERL,
                             (path_script) ? path_script : argv[1]);
                if (path_script)
                    free (path_script);
            }
            else
            {
                irc_display_prefix (NULL, PREFIX_ERROR);
                gui_printf (NULL,
                            _("%s unknown option for \"%s\" command\n"),
                            WEECHAT_ERROR, "perl");
            }
            break;
        default:
            irc_display_prefix (NULL, PREFIX_ERROR);
            gui_printf (NULL,
                        _("%s wrong argument count for \"%s\" command\n"),
                        WEECHAT_ERROR, "perl");
    }
    #else
    irc_display_prefix (NULL, PREFIX_ERROR);
    gui_printf (NULL,
                _("WeeChat was build without Perl support.\n"
                "Please rebuild WeeChat with "
                "\"--enable-perl\" option for ./configure script\n"));
    /* make gcc happy */
    (void) argc;
    (void) argv;
    #endif /* PLUGIN_PERL */
    
    #else
    /* make gcc happy */
    (void) argc;
    (void) argv;
    #endif /* PLUGINS */
    
    return 0;
}

/*
 * weechat_cmd_save: save options to disk
 */

int
weechat_cmd_save (int argc, char **argv)
{
    return (config_write ((argc == 1) ? argv[0] : NULL));
}

/*
 * weechat_cmd_server: list, add or remove server(s)
 */

int
weechat_cmd_server (int argc, char **argv)
{
    int i;
    t_irc_server server, *ptr_server, *server_found, *new_server;
    t_gui_buffer *ptr_buffer;
    
    if ((argc == 0) || (argc == 1))
    {
        /* list all servers */
        if (argc == 0)
        {
            if (irc_servers)
            {
                for (ptr_server = irc_servers; ptr_server;
                     ptr_server = ptr_server->next_server)
                {
                    irc_display_server (ptr_server);
                }
            }
            else
            {
                irc_display_prefix (NULL, PREFIX_INFO);
                gui_printf (NULL, _("No server.\n"));
            }
        }
        else
        {
            ptr_server = server_search (argv[0]);
            if (ptr_server)
                irc_display_server (ptr_server);
            else
            {
                irc_display_prefix (NULL, PREFIX_INFO);
                gui_printf (NULL, _("Server '%s' not found.\n"), argv[0]);
            }
        }
    }
    else
    {
        if (strcasecmp (argv[0], "del") == 0)
        {
            if (argc < 2)
            {
                irc_display_prefix (NULL, PREFIX_ERROR);
                gui_printf (NULL,
                            _("%s missing servername for \"%s\" command\n"),
                            WEECHAT_ERROR, "server del");
                return -1;
            }
            if (argc > 2)
            {
                irc_display_prefix (NULL, PREFIX_ERROR);
                gui_printf (NULL,
                            _("%s too much arguments for \"%s\" command, ignoring arguments\n"),
                            WEECHAT_WARNING, "server del");
            }
            
            /* look for server by name */
            server_found = NULL;
            for (ptr_server = irc_servers; ptr_server;
                 ptr_server = ptr_server->next_server)
            {
                if (strcmp (ptr_server->name, argv[1]) == 0)
                {
                    server_found = ptr_server;
                    break;
                }
            }
            if (!server_found)
            {
                irc_display_prefix (NULL, PREFIX_ERROR);
                gui_printf (NULL,
                            _("%s server \"%s\" not found for \"%s\" command\n"),
                            WEECHAT_ERROR, argv[1], "server del");
                return -1;
            }
            if (server_found->is_connected)
            {
                irc_display_prefix (NULL, PREFIX_ERROR);
                gui_printf (NULL,
                            _("%s you can not delete server \"%s\" because you are connected to. "
                            "Try /disconnect %s before.\n"),
                            WEECHAT_ERROR, argv[1], argv[1]);
                return -1;
            }
            
            for (ptr_buffer = gui_buffers; ptr_buffer; ptr_buffer = ptr_buffer->next_buffer)
            {
                if (SERVER(ptr_buffer) == server_found)
                {
                    ptr_buffer->server = NULL;
                    ptr_buffer->channel = NULL;
                }
            }
            
            irc_display_prefix (NULL, PREFIX_INFO);
            gui_printf_color (NULL, COLOR_WIN_CHAT, _("Server"));
            gui_printf_color (NULL, COLOR_WIN_CHAT_CHANNEL,
                              " %s ", server_found->name);
            gui_printf_color (NULL, COLOR_WIN_CHAT, _("has been deleted\n"));
            
            server_free (server_found);
            gui_redraw_buffer (gui_current_window->buffer);
            
            return 0;
        }
        
        /* init server struct */
        server_init (&server);
        
        if (argc < 3)
        {
            irc_display_prefix (NULL, PREFIX_ERROR);
            gui_printf (NULL,
                        _("%s missing parameters for \"%s\" command\n"),
                        WEECHAT_ERROR, "server");
            server_destroy (&server);
            return -1;
        }
        
        if (server_name_already_exists (argv[0]))
        {
            irc_display_prefix (NULL, PREFIX_ERROR);
            gui_printf (NULL,
                        _("%s server \"%s\" already exists, can't create it!\n"),
                        WEECHAT_ERROR, argv[0]);
            server_destroy (&server);
            return -1;
        }
        
        server.name = strdup (argv[0]);
        server.address = strdup (argv[1]);
        server.port = atoi (argv[2]);
        
        /* parse arguments */
        for (i = 3; i < argc; i++)
        {
            if (argv[i][0] == '-')
            {
                if (strcasecmp (argv[i], "-auto") == 0)
                    server.autoconnect = 1;
                if (strcasecmp (argv[i], "-noauto") == 0)
                    server.autoconnect = 0;
                if (strcasecmp (argv[i], "-pwd") == 0)
                {
                    if (i == (argc - 1))
                    {
                        irc_display_prefix (NULL, PREFIX_ERROR);
                        gui_printf (NULL,
                                    _("%s missing password for \"%s\" parameter\n"),
                                    WEECHAT_ERROR, "-pwd");
                        server_destroy (&server);
                        return -1;
                    }
                    server.password = strdup (argv[++i]);
                }
                if (strcasecmp (argv[i], "-nicks") == 0)
                {
                    if (i >= (argc - 3))
                    {
                        irc_display_prefix (NULL, PREFIX_ERROR);
                        gui_printf (NULL,
                                    _("%s missing nick(s) for \"%s\" parameter\n"),
                                    WEECHAT_ERROR, "-nicks");
                        server_destroy (&server);
                        return -1;
                    }
                    server.nick1 = strdup (argv[++i]);
                    server.nick2 = strdup (argv[++i]);
                    server.nick3 = strdup (argv[++i]);
                }
                if (strcasecmp (argv[i], "-username") == 0)
                {
                    if (i == (argc - 1))
                    {
                        irc_display_prefix (NULL, PREFIX_ERROR);
                        gui_printf (NULL,
                                    _("%s missing password for \"%s\" parameter\n"),
                                    WEECHAT_ERROR, "-username");
                        server_destroy (&server);
                        return -1;
                    }
                    server.username = strdup (argv[++i]);
                }
                if (strcasecmp (argv[i], "-realname") == 0)
                {
                    if (i == (argc - 1))
                    {
                        irc_display_prefix (NULL, PREFIX_ERROR);
                        gui_printf (NULL,
                                    _("%s missing password for \"%s\" parameter\n"),
                                    WEECHAT_ERROR, "-realname");
                        server_destroy (&server);
                        return -1;
                    }
                    server.realname = strdup (argv[++i]);
                }
                if (strcasecmp (argv[i], "-command") == 0)
                {
                    if (i == (argc - 1))
                    {
                        irc_display_prefix (NULL, PREFIX_ERROR);
                        gui_printf (NULL,
                                    _("%s missing command for \"%s\" parameter\n"),
                                    WEECHAT_ERROR, "-command");
                        server_destroy (&server);
                        return -1;
                    }
                    server.command = strdup (argv[++i]);
                }
                if (strcasecmp (argv[i], "-autojoin") == 0)
                {
                    if (i == (argc - 1))
                    {
                        irc_display_prefix (NULL, PREFIX_ERROR);
                        gui_printf (NULL,
                                    _("%s missing password for \"%s\" parameter\n"),
                                    WEECHAT_ERROR, "-autojoin");
                        server_destroy (&server);
                        return -1;
                    }
                    server.autojoin = strdup (argv[++i]);
                }
            }
        }
        
        /* create new server */
        new_server = server_new (server.name, server.autoconnect,
                                 server.autoreconnect,
                                 server.autoreconnect_delay,
                                 0, server.address, server.port, server.password,
                                 server.nick1, server.nick2, server.nick3,
                                 server.username, server.realname,
                                 server.command, 1, server.autojoin, 1);
        if (new_server)
        {
            irc_display_prefix (NULL, PREFIX_INFO);
            gui_printf_color (NULL, COLOR_WIN_CHAT, _("Server"));
            gui_printf_color (NULL, COLOR_WIN_CHAT_CHANNEL,
                              " %s ", server.name);
            gui_printf_color (NULL, COLOR_WIN_CHAT, _("created\n"));
        }
        else
        {
            irc_display_prefix (NULL, PREFIX_ERROR);
            gui_printf (NULL,
                        _("%s unable to create server\n"),
                        WEECHAT_ERROR);
            server_destroy (&server);
            return -1;
        }
        
        if (new_server->autoconnect)
        {
            (void) gui_buffer_new (gui_current_window, new_server, NULL, 0, 1);
            server_connect (new_server);
        }
        
        server_destroy (&server);
    }
    return 0;
}

/*
 * weechat_set_cmd_display_option: display config option
 */

void
weechat_set_cmd_display_option (t_config_option *option, char *prefix, void *value)
{
    char *color_name, *pos_nickserv, *pos_pwd, *value2;
    
    gui_printf (NULL, "  %s%s%s",
                (prefix) ? prefix : "",
                (prefix) ? "." : "",
                option->option_name);
    gui_printf_color (NULL, COLOR_WIN_CHAT_DARK, " = ");
    if (!value)
    {
        if (option->option_type == OPTION_TYPE_STRING)
            value = option->ptr_string;
        else
            value = option->ptr_int;
    }
    switch (option->option_type)
    {
        case OPTION_TYPE_BOOLEAN:
            gui_printf_color (NULL, COLOR_WIN_CHAT_HOST,
                              "%s\n", (*((int *)value)) ? "ON" : "OFF");
            break;
        case OPTION_TYPE_INT:
            gui_printf_color (NULL, COLOR_WIN_CHAT_HOST,
                              "%d\n", *((int *)value));
            break;
        case OPTION_TYPE_INT_WITH_STRING:
            gui_printf_color (NULL, COLOR_WIN_CHAT_HOST,
                              "%s\n", option->array_values[*((int *)value)]);
            break;
        case OPTION_TYPE_COLOR:
            color_name = gui_get_color_by_value (*((int *)value));
            gui_printf_color (NULL, COLOR_WIN_CHAT_HOST,
                              "%s\n", (color_name) ? color_name : _("(unknown)"));
            break;
        case OPTION_TYPE_STRING:
            if (*((char **)value))
            {
                value2 = strdup (*((char **)value));
                pos_nickserv = NULL;
                pos_pwd = NULL;
                pos_nickserv = strstr (value2, "nickserv");
                if (pos_nickserv)
                {
                    pos_pwd = strstr (value2, "identify ");
                    if (!pos_pwd)
                        pos_pwd = strstr (value2, "register ");
                }
                if (cfg_log_hide_nickserv_pwd && pos_nickserv && pos_pwd)
                {
                    pos_pwd += 9;
                    while (pos_pwd[0])
                    {
                        pos_pwd[0] = '*';
                        pos_pwd++;
                    }
                    gui_printf (NULL, _("(password hidden) "));
                }
                gui_printf_color (NULL, COLOR_WIN_CHAT_HOST, "%s", value2);
                free (value2);
            }
            gui_printf (NULL, "\n");
            break;
    }
}

/*
 * weechat_cmd_set: set options
 */

int
weechat_cmd_set (char *arguments)
{
    char *option, *value, *pos;
    int i, j, section_displayed;
    t_config_option *ptr_option;
    t_irc_server *ptr_server;
    char option_name[256];
    void *ptr_option_value;
    int number_found;

    option = NULL;
    value = NULL;
    if (arguments && arguments[0])
    {
        option = arguments;
        value = strchr (option, '=');
        if (value)
        {
            value[0] = '\0';
            
            /* remove spaces before '=' */
            pos = value - 1;
            while ((pos > option) && (pos[0] == ' '))
            {
                pos[0] = '\0';
                pos--;
            }
            
            /* skip spaces after '=' */
            value++;
            while (value[0] && (value[0] == ' '))
            {
                value++;
            }
        }
    }
    
    if (value)
    {
        pos = strchr (option, '.');
        if (pos)
        {
            /* server config option modification */
            pos[0] = '\0';
            ptr_server = server_search (option);
            if (!ptr_server)
            {
                irc_display_prefix (NULL, PREFIX_ERROR);
                gui_printf (NULL,
                            _("%s server \"%s\" not found\n"),
                            WEECHAT_ERROR, option);
            }
            else
            {
                switch (config_set_server_value (ptr_server, pos + 1, value))
                {
                    case 0:
                        gui_printf_color (NULL, COLOR_WIN_CHAT_DARK, "\n[");
                        gui_printf_color (NULL, COLOR_WIN_CHAT_CHANNEL, "%s",
                                          config_sections[CONFIG_SECTION_SERVER].section_name);
                        gui_printf_color (NULL, COLOR_WIN_CHAT_NICK, " %s",
                                          ptr_server->name);
                        gui_printf_color (NULL, COLOR_WIN_CHAT_DARK, "]\n");
                        for (i = 0; weechat_options[CONFIG_SECTION_SERVER][i].option_name; i++)
                        {
                            if (strcmp (weechat_options[CONFIG_SECTION_SERVER][i].option_name, pos + 1) == 0)
                                break;
                        }
                        if (weechat_options[CONFIG_SECTION_SERVER][i].option_name)
                        {
                            ptr_option_value = config_get_server_option_ptr (ptr_server,
                                weechat_options[CONFIG_SECTION_SERVER][i].option_name);
                            weechat_set_cmd_display_option (&weechat_options[CONFIG_SECTION_SERVER][i],
                                                            ptr_server->name,
                                                            ptr_option_value);
                        }
                        config_change_buffer_content ();
                        break;
                    case -1:
                        irc_display_prefix (NULL, PREFIX_ERROR);
                        gui_printf (NULL, _("%s config option \"%s\" not found\n"),
                                    WEECHAT_ERROR, pos + 1);
                        break;
                    case -2:
                        irc_display_prefix (NULL, PREFIX_ERROR);
                        gui_printf (NULL, _("%s incorrect value for option \"%s\"\n"),
                                    WEECHAT_ERROR, pos + 1);
                        break;
                }
            }
            pos[0] = '.';
        }
        else
        {
            ptr_option = config_option_search (option);
            if (ptr_option)
            {
                if (ptr_option->handler_change == NULL)
                {
                    irc_display_prefix (NULL, PREFIX_ERROR);
                    gui_printf (NULL,
                                _("%s option \"%s\" can not be changed while WeeChat is running\n"),
                                WEECHAT_ERROR, option);
                }
                else
                {
                    if (config_option_set_value (ptr_option, value) == 0)
                    {
                        (void) (ptr_option->handler_change());
                        gui_printf_color (NULL, COLOR_WIN_CHAT_DARK, "\n[");
                        gui_printf_color (NULL, COLOR_WIN_CHAT_CHANNEL,
                                          "%s", config_get_section (ptr_option));
                        gui_printf_color (NULL, COLOR_WIN_CHAT_DARK, "]\n");
                        weechat_set_cmd_display_option (ptr_option, NULL, NULL);
                    }
                    else
                    {
                        irc_display_prefix (NULL, PREFIX_ERROR);
                        gui_printf (NULL, _("%s incorrect value for option \"%s\"\n"),
                                    WEECHAT_ERROR, option);
                    }
                }
            }
            else
            {
                irc_display_prefix (NULL, PREFIX_ERROR);
                gui_printf (NULL, _("%s config option \"%s\" not found\n"),
                            WEECHAT_ERROR, option);
            }
        }
    }
    else
    {
        number_found = 0;
        for (i = 0; i < CONFIG_NUMBER_SECTIONS; i++)
        {
            section_displayed = 0;
            if ((i != CONFIG_SECTION_ALIAS) && (i != CONFIG_SECTION_SERVER))
            {
                for (j = 0; weechat_options[i][j].option_name; j++)
                {
                    if ((!option) ||
                        ((option) && (option[0])
                         && (strstr (weechat_options[i][j].option_name, option)
                             != NULL)))
                    {
                        if (!section_displayed)
                        {
                            gui_printf_color (NULL, COLOR_WIN_CHAT_DARK, "\n[");
                            gui_printf_color (NULL, COLOR_WIN_CHAT_CHANNEL,
                                              "%s",
                                              config_sections[i].section_name);
                            gui_printf_color (NULL, COLOR_WIN_CHAT_DARK, "]\n");
                            section_displayed = 1;
                        }
                        weechat_set_cmd_display_option (&weechat_options[i][j], NULL, NULL);
                        number_found++;
                    }
                }
            }
        }
        for (ptr_server = irc_servers; ptr_server;
             ptr_server = ptr_server->next_server)
        {
            section_displayed = 0;
            for (i = 0; weechat_options[CONFIG_SECTION_SERVER][i].option_name; i++)
            {
                snprintf (option_name, sizeof (option_name), "%s.%s",
                          ptr_server->name, 
                          weechat_options[CONFIG_SECTION_SERVER][i].option_name);
                if ((!option) ||
                        ((option) && (option[0])
                         && (strstr (option_name, option) != NULL)))
                {
                    if (!section_displayed)
                    {
                        gui_printf_color (NULL, COLOR_WIN_CHAT_DARK, "\n[");
                        gui_printf_color (NULL, COLOR_WIN_CHAT_CHANNEL, "%s",
                                          config_sections[CONFIG_SECTION_SERVER].section_name);
                        gui_printf_color (NULL, COLOR_WIN_CHAT_NICK, " %s",
                                          ptr_server->name);
                        gui_printf_color (NULL, COLOR_WIN_CHAT_DARK, "]\n");
                        section_displayed = 1;
                    }
                    ptr_option_value = config_get_server_option_ptr (ptr_server,
                        weechat_options[CONFIG_SECTION_SERVER][i].option_name);
                    if (ptr_option_value)
                    {
                        weechat_set_cmd_display_option (&weechat_options[CONFIG_SECTION_SERVER][i],
                                                        ptr_server->name,
                                                        ptr_option_value);
                        number_found++;
                    }
                }
            }
        }
        if (number_found == 0)
        {
            if (option)
                gui_printf (NULL, _("No config option found with \"%s\"\n"),
                            option);
            else
                gui_printf (NULL, _("No config option found\n"));
        }
        else
        {
            gui_printf_color (NULL, COLOR_WIN_CHAT_CHANNEL, "\n%d ", number_found);
            if (option)
                gui_printf (NULL, _("config option(s) found with \"%s\"\n"),
                            option);
            else
                gui_printf (NULL, _("config option(s) found\n"));
        }
    }
    return 0;
}

/*
 * cmd_unalias: remove an alias
 */

int
weechat_cmd_unalias (char *arguments)
{
    t_weelist *ptr_weelist;
    t_weechat_alias *ptr_alias;
    
    ptr_weelist = weelist_search (index_commands, arguments);
    if (!ptr_weelist)
    {
        irc_display_prefix (NULL, PREFIX_ERROR);
        gui_printf (NULL, _("%s alias or command \"%s\" not found\n"),
                    WEECHAT_ERROR, arguments);
        return -1;
    }
    
    weelist_remove (&index_commands, &last_index_command, ptr_weelist);
    ptr_alias = alias_search (arguments);
    if (ptr_alias)
        alias_free (ptr_alias);
    irc_display_prefix (NULL, PREFIX_INFO);
    gui_printf (NULL, _("Alias \"%s\" removed\n"),
                arguments);
    return 0;
}

/*
 * weechat_cmd_window: manage windows
 */

int
weechat_cmd_window (int argc, char **argv)
{
    t_gui_window *ptr_win;
    int i;
    
    if ((argc == 0) || ((argc == 1) && (strcasecmp (argv[0], "list") == 0)))
    {
        /* list opened windows */
        
        gui_printf (NULL, "\n");
        gui_printf (NULL, _("Opened windows:\n"));
        
        i = 1;
        for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
        {
            gui_printf_color (NULL, COLOR_WIN_CHAT_DARK, "[");
            gui_printf (NULL, "%d", i);
            gui_printf_color (NULL, COLOR_WIN_CHAT_DARK, "] (");
            gui_printf (NULL, "%d", ptr_win->win_x);
            gui_printf_color (NULL, COLOR_WIN_CHAT_DARK, ":");
            gui_printf (NULL, "%d", ptr_win->win_y);
            gui_printf_color (NULL, COLOR_WIN_CHAT_DARK, ";");
            gui_printf (NULL, "%d", ptr_win->win_width);
            gui_printf_color (NULL, COLOR_WIN_CHAT_DARK, "x");
            gui_printf (NULL, "%d", ptr_win->win_height);
            gui_printf_color (NULL, COLOR_WIN_CHAT_DARK, ") ");
            
            weechat_cmd_buffer_display_info (ptr_win->buffer);
            
            i++;
        }
    }
    else
    {
        if (strcasecmp (argv[0], "splith") == 0)
        {
            /* split window horizontally */
            gui_window_split_horiz (gui_current_window);
        }
        else if (strcasecmp (argv[0], "splitv") == 0)
        {
            /* split window vertically */
            gui_window_split_vertic (gui_current_window);
        }
        else if (strcasecmp (argv[0], "merge") == 0)
        {
            if (argc >= 2)
            {
                if (strcasecmp (argv[1], "down") == 0)
                    gui_window_merge_down (gui_current_window);
                else if (strcasecmp (argv[1], "up") == 0)
                    gui_window_merge_up (gui_current_window);
                else if (strcasecmp (argv[1], "left") == 0)
                    gui_window_merge_left (gui_current_window);
                else if (strcasecmp (argv[1], "right") == 0)
                    gui_window_merge_right (gui_current_window);
                else if (strcasecmp (argv[1], "all") == 0)
                    gui_window_merge_all (gui_current_window);
                else
                {
                    irc_display_prefix (NULL, PREFIX_ERROR);
                    gui_printf (NULL,
                                _("%s unknown option for \"%s\" command\n"),
                                WEECHAT_ERROR, "window merge");
                    return -1;
                }
            }
            else
                gui_window_merge_auto (gui_current_window);
        }
        else
        {
            irc_display_prefix (NULL, PREFIX_ERROR);
            gui_printf (NULL,
                        _("%s unknown option for \"%s\" command\n"),
                        WEECHAT_ERROR, "window");
            return -1;
        }
    }
    return 0;
}
