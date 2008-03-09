/*
 * Copyright (c) 2003-2008 by FlashCode <flashcode@flashtux.org>
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
#include "wee-config.h"
#include "wee-config-file.h"
#include "wee-debug.h"
#include "wee-hook.h"
#include "wee-input.h"
#include "wee-log.h"
#include "wee-upgrade.h"
#include "wee-string.h"
#include "wee-utf8.h"
#include "wee-list.h"
#include "../gui/gui-bar.h"
#include "../gui/gui-bar-item.h"
#include "../gui/gui-buffer.h"
#include "../gui/gui-chat.h"
#include "../gui/gui-color.h"
#include "../gui/gui-history.h"
#include "../gui/gui-input.h"
#include "../gui/gui-keyboard.h"
#include "../gui/gui-status.h"
#include "../gui/gui-window.h"
#include "../plugins/plugin.h"
#include "../plugins/plugin-config.h"


/*
 * command_bar: manage bars
 */

int
command_bar (void *data, struct t_gui_buffer *buffer,
             int argc, char **argv, char **argv_eol)
{
    int i, type, position, size, separator;
    long number;
    char *error;
    struct t_gui_bar *ptr_bar;
    struct t_gui_bar_item *ptr_item;
    
    /* make C compiler happy */
    (void) data;
    (void) buffer;
    (void) argc;
    (void) argv;
    (void) argv_eol;
    
    if ((argc == 1)
        || ((argc == 2) && (string_strcasecmp (argv[1], "list") == 0)))
    {
        /* list of bars */
        if (gui_bars)
        {
            gui_chat_printf (NULL, "");
            gui_chat_printf (NULL, _("List of bars:"));
            for (ptr_bar = gui_bars; ptr_bar;
                 ptr_bar = ptr_bar->next_bar)
            {
                gui_chat_printf (NULL,
                                 _("  %d. %s: %s, %s, %s: %d, items: %s%s (plugin: %s)"),
                                 ptr_bar->number,
                                 ptr_bar->name,
                                 gui_bar_type_str[ptr_bar->type],
                                 gui_bar_position_str[ptr_bar->position],
                                 ((ptr_bar->position == GUI_BAR_POSITION_BOTTOM)
                                  || (ptr_bar->position == GUI_BAR_POSITION_TOP)) ?
                                 _("height") : _("width"),
                                 ptr_bar->size,
                                 (ptr_bar->items) ? ptr_bar->items : "-",
                                 (ptr_bar->separator) ?
                                 _(", with separator") : "",
                                 (ptr_bar->plugin) ? ptr_bar->plugin->name : "-");
            }
        }
        else
            gui_chat_printf (NULL, _("No bar defined"));

        /* list of bar items */
        if (gui_bar_items)
        {
            gui_chat_printf (NULL, "");
            gui_chat_printf (NULL, _("List of bar items:"));
            for (ptr_item = gui_bar_items; ptr_item;
                 ptr_item = ptr_item->next_item)
            {
                gui_chat_printf (NULL,
                                 _("  %s (plugin: %s)"),
                                 ptr_item->name,
                                 (ptr_item->plugin) ? ptr_item->plugin->name : "-");
            }
        }
        else
            gui_chat_printf (NULL, _("No bar item defined"));
    }
    else
    {
        if (string_strcasecmp (argv[1], "add") == 0)
        {
            if (argc < 8)
            {
                gui_chat_printf (NULL,
                                 _("%sError: missing arguments for \"%s\" "
                                   "command"),
                                 gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                 "bar");
                return WEECHAT_RC_ERROR;
            }
            type = -1;
            for (i = 0; i < GUI_BAR_NUM_TYPES; i++)
            {
                if (string_strcasecmp (argv[3], gui_bar_type_str[i]) == 0)
                {
                    type = i;
                    break;
                }
            }
            if (type < 0)
            {
                gui_chat_printf (NULL,
                                 _("%sError: wrong type \"%s\" for bar "
                                   "\"%s\""),
                                 gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                 argv[3], argv[2]);
                return WEECHAT_RC_ERROR;
            }
            position = -1;
            for (i = 0; i < GUI_BAR_NUM_POSITIONS; i++)
            {
                if (string_strcasecmp (argv[4], gui_bar_position_str[i]) == 0)
                {
                    position = i;
                    break;
                }
            }
            if (position < 0)
            {
                gui_chat_printf (NULL,
                                 _("%sError: wrong position \"%s\" for bar "
                                   "\"%s\""),
                                 gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                 argv[4], argv[2]);
                return WEECHAT_RC_ERROR;
            }
            error = NULL;
            number = strtol (argv[5], &error, 10);
            if (error && (error[0] == '\0'))
            {
                size = number;
                separator = 0;
                if (strcmp (argv[6], "0") != 0)
                    separator = 1;

                /* create bar */
                if (gui_bar_new (NULL, argv[2], argv[3], argv[4], size,
                                 separator, argv[7]))
                    gui_chat_printf (NULL, _("%sBar \"%s\" created"),
                                     gui_chat_prefix[GUI_CHAT_PREFIX_INFO],
                                     argv[2]);
                else
                    gui_chat_printf (NULL, _("%sError: failed to create bar "
                                             "\"%s\""),
                                     gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                     argv[2]);
            }
            else
            {
                gui_chat_printf (NULL,
                                 _("%sError: wrong size \"%s\" for bar "
                                   "\"%s\""),
                                 gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                 argv[5], argv[2]);
                return WEECHAT_RC_ERROR;
            }
        }
        else
        {
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * command_buffer: manage buffers
 */

int
command_buffer (void *data, struct t_gui_buffer *buffer,
                int argc, char **argv, char **argv_eol)
{
    struct t_gui_buffer *ptr_buffer;
    long number;
    char *error, *value;
    int i, target_buffer;
    
    /* make C compiler happy */
    (void) data;
    
    if ((argc == 1)
        || ((argc == 2) && (string_strcasecmp (argv[1], "list") == 0)))
    {
        /* list buffers */
        
        gui_chat_printf (NULL, "");
        gui_chat_printf (NULL, _("Buffers list:"));
        
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
                             ptr_buffer->plugin->name : "core",
                             ptr_buffer->category,
                             ptr_buffer->name);
        }
    }
    else
    {
        if (string_strcasecmp (argv[1], "clear") == 0)
        {
            if (argc > 2)
            {
                if (string_strcasecmp (argv[2], "-all") == 0)
                    gui_buffer_clear_all ();
                else
                {
                    for (i = 2; i < argc; i++)
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
        }
        else if (string_strcasecmp (argv[1], "move") == 0)
        {
            /* move buffer to another number in the list */
            
            if (argc < 3)
            {
                gui_chat_printf (NULL,
                                 _("%sError: missing arguments for \"%s\" "
                                   "command"),
                                 gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                 "buffer");
                return WEECHAT_RC_ERROR;
            }
            
            error = NULL;
            number = strtol (((argv[2][0] == '+') || (argv[2][0] == '-')) ?
                             argv[2] + 1 : argv[2],
                             &error, 10);
            if (error && (error[0] == '\0'))
            {
                if (argv[2][0] == '+')
                    gui_buffer_move_to_number (buffer,
                                               buffer->number + ((int) number));
                else if (argv[2][0] == '-')
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
                return WEECHAT_RC_ERROR;
            }
        }
        else if (string_strcasecmp (argv[1], "close") == 0)
        {
            if (!buffer->plugin)
            {
                gui_chat_printf (NULL,
                                 _("%sError: WeeChat main buffer can't be "
                                   "closed"),
                                 gui_chat_prefix[GUI_CHAT_PREFIX_ERROR]);
                return WEECHAT_RC_ERROR;
            }
            gui_buffer_close (buffer, 1);
            gui_status_refresh_needed = 1;
            gui_current_window->buffer->input_refresh_needed = 1;
        }
        else if (string_strcasecmp (argv[1], "notify") == 0)
        {
            if (argc < 3)
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
                number = strtol (argv[2], &error, 10);
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
                        return WEECHAT_RC_ERROR;
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
                    return WEECHAT_RC_ERROR;
                }
            }
        }
        else if (string_strcasecmp (argv[1], "set") == 0)
        {
            /* set a property on buffer */
            
            if (argc < 4)
            {
                gui_chat_printf (NULL,
                                 _("%sError: missing arguments for \"%s\" "
                                   "command"),
                                 gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                 "buffer");
                return WEECHAT_RC_ERROR;
            }
            value = string_remove_quotes (argv_eol[3], "'\"");
            gui_buffer_set (buffer, argv[2], (value) ? value : argv_eol[3]);
            if (value)
                free (value);
        }
        else
        {
            /* jump to buffer by number or server/channel name */
            
            if (argv[1][0] == '-')
            {
                /* relative jump '-' */
                error = NULL;
                number = strtol (argv[1] + 1, &error, 10);
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
            else if (argv[1][0] == '+')
            {
                /* relative jump '+' */
                error = NULL;
                number = strtol (argv[1] + 1, &error, 10);
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
                number = strtol (argv[1], &error, 10);
                if (error && (error[0] == '\0'))
                    gui_buffer_switch_by_number (gui_current_window,
                                                 (int) number);
                else
                {
                    ptr_buffer = NULL;
                    if (argc > 2)
                        ptr_buffer = gui_buffer_search_by_category_name (argv[1],
                                                                         argv[2]);
                    else
                    {
                        ptr_buffer = gui_buffer_search_by_category_name (argv[1],
                                                                         NULL);
                        if (!ptr_buffer)
                            ptr_buffer = gui_buffer_search_by_category_name (NULL,
                                                                             argv[1]);
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
    return WEECHAT_RC_OK;
}

/*
 * command_builtin: launch WeeChat/IRC builtin command
 */

int
command_builtin (void *data, struct t_gui_buffer *buffer,
                 int argc, char **argv, char **argv_eol)
{
    char *command;
    int length;
    
    /* make C compiler happy */
    (void) data;
    
    if (argc > 1)
    {
        if (argv[1][0] == '/')
            input_data (buffer, argv_eol[1], 1);
        else
        {
            length = strlen (argv_eol[1]) + 2;
            command = (char *)malloc (length * sizeof (char));
            if (command)
            {
                snprintf (command, length, "/%s", argv_eol[1]);
                input_data (buffer, command, 1);
                free (command);
            }
        }
    }
    return WEECHAT_RC_OK;
}

/*
 * command_help: display help about commands
 */

int
command_help (void *data, struct t_gui_buffer *buffer,
              int argc, char **argv, char **argv_eol)
{
    struct t_hook *ptr_hook;
    
    /* make C compiler happy */
    (void) data;
    (void) buffer;
    (void) argv_eol;
    
    switch (argc)
    {
        case 1:
            gui_chat_printf (NULL, "");
            gui_chat_printf (NULL,
                             /* TRANSLATORS: %s is "WeeChat" */
                             _("%s internal commands:"),
                             PACKAGE_NAME);
            for (ptr_hook = weechat_hooks[HOOK_TYPE_COMMAND]; ptr_hook;
                 ptr_hook = ptr_hook->next_hook)
            {
                if (!ptr_hook->deleted
                    && !ptr_hook->plugin
                    && HOOK_COMMAND(ptr_hook, command)
                    && HOOK_COMMAND(ptr_hook, command)[0])
                {
                    gui_chat_printf (NULL, "   %s%s%s%s%s%s%s%s",
                                     GUI_COLOR(GUI_COLOR_CHAT_BUFFER),
                                     (HOOK_COMMAND(ptr_hook, level) > 0) ?
                                     "(" : "",
                                     HOOK_COMMAND(ptr_hook, command),
                                     (HOOK_COMMAND(ptr_hook, level) > 0) ?
                                     ")" : "",
                                     GUI_COLOR(GUI_COLOR_CHAT),
                                     (HOOK_COMMAND(ptr_hook, description)
                                      && HOOK_COMMAND(ptr_hook, description)[0]) ?
                                     " - " : "",
                                     (HOOK_COMMAND(ptr_hook, description)
                                      && HOOK_COMMAND(ptr_hook, description)[0]) ?
                                     _(HOOK_COMMAND(ptr_hook, description)) : "",
                                     (HOOK_COMMAND(ptr_hook, level) > 0) ?
                                     _("  (used by a plugin)") : "");
                }
            }
            gui_chat_printf (NULL, "");
            gui_chat_printf (NULL, _("Other commands:"));
            for (ptr_hook = weechat_hooks[HOOK_TYPE_COMMAND]; ptr_hook;
                 ptr_hook = ptr_hook->next_hook)
            {
                if (!ptr_hook->deleted
                    && ptr_hook->plugin
                    && HOOK_COMMAND(ptr_hook, command)
                    && HOOK_COMMAND(ptr_hook, command)[0])
                {
                    gui_chat_printf (NULL, "   %s%s%s%s%s%s%s%s",
                                     GUI_COLOR(GUI_COLOR_CHAT_BUFFER),
                                     (HOOK_COMMAND(ptr_hook, level) > 0) ?
                                     "(" : "",
                                     HOOK_COMMAND(ptr_hook, command),
                                     (HOOK_COMMAND(ptr_hook, level) > 0) ?
                                     ")" : "",
                                     GUI_COLOR(GUI_COLOR_CHAT),
                                     (HOOK_COMMAND(ptr_hook, description)
                                      && HOOK_COMMAND(ptr_hook, description)[0]) ?
                                     " - " : "",
                                     (HOOK_COMMAND(ptr_hook, description)
                                      && HOOK_COMMAND(ptr_hook, description)[0]) ?
                                     _(HOOK_COMMAND(ptr_hook, description)) : "",
                                     (HOOK_COMMAND(ptr_hook, level) > 0) ?
                                     _("  (masked by a plugin)") : "");
                }
            }
            break;
        case 2:
            for (ptr_hook = weechat_hooks[HOOK_TYPE_COMMAND]; ptr_hook;
                 ptr_hook = ptr_hook->next_hook)
            {
                if (!ptr_hook->deleted
                    && HOOK_COMMAND(ptr_hook, command)
                    && HOOK_COMMAND(ptr_hook, command)[0]
                    && (HOOK_COMMAND(ptr_hook, level) == 0)
                    && (string_strcasecmp (HOOK_COMMAND(ptr_hook, command),
                                           argv[1]) == 0))
                {
                    gui_chat_printf (NULL, "");
                    gui_chat_printf (NULL,
                                     "[%s%s]  %s/%s  %s%s",
                                     (ptr_hook->plugin) ?
                                     _("plugin:") : "weechat",
                                     (ptr_hook->plugin) ?
                                     ptr_hook->plugin->name : "",
                                     GUI_COLOR(GUI_COLOR_CHAT_BUFFER),
                                     HOOK_COMMAND(ptr_hook, command),
                                     GUI_COLOR(GUI_COLOR_CHAT),
                                     (HOOK_COMMAND(ptr_hook, args)
                                      && HOOK_COMMAND(ptr_hook, args)[0]) ?
                                     _(HOOK_COMMAND(ptr_hook, args)) : "");
                    if (HOOK_COMMAND(ptr_hook, description)
                        && HOOK_COMMAND(ptr_hook, description)[0])
                    {
                        gui_chat_printf (NULL, "");
                        gui_chat_printf (NULL, "%s",
                                         _(HOOK_COMMAND(ptr_hook, description)));
                    }
                    if (HOOK_COMMAND(ptr_hook, args_description)
                        && HOOK_COMMAND(ptr_hook, args_description)[0])
                    {
                        gui_chat_printf (NULL, "");
                        gui_chat_printf (NULL, "%s",
                                         _(HOOK_COMMAND(ptr_hook, args_description)));
                    }
                    return WEECHAT_RC_OK;
                }
            }
            gui_chat_printf (NULL,
                             _("%sNo help available, \"%s\" is an "
                               "unknown command"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             argv[1]);
            break;
    }
    return WEECHAT_RC_OK;
}

/*
 * command_history: display current buffer history
 */

int
command_history (void *data, struct t_gui_buffer *buffer,
                 int argc, char **argv, char **argv_eol)
{
    struct t_gui_history *ptr_history;
    int n, n_total, n_user, displayed;
    
    /* make C compiler happy */
    (void) data;
    (void) argv_eol;
    
    n_user = CONFIG_INTEGER(config_history_display_default);
    
    if (argc == 2)
    {
        if (string_strcasecmp (argv[1], "clear") == 0)
        {
            gui_history_buffer_free (buffer);
            return WEECHAT_RC_OK;
        }
        else
            n_user = atoi (argv[1]);
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
    
    return WEECHAT_RC_OK;
}

/*
 * command_key_display: display a key binding
 */

void
command_key_display (struct t_gui_key *key, int new_key)
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
command_key (void *data, struct t_gui_buffer *buffer,
             int argc, char **argv, char **argv_eol)
{
    char *args, *internal_code;
    int i;
    struct t_gui_key *ptr_key;
    t_gui_key_func *ptr_function;
    
    /* make C compiler happy */
    (void) data;
    (void) buffer;

    if (argc == 1)
    {
        gui_chat_printf (NULL, "");
        gui_chat_printf (NULL, _("Key bindings:"));
        for (ptr_key = gui_keys; ptr_key; ptr_key = ptr_key->next_key)
        {
            command_key_display (ptr_key, 0);
        }
        return WEECHAT_RC_OK;
    }

    if (string_strcasecmp (argv[1], "functions") == 0)
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
        return WEECHAT_RC_OK;
    }

    if (string_strcasecmp (argv[1], "reset") == 0)
    {
        if ((argc >= 3) && (string_strcasecmp (argv[2], "-yes") == 0))
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
            return WEECHAT_RC_ERROR;
        }
        return WEECHAT_RC_OK;
    }

    if (string_strcasecmp (argv[1], "unbind") == 0)
    {
        if (argc >= 3)
        {
            if (gui_keyboard_unbind (argv[2]))
            {
                gui_chat_printf (NULL,
                                 _("%sKey \"%s\" unbound"),
                                 gui_chat_prefix[GUI_CHAT_PREFIX_INFO],
                                 argv[2]);
            }
            else
            {
                gui_chat_printf (NULL,
                                 _("%sError: unable to unbind key \"%s\""),
                                 gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                 argv[2]);
                return WEECHAT_RC_ERROR;
            }
        }
        return WEECHAT_RC_OK;
    }
    
    if (string_strcasecmp (argv[1], "call") == 0)
    {
        if (argc >= 3)
        {
            ptr_function = gui_keyboard_function_search_by_name (argv[2]);
            if (ptr_function)
            {
                if (argc >= 4)
                {
                    args = string_remove_quotes (argv_eol[3], "'\"");
                    (void)(*ptr_function)((args) ? args : argv_eol[3]);
                    if (args)
                        free (args);
                }
                else
                    (void)(*ptr_function)(NULL);
            }
            else
            {
                gui_chat_printf (NULL,
                                 _("%sError: unknown key function \"%s\""),
                                 gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                 argv[2]);
                return WEECHAT_RC_ERROR;
            }
        }
        return WEECHAT_RC_OK;
    }
    
    /* display a key */
    if (argc == 2)
    {
        ptr_key = NULL;
        internal_code = gui_keyboard_get_internal_code (argv[1]);
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
                             _("No key found"));
        }
        if (internal_code)
            free (internal_code);
        return WEECHAT_RC_OK;
    }

    /* bind new key */
    ptr_key = gui_keyboard_bind (argv[1], argv_eol[2]);
    if (ptr_key)
        command_key_display (ptr_key, 1);
    else
    {
        gui_chat_printf (NULL,
                         _("%sError: unable to bind key \"%s\""),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         argv[1]);
        return WEECHAT_RC_ERROR;
    }
    return WEECHAT_RC_OK;
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
                /* second line of plugin info */
                gui_chat_printf (NULL,
                                 _("  written by \"%s\", license: %s"),
                                 ptr_plugin->author,
                                 ptr_plugin->license);
                
                /* commands hooked */
                hook_found = 0;
                for (ptr_hook = weechat_hooks[HOOK_TYPE_COMMAND]; ptr_hook;
                     ptr_hook = ptr_hook->next_hook)
                {
                    if (!ptr_hook->deleted && (ptr_hook->plugin == ptr_plugin))
                    {
                        if (!hook_found)
                            gui_chat_printf (NULL, _("    commands hooked:"));
                        hook_found = 1;
                        gui_chat_printf (NULL,
                                         "      /%s %s%s%s",
                                         HOOK_COMMAND(ptr_hook, command),
                                         HOOK_COMMAND(ptr_hook, description) ? "(" : "",
                                         HOOK_COMMAND(ptr_hook, description) ?
                                         HOOK_COMMAND(ptr_hook, description) : "",
                                         HOOK_COMMAND(ptr_hook, description) ? ")" : "");
                    }
                }
                
                /* timers hooked */
                hook_found = 0;
                for (ptr_hook = weechat_hooks[HOOK_TYPE_TIMER]; ptr_hook;
                     ptr_hook = ptr_hook->next_hook)
                {
                    if (!ptr_hook->deleted && (ptr_hook->plugin == ptr_plugin))
                    {
                        if (!hook_found)
                            gui_chat_printf (NULL, _("    timers hooked:"));
                        hook_found = 1;
                        interval = (HOOK_TIMER(ptr_hook, interval) % 1000 == 0) ?
                            HOOK_TIMER(ptr_hook, interval) / 1000 :
                            HOOK_TIMER(ptr_hook, interval);
                        if (HOOK_TIMER(ptr_hook, remaining_calls) > 0)
                            gui_chat_printf (NULL,
                                             _("      %d %s "
                                               "(%d calls remaining)"),
                                             interval,
                                             (HOOK_TIMER(ptr_hook, interval) % 1000 == 0) ?
                                             (NG_("second", "seconds", interval)) :
                                             (NG_("millisecond", "milliseconds", interval)),
                                             HOOK_TIMER(ptr_hook, remaining_calls));
                        else
                            gui_chat_printf (NULL,
                                             _("      %d %s "
                                               "(no call limit)"),
                                             interval,
                                             (HOOK_TIMER(ptr_hook, interval) % 1000 == 0) ?
                                             (NG_("second", "seconds", interval)) :
                                             (NG_("millisecond", "milliseconds", interval)));
                    }
                }
                
                /* fd hooked */
                hook_found = 0;
                for (ptr_hook = weechat_hooks[HOOK_TYPE_FD]; ptr_hook;
                     ptr_hook = ptr_hook->next_hook)
                {
                    if (!ptr_hook->deleted && (ptr_hook->plugin == ptr_plugin))
                    {
                        if (!hook_found)
                            gui_chat_printf (NULL,
                                             _("    fd hooked:"));
                        hook_found = 1;
                        gui_chat_printf (NULL,
                                         _("      %d (flags: %d)"),
                                         HOOK_FD(ptr_hook, fd),
                                         HOOK_FD(ptr_hook, flags));
                    }
                }
                
                /* prints hooked */
                hook_found = 0;
                for (ptr_hook = weechat_hooks[HOOK_TYPE_PRINT]; ptr_hook;
                     ptr_hook = ptr_hook->next_hook)
                {
                    if (!ptr_hook->deleted && (ptr_hook->plugin == ptr_plugin))
                    {
                        if (!hook_found)
                            gui_chat_printf (NULL, _("    prints hooked:"));
                        hook_found = 1;
                        if (HOOK_PRINT(ptr_hook, buffer))
                            gui_chat_printf (NULL,
                                             _("      buffer: %s / %s, message: \"%s\""),
                                             HOOK_PRINT(ptr_hook, buffer)->category,
                                             HOOK_PRINT(ptr_hook, buffer)->name,
                                             HOOK_PRINT(ptr_hook, message) ?
                                             HOOK_PRINT(ptr_hook, message) : _("(none)"));
                        else
                            gui_chat_printf (NULL,
                                             _("      message: \"%s\""),
                                             HOOK_PRINT(ptr_hook, message) ?
                                             HOOK_PRINT(ptr_hook, message) : _("(none)"));
                    }
                }
                
                /* signals hooked */
                hook_found = 0;
                for (ptr_hook = weechat_hooks[HOOK_TYPE_SIGNAL]; ptr_hook;
                     ptr_hook = ptr_hook->next_hook)
                {
                    if (!ptr_hook->deleted && (ptr_hook->plugin == ptr_plugin))
                    {
                        if (!hook_found)
                            gui_chat_printf (NULL, _("    signals hooked:"));
                        hook_found = 1;
                        gui_chat_printf (NULL,
                                         _("      signal: %s"),
                                         HOOK_SIGNAL(ptr_hook, signal) ?
                                         HOOK_SIGNAL(ptr_hook, signal) : _("(all)"));
                    }
                }
                
                /* config options hooked */
                hook_found = 0;
                for (ptr_hook = weechat_hooks[HOOK_TYPE_CONFIG]; ptr_hook;
                     ptr_hook = ptr_hook->next_hook)
                {
                    if (!ptr_hook->deleted && (ptr_hook->plugin == ptr_plugin))
                    {
                        if (!hook_found)
                            gui_chat_printf (NULL,
                                             _("    configuration options "
                                               "hooked:"));
                        hook_found = 1;
                        gui_chat_printf (NULL,
                                         "      (%s) %s",
                                         HOOK_CONFIG(ptr_hook, type) ?
                                         HOOK_CONFIG(ptr_hook, type) : "*",
                                         HOOK_CONFIG(ptr_hook, option) ?
                                         HOOK_CONFIG(ptr_hook, option) : "*");
                    }
                }

                /* completion hooked */
                hook_found = 0;
                for (ptr_hook = weechat_hooks[HOOK_TYPE_COMPLETION]; ptr_hook;
                     ptr_hook = ptr_hook->next_hook)
                {
                    if (!ptr_hook->deleted && (ptr_hook->plugin == ptr_plugin))
                    {
                        if (!hook_found)
                            gui_chat_printf (NULL,
                                             _("    completions hooked:"));
                        hook_found = 1;
                        gui_chat_printf (NULL,
                                         "        %s",
                                         HOOK_COMPLETION(ptr_hook, completion));
                    }
                }

                /* modifier hooked */
                hook_found = 0;
                for (ptr_hook = weechat_hooks[HOOK_TYPE_MODIFIER]; ptr_hook;
                     ptr_hook = ptr_hook->next_hook)
                {
                    if (!ptr_hook->deleted && (ptr_hook->plugin == ptr_plugin))
                    {
                        if (!hook_found)
                            gui_chat_printf (NULL,
                                             _("    modifiers hooked:"));
                        hook_found = 1;
                        gui_chat_printf (NULL,
                                         "        %s",
                                         HOOK_MODIFIER(ptr_hook, modifier));
                    }
                }
            }
        }
    }
    if (plugins_found == 0)
    {
        if (name)
            gui_chat_printf (NULL, _("No plugin found"));
        else
            gui_chat_printf (NULL, _("  (no plugin)"));
    }
}
    
/*
 * command_plugin: list/load/unload WeeChat plugins
 */

int
command_plugin (void *data, struct t_gui_buffer *buffer,
                int argc, char **argv, char **argv_eol)
{
    /* make C compiler happy */
    (void) data;
    (void) buffer;
    (void) argv_eol;
    
    switch (argc)
    {
        case 1:
            command_plugin_list (NULL, 0);
            break;
        case 2:
            if (string_strcasecmp (argv[1], "list") == 0)
                command_plugin_list (NULL, 0);
            else if (string_strcasecmp (argv[1], "listfull") == 0)
                command_plugin_list (NULL, 1);
            else if (string_strcasecmp (argv[1], "autoload") == 0)
                plugin_auto_load ();
            else if (string_strcasecmp (argv[1], "reload") == 0)
            {
                plugin_unload_all ();
                plugin_auto_load ();
            }
            else if (string_strcasecmp (argv[1], "unload") == 0)
                plugin_unload_all ();
            break;
        case 3:
            if (string_strcasecmp (argv[1], "list") == 0)
                command_plugin_list (argv[2], 0);
            else if (string_strcasecmp (argv[1], "listfull") == 0)
                command_plugin_list (argv[2], 1);
            else if (string_strcasecmp (argv[1], "load") == 0)
                plugin_load (argv[2]);
            else if (string_strcasecmp (argv[1], "reload") == 0)
                plugin_reload_name (argv[2]);
            else if (string_strcasecmp (argv[1], "unload") == 0)
                plugin_unload_name (argv[2]);
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
    
    return WEECHAT_RC_OK;
}

/*
 * command_quit: quit WeeChat
 */

int
command_quit (void *data, struct t_gui_buffer *buffer,
              int argc, char **argv, char **argv_eol)
{
    /* make C compiler happy */
    (void) data;
    (void) buffer;
    (void) argc;
    (void) argv;
    
    hook_signal_send ("quit",
                      WEECHAT_HOOK_SIGNAL_STRING,
                      (argc > 1) ?
                      argv_eol[1] : CONFIG_STRING(config_look_default_msg_quit));
    
    quit_weechat = 1;
    
    return WEECHAT_RC_OK;
}

/*
 * command_reload_file: reload a configuration file
 */

void
command_reload_file (struct t_config_file *config_file)
{
    if ((int) (config_file->callback_reload) (config_file->callback_reload_data,
                                              config_file) == 0)
    {
        gui_chat_printf (NULL,
                         _("%sOptions reloaded from %s"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_INFO],
                         config_file->filename);
    }
    else
    {
        gui_chat_printf (NULL,
                         _("%sError: failed to reload options from %s"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         config_file->filename);
    }
}

/*
 * command_reload: reload WeeChat and plugins options from disk
 */

int
command_reload (void *data, struct t_gui_buffer *buffer,
                int argc, char **argv, char **argv_eol)
{
    struct t_config_file *ptr_config_file;
    int i;
    
    /* make C compiler happy */
    (void) data;
    (void) buffer;
    (void) argv_eol;
    
    if (argc > 1)
    {
        for (i = 1; i < argc; i++)
        {
            ptr_config_file = config_file_search (argv[i]);
            if (ptr_config_file)
            {
                command_reload_file (ptr_config_file);
            }
            else
            {
                gui_chat_printf (NULL,
                                 _("%sUnknown configuration file \"%s\""),
                                 gui_chat_prefix[GUI_CHAT_PREFIX_INFO],
                                 argv[i]);
            }
        }
    }
    else
    {
        for (ptr_config_file = config_files; ptr_config_file;
             ptr_config_file = ptr_config_file->next_config)
        {
            if (ptr_config_file->callback_reload)
            {
                command_reload_file (ptr_config_file);
            }
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * command_save_file: save a configuration file
 */

void
command_save_file (struct t_config_file *config_file)
{
    if (config_file_write (config_file) == 0)
    {
        gui_chat_printf (NULL,
                         _("%sOptions saved to %s"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_INFO],
                         config_file->filename);
    }
    else
    {
        gui_chat_printf (NULL,
                         _("%sError: failed to save options to %s"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         config_file->filename);
    }
}

/*
 * command_save: save WeeChat and plugins options to disk
 */

int
command_save (void *data, struct t_gui_buffer *buffer,
              int argc, char **argv, char **argv_eol)
{
    struct t_config_file *ptr_config_file;
    int i;
    
    /* make C compiler happy */
    (void) data;
    (void) buffer;
    (void) argv_eol;

    if (argc > 1)
    {
        for (i = 1; i < argc; i++)
        {
            ptr_config_file = config_file_search (argv[i]);
            if (ptr_config_file)
            {
                command_save_file (ptr_config_file);
            }
            else
            {
                gui_chat_printf (NULL,
                                 _("%sUnknown configuration file \"%s\""),
                                 gui_chat_prefix[GUI_CHAT_PREFIX_INFO],
                                 argv[i]);
            }
        }
    }
    else
    {
        for (ptr_config_file = config_files; ptr_config_file;
             ptr_config_file = ptr_config_file->next_config)
        {
            command_save_file (ptr_config_file);
        }
    }
    
    return WEECHAT_RC_OK;
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
        case CONFIG_OPTION_BOOLEAN:
            gui_chat_printf (NULL, "%s%s%s%s = %s%s",
                             prefix,
                             (message) ? message : "  ",
                             option->name,
                             GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                             GUI_COLOR(GUI_COLOR_CHAT_HOST),
                             (CONFIG_BOOLEAN(option) == CONFIG_BOOLEAN_TRUE) ?
                             "ON" : "OFF");
            break;
        case CONFIG_OPTION_INTEGER:
            if (option->string_values)
                gui_chat_printf (NULL, "%s%s%s%s = %s%s",
                                 prefix,
                                 (message) ? message : "  ",
                                 option->name,
                                 GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                 GUI_COLOR(GUI_COLOR_CHAT_HOST),
                                 option->string_values[CONFIG_INTEGER(option)]);
            else
                gui_chat_printf (NULL, "%s%s%s%s = %s%d",
                                 prefix,
                                 (message) ? message : "  ",
                                 option->name,
                                 GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                 GUI_COLOR(GUI_COLOR_CHAT_HOST),
                                 CONFIG_INTEGER(option));
            break;
        case CONFIG_OPTION_STRING:
            gui_chat_printf (NULL, "%s%s%s%s = \"%s%s%s\"",
                             prefix,
                             (message) ? message : "  ",
                             option->name,
                             GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                             GUI_COLOR(GUI_COLOR_CHAT_HOST),
                             (option->value) ? CONFIG_STRING(option) : "",
                             GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS));
            break;
        case CONFIG_OPTION_COLOR:
            color_name = gui_color_get_name (CONFIG_COLOR(option));
            gui_chat_printf (NULL, "%s%s%s%s = %s%s",
                             prefix,
                             (message) ? message : "  ",
                             option->name,
                             GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                             GUI_COLOR(GUI_COLOR_CHAT_HOST),
                             (color_name) ? color_name : _("(unknown)"));
            break;
    }
}

/*
 * command_set_display_option_list: display list of options
 *                                  return: number of options displayed
 */

int
command_set_display_option_list (struct t_config_file *config_file,
                                 char *message, char *search)
{
    int number_found, section_displayed;
    struct t_config_section *ptr_section;
    struct t_config_option *ptr_option;
    
    if (!config_file)
        return 0;
    
    number_found = 0;
    
    for (ptr_section = config_file->sections; ptr_section;
         ptr_section = ptr_section->next_section)
    {
        if (ptr_section->options)
        {
            section_displayed = 0;
            for (ptr_option = ptr_section->options; ptr_option;
                 ptr_option = ptr_option->next_option)
            {
                if ((!search) ||
                    ((search) && (search[0])
                     && (string_strcasestr (ptr_option->name, search))))
                {
                    if (!section_displayed)
                    {
                        gui_chat_printf (NULL, "");
                        gui_chat_printf (NULL, "%s[%s%s%s]",
                                         GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                         GUI_COLOR(GUI_COLOR_CHAT_BUFFER),
                                         ptr_section->name,
                                         GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS));
                        section_displayed = 1;
                    }
                    command_set_display_option (ptr_option, "", message);
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
command_set (void *data, struct t_gui_buffer *buffer,
             int argc, char **argv, char **argv_eol)
{
    char *value;
    struct t_config_option *ptr_option;
    int number_found, rc;
    
    /* make C compiler happy */
    (void) data;
    (void) buffer;
    
    /* display list of options */
    if (argc < 3)
    {
        number_found = 0;
        
        number_found += command_set_display_option_list (weechat_config_file,
                                                         NULL,
                                                         (argc == 2) ?
                                                         argv[1] : NULL);
        
        if (number_found == 0)
        {
            if (argc == 2)
                gui_chat_printf (NULL,
                                 _("No configuration option found with "
                                   "\"%s\""),
                                 argv[1]);
            else
                gui_chat_printf (NULL,
                                 _("No configuration option found"));
        }
        else
        {
            gui_chat_printf (NULL, "");
            if (argc == 2)
                gui_chat_printf (NULL,
                                 NG_("%s%d%s configuration option found with "
                                     "\"%s\"",
                                     "%s%d%s configuration options found with "
                                     "\"%s\"",
                                     number_found),
                                 GUI_COLOR(GUI_COLOR_CHAT_BUFFER),
                                 number_found,
                                 GUI_COLOR(GUI_COLOR_CHAT),
                                 argv[1]);
            else
                gui_chat_printf (NULL,
                                 NG_("%s%d%s configuration option found",
                                     "%s%d%s configuration options found",
                                     number_found),
                                 GUI_COLOR(GUI_COLOR_CHAT_BUFFER),
                                 number_found,
                                 GUI_COLOR(GUI_COLOR_CHAT));
        }
        return WEECHAT_RC_OK;
    }
    
    /* set option value */
    if ((argc >= 4) && (string_strcasecmp (argv[2], "=") == 0))
    {
        ptr_option = config_file_search_option (weechat_config_file, NULL, argv[1]);
        if (!ptr_option)
        {
            gui_chat_printf (NULL,
                             _("%sError: configuration option \"%s\" not "
                               "found"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             argv[1]);
            return WEECHAT_RC_ERROR;
        }
        value = string_remove_quotes (argv_eol[3], "'\"");
        rc = config_file_option_set (ptr_option,
                                     (value) ? value : argv_eol[3],
                                     0);
        if (value)
            free (value);
        if (rc > 0)
        {
            command_set_display_option (ptr_option,
                                        gui_chat_prefix[GUI_CHAT_PREFIX_INFO],
                                        _("Option changed: "));
            if ((rc == 2) && (ptr_option->callback_change))
            {
                (void) (ptr_option->callback_change)
                    (ptr_option->callback_change_data);
            }
        }
        else
        {
            gui_chat_printf (NULL,
                             _("%sError: incorrect value for "
                               "option \"%s\""),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             argv[1]);
            return WEECHAT_RC_ERROR;
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * command_setp: set plugin options
 */

int
command_setp (void *data, struct t_gui_buffer *buffer,
              int argc, char **argv, char **argv_eol)
{
    char *pos, *ptr_name, *value;
    struct t_config_option *ptr_option;
    int number_found;
    
    /* make C compiler happy */
    (void) data;
    (void) buffer;
    (void) argc;
    (void) argv;

    if (argc < 3)
    {
        number_found = 0;
        for (ptr_option = plugin_options; ptr_option;
             ptr_option = ptr_option->next_option)
        {
            if ((argc == 1) ||
                (strstr (ptr_option->name, argv[1])))
            {
                if (number_found == 0)
                    gui_chat_printf (NULL, "");
                gui_chat_printf (NULL, "  %s%s = \"%s%s%s\"",
                                 ptr_option->name,
                                 GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                 GUI_COLOR(GUI_COLOR_CHAT_HOST),
                                 (char *)ptr_option->value,
                                 GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS));
                number_found++;
            }
        }
        if (number_found == 0)
        {
            if (argc == 2)
                gui_chat_printf (NULL,
                                 _("No plugin option found with \"%s\""),
                                 argv[1]);
            else
                gui_chat_printf (NULL, _("No plugin option found"));
        }
        else
        {
            gui_chat_printf (NULL, "");
            if (argc == 2)
                gui_chat_printf (NULL,
                                 NG_("%s%d%s plugin option found with \"%s\"",
                                     "%s%d%s plugin options found with \"%s\"",
                                     number_found),
                                 GUI_COLOR(GUI_COLOR_CHAT_BUFFER),
                                 number_found,
                                 GUI_COLOR(GUI_COLOR_CHAT),
                                 argv[1]);
            else
                gui_chat_printf (NULL,
                                 NG_("%s%d%s plugin option found",
                                     "%s%d%s plugin options found",
                                     number_found),
                                 GUI_COLOR(GUI_COLOR_CHAT_BUFFER),
                                 number_found,
                                 GUI_COLOR(GUI_COLOR_CHAT));
        }
    }

    if ((argc >= 4) && (string_strcasecmp (argv[2], "=") == 0))
    {
        ptr_name = NULL;
        ptr_option = plugin_config_search_internal (argv[1]);
        if (ptr_option)
            ptr_name = ptr_option->name;
        else
        {
            pos = strchr (argv[1], '.');
            if (pos)
                pos[0] = '\0';
            if (!pos || !pos[1] || (!plugin_search (argv[1])))
            {
                gui_chat_printf (NULL,
                                 _("%sError: plugin \"%s\" not found"),
                                 gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                 argv[1]);
                if (pos)
                    pos[0] = '.';
                return WEECHAT_RC_ERROR;
            }
            else
                ptr_name = argv[1];
            if (pos)
                pos[0] = '.';
        }
        if (ptr_name)
        {
            value = string_remove_quotes (argv_eol[3], "'\"");
            if (plugin_config_set_internal (ptr_name, (value) ? value : argv_eol[3]))
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
                return WEECHAT_RC_ERROR;
            }
            if (value)
                free (value);
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * command_upgrade: upgrade WeeChat
 */

int
command_upgrade (void *data, struct t_gui_buffer *buffer,
                 int argc, char **argv, char **argv_eol)
{
    /*int filename_length;
    char *filename, *ptr_binary;
    char *exec_args[7] = { NULL, "-a", "--dir", NULL, "--session", NULL, NULL };*/
    
    /* make C compiler happy */
    (void) data;
    (void) buffer;
    (void) argc;
    (void) argv;
    (void) argv_eol;
    
    /*ptr_binary = (argc > 1) ? argv_eol[1] : weechat_argv0;
    
    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        if (ptr_server->child_pid != 0)
        {
            gui_chat_printf_error (NULL,
                              _("Error: can't upgrade: connection to at least "
                                "one server is pending"));
            return WEECHAT_RC_ERROR;
            }*/
        /* TODO: remove this test, and fix gnutls save/load in session */
    /*if (ptr_server->is_connected && ptr_server->ssl_connected)
        {
            gui_chat_printf_error_nolog (NULL,
                                    _("Error: can't upgrade: connection to at least "
                                      "one SSL server is active "
                                      "(should be fixed in a future version)"));
            return WEECHAT_RC_ERROR;
        }
        if (ptr_server->outqueue)
        {
            gui_chat_printf_error_nolog (NULL,
                                    _("Error: can't upgrade: anti-flood is active on "
                                      "at least one server (sending many lines)"));
            return WEECHAT_RC_ERROR;
        }
    }
    
    filename_length = strlen (weechat_home) + strlen (WEECHAT_SESSION_NAME) + 2;
    filename = (char *)malloc (filename_length * sizeof (char));
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
        return WEECHAT_RC_ERROR;
    }
    
    exec_args[0] = strdup (ptr_binary);
    exec_args[3] = strdup (weechat_home);
    exec_args[5] = strdup (filename);*/
    
    /* unload plugins, save config, then upgrade */
    plugin_end ();
    /*if (CONFIG_BOOLEAN(config_look_save_on_exit))
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
    return WEECHAT_RC_ERROR;
}

/*
 * command_uptime: display WeeChat uptime
 */

int
command_uptime (void *data, struct t_gui_buffer *buffer,
                int argc, char **argv, char **argv_eol)
{
    time_t running_time;
    int day, hour, min, sec;
    char string[256];
    
    /* make C compiler happy */
    (void) data;
    (void) argv_eol;
    
    running_time = time (NULL) - weechat_start_time;
    day = running_time / (60 * 60 * 24);
    hour = (running_time % (60 * 60 * 24)) / (60 * 60);
    min = ((running_time % (60 * 60 * 24)) % (60 * 60)) / 60;
    sec = ((running_time % (60 * 60 * 24)) % (60 * 60)) % 60;
    
    if ((argc == 2) && (string_strcasecmp (argv[1], "-o") == 0))
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
    
    return WEECHAT_RC_OK;
}

/*
 * command_window: manage windows
 */

int
command_window (void *data, struct t_gui_buffer *buffer,
                int argc, char **argv, char **argv_eol)
{
    struct t_gui_window *ptr_win;
    int i;
    char *error;
    long number;

    /* make C compiler happy */
    (void) data;
    (void) buffer;
    (void) argv_eol;
    
    if ((argc == 1)
        || ((argc == 2) && (string_strcasecmp (argv[1], "list") == 0)))
    {
        /* list open windows */
        
        gui_chat_printf (NULL, "");
        gui_chat_printf (NULL, _("Windows list:"));
        
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
        if (string_strcasecmp (argv[1], "splith") == 0)
        {
            /* split window horizontally */
            if (argc > 2)
            {
                error = NULL;
                number = strtol (argv[2], &error, 10);
                if (error && (error[0] == '\0')
                    && (number > 0) && (number < 100))
                    gui_window_split_horiz (gui_current_window, number);
            }
            else
                gui_window_split_horiz (gui_current_window, 50);
        }
        else if (string_strcasecmp (argv[1], "splitv") == 0)
        {
            /* split window vertically */
            if (argc > 2)
            {
                error = NULL;
                number = strtol (argv[2], &error, 10);
                if (error && (error[0] == '\0')
                    && (number > 0) && (number < 100))
                    gui_window_split_vertic (gui_current_window, number);
            }
            else
                gui_window_split_vertic (gui_current_window, 50);
        }
        else if (string_strcasecmp (argv[1], "resize") == 0)
        {
            /* resize window */
            if (argc > 2)
            {
                error = NULL;
                number = strtol (argv[2], &error, 10);
                if (error && (error[0] == '\0')
                    && (number > 0) && (number < 100))
                    gui_window_resize (gui_current_window, number);
            }
        }
        else if (string_strcasecmp (argv[1], "merge") == 0)
        {
            if (argc > 2)
            {
                if (string_strcasecmp (argv[2], "all") == 0)
                    gui_window_merge_all (gui_current_window);
                else
                {
                    gui_chat_printf (NULL,
                                     _("%sError: unknown option for \"%s\" "
                                       "command"),
                                     gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                     "window merge");
                    return WEECHAT_RC_ERROR;
                }
            }
            else
            {
                if (!gui_window_merge (gui_current_window))
                {
                    gui_chat_printf (NULL,
                                     _("%sError: can not merge windows, "
                                       "there's no other window with same "
                                       "size near current one"),
                                     gui_chat_prefix[GUI_CHAT_PREFIX_ERROR]);
                    return WEECHAT_RC_ERROR;
                }
            }
        }
        else if (string_strncasecmp (argv[1], "b", 1) == 0)
        {
            /* jump to window by buffer number */
            error = NULL;
            number = strtol (argv[1] + 1, &error, 10);
            if (error && (error[0] == '\0'))
                gui_window_switch_by_buffer (gui_current_window, number);
        }
        else if (string_strcasecmp (argv[1], "-1") == 0)
            gui_window_switch_previous (gui_current_window);
        else if (string_strcasecmp (argv[1], "+1") == 0)
            gui_window_switch_next (gui_current_window);
        else if (string_strcasecmp (argv[1], "up") == 0)
            gui_window_switch_up (gui_current_window);
        else if (string_strcasecmp (argv[1], "down") == 0)
            gui_window_switch_down (gui_current_window);
        else if (string_strcasecmp (argv[1], "left") == 0)
            gui_window_switch_left (gui_current_window);
        else if (string_strcasecmp (argv[1], "right") == 0)
            gui_window_switch_right (gui_current_window);
        else if (string_strcasecmp (argv[1], "scroll") == 0)
        {
            if (argc > 2)
                gui_window_scroll (gui_current_window, argv[2]);
        }
        else
        {
            gui_chat_printf (NULL,
                             _("%sError: unknown option for \"%s\" command"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             "window");
            return WEECHAT_RC_ERROR;
        }
    }
    return WEECHAT_RC_OK;
}

/*
 * command_init: init WeeChat commands (create hooks)
 */

void
command_init ()
{
    hook_command (NULL, "bar",
                  N_("manage bars"),
                  N_("[add name type position size separator item1,item2,...] "
                     "| [list]"),
                  N_("      add: add a new bar\n"
                     "     name: name of bar (must be unique)\n"
                     "     type: \"root\" (outside windows), \"window_active\" "
                     "(inside active window), or \"window_inactive\" (inside "
                     "each inactive window)\n"
                     " position: bottom, top, left or right\n"
                     "     size: size of bar (in chars)\n"
                     "separator: 1 for using separator (line), 0 or nothing "
                     "means no separator\n"
                     "item1,...: items for this bar\n"
                     "     list: list all bars"),
                  "list",
                  &command_bar, NULL);
    hook_command (NULL, "buffer",
                  N_("manage buffers"),
                  N_("[action [args] | number | [[server] [channel]]]"),
                  N_(" action: action to do:\n"
                     "  clear: clear buffer content (-all for all buffers, "
                     "number for a buffer, or nothing for current buffer)\n"
                     "   move: move buffer in the list (may be relative, for "
                     "example -1)\n"
                     "  close: close buffer\n"
                     "   list: list buffers (no parameter implies this list)\n"
                     " notify: set notify level for buffer (0=never, "
                     "1=highlight, 2=1+msg, 3=2+join/part)\n"
                     "         (when executed on server buffer, this sets "
                      "default notify level for whole server)\n"
                     " scroll: scroll in history (may be relative, and may "
                     "end by a letter: s=sec, m=min, h=hour, d=day, M=month, "
                     "y=year); if there is only letter, then scroll to "
                     "beginning of this item\n\n"
                     " number: jump to buffer by number\n"
                     "server,\n"
                     "channel: jump to buffer by server and/or channel name\n\n"
                     "Examples:\n"
                     "clear current buffer: /buffer clear\n"
                     "   clear all buffers: /buffer clear -all\n"
                     "         move buffer: /buffer move 5\n"
                     "        close buffer: /buffer close this is part msg\n"
                     "          set notify: /buffer notify 2\n"
                     "     scroll 1 day up: /buffer scroll 1d  ==  /buffer "
                     " scroll -1d  ==  /buffer scroll -24h\n"
                     " scroll to beginning\n"
                     "         of this day: /buffer scroll d\n"
                     "  scroll 15 min down: /buffer scroll +15m\n"
                     "   scroll 20 msgs up: /buffer scroll -20\n"
                     "    jump to #weechat: /buffer #weechat"),
                  "clear|move|close|list|notify|scroll|set|%b|%c %b|%c",
                  &command_buffer, NULL);
    hook_command (NULL, "builtin",
                  N_("launch WeeChat builtin command (do not look at commands "
                     "hooked)"),
                  N_("command"),
                  N_("command: command to execute (a '/' is automatically "
                     "added if not found at beginning of command)"),
                  "%w",
                  &command_builtin, NULL);
    hook_command (NULL, "help",
                  N_("display help about commands"),
                  N_("[command]"),
                  N_("command: name of a WeeChat or IRC command"),
                  "%w|%h",
                  &command_help, NULL);
    hook_command (NULL, "history",
                  N_("show buffer command history"),
                  N_("[clear | value]"),
                  N_("clear: clear history\n"
                     "value: number of history entries to show"),
                  "-clear",
                  &command_history, NULL);
    hook_command (NULL, "key",
                  N_("bind/unbind keys"),
                  N_("[key [function/command]] [unbind key] [functions] "
                     "[call function [\"args\"]] [reset -yes]"),
                  N_("      key: display or bind this key to an internal "
                     "function or a command (beginning by \"/\")\n"
                     "   unbind: unbind a key\n"
                     "functions: list internal functions for key bindings\n"
                     "     call: call a function by name (with optional "
                     "arguments)\n"
                     "    reset: restore bindings to the default values and "
                     "delete ALL personal bindings (use carefully!)"),
                  "unbind|functions|call|reset %k",
                  &command_key, NULL);
    hook_command (NULL, "plugin",
                  N_("list/load/unload plugins"),
                  N_("[list [name]] | [listfull [name]] | [load filename] | "
                     "[autoload] | [reload [name]] | [unload [name]]"),
                  N_("    list: list loaded plugins\n"
                     "listfull: list loaded plugins with detailed info for "
                     "each plugin\n"
                     "    load: load a plugin\n"
                     "autoload: autoload plugins in system or user directory\n"
                     "  reload: reload one plugin (if no name given, unload "
                     "all plugins, then autoload plugins)\n"
                     "  unload: unload one or all plugins\n\n"
                     "Without argument, /plugin command lists loaded plugins."),
                  "list|listfull|load|autoload|reload|unload %f|%p",
                  &command_plugin, NULL);
    hook_command (NULL, "quit",
                  N_("quit WeeChat"),
                  "", "",
                  "%q",
                  &command_quit, NULL);
    hook_command (NULL, "reload",
                  N_("reload configuration files from disk"),
                  N_("[file [file...]]"),
                  N_("file: configuration file to reload\n\n"
                     "Without argument, all files (WeeChat and plugins) are "
                     "reloaded."),
                  "%C|%*",
                  &command_reload, NULL);
    hook_command (NULL, "save",
                  N_("save configuration files to disk"),
                  N_("[file [file...]]"),
                  N_("file: configuration file to save\n\n"
                     "Without argument, all files (WeeChat and plugins) are "
                     "saved."),
                  "%C|%*",
                  &command_save, NULL);
    hook_command (NULL, "set",
                  N_("set config options"),
                  N_("[option [ = value]]"),
                  N_("option: name of an option (if name is full "
                     "and no value is given, then help is displayed on "
                     "option)\n"
                     " value: value for option\n\n"
                     "Option may be: servername.server_xxx where "
                     "\"servername\" is an internal server name and \"xxx\" "
                     "an option for this server."),
                  "%o = %v",
                  &command_set, NULL);
    hook_command (NULL, "setp",
                  N_("set plugin config options"),
                  N_("[option [ = value]]"),
                  N_("option: name of a plugin option\n"
                     " value: value for option\n\n"
                     "Option is format: plugin.option, example: "
                     "perl.myscript.item1"),
                  "%O = %V",
                  &command_setp, NULL);
    hook_command (NULL, "upgrade",
                  N_("upgrade WeeChat without disconnecting from servers"),
                  N_("[path_to_binary]"),
                  N_("path_to_binary: path to WeeChat binary (default is "
                     "current binary)\n\n"
                     "This command run again a WeeChat binary, so it should "
                     "have been compiled or installed with a package manager "
                     "before running this command."),
                  "%f",
                  &command_upgrade, NULL);
    hook_command (NULL, "uptime",
                  N_("show WeeChat uptime"),
                  N_("[-o]"),
                  N_("-o: send uptime on current channel as an IRC message"),
                  "-o",
                  &command_uptime, NULL);
    hook_command (NULL, "window",
                  N_("manage windows"),
                  N_("[list | -1 | +1 | b# | up | down | left | right | "
                     "splith [pct] | splitv [pct] | resize pct | "
                     "merge [all]]"),
                  N_("  list: list open windows (no parameter implies this "
                     "list)\n"
                     "    -1: jump to previous window\n"
                     "    +1: jump to next window\n"
                     "    b#: jump to next window displaying buffer number #\n"
                     "    up: switch to window above current one\n"
                     "  down: switch to window below current one\n"
                     "  left: switch to window on the left\n"
                     " right: switch to window on the right\n"
                     "splith: split current window horizontally\n"
                     "splitv: split current window vertically\n"
                     "resize: resize window size, new size is <pct> "
                     "pourcentage of parent window\n"
                     " merge: merge window with another (all = keep only one "
                     "window)\n\n"
                     "For splith and splitv, pct is a pourcentage which "
                     "represents size of new window, computed with current "
                     "window as size reference. For example 25 means create a "
                     "new window with size = current_size / 4"),
                  "list|-1|+1|up|down|left|right|splith|splitv|resize|merge all",
                  &command_window, NULL);
}

/*
 * command_print_stdout: print list of commands on standard output
 */

void
command_print_stdout ()
{
    struct t_hook *ptr_hook;

    for (ptr_hook = weechat_hooks[HOOK_TYPE_COMMAND]; ptr_hook;
         ptr_hook = ptr_hook->next_hook)
    {
        if (!ptr_hook->deleted
            && HOOK_COMMAND(ptr_hook, command)
            && HOOK_COMMAND(ptr_hook, command)[0]
            && !ptr_hook->plugin)
        {
            string_iconv_fprintf (stdout, "* %s",
                                  HOOK_COMMAND(ptr_hook, command));
            if (HOOK_COMMAND(ptr_hook, args)
                && HOOK_COMMAND(ptr_hook, args)[0])
            {
                string_iconv_fprintf (stdout, "  %s\n\n",
                                      _(HOOK_COMMAND(ptr_hook, args)));
            }
            else
            {
                string_iconv_fprintf (stdout, "\n\n");
            }
            string_iconv_fprintf (stdout, "%s\n\n",
                                  _(HOOK_COMMAND(ptr_hook, description)));
            if (HOOK_COMMAND(ptr_hook, args_description)
                && HOOK_COMMAND(ptr_hook, args_description)[0])
            {
                string_iconv_fprintf (stdout, "%s\n\n",
                                      _(HOOK_COMMAND(ptr_hook, args_description)));
            }
        }
    }
}
