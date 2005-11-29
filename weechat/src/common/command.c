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
    N_("[action | number | [[server] [channel]]]"),
    N_("action: action to do:\n"
       "  move: move buffer in the list (may be relative, for example -1)\n"
       "  close: close buffer (for channel: same as /part without part message)\n"
       "  list: list opened buffers (no parameter implies this list)\n"
       "  notify: set notify level for buffer (0=never, 1=highlight, 2=1+msg, 3=2+join/part)\n"
       "server,channel: jump to buffer by server and/or channel name\n"
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
  { "debug", N_("print debug messages"),
    N_("dump | windows"),
    N_("dump: save memory dump in WeeChat log file (same dump is written when WeeChat crashes)\n"
       "windows: display windows tree"),
    1, 1, weechat_cmd_debug, NULL },
  { "help", N_("display help about commands"),
    N_("[command]"), N_("command: name of a WeeChat or IRC command"),
    0, 1, weechat_cmd_help, NULL },
  { "history", N_("show buffer command history"),
    N_("[clear | value]"),
    N_("clear: clear history\n"
       "value: number of history entries to show"
        ),
    0, 1, weechat_cmd_history, NULL },
  { "ignore", N_("ignore IRC messages and/or hosts"),
    N_("[mask [[type | command] [channel [server]]]]"),
    N_("   mask: nick or host mask to ignore\n"
       "   type: type of message to ignore (action, ctcp, dcc, pv)\n"
       "command: IRC command\n"
       "channel: name of channel for ignore\n"
       " server: name of server for ignore\n\n"
       "For each argument, '*' means all.\n"
       "Without argument, /ignore command lists all defined ignore."),
    0, 4, weechat_cmd_ignore, NULL },
  { "key", N_("bind/unbind keys"),
    N_("[key function/command] [unbind key] [functions] [reset -yes]"),
    N_("key: bind this key to an internal function or a command (beginning by \"/\")\n"
       "unbind: unbind a key (if \"all\", default bindings are restored)\n"
       "functions: list internal functions for key bindings\n"
       "reset: restore bindings to the default values and delete ALL personal binding (use carefully!)"),
    0, MAX_ARGS, NULL, weechat_cmd_key },
  { "plugin", N_("list/load/unload plugins"),
    N_("[load filename] | [autoload] | [reload] | [unload]"),
    N_("filename: WeeChat plugin (file) to load\n\n"
       "Without argument, /plugin command lists all loaded plugins."),
    0, 2, weechat_cmd_plugin, NULL },
  { "server", N_("list, add or remove servers"),
    N_("[servername] | "
       "[servername hostname port [-auto | -noauto] [-ipv6] [-ssl] [-pwd password] [-nicks nick1 "
       "nick2 nick3] [-username username] [-realname realname] "
       "[-command command] [-autojoin channel[,channel]] ] | "
       "[del servername]"),
    N_("servername: server name, for internal & display use\n"
       "hostname: name or IP address of server\n"
       "port: port for server (integer)\n"
       "ipv6: use IPv6 protocol\n"
       "ssl: use SSL protocol\n"
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
    N_("[option [ = value]]"),
    N_("option: name of an option (if name is full "
       "and no value is given, then help is displayed on option)\n"
       "value: value for option"),
    0, MAX_ARGS, NULL, weechat_cmd_set },
  { "unalias", N_("remove an alias"),
    N_("alias_name"), N_("alias_name: name of alias to remove"),
    1, 1, NULL, weechat_cmd_unalias },
  { "unignore", N_("unignore IRC messages and/or hosts"),
    N_("[number | [mask [[type | command] [channel [server]]]]]"),
    N_(" number: # of ignore to unignore (number is displayed by list of ignore)\n"
       "   mask: nick or host mask to unignore\n"
       "   type: type of message to unignore (action, ctcp, dcc, pv)\n"
       "command: IRC command\n"
       "channel: name of channel for unignore\n"
       " server: name of server for unignore\n\n"
       "For each argument, '*' means all.\n"
       "Without argument, /unignore command lists all defined ignore."),
    0, 4, weechat_cmd_unignore, NULL },
  { "uptime", N_("show WeeChat uptime"),
    N_("[-o]"),
    N_("-o: send uptime on current channel as an IRC message"),
    0, 1, weechat_cmd_uptime, NULL },
  { "window", N_("manage windows"),
    N_("[list | -1 | +1 | b# | splith [pct] | splitv [pct] | resize pct | merge [all]]"),
    N_("list: list opened windows (no parameter implies this list)\n"
       "-1: jump to previous window\n"
       "+1: jump to next window\n"
       "b#: jump to next window displaying buffer number #\n"
       "splith: split current window horizontally\n"
       "splitv: split current window vertically\n"
       "resize: resize window size, new size is <pct>%% of parent window\n"
       "merge: merge window with another (all = keep only one window)\n\n"
       "For splith and splitv, pct is a pourcentage which represents "
       "size of new window, computed with current window as size reference. "
       "For example 25 means create a new window with size = current_size / 4"),
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
        if (ascii_strcasecmp (alias_name, ptr_alias->alias_name) == 0)
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
        if (ascii_strcasecmp (alias_name, ptr_alias->alias_name) < 0)
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
        irc_display_prefix (NULL, NULL, PREFIX_ERROR);
        gui_printf (NULL, _("%s alias or command \"%s\" already exists!\n"),
                    WEECHAT_ERROR, alias_name);
        return NULL;
    }
    pos = strchr (alias_command, ' ');
    if (pos)
        pos[0] = '\0';
    if (alias_search (alias_command))
    {
        irc_display_prefix (NULL, NULL, PREFIX_ERROR);
        gui_printf (NULL, _("%s alias cannot run another alias!\n"),
                    WEECHAT_ERROR);
        return NULL;
    }
    if (!weelist_search (index_commands, alias_command))
    {
        irc_display_prefix (NULL, NULL, PREFIX_ERROR);
        gui_printf (NULL, _("%s target command \"/%s\" does not exist!\n"),
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
 * free_exploded_string: free an exploded string
 */

void
free_exploded_string (char **exploded_string)
{
    int i;
    
    if (exploded_string)
    {
        for (i = 0; exploded_string[i]; i++)
            free (exploded_string[i]);
        free (exploded_string);
    }
}

/*
 * exec_weechat_command: executes a command (WeeChat internal or IRC)
 *                       returns: 1 if command was executed succesfully
 *                                0 if error (command not executed)
 */

int
exec_weechat_command (t_irc_server *server, char *string)
{
    int i, argc, return_code, length1, length2;
    char *command, *pos, *ptr_args, *ptr_args_color, **argv, *alias_command;
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
    
    ptr_args_color = NULL;
    
    if (ptr_args && (cfg_irc_colors_send))
    {
        ptr_args_color = (char *)gui_color_encode ((unsigned char *)ptr_args);
        if (ptr_args_color)
            ptr_args = ptr_args_color;
    }
    
#ifdef PLUGINS
    if (!plugin_cmd_handler_exec ((server) ? server->name : "", command + 1, ptr_args))
#else
    if (1)
#endif
    {
        argv = explode_string (ptr_args, " ", 0, &argc);
        
        for (i = 0; weechat_commands[i].command_name; i++)
        {
            if (ascii_strcasecmp (weechat_commands[i].command_name, command + 1) == 0)
            {
                if ((argc < weechat_commands[i].min_arg)
                    || (argc > weechat_commands[i].max_arg))
                {
                    if (weechat_commands[i].min_arg ==
                        weechat_commands[i].max_arg)
                    {
                        irc_display_prefix (NULL, NULL, PREFIX_ERROR);
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
                        irc_display_prefix (NULL, NULL, PREFIX_ERROR);
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
                        irc_display_prefix (NULL, NULL, PREFIX_ERROR);
                        gui_printf (NULL,
                                    _("%s command \"%s\" failed\n"),
                                    WEECHAT_ERROR, command + 1);
                    }
                }
                free_exploded_string (argv);
                free (command);
                if (ptr_args_color)
                    free (ptr_args_color);
                return 1;
            }
        }
        for (i = 0; irc_commands[i].command_name; i++)
        {
            if ((ascii_strcasecmp (irc_commands[i].command_name, command + 1) == 0) &&
                ((irc_commands[i].cmd_function_args) ||
                (irc_commands[i].cmd_function_1arg)))
            {
                if ((argc < irc_commands[i].min_arg)
                    || (argc > irc_commands[i].max_arg))
                {
                    if (irc_commands[i].min_arg == irc_commands[i].max_arg)
                    {
                        irc_display_prefix (NULL, NULL, PREFIX_ERROR);
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
                        irc_display_prefix (NULL, NULL, PREFIX_ERROR);
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
                        irc_display_prefix (NULL, NULL, PREFIX_ERROR);
                        gui_printf (NULL,
                                    _("%s command \"%s\" needs a server connection!\n"),
                                    WEECHAT_ERROR, irc_commands[i].command_name);
                        free (command);
                        if (ptr_args_color)
                            free (ptr_args_color);
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
                        irc_display_prefix (NULL, NULL, PREFIX_ERROR);
                        gui_printf (NULL,
                                    _("%s command \"%s\" failed\n"),
                                    WEECHAT_ERROR, command + 1);
                    }
                }
                free_exploded_string (argv);
                free (command);
                if (ptr_args_color)
                    free (ptr_args_color);
                return 1;
            }
        }
        for (ptr_alias = weechat_alias; ptr_alias;
             ptr_alias = ptr_alias->next_alias)
        {
            if (ascii_strcasecmp (ptr_alias->alias_name, command + 1) == 0)
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
                
                free_exploded_string (argv);
                free (command);
                if (ptr_args_color)
                    free (ptr_args_color);
                return 1;
            }
        }
        irc_display_prefix (NULL, NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("%s unknown command \"%s\" (type /help for help)\n"),
                    WEECHAT_ERROR,
                    command + 1);
        free_exploded_string (argv);
    }
    free (command);
    if (ptr_args_color)
        free (ptr_args_color);
    return 0;
}

/*
 * user_command: interprets user command (if beginning with '/')
 *               any other text is sent to the server, if connected
 */

void
user_command (t_irc_server *server, t_gui_buffer *buffer, char *command)
{
    t_irc_nick *ptr_nick;
    int plugin_args_length;
    char *command_with_colors, *command_with_colors2, *plugin_args;
    
    if ((!command) || (!command[0]) || (command[0] == '\r') || (command[0] == '\n'))
        return;
    
    if ((command[0] == '/') && (command[1] != '/'))
    {
        /* WeeChat internal command (or IRC command) */
        (void) exec_weechat_command (server, command);
    }
    else
    {
        if (!buffer)
            buffer = gui_current_window->buffer;
        
        if ((command[0] == '/') && (command[1] == '/'))
            command++;
        
        if (server && (!BUFFER_IS_SERVER(buffer)))
        {
            command_with_colors = (cfg_irc_colors_send) ?
                (char *)gui_color_encode ((unsigned char *)command) : NULL;
            
            if (CHANNEL(buffer)->dcc_chat)
                dcc_chat_sendf ((t_irc_dcc *)(CHANNEL(buffer)->dcc_chat),
                                "%s\r\n",
                                (command_with_colors) ? command_with_colors : command);
            else
                server_sendf (server, "PRIVMSG %s :%s\r\n",
                              CHANNEL(buffer)->name,
                              (command_with_colors) ?
                              command_with_colors : command);
            
            command_with_colors2 = (command_with_colors) ?
                (char *)gui_color_decode ((unsigned char *)command_with_colors, 1) : NULL;
            
            if (CHANNEL(buffer)->type == CHAT_PRIVATE)
            {
                gui_printf_type (CHANNEL(buffer)->buffer,
                                 MSG_TYPE_NICK,
                                 "%s<%s%s%s> ",
                                 GUI_COLOR(COLOR_WIN_CHAT_DARK),
                                 GUI_COLOR(COLOR_WIN_NICK_SELF),
                                 server->nick,
                                 GUI_COLOR(COLOR_WIN_CHAT_DARK));
                gui_printf_type (CHANNEL(buffer)->buffer,
                                 MSG_TYPE_MSG,
                                 "%s%s\n",
                                 GUI_COLOR(COLOR_WIN_CHAT),
                                 (command_with_colors2) ?
                                 command_with_colors2 : command);
            }
            else
            {
                ptr_nick = nick_search (CHANNEL(buffer), server->nick);
                if (ptr_nick)
                {
                    irc_display_nick (CHANNEL(buffer)->buffer, ptr_nick, NULL,
                                      MSG_TYPE_NICK, 1, 1, 0);
                    gui_printf (CHANNEL(buffer)->buffer,
                                "%s\n",
                                (command_with_colors2) ?
                                command_with_colors2 : command);
                }
                else
                {
                    irc_display_prefix (server, server->buffer, PREFIX_ERROR);
                    gui_printf (server->buffer,
                                _("%s cannot find nick for sending message\n"),
                                WEECHAT_ERROR);
                }
            }
            
            if (command_with_colors)
                free (command_with_colors);
            if (command_with_colors2)
                free (command_with_colors2);
            
            /* sending a copy of the message as PRIVMSG to plugins because irc server doesn't */
            
            /* code commented by FlashCode, 2005-11-06: problem when a handler
               is called after a weechat::command("somethin") in perl, reetrance,
               and crash at perl script unload */
            
            /* make gcc happy */
            (void) plugin_args_length;
            (void) plugin_args;
            /*plugin_args_length = strlen ("localhost PRIVMSG  :") +
                strlen (CHANNEL(buffer)->name) + strlen(command) + 16;
            plugin_args = (char *) malloc (plugin_args_length * sizeof (*plugin_args));
            
            if (plugin_args)
            {
                snprintf (plugin_args, plugin_args_length,
                          "localhost PRIVMSG %s :%s", 
                          CHANNEL(buffer)->name, command);
#ifdef PLUGINS
                plugin_msg_handler_exec (server->name, "privmsg", plugin_args);
#endif
                free (plugin_args);
            }
            else
            {
                irc_display_prefix (NULL, NULL, PREFIX_ERROR);
                gui_printf (NULL,
                            _("%s unable to call handler for message (not enough memory)\n"),
                            WEECHAT_ERROR);
            }*/
        }
        else
        {
            irc_display_prefix (NULL, (server) ? server->buffer : NULL, PREFIX_ERROR);
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
                irc_display_prefix (NULL, NULL, PREFIX_ERROR);
                gui_printf (NULL, _("%s missing arguments for \"%s\" command\n"),
                            WEECHAT_ERROR, "alias");
                return -1;
            }
            if (arguments[0] == '/')
            {
                irc_display_prefix (NULL, NULL, PREFIX_ERROR);
                gui_printf (NULL, _("%s alias can not start with \"/\"\n"),
                            WEECHAT_ERROR, "alias");
                return -1;
            }
            if (!alias_new (arguments, pos))
                return -1;
            if (weelist_add (&index_commands, &last_index_command, arguments))
            {
                irc_display_prefix (NULL, NULL, PREFIX_INFO);
                gui_printf (NULL, _("Alias \"%s\" => \"%s\" created\n"),
                            arguments, pos);
            }
            else
            {
                irc_display_prefix (NULL, NULL, PREFIX_ERROR);
                gui_printf (NULL, _("Failed to create alias \"%s\" => \"%s\" "
                            "(not enough memory)\n"),
                            arguments, pos);
                return -1;
            }
        }
        else
        {
            irc_display_prefix (NULL, NULL, PREFIX_ERROR);
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
                gui_printf (NULL, "  %s %s=>%s %s\n",
                            ptr_alias->alias_name,
                            GUI_COLOR(COLOR_WIN_CHAT_DARK),
                            GUI_COLOR(COLOR_WIN_CHAT),
                            ptr_alias->alias_command + 1);
            }
        }
        else
        {
            irc_display_prefix (NULL, NULL, PREFIX_INFO);
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
        gui_printf (NULL, "%sDCC\n",
                    GUI_COLOR(COLOR_WIN_CHAT_CHANNEL));
    else if (BUFFER_IS_SERVER (buffer))
        gui_printf (NULL, _("%sServer: %s%s\n"),
                    GUI_COLOR(COLOR_WIN_CHAT),
                    GUI_COLOR(COLOR_WIN_CHAT_SERVER),
                    SERVER(buffer)->name);
    else if (BUFFER_IS_CHANNEL (buffer))
        gui_printf (NULL, _("%sChannel: %s%s %s(server: %s%s%s)\n"),
                    GUI_COLOR(COLOR_WIN_CHAT),
                    GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                    CHANNEL(buffer)->name,
                    GUI_COLOR(COLOR_WIN_CHAT),
                    GUI_COLOR(COLOR_WIN_CHAT_SERVER),
                    SERVER(buffer)->name,
                    GUI_COLOR(COLOR_WIN_CHAT));
    else if (BUFFER_IS_PRIVATE (buffer))
        gui_printf (NULL, _("%sPrivate with: %s%s %s(server: %s%s%s)\n"),
                    GUI_COLOR(COLOR_WIN_CHAT),
                    GUI_COLOR(COLOR_WIN_CHAT_NICK),
                    CHANNEL(buffer)->name,
                    GUI_COLOR(COLOR_WIN_CHAT),
                    GUI_COLOR(COLOR_WIN_CHAT_SERVER),
                    SERVER(buffer)->name,
                    GUI_COLOR(COLOR_WIN_CHAT));
    else
        gui_printf (NULL, _("not connected\n"));
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
    int target_buffer;
    
    if ((argc == 0) || ((argc == 1) && (ascii_strcasecmp (argv[0], "list") == 0)))
    {
        /* list opened buffers */
        
        gui_printf (NULL, "\n");
        gui_printf (NULL, _("Opened buffers:\n"));
        
        for (ptr_buffer = gui_buffers; ptr_buffer; ptr_buffer = ptr_buffer->next_buffer)
        {
            gui_printf (NULL, "%s[%s%d%s] ",
                        GUI_COLOR(COLOR_WIN_CHAT_DARK),
                        GUI_COLOR(COLOR_WIN_CHAT),
                        ptr_buffer->number,
                        GUI_COLOR(COLOR_WIN_CHAT_DARK));
            weechat_cmd_buffer_display_info (ptr_buffer);
        }
    }
    else
    {
        if (ascii_strcasecmp (argv[0], "move") == 0)
        {
            /* move buffer to another number in the list */
            
            if (argc < 2)
            {
                irc_display_prefix (NULL, NULL, PREFIX_ERROR);
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
                    gui_buffer_move_to_number (gui_current_window,
                                               gui_current_window->buffer->number + ((int) number));
                else if (argv[1][0] == '-')
                    gui_buffer_move_to_number (gui_current_window,
                                               gui_current_window->buffer->number - ((int) number));
                else
                    gui_buffer_move_to_number (gui_current_window, (int) number);
            }
            else
            {
                /* invalid number */
                irc_display_prefix (NULL, NULL, PREFIX_ERROR);
                gui_printf (NULL, _("%s incorrect buffer number\n"),
                            WEECHAT_ERROR);
                return -1;
            }
        }
        else if (ascii_strcasecmp (argv[0], "close") == 0)
        {
            /* close buffer (server or channel/private) */
            
            if ((!gui_current_window->buffer->next_buffer)
                && (gui_current_window->buffer == gui_buffers)
                && ((!gui_current_window->buffer->all_servers)
                    || (!SERVER(gui_current_window->buffer))))
            {
                irc_display_prefix (NULL, NULL, PREFIX_ERROR);
                gui_printf (NULL,
                            _("%s can not close the single buffer\n"),
                            WEECHAT_ERROR);
                return -1;
            }
            if (BUFFER_IS_SERVER(gui_current_window->buffer))
            {
                if (SERVER(gui_current_window->buffer)->channels)
                {
                    irc_display_prefix (NULL, NULL, PREFIX_ERROR);
                    gui_printf (NULL,
                                _("%s can not close server buffer while channels "
                                "are opened\n"),
                                WEECHAT_ERROR);
                    return -1;
                }
                server_disconnect (SERVER(gui_current_window->buffer), 0);
                ptr_server = SERVER(gui_current_window->buffer);
                if (!gui_current_window->buffer->all_servers)
                {
                    gui_buffer_free (gui_current_window->buffer, 1);
                    ptr_server->buffer = NULL;
                }
                else
                {
                    ptr_server->buffer = NULL;
                    gui_current_window->buffer->server = NULL;
                    gui_window_switch_server (gui_current_window);
                }

            }
            else
            {
                if (SERVER(gui_current_window->buffer))
                {
                    if (SERVER(gui_current_window->buffer)->is_connected
                        && CHANNEL(gui_current_window->buffer)
                        && CHANNEL(gui_current_window->buffer)->nicks)
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
        else if (ascii_strcasecmp (argv[0], "notify") == 0)
        {
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
                    if ((!BUFFER_IS_CHANNEL(ptr_buffer))
                        && (!BUFFER_IS_PRIVATE(ptr_buffer)))
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
                /* set notify level for buffer */
                error = NULL;
                number = strtol (argv[1], &error, 10);
                if ((error) && (error[0] == '\0'))
                {
                    if ((number < NOTIFY_LEVEL_MIN) || (number > NOTIFY_LEVEL_MAX))
                    {
                        /* invalid highlight level */
                        irc_display_prefix (NULL, NULL, PREFIX_ERROR);
                        gui_printf (NULL, _("%s incorrect notify level (must be between %d and %d)\n"),
                                    WEECHAT_ERROR, NOTIFY_LEVEL_MIN, NOTIFY_LEVEL_MAX);
                        return -1;
                    }
                    if ((!BUFFER_IS_CHANNEL(gui_current_window->buffer))
                        && (!BUFFER_IS_PRIVATE(gui_current_window->buffer)))
                    {
                        /* invalid buffer type (only ok on channel or private) */
                        irc_display_prefix (NULL, NULL, PREFIX_ERROR);
                        gui_printf (NULL, _("%s incorrect buffer for notify (must be channel or private)\n"),
                                    WEECHAT_ERROR);
                        return -1;
                    }
                    gui_current_window->buffer->notify_level = number;
                    channel_set_notify_level (SERVER(gui_current_window->buffer),
                                              CHANNEL(gui_current_window->buffer),
                                              number);
                    irc_display_prefix (NULL, NULL, PREFIX_INFO);
                    gui_printf (NULL, _("New notify level for %s%s%s: %s%d %s"),
                                GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                                CHANNEL(gui_current_window->buffer)->name,
                                GUI_COLOR(COLOR_WIN_CHAT),
                                GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                                number,
                                GUI_COLOR(COLOR_WIN_CHAT));
                    switch (number)
                    {
                        case 0:
                            gui_printf (NULL, _("(hotlist: never)\n"));
                            break;
                        case 1:
                            gui_printf (NULL, _("(hotlist: highlights)\n"));
                            break;
                        case 2:
                            gui_printf (NULL, _("(hotlist: highlights + messages)\n"));
                            break;
                        case 3:
                            gui_printf (NULL, _("(hotlist: highlights + messages + join/part (all))\n"));
                            break;
                        default:
                            gui_printf (NULL, "\n");
                            break;
                    }
                }
                else
                {
                    /* invalid number */
                    irc_display_prefix (NULL, NULL, PREFIX_ERROR);
                    gui_printf (NULL, _("%s incorrect notify level (must be between %d and %d)\n"),
                                WEECHAT_ERROR, NOTIFY_LEVEL_MIN, NOTIFY_LEVEL_MAX);
                    return -1;
                }
            }
        }
        else
        {
            /* jump to buffer by number or server/channel name */
            
            if (argv[0][0] == '-')
            {
                /* relative jump '-' */
                error = NULL;
                number = strtol (argv[0] + 1, &error, 10);
                if ((error) && (error[0] == '\0'))
                {
                    target_buffer = gui_current_window->buffer->number - (int) number;
                    if (target_buffer < 1)
                        target_buffer = (last_gui_buffer) ?
                            last_gui_buffer->number + target_buffer : 1;
                    gui_buffer_switch_by_number (gui_current_window,
                                                 target_buffer);
                }
            }
            else if (argv[0][0] == '+')
            {
                /* relative jump '+' */
                error = NULL;
                number = strtol (argv[0] + 1, &error, 10);
                if ((error) && (error[0] == '\0'))
                {
                    target_buffer = gui_current_window->buffer->number + (int) number;
                    if (last_gui_buffer && target_buffer > last_gui_buffer->number)
                        target_buffer -= last_gui_buffer->number;
                    gui_buffer_switch_by_number (gui_current_window,
                                                 target_buffer);
                }
            }
            else
            {
                /* absolute jump by number, or by server/channel name */
                error = NULL;
                number = strtol (argv[0], &error, 10);
                if ((error) && (error[0] == '\0'))
                    gui_buffer_switch_by_number (gui_current_window, (int) number);
                else
                {
                    ptr_buffer = NULL;
                    if (argc > 1)
                        ptr_buffer = gui_buffer_search (argv[0], argv[1]);
                    else
                    {
                        if (string_is_channel (argv[0]))
                            ptr_buffer = gui_buffer_search (NULL, argv[0]);
                        else
                            ptr_buffer = gui_buffer_search (argv[0], NULL);
                    }
                    if (ptr_buffer)
                    {
                        gui_switch_to_buffer (gui_current_window, ptr_buffer);
                        gui_redraw_buffer (ptr_buffer);
                    }
                }
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
        if (ascii_strcasecmp (argv[0], "-all") == 0)
            gui_buffer_clear_all ();
        else
        {
            irc_display_prefix (NULL, NULL, PREFIX_ERROR);
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
            irc_display_prefix (NULL, NULL, PREFIX_ERROR);
            gui_printf (NULL,
                        _("%s already connected to server \"%s\"!\n"),
                        WEECHAT_ERROR, ptr_server->name);
            return -1;
        }
        if (ptr_server->child_pid > 0)
        {
            irc_display_prefix (NULL, NULL, PREFIX_ERROR);
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
        irc_display_prefix (NULL, NULL, PREFIX_ERROR);
        gui_printf (NULL, _("%s server not found\n"), WEECHAT_ERROR);
        return -1;
    }
    return 0;
}

/*
 * weechat_cmd_debug_display_windows: display tree of windows
 */

void
weechat_cmd_debug_display_windows (t_gui_window_tree *tree, int indent)
{
    int i;
    
    if (tree)
    {
        for (i = 0; i < indent; i++)
            gui_printf_nolog (NULL, "  ");
        
        if (tree->window)
        {
            /* leaf */
            gui_printf_nolog (NULL, "leaf: %X (parent:%X), win=%X, child1=%X, child2=%X, %d,%d %dx%d, %d%%x%d%%\n",
                              tree, tree->parent_node, tree->window,
                              tree->child1, tree->child2,
                              tree->window->win_x, tree->window->win_y,
                              tree->window->win_width, tree->window->win_height,
                              tree->window->win_width_pct, tree->window->win_height_pct);
        }
        else
        {
            /* node */
            gui_printf_nolog (NULL, "node: %X (parent:%X), win=%X, child1=%X, child2=%X)\n",
                              tree, tree->parent_node, tree->window,
                              tree->child1, tree->child2);
        }
        
        if (tree->child1)
            weechat_cmd_debug_display_windows (tree->child1, indent + 1);
        if (tree->child2)
            weechat_cmd_debug_display_windows (tree->child2, indent + 1);
    }
}

/*
 * weechat_cmd_debug: print debug messages
 */

int
weechat_cmd_debug (int argc, char **argv)
{
    if (argc != 1)
    {
        irc_display_prefix (NULL, NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("%s wrong argument count for \"%s\" command\n"),
                    WEECHAT_ERROR, "debug");
        return -1;
    }
    
    if (ascii_strcasecmp (argv[0], "dump") == 0)
    {
        wee_dump (0);
    }
    else if (ascii_strcasecmp (argv[0], "windows") == 0)
    {
        gui_printf_nolog (NULL, "\n");
        gui_printf_nolog (NULL, "DEBUG: windows tree:\n");
        weechat_cmd_debug_display_windows (gui_windows_tree, 1);
    }
    else
    {
        irc_display_prefix (NULL, NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("%s unknown option for \"%s\" command\n"),
                    WEECHAT_ERROR, "debug");
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
            irc_display_prefix (NULL, ptr_server->buffer, PREFIX_ERROR);
            gui_printf (ptr_server->buffer,
                        _("%s not connected to server \"%s\"!\n"),
                        WEECHAT_ERROR, ptr_server->name);
            return -1;
        }
        if (ptr_server->reconnect_start > 0)
        {
            irc_display_prefix (NULL, ptr_server->buffer, PREFIX_INFO);
            gui_printf (ptr_server->buffer,
                        _("Auto-reconnection is cancelled\n"));
        }
        server_disconnect (ptr_server, 0);
        gui_draw_buffer_status (gui_current_window->buffer, 1);
    }
    else
    {
        irc_display_prefix (NULL, NULL, PREFIX_ERROR);
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
#ifdef PLUGINS
    t_weechat_plugin *ptr_plugin;
    t_plugin_handler *ptr_handler;
#endif

    switch (argc)
    {
        case 0:
            gui_printf (NULL, "\n");
            gui_printf (NULL, _("%s internal commands:\n"), PACKAGE_NAME);
            for (i = 0; weechat_commands[i].command_name; i++)
            {
                gui_printf (NULL, "   %s%s %s- %s\n",
                            GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                            weechat_commands[i].command_name,
                            GUI_COLOR(COLOR_WIN_CHAT),
                            _(weechat_commands[i].command_description));
            }
            gui_printf (NULL, "\n");
            gui_printf (NULL, _("IRC commands:\n"));
            for (i = 0; irc_commands[i].command_name; i++)
            {
                if (irc_commands[i].cmd_function_args || irc_commands[i].cmd_function_1arg)
                {
                    gui_printf (NULL, "   %s%s %s- %s\n",
                                GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                                irc_commands[i].command_name,
                                GUI_COLOR(COLOR_WIN_CHAT),
                                _(irc_commands[i].command_description));
                }
            }
#ifdef PLUGINS
            gui_printf (NULL, "\n");
            gui_printf (NULL, _("Plugin commands:\n"));
            for (ptr_plugin = weechat_plugins; ptr_plugin;
                 ptr_plugin = ptr_plugin->next_plugin)
            {
                for (ptr_handler = ptr_plugin->handlers;
                     ptr_handler; ptr_handler = ptr_handler->next_handler)
                {
                    if (ptr_handler->type == HANDLER_COMMAND)
                    {
                        gui_printf (NULL, "   %s%s",
                                    GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                                    ptr_handler->command);
                        if (ptr_handler->description
                            && ptr_handler->description[0])
                            gui_printf (NULL, " %s- %s",
                                        GUI_COLOR(COLOR_WIN_CHAT),
                                        ptr_handler->description);
                        gui_printf (NULL, "\n");
                    }
                }
            }
#endif
            break;
        case 1:
            for (i = 0; weechat_commands[i].command_name; i++)
            {
                if (ascii_strcasecmp (weechat_commands[i].command_name, argv[0]) == 0)
                {
                    gui_printf (NULL, "\n");
                    gui_printf (NULL, "[w]");
                    gui_printf (NULL, "  %s/%s",
                                GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                                weechat_commands[i].command_name);
                    if (weechat_commands[i].arguments &&
                        weechat_commands[i].arguments[0])
                        gui_printf (NULL, "  %s%s\n",
                                    GUI_COLOR(COLOR_WIN_CHAT),
                                    _(weechat_commands[i].arguments));
                    else
                        gui_printf (NULL, "\n");
                    if (weechat_commands[i].command_description &&
                        weechat_commands[i].command_description[0])
                        gui_printf (NULL, "\n%s\n",
                                    _(weechat_commands[i].command_description));
                    if (weechat_commands[i].arguments_description &&
                        weechat_commands[i].arguments_description[0])
                        gui_printf (NULL, "\n%s\n",
                                    _(weechat_commands[i].arguments_description));
                    return 0;
                }
            }
            for (i = 0; irc_commands[i].command_name; i++)
            {
                if ((ascii_strcasecmp (irc_commands[i].command_name, argv[0]) == 0)
                    && (irc_commands[i].cmd_function_args || irc_commands[i].cmd_function_1arg))
                {
                    gui_printf (NULL, "\n");
                    gui_printf (NULL, "[i]");
                    gui_printf (NULL, "  %s/%s",
                                GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                                irc_commands[i].command_name);
                    if (irc_commands[i].arguments &&
                        irc_commands[i].arguments[0])
                        gui_printf (NULL, "  %s%s\n",
                                    GUI_COLOR(COLOR_WIN_CHAT),
                                    _(irc_commands[i].arguments));
                    else
                        gui_printf (NULL, "\n");
                    if (irc_commands[i].command_description &&
                        irc_commands[i].command_description[0])
                        gui_printf (NULL, "\n%s\n",
                                    _(irc_commands[i].command_description));
                    if (irc_commands[i].arguments_description &&
                        irc_commands[i].arguments_description[0])
                        gui_printf (NULL, "\n%s\n",
                                    _(irc_commands[i].arguments_description));
                    return 0;
                }
            }
#ifdef PLUGINS
            for (ptr_plugin = weechat_plugins; ptr_plugin;
                 ptr_plugin = ptr_plugin->next_plugin)
            {
                for (ptr_handler = ptr_plugin->handlers;
                     ptr_handler; ptr_handler = ptr_handler->next_handler)
                {
                    if ((ptr_handler->type == HANDLER_COMMAND)
                        && (ascii_strcasecmp (ptr_handler->command, argv[0]) == 0))
                    {
                        gui_printf (NULL, "\n");
                        gui_printf (NULL, "[p]");
                        gui_printf (NULL, "  %s/%s",
                                    GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                                    ptr_handler->command);
                        if (ptr_handler->arguments &&
                            ptr_handler->arguments[0])
                            gui_printf (NULL, "  %s%s\n",
                                        GUI_COLOR(COLOR_WIN_CHAT),
                                        ptr_handler->arguments);
                        else
                            gui_printf (NULL, "\n");
                        if (ptr_handler->description &&
                            ptr_handler->description[0])
                            gui_printf (NULL, "\n%s\n",
                                        ptr_handler->description);
                        if (ptr_handler->arguments_description &&
                            ptr_handler->arguments_description[0])
                            gui_printf (NULL, "\n%s\n",
                                        ptr_handler->arguments_description);
                        return 0;
                    }
                }
            }
#endif
            irc_display_prefix (NULL, NULL, PREFIX_ERROR);
            gui_printf (NULL,
                        _("No help available, \"%s\" is an unknown command\n"),
                        argv[0]);
            break;
    }
    return 0;
}

/*
 * weechat_cmd_history: display current buffer history
 */

int
weechat_cmd_history (int argc, char **argv) {

    t_history *p;
    int n;
    int n_total;
    int n_user = cfg_history_display_default;

    if (argc == 1)
    {
        if (ascii_strcasecmp (argv[0], "clear") == 0)
        {
            history_buffer_free (gui_current_window->buffer);
            return 0;
        }
        else
            n_user = atoi (argv[0]);
    }

    if (gui_current_window->buffer->history != NULL)
    {
        for(n_total = 1, p = gui_current_window->buffer->history; p->next_history != NULL; p = p->next_history, n_total++);
        for(n=0; p != NULL; p=p->prev_history, n++)
        {
            if (n_user > 0 && (n_total-n_user) > n)
                continue;
            irc_display_prefix (NULL, gui_current_window->buffer,
                                PREFIX_INFO);
            gui_printf_nolog (gui_current_window->buffer,
                              "%s\n", p->text);
        }
    }

    return 0;
}

/*
 * weechat_cmd_ignore_display: display an ignore entry
 */

void
weechat_cmd_ignore_display (char *text, t_irc_ignore *ptr_ignore)
{
    if (text)
        gui_printf (NULL, "%s%s ",
                    GUI_COLOR(COLOR_WIN_CHAT),
                    text);
    
    gui_printf (NULL, _("%son %s%s%s/%s%s%s:%s ignoring %s%s%s from %s%s\n"),
                GUI_COLOR(COLOR_WIN_CHAT),
                GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                ptr_ignore->server_name,
                GUI_COLOR(COLOR_WIN_CHAT_DARK),
                GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                ptr_ignore->channel_name,
                GUI_COLOR(COLOR_WIN_CHAT_DARK),
                GUI_COLOR(COLOR_WIN_CHAT),
                GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                ptr_ignore->type,
                GUI_COLOR(COLOR_WIN_CHAT),
                GUI_COLOR(COLOR_WIN_CHAT_HOST),
                ptr_ignore->mask);
}

/*
 * weechat_cmd_ignore: ignore IRC commands and/or hosts
 */

int
weechat_cmd_ignore (int argc, char **argv)
{
    t_irc_ignore *ptr_ignore;
    int i;
    
    ptr_ignore = NULL;
    switch (argc)
    {
        case 0:
            /* List all ignore */
            if (irc_ignore)
            {
                gui_printf (NULL, "\n");
                gui_printf (NULL, _("List of ignore:\n"));
                i = 0;
                for (ptr_ignore = irc_ignore; ptr_ignore;
                     ptr_ignore = ptr_ignore->next_ignore)
                {
                    i++;
                    gui_printf (NULL, "%s[%s%d%s] ",
                                GUI_COLOR(COLOR_WIN_CHAT_DARK),
                                GUI_COLOR(COLOR_WIN_CHAT),
                                i,
                                GUI_COLOR(COLOR_WIN_CHAT_DARK));
                    weechat_cmd_ignore_display (NULL, ptr_ignore);
                }
            }
            else
            {
                irc_display_prefix (NULL, NULL, PREFIX_INFO);
                gui_printf (NULL, _("No ignore defined.\n"));
            }
            return 0;
            break;
        case 1:
            ptr_ignore = ignore_add (argv[0], "*", "*",
                                     (SERVER(gui_current_window->buffer)) ?
                                     SERVER(gui_current_window->buffer)->name : "*");
            break;
        case 2:
            ptr_ignore = ignore_add (argv[0], argv[1], "*",
                                     (SERVER(gui_current_window->buffer)) ?
                                     SERVER(gui_current_window->buffer)->name : "*");
            break;
        case 3:
            ptr_ignore = ignore_add (argv[0], argv[1], argv[2],
                                     (SERVER(gui_current_window->buffer)) ?
                                     SERVER(gui_current_window->buffer)->name : "*");
            break;
        case 4:
            ptr_ignore = ignore_add (argv[0], argv[1], argv[2], argv[3]);
            break;
    }
    if (ptr_ignore)
    {
        gui_printf (NULL, "\n");
        weechat_cmd_ignore_display (_("New ignore:"), ptr_ignore);
        return 0;
    }
    else
        return -1;
}

/*
 * weechat_cmd_key_display: display a key binding
 */

void
weechat_cmd_key_display (t_gui_key *key, int new_key)
{
    char *expanded_name;

    expanded_name = gui_key_get_expanded_name (key->key);
    if (new_key)
    {
        irc_display_prefix (NULL, NULL, PREFIX_INFO);
        gui_printf (NULL, _("New key binding:  %s"),
                    (expanded_name) ? expanded_name : key->key);
    }
    else
        gui_printf (NULL, "  %20s", (expanded_name) ? expanded_name : key->key);
    gui_printf (NULL, "%s => %s%s\n",
                GUI_COLOR(COLOR_WIN_CHAT_DARK),
                GUI_COLOR(COLOR_WIN_CHAT),
                (key->function) ?
                gui_key_function_search_by_ptr (key->function) : key->command);
    if (expanded_name)
        free (expanded_name);
}

/*
 * weechat_cmd_key: bind/unbind keys
 */

int
weechat_cmd_key (char *arguments)
{
    char *pos;
    int i;
    t_gui_key *ptr_key;
    
    if (arguments)
    {
        while (arguments[0] == ' ')
            arguments++;
    }

    if (!arguments || (arguments[0] == '\0'))
    {
        gui_printf (NULL, "\n");
        gui_printf (NULL, _("Key bindings:\n"));
        for (ptr_key = gui_keys; ptr_key; ptr_key = ptr_key->next_key)
        {
            weechat_cmd_key_display (ptr_key, 0);
        }
    }
    else if (ascii_strncasecmp (arguments, "unbind ", 7) == 0)
    {
        arguments += 7;
        while (arguments[0] == ' ')
            arguments++;
        if (gui_key_unbind (arguments))
        {
            irc_display_prefix (NULL, NULL, PREFIX_INFO);
            gui_printf (NULL, _("Key \"%s\" unbinded\n"), arguments);
        }
        else
        {
            irc_display_prefix (NULL, NULL, PREFIX_ERROR);
            gui_printf (NULL,
                        _("%s unable to unbind key \"%s\"\n"),
                        WEECHAT_ERROR, arguments);
            return -1;
        }
    }
    else if (ascii_strcasecmp (arguments, "functions") == 0)
    {
        gui_printf (NULL, "\n");
        gui_printf (NULL, _("Internal key functions:\n"));
        i = 0;
        while (gui_key_functions[i].function_name)
        {
            gui_printf (NULL, "%25s  %s\n",
                        gui_key_functions[i].function_name,
                        _(gui_key_functions[i].description));
            i++;
        }
    }
    else if (ascii_strncasecmp (arguments, "reset", 5) == 0)
    {
        arguments += 5;
        while (arguments[0] == ' ')
            arguments++;
        if (ascii_strcasecmp (arguments, "-yes") == 0)
        {
            gui_key_free_all ();
            gui_key_init ();
            irc_display_prefix (NULL, NULL, PREFIX_INFO);
            gui_printf (NULL, _("Default key bindings restored\n"));
        }
        else
        {
            irc_display_prefix (NULL, NULL, PREFIX_ERROR);
            gui_printf (NULL,
                        _("%s \"-yes\" argument is required for keys reset (securuty reason)\n"),
                        WEECHAT_ERROR);
            return -1;
        }
    }
    else
    {
        while (arguments[0] == ' ')
            arguments++;
        pos = strchr (arguments, ' ');
        if (!pos)
        {
            irc_display_prefix (NULL, NULL, PREFIX_ERROR);
            gui_printf (NULL,
                        _("%s wrong argument count for \"%s\" command\n"),
                        WEECHAT_ERROR, "key");
            return -1;
        }
        pos[0] = '\0';
        pos++;
        while (pos[0] == ' ')
            pos++;
        ptr_key = gui_key_bind (arguments, pos);
        if (ptr_key)
            weechat_cmd_key_display (ptr_key, 1);
        else
        {
            irc_display_prefix (NULL, NULL, PREFIX_ERROR);
            gui_printf (NULL,
                        _("%s unable to bind key \"%s\"\n"),
                        WEECHAT_ERROR, arguments);
            return -1;
        }
    }
    
    return 0;
}

/*
 * weechat_cmd_plugin: list/load/unload WeeChat plugins
 */

int
weechat_cmd_plugin (int argc, char **argv)
{
#ifdef PLUGINS
    t_weechat_plugin *ptr_plugin;
    t_plugin_handler *ptr_handler;
    int handler_found;
    
    switch (argc)
    {
        case 0:
            /* list plugins */
            gui_printf (NULL, "\n");
            irc_display_prefix (NULL, NULL, PREFIX_PLUGIN);
            gui_printf (NULL, _("Plugins loaded:\n"));
            for (ptr_plugin = weechat_plugins; ptr_plugin;
                 ptr_plugin = ptr_plugin->next_plugin)
            {
                /* plugin info */
                irc_display_prefix (NULL, NULL, PREFIX_PLUGIN);
                gui_printf (NULL, "  %s%s%s v%s - %s (%s)\n",
                            GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                            ptr_plugin->name,
                            GUI_COLOR(COLOR_WIN_CHAT),
                            ptr_plugin->version,
                            ptr_plugin->description,
                            ptr_plugin->filename);
                
                /* message handlers */
                irc_display_prefix (NULL, NULL, PREFIX_PLUGIN);
                gui_printf (NULL, _("     message handlers:\n"));
                handler_found = 0;
                for (ptr_handler = ptr_plugin->handlers;
                     ptr_handler; ptr_handler = ptr_handler->next_handler)
                {
                    if (ptr_handler->type == HANDLER_MESSAGE)
                    {
                        handler_found = 1;
                        irc_display_prefix (NULL, NULL, PREFIX_PLUGIN);
                        gui_printf (NULL, _("       IRC(%s)\n"),
                                    ptr_handler->irc_command);
                    }
                }
                if (!handler_found)
                {
                    irc_display_prefix (NULL, NULL, PREFIX_PLUGIN);
                    gui_printf (NULL, _("       (no message handler)\n"));
                }
                
                /* command handlers */
                irc_display_prefix (NULL, NULL, PREFIX_PLUGIN);
                gui_printf (NULL, _("     command handlers:\n"));
                handler_found = 0;
                for (ptr_handler = ptr_plugin->handlers;
                     ptr_handler; ptr_handler = ptr_handler->next_handler)
                {
                    if (ptr_handler->type == HANDLER_COMMAND)
                    {
                        handler_found = 1;
                        irc_display_prefix (NULL, NULL, PREFIX_PLUGIN);
                        gui_printf (NULL, "       /%s",
                                    ptr_handler->command);
                        if (ptr_handler->description
                            && ptr_handler->description[0])
                            gui_printf (NULL, " (%s)",
                                        ptr_handler->description);
                        gui_printf (NULL, "\n");
                    }
                }
                if (!handler_found)
                {
                    irc_display_prefix (NULL, NULL, PREFIX_PLUGIN);
                    gui_printf (NULL, _("       (no command handler)\n"));
                }
            }
            if (!weechat_plugins)
            {
                irc_display_prefix (NULL, NULL, PREFIX_PLUGIN);
                gui_printf (NULL, _("  (no plugin)\n"));
            }
            break;
        case 1:
            if (ascii_strcasecmp (argv[0], "autoload") == 0)
                plugin_auto_load ();
            else if (ascii_strcasecmp (argv[0], "reload") == 0)
            {
                plugin_unload_all ();
                plugin_auto_load ();
            }
            else if (ascii_strcasecmp (argv[0], "unload") == 0)
                plugin_unload_all ();
            break;
        case 2:
            if (ascii_strcasecmp (argv[0], "load") == 0)
                plugin_load (argv[1]);
            else if (ascii_strcasecmp (argv[0], "unload") == 0)
                plugin_unload_name (argv[1]);
            else
            {
                irc_display_prefix (NULL, NULL, PREFIX_ERROR);
                gui_printf (NULL,
                            _("%s unknown option for \"%s\" command\n"),
                            WEECHAT_ERROR, "plugin");
            }
            break;
        default:
            irc_display_prefix (NULL, NULL, PREFIX_ERROR);
            gui_printf (NULL,
                        _("%s wrong argument count for \"%s\" command\n"),
                        WEECHAT_ERROR, "plugin");
    }
#else
    irc_display_prefix (NULL, NULL, PREFIX_ERROR);
    gui_printf (NULL,
                _("Command \"plugin\" is not available, WeeChat was built "
                  "without plugins support.\n"));
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
    char *server_name;
    
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
                irc_display_prefix (NULL, NULL, PREFIX_INFO);
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
                irc_display_prefix (NULL, NULL, PREFIX_INFO);
                gui_printf (NULL, _("Server '%s' not found.\n"), argv[0]);
            }
        }
    }
    else
    {
        if (ascii_strcasecmp (argv[0], "del") == 0)
        {
            if (argc < 2)
            {
                irc_display_prefix (NULL, NULL, PREFIX_ERROR);
                gui_printf (NULL,
                            _("%s missing servername for \"%s\" command\n"),
                            WEECHAT_ERROR, "server del");
                return -1;
            }
            if (argc > 2)
            {
                irc_display_prefix (NULL, NULL, PREFIX_ERROR);
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
                irc_display_prefix (NULL, NULL, PREFIX_ERROR);
                gui_printf (NULL,
                            _("%s server \"%s\" not found for \"%s\" command\n"),
                            WEECHAT_ERROR, argv[1], "server del");
                return -1;
            }
            if (server_found->is_connected)
            {
                irc_display_prefix (NULL, NULL, PREFIX_ERROR);
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
            
            server_name = strdup (server_found->name);
            
            server_free (server_found);
            
            irc_display_prefix (NULL, NULL, PREFIX_INFO);
            gui_printf (NULL, _("Server %s%s%s has been deleted\n"),
                        GUI_COLOR(COLOR_WIN_CHAT_SERVER),
                        server_name,
                        GUI_COLOR(COLOR_WIN_CHAT));
            if (server_name)
                free (server_name);
            
            gui_redraw_buffer (gui_current_window->buffer);
            
            return 0;
        }
        
        /* init server struct */
        server_init (&server);
        
        if (argc < 3)
        {
            irc_display_prefix (NULL, NULL, PREFIX_ERROR);
            gui_printf (NULL,
                        _("%s missing parameters for \"%s\" command\n"),
                        WEECHAT_ERROR, "server");
            server_destroy (&server);
            return -1;
        }
        
        if (server_name_already_exists (argv[0]))
        {
            irc_display_prefix (NULL, NULL, PREFIX_ERROR);
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
                if (ascii_strcasecmp (argv[i], "-auto") == 0)
                    server.autoconnect = 1;
                if (ascii_strcasecmp (argv[i], "-noauto") == 0)
                    server.autoconnect = 0;
                if (ascii_strcasecmp (argv[i], "-ipv6") == 0)
                    server.ipv6 = 1;
                if (ascii_strcasecmp (argv[i], "-ssl") == 0)
                    server.ssl = 1;
                if (ascii_strcasecmp (argv[i], "-pwd") == 0)
                {
                    if (i == (argc - 1))
                    {
                        irc_display_prefix (NULL, NULL, PREFIX_ERROR);
                        gui_printf (NULL,
                                    _("%s missing password for \"%s\" parameter\n"),
                                    WEECHAT_ERROR, "-pwd");
                        server_destroy (&server);
                        return -1;
                    }
                    server.password = strdup (argv[++i]);
                }
                if (ascii_strcasecmp (argv[i], "-nicks") == 0)
                {
                    if (i >= (argc - 3))
                    {
                        irc_display_prefix (NULL, NULL, PREFIX_ERROR);
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
                if (ascii_strcasecmp (argv[i], "-username") == 0)
                {
                    if (i == (argc - 1))
                    {
                        irc_display_prefix (NULL, NULL, PREFIX_ERROR);
                        gui_printf (NULL,
                                    _("%s missing password for \"%s\" parameter\n"),
                                    WEECHAT_ERROR, "-username");
                        server_destroy (&server);
                        return -1;
                    }
                    server.username = strdup (argv[++i]);
                }
                if (ascii_strcasecmp (argv[i], "-realname") == 0)
                {
                    if (i == (argc - 1))
                    {
                        irc_display_prefix (NULL, NULL, PREFIX_ERROR);
                        gui_printf (NULL,
                                    _("%s missing password for \"%s\" parameter\n"),
                                    WEECHAT_ERROR, "-realname");
                        server_destroy (&server);
                        return -1;
                    }
                    server.realname = strdup (argv[++i]);
                }
                if (ascii_strcasecmp (argv[i], "-command") == 0)
                {
                    if (i == (argc - 1))
                    {
                        irc_display_prefix (NULL, NULL, PREFIX_ERROR);
                        gui_printf (NULL,
                                    _("%s missing command for \"%s\" parameter\n"),
                                    WEECHAT_ERROR, "-command");
                        server_destroy (&server);
                        return -1;
                    }
                    server.command = strdup (argv[++i]);
                }
                if (ascii_strcasecmp (argv[i], "-autojoin") == 0)
                {
                    if (i == (argc - 1))
                    {
                        irc_display_prefix (NULL, NULL, PREFIX_ERROR);
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
                                 0, server.address, server.port, server.ipv6,
                                 server.ssl, server.password,
                                 server.nick1, server.nick2, server.nick3,
                                 server.username, server.realname,
                                 server.command, 1, server.autojoin, 1, NULL);
        if (new_server)
        {
            irc_display_prefix (NULL, NULL, PREFIX_INFO);
            gui_printf (NULL, _("Server %s%s%s created\n"),
                        GUI_COLOR(COLOR_WIN_CHAT_SERVER),
                        server.name,
                        GUI_COLOR(COLOR_WIN_CHAT));
        }
        else
        {
            irc_display_prefix (NULL, NULL, PREFIX_ERROR);
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
 * weechat_cmd_set_display_option: display config option
 */

void
weechat_cmd_set_display_option (t_config_option *option, char *prefix, void *value)
{
    char *color_name, *pos_nickserv, *pos_pwd, *value2;
    
    gui_printf (NULL, "  %s%s%s%s = ",
                (prefix) ? prefix : "",
                (prefix) ? "." : "",
                option->option_name,
                GUI_COLOR(COLOR_WIN_CHAT_DARK));
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
            gui_printf (NULL, "%s%s\n",
                        GUI_COLOR(COLOR_WIN_CHAT_HOST),
                        (*((int *)value)) ? "ON" : "OFF");
            break;
        case OPTION_TYPE_INT:
            gui_printf (NULL, "%s%d\n",
                        GUI_COLOR(COLOR_WIN_CHAT_HOST),
                        *((int *)value));
            break;
        case OPTION_TYPE_INT_WITH_STRING:
            gui_printf (NULL, "%s%s\n",
                        GUI_COLOR(COLOR_WIN_CHAT_HOST),
                        option->array_values[*((int *)value)]);
            break;
        case OPTION_TYPE_COLOR:
            color_name = gui_get_color_name (*((int *)value));
            gui_printf (NULL, "%s%s\n",
                        GUI_COLOR(COLOR_WIN_CHAT_HOST),
                        (color_name) ? color_name : _("(unknown)"));
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
                    gui_printf (NULL, _("%s(password hidden) "),
                                GUI_COLOR(COLOR_WIN_CHAT));
                }
                gui_printf (NULL, "%s\"%s%s%s\"",
                            GUI_COLOR(COLOR_WIN_CHAT_DARK),
                            GUI_COLOR(COLOR_WIN_CHAT_HOST),
                            value2,
                            GUI_COLOR(COLOR_WIN_CHAT_DARK));
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
    int last_section, last_option, number_found;

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
            
            /* remove simple or double quotes 
               and spaces at the end */
            if (strlen(value) > 1)
            {
                pos = value + strlen (value) - 1;
                while ((pos > value) && (pos[0] == ' '))
                {
                    pos[0] = '\0';
                    pos--;
                }
                pos = value + strlen (value) - 1;
                if (((value[0] == '\'') &&
                     (pos[0] == '\'')) ||
                    ((value[0] == '"') &&
                     (pos[0] == '"')))
                {
                    pos[0] = '\0';
                    value++;
                }
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
                irc_display_prefix (NULL, NULL, PREFIX_ERROR);
                gui_printf (NULL,
                            _("%s server \"%s\" not found\n"),
                            WEECHAT_ERROR, option);
            }
            else
            {
                switch (config_set_server_value (ptr_server, pos + 1, value))
                {
                    case 0:
                        gui_printf (NULL, "\n");
                        gui_printf (NULL, "%s[%s%s %s%s%s]\n",
                                    GUI_COLOR(COLOR_WIN_CHAT_DARK),
                                    GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                                    config_sections[CONFIG_SECTION_SERVER].section_name,
                                    GUI_COLOR(COLOR_WIN_CHAT_SERVER),
                                    ptr_server->name,
                                    GUI_COLOR(COLOR_WIN_CHAT_DARK));
                        for (i = 0; weechat_options[CONFIG_SECTION_SERVER][i].option_name; i++)
                        {
                            if (strcmp (weechat_options[CONFIG_SECTION_SERVER][i].option_name, pos + 1) == 0)
                                break;
                        }
                        if (weechat_options[CONFIG_SECTION_SERVER][i].option_name)
                        {
                            ptr_option_value = config_get_server_option_ptr (ptr_server,
                                weechat_options[CONFIG_SECTION_SERVER][i].option_name);
                            weechat_cmd_set_display_option (&weechat_options[CONFIG_SECTION_SERVER][i],
                                                            ptr_server->name,
                                                            ptr_option_value);
                        }
                        config_change_buffer_content ();
                        break;
                    case -1:
                        irc_display_prefix (NULL, NULL, PREFIX_ERROR);
                        gui_printf (NULL, _("%s config option \"%s\" not found\n"),
                                    WEECHAT_ERROR, pos + 1);
                        break;
                    case -2:
                        irc_display_prefix (NULL, NULL, PREFIX_ERROR);
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
                    irc_display_prefix (NULL, NULL, PREFIX_ERROR);
                    gui_printf (NULL,
                                _("%s option \"%s\" can not be changed while WeeChat is running\n"),
                                WEECHAT_ERROR, option);
                }
                else
                {
                    if (config_option_set_value (ptr_option, value) == 0)
                    {
                        (void) (ptr_option->handler_change());
                        gui_printf (NULL, "\n");
                        gui_printf (NULL, "%s[%s%s%s]\n",
                                    GUI_COLOR(COLOR_WIN_CHAT_DARK),
                                    GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                                    config_get_section (ptr_option),
                                    GUI_COLOR(COLOR_WIN_CHAT_DARK));
                        weechat_cmd_set_display_option (ptr_option, NULL, NULL);
                    }
                    else
                    {
                        irc_display_prefix (NULL, NULL, PREFIX_ERROR);
                        gui_printf (NULL, _("%s incorrect value for option \"%s\"\n"),
                                    WEECHAT_ERROR, option);
                    }
                }
            }
            else
            {
                irc_display_prefix (NULL, NULL, PREFIX_ERROR);
                gui_printf (NULL, _("%s config option \"%s\" not found\n"),
                            WEECHAT_ERROR, option);
            }
        }
    }
    else
    {
        last_section = -1;
        last_option = -1;
        number_found = 0;
        for (i = 0; i < CONFIG_NUMBER_SECTIONS; i++)
        {
            section_displayed = 0;
            if ((i != CONFIG_SECTION_KEYS) && (i != CONFIG_SECTION_ALIAS)
                && (i != CONFIG_SECTION_IGNORE) && (i != CONFIG_SECTION_SERVER))
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
                            gui_printf (NULL, "\n");
                            gui_printf (NULL, "%s[%s%s%s]\n",
                                        GUI_COLOR(COLOR_WIN_CHAT_DARK),
                                        GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                                        config_sections[i].section_name,
                                        GUI_COLOR(COLOR_WIN_CHAT_DARK));
                            section_displayed = 1;
                        }
                        weechat_cmd_set_display_option (&weechat_options[i][j], NULL, NULL);
                        last_section = i;
                        last_option = j;
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
                        gui_printf (NULL, "\n");
                        gui_printf (NULL, "%s[%s%s %s%s%s]\n",
                                    GUI_COLOR(COLOR_WIN_CHAT_DARK),
                                    GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                                    config_sections[CONFIG_SECTION_SERVER].section_name,
                                    GUI_COLOR(COLOR_WIN_CHAT_SERVER),
                                    ptr_server->name,
                                    GUI_COLOR(COLOR_WIN_CHAT_DARK));
                        section_displayed = 1;
                    }
                    ptr_option_value = config_get_server_option_ptr (ptr_server,
                        weechat_options[CONFIG_SECTION_SERVER][i].option_name);
                    if (ptr_option_value)
                    {
                        weechat_cmd_set_display_option (&weechat_options[CONFIG_SECTION_SERVER][i],
                                                        ptr_server->name,
                                                        ptr_option_value);
                        last_section = CONFIG_SECTION_SERVER;
                        last_option = i;
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
            if ((number_found == 1) && (last_section >= 0) && (last_option >= 0))
            {
                gui_printf (NULL, "\n");
                gui_printf (NULL, _("%sDetail:\n"),
                            GUI_COLOR(COLOR_WIN_CHAT_CHANNEL));
                switch (weechat_options[last_section][last_option].option_type)
                {
                    case OPTION_TYPE_BOOLEAN:
                        gui_printf (NULL, _("  . type boolean (values: 'on' or 'off')\n"));
                        gui_printf (NULL, _("  . default value: '%s'\n"),
                                    (weechat_options[last_section][last_option].default_int == BOOL_TRUE) ?
                                    "on" : "off");
                        break;
                    case OPTION_TYPE_INT:
                        gui_printf (NULL, _("  . type integer (values: between %d and %d)\n"),
                                    weechat_options[last_section][last_option].min,
                                    weechat_options[last_section][last_option].max);
                        gui_printf (NULL, _("  . default value: %d\n"),
                                    weechat_options[last_section][last_option].default_int);
                        break;
                    case OPTION_TYPE_INT_WITH_STRING:
                        gui_printf (NULL, _("  . type string (values: "));
                        i = 0;
                        while (weechat_options[last_section][last_option].array_values[i])
                        {
                            gui_printf (NULL, "'%s'",
                                        weechat_options[last_section][last_option].array_values[i]);
                            if (weechat_options[last_section][last_option].array_values[i + 1])
                                gui_printf (NULL, ", ");
                            i++;
                        }
                        gui_printf (NULL, ")\n");
                        gui_printf (NULL, _("  . default value: '%s'\n"),
                            (weechat_options[last_section][last_option].default_string) ?
                            weechat_options[last_section][last_option].default_string : _("empty"));
                        break;
                    case OPTION_TYPE_COLOR:
                        gui_printf (NULL, _("  . type color (Curses or Gtk color, look at WeeChat doc)\n"));
                        gui_printf (NULL, _("  . default value: '%s'\n"),
                            (weechat_options[last_section][last_option].default_string) ?
                            weechat_options[last_section][last_option].default_string : _("empty"));
                        break;
                    case OPTION_TYPE_STRING:
                        gui_printf (NULL, _("  . type string (any string)\n"));
                        gui_printf (NULL, _("  . default value: '%s'\n"),
                                    (weechat_options[last_section][last_option].default_string) ?
                                    weechat_options[last_section][last_option].default_string : _("empty"));
                        break;
                }
                gui_printf (NULL, _("  . description: %s\n"),
                            _(weechat_options[last_section][last_option].long_description));
            }
            else
            {
                gui_printf (NULL, "\n");
                gui_printf (NULL, "%s%d %s",
                            GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                            number_found,
                            GUI_COLOR(COLOR_WIN_CHAT));
                if (option)
                    gui_printf (NULL, _("config option(s) found with \"%s\"\n"),
                                option);
                else
                    gui_printf (NULL, _("config option(s) found\n"));
            }
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
        irc_display_prefix (NULL, NULL, PREFIX_ERROR);
        gui_printf (NULL, _("%s alias or command \"%s\" not found\n"),
                    WEECHAT_ERROR, arguments);
        return -1;
    }
    
    weelist_remove (&index_commands, &last_index_command, ptr_weelist);
    ptr_alias = alias_search (arguments);
    if (ptr_alias)
        alias_free (ptr_alias);
    irc_display_prefix (NULL, NULL, PREFIX_INFO);
    gui_printf (NULL, _("Alias \"%s\" removed\n"),
                arguments);
    return 0;
}

/*
 * weechat_cmd_unignore: unignore IRC commands and/or hosts
 */

int
weechat_cmd_unignore (int argc, char **argv)
{
    char *error;
    int number, ret;
    
    ret = 0;
    switch (argc)
    {
        case 0:
            /* List all ignore */
            weechat_cmd_ignore (argc, argv);
            return 0;
            break;
        case 1:
            error = NULL;
            number = strtol (argv[0], &error, 10);
            if ((error) && (error[0] == '\0'))
                ret = ignore_search_free_by_number (number);
            else
                ret = ignore_search_free (argv[0], "*", "*",
                                    (SERVER(gui_current_window->buffer)) ?
                                     SERVER(gui_current_window->buffer)->name : "*");
            break;
        case 2:
            ret = ignore_search_free (argv[0], argv[1], "*",
                                      (SERVER(gui_current_window->buffer)) ?
                                      SERVER(gui_current_window->buffer)->name : "*");
            break;
        case 3:
            ret = ignore_search_free (argv[0], argv[1], argv[2],
                                      (SERVER(gui_current_window->buffer)) ?
                                      SERVER(gui_current_window->buffer)->name : "*");
            break;
        case 4:
            ret = ignore_search_free (argv[0], argv[1], argv[2], argv[3]);
            break;
    }
    
    if (ret)
    {
        irc_display_prefix (NULL, NULL, PREFIX_INFO);
        gui_printf (NULL, "%s%d%s ",
                    GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                    ret,
                    GUI_COLOR(COLOR_WIN_CHAT));
        if (ret > 1)
            gui_printf (NULL, _("ignore were removed.\n"));
        else
            gui_printf (NULL, _("ignore was removed.\n"));
    }
    else
    {
        irc_display_prefix (NULL, NULL, PREFIX_ERROR);
        gui_printf (NULL, _("%s no ignore found\n"),
                    WEECHAT_ERROR);
        return -1;
    }
    
    return 0;
}

/*
 * weechat_cmd_uptime: display WeeChat uptime
 */

int
weechat_cmd_uptime (int argc, char **argv)
{
    time_t running_time;
    int day, hour, min, sec;
    char string[256];
    
    running_time = time (NULL) - weechat_start_time;
    day = running_time / (60 * 60 * 24);
    hour = (running_time % (60 * 60 * 24)) / (60 * 60);
    min = ((running_time % (60 * 60 * 24)) % (60 * 60)) / 60;
    sec = ((running_time % (60 * 60 * 24)) % (60 * 60)) % 60;
    
    if ((argc == 1) && (strcmp (argv[0], "-o") == 0)
        && ((BUFFER_IS_CHANNEL(gui_current_window->buffer))
            || (BUFFER_IS_PRIVATE(gui_current_window->buffer))))
    {
        snprintf (string, sizeof (string),
                  _("WeeChat uptime: %d %s %02d:%02d:%02d, started on %s"),
                  day,
                  (day > 1) ? _("days") : _("day"),
                  hour,
                  min,
                  sec,
                  ctime (&weechat_start_time));
        string[strlen (string) - 1] = '\0';
        user_command (SERVER(gui_current_window->buffer),
                      gui_current_window->buffer,
                      string);
    }
    else
    {
        irc_display_prefix (NULL, gui_current_window->buffer,
                            PREFIX_INFO);
        gui_printf_nolog (gui_current_window->buffer,
                          _("WeeChat uptime: %s%d %s%s "
                            "%s%02d%s:%s%02d%s:%s%02d%s, "
                            "started on %s%s"),
                          GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                          day,
                          GUI_COLOR(COLOR_WIN_CHAT),
                          (day > 1) ? _("days") : _("day"),
                          GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                          hour,
                          GUI_COLOR(COLOR_WIN_CHAT),
                          GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                          min,
                          GUI_COLOR(COLOR_WIN_CHAT),
                          GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                          sec,
                          GUI_COLOR(COLOR_WIN_CHAT),
                          GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                          ctime (&weechat_start_time));
    }
    
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
    char *error;
    long number;
    
    if ((argc == 0) || ((argc == 1) && (ascii_strcasecmp (argv[0], "list") == 0)))
    {
        /* list opened windows */
        
        gui_printf (NULL, "\n");
        gui_printf (NULL, _("Opened windows:\n"));
        
        i = 1;
        for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
        {
            gui_printf (NULL, "%s[%s%d%s] (%s%d:%d%s;%s%dx%d%s) ",
                        GUI_COLOR(COLOR_WIN_CHAT_DARK),
                        GUI_COLOR(COLOR_WIN_CHAT),
                        i,
                        GUI_COLOR(COLOR_WIN_CHAT_DARK),
                        GUI_COLOR(COLOR_WIN_CHAT),
                        ptr_win->win_x,
                        ptr_win->win_y,
                        GUI_COLOR(COLOR_WIN_CHAT_DARK),
                        GUI_COLOR(COLOR_WIN_CHAT),
                        ptr_win->win_width,
                        ptr_win->win_height,
                        GUI_COLOR(COLOR_WIN_CHAT_DARK));
            
            weechat_cmd_buffer_display_info (ptr_win->buffer);
            
            i++;
        }
    }
    else
    {
        if (ascii_strcasecmp (argv[0], "splith") == 0)
        {
            /* split window horizontally */
            if (argc > 1)
            {
                error = NULL;
                number = strtol (argv[1], &error, 10);
                if ((error) && (error[0] == '\0')
                    && (number > 0) && (number < 100))
                    gui_window_split_horiz (gui_current_window, number);
            }
            else
                gui_window_split_horiz (gui_current_window, 50);
        }
        else if (ascii_strcasecmp (argv[0], "splitv") == 0)
        {
            /* split window vertically */
            if (argc > 1)
            {
                error = NULL;
                number = strtol (argv[1], &error, 10);
                if ((error) && (error[0] == '\0')
                    && (number > 0) && (number < 100))
                    gui_window_split_vertic (gui_current_window, number);
            }
            else
                gui_window_split_vertic (gui_current_window, 50);
        }
        else if (ascii_strcasecmp (argv[0], "resize") == 0)
        {
            /* resize window */
            if (argc > 1)
            {
                error = NULL;
                number = strtol (argv[1], &error, 10);
                if ((error) && (error[0] == '\0')
                    && (number > 0) && (number < 100))
                    gui_window_resize (gui_current_window, number);
            }
        }
        else if (ascii_strcasecmp (argv[0], "merge") == 0)
        {
            if (argc >= 2)
            {
                if (ascii_strcasecmp (argv[1], "all") == 0)
                    gui_window_merge_all (gui_current_window);
                else
                {
                    irc_display_prefix (NULL, NULL, PREFIX_ERROR);
                    gui_printf (NULL,
                                _("%s unknown option for \"%s\" command\n"),
                                WEECHAT_ERROR, "window merge");
                    return -1;
                }
            }
            else
            {
                if (!gui_window_merge (gui_current_window))
                {
                    irc_display_prefix (NULL, NULL, PREFIX_ERROR);
                    gui_printf (NULL,
                                _("%s can not merge windows, "
                                  "there's no other window with same size "
                                  "near current one.\n"),
                                WEECHAT_ERROR);
                    return -1;
                }
            }
        }
        else if (ascii_strncasecmp (argv[0], "b", 1) == 0)
        {
            /* jump to window by buffer number */
            error = NULL;
            number = strtol (argv[0] + 1, &error, 10);
            if ((error) && (error[0] == '\0'))
                gui_window_switch_by_buffer (gui_current_window, number);
        }
        else if (ascii_strcasecmp (argv[0], "-1") == 0)
            gui_window_switch_previous (gui_current_window);
        else if (ascii_strcasecmp (argv[0], "+1") == 0)
            gui_window_switch_next (gui_current_window);
        else if (ascii_strcasecmp (argv[0], "up") == 0)
            gui_window_switch_up (gui_current_window);
        else if (ascii_strcasecmp (argv[0], "down") == 0)
            gui_window_switch_down (gui_current_window);
        else if (ascii_strcasecmp (argv[0], "left") == 0)
            gui_window_switch_left (gui_current_window);
        else if (ascii_strcasecmp (argv[0], "right") == 0)
            gui_window_switch_right (gui_current_window);
        else
        {
            irc_display_prefix (NULL, NULL, PREFIX_ERROR);
            gui_printf (NULL,
                        _("%s unknown option for \"%s\" command\n"),
                        WEECHAT_ERROR, "window");
            return -1;
        }
    }
    return 0;
}
