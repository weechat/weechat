/*
 * Copyright (c) 2003 by FlashCode <flashcode@flashtux.org>
 *                       Bounga <bounga@altern.org>
 *                       Xahlexx <xahlexx@tuxisland.org>
 * See README for License detail.
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
  { "clear", N_("clear window(s)"),
    N_("[-all]"),
    N_("-all: clear all windows"),
    0, 1, weechat_cmd_clear, NULL },
  { "connect", N_("connect to a server"),
    N_("servername"),
    N_("servername: server name to connect"),
    1, 1, weechat_cmd_connect, NULL },
  { "disconnect", N_("disconnect from a server"),
    N_("servername"),
    N_("servername: server name to disconnect"),
    1, 1, weechat_cmd_disconnect, NULL },
  { "help", N_("display help about commands"),
    N_("[command]"), N_("command: name of a WeeChat or IRC command"),
    0, 1, weechat_cmd_help, NULL },
  { "perl", N_("list/load/unload Perl scripts"),
    N_("[load filename] | [unload]"),
    N_("filename: Perl script (file) to load\n"
    "Without argument, /perl command lists all loaded Perl scripts."),
    0, 2, weechat_cmd_perl, NULL },
  { "server", N_("list, add or remove servers"),
    N_("[list] | "
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
    "realname: real name of user\n"),
    0, MAX_ARGS, weechat_cmd_server, NULL },
  { "save", N_("save config to disk"),
    N_("[file]"), N_("file: filename for writing config"),
    0, 1, weechat_cmd_save, NULL },
  { "set", N_("set config parameters"),
    N_("[option [value]]"), N_("option: name of an option\nvalue: value for option"),
    0, 2, weechat_cmd_set, NULL },
  { "unalias", N_("remove an alias"),
    N_("alias_name"), N_("alias_name: name of alias to remove"),
    1, 1, NULL, weechat_cmd_unalias },
  { NULL, NULL, NULL, NULL, 0, 0, NULL, NULL }
};

t_weechat_alias *weechat_alias = NULL;
t_weechat_alias *weechat_last_alias = NULL;

t_index_command *index_commands;
t_index_command *last_index_command;


/*
 * index_command_search: search a command
 */

t_index_command *
index_command_search (char *command)
{
    t_index_command *ptr_index;
    
    for (ptr_index = index_commands; ptr_index; ptr_index = ptr_index->next_index)
    {
        if (strcasecmp (command, ptr_index->command_name) == 0)
            return ptr_index;
    }
    return NULL;
}

/*
 * index_command_find_pos: find position for a command index (for sorting index)
 */

t_index_command *
index_command_find_pos (char *command)
{
    t_index_command *ptr_index;
    
    for (ptr_index = index_commands; ptr_index; ptr_index = ptr_index->next_index)
    {
        if (strcasecmp (command, ptr_index->command_name) < 0)
            return ptr_index;
    }
    return NULL;
}

/*
 * index_command_insert_sorted: insert index into sorted list
 */

void
index_command_insert_sorted (t_index_command *index)
{
    t_index_command *pos_index;
    
    pos_index = index_command_find_pos (index->command_name);
    
    if (index_commands)
    {
        if (pos_index)
        {
            /* insert index into the list (before index found) */
            index->prev_index = pos_index->prev_index;
            index->next_index = pos_index;
            if (pos_index->prev_index)
                pos_index->prev_index->next_index = index;
            else
                index_commands = index;
            pos_index->prev_index = index;
        }
        else
        {
            /* add index to the end */
            index->prev_index = last_index_command;
            index->next_index = NULL;
            last_index_command->next_index = index;
            last_index_command = index;
        }
    }
    else
    {
        index->prev_index = NULL;
        index->next_index = NULL;
        index_commands = index;
        last_index_command = index;
    }
}

/*
 * index_command_new: create new index command and add it to index list
 */

t_index_command *
index_command_new (char *command_name)
{
    t_index_command *new_index;
    
    if ((new_index = ((t_index_command *) malloc (sizeof (t_index_command)))))
    {
        new_index->command_name = strdup (command_name);
        index_command_insert_sorted (new_index);
        return new_index;
    }
    return NULL;
}

/*
 * index_command_build: build an index of commands (internal, irc and alias)
 *                      This list will be sorted, and used for completion
 */

void
index_command_build ()
{
    int i;
    
    index_commands = NULL;
    last_index_command = NULL;
    i = 0;
    while (weechat_commands[i].command_name)
    {
        index_command_new (weechat_commands[i].command_name);
        i++;
    }
    i = 0;
    while (irc_commands[i].command_name)
    {
        if (irc_commands[i].cmd_function_args || irc_commands[i].cmd_function_1arg)
            index_command_new (irc_commands[i].command_name);
        i++;
    }
}

/*
 * index_command_free: free an index command and reomve it from list
 */

void
index_command_free (t_index_command *index)
{
    t_index_command *new_index_commands;

    /* remove index command from list */
    if (last_index_command == index)
        last_index_command = index->prev_index;
    if (index->prev_index)
    {
        (index->prev_index)->next_index = index->next_index;
        new_index_commands = index_commands;
    }
    else
        new_index_commands = index->next_index;
    
    if (index->next_index)
        (index->next_index)->prev_index = index->prev_index;

    /* free data */
    if (index->command_name)
        free (index->command_name);
    free (index);
    index_commands = new_index_commands;
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
    
    if (index_command_search (alias_name))
    {
        gui_printf (NULL, _("%s alias or command \"%s\" already exists!\n"),
                    WEECHAT_ERROR, alias_name);
        return NULL;
    }
    pos = strchr (alias_command, ' ');
    if (pos)
        pos[0] = '\0';
    if (alias_search (alias_command))
    {
        gui_printf (NULL, _("%s alias cannot run another alias!\n"),
                    WEECHAT_ERROR);
        return NULL;
    }
    if (!index_command_search (alias_command))
    {
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
        new_alias->alias_command[0] = '/';
        strcpy (new_alias->alias_command + 1, alias_command);
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
 * explode_string: explode a string according to separators
 */

char **
explode_string (char *string, char *separators, int num_items_max,
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

    if ((!string[0]) || (string[0] != '/'))
        return 0;

    command = strdup (string);
    
    /* look for end of command */
    ptr_args = NULL;
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
    
    if (!plugin_exec_command (command + 1, ptr_args))
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
                        gui_printf (NULL,
                                    _("%s wrong argument count for %s command \"%s\" "
                                    "(expected: %d arg%s)\n"),
                                    WEECHAT_ERROR, PACKAGE_NAME,
                                    command + 1,
                                    weechat_commands[i].max_arg,
                                    (weechat_commands[i].max_arg >
                                     1) ? "s" : "");
                    else
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
                else
                {
                    if (weechat_commands[i].cmd_function_args)
                        return_code = (int) (weechat_commands[i].cmd_function_args)
                                            (argc, argv);
                    else
                        return_code = (int) (weechat_commands[i].cmd_function_1arg)
                                            (ptr_args);
                    if (return_code < 0)
                        gui_printf (NULL,
                                    _("%s %s command \"%s\" failed\n"),
                                    WEECHAT_ERROR, PACKAGE_NAME, command + 1);
                }
                if (argv)
                {
                    for (j = 0; argv[j]; j++)
                        free (argv[j]);
                    free (argv);
                }
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
                        gui_printf
                            (NULL,
                             _("%s wrong argument count for IRC command \"%s\" "
                             "(expected: %d arg%s)\n"),
                             WEECHAT_ERROR,
                             command + 1,
                             irc_commands[i].max_arg,
                             (irc_commands[i].max_arg > 1) ? "s" : "");
                    else
                        gui_printf
                            (NULL,
                             _("%s wrong argument count for IRC command \"%s\" "
                             "(expected: between %d and %d arg%s)\n"),
                             WEECHAT_ERROR,
                             command + 1,
                             irc_commands[i].min_arg, irc_commands[i].max_arg,
                             (irc_commands[i].max_arg > 1) ? "s" : "");
                }
                else
                {
                    if ((irc_commands[i].need_connection) &&
                        ((!server) || (!server->is_connected)))
                    {
                        gui_printf (NULL,
                                    _("%s command \"%s\" needs a server connection!\n"),
                                    WEECHAT_ERROR, irc_commands[i].command_name);
                        return 0;
                    }
                    if (irc_commands[i].cmd_function_args)
                        return_code = (int) (irc_commands[i].cmd_function_args)
                                      (server, argc, argv);
                    else
                        return_code = (int) (irc_commands[i].cmd_function_1arg)
                                      (server, ptr_args);
                    if (return_code < 0)
                        gui_printf (NULL,
                                    _("%s IRC command \"%s\" failed\n"),
                                    WEECHAT_ERROR, command + 1);
                }
                if (argv)
                {
                    for (j = 0; argv[j]; j++)
                        free (argv[j]);
                    free (argv);
                }
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
                    strcpy (alias_command, ptr_alias->alias_command);
                    alias_command[length1] = ' ';
                    strcpy (alias_command + length1 + 1, ptr_args);
                    exec_weechat_command (server, alias_command);
                    free (alias_command);
                }
                else
                    exec_weechat_command (server, ptr_alias->alias_command);
                
                if (argv)
                {
                    for (j = 0; argv[j]; j++)
                        free (argv[j]);
                    free (argv);
                }
                return 1;
            }
        }
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
        exec_weechat_command (server, command);
    }
    else
    {
        if ((command[0] == '/') && (command[1] == '/'))
            command++;
        if (server && (!WIN_IS_SERVER(gui_current_window)))
        {
            server_sendf (server, "PRIVMSG %s :%s\r\n",
                          CHANNEL(gui_current_window)->name,
                          command);

            if (WIN_IS_PRIVATE(gui_current_window))
            {
                gui_printf_color_type (CHANNEL(gui_current_window)->window,
                                       MSG_TYPE_NICK,
                                       COLOR_WIN_CHAT_DARK, "<");
                gui_printf_color_type (CHANNEL(gui_current_window)->window,
                                       MSG_TYPE_NICK,
                                       COLOR_WIN_NICK_SELF,
                                       "%s", server->nick);
                gui_printf_color_type (CHANNEL(gui_current_window)->window,
                                       MSG_TYPE_NICK,
                                       COLOR_WIN_CHAT_DARK, "> ");
                gui_printf_color_type (CHANNEL(gui_current_window)->window,
                                       MSG_TYPE_MSG,
                                       COLOR_WIN_CHAT, "%s\n", command);
            }
            else
            {
                ptr_nick = nick_search (CHANNEL(gui_current_window), server->nick);
                if (ptr_nick)
                {
                    irc_display_nick (CHANNEL(gui_current_window)->window, ptr_nick,
                                      MSG_TYPE_NICK, 1, 1, 0);
                    gui_printf_color (CHANNEL(gui_current_window)->window,
                                      COLOR_WIN_CHAT, "%s\n", command);
                }
                else
                    gui_printf (server->window,
                                _("%s cannot find nick for sending message\n"),
                                WEECHAT_ERROR);
            }
        }
        else
            gui_printf ((server) ? server->window : NULL,
                        _("This window is not a channel!\n"));
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
                gui_printf (NULL, _("%s missing arguments for \"%s\" command\n"),
                            WEECHAT_ERROR, "alias");
                return -1;
            }
            if (!alias_new (arguments, pos))
                return -1;
            index_command_new (arguments);
            gui_printf (NULL, _("Alias \"%s\" => \"%s\" created\n"),
                        arguments, pos);
        }
        else
        {
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
            gui_printf (NULL, _("No alias defined.\n"));
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
            gui_window_clear_all ();
        else
        {
            gui_printf (NULL,
                        _("unknown parameter \"%s\" for \"%s\" command\n"),
                        argv[0], "clear");
            return -1;
        }
    }
    else
        gui_window_clear (gui_current_window);
    return 0;
}

/*
 * weechat_cmd_connect: connect to a server
 */

int
weechat_cmd_connect (int argc, char **argv)
{
    t_irc_server *ptr_server;
    t_irc_channel *ptr_channel;
    
    /* make gcc happy */
    (void) argc;
    
    ptr_server = server_search (argv[0]);
    if (ptr_server)
    {
        if (ptr_server->is_connected)
        {
            gui_printf (NULL,
                        _("%s already connected to server \"%s\"!\n"),
                        WEECHAT_ERROR, argv[0]);
            return -1;
        }
        if (!ptr_server->window)
            gui_window_new (ptr_server, NULL);
        if (server_connect (ptr_server))
        {
            irc_login (ptr_server);
            for (ptr_channel = ptr_server->channels; ptr_channel;
                 ptr_channel = ptr_channel->next_channel)
            {
                if (ptr_channel->type == CHAT_CHANNEL)
                    server_sendf (ptr_server, "JOIN %s\r\n", ptr_channel->name);
            }
        }
    }
    else
    {
        gui_printf (NULL,
                    _("%s server \"%s\" not found\n"),
                    WEECHAT_ERROR, argv[0]);
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
    
    /* make gcc happy */
    (void) argc;
    
    ptr_server = server_search (argv[0]);
    if (ptr_server)
    {
        if (!ptr_server->is_connected)
        {
            gui_printf (NULL,
                        _("%s not connected to server \"%s\"!\n"),
                        WEECHAT_ERROR, argv[0]);
            return -1;
        }
        server_disconnect (ptr_server);
        gui_redraw_window_status (gui_current_window);
    }
    else
    {
        gui_printf (NULL,
                    _("%s server \"%s\" not found\n"),
                    WEECHAT_ERROR, argv[0]);
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
        gui_printf (NULL,
                    _("> List of %s internal commands:\n"), PACKAGE_NAME);
        for (i = 0; weechat_commands[i].command_name; i++)
            gui_printf (NULL, "    %s - %s\n",
                        weechat_commands[i].command_name,
                        weechat_commands[i].command_description);
        gui_printf (NULL, _("> List of IRC commands:\n"));
        for (i = 0; irc_commands[i].command_name; i++)
            if (irc_commands[i].cmd_function_args || irc_commands[i].cmd_function_1arg)
                gui_printf (NULL, "    %s - %s\n",
                            irc_commands[i].command_name,
                            irc_commands[i].command_description);
    }
    if (argc == 1)
    {
        for (i = 0; weechat_commands[i].command_name; i++)
        {
            if (strcasecmp (weechat_commands[i].command_name, argv[0]) == 0)
            {
                gui_printf
                    (NULL,
                     _("> Help on %s internal command \"%s\":\n"),
                     PACKAGE_NAME, weechat_commands[i].command_name);
                gui_printf (NULL,
                            _("Syntax: /%s %s\n"),
                            weechat_commands[i].command_name,
                            (weechat_commands[i].
                             arguments) ? weechat_commands[i].
                            arguments : "");
                if (weechat_commands[i].arguments_description)
                {
                    gui_printf (NULL, "%s\n",
                                weechat_commands[i].
                                arguments_description);
                }
                return 0;
            }
        }
        for (i = 0; irc_commands[i].command_name; i++)
        {
            if (strcasecmp (irc_commands[i].command_name, argv[0]) == 0)
            {
                gui_printf (NULL,
                            _("> Help on IRC command \"%s\":\n"),
                            irc_commands[i].command_name);
                gui_printf (NULL, _("Syntax: /%s %s\n"),
                            irc_commands[i].command_name,
                            (irc_commands[i].arguments) ?
                            irc_commands[i].arguments : "");
                if (irc_commands[i].arguments_description)
                {
                    gui_printf (NULL, "%s\n",
                                irc_commands[i].
                                arguments_description);
                }
                return 0;
            }
        }
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
    int handler_found;
    
    #ifdef PLUGIN_PERL
    switch (argc)
    {
        case 0:
            /* list registered Perl scripts */
            irc_display_prefix (NULL, PREFIX_PLUGIN);
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
            irc_display_prefix (NULL, PREFIX_PLUGIN);
            gui_printf (NULL, _("Perl message handlers:\n"));
            handler_found = 0;
            for (ptr_plugin_handler = plugin_msg_handlers; ptr_plugin_handler;
                 ptr_plugin_handler = ptr_plugin_handler->next_handler)
            {
                if (ptr_plugin_handler->plugin_type == PLUGIN_TYPE_PERL)
                {
                    handler_found = 1;
                    irc_display_prefix (NULL, PREFIX_PLUGIN);
                    gui_printf (NULL, "  IRC(%s) => Perl(%s)\n",
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
            irc_display_prefix (NULL, PREFIX_PLUGIN);
            gui_printf (NULL, _("Perl command handlers:\n"));
            handler_found = 0;
            for (ptr_plugin_handler = plugin_cmd_handlers; ptr_plugin_handler;
                 ptr_plugin_handler = ptr_plugin_handler->next_handler)
            {
                if (ptr_plugin_handler->plugin_type == PLUGIN_TYPE_PERL)
                {
                    handler_found = 1;
                    irc_display_prefix (NULL, PREFIX_PLUGIN);
                    gui_printf (NULL, "  Command /%s => Perl(%s)\n",
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
            if (strcmp (argv[0], "unload") == 0)
            {
                /* unload all Perl scripts */
                plugin_unload (PLUGIN_TYPE_PERL, NULL);
            }
            break;
        case 2:
            if (strcmp (argv[0], "load") == 0)
            {
                /* load Perl script */
                plugin_load (PLUGIN_TYPE_PERL, argv[1]);
            }
            else
            {
                gui_printf (NULL,
                            _("%s unknown option for \"%s\" command\n"),
                            WEECHAT_ERROR, "perl");
            }
            break;
        default:
            gui_printf (NULL,
                        _("%s wrong argument count for \"%s\" command\n"),
                        WEECHAT_ERROR, "perl");
    }
    #else
    gui_printf (NULL,
                _("WeeChat was build without Perl support.\n"
                "Please rebuild WeeChat with "
                "\"--enable-perl\" option for ./configure script\n"));
    /* make gcc happy */
    (void) argc;
    (void) argv;
    #endif /* PLUGIN_PERL */
    
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
    
    if ((argc == 0) || ((argc == 1) && (strcasecmp (argv[0], "list") == 0)))
    {
        /* list all servers */
        if (irc_servers)
        {
            for (ptr_server = irc_servers; ptr_server;
                 ptr_server = ptr_server->next_server)
            {
                irc_display_prefix (NULL, PREFIX_INFO);
                gui_printf_color (NULL,
                                  COLOR_WIN_CHAT,
                                  _("Server: "));
                gui_printf_color (NULL,
                                  COLOR_WIN_CHAT_CHANNEL,
                                  "%s", ptr_server->name);
                gui_printf_color (NULL,
                                  COLOR_WIN_CHAT_DARK,
                                  " [");
                gui_printf_color (NULL,
                                  COLOR_WIN_CHAT,
                                  "%s",
                                  (ptr_server->is_connected) ?
                                      _("connected") : _("not connected"));
                gui_printf_color (NULL,
                                  COLOR_WIN_CHAT_DARK,
                                  "]\n");
                irc_display_prefix (NULL, PREFIX_INFO);
                gui_printf_color (NULL,
                                  COLOR_WIN_CHAT,
                                  _("  Autoconnect: %s\n"),
                                  (ptr_server->autoconnect) ? _("yes") : _("no"));
                irc_display_prefix (NULL, PREFIX_INFO);
                gui_printf_color (NULL,
                                  COLOR_WIN_CHAT,
                                  _("  Hostname   : %s\n"),
                                  ptr_server->address);
                irc_display_prefix (NULL, PREFIX_INFO);
                gui_printf_color (NULL,
                                  COLOR_WIN_CHAT,
                                  _("  Port       : %d\n"),
                                  ptr_server->port);
                irc_display_prefix (NULL, PREFIX_INFO);
                if (ptr_server->password && ptr_server->password[0])
                    gui_printf_color (NULL,
                                      COLOR_WIN_CHAT,
                                      _("  Password   : (hidden)\n"));
                else
                    gui_printf_color (NULL,
                                      COLOR_WIN_CHAT,
                                      _("  Password   : (none)\n"));
                irc_display_prefix (NULL, PREFIX_INFO);
                gui_printf_color (NULL,
                                  COLOR_WIN_CHAT,
                                  _("  Nicks      : %s"),
                                  ptr_server->nick1);
                gui_printf_color (NULL,
                                  COLOR_WIN_CHAT_DARK,
                                  " / ");
                gui_printf_color (NULL,
                                  COLOR_WIN_CHAT,
                                  "%s", ptr_server->nick2);
                gui_printf_color (NULL,
                                  COLOR_WIN_CHAT_DARK,
                                  " / ");
                gui_printf_color (NULL,
                                  COLOR_WIN_CHAT,
                                  "%s\n", ptr_server->nick3);
                irc_display_prefix (NULL, PREFIX_INFO);
                gui_printf_color (NULL,
                                  COLOR_WIN_CHAT,
                                  _("  Username   : %s\n"),
                                  ptr_server->username);
                irc_display_prefix (NULL, PREFIX_INFO);
                gui_printf_color (NULL,
                                  COLOR_WIN_CHAT,
                                  _("  Realname   : %s\n"),
                                  ptr_server->realname);
                irc_display_prefix (NULL, PREFIX_INFO);
                if (ptr_server->command && ptr_server->command[0])
                    gui_printf_color (NULL,
                                      COLOR_WIN_CHAT,
                                      _("  Command    : %s\n"),
                                      ptr_server->command);
                else
                    gui_printf_color (NULL,
                                      COLOR_WIN_CHAT,
                                      _("  Command    : (none)\n"));
                irc_display_prefix (NULL, PREFIX_INFO);
                if (ptr_server->autojoin && ptr_server->autojoin[0])
                    gui_printf_color (NULL,
                                      COLOR_WIN_CHAT,
                                      _("  Auto-join  : %s\n"),
                                      ptr_server->autojoin);
                else
                    gui_printf_color (NULL,
                                      COLOR_WIN_CHAT,
                                      _("  Auto-join  : (none)\n"));
            }
        }
        else
            gui_printf (NULL, _("No server.\n"));
    }
    else
    {
        if (strcasecmp (argv[0], "del") == 0)
        {
            if (argc < 2)
            {
                gui_printf (NULL,
                            _("%s missing servername for \"%s\" command\n"),
                            WEECHAT_ERROR, "server del");
                return -1;
            }
            if (argc > 2)
                gui_printf (NULL,
                            _("%s too much arguments for \"%s\" command, ignoring arguments\n"),
                            WEECHAT_WARNING, "server del");
            
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
                gui_printf (NULL,
                            _("%s server \"%s\" not found for \"%s\" command\n"),
                            WEECHAT_ERROR, argv[1], "server del");
                return -1;
            }
            
            irc_display_prefix (NULL, PREFIX_INFO);
            gui_printf_color (NULL, COLOR_WIN_CHAT, _("Server"));
            gui_printf_color (NULL, COLOR_WIN_CHAT_CHANNEL,
                              " %s ", server_found->name);
            gui_printf_color (NULL, COLOR_WIN_CHAT, _("has been deleted\n"));
            
            server_free (server_found);
            gui_redraw_window (gui_current_window);
            
            return 0;
        }
        
        /* init server struct */
        server_init (&server);
        
        if (argc < 3)
        {
            gui_printf (NULL,
                        _("%s missing parameters for \"%s\" command\n"),
                        WEECHAT_ERROR, "server");
            server_destroy (&server);
            return -1;
        }
        
        if (server_name_already_exists (argv[0]))
        {
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
        new_server = server_new (server.name, server.autoconnect, server.address,
                                 server.port, server.password, server.nick1,
                                 server.nick2, server.nick3, server.username,
                                 server.realname, server.command,
                                 server.autojoin);
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
            gui_printf (NULL,
                        _("%s unable to create server\n"),
                        WEECHAT_ERROR);
            server_destroy (&server);
            return -1;
        }
        
        if (new_server->autoconnect)
        {
            gui_window_new (new_server, NULL);
            if (server_connect (new_server))
                irc_login (new_server);
        }
        
        server_destroy (&server);
    }
    return 0;
}

/*
 * weechat_cmd_set: set options
 */

int
weechat_cmd_set (int argc, char **argv)
{
    int i, j, section_displayed;
    char *color_name;

    /* TODO: complete /set command */
    for (i = 0; i < CONFIG_NUMBER_SECTIONS; i++)
    {
        section_displayed = 0;
        if ((i != CONFIG_SECTION_ALIAS) && (i != CONFIG_SECTION_SERVER))
        {
            for (j = 0; weechat_options[i][j].option_name; j++)
            {
                if ((argc == 0) ||
                    ((argc > 0)
                     && (strstr (weechat_options[i][j].option_name, argv[0])
                         != NULL)))
                {
                    if (!section_displayed)
                    {
                        gui_printf (NULL, "[%s]\n",
                                             config_sections[i].section_name);
                        section_displayed = 1;
                    }
                    switch (weechat_options[i][j].option_type)
                    {
                    case OPTION_TYPE_BOOLEAN:
                        gui_printf (NULL, "  %s = %s\n",
                                    weechat_options[i][j].option_name,
                                    (*weechat_options[i][j].ptr_int) ?
                                        "ON" : "OFF");
                        break;
                    case OPTION_TYPE_INT:
                        gui_printf (NULL,
                                    "  %s = %d\n",
                                    weechat_options[i][j].option_name,
                                    *weechat_options[i][j].ptr_int);
                        break;
                    case OPTION_TYPE_INT_WITH_STRING:
                        gui_printf (NULL,
                                    "  %s = %s\n",
                                    weechat_options[i][j].option_name,
                                    weechat_options[i][j].array_values[*weechat_options[i][j].ptr_int]);
                        break;
                    case OPTION_TYPE_COLOR:
                        color_name = gui_get_color_by_value (*weechat_options[i][j].ptr_int);
                        gui_printf (NULL,
                                    "  %s = %s\n",
                                    weechat_options[i][j].option_name,
                                    (color_name) ? color_name : _("(unknown)"));
                        break;
                    case OPTION_TYPE_STRING:
                        gui_printf (NULL, "  %s = %s\n",
                                    weechat_options[i][j].
                                    option_name,
                                    (*weechat_options[i][j].
                                     ptr_string) ?
                                    *weechat_options[i][j].
                                    ptr_string : "");
                        break;
                    }
                }
            }
        }
    }
    gui_printf (NULL, "(TODO) \"/set\" command not fully developed!\n");
    return 0;
}

/*
 * cmd_unalias: remove an alias
 */

int
weechat_cmd_unalias (char *arguments)
{
    t_index_command *ptr_index;
    t_weechat_alias *ptr_alias;
    
    ptr_index = index_command_search (arguments);
    if (!ptr_index)
    {
        gui_printf (NULL, _("%s alias or command \"%s\" not found\n"),
                    WEECHAT_ERROR, arguments);
        return -1;
    }
    
    index_command_free (ptr_index);
    ptr_alias = alias_search (arguments);
    if (ptr_alias)
        alias_free (ptr_alias);
    gui_printf (NULL, _("Alias \"%s\" removed\n"),
                arguments);
    return 0;
}
