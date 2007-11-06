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

/* wee-command.c: WeeChat commands */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "weechat.h"
#include "wee-command.h"
#include "wee-alias.h"
#include "wee-config.h"
#include "wee-hook.h"
#include "wee-input.h"
#include "wee-log.h"
#include "wee-upgrade.h"
#include "wee-string.h"
#include "wee-utf8.h"
#include "wee-list.h"
#include "../gui/gui-chat.h"
#include "../gui/gui-history.h"
#include "../gui/gui-keyboard.h"
#include "../gui/gui-window.h"
#include "../plugins/plugin.h"
#include "../plugins/plugin-config.h"


/* WeeChat internal commands */

struct command weechat_commands[] =
{ { "alias",
    N_("create an alias for a command"),
    N_("[alias_name [command [arguments]]]"),
    N_("alias_name: name of alias\n"
       "   command: command name (WeeChat or IRC command, many commands "
       "can be separated by semicolons)\n"
       " arguments: arguments for command\n\n"
        "Note: in command, special variables $1, $2,..,$9 are replaced by "
       "arguments given by user, and $* is replaced by all arguments.\n"
       "Variables $nick, $channel and $server are replaced by current "
       "nick/channel/server."),
    "%- %A", 0, MAX_ARGS, 1, command_alias },
  { "buffer",
    N_("manage buffers"),
    N_("[action [args] | number | [[server] [channel]]]"),
    N_(" action: action to do:\n"
       "   move: move buffer in the list (may be relative, for example -1)\n"
       "  close: close buffer\n"
       "   list: list open buffers (no parameter implies this list)\n"
       " notify: set notify level for buffer (0=never, 1=highlight, 2=1+msg, "
       "3=2+join/part)\n"
       "         (when executed on server buffer, this sets default notify "
       "level for whole server)\n"
       " scroll: scroll in history (may be relative, and may end by a letter: "
       "s=sec, m=min, h=hour, d=day, M=month, y=year); if there is "
       "only letter, then scroll to beginning of this item\n\n"
       " number: jump to buffer by number\n"
       "server,\n"
       "channel: jump to buffer by server and/or channel name\n\n"
       "Examples:\n"
       "        move buffer: /buffer move 5\n"
       "       close buffer: /buffer close this is part msg\n"
       "         set notify: /buffer notify 2\n"
       "    scroll 1 day up: /buffer scroll 1d  ==  /buffer scroll -1d  ==  "
       "/buffer scroll -24h\n"
       "scroll to beginning\n"
       "        of this day: /buffer scroll d\n"
       " scroll 15 min down: /buffer scroll +15m\n"
       "  scroll 20 msgs up: /buffer scroll -20\n"
       "   jump to #weechat: /buffer #weechat"),
    "move|close|list|notify|scroll|set|%S|%C %S|%C",
    0, MAX_ARGS, 0, command_buffer },
  { "builtin",
    N_("launch WeeChat builtin command (do not look at commands hooked or "
       "aliases)"),
    N_("command"),
    N_("command: command to execute (a '/' is automatically added if not "
       "found at beginning of command)\n"),
    "%w|%i", 0, MAX_ARGS, 1, command_builtin },
  { "clear",
    N_("clear window(s)"),
    N_("[-all | number [number ...]]"),
    N_("  -all: clear all buffers\n"
       "number: clear buffer by number"),
    "-all", 0, MAX_ARGS, 0, command_clear },
  { "debug",
    N_("print debug messages"),
    N_("dump | buffer | windows"),
    N_("   dump: save memory dump in WeeChat log file (same dump is written "
       "when WeeChat crashes)\n"
       " buffer: dump buffer content with hexadecimal values in log file\n"
       "windows: display windows tree"),
    "dump|buffer|windows", 1, 1, 0, command_debug },
  { "help",
    N_("display help about commands"),
    N_("[command]"),
    N_("command: name of a WeeChat or IRC command"),
    "%w|%i|%h", 0, 1, 0, command_help },
  { "history",
    N_("show buffer command history"),
    N_("[clear | value]"),
    N_("clear: clear history\n"
       "value: number of history entries to show"),
    "clear", 0, 1, 0, command_history },
  { "key",
    N_("bind/unbind keys"),
    N_("[key [function/command]] [unbind key] [functions] [call function "
       "[\"args\"]] [reset -yes]"),
    N_("      key: display or bind this key to an internal function or a "
       "command (beginning by \"/\")\n"
       "   unbind: unbind a key\n"
       "functions: list internal functions for key bindings\n"
       "     call: call a function by name (with optional arguments)\n"
       "    reset: restore bindings to the default values and delete ALL "
       "personal bindings (use carefully!)"),
    "unbind|functions|call|reset %k", 0, MAX_ARGS, 0, command_key },
  { "plugin",
    N_("list/load/unload plugins"),
    N_("[list [name]] | [listfull [name]] | [load filename] | [autoload] | "
       "[reload [name]] | [unload [name]]"),
    N_("    list: list loaded plugins\n"
       "listfull: list loaded plugins with detailed info for each plugin\n"
       "    load: load a plugin\n"
       "autoload: autoload plugins in system or user directory\n"
       "  reload: reload one plugin (if no name given, unload all plugins, "
       "then autoload plugins)\n"
       "  unload: unload one or all plugins\n\n"
       "Without argument, /plugin command lists loaded plugins."),
    "list|listfull|load|autoload|reload|unload %P", 0, 2, 0, command_plugin },
  { "quit",
    "", "", "",
    NULL, 0, 0, 0, command_quit },
  { "save",
    N_("save configuration files to disk"),
    "", "",
    NULL, 0, 1, 0, command_save },
  { "set", N_("set config options"),
    N_("[option [ = value]]"),
    N_("option: name of an option (if name is full "
       "and no value is given, then help is displayed on option)\n"
       " value: value for option\n\n"
       "Option may be: servername.server_xxx where \"servername\" is an "
       "internal server name and \"xxx\" an option for this server."),
    "%o = %v", 0, MAX_ARGS, 0, command_set },
  { "setp",
    N_("set plugin config options"),
    N_("[option [ = value]]"),
    N_("option: name of a plugin option\n"
       " value: value for option\n\n"
       "Option is format: plugin.option, example: perl.myscript.item1"),
    "%O = %V", 0, MAX_ARGS, 0, command_setp },
  { "unalias",
    N_("remove an alias"),
    N_("alias_name"), N_("alias_name: name of alias to remove"),
    "%a", 1, 1, 0, command_unalias },
  { "upgrade",
    N_("upgrade WeeChat without disconnecting from servers"),
    N_("[path_to_binary]"),
    N_("path_to_binary: path to WeeChat binary (default is current binary)\n\n"
       "This command run again a WeeChat binary, so it should have been "
       "compiled or installed with a package manager before running this "
       "command."),
    "%f", 0, 1, 0, command_upgrade },
  { "uptime",
    N_("show WeeChat uptime"),
    N_("[-o]"),
    N_("-o: send uptime on current channel as an IRC message"),
    "-o", 0, 1, 0, command_uptime },
  { "window",
    N_("manage windows"),
    N_("[list | -1 | +1 | b# | up | down | left | right | splith [pct] "
       "| splitv [pct] | resize pct | merge [all]]"),
    N_("  list: list open windows (no parameter implies this list)\n"
       "    -1: jump to previous window\n"
       "    +1: jump to next window\n"
       "    b#: jump to next window displaying buffer number #\n"
       "    up: switch to window above current one\n"
       "  down: switch to window below current one\n"
       "  left: switch to window on the left\n"
       " right: switch to window on the right\n"
       "splith: split current window horizontally\n"
       "splitv: split current window vertically\n"
       "resize: resize window size, new size is <pct> pourcentage of parent "
       "window\n"
       " merge: merge window with another (all = keep only one window)\n\n"
       "For splith and splitv, pct is a pourcentage which represents "
       "size of new window, computed with current window as size reference. "
       "For example 25 means create a new window with size = current_size / 4"),
    "list|-1|+1|up|down|left|right|splith|splitv|resize|merge all",
    0, 2, 0, command_window },
  { NULL, NULL, NULL, NULL, NULL, 0, 0, 0, NULL }
};

struct t_weelist *weechat_index_commands;
struct t_weelist *weechat_last_index_command;


/*
 * command_is_used: return 1 if command is used by weechat
 *                  (WeeChat/alias command)
 */

int
command_is_used (char *command)
{
    struct alias *ptr_alias;
    int i;
    
    /* look for alias */
    for (ptr_alias = weechat_alias; ptr_alias;
         ptr_alias = ptr_alias->next_alias)
    {
        if (string_strcasecmp (ptr_alias->name, command) == 0)
            return 1;
    }
    
    /* look for WeeChat command */
    for (i = 0; weechat_commands[i].name; i++)
    {
        if (string_strcasecmp (weechat_commands[i].name, command) == 0)
            return 1;
    }
    
    /* no command/alias found */
    return 0;
}

/*
 * command_index_build: build an index of commands (internal, irc and alias)
 *                      This list will be sorted, and used for completion
 */

void
command_index_build ()
{
    int i;
    
    weechat_index_commands = NULL;
    weechat_last_index_command = NULL;
    i = 0;
    while (weechat_commands[i].name)
    {
        (void) weelist_add (&weechat_index_commands,
                            &weechat_last_index_command,
                            weechat_commands[i].name,
                            WEELIST_POS_SORT);
        i++;
    }
}

/*
 * command_index_free: remove all commands in index
 */

void
command_index_free ()
{
    while (weechat_index_commands)
    {
        weelist_remove (&weechat_index_commands,
                        &weechat_last_index_command,
                        weechat_index_commands);
    }
}

/*
 * command_index_add: add command to commands index
 */

void
command_index_add (char *command)
{
    if (!weelist_search (weechat_index_commands, command))
        weelist_add (&weechat_index_commands, &weechat_last_index_command,
                     command, WEELIST_POS_SORT);
}

/*
 * command_index_remove: remove command from commands index
 */

void
command_index_remove (char *command)
{
    if (!command_is_used (command))
        weelist_remove (&weechat_index_commands, &weechat_last_index_command,
                        weelist_search (weechat_index_commands, command));
}

/*
 * command_is_command: return 1 if line is a command, 0 otherwise
 */

int
command_is_command (char *line)
{
    char *pos_slash, *pos_space;

    if (strncmp (line, "/*", 2) == 0)
        return 0;
    
    pos_slash = strchr (line + 1, '/');
    pos_space = strchr (line + 1, ' ');
    
    return (line[0] == '/')
        && (!pos_slash || (pos_space && pos_slash > pos_space));
}

/*
 * command_print_stdout: print list of commands on standard output
 */

void
command_print_stdout (struct command *commands)
{
    int i;
    
    for (i = 0; commands[i].name; i++)
    {
        string_iconv_fprintf (stdout, "* %s", commands[i].name);
        if (commands[i].arguments &&
            commands[i].arguments[0])
            string_iconv_fprintf (stdout, "  %s\n\n", _(commands[i].arguments));
        else
            string_iconv_fprintf (stdout, "\n\n");
        string_iconv_fprintf (stdout, "%s\n\n", _(commands[i].description));
        if (commands[i].arguments_description &&
            commands[i].arguments_description[0])
            string_iconv_fprintf (stdout, "%s\n\n",
                                  _(commands[i].arguments_description));
    }
}

/*
 * command_alias: display or create alias
 */

int
command_alias (struct t_gui_buffer *buffer,
               int argc, char **argv, char **argv_eol)
{
    char *alias_name;
    struct alias *ptr_alias;
    
    /* make C compiler happy */
    (void) buffer;
    
    if (argc > 0)
    {
        alias_name = (argv[0][0] == '/') ? argv[0] + 1 : argv[0];
        if (argc > 1)
        {
            /* Define new alias */
            if (!alias_new (alias_name, argv_eol[1]))
                return -1;
            
            if (weelist_add (&weechat_index_commands,
                             &weechat_last_index_command,
                             alias_name,
                             WEELIST_POS_SORT))
            {
                gui_chat_printf (NULL,
                                 _("%sAlias \"%s\" => \"%s\" created"),
                                 gui_chat_prefix[GUI_CHAT_PREFIX_INFO],
                                 alias_name, argv_eol[1]);
            }
            else
            {
                gui_chat_printf (NULL,
                                 _("%sError: not enough memory for creating "
                                   "alias \"%s\" => \"%s\""),
                                 gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                 alias_name, argv_eol[1]);
                return -1;
            }
        }
        else
        {
            /* Display one alias */
            ptr_alias = alias_search (alias_name);
	    if (ptr_alias)
	    {
		gui_chat_printf (NULL, "");
		gui_chat_printf (NULL, _("Alias:"));
		gui_chat_printf (NULL, "  %s %s=>%s %s",
                                 ptr_alias->name,
                                 GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                 GUI_COLOR(GUI_COLOR_CHAT),
                                 ptr_alias->command);
	    }
            else
                gui_chat_printf (NULL,
                                 _("%sNo alias found."),
                                 gui_chat_prefix[GUI_CHAT_PREFIX_INFO]);
        }
    }
    else
    {
        /* List all aliases */
        if (weechat_alias)
        {
            gui_chat_printf (NULL, "");
            gui_chat_printf (NULL, _("List of aliases:"));
            for (ptr_alias = weechat_alias; ptr_alias;
                 ptr_alias = ptr_alias->next_alias)
            {
                gui_chat_printf (NULL,
                                 "  %s %s=>%s %s",
                                 ptr_alias->name,
                                 GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                 GUI_COLOR(GUI_COLOR_CHAT),
                                 ptr_alias->command);
            }
        }
        else
            gui_chat_printf (NULL,
                             _("%sNo alias defined."),
                             gui_chat_prefix[GUI_CHAT_PREFIX_INFO]);
    }
    return 0;
}

/*
 * command_buffer: manage buffers
 */

int
command_buffer (struct t_gui_buffer *buffer,
                int argc, char **argv, char **argv_eol)
{
    struct t_gui_buffer *ptr_buffer;
    long number;
    char *error ,*value;
    int target_buffer;
    
    if ((argc == 0)
        || ((argc == 1) && (string_strcasecmp (argv[0], "list") == 0)))
    {
        /* list open buffers */
        
        gui_chat_printf (NULL, "");
        gui_chat_printf (NULL, _("Open buffers:"));
        
        for (ptr_buffer = gui_buffers; ptr_buffer;
             ptr_buffer = ptr_buffer->next_buffer)
        {
            gui_chat_printf (NULL,
                             "%s[%s%d%s]%s (%s) %s / %s",
                             GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                             GUI_COLOR(GUI_COLOR_CHAT),
                             ptr_buffer->number,
                             GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                             GUI_COLOR(GUI_COLOR_CHAT),
                             (ptr_buffer->plugin) ?
                             ((struct t_weechat_plugin *)ptr_buffer->plugin)->name :
                             "weechat",
                             ptr_buffer->category,
                             ptr_buffer->name);
        }
    }
    else
    {
        if (string_strcasecmp (argv[0], "move") == 0)
        {
            /* move buffer to another number in the list */
            
            if (argc < 2)
            {
                gui_chat_printf (NULL,
                                 _("%sError: missing arguments for \"%s\" "
                                   "command"),
                                 gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                 "buffer");
                return -1;
            }
            
            error = NULL;
            number = strtol (((argv[1][0] == '+') || (argv[1][0] == '-')) ?
                             argv[1] + 1 : argv[1],
                             &error, 10);
            if (error && (error[0] == '\0'))
            {
                if (argv[1][0] == '+')
                    gui_buffer_move_to_number (buffer,
                                               buffer->number + ((int) number));
                else if (argv[1][0] == '-')
                    gui_buffer_move_to_number (buffer,
                                               buffer->number - ((int) number));
                else
                    gui_buffer_move_to_number (buffer, (int) number);
            }
            else
            {
                /* invalid number */
                gui_chat_printf (NULL,
                                 _("%sError: incorrect buffer number"),
                                 gui_chat_prefix[GUI_CHAT_PREFIX_ERROR]);
                return -1;
            }
        }
        else if (string_strcasecmp (argv[0], "notify") == 0)
        {
            if (argc < 2)
            {
                /* display notify level for all buffers */
                gui_chat_printf (NULL, "");
                gui_chat_printf (NULL, _("Notify levels:"));
                for (ptr_buffer = gui_buffers; ptr_buffer;
                     ptr_buffer = ptr_buffer->next_buffer)
                {
                    gui_chat_printf (NULL,
                                     "  %d.%s: %d",
                                     ptr_buffer->number,
                                     ptr_buffer->name,
                                     ptr_buffer->notify_level); 
                }
                gui_chat_printf (NULL, "");
            }
            else
            {
                /* set notify level for buffer */
                error = NULL;
                number = strtol (argv[1], &error, 10);
                if (error && (error[0] == '\0'))
                {
                    if ((number < GUI_BUFFER_NOTIFY_LEVEL_MIN)
                        || (number > GUI_BUFFER_NOTIFY_LEVEL_MAX))
                    {
                        /* invalid highlight level */
                        gui_chat_printf (NULL,
                                         _("%sError: incorrect notify level "
                                           "(must be between %d and %d)"),
                                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                         GUI_BUFFER_NOTIFY_LEVEL_MIN,
                                         GUI_BUFFER_NOTIFY_LEVEL_MAX);
                        return -1;
                    }
                    gui_chat_printf (NULL,
                                     _("%sNew notify level for %s%s%s: "
                                       "%d %s"),
                                     gui_chat_prefix[GUI_CHAT_PREFIX_INFO],
                                     GUI_COLOR(GUI_COLOR_CHAT_BUFFER),
                                     buffer->name,
                                     GUI_COLOR(GUI_COLOR_CHAT),
                                     number,
                                     GUI_COLOR(GUI_COLOR_CHAT));
                    switch (number)
                    {
                        case 0:
                            gui_chat_printf (NULL,
                                             _("(hotlist: never)"));
                            break;
                        case 1:
                            gui_chat_printf (NULL,
                                             _("(hotlist: highlights)"));
                            break;
                        case 2:
                            gui_chat_printf (NULL,
                                             _("(hotlist: highlights + "
                                               "messages)"));
                            break;
                        case 3:
                            gui_chat_printf (NULL,
                                             _("(hotlist: highlights + "
                                               "messages + join/part "
                                               "(all))"));
                            break;
                        default:
                            gui_chat_printf (NULL, "");
                            break;
                    }
                }
                else
                {
                    /* invalid number */
                    gui_chat_printf (NULL,
                                     _("%sError: incorrect notify level (must "
                                       "be between %d and %d)"),
                                     gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                     GUI_BUFFER_NOTIFY_LEVEL_MIN,
                                     GUI_BUFFER_NOTIFY_LEVEL_MAX);
                    return -1;
                }
            }
        }
        else if (string_strcasecmp (argv[0], "set") == 0)
        {
            /* set a property on buffer */
            
            if (argc < 3)
            {
                gui_chat_printf (NULL,
                                 _("%sError: missing arguments for \"%s\" "
                                   "command"),
                                 gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                 "buffer");
                return -1;
            }
            value = string_remove_quotes (argv_eol[2], "'\"");
            gui_buffer_set (buffer, argv[1], (value) ? value : argv_eol[2]);
            if (value)
                free (value);
        }
        else
        {
            /* jump to buffer by number or server/channel name */
            
            if (argv[0][0] == '-')
            {
                /* relative jump '-' */
                error = NULL;
                number = strtol (argv[0] + 1, &error, 10);
                if (error && (error[0] == '\0'))
                {
                    target_buffer = buffer->number - (int) number;
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
                if (error && (error[0] == '\0'))
                {
                    target_buffer = buffer->number + (int) number;
                    if (last_gui_buffer && target_buffer > last_gui_buffer->number)
                        target_buffer -= last_gui_buffer->number;
                    gui_buffer_switch_by_number (gui_current_window,
                                                 target_buffer);
                }
            }
            else
            {
                /* absolute jump by number, or by category/name */
                error = NULL;
                number = strtol (argv[0], &error, 10);
                if (error && (error[0] == '\0'))
                    gui_buffer_switch_by_number (gui_current_window,
                                                 (int) number);
                else
                {
                    ptr_buffer = NULL;
                    if (argc > 1)
                        ptr_buffer = gui_buffer_search_by_category_name (argv[0],
                                                                         argv[1]);
                    else
                    {
                        ptr_buffer = gui_buffer_search_by_category_name (argv[0],
                                                                         NULL);
                        if (!ptr_buffer)
                            ptr_buffer = gui_buffer_search_by_category_name (NULL,
                                                                             argv[0]);
                    }
                    if (ptr_buffer)
                    {
                        gui_window_switch_to_buffer (gui_current_window,
                                                     ptr_buffer);
                        gui_window_redraw_buffer (ptr_buffer);
                    }
                }
            }
            
        }
    }
    return 0;
}

/*
 * command_builtin: launch WeeChat/IRC builtin command
 */

int
command_builtin (struct t_gui_buffer *buffer,
                 int argc, char **argv, char **argv_eol)
{
    char *command;
    int length;
    
    if (argc > 0)
    {
        if (argv[0][0] == '/')
            input_data (buffer, argv_eol[0], 1);
        else
        {
            length = strlen (argv_eol[0]) + 2;
            command = (char *)malloc (length);
            if (command)
            {
                snprintf (command, length, "/%s", argv_eol[0]);
                input_data (buffer, command, 1);
                free (command);
            }
        }
    }
    return 0;
}

/*
 * command_clear: display or create alias
 */

int
command_clear (struct t_gui_buffer *buffer,
               int argc, char**argv, char **argv_eol)
{
    struct t_gui_buffer *ptr_buffer;
    char *error;
    long number;
    int i;
    
    /* make C compiler happy */
    (void) argv_eol;
    
    if (argc > 0)
    {
        if (string_strcasecmp (argv[0], "-all") == 0)
            gui_buffer_clear_all ();
        else
        {
            for (i = 0; i < argc; i++)
            {
                error = NULL;
                number = strtol (argv[i], &error, 10);
                if (error && (error[0] == '\0'))
                {
                    ptr_buffer = gui_buffer_search_by_number (number);
                    if (ptr_buffer)
                        gui_buffer_clear (ptr_buffer);
                }
            }
        }
    }
    else
        gui_buffer_clear (buffer);
    
    return 0;
}

/*
 * command_debug_display_windows: display tree of windows
 */

void
command_debug_display_windows (struct t_gui_window_tree *tree, int indent)
{
    if (tree)
    {
        if (tree->window)
        {
            /* leaf */
            gui_chat_printf (NULL,
                             "leaf: %X (parent:%X), win=%X, child1=%X, "
                             "child2=%X, %d,%d %dx%d, %d%%x%d%%",
                             tree, tree->parent_node, tree->window,
                             tree->child1, tree->child2,
                             tree->window->win_x, tree->window->win_y,
                             tree->window->win_width, tree->window->win_height,
                             tree->window->win_width_pct,
                             tree->window->win_height_pct);
        }
        else
        {
            /* node */
            gui_chat_printf (NULL,
                             "node: %X (parent:%X), win=%X, child1=%X, "
                             "child2=%X)",
                             tree, tree->parent_node, tree->window,
                             tree->child1, tree->child2);
        }
        
        if (tree->child1)
            command_debug_display_windows (tree->child1, indent + 1);
        if (tree->child2)
            command_debug_display_windows (tree->child2, indent + 1);
    }
}

/*
 * command_debug: print debug messages
 */

int
command_debug (struct t_gui_buffer *buffer,
               int argc, char **argv, char **argv_eol)
{
    /* make C compiler happy */
    (void) argv_eol;

    if (argc >= 1)
    {
        if (string_strcasecmp (argv[0], "dump") == 0)
        {
            weechat_dump (0);
        }
        else if (string_strcasecmp (argv[0], "buffer") == 0)
        {
            gui_buffer_dump_hexa (buffer);
        }
        else if (string_strcasecmp (argv[0], "windows") == 0)
        {
            gui_chat_printf (NULL, "");
            gui_chat_printf (NULL, "DEBUG: windows tree:");
            command_debug_display_windows (gui_windows_tree, 1);
        }
        else
        {
            gui_chat_printf (NULL,
                             _("%sError: unknown option for \"%s\" command"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             "debug");
            return -1;
        }
    }
    
    return 0;
}

/*
 * command_help: display help about commands
 */

int
command_help (struct t_gui_buffer *buffer,
              int argc, char **argv, char **argv_eol)
{
    int i;
    struct t_hook *ptr_hook;
    
    /* make C compiler happy */
    (void) buffer;
    (void) argv_eol;
    
    switch (argc)
    {
        case 0:
            gui_chat_printf (NULL, "");
            gui_chat_printf (NULL, _("%s internal commands:"), PACKAGE_NAME);
            for (i = 0; weechat_commands[i].name; i++)
            {
                if (weechat_commands[i].description
                    && weechat_commands[i].description[0])
                    gui_chat_printf (NULL, "   %s%s %s- %s",
                                     GUI_COLOR(GUI_COLOR_CHAT_BUFFER),
                                     weechat_commands[i].name,
                                     GUI_COLOR(GUI_COLOR_CHAT),
                                     _(weechat_commands[i].description));
                else
                    gui_chat_printf (NULL, "   %s%s",
                                     GUI_COLOR(GUI_COLOR_CHAT_BUFFER),
                                     weechat_commands[i].name);
            }
            gui_chat_printf (NULL, "");
            gui_chat_printf (NULL, _("Other commands:"));
            for (ptr_hook = weechat_hooks; ptr_hook;
                 ptr_hook = ptr_hook->next_hook)
            {
                if ((ptr_hook->type == HOOK_TYPE_COMMAND)
                    && HOOK_COMMAND(ptr_hook, command)
                    && HOOK_COMMAND(ptr_hook, command)[0])
                {
                    if (HOOK_COMMAND(ptr_hook, description)
                        && HOOK_COMMAND(ptr_hook, description)[0])
                        gui_chat_printf (NULL, "   %s%s %s- %s",
                                         GUI_COLOR(GUI_COLOR_CHAT_BUFFER),
                                         HOOK_COMMAND(ptr_hook, command),
                                         GUI_COLOR(GUI_COLOR_CHAT),
                                         HOOK_COMMAND(ptr_hook, description));
                    else
                        gui_chat_printf (NULL, "   %s%s",
                                         GUI_COLOR(GUI_COLOR_CHAT_BUFFER),
                                         HOOK_COMMAND(ptr_hook, command));
                }
            }
            break;
        case 1:
            for (ptr_hook = weechat_hooks; ptr_hook;
                 ptr_hook = ptr_hook->next_hook)
            {
                if ((ptr_hook->type == HOOK_TYPE_COMMAND)
                    && HOOK_COMMAND(ptr_hook, command)
                    && HOOK_COMMAND(ptr_hook, command)[0]
                    && (string_strcasecmp (HOOK_COMMAND(ptr_hook, command),
                                           argv[0]) == 0))
                {
                    gui_chat_printf (NULL, "");
                    gui_chat_printf (NULL,
                                     _("[%s%s]  %s/%s  %s%s"),
                                     (ptr_hook->plugin) ?
                                     _("plugin:") : "weechat",
                                     (ptr_hook->plugin) ?
                                     ptr_hook->plugin->name : "",
                                     GUI_COLOR(GUI_COLOR_CHAT_BUFFER),
                                     HOOK_COMMAND(ptr_hook, command),
                                     GUI_COLOR(GUI_COLOR_CHAT),
                                     (HOOK_COMMAND(ptr_hook, args)
                                      && HOOK_COMMAND(ptr_hook, args)[0]) ?
                                     HOOK_COMMAND(ptr_hook, args) : "");
                    if (HOOK_COMMAND(ptr_hook, description)
                        && HOOK_COMMAND(ptr_hook, description)[0])
                    {
                        gui_chat_printf (NULL, "");
                        gui_chat_printf (NULL, "%s",
                                         HOOK_COMMAND(ptr_hook, description));
                    }
                    if (HOOK_COMMAND(ptr_hook, args_description)
                        && HOOK_COMMAND(ptr_hook, args_description)[0])
                    {
                        gui_chat_printf (NULL, "");
                        gui_chat_printf (NULL, "%s",
                                         HOOK_COMMAND(ptr_hook, args_description));
                    }
                    return 0;
                }
            }
            for (i = 0; weechat_commands[i].name; i++)
            {
                if (string_strcasecmp (weechat_commands[i].name, argv[0]) == 0)
                {
                    gui_chat_printf (NULL, "");
                    gui_chat_printf (NULL, "[weechat]  %s/%s  %s%s",
                                     GUI_COLOR(GUI_COLOR_CHAT_BUFFER),
                                     weechat_commands[i].name,
                                     GUI_COLOR(GUI_COLOR_CHAT),
                                     (weechat_commands[i].arguments
                                      && weechat_commands[i].arguments[0]) ?
                                     _(weechat_commands[i].arguments) : "");
                    if (weechat_commands[i].description &&
                        weechat_commands[i].description[0])
                    {
                        gui_chat_printf (NULL, "");
                        gui_chat_printf (NULL, "%s",
                                         _(weechat_commands[i].description));
                    }
                    if (weechat_commands[i].arguments_description &&
                        weechat_commands[i].arguments_description[0])
                    {
                        gui_chat_printf (NULL, "");
                        gui_chat_printf (NULL, "%s",
                                         _(weechat_commands[i].arguments_description));
                    }
                    return 0;
                }
            }
            gui_chat_printf (NULL,
                             _("%sNo help available, \"%s\" is an "
                               "unknown command"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             argv[0]);
            break;
    }
    return 0;
}

/*
 * command_history: display current buffer history
 */

int
command_history (struct t_gui_buffer *buffer,
                 int argc, char **argv, char **argv_eol)
{
    struct t_gui_history *ptr_history;
    int n, n_total, n_user, displayed;
    
    /* make C compiler happy */
    (void) argv_eol;
    
    n_user = cfg_history_display_default;
    
    if (argc == 1)
    {
        if (string_strcasecmp (argv[0], "clear") == 0)
        {
            gui_history_buffer_free (buffer);
            return 0;
        }
        else
            n_user = atoi (argv[0]);
    }
    
    if (buffer->history)
    {
        n_total = 1;
        for (ptr_history = buffer->history;
             ptr_history->next_history;
             ptr_history = ptr_history->next_history)
        {
            n_total++;
        }
        displayed = 0;
        for (n = 0; ptr_history; ptr_history = ptr_history->prev_history, n++)
        {
            if ((n_user > 0) && ((n_total - n_user) > n))
                continue;
            if (!displayed)
            {
                gui_chat_printf (buffer, "");
                gui_chat_printf (buffer, _("Buffer command history:"));
            }
            gui_chat_printf (buffer, "%s", ptr_history->text);
            displayed = 1;
        }
    }
    
    return 0;
}

/*
 * command_key_display: display a key binding
 */

void
command_key_display (t_gui_key *key, int new_key)
{
    char *expanded_name;

    expanded_name = gui_keyboard_get_expanded_name (key->key);
    if (new_key)
        gui_chat_printf (NULL,
                         _("%sNew key binding: %s%s => %s%s%s%s%s"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_INFO],
                         (expanded_name) ? expanded_name : key->key,
                         GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                         GUI_COLOR(GUI_COLOR_CHAT),
                         (key->function) ?
                         gui_keyboard_function_search_by_ptr (key->function) : key->command,
                         (key->args) ? " \"" : "",
                         (key->args) ? key->args : "",
                         (key->args) ? "\"" : "");
    else
        gui_chat_printf (NULL, "  %20s%s => %s%s%s%s%s",
                         (expanded_name) ? expanded_name : key->key,
                         GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                         GUI_COLOR(GUI_COLOR_CHAT),
                         (key->function) ?
                         gui_keyboard_function_search_by_ptr (key->function) : key->command,
                         (key->args) ? " \"" : "",
                         (key->args) ? key->args : "",
                         (key->args) ? "\"" : "");
    if (expanded_name)
        free (expanded_name);
}

/*
 * command_key: bind/unbind keys
 */

int
command_key (struct t_gui_buffer *buffer,
             int argc, char **argv, char **argv_eol)
{
    char *args, *internal_code;
    int i;
    t_gui_key *ptr_key;
    t_gui_key_func *ptr_function;
    
    /* make C compiler happy */
    (void) buffer;

    if (argc == 0)
    {
        gui_chat_printf (NULL, "");
        gui_chat_printf (NULL, _("Key bindings:"));
        for (ptr_key = gui_keys; ptr_key; ptr_key = ptr_key->next_key)
        {
            command_key_display (ptr_key, 0);
        }
        return 0;
    }

    if (string_strcasecmp (argv[0], "functions") == 0)
    {
        gui_chat_printf (NULL, "");
        gui_chat_printf (NULL, _("Internal key functions:"));
        i = 0;
        while (gui_key_functions[i].function_name)
        {
            gui_chat_printf (NULL,
                             "%25s  %s",
                             gui_key_functions[i].function_name,
                             _(gui_key_functions[i].description));
            i++;
        }
        return 0;
    }

    if (string_strcasecmp (argv[0], "reset") == 0)
    {
        if ((argc == 1) && (string_strcasecmp (argv[1], "-yes") == 0))
        {
            gui_keyboard_free_all ();
            gui_keyboard_init ();
            gui_chat_printf (NULL,
                             _("%sDefault key bindings restored"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_INFO]);
        }
        else
        {
            gui_chat_printf (NULL,
                             _("%sError: \"-yes\" argument is required for "
                               "keys reset (security reason)"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR]);
            return -1;
        }
        return 0;
    }

    if (string_strcasecmp (argv[0], "unbind") == 0)
    {
        if (argc >= 2)
        {
            if (gui_keyboard_unbind (argv[1]))
            {
                gui_chat_printf (NULL,
                                 _("%sKey \"%s\" unbound"),
                                 gui_chat_prefix[GUI_CHAT_PREFIX_INFO],
                                 argv[1]);
            }
            else
            {
                gui_chat_printf (NULL,
                                 _("%sError: unable to unbind key \"%s\""),
                                 gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                 argv[1]);
                return -1;
            }
        }
        return 0;
    }
    
    if (string_strcasecmp (argv[0], "call") == 0)
    {
        if (argc >= 2)
        {
            ptr_function = gui_keyboard_function_search_by_name (argv[1]);
            if (ptr_function)
            {
                args = string_remove_quotes (argv_eol[2], "'\"");
                (void)(*ptr_function)((args) ? args : argv_eol[2]);
                if (args)
                    free (args);
            }
            else
            {
                gui_chat_printf (NULL,
                                 _("%sError: unknown key function \"%s\""),
                                 gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                 argv[1]);
                return -1;
            }
        }
        return 0;
    }
    
    /* display a key */
    if (argc == 1)
    {
        ptr_key = NULL;
        internal_code = gui_keyboard_get_internal_code (argv[0]);
        if (internal_code)
            ptr_key = gui_keyboard_search (internal_code);
        if (ptr_key)
        {
            gui_chat_printf (NULL, "");
            gui_chat_printf (NULL, _("Key:"));
            command_key_display (ptr_key, 0);
        }
        else
        {
            gui_chat_printf (NULL,
                             _("No key found."));
        }
        if (internal_code)
            free (internal_code);
        return 0;
    }

    /* bind new key */
    ptr_key = gui_keyboard_bind (argv[0], argv_eol[1]);
    if (ptr_key)
        command_key_display (ptr_key, 1);
    else
    {
        gui_chat_printf (NULL,
                         _("%sError: unable to bind key \"%s\""),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         argv[0]);
        return -1;
    }
    return 0;
}

/*
 * command_plugin_list: list loaded plugins
 */

void
command_plugin_list (char *name, int full)
{
    struct t_weechat_plugin *ptr_plugin;
    struct t_hook *ptr_hook;
    int plugins_found, hook_found, interval;
    
    gui_chat_printf (NULL, "");
    if (!name)
    {
        gui_chat_printf (NULL, _("Plugins loaded:"));
    }
    
    plugins_found = 0;
    
    for (ptr_plugin = weechat_plugins; ptr_plugin;
         ptr_plugin = ptr_plugin->next_plugin)
    {
        if (!name || (string_strcasestr (ptr_plugin->name, name)))
        {
            plugins_found++;
            
            /* plugin info */
            gui_chat_printf (NULL,
                             "  %s%s%s v%s - %s (%s)",
                             GUI_COLOR(GUI_COLOR_CHAT_BUFFER),
                             ptr_plugin->name,
                             GUI_COLOR(GUI_COLOR_CHAT),
                             ptr_plugin->version,
                             ptr_plugin->description,
                             ptr_plugin->filename);
            
            if (full)
            {
                /* commands hooked */
                hook_found = 0;
                for (ptr_hook = weechat_hooks; ptr_hook;
                     ptr_hook = ptr_hook->next_hook)
                {
                    if ((ptr_hook->plugin == ptr_plugin)
                        && (ptr_hook->type == HOOK_TYPE_COMMAND))
                    {
                        if (!hook_found)
                            gui_chat_printf (NULL, _("     commands hooked:"));
                        hook_found = 1;
                        gui_chat_printf (NULL,
                                         "       /%s %s%s%s",
                                         ((struct t_hook_command *)ptr_hook->hook_data)->command,
                                         (((struct t_hook_command *)ptr_hook->hook_data)->description) ? "(" : "",
                                         (((struct t_hook_command *)ptr_hook->hook_data)->description) ?
                                         ((struct t_hook_command *)ptr_hook->hook_data)->description : "",
                                         (((struct t_hook_command *)ptr_hook->hook_data)->description) ? ")" : "");
                    }
                }
                
                /* messages hooked */
                hook_found = 0;
                for (ptr_hook = weechat_hooks; ptr_hook;
                     ptr_hook = ptr_hook->next_hook)
                {
                    if ((ptr_hook->plugin == ptr_plugin)
                        && (ptr_hook->type == HOOK_TYPE_MESSAGE))
                    {
                        if (!hook_found)
                            gui_chat_printf (NULL, _("     messages hooked:"));
                        hook_found = 1;
                        gui_chat_printf (NULL,
                                         "       %s",
                                         ((struct t_hook_message *)ptr_hook->hook_data)->message);
                    }
                }
                
                /* config options hooked */
                hook_found = 0;
                for (ptr_hook = weechat_hooks; ptr_hook;
                     ptr_hook = ptr_hook->next_hook)
                {
                    if ((ptr_hook->plugin == ptr_plugin)
                        && (ptr_hook->type == HOOK_TYPE_MESSAGE))
                    {
                        if (!hook_found)
                            gui_chat_printf (NULL,
                                             _("     configuration otions "
                                               "hooked:"));
                        hook_found = 1;
                        gui_chat_printf (NULL,
                                         "       (%s) %s",
                                         ((struct t_hook_config *)ptr_hook->hook_data)->type ?
                                         ((struct t_hook_config *)ptr_hook->hook_data)->type : "*",
                                         ((struct t_hook_config *)ptr_hook->hook_data)->option ?
                                         ((struct t_hook_config *)ptr_hook->hook_data)->option : "*");

                    }
                }
                
                /* timers hooked */
                hook_found = 0;
                for (ptr_hook = weechat_hooks; ptr_hook;
                     ptr_hook = ptr_hook->next_hook)
                {
                    if ((ptr_hook->plugin == ptr_plugin)
                        && (ptr_hook->type == HOOK_TYPE_TIMER))
                    {
                        if (!hook_found)
                            gui_chat_printf (NULL, _("     timers hooked:"));
                        hook_found = 1;
                        interval = (((struct t_hook_timer *)ptr_hook->hook_data)->interval % 1000 == 0) ?
                            ((struct t_hook_timer *)ptr_hook->hook_data)->interval / 1000 :
                            ((struct t_hook_timer *)ptr_hook->hook_data)->interval;
                        if (((struct t_hook_timer *)ptr_hook->hook_data)->remaining_calls > 0)
                            gui_chat_printf (NULL,
                                             _("       %d %s "
                                               "(%d calls remaining)"),
                                             interval,
                                             (((struct t_hook_timer *)ptr_hook->hook_data)->interval % 1000 == 0) ?
                                             ((interval > 1) ?
                                              _("seconds") :
                                              _("second")) :
                                             ((interval > 1) ?
                                              _("milliseconds") :
                                              _("millisecond")),
                                             ((struct t_hook_timer *)ptr_hook->hook_data)->remaining_calls);
                        else
                            gui_chat_printf (NULL,
                                             _("       %d %s "
                                               "(no call limit)"),
                                             interval,
                                             (((struct t_hook_timer *)ptr_hook->hook_data)->interval % 1000 == 0) ?
                                             ((interval > 1) ?
                                              _("seconds") :
                                              _("second")) :
                                             ((interval > 1) ?
                                              _("milliseconds") :
                                              _("millisecond")));
                    }
                }

                /* fd hooked */
                hook_found = 0;
                for (ptr_hook = weechat_hooks; ptr_hook;
                     ptr_hook = ptr_hook->next_hook)
                {
                    if ((ptr_hook->plugin == ptr_plugin)
                        && (ptr_hook->type == HOOK_TYPE_MESSAGE))
                    {
                        if (!hook_found)
                            gui_chat_printf (NULL,
                                             _("     fd hooked:"));
                        hook_found = 1;
                        gui_chat_printf (NULL,
                                         _("       %d (flags: %d)"),
                                         ((struct t_hook_fd *)ptr_hook->hook_data)->fd,
                                         ((struct t_hook_fd *)ptr_hook->hook_data)->flags);
                    }
                }
                
                /* keyboards hooked */
                hook_found = 0;
                for (ptr_hook = weechat_hooks; ptr_hook;
                     ptr_hook = ptr_hook->next_hook)
                {
                    if ((ptr_hook->plugin == ptr_plugin)
                        && (ptr_hook->type == HOOK_TYPE_KEYBOARD))
                        hook_found++;
                }
                if (hook_found)
                    gui_chat_printf (NULL, _("     %d keyboards hooked"),
                                     hook_found);
            }
        }
    }
    if (plugins_found == 0)
    {
        if (name)
            gui_chat_printf (NULL, _("No plugin found."));
        else
            gui_chat_printf (NULL, _("  (no plugin)"));
    }
}
    
/*
 * command_plugin: list/load/unload WeeChat plugins
 */

int
command_plugin (struct t_gui_buffer *buffer,
                int argc, char **argv, char **argv_eol)
{
    /* make C compiler happy */
    (void) buffer;
    (void) argv_eol;
    
    switch (argc)
    {
        case 0:
            command_plugin_list (NULL, 0);
            break;
        case 1:
            if (string_strcasecmp (argv[0], "list") == 0)
                command_plugin_list (NULL, 0);
            else if (string_strcasecmp (argv[0], "listfull") == 0)
                command_plugin_list (NULL, 1);
            else if (string_strcasecmp (argv[0], "autoload") == 0)
                plugin_auto_load ();
            else if (string_strcasecmp (argv[0], "reload") == 0)
            {
                plugin_unload_all ();
                plugin_auto_load ();
            }
            else if (string_strcasecmp (argv[0], "unload") == 0)
                plugin_unload_all ();
            break;
        case 2:
            if (string_strcasecmp (argv[0], "list") == 0)
                command_plugin_list (argv[1], 0);
            else if (string_strcasecmp (argv[0], "listfull") == 0)
                command_plugin_list (argv[1], 1);
            else if (string_strcasecmp (argv[0], "load") == 0)
                plugin_load (argv[1]);
            else if (string_strcasecmp (argv[0], "reload") == 0)
                plugin_reload_name (argv[1]);
            else if (string_strcasecmp (argv[0], "unload") == 0)
                plugin_unload_name (argv[1]);
            else
            {
                gui_chat_printf (NULL,
                                 _("%sError: unknown option for \"%s\" "
                                   "command"),
                                 gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                 "plugin");
            }
            break;
        default:
            gui_chat_printf (NULL,
                             _("%sError: wrong argument count for \"%s\" "
                               "command"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             "plugin");
    }
    
    return 0;
}

/*
 * command_quit: quit WeeChat
 */

int
command_quit (struct t_gui_buffer *buffer,
              int argc, char **argv, char **argv_eol)
{
    (void) buffer;
    (void) argc;
    (void) argv;
    (void) argv_eol;
    
    quit_weechat = 1;
    
    return 0;
}

/*
 * command_save: save WeeChat and plugins options to disk
 */

int
command_save (struct t_gui_buffer *buffer,
              int argc, char **argv, char **argv_eol)
{
    /* make C compiler happy */
    (void) buffer;
    (void) argc;
    (void) argv;
    (void) argv_eol;

    /* save WeeChat configuration */
    if (weechat_config_write () == 0)
        gui_chat_printf (NULL,
                         _("%sWeeChat configuration file saved"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_INFO]);
    else
        gui_chat_printf (NULL,
                         _("%sError: failed to save configuration file"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR]);
    
    /* save plugins configuration */
    if (plugin_config_write () == 0)
        gui_chat_printf (NULL, _("%sPlugins options saved"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_INFO]);
    else
        gui_chat_printf (NULL,
                         _("%sError: failed to save plugins options"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR]);
    
    return 0;
}

/*
 * command_set_display_option: display config option
 */

void
command_set_display_option (struct t_config_option *option, char *prefix,
                            char *message)
{
    char *color_name;
    
    switch (option->type)
    {
        case OPTION_TYPE_BOOLEAN:
            gui_chat_printf (NULL, "%s%s%s%s = %s%s",
                             prefix,
                             (message) ? message : "  ",
                             option->name,
                             GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                             GUI_COLOR(GUI_COLOR_CHAT_HOST),
                             (*((int *)(option->ptr_int))) ? "ON" : "OFF");
            break;
        case OPTION_TYPE_INT:
            gui_chat_printf (NULL, "%s%s%s%s = %s%d",
                             prefix,
                             (message) ? message : "  ",
                             option->name,
                             GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                             GUI_COLOR(GUI_COLOR_CHAT_HOST),
                             *((int *)(option->ptr_int)));
            break;
        case OPTION_TYPE_INT_WITH_STRING:
            gui_chat_printf (NULL, "%s%s%s%s = %s%s",
                             prefix,
                             (message) ? message : "  ",
                             option->name,
                             GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                             GUI_COLOR(GUI_COLOR_CHAT_HOST),
                             option->array_values[*((int *)(option->ptr_int))]);
            break;
        case OPTION_TYPE_COLOR:
            color_name = gui_color_get_name (*((int *)(option->ptr_int)));
            gui_chat_printf (NULL, "%s%s%s%s = %s%s",
                             prefix,
                             (message) ? message : "  ",
                             option->name,
                             GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                             GUI_COLOR(GUI_COLOR_CHAT_HOST),
                             (color_name) ? color_name : _("(unknown)"));
            break;
        case OPTION_TYPE_STRING:
            if (*((char **)(option->ptr_string)))
                gui_chat_printf (NULL, "%s%s%s%s = \"%s%s%s\"",
                                 prefix,
                                 (message) ? message : "  ",
                                 option->name,
                                 GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                 GUI_COLOR(GUI_COLOR_CHAT_HOST),
                                 *(option->ptr_string),
                                 GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS));
            else
                gui_chat_printf (NULL, "%s%s%s%s = \"\"",
                                 prefix,
                                 (message) ? message : "  ",
                                 option->name,
                                 GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS));
            break;
    }
}

/*
 * command_set_display_option_list: display list of options
 *                                      Return: number of options displayed
 */

int
command_set_display_option_list (char **config_sections,
                                 struct t_config_option **config_options,
                                 char *message, char *search)
{
    int i, j, number_found, section_displayed;
    
    if (!config_sections || !config_options)
        return 0;
    
    number_found = 0;
    
    for (i = 0; config_sections[i]; i++)
    {
        if (config_options[i])
        {
            section_displayed = 0;
            for (j = 0; config_options[i][j].name; j++)
            {
                if ((!search) ||
                    ((search) && (search[0])
                     && (string_strcasestr (config_options[i][j].name, search))))
                {
                    if (!section_displayed)
                    {
                        gui_chat_printf (NULL, "");
                        gui_chat_printf (NULL, "%s[%s%s%s]",
                                         GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                         GUI_COLOR(GUI_COLOR_CHAT_BUFFER),
                                         config_sections[i],
                                         GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS));
                        section_displayed = 1;
                    }
                    command_set_display_option (&config_options[i][j],
                                                "", message);
                    number_found++;
                }
            }
        }
    }
    
    return number_found;
}

/*
 * command_set: set config options
 */

int
command_set (struct t_gui_buffer *buffer,
             int argc, char **argv, char **argv_eol)
{
    char *value;
    struct t_config_option *ptr_option;
    int number_found, rc;
    
    /* make C compiler happy */
    (void) buffer;
    
    /* display list of options */
    if (argc < 2)
    {
        number_found = 0;
        
        number_found += command_set_display_option_list (weechat_config_sections,
                                                         weechat_config_options,
                                                         NULL,
                                                         (argc == 1) ? argv[0] : NULL);
        
        if (number_found == 0)
        {
            if (argc == 1)
                gui_chat_printf (NULL,
                                 _("No configuration option found with "
                                   "\"%s\""),
                                 argv[0]);
            else
                gui_chat_printf (NULL,
                                 _("No configuration option found"));
        }
        else
        {
            gui_chat_printf (NULL, "");
            if (argc == 1)
                gui_chat_printf (NULL,
                                 _("%s%d%s configuration option(s) found with "
                                   "\"%s\""),
                                 GUI_COLOR(GUI_COLOR_CHAT_BUFFER),
                                 number_found,
                                 GUI_COLOR(GUI_COLOR_CHAT),
                                 argv[0]);
            else
                gui_chat_printf (NULL,
                                 _("%s%d%s configuration option(s) found"),
                                 GUI_COLOR(GUI_COLOR_CHAT_BUFFER),
                                 number_found,
                                 GUI_COLOR(GUI_COLOR_CHAT));
        }
        return 0;
    }
    
    /* set option value */
    if ((argc >= 3) && (string_strcasecmp (argv[1], "=") == 0))
    {
        ptr_option = config_option_section_option_search (weechat_config_sections,
                                                          weechat_config_options,
                                                          argv[0]);
        if (!ptr_option)
        {
            gui_chat_printf (NULL,
                             _("%sError: configuration option \"%s\" not "
                               "found"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             argv[0]);
            return -1;
        }
        if (!ptr_option->handler_change)
        {
            gui_chat_printf (NULL,
                             _("%sError: option \"%s\" can not be "
                               "changed while WeeChat is "
                               "running"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             argv[0]);
            return -1;
        }
        value = string_remove_quotes (argv_eol[2], "'\"");
        rc = config_option_set (ptr_option,
                                (value) ? value : argv_eol[2]);
        if (value)
            free (value);
        if (rc == 0)
        {
            command_set_display_option (ptr_option,
                                        gui_chat_prefix[GUI_CHAT_PREFIX_INFO],
                                        _("Option changed: "));
            (void) (ptr_option->handler_change());
        }
        else
        {
            gui_chat_printf (NULL,
                             _("%sError: incorrect value for "
                               "option \"%s\""),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             argv[0]);
            return -1;
        }
    }
    
    return 0;
}

/*
 * command_setp: set plugin options
 */

int
command_setp (struct t_gui_buffer *buffer,
              int argc, char **argv, char **argv_eol)
{
    char *pos, *ptr_name, *value;
    struct t_plugin_option *ptr_option;
    int number_found;
    
    /* make C compiler happy */
    (void) buffer;
    (void) argc;
    (void) argv;

    if (argc < 2)
    {
        number_found = 0;
        for (ptr_option = plugin_options; ptr_option;
             ptr_option = ptr_option->next_option)
        {
            if ((argc == 0) ||
                (strstr (ptr_option->name, argv[0])))
            {
                if (number_found == 0)
                    gui_chat_printf (NULL, "");
                gui_chat_printf (NULL, "  %s%s = \"%s%s%s\"",
                                 ptr_option->name,
                                 GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                 GUI_COLOR(GUI_COLOR_CHAT_HOST),
                                 ptr_option->value,
                                 GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS));
                number_found++;
            }
        }
        if (number_found == 0)
        {
            if (argc == 1)
                gui_chat_printf (NULL,
                                 _("No plugin option found with \"%s\""),
                                 argv[0]);
            else
                gui_chat_printf (NULL, _("No plugin option found"));
        }
        else
        {
            gui_chat_printf (NULL, "");
            if (argc == 1)
                gui_chat_printf (NULL, "%s%d%s plugin option(s) found with \"%s\"",
                                 GUI_COLOR(GUI_COLOR_CHAT_BUFFER),
                                 number_found,
                                 GUI_COLOR(GUI_COLOR_CHAT),
                                 argv[0]);
            else
                gui_chat_printf (NULL, "%s%d%s plugin option(s) found",
                                 GUI_COLOR(GUI_COLOR_CHAT_BUFFER),
                                 number_found,
                                 GUI_COLOR(GUI_COLOR_CHAT));
        }
    }

    if ((argc >= 3) && (string_strcasecmp (argv[1], "=") == 0))
    {
        ptr_name = NULL;
        ptr_option = plugin_config_search_internal (argv[0]);
        if (ptr_option)
            ptr_name = ptr_option->name;
        else
        {
            pos = strchr (argv[0], '.');
            if (pos)
                pos[0] = '\0';
            if (!pos || !pos[1] || (!plugin_search (argv[0])))
            {
                gui_chat_printf (NULL,
                                 _("%sError: plugin \"%s\" not found"),
                                 gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                 argv[0]);
                if (pos)
                    pos[0] = '.';
                return -1;
            }
            else
                ptr_name = argv[0];
            if (pos)
                pos[0] = '.';
        }
        if (ptr_name)
        {
            value = string_remove_quotes (argv_eol[2], "'\"");
            if (plugin_config_set_internal (ptr_name, (value) ? value : argv[2]))
                gui_chat_printf (NULL,
                                 _("Plugin option changed: %s%s = \"%s%s%s\""),
                                 ptr_name,
                                 GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                 GUI_COLOR(GUI_COLOR_CHAT_HOST),
                                 value,
                                 GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS));
            else
            {
                gui_chat_printf (NULL,
                                 _("%sError: incorrect value for plugin "
                                   "option \"%s\""),
                                 gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                 ptr_name);
                if (value)
                    free (value);
                return -1;
            }
            if (value)
                free (value);
        }
    }
    
    return 0;
}

/*
 * command_unalias: remove an alias
 */

int
command_unalias (struct t_gui_buffer *buffer,
                 int argc, char **argv, char **argv_eol)
{
    char *alias_name;
    struct t_weelist *ptr_weelist;
    struct alias *ptr_alias;
    
    /* make C compiler happy */
    (void) buffer;
    (void) argv_eol;

    if (argc > 0)
    {
        alias_name = (argv[0][0] == '/') ? argv[0] + 1 : argv[0];
        ptr_alias = alias_search (alias_name);
        if (!ptr_alias)
        {
            gui_chat_printf (NULL,
                             _("%sAlias \"%s\" not found"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             alias_name);
            return -1;
        }
        alias_free (ptr_alias);
        ptr_weelist = weelist_search (weechat_index_commands, alias_name);
        if (ptr_weelist)
            weelist_remove (&weechat_index_commands,
                            &weechat_last_index_command,
                            ptr_weelist);
        gui_chat_printf (NULL,
                         _("%sAlias \"%s\" removed"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_INFO],
                         alias_name);
    }
    return 0;
}

/*
 * command_upgrade: upgrade WeeChat
 */

int
command_upgrade (struct t_gui_buffer *buffer,
                 int argc, char **argv, char **argv_eol)
{
    /*int filename_length;
    char *filename, *ptr_binary;
    char *exec_args[7] = { NULL, "-a", "--dir", NULL, "--session", NULL, NULL };*/
    
    /* make C compiler happy */
    (void) buffer;
    (void) argc;
    (void) argv;
    (void) argv_eol;
    
    /*ptr_binary = (argc > 0) ? argv[0] : weechat_argv0;
    
    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        if (ptr_server->child_pid != 0)
        {
            gui_chat_printf_error (NULL,
                              _("Error: can't upgrade: connection to at least "
                                "one server is pending"));
            return -1;
            }*/
        /* TODO: remove this test, and fix gnutls save/load in session */
    /*if (ptr_server->is_connected && ptr_server->ssl_connected)
        {
            gui_chat_printf_error_nolog (NULL,
                                    _("Error: can't upgrade: connection to at least "
                                      "one SSL server is active "
                                      "(should be fixed in a future version)"));
            return -1;
        }
        if (ptr_server->outqueue)
        {
            gui_chat_printf_error_nolog (NULL,
                                    _("Error: can't upgrade: anti-flood is active on "
                                      "at least one server (sending many lines)"));
            return -1;
        }
    }
    
    filename_length = strlen (weechat_home) + strlen (WEECHAT_SESSION_NAME) + 2;
    filename = (char *) malloc (filename_length * sizeof (char));
    if (!filename)
        return -2;
    snprintf (filename, filename_length, "%s%s" WEECHAT_SESSION_NAME,
              weechat_home, DIR_SEPARATOR);
    
    gui_chat_printf_info_nolog (NULL,
                           _("Upgrading WeeChat..."));
    
    if (!session_save (filename))
    {
        free (filename);
        gui_chat_printf_error_nolog (NULL,
                                _("Error: unable to save session in file"));
        return -1;
    }
    
    exec_args[0] = strdup (ptr_binary);
    exec_args[3] = strdup (weechat_home);
    exec_args[5] = strdup (filename);*/
    
    /* unload plugins, save config, then upgrade */
    plugin_end ();
    /*if (cfg_look_save_on_exit)
        (void) config_write (NULL);
    gui_main_end ();
    fifo_remove ();
    weechat_log_close ();
    
    execvp (exec_args[0], exec_args);*/
    
    /* this code should not be reached if execvp is ok */
    plugin_init (1);
    
    /*string_iconv_fprintf (stderr,
                            _("Error: exec failed (program: \"%s\"), exiting WeeChat"),
                            exec_args[0]);
    
    free (exec_args[0]);
    free (exec_args[3]);
    free (filename);
    
    exit (EXIT_FAILURE);*/
    
    /* never executed */
    return -1;
}

/*
 * command_uptime: display WeeChat uptime
 */

int
command_uptime (struct t_gui_buffer *buffer,
                int argc, char **argv, char **argv_eol)
{
    time_t running_time;
    int day, hour, min, sec;
    char string[256];
    
    /* make C compiler happy */
    (void) argv_eol;
    
    running_time = time (NULL) - weechat_start_time;
    day = running_time / (60 * 60 * 24);
    hour = (running_time % (60 * 60 * 24)) / (60 * 60);
    min = ((running_time % (60 * 60 * 24)) % (60 * 60)) / 60;
    sec = ((running_time % (60 * 60 * 24)) % (60 * 60)) % 60;
    
    if ((argc == 1) && (string_strcasecmp (argv[0], "-o") == 0))
    {
        snprintf (string, sizeof (string),
                  _("WeeChat uptime: %d %s %02d:%02d:%02d, started on %s"),
                  day,
                  NG_("day", "days", day),
                  hour,
                  min,
                  sec,
                  ctime (&weechat_start_time));
        string[strlen (string) - 1] = '\0';
        input_data (buffer, string, 0);
    }
    else
    {
        gui_chat_printf (NULL,
                         _("WeeChat uptime: %s%d %s%s "
                           "%s%02d%s:%s%02d%s:%s%02d%s, "
                           "started on %s%s"),
                         GUI_COLOR(GUI_COLOR_CHAT_BUFFER),
                         day,
                         GUI_COLOR(GUI_COLOR_CHAT),
                         NG_("day", "days", day),
                         GUI_COLOR(GUI_COLOR_CHAT_BUFFER),
                         hour,
                         GUI_COLOR(GUI_COLOR_CHAT),
                         GUI_COLOR(GUI_COLOR_CHAT_BUFFER),
                         min,
                         GUI_COLOR(GUI_COLOR_CHAT),
                         GUI_COLOR(GUI_COLOR_CHAT_BUFFER),
                         sec,
                         GUI_COLOR(GUI_COLOR_CHAT),
                         GUI_COLOR(GUI_COLOR_CHAT_BUFFER),
                         ctime (&weechat_start_time));
    }
    
    return 0;
}

/*
 * command_window: manage windows
 */

int
command_window (struct t_gui_buffer *buffer,
                int argc, char **argv, char **argv_eol)
{
    struct t_gui_window *ptr_win;
    int i;
    char *error;
    long number;

    /* make C compiler happy */
    (void) buffer;
    (void) argv_eol;
    
    if ((argc == 0)
        || ((argc == 1) && (string_strcasecmp (argv[0], "list") == 0)))
    {
        /* list open windows */
        
        gui_chat_printf (NULL, "");
        gui_chat_printf (NULL, _("Open windows:"));
        
        i = 1;
        for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
        {
            gui_chat_printf (NULL, "%s[%s%d%s] (%s%d:%d%s;%s%dx%d%s) ",
                             GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                             GUI_COLOR(GUI_COLOR_CHAT),
                             i,
                             GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                             GUI_COLOR(GUI_COLOR_CHAT),
                             ptr_win->win_x,
                             ptr_win->win_y,
                             GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                             GUI_COLOR(GUI_COLOR_CHAT),
                             ptr_win->win_width,
                             ptr_win->win_height,
                             GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS));
            i++;
        }
    }
    else
    {
        if (string_strcasecmp (argv[0], "splith") == 0)
        {
            /* split window horizontally */
            if (argc > 1)
            {
                error = NULL;
                number = strtol (argv[1], &error, 10);
                if (error && (error[0] == '\0')
                    && (number > 0) && (number < 100))
                    gui_window_split_horiz (gui_current_window, number);
            }
            else
                gui_window_split_horiz (gui_current_window, 50);
        }
        else if (string_strcasecmp (argv[0], "splitv") == 0)
        {
            /* split window vertically */
            if (argc > 1)
            {
                error = NULL;
                number = strtol (argv[1], &error, 10);
                if (error && (error[0] == '\0')
                    && (number > 0) && (number < 100))
                    gui_window_split_vertic (gui_current_window, number);
            }
            else
                gui_window_split_vertic (gui_current_window, 50);
        }
        else if (string_strcasecmp (argv[0], "resize") == 0)
        {
            /* resize window */
            if (argc > 1)
            {
                error = NULL;
                number = strtol (argv[1], &error, 10);
                if (error && (error[0] == '\0')
                    && (number > 0) && (number < 100))
                    gui_window_resize (gui_current_window, number);
            }
        }
        else if (string_strcasecmp (argv[0], "merge") == 0)
        {
            if (argc >= 2)
            {
                if (string_strcasecmp (argv[1], "all") == 0)
                    gui_window_merge_all (gui_current_window);
                else
                {
                    gui_chat_printf (NULL,
                                     _("%sError: unknown option for \"%s\" "
                                       "command"),
                                     gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                     "window merge");
                    return -1;
                }
            }
            else
            {
                if (!gui_window_merge (gui_current_window))
                {
                    gui_chat_printf (NULL,
                                     _("%sError: can not merge windows, "
                                       "there's no other window with same "
                                       "size near current one."),
                                     gui_chat_prefix[GUI_CHAT_PREFIX_ERROR]);
                    return -1;
                }
            }
        }
        else if (string_strncasecmp (argv[0], "b", 1) == 0)
        {
            /* jump to window by buffer number */
            error = NULL;
            number = strtol (argv[0] + 1, &error, 10);
            if (error && (error[0] == '\0'))
                gui_window_switch_by_buffer (gui_current_window, number);
        }
        else if (string_strcasecmp (argv[0], "-1") == 0)
            gui_window_switch_previous (gui_current_window);
        else if (string_strcasecmp (argv[0], "+1") == 0)
            gui_window_switch_next (gui_current_window);
        else if (string_strcasecmp (argv[0], "up") == 0)
            gui_window_switch_up (gui_current_window);
        else if (string_strcasecmp (argv[0], "down") == 0)
            gui_window_switch_down (gui_current_window);
        else if (string_strcasecmp (argv[0], "left") == 0)
            gui_window_switch_left (gui_current_window);
        else if (string_strcasecmp (argv[0], "right") == 0)
            gui_window_switch_right (gui_current_window);
        else if (string_strcasecmp (argv[0], "scroll") == 0)
        {
            if (argc >= 2)
                gui_window_scroll (gui_current_window, argv[1]);
        }
        else
        {
            gui_chat_printf (NULL,
                             _("%sError: unknown option for \"%s\" command"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             "window");
            return -1;
        }
    }
    return 0;
}
