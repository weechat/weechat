/*
 * Copyright (c) 2003-2009 by FlashCode <flashcode@flashtux.org>
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
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "weechat.h"
#include "wee-command.h"
#include "wee-config.h"
#include "wee-config-file.h"
#include "wee-debug.h"
#include "wee-hook.h"
#include "wee-input.h"
#include "wee-log.h"
#include "wee-proxy.h"
#include "wee-string.h"
#include "wee-upgrade.h"
#include "wee-utf8.h"
#include "wee-list.h"
#include "../gui/gui-bar.h"
#include "../gui/gui-bar-item.h"
#include "../gui/gui-buffer.h"
#include "../gui/gui-chat.h"
#include "../gui/gui-color.h"
#include "../gui/gui-filter.h"
#include "../gui/gui-history.h"
#include "../gui/gui-hotlist.h"
#include "../gui/gui-input.h"
#include "../gui/gui-keyboard.h"
#include "../gui/gui-layout.h"
#include "../gui/gui-main.h"
#include "../gui/gui-window.h"
#include "../plugins/plugin.h"
#include "../plugins/plugin-config.h"


/*
 * command_bar_list: list bars
 */

void
command_bar_list (int full)
{
    struct t_gui_bar *ptr_bar;
    char str_size[16];
    
    if (gui_bars)
    {
        gui_chat_printf (NULL, "");
        gui_chat_printf (NULL, _("List of bars:"));
        for (ptr_bar = gui_bars; ptr_bar;
             ptr_bar = ptr_bar->next_bar)
        {
            snprintf (str_size, sizeof (str_size),
                      "%d", CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_SIZE]));
            if (full)
            {
                gui_chat_printf (NULL,
                                 _("  %s%s%s: %s%s%s (cond: %s), %s, "
                                   "filling: %s(top/bottom)/%s(left/right), "
                                   "%s: %s"),
                                 GUI_COLOR(GUI_COLOR_CHAT_BUFFER),
                                 ptr_bar->name,
                                 GUI_COLOR(GUI_COLOR_CHAT),
                                 (CONFIG_BOOLEAN(ptr_bar->options[GUI_BAR_OPTION_HIDDEN])) ? _("(hidden)") : "",
                                 (CONFIG_BOOLEAN(ptr_bar->options[GUI_BAR_OPTION_HIDDEN])) ? " " : "",
                                 gui_bar_type_string[CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_TYPE])],
                                 (CONFIG_STRING(ptr_bar->options[GUI_BAR_OPTION_CONDITIONS])
                                  && CONFIG_STRING(ptr_bar->options[GUI_BAR_OPTION_CONDITIONS])[0]) ?
                                 CONFIG_STRING(ptr_bar->options[GUI_BAR_OPTION_CONDITIONS]) : "-",
                                 gui_bar_position_string[CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_POSITION])],
                                 gui_bar_filling_string[CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_FILLING_TOP_BOTTOM])],
                                 gui_bar_filling_string[CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_FILLING_LEFT_RIGHT])],
                                 ((CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_POSITION]) == GUI_BAR_POSITION_BOTTOM)
                                  || (CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_POSITION]) == GUI_BAR_POSITION_TOP)) ?
                                 _("height") : _("width"),
                                 (CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_SIZE]) == 0) ? _("auto") : str_size);
                gui_chat_printf (NULL,
                                 _("    priority: %d, fg: %s, bg: %s, items: %s%s"),
                                 CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_PRIORITY]),
                                 gui_color_get_name (CONFIG_COLOR(ptr_bar->options[GUI_BAR_OPTION_COLOR_FG])),
                                 gui_color_get_name (CONFIG_COLOR(ptr_bar->options[GUI_BAR_OPTION_COLOR_BG])),
                                 (CONFIG_STRING(ptr_bar->options[GUI_BAR_OPTION_ITEMS])
                                  && CONFIG_STRING(ptr_bar->options[GUI_BAR_OPTION_ITEMS])[0]) ?
                                 CONFIG_STRING(ptr_bar->options[GUI_BAR_OPTION_ITEMS]) : "-",
                                 (CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_SEPARATOR])) ?
                                 _(", with separator") : "");
            }
            else
            {
                gui_chat_printf (NULL,
                                 "  %s%s%s: %s%s%s, %s, %s: %s",
                                 GUI_COLOR(GUI_COLOR_CHAT_BUFFER),
                                 ptr_bar->name,
                                 GUI_COLOR(GUI_COLOR_CHAT),
                                 (CONFIG_BOOLEAN(ptr_bar->options[GUI_BAR_OPTION_HIDDEN])) ? _("(hidden)") : "",
                                 (CONFIG_BOOLEAN(ptr_bar->options[GUI_BAR_OPTION_HIDDEN])) ? " " : "",
                                 gui_bar_type_string[CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_TYPE])],
                                 gui_bar_position_string[CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_POSITION])],
                                 ((CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_POSITION]) == GUI_BAR_POSITION_BOTTOM)
                                  || (CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_POSITION]) == GUI_BAR_POSITION_TOP)) ?
                                 _("height") : _("width"),
                                 (CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_SIZE]) == 0) ? _("auto") : str_size);
            }
        }
    }
    else
        gui_chat_printf (NULL, _("No bar defined"));
}

/*
 * command_bar: manage bars
 */

int
command_bar (void *data, struct t_gui_buffer *buffer,
             int argc, char **argv, char **argv_eol)
{
    int type, position;
    long number;
    char *error, *str_type, *pos_condition;
    struct t_gui_bar *ptr_bar;
    struct t_gui_bar_item *ptr_item;
    struct t_gui_buffer *ptr_buffer;
    
    /* make C compiler happy */
    (void) data;
    
    /* list of bars */
    if ((argc == 1)
        || ((argc == 2) && (string_strcasecmp (argv[1], "list") == 0)))
    {
        command_bar_list (0);
        return WEECHAT_RC_OK;
    }
    
    /* full list of bars */
    if ((argc == 2) && (string_strcasecmp (argv[1], "listfull") == 0))
    {
        command_bar_list (1);
        return WEECHAT_RC_OK;
    }
    
    /* list of bar items */
    if ((argc == 1)
        || ((argc == 2) && (string_strcasecmp (argv[1], "listitems") == 0)))
    {
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
        
        return WEECHAT_RC_OK;
    }
    
    /* add a new bar */
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
        pos_condition = strchr (argv[3], ',');
        if (pos_condition)
        {
            str_type = string_strndup (argv[3], pos_condition - argv[3]);
            pos_condition++;
        }
        else
        {
            str_type = strdup (argv[3]);
        }
        if (!str_type)
        {
            gui_chat_printf (NULL,
                             _("%sNot enough memory"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR]);
            return WEECHAT_RC_ERROR;
        }
        type = gui_bar_search_type (str_type);
        if (type < 0)
        {
            gui_chat_printf (NULL,
                             _("%sError: wrong type \"%s\" for bar "
                               "\"%s\""),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             str_type, argv[2]);
            free (str_type);
            return WEECHAT_RC_ERROR;
        }
        position = gui_bar_search_position (argv[4]);
        if (position < 0)
        {
            gui_chat_printf (NULL,
                             _("%sError: wrong position \"%s\" for bar "
                               "\"%s\""),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             argv[4], argv[2]);
            free (str_type);
            return WEECHAT_RC_ERROR;
        }
        error = NULL;
        number = strtol (argv[5], &error, 10);
        if (error && !error[0])
        {
            /* create bar */
            if (gui_bar_new (argv[2], "0", "0", str_type,
                             (pos_condition) ? pos_condition : "",
                             argv[4],
                             "horizontal", "vertical",
                             argv[5], "0", "default", "default", "default",
                             argv[6], argv_eol[7]))
            {
                gui_chat_printf (NULL, _("Bar \"%s\" created"),
                                 argv[2]);
            }
            else
            {
                gui_chat_printf (NULL, _("%sError: failed to create bar "
                                         "\"%s\""),
                                 gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                 argv[2]);
            }
        }
        else
        {
            gui_chat_printf (NULL,
                             _("%sError: wrong size \"%s\" for bar "
                               "\"%s\""),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             argv[5], argv[2]);
            free (str_type);
            return WEECHAT_RC_ERROR;
        }
        free (str_type);

        return WEECHAT_RC_OK;
    }
    
    /* create default bars */
    if (string_strcasecmp (argv[1], "default") == 0)
    {
        gui_bar_create_default ();
        return WEECHAT_RC_OK;
    }
    
    /* delete a bar */
    if (string_strcasecmp (argv[1], "del") == 0)
    {
        if (argc < 3)
        {
            gui_chat_printf (NULL,
                             _("%sError: missing arguments for \"%s\" "
                               "command"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             "bar");
            return WEECHAT_RC_ERROR;
        }
        if (string_strcasecmp (argv[2], "-all") == 0)
        {
            gui_bar_free_all ();
            gui_chat_printf (NULL, _("All bars have been deleted"));
            gui_bar_create_default_input ();
        }
        else
        {
            ptr_bar = gui_bar_search (argv[2]);
            if (!ptr_bar)
            {
                gui_chat_printf (NULL,
                                 _("%sError: unknown bar \"%s\""),
                                 gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                 argv[2]);
                return WEECHAT_RC_ERROR;
            }
            gui_bar_free (ptr_bar);
            gui_chat_printf (NULL, _("Bar deleted"));
            gui_bar_create_default_input ();
        }
        
        return WEECHAT_RC_OK;
    }
    
    /* set a bar property */
    if (string_strcasecmp (argv[1], "set") == 0)
    {
        if (argc < 5)
        {
            gui_chat_printf (NULL,
                             _("%sError: missing arguments for \"%s\" "
                               "command"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             "bar");
            return WEECHAT_RC_ERROR;
        }
        ptr_bar = gui_bar_search (argv[2]);
        if (!ptr_bar)
        {
            gui_chat_printf (NULL,
                             _("%sError: unknown bar \"%s\""),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             argv[2]);
            return WEECHAT_RC_ERROR;
        }
        if (!gui_bar_set (ptr_bar, argv[3], argv_eol[4]))
        {
            gui_chat_printf (NULL,
                             _("%sError: unable to set option \"%s\" for "
                               "bar \"%s\""),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             argv[3], argv[2]);
            return WEECHAT_RC_ERROR;
        }
        
        return WEECHAT_RC_OK;
    }
    
    /* hide a bar */
    if (string_strcasecmp (argv[1], "hide") == 0)
    {
        if (argc < 3)
        {
            gui_chat_printf (NULL,
                             _("%sError: missing arguments for \"%s\" "
                               "command"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             "bar");
            return WEECHAT_RC_ERROR;
        }
        ptr_bar = gui_bar_search (argv[2]);
        if (!ptr_bar)
        {
            gui_chat_printf (NULL,
                             _("%sError: unknown bar \"%s\""),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             argv[2]);
            return WEECHAT_RC_ERROR;
        }
        if (!CONFIG_BOOLEAN(ptr_bar->options[GUI_BAR_OPTION_HIDDEN]))
        {
            if (gui_bar_set (ptr_bar, "hidden", "1"))
            {
                gui_chat_printf (NULL, _("Bar \"%s\" is now hidden"),
                                 ptr_bar->name);
            }
        }
        
        return WEECHAT_RC_OK;
    }

    /* show a bar */
    if (string_strcasecmp (argv[1], "show") == 0)
    {
        if (argc < 3)
        {
            gui_chat_printf (NULL,
                             _("%sError: missing arguments for \"%s\" "
                               "command"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             "bar");
            return WEECHAT_RC_ERROR;
        }
        ptr_bar = gui_bar_search (argv[2]);
        if (!ptr_bar)
        {
            gui_chat_printf (NULL,
                             _("%sError: unknown bar \"%s\""),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             argv[2]);
            return WEECHAT_RC_ERROR;
        }
        if (CONFIG_BOOLEAN(ptr_bar->options[GUI_BAR_OPTION_HIDDEN]))
        {
            if (gui_bar_set (ptr_bar, "hidden", "0"))
            {
                gui_chat_printf (NULL, _("Bar \"%s\" is now visible"),
                                 ptr_bar->name);
            }
        }
        
        return WEECHAT_RC_OK;
    }
    
    /* toggle a bar visible/hidden */
    if (string_strcasecmp (argv[1], "toggle") == 0)
    {
        if (argc < 3)
        {
            gui_chat_printf (NULL,
                             _("%sError: missing arguments for \"%s\" "
                               "command"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             "bar");
            return WEECHAT_RC_ERROR;
        }
        ptr_bar = gui_bar_search (argv[2]);
        if (!ptr_bar)
        {
            gui_chat_printf (NULL,
                             _("%sError: unknown bar \"%s\""),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             argv[2]);
            return WEECHAT_RC_ERROR;
        }
        gui_bar_set (ptr_bar, "hidden",
                     CONFIG_BOOLEAN(ptr_bar->options[GUI_BAR_OPTION_HIDDEN]) ? "0" : "1");
        
        return WEECHAT_RC_OK;
    }
    
    /* scroll in a bar */
    if (string_strcasecmp (argv[1], "scroll") == 0)
    {
        if (argc < 5)
        {
            gui_chat_printf (NULL,
                             _("%sError: missing arguments for \"%s\" "
                               "command"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             "bar");
            return WEECHAT_RC_ERROR;
        }
        ptr_bar = gui_bar_search (argv[2]);
        if (ptr_bar)
        {
            if (strcmp (argv[3], "*") == 0)
                ptr_buffer = buffer;
            else
                ptr_buffer = gui_buffer_search_by_name (NULL, argv[3]);
            if (!ptr_buffer)
            {
                gui_chat_printf (NULL,
                                 _("%sError: buffer not found for \"%s\" command"),
                                 gui_chat_prefix[GUI_CHAT_PREFIX_ERROR], "bar");
                return WEECHAT_RC_ERROR;
            }
            if (!gui_bar_scroll (ptr_bar, ptr_buffer, argv_eol[4]))
            {
                gui_chat_printf (NULL,
                                 _("%sError: unable to scroll bar \"%s\""),
                                 gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                 argv[2]);
                return WEECHAT_RC_ERROR;
            }
        }
        return WEECHAT_RC_OK;
    }
    
    gui_chat_printf (NULL,
                     _("%sError: unknown option for \"%s\" "
                       "command"),
                     gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                     "bar");
    return WEECHAT_RC_ERROR;
}

/*
 * command_buffer: manage buffers
 */

int
command_buffer (void *data, struct t_gui_buffer *buffer,
                int argc, char **argv, char **argv_eol)
{
    struct t_gui_buffer *ptr_buffer;
    struct t_gui_buffer_local_var *ptr_local_var;
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
                             "  %s[%s%d%s]%s (%s) %s%s%s (notify: %d)",
                             GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                             GUI_COLOR(GUI_COLOR_CHAT),
                             ptr_buffer->number,
                             GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                             GUI_COLOR(GUI_COLOR_CHAT),
                             plugin_get_name (ptr_buffer->plugin),
                             GUI_COLOR(GUI_COLOR_CHAT_BUFFER),
                             ptr_buffer->name,
                             GUI_COLOR(GUI_COLOR_CHAT),
                             ptr_buffer->notify);
        }
        
        return WEECHAT_RC_OK;
    }
    
    /* clear content of buffer */
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
                    if (error && !error[0])
                    {
                        ptr_buffer = gui_buffer_search_by_number (number);
                        if (ptr_buffer && (ptr_buffer->type == GUI_BUFFER_TYPE_FORMATED))
                            gui_buffer_clear (ptr_buffer);
                    }
                }
            }
        }
        else
        {
            if (buffer->type == GUI_BUFFER_TYPE_FORMATED)
                gui_buffer_clear (buffer);
        }
        
        return WEECHAT_RC_OK;
    }
    
    /* move buffer to another number in the list */
    if (string_strcasecmp (argv[1], "move") == 0)
    {
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
        if (error && !error[0])
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
        
        return WEECHAT_RC_OK;
    }
    
    /* close buffer */
    if (string_strcasecmp (argv[1], "close") == 0)
    {
        if (!buffer->plugin)
        {
            gui_chat_printf (NULL,
                             _("%sError: WeeChat main buffer can't be "
                               "closed"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR]);
            return WEECHAT_RC_ERROR;
        }
        gui_buffer_close (buffer);
        
        return WEECHAT_RC_OK;
    }

    /* display buffer notify */
    if (string_strcasecmp (argv[1], "notify") == 0)
    {
        /* display notify level for all buffers */
        gui_chat_printf (NULL, "");
        gui_chat_printf (NULL, _("Notify levels:"));
        for (ptr_buffer = gui_buffers; ptr_buffer;
             ptr_buffer = ptr_buffer->next_buffer)
        {
            gui_chat_printf (NULL,
                             "  %d.%s: %d (%s)",
                             ptr_buffer->number,
                             ptr_buffer->name,
                             ptr_buffer->notify,
                             gui_buffer_notify_string[ptr_buffer->notify]);
        }
        gui_chat_printf (NULL, "");
        
        return WEECHAT_RC_OK;
    }
    
    /* display local variables on buffer */
    if (string_strcasecmp (argv[1], "localvar") == 0)
    {
        if (buffer->local_variables)
        {
            gui_chat_printf (NULL, "");
            gui_chat_printf (NULL, _("Local variables for buffer \"%s\":"),
                             buffer->name);
            for (ptr_local_var = buffer->local_variables; ptr_local_var;
                 ptr_local_var = ptr_local_var->next_var)
            {
                gui_chat_printf (NULL,
                                 "  %s: \"%s\"",
                                 ptr_local_var->name,
                                 ptr_local_var->value);
            }
        }
        else
        {
            gui_chat_printf (NULL, _("No local variable defined for buffer \"%s\""),
                             buffer->name);
        }
        return WEECHAT_RC_OK;
    }
    
    /* set a property on buffer */
    if (string_strcasecmp (argv[1], "set") == 0)
    {
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
        
        return WEECHAT_RC_OK;
    }
    
    /* relative jump '-' */
    if (argv[1][0] == '-')
    {
        error = NULL;
        number = strtol (argv[1] + 1, &error, 10);
        if (error && !error[0])
        {
            target_buffer = buffer->number - (int) number;
            if (target_buffer < 1)
                target_buffer = (last_gui_buffer) ?
                    last_gui_buffer->number + target_buffer : 1;
            gui_buffer_switch_by_number (gui_current_window,
                                         target_buffer);
        }
        
        return WEECHAT_RC_OK;
    }
    
    /* relative jump '+' */
    if (argv[1][0] == '+')
    {
        error = NULL;
        number = strtol (argv[1] + 1, &error, 10);
        if (error && !error[0])
        {
            target_buffer = buffer->number + (int) number;
            if (last_gui_buffer && target_buffer > last_gui_buffer->number)
                target_buffer -= last_gui_buffer->number;
            gui_buffer_switch_by_number (gui_current_window,
                                         target_buffer);
        }
        
        return WEECHAT_RC_OK;
    }

    /* jump to buffer by number or name */
    error = NULL;
    number = strtol (argv[1], &error, 10);
    if (error && !error[0])
        gui_buffer_switch_by_number (gui_current_window,
                                     (int) number);
    else
    {
        ptr_buffer = NULL;
        ptr_buffer = gui_buffer_search_by_partial_name (NULL, argv_eol[1]);
        if (ptr_buffer)
        {
            gui_window_switch_to_buffer (gui_current_window, ptr_buffer, 1);
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * command_command: launch explicit WeeChat or plugin command
 */

int
command_command (void *data, struct t_gui_buffer *buffer,
                 int argc, char **argv, char **argv_eol)
{
    int length;
    char *command;
    struct t_weechat_plugin *ptr_plugin;
    
    /* make C compiler happy */
    (void) data;
    
    if (argc > 2)
    {
        ptr_plugin = NULL;
        if (string_strcasecmp (argv[1], "weechat") != 0)
        {
            ptr_plugin = plugin_search (argv[1]);
            if (!ptr_plugin)
            {
                gui_chat_printf (NULL, _("%sPlugin \"%s\" not found"),
                                 gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                 argv[1]);
                return WEECHAT_RC_ERROR;
            }
        }
        if (argv_eol[2][0] == '/')
        {
            input_exec_command (buffer, 0, ptr_plugin, argv_eol[2]);
        }
        else
        {
            length = strlen (argv_eol[2]) + 2;
            command = malloc (length);
            if (command)
            {
                snprintf (command, length, "/%s", argv_eol[2]);
                input_exec_command (buffer, 0, ptr_plugin, command);
                free (command);
            }
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * command_debug: control debug for core/plugins
 */

int
command_debug (void *data, struct t_gui_buffer *buffer,
               int argc, char **argv, char **argv_eol)
{
    struct t_config_option *ptr_option;
    struct t_weechat_plugin *ptr_plugin;
    
    /* make C compiler happy */
    (void) data;
    (void) argv_eol;
    
    if ((argc == 1)
        || ((argc == 2) && (string_strcasecmp (argv[1], "list") == 0)))
    {
        gui_chat_printf (NULL, "");
        gui_chat_printf (NULL, "Debug:");
        
        ptr_option = config_weechat_debug_get (PLUGIN_CORE);
        gui_chat_printf (NULL, "  %s: %d",
                         PLUGIN_CORE,
                         (ptr_option) ? CONFIG_INTEGER(ptr_option) : 0);
        for (ptr_plugin = weechat_plugins; ptr_plugin;
             ptr_plugin = ptr_plugin->next_plugin)
        {
            gui_chat_printf (NULL, "  %s: %d",
                             ptr_plugin->name,
                             ptr_plugin->debug);
        }
        return WEECHAT_RC_OK;
    }
    
    if (string_strcasecmp (argv[1], "dump") == 0)
    {
        hook_signal_send ("debug_dump", WEECHAT_HOOK_SIGNAL_STRING, NULL);
    }
    else if (string_strcasecmp (argv[1], "buffer") == 0)
    {
        gui_buffer_dump_hexa (buffer);
    }
    else if (string_strcasecmp (argv[1], "windows") == 0)
    {
        debug_windows_tree ();
    }
    else if (argc >= 3)
    {
        if (strcmp (argv[2], "0") == 0)
        {
            /* disable debug for a plugin */
            ptr_option = config_weechat_debug_get (argv[1]);
            if (ptr_option)
            {
                config_file_option_free (ptr_option);
                config_weechat_debug_set_all ();
                gui_chat_printf (NULL, _("Debug disabled for \"%s\""),
                                 argv[1]);
            }
        }
        else
        {
            /* set debug level for a plugin */
            if (config_weechat_debug_set (argv[1], argv[2]) != WEECHAT_CONFIG_OPTION_SET_ERROR)
            {
                ptr_option = config_weechat_debug_get (argv[1]);
                if (ptr_option)
                {
                    gui_chat_printf (NULL, "%s: \"%s\" => %d",
                                     "debug", argv[1],
                                     CONFIG_INTEGER(ptr_option));
                }
            }
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * command_filter_display: display one filter
 */

void
command_filter_display (struct t_gui_filter *filter)
{
    gui_chat_printf_date_tags (NULL, 0, GUI_FILTER_TAG_NO_FILTER,
                               _("  %s[%s%s%s]%s buffer: %s%s%s%s%s "
                                 "/ tags: %s / regex: %s %s"),
                               GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                               GUI_COLOR(GUI_COLOR_CHAT),
                               filter->name,
                               GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                               GUI_COLOR(GUI_COLOR_CHAT),
                               GUI_COLOR(GUI_COLOR_CHAT_BUFFER),
                               (filter->plugin_name) ? filter->plugin_name : "",
                               (filter->plugin_name) ? "." : "",
                               filter->buffer_name,
                               GUI_COLOR(GUI_COLOR_CHAT),
                               filter->tags,
                               filter->regex,
                               (filter->enabled) ?
                               "" : _("(disabled)"));
}

/*
 * command_filter: manage message filters
 */

int
command_filter (void *data, struct t_gui_buffer *buffer,
                int argc, char **argv, char **argv_eol)
{
    struct t_gui_filter *ptr_filter;
    
    /* make C compiler happy */
    (void) data;
    (void) buffer;
    
    if ((argc == 1)
        || ((argc == 2) && (string_strcasecmp (argv[1], "list") == 0)))
    {
        /* display all filters */
        gui_chat_printf_date_tags (NULL, 0, GUI_FILTER_TAG_NO_FILTER, "");
        gui_chat_printf_date_tags (NULL, 0, GUI_FILTER_TAG_NO_FILTER,
                                   "%s",
                                   (gui_filters_enabled) ?
                                   _("Message filtering enabled") :
                                   _("Message filtering disabled"));
        
        if (gui_filters)
        {
            gui_chat_printf_date_tags (NULL, 0, GUI_FILTER_TAG_NO_FILTER,
                                       _("Message filters:"));
            for (ptr_filter = gui_filters; ptr_filter;
                 ptr_filter = ptr_filter->next_filter)
            {
                command_filter_display (ptr_filter);
            }
        }
        else
        {
            gui_chat_printf_date_tags (NULL, 0, GUI_FILTER_TAG_NO_FILTER,
                                       _("No message filter defined"));
        }
        
        return WEECHAT_RC_OK;
    }
    
    /* enable global filtering or a filter */
    if (string_strcasecmp (argv[1], "enable") == 0)
    {
        if (argc > 2)
        {
            /* enable a filter */
            ptr_filter = gui_filter_search_by_name (argv[2]);
            if (ptr_filter)
            {
                if (!ptr_filter->enabled)
                {
                    gui_filter_enable (ptr_filter);
                    gui_chat_printf_date_tags (NULL, 0, GUI_FILTER_TAG_NO_FILTER,
                                               _("Filter \"%s\" enabled"),
                                               ptr_filter->name);
                }
            }
            else
            {
                gui_chat_printf_date_tags (NULL, 0, GUI_FILTER_TAG_NO_FILTER,
                                           _("%sError: filter \"%s\" not found"),
                                           gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                           argv[2]);
                return WEECHAT_RC_ERROR;
            }
        }
        else
        {
            /* enable global filtering */
            if (!gui_filters_enabled)
            {
                gui_filter_global_enable ();
                gui_chat_printf_date_tags (NULL, 0, GUI_FILTER_TAG_NO_FILTER,
                                           _("Message filtering enabled"));
            }
        }
        return WEECHAT_RC_OK;
    }

    /* disable global filtering or a filter */
    if (string_strcasecmp (argv[1], "disable") == 0)
    {
        if (argc > 2)
        {
            /* enable a filter */
            ptr_filter = gui_filter_search_by_name (argv[2]);
            if (ptr_filter)
            {
                if (ptr_filter->enabled)
                {
                    gui_filter_disable (ptr_filter);
                    gui_chat_printf_date_tags (NULL, 0, GUI_FILTER_TAG_NO_FILTER,
                                               _("Filter \"%s\" disabled"),
                                               ptr_filter->name);
                }
            }
            else
            {
                gui_chat_printf_date_tags (NULL, 0, GUI_FILTER_TAG_NO_FILTER,
                                           _("%sError: filter \"%s\" not found"),
                                           gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                           argv[2]);
                return WEECHAT_RC_ERROR;
            }
        }
        else
        {
            /* disable global filtering */
            if (gui_filters_enabled)
            {
                gui_filter_global_disable ();
                gui_chat_printf_date_tags (NULL, 0, GUI_FILTER_TAG_NO_FILTER,
                                           _("Message filtering disabled"));
            }
        }
        return WEECHAT_RC_OK;
    }
    
    /* toggle global filtering or a filter on/off */
    if (string_strcasecmp (argv[1], "toggle") == 0)
    {
        if (argc > 2)
        {
            /* toggle a filter */
            ptr_filter = gui_filter_search_by_name (argv[2]);
            if (ptr_filter)
            {
                if (ptr_filter->enabled)
                    gui_filter_disable (ptr_filter);
                else
                    gui_filter_enable (ptr_filter);
            }
            else
            {
                gui_chat_printf_date_tags (NULL, 0, GUI_FILTER_TAG_NO_FILTER,
                                           _("%sError: filter \"%s\" not found"),
                                           gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                           argv[2]);
                return WEECHAT_RC_ERROR;
            }
        }
        else
        {
            if (gui_filters_enabled)
                gui_filter_global_disable ();
            else
                gui_filter_global_enable ();
        }
        return WEECHAT_RC_OK;
    }
    
    /* add filter */
    if (string_strcasecmp (argv[1], "add") == 0)
    {
        if (argc < 6)
        {
            gui_chat_printf_date_tags (NULL, 0, GUI_FILTER_TAG_NO_FILTER,
                                       _("%sError: missing arguments for \"%s\" "
                                         "command"),
                                       gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                       "filter add");
            return WEECHAT_RC_ERROR;
        }
        if (gui_filter_search_by_name (argv[2]))
        {
            gui_chat_printf_date_tags (NULL, 0, GUI_FILTER_TAG_NO_FILTER,
                                       _("%sError: filter \"%s\" already exists"),
                                       gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                       argv[2]);
            return WEECHAT_RC_ERROR;
        }
        if ((strcmp (argv[4], "*") == 0) && (strcmp (argv_eol[5], "*") == 0))
        {
            gui_chat_printf_date_tags (NULL, 0, GUI_FILTER_TAG_NO_FILTER,
                                       _("%sError: you must specify at least "
                                         "tag(s) or regex for filter"),
                                       gui_chat_prefix[GUI_CHAT_PREFIX_ERROR]);
            return WEECHAT_RC_ERROR;
        }
        
        ptr_filter = gui_filter_new (1, argv[2], argv[3], argv[4], argv_eol[5]);
        
        if (ptr_filter)
        {
            gui_chat_printf (NULL, "");
            gui_chat_printf_date_tags (NULL, 0, GUI_FILTER_TAG_NO_FILTER,
                                       _("Filter \"%s\" added:"),
                                       argv[2]);
            command_filter_display (ptr_filter);
        }
        else
        {
            gui_chat_printf_date_tags (NULL, 0, GUI_FILTER_TAG_NO_FILTER,
                                       _("%sError adding filter"),
                                       gui_chat_prefix[GUI_CHAT_PREFIX_ERROR]);
        }
        
        return WEECHAT_RC_OK;
    }

    /* rename a filter */
    if (string_strcasecmp (argv[1], "rename") == 0)
    {
        if (argc < 4)
        {
            gui_chat_printf_date_tags (NULL, 0, GUI_FILTER_TAG_NO_FILTER,
                                       _("%sError: missing arguments for \"%s\" "
                                         "command"),
                                       gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                       "filter rename");
            return WEECHAT_RC_ERROR;
        }
        
        /* rename filter */
        ptr_filter = gui_filter_search_by_name (argv[2]);
        if (ptr_filter)
        {
            if (gui_filter_rename (ptr_filter, argv[3]))
            {
                gui_chat_printf_date_tags (NULL, 0, GUI_FILTER_TAG_NO_FILTER,
                                           _("Filter \"%s\" renamed to \"%s\""),
                                           argv[2], argv[3]);
            }
            else
            {
                gui_chat_printf_date_tags (NULL, 0, GUI_FILTER_TAG_NO_FILTER,
                                           _("%sError: unable to rename filter "
                                             "\"%s\" to \"%s\""),
                                           gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                           argv[2], argv[3]);
                return WEECHAT_RC_ERROR;
            }
        }
        else
        {
            gui_chat_printf_date_tags (NULL, 0, GUI_FILTER_TAG_NO_FILTER,
                                       _("%sError: filter \"%s\" not found"),
                                       gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                       argv[2]);
            return WEECHAT_RC_ERROR;
        }
        
        return WEECHAT_RC_OK;
    }
    
    /* delete filter */
    if (string_strcasecmp (argv[1], "del") == 0)
    {
        if (argc < 3)
        {
            gui_chat_printf_date_tags (NULL, 0, GUI_FILTER_TAG_NO_FILTER,
                                       _("%sError: missing arguments for \"%s\" "
                                         "command"),
                                       gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                       "filter del");
            return WEECHAT_RC_ERROR;
        }
        if (string_strcasecmp (argv[2], "-all") == 0)
        {
            if (gui_filters)
            {
                gui_filter_free_all ();
                gui_chat_printf_date_tags (NULL, 0, GUI_FILTER_TAG_NO_FILTER,
                                           _("All filters have been deleted"));
            }
            else
            {
                gui_chat_printf_date_tags (NULL, 0, GUI_FILTER_TAG_NO_FILTER,
                                           _("No message filter defined"));
            }
        }
        else
        {
            ptr_filter = gui_filter_search_by_name (argv[2]);
            if (ptr_filter)
            {
                gui_filter_free (ptr_filter);
                gui_chat_printf_date_tags (NULL, 0, GUI_FILTER_TAG_NO_FILTER,
                                           _("Filter \"%s\" deleted"),
                                           argv[2]);
            }
            else
            {
                gui_chat_printf_date_tags (NULL, 0, GUI_FILTER_TAG_NO_FILTER,
                                           _("%sError: filter \"%s\" not found"),
                                           gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                           argv[2]);
                    return WEECHAT_RC_ERROR;
            }
        }
        return WEECHAT_RC_OK;
    }
    
    gui_chat_printf_date_tags (NULL, 0, GUI_FILTER_TAG_NO_FILTER,
                               _("%sError: unknown option for \"%s\" "
                                 "command"),
                               gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                               "filter");
    return WEECHAT_RC_ERROR;

}

/*
 * command_help: display help about commands
 */

int
command_help (void *data, struct t_gui_buffer *buffer,
              int argc, char **argv, char **argv_eol)
{
    struct t_hook *ptr_hook;
    struct t_config_option *ptr_option;
    int i, length;
    char *string;
    
    /* make C compiler happy */
    (void) data;
    (void) buffer;
    (void) argv_eol;
    
    /* display help for all commands */
    if (argc == 1)
    {
        gui_chat_printf (NULL, "");
        gui_chat_printf (NULL,
                         /* TRANSLATORS: %s is "weechat" */
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
        
        return WEECHAT_RC_OK;
    }
    
    /* look for command */
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
    
    /* look for option */
    config_file_search_with_string (argv[1], NULL, NULL, &ptr_option, NULL);
    if (ptr_option)
    {
        gui_chat_printf (NULL, "");
        gui_chat_printf (NULL, _("Option \"%s%s%s\":"),
                         GUI_COLOR(GUI_COLOR_CHAT_CHANNEL),
                         argv[1],
                         GUI_COLOR(GUI_COLOR_CHAT));
        gui_chat_printf (NULL, "  %s: %s",
                         _("description"),
                         _(ptr_option->description));
        switch (ptr_option->type)
        {
            case CONFIG_OPTION_TYPE_BOOLEAN:
                gui_chat_printf (NULL, "  %s: %s",
                                 _("type"), _("boolean"));
                gui_chat_printf (NULL, "  %s: on, off",
                                 _("values"));
                if (ptr_option->default_value)
                {
                    gui_chat_printf (NULL, "  %s: %s",
                                     _("default value"),
                                     (CONFIG_BOOLEAN_DEFAULT(ptr_option) == CONFIG_BOOLEAN_TRUE) ?
                                     "on" : "off");
                }
                else
                {
                    gui_chat_printf (NULL, "  %s: %s",
                                     _("default value"),
                                     _("(undefined)"));
                }
                if (ptr_option->value)
                {
                    gui_chat_printf (NULL, "  %s: %s%s",
                                     _("current value"),
                                     GUI_COLOR(GUI_COLOR_CHAT_HOST),
                                     (CONFIG_BOOLEAN(ptr_option) == CONFIG_BOOLEAN_TRUE) ?
                                     "on" : "off");
                }
                else
                {
                    gui_chat_printf (NULL, "  %s: %s",
                                     _("current value"),
                                     _("(undefined)"));
                }
                break;
            case CONFIG_OPTION_TYPE_INTEGER:
                if (ptr_option->string_values)
                {
                    length = 0;
                    i = 0;
                    while (ptr_option->string_values[i])
                    {
                        length += strlen (ptr_option->string_values[i]) + 5;
                        i++;
                    }
                    string = malloc (length);
                    if (string)
                    {
                        string[0] = '\0';
                        i = 0;
                        while (ptr_option->string_values[i])
                        {
                            strcat (string, "'");
                            strcat (string, ptr_option->string_values[i]);
                            strcat (string, "'");
                            if (ptr_option->string_values[i + 1])
                                strcat (string, ", ");
                            i++;
                        }
                        gui_chat_printf (NULL, "  %s: %s",
                                         _("type"), _("string"));
                        gui_chat_printf (NULL, "  %s: %s",
                                         _("values"), string);
                        if (ptr_option->default_value)
                        {
                            gui_chat_printf (NULL, "  %s: \"%s\"",
                                             _("default value"),
                                             ptr_option->string_values[CONFIG_INTEGER_DEFAULT(ptr_option)]);
                        }
                        else
                        {
                            gui_chat_printf (NULL, "  %s: %s",
                                             _("default value"),
                                             _("(undefined)"));
                        }
                        if (ptr_option->value)
                        {
                            gui_chat_printf (NULL,
                                             "  %s: \"%s%s%s\"",
                                             _("current value"),
                                             GUI_COLOR(GUI_COLOR_CHAT_HOST),
                                             ptr_option->string_values[CONFIG_INTEGER(ptr_option)],
                                             GUI_COLOR(GUI_COLOR_CHAT));
                        }
                        else
                        {
                            gui_chat_printf (NULL,
                                             "  %s: %s",
                                             _("current value"),
                                             _("(undefined)"));
                        }
                        free (string);
                    }
                }
                else
                {
                    gui_chat_printf (NULL, "  %s: %s",
                                     _("type"), _("integer"));
                    gui_chat_printf (NULL, "  %s: %d .. %d",
                                     _("values"),
                                     ptr_option->min, ptr_option->max);
                    if (ptr_option->default_value)
                    {
                        gui_chat_printf (NULL, "  %s: %d",
                                         _("default value"),
                                         CONFIG_INTEGER_DEFAULT(ptr_option));
                    }
                    else
                    {
                        gui_chat_printf (NULL, "  %s: %s",
                                         _("default value"),
                                         _("(undefined)"));
                    }
                    if (ptr_option->value)
                    {
                        gui_chat_printf (NULL, "  %s: %s%d",
                                         _("current value"),
                                         GUI_COLOR(GUI_COLOR_CHAT_HOST),
                                         CONFIG_INTEGER(ptr_option));
                    }
                    else
                    {
                        gui_chat_printf (NULL, "  %s: %s",
                                         _("current value"),
                                         _("(undefined)"));
                    }
                }
                break;
            case CONFIG_OPTION_TYPE_STRING:
                switch (ptr_option->max)
                {
                    case 0:
                        gui_chat_printf (NULL, "  %s: %s",
                                         _("type"), _("string"));
                        gui_chat_printf (NULL, "  %s: %s",
                                         _("values"), _("any string"));
                        break;
                    case 1:
                        gui_chat_printf (NULL, "  %s: %s",
                                         _("type"), _("string"));
                        gui_chat_printf (NULL, "  %s: %s",
                                         _("values"), _("any char"));
                        break;
                    default:
                        gui_chat_printf (NULL, "  %s: %s",
                                         _("type"), _("string"));
                        gui_chat_printf (NULL, "  %s: %s (%s: %d)",
                                         _("values"), _("any string"),
                                         _("max chars"),
                                         ptr_option->max);
                        break;
                }
                if (ptr_option->default_value)
                {
                    gui_chat_printf (NULL, "  %s: \"%s\"",
                                     _("default value"),
                                     CONFIG_STRING_DEFAULT(ptr_option));
                }
                else
                {
                    gui_chat_printf (NULL, "  %s: %s",
                                     _("default value"),
                                     _("(undefined)"));
                }
                if (ptr_option->value)
                {
                    gui_chat_printf (NULL, "  %s: \"%s%s%s\"",
                                     _("current value"),
                                     GUI_COLOR(GUI_COLOR_CHAT_HOST),
                                     CONFIG_STRING(ptr_option),
                                     GUI_COLOR(GUI_COLOR_CHAT));
                }
                else
                {
                    gui_chat_printf (NULL, "  %s: %s",
                                     _("current value"),
                                     _("(undefined)"));
                }
                break;
            case CONFIG_OPTION_TYPE_COLOR:
                gui_chat_printf (NULL, "  %s: %s",
                                 _("type"), _("color"));
                gui_chat_printf (NULL, "  %s: %s",
                                 _("values"), _("a color name"));
                if (ptr_option->default_value)
                {
                    gui_chat_printf (NULL, "  %s: %s",
                                     _("default value"),
                                     gui_color_get_name (CONFIG_COLOR_DEFAULT(ptr_option)));
                }
                else
                {
                    gui_chat_printf (NULL, "  %s: %s",
                                     _("default value"),
                                     _("(undefined)"));
                }
                if (ptr_option->value)
                {
                    gui_chat_printf (NULL, "  %s: %s%s",
                                     _("current value"),
                                     GUI_COLOR(GUI_COLOR_CHAT_HOST),
                                     gui_color_get_name (CONFIG_COLOR(ptr_option)));
                }
                else
                {
                    gui_chat_printf (NULL, "  %s: %s",
                                     _("current value"),
                                     _("(undefined)"));
                }
                break;
            case CONFIG_NUM_OPTION_TYPES:
                break;
        }
        return WEECHAT_RC_OK;
    }
    
    gui_chat_printf (NULL,
                     _("%sNo help available, \"%s\" is not a command or an "
                       "option"),
                     gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                     argv[1]);
    
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
 * command_input: input actions (used by key bindings)
 */

int
command_input (void *data, struct t_gui_buffer *buffer,
               int argc, char **argv, char **argv_eol)
{
    /* make C compiler happy */
    (void) data;
    (void) buffer;
    
    if (argc > 1)
    {
        if (string_strcasecmp (argv[1], "return") == 0)
            gui_input_return ();
        else if (string_strcasecmp (argv[1], "complete_next") == 0)
            gui_input_complete_next ();
        else if (string_strcasecmp (argv[1], "complete_previous") == 0)
            gui_input_complete_previous ();
        else if (string_strcasecmp (argv[1], "search_text") == 0)
            gui_input_search_text ();
        else if (string_strcasecmp (argv[1], "delete_previous_char") == 0)
            gui_input_delete_previous_char ();
        else if (string_strcasecmp (argv[1], "delete_next_char") == 0)
            gui_input_delete_next_char ();
        else if (string_strcasecmp (argv[1], "delete_previous_word") == 0)
            gui_input_delete_previous_word ();
        else if (string_strcasecmp (argv[1], "delete_next_word") == 0)
            gui_input_delete_next_word ();
        else if (string_strcasecmp (argv[1], "delete_beginning_of_line") == 0)
            gui_input_delete_beginning_of_line ();
        else if (string_strcasecmp (argv[1], "delete_end_of_line") == 0)
            gui_input_delete_end_of_line ();
        else if (string_strcasecmp (argv[1], "delete_line") == 0)
            gui_input_delete_line ();
        else if (string_strcasecmp (argv[1], "clipboard_paste") == 0)
            gui_input_clipboard_paste ();
        else if (string_strcasecmp (argv[1], "transpose_chars") == 0)
            gui_input_transpose_chars ();
        else if (string_strcasecmp (argv[1], "move_beginning_of_line") == 0)
            gui_input_move_beginning_of_line ();
        else if (string_strcasecmp (argv[1], "move_end_of_line") == 0)
            gui_input_move_end_of_line ();
        else if (string_strcasecmp (argv[1], "move_previous_char") == 0)
            gui_input_move_previous_char ();
        else if (string_strcasecmp (argv[1], "move_next_char") == 0)
            gui_input_move_next_char ();
        else if (string_strcasecmp (argv[1], "move_previous_word") == 0)
            gui_input_move_previous_word ();
        else if (string_strcasecmp (argv[1], "move_next_word") == 0)
            gui_input_move_next_word ();
        else if (string_strcasecmp (argv[1], "history_previous") == 0)
            gui_input_history_previous ();
        else if (string_strcasecmp (argv[1], "history_next") == 0)
            gui_input_history_next ();
        else if (string_strcasecmp (argv[1], "history_global_previous") == 0)
            gui_input_history_global_previous ();
        else if (string_strcasecmp (argv[1], "history_global_next") == 0)
            gui_input_history_global_next ();
        else if (string_strcasecmp (argv[1], "jump_smart") == 0)
            gui_input_jump_smart ();
        else if (string_strcasecmp (argv[1], "jump_last_buffer") == 0)
            gui_input_jump_last_buffer ();
        else if (string_strcasecmp (argv[1], "jump_previous_buffer") == 0)
            gui_input_jump_previous_buffer ();
        else if (string_strcasecmp (argv[1], "hotlist_clear") == 0)
            gui_input_hotlist_clear ();
        else if (string_strcasecmp (argv[1], "grab_key") == 0)
            gui_input_grab_key ();
        else if (string_strcasecmp (argv[1], "scroll_unread") == 0)
            gui_input_scroll_unread ();
        else if (string_strcasecmp (argv[1], "set_unread") == 0)
            gui_input_set_unread ();
        else if (string_strcasecmp (argv[1], "set_unread_current_buffer") == 0)
            gui_input_set_unread_current_buffer ();
        else if (string_strcasecmp (argv[1], "insert") == 0)
        {
            if (argc > 2)
                gui_input_insert (argv_eol[2]);
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
    {
        gui_chat_printf (NULL,
                         _("New key binding: %s%s => %s%s"),
                         (expanded_name) ? expanded_name : key->key,
                         GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                         GUI_COLOR(GUI_COLOR_CHAT),
                         key->command);
    }
    else
    {
        gui_chat_printf (NULL, "  %20s%s => %s%s",
                         (expanded_name) ? expanded_name : key->key,
                         GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                         GUI_COLOR(GUI_COLOR_CHAT),
                         key->command);
    }
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
    char *internal_code;
    struct t_gui_key *ptr_key;
    
    /* make C compiler happy */
    (void) data;
    (void) buffer;
    
    /* display all key bindings */
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
    
    /* reset keys (only with "-yes", for security reason) */
    if (string_strcasecmp (argv[1], "reset") == 0)
    {
        if ((argc >= 3) && (string_strcasecmp (argv[2], "-yes") == 0))
        {
            gui_keyboard_free_all (&gui_keys, &last_gui_key);
            gui_keyboard_init ();
            gui_chat_printf (NULL,
                             _("Default key bindings restored"));
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
    
    /* unbind a key */
    if (string_strcasecmp (argv[1], "unbind") == 0)
    {
        if (argc >= 3)
        {
            if (gui_keyboard_unbind (NULL, argv[2]))
            {
                gui_chat_printf (NULL,
                                 _("Key \"%s\" unbound"),
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
    
    /* display a key */
    if (argc == 2)
    {
        ptr_key = NULL;
        internal_code = gui_keyboard_get_internal_code (argv[1]);
        if (internal_code)
            ptr_key = gui_keyboard_search (NULL, internal_code);
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
    ptr_key = gui_keyboard_bind (NULL, argv[1], argv_eol[2]);
    if (ptr_key)
    {
        command_key_display (ptr_key, 1);
    }
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
 * command_layout_display_tree: display tree of windows
 */

void
command_layout_display_tree (struct t_gui_layout_window *layout_window,
                             int indent)
{
    char format[128];
    
    if (layout_window)
    {
        if (layout_window->plugin_name)
        {
            /* leaf */
            snprintf (format, sizeof (format), "%%-%ds%s",
                      indent * 2,
                      _("leaf: id: %d, parent: %d, plugin: \"%s\", "
                        "buffer: \"%s\""));
            gui_chat_printf (NULL, format,
                             " ",
                             layout_window->internal_id,
                             (layout_window->parent_node) ?
                             layout_window->parent_node->internal_id : 0,
                             (layout_window->plugin_name) ?
                             layout_window->plugin_name : "-",
                             (layout_window->buffer_name) ?
                             layout_window->buffer_name : "-");
        }
        else
        {
            /* node */
            snprintf (format, sizeof (format), "%%-%ds%s",
                      indent * 2,
                      _("node: id: %d, parent: %d, child1: %d, child2: %d, "
                        "size: %d%% (%s)"));
            gui_chat_printf (NULL, format,
                             " ",
                             layout_window->internal_id,
                             (layout_window->parent_node) ?
                             layout_window->parent_node->internal_id : 0,
                             (layout_window->child1) ?
                             layout_window->child1->internal_id : 0,
                             (layout_window->child2) ?
                             layout_window->child2->internal_id : 0,
                             layout_window->split_pct,
                             (layout_window->split_horiz) ?
                             _("horizontal split") : _("vertical split"));
        }
        
        if (layout_window->child1)
            command_layout_display_tree (layout_window->child1, indent + 1);
        
        if (layout_window->child2)
            command_layout_display_tree (layout_window->child2, indent + 1);
    }
}

/*
 * command_layout: save/apply buffers/windows layout
 */

int
command_layout (void *data, struct t_gui_buffer *buffer,
                int argc, char **argv, char **argv_eol)
{
    struct t_gui_layout_buffer *ptr_layout_buffer;
    int flag_buffers, flag_windows;
    
    /* make C compiler happy */
    (void) data;
    (void) buffer;
    (void) argv_eol;
    
    /* display all key bindings */
    if (argc == 1)
    {
        if (gui_layout_buffers || gui_layout_windows)
        {
            if (gui_layout_buffers)
            {
                gui_chat_printf (NULL, "");
                gui_chat_printf (NULL, _("Saved layout for buffers:"));
                for (ptr_layout_buffer = gui_layout_buffers;
                     ptr_layout_buffer;
                     ptr_layout_buffer = ptr_layout_buffer->next_layout)
                {
                    gui_chat_printf (NULL, "  %d. %s / %s",
                                     ptr_layout_buffer->number,
                                     ptr_layout_buffer->plugin_name,
                                     ptr_layout_buffer->buffer_name);
                }
            }
            if (gui_layout_windows)
            {
                gui_chat_printf (NULL, "");
                gui_chat_printf (NULL, _("Saved layout for windows:"));
                command_layout_display_tree (gui_layout_windows, 1);
            }
        }
        else
            gui_chat_printf (NULL, _("No layout saved"));
        
        return WEECHAT_RC_OK;
    }
    
    flag_buffers = 1;
    flag_windows = 1;
    
    if (argc > 2)
    {
        if (string_strcasecmp (argv[2], "buffers") == 0)
            flag_windows = 0;
        else if (string_strcasecmp (argv[2], "windows") == 0)
            flag_buffers = 0;
    }
    
    /* save layout */
    if (string_strcasecmp (argv[1], "save") == 0)
    {
        if (flag_buffers)
        {
            gui_layout_buffer_save ();
            gui_chat_printf (NULL,
                             _("Layout saved for buffers (order of buffers)"));
        }
        if (flag_windows)
        {
            gui_layout_window_save ();
            gui_chat_printf (NULL,
                             _("Layout saved for windows (buffer displayed by "
                               "each window)"));
        }
        
        return WEECHAT_RC_OK;
    }
    
    /* apply layout */
    if (string_strcasecmp (argv[1], "apply") == 0)
    {
        if (flag_buffers)
            gui_layout_buffer_apply ();
        if (flag_windows)
            gui_layout_window_apply ();
        
        return WEECHAT_RC_OK;
    }

    /* reset layout */
    if (string_strcasecmp (argv[1], "reset") == 0)
    {
        if (flag_buffers)
        {
            gui_layout_buffer_reset ();
            gui_chat_printf (NULL,
                             _("Layout reset for buffers"));
        }
        if (flag_windows)
        {
            gui_layout_window_reset ();
            gui_chat_printf (NULL,
                             _("Layout reset for windows"));
        }
        
        return WEECHAT_RC_OK;
    }
    
    return WEECHAT_RC_OK;
}

/*
 * command_plugin_list: list loaded plugins
 */

void
command_plugin_list (const char *name, int full)
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
                                         _("      %d (flags: 0x%x:%s%s%s)"),
                                         HOOK_FD(ptr_hook, fd),
                                         HOOK_FD(ptr_hook, flags),
                                         (HOOK_FD(ptr_hook, flags) & HOOK_FD_FLAG_READ) ?
                                         _(" read") : "",
                                         (HOOK_FD(ptr_hook, flags) & HOOK_FD_FLAG_WRITE) ?
                                         _(" write") : "",
                                         (HOOK_FD(ptr_hook, flags) & HOOK_FD_FLAG_EXCEPTION) ?
                                         _(" exception") : "");
                    }
                }

                /* connect hooked */
                hook_found = 0;
                for (ptr_hook = weechat_hooks[HOOK_TYPE_CONNECT]; ptr_hook;
                     ptr_hook = ptr_hook->next_hook)
                {
                    if (!ptr_hook->deleted && (ptr_hook->plugin == ptr_plugin))
                    {
                        if (!hook_found)
                            gui_chat_printf (NULL,
                                             _("    connect hooked:"));
                        hook_found = 1;
                        gui_chat_printf (NULL,
                                         _("      socket: %d, address: %s, "
                                           "port: %d"),
                                         HOOK_CONNECT(ptr_hook, sock),
                                         HOOK_CONNECT(ptr_hook, address),
                                         HOOK_CONNECT(ptr_hook, port));
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
                                             _("      buffer: %s, message: \"%s\""),
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
                                         "      %s",
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
                                         HOOK_COMPLETION(ptr_hook, completion_item));
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
            /* list all plugins */
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
 * command_proxy_list: list proxies
 */

void
command_proxy_list ()
{
    struct t_proxy *ptr_proxy;
    
    if (weechat_proxies)
    {
        gui_chat_printf (NULL, "");
        gui_chat_printf (NULL, _("List of proxies:"));
        for (ptr_proxy = weechat_proxies; ptr_proxy;
             ptr_proxy = ptr_proxy->next_proxy)
        {
            gui_chat_printf (NULL,
                             _("  %s%s%s: %s, %s/%d (%s), username: %s, "
                               "password: %s"),
                             GUI_COLOR(GUI_COLOR_CHAT_BUFFER),
                             ptr_proxy->name,
                             GUI_COLOR(GUI_COLOR_CHAT),
                             proxy_type_string[CONFIG_INTEGER(ptr_proxy->options[PROXY_OPTION_TYPE])],
                             CONFIG_STRING(ptr_proxy->options[PROXY_OPTION_ADDRESS]),
                             CONFIG_INTEGER(ptr_proxy->options[PROXY_OPTION_PORT]),
                             (CONFIG_INTEGER(ptr_proxy->options[PROXY_OPTION_IPV6])) ? "IPv6" : "IPv4",
                             (CONFIG_STRING(ptr_proxy->options[PROXY_OPTION_USERNAME]) &&
                              CONFIG_STRING(ptr_proxy->options[PROXY_OPTION_USERNAME])[0]) ?
                             CONFIG_STRING(ptr_proxy->options[PROXY_OPTION_USERNAME]) : _("(none)"),
                             (CONFIG_STRING(ptr_proxy->options[PROXY_OPTION_PASSWORD]) &&
                              CONFIG_STRING(ptr_proxy->options[PROXY_OPTION_PASSWORD])[0]) ?
                             CONFIG_STRING(ptr_proxy->options[PROXY_OPTION_PASSWORD]) : _("(none)"));
        }
    }
    else
        gui_chat_printf (NULL, _("No proxy defined"));
}

/*
 * command_proxy: manage proxies
 */

int
command_proxy (void *data, struct t_gui_buffer *buffer,
               int argc, char **argv, char **argv_eol)
{
    int type;
    long number;
    char *error;
    struct t_proxy *ptr_proxy;
    
    /* make C compiler happy */
    (void) data;
    (void) buffer;
    
    /* list of bars */
    if ((argc == 1)
        || ((argc == 2) && (string_strcasecmp (argv[1], "list") == 0)))
    {
        command_proxy_list ();
        return WEECHAT_RC_OK;
    }
    
    /* add a new proxy */
    if (string_strcasecmp (argv[1], "add") == 0)
    {
        if (argc < 6)
        {
            gui_chat_printf (NULL,
                             _("%sError: missing arguments for \"%s\" "
                               "command"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             "proxy");
            return WEECHAT_RC_ERROR;
        }
        type = proxy_search_type (argv[3]);
        if (type < 0)
        {
            gui_chat_printf (NULL,
                             _("%sError: wrong type \"%s\" for proxy "
                               "\"%s\""),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             argv[3], argv[2]);
            return WEECHAT_RC_ERROR;
        }
        error = NULL;
        number = strtol (argv[5], &error, 10);
        if (error && !error[0])
        {
            /* create proxy */
            if (proxy_new (argv[2], argv[3], "off", argv[4], argv[5],
                           (argc >= 7) ? argv[6] : NULL,
                           (argc >= 8) ? argv_eol[7] : NULL))
            {
                gui_chat_printf (NULL, _("Proxy \"%s\" created"),
                                 argv[2]);
            }
            else
            {
                gui_chat_printf (NULL, _("%sError: failed to create proxy "
                                         "\"%s\""),
                                 gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                 argv[2]);
            }
        }
        else
        {
            gui_chat_printf (NULL,
                             _("%sError: wrong port \"%s\" for proxy "
                               "\"%s\""),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             argv[5], argv[2]);
            return WEECHAT_RC_ERROR;
        }
        
        return WEECHAT_RC_OK;
    }
    
    /* delete a proxy */
    if (string_strcasecmp (argv[1], "del") == 0)
    {
        if (argc < 3)
        {
            gui_chat_printf (NULL,
                             _("%sError: missing arguments for \"%s\" "
                               "command"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             "proxy");
            return WEECHAT_RC_ERROR;
        }
        if (string_strcasecmp (argv[2], "-all") == 0)
        {
            proxy_free_all ();
            gui_chat_printf (NULL, _("All proxies have been deleted"));
        }
        else
        {
            ptr_proxy = proxy_search (argv[2]);
            if (!ptr_proxy)
            {
                gui_chat_printf (NULL,
                                 _("%sError: unknown proxy \"%s\""),
                                 gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                 argv[2]);
                return WEECHAT_RC_ERROR;
            }
            proxy_free (ptr_proxy);
            gui_chat_printf (NULL, _("Proxy deleted"));
        }
        
        return WEECHAT_RC_OK;
    }
    
    /* set a proxy property */
    if (string_strcasecmp (argv[1], "set") == 0)
    {
        if (argc < 5)
        {
            gui_chat_printf (NULL,
                             _("%sError: missing arguments for \"%s\" "
                               "command"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             "proxy");
            return WEECHAT_RC_ERROR;
        }
        ptr_proxy = proxy_search (argv[2]);
        if (!ptr_proxy)
        {
            gui_chat_printf (NULL,
                             _("%sError: unknown proxy \"%s\""),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             argv[2]);
            return WEECHAT_RC_ERROR;
        }
        if (!proxy_set (ptr_proxy, argv[3], argv_eol[4]))
        {
            gui_chat_printf (NULL,
                             _("%sError: unable to set option \"%s\" for "
                               "proxy \"%s\""),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             argv[3], argv[2]);
            return WEECHAT_RC_ERROR;
        }
        
        return WEECHAT_RC_OK;
    }
    
    gui_chat_printf (NULL,
                     _("%sError: unknown option for \"%s\" "
                       "command"),
                     gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                     "proxy");
    return WEECHAT_RC_ERROR;
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
    (void) argv;
    
    /* send quit signal (used by plugins to disconnect from servers,..) */
    hook_signal_send ("quit",
                      WEECHAT_HOOK_SIGNAL_STRING,
                      (argc > 1) ? argv_eol[1] : NULL);
    
    /* force end of main loop */
    weechat_quit = 1;
    
    return WEECHAT_RC_OK;
}

/*
 * command_reload_file: reload a configuration file
 */

void
command_reload_file (struct t_config_file *config_file)
{
    int rc;
    
    if (config_file->callback_reload)
        rc = (int) (config_file->callback_reload)
            (config_file->callback_reload_data, config_file);
    else
        rc = config_file_reload (config_file);
    
    if (rc == 0)
    {
        gui_chat_printf (NULL,
                         _("Options reloaded from %s"),
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
                                 _("Unknown configuration file \"%s\""),
                                 argv[i]);
            }
        }
    }
    else
    {
        for (ptr_config_file = config_files; ptr_config_file;
             ptr_config_file = ptr_config_file->next_config)
        {
            command_reload_file (ptr_config_file);
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * command_save_file: save a configuration file to disk
 */

void
command_save_file (struct t_config_file *config_file)
{
    if (config_file_write (config_file) == 0)
    {
        gui_chat_printf (NULL,
                         _("Options saved to %s"),
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
 * command_save: save configuration files to disk
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
        /* save configuration files asked by user */
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
                                 _("Unknown configuration file \"%s\""),
                                 argv[i]);
            }
        }
    }
    else
    {
        /* save all configuration files */
        for (ptr_config_file = config_files; ptr_config_file;
             ptr_config_file = ptr_config_file->next_config)
        {
            command_save_file (ptr_config_file);
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * command_set_display_section: display configuration section
 */

void
command_set_display_section (struct t_config_file *config_file,
                             struct t_config_section *section)
{
    gui_chat_printf (NULL, "");
    gui_chat_printf (NULL, "%s[%s%s%s]%s (%s)",
                     GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                     GUI_COLOR(GUI_COLOR_CHAT_BUFFER),
                     section->name,
                     GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                     GUI_COLOR(GUI_COLOR_CHAT),
                     config_file->filename);
}
    
/*
 * command_set_display_option: display configuration option
 */

void
command_set_display_option (struct t_config_option *option,
                            const char *message)
{
    const char *color_name;

    if (option->value)
    {
        switch (option->type)
        {
            case CONFIG_OPTION_TYPE_BOOLEAN:
                gui_chat_printf (NULL, "%s%s.%s.%s%s = %s%s",
                                 (message) ? message : "  ",
                                 option->config_file->name,
                                 option->section->name,
                                 option->name,
                                 GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                 GUI_COLOR(GUI_COLOR_CHAT_HOST),
                                 (CONFIG_BOOLEAN(option) == CONFIG_BOOLEAN_TRUE) ?
                                  "on" : "off");
                break;
            case CONFIG_OPTION_TYPE_INTEGER:
                if (option->string_values)
                {
                    gui_chat_printf (NULL, "%s%s.%s.%s%s = %s%s",
                                     (message) ? message : "  ",
                                     option->config_file->name,
                                     option->section->name,
                                     option->name,
                                     GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                     GUI_COLOR(GUI_COLOR_CHAT_HOST),
                                     option->string_values[CONFIG_INTEGER(option)]);
                }
                else
                {
                    gui_chat_printf (NULL, "%s%s.%s.%s%s = %s%d",
                                     (message) ? message : "  ",
                                     option->config_file->name,
                                     option->section->name,
                                     option->name,
                                     GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                     GUI_COLOR(GUI_COLOR_CHAT_HOST),
                                     CONFIG_INTEGER(option));
                }
                break;
            case CONFIG_OPTION_TYPE_STRING:
                gui_chat_printf (NULL, "%s%s.%s.%s%s = \"%s%s%s\"",
                                 (message) ? message : "  ",
                                 option->config_file->name,
                                 option->section->name,
                                 option->name,
                                 GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                 GUI_COLOR(GUI_COLOR_CHAT_HOST),
                                 CONFIG_STRING(option),
                                 GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS));
                break;
            case CONFIG_OPTION_TYPE_COLOR:
                color_name = gui_color_get_name (CONFIG_COLOR(option));
                gui_chat_printf (NULL, "%s%s.%s.%s%s = %s%s",
                                 (message) ? message : "  ",
                                 option->config_file->name,
                                 option->section->name,
                                 option->name,
                                 GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                 GUI_COLOR(GUI_COLOR_CHAT_HOST),
                                 (color_name) ? color_name : _("(unknown)"));
                break;
            case CONFIG_NUM_OPTION_TYPES:
                /* make C compiler happy */
                break;
        }
    }
    else
    {
        gui_chat_printf (NULL, "%s%s.%s.%s",
                         (message) ? message : "  ",
                         option->config_file->name,
                         option->section->name,
                         option->name);
    }
}

/*
 * command_set_display_option_list: display list of options
 *                                  return: number of options displayed
 */

int
command_set_display_option_list (const char *message, const char *search)
{
    int number_found, section_displayed, length;
    struct t_config_file *ptr_config;
    struct t_config_section *ptr_section;
    struct t_config_option *ptr_option;
    char *option_full_name;
    
    number_found = 0;
    
    for (ptr_config = config_files; ptr_config;
         ptr_config = ptr_config->next_config)
    {
        for (ptr_section = ptr_config->sections; ptr_section;
             ptr_section = ptr_section->next_section)
        {
            section_displayed = 0;
            
            for (ptr_option = ptr_section->options; ptr_option;
                 ptr_option = ptr_option->next_option)
            {
                length = strlen (ptr_config->name) + 1
                    + strlen (ptr_section->name) + 1
                    + strlen (ptr_option->name) + 1;
                option_full_name = malloc (length);
                if (option_full_name)
                {
                    snprintf (option_full_name, length, "%s.%s.%s",
                              ptr_config->name, ptr_section->name,
                              ptr_option->name);
                    if ((!search) ||
                        (search && search[0]
                         && (string_match (option_full_name, search, 0))))
                    {
                        if (!section_displayed)
                        {
                            command_set_display_section (ptr_config,
                                                         ptr_section);
                            section_displayed = 1;
                        }
                        command_set_display_option (ptr_option, message);
                        number_found++;
                    }
                    free (option_full_name);
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
    int number_found, rc;
    struct t_config_option *ptr_option;
    
    /* make C compiler happy */
    (void) data;
    (void) buffer;
    
    /* display list of options */
    if (argc < 3)
    {
        number_found = 0;
        
        number_found += command_set_display_option_list (NULL,
                                                         (argc == 2) ?
                                                         argv[1] : NULL);
        
        if (number_found == 0)
        {
            if (argc == 2)
                gui_chat_printf (NULL,
                                 _("%sOption \"%s\" not found (tip: you can use "
                                   "\"*\" at beginning and/or end of option to "
                                   "see a sublist)"),
                                 gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
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
                                 NG_("%s%d%s configuration option found "
                                     "matching with \"%s\"",
                                     "%s%d%s configuration options found "
                                     "matching with \"%s\"",
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
    value =(string_strcasecmp (argv_eol[2], WEECHAT_CONFIG_OPTION_NULL) == 0) ?
        NULL : string_remove_quotes (argv_eol[2], "'\"");
    rc = config_file_option_set_with_string (argv[1], value);
    if (value)
        free (value);
    switch (rc)
    {
        case WEECHAT_CONFIG_OPTION_SET_ERROR:
            gui_chat_printf (NULL,
                             _("%sError: failed to set option \"%s\""),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             argv[1]);
            return WEECHAT_RC_ERROR;
        case WEECHAT_CONFIG_OPTION_SET_OPTION_NOT_FOUND:
            gui_chat_printf (NULL,
                             _("%sError: configuration option \"%s\" not "
                               "found"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             argv[1]);
            return WEECHAT_RC_ERROR;
        default:
            config_file_search_with_string (argv[1], NULL, NULL,
                                            &ptr_option, NULL);
            if (ptr_option)
                command_set_display_option (ptr_option,
                                            _("Option changed: "));
            else
                gui_chat_printf (NULL, _("Option changed"));
            break;
    }
    
    return WEECHAT_RC_OK;
}

/*
 * command_unset: unset/reset config options
 */

int
command_unset (void *data, struct t_gui_buffer *buffer,
               int argc, char **argv, char **argv_eol)
{
    struct t_config_file *ptr_config;
    struct t_config_section *ptr_section;
    struct t_config_option *ptr_option, *next_option;
    char *option_full_name;
    int length, number_reset, number_removed;
    
    /* make C compiler happy */
    (void) data;
    (void) buffer;
    (void) argv;
    
    number_reset = 0;
    number_removed = 0;
    
    if (argc >= 2)
    {
        for (ptr_config = config_files; ptr_config;
             ptr_config = ptr_config->next_config)
        {
            for (ptr_section = ptr_config->sections; ptr_section;
                 ptr_section = ptr_section->next_section)
            {
                ptr_option = ptr_section->options;
                while (ptr_option)
                {
                    next_option = ptr_option->next_option;
                    
                    length = strlen (ptr_config->name) + 1
                        + strlen (ptr_section->name) + 1
                        + strlen (ptr_option->name) + 1;
                    option_full_name = malloc (length);
                    if (option_full_name)
                    {
                        snprintf (option_full_name, length, "%s.%s.%s",
                                  ptr_config->name, ptr_section->name,
                                  ptr_option->name);
                        if (string_match (option_full_name, argv_eol[1], 0))
                        {
                            switch (config_file_option_unset (ptr_option))
                            {
                                case WEECHAT_CONFIG_OPTION_UNSET_ERROR:
                                    gui_chat_printf (NULL,
                                                     _("%sFailed to unset "
                                                       "option \"%s\""),
                                                     gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                                     option_full_name);
                                    break;
                                case WEECHAT_CONFIG_OPTION_UNSET_OK_NO_RESET:
                                    break;
                                case WEECHAT_CONFIG_OPTION_UNSET_OK_RESET:
                                    command_set_display_option (ptr_option,
                                                                _("Option reset: "));
                                    number_reset++;
                                    break;
                                case WEECHAT_CONFIG_OPTION_UNSET_OK_REMOVED:
                                    gui_chat_printf (NULL,
                                                     _("Option removed: %s"),
                                                     option_full_name);
                                    number_removed++;
                                    break;
                            }
                        }
                        free (option_full_name);
                    }
                    
                    ptr_option = next_option;
                }
            }
        }
        gui_chat_printf (NULL,
                         _("%d option(s) reset, %d option(s) removed"),
                         number_reset,
                         number_removed);
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
    char *ptr_binary;
    char *exec_args[7] = { NULL, "-a", "--dir", NULL, "--upgrade", NULL };
    struct stat stat_buf;
    int rc;
    
    /* make C compiler happy */
    (void) data;
    (void) buffer;
    (void) argv;
    
    if (argc > 1)
    {
        ptr_binary = string_replace (argv_eol[1], "~", getenv ("HOME"));

        if (ptr_binary)
        {
            /* check if weechat binary is here and executable by user */
            rc = stat (ptr_binary, &stat_buf);
            if ((rc != 0) || (!S_ISREG(stat_buf.st_mode)))
            {
                gui_chat_printf (NULL,
                                 _("%sCan't upgrade: WeeChat binary \"%s\" "
                                   "does not exist"),
                                 gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                 ptr_binary);
                free (ptr_binary);
                return WEECHAT_RC_ERROR;
            }
            if ((!(stat_buf.st_mode & S_IXUSR)) && (!(stat_buf.st_mode & S_IXGRP))
                && (!(stat_buf.st_mode & S_IXOTH)))
            {
                gui_chat_printf (NULL,
                                 _("%sCan't upgrade: WeeChat binary \"%s\" "
                                   "does not have execute permissions"),
                                 gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                 ptr_binary);
                free (ptr_binary);
                return WEECHAT_RC_ERROR;
            }
        }
    }
    else
        ptr_binary = strdup (weechat_argv0);
    
    if (!ptr_binary)
    {
        gui_chat_printf (NULL,
                         _("%sNot enough memory"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR]);
        return WEECHAT_RC_ERROR;
    }
    
    gui_chat_printf (NULL,
                     _("Upgrading WeeChat with binary file: \"%s\"..."),
                     ptr_binary);
    
    /* send "upgrade" signal to plugins */
    hook_signal_send ("upgrade", WEECHAT_HOOK_SIGNAL_STRING, NULL);
    
    if (!upgrade_weechat_save ())
    {
        gui_chat_printf (NULL,
                         _("%sError: unable to save session in file"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR]);
        free (ptr_binary);
        return WEECHAT_RC_ERROR;
    }
    
    exec_args[0] = ptr_binary;
    exec_args[3] = strdup (weechat_home);
    
    /* save layout, unload plugins, save config, then upgrade */
    gui_layout_save_on_exit ();
    plugin_end ();
    if (CONFIG_BOOLEAN(config_look_save_config_on_exit))
        (void) config_weechat_write ();
    gui_main_end (1);
    log_close ();
    
    execvp (exec_args[0], exec_args);
    
    /* this code should not be reached if execvp is ok */
    string_iconv_fprintf (stderr, "\n\n*****\n");
    string_iconv_fprintf (stderr,
                          _("***** Error: exec failed (program: \"%s\"), exiting WeeChat"),
                          exec_args[0]);
    string_iconv_fprintf (stderr, "\n*****\n\n");
    
    free (exec_args[0]);
    free (exec_args[3]);
    
    exit (EXIT_FAILURE);
    
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
        input_data (buffer, string);
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
        /* list all windows */        
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

        return WEECHAT_RC_OK;
    }
    
    /* page up in current window */
    if (string_strcasecmp (argv[1], "page_up") == 0)
    {
        gui_window_page_up (gui_current_window);
        return WEECHAT_RC_OK;
    }
    
    /* page down in current window */
    if (string_strcasecmp (argv[1], "page_down") == 0)
    {
        gui_window_page_down (gui_current_window);
        return WEECHAT_RC_OK;
    }
    
    /* scroll up current window */
    if (string_strcasecmp (argv[1], "scroll_up") == 0)
    {
        gui_window_scroll_up (gui_current_window);
        return WEECHAT_RC_OK;
    }
    
    /* scroll down current window */
    if (string_strcasecmp (argv[1], "scroll_down") == 0)
    {
        gui_window_scroll_down (gui_current_window);
        return WEECHAT_RC_OK;
    }
    
    /* scroll to top of current window */
    if (string_strcasecmp (argv[1], "scroll_top") == 0)
    {
        gui_window_scroll_top (gui_current_window);
        return WEECHAT_RC_OK;
    }
    
    /* scroll to bottom of current window */
    if (string_strcasecmp (argv[1], "scroll_bottom") == 0)
    {
        gui_window_scroll_bottom (gui_current_window);
        return WEECHAT_RC_OK;
    }
    
    /* scroll to previous highlight of current window */
    if (string_strcasecmp (argv[1], "scroll_previous_highlight") == 0)
    {
        gui_window_scroll_previous_highlight (gui_current_window);
        return WEECHAT_RC_OK;
    }
    
    /* scroll to next highlight of current window */
    if (string_strcasecmp (argv[1], "scroll_next_highlight") == 0)
    {
        gui_window_scroll_next_highlight (gui_current_window);
        return WEECHAT_RC_OK;
    }
    
    /* refresh screen */
    if (string_strcasecmp (argv[1], "refresh") == 0)
    {
        gui_window_refresh_needed = 1;
        return WEECHAT_RC_OK;
    }
    
    /* split window horizontally */
    if (string_strcasecmp (argv[1], "splith") == 0)
    {
        if (argc > 2)
        {
            error = NULL;
            number = strtol (argv[2], &error, 10);
            if (error && !error[0]
                && (number > 0) && (number < 100))
                gui_window_split_horizontal (gui_current_window, number);
        }
        else
            gui_window_split_horizontal (gui_current_window, 50);

        return WEECHAT_RC_OK;
    }
    
    /* split window vertically */
    if (string_strcasecmp (argv[1], "splitv") == 0)
    {
        if (argc > 2)
        {
            error = NULL;
            number = strtol (argv[2], &error, 10);
            if (error && !error[0]
                && (number > 0) && (number < 100))
                gui_window_split_vertical (gui_current_window, number);
        }
        else
            gui_window_split_vertical (gui_current_window, 50);

        return WEECHAT_RC_OK;
    }
    
    /* resize window */
    if (string_strcasecmp (argv[1], "resize") == 0)
    {
        if (argc > 2)
        {
            error = NULL;
            number = strtol (argv[2], &error, 10);
            if (error && !error[0]
                && (number > 0) && (number < 100))
                gui_window_resize (gui_current_window, number);
        }
        return WEECHAT_RC_OK;
    }
    
    /* merge windows */
    if (string_strcasecmp (argv[1], "merge") == 0)
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
        return WEECHAT_RC_OK;
    }
    
    /* jump to window by buffer number */
    if (string_strncasecmp (argv[1], "b", 1) == 0)
    {
        error = NULL;
        number = strtol (argv[1] + 1, &error, 10);
        if (error && !error[0])
            gui_window_switch_by_buffer (gui_current_window, number);
        return WEECHAT_RC_OK;
    }
    
    /* switch to previous window */
    if (string_strcasecmp (argv[1], "-1") == 0)
    {
        gui_window_switch_previous (gui_current_window);
        return WEECHAT_RC_OK;
    }
    
    /* switch to next window */
    if (string_strcasecmp (argv[1], "+1") == 0)
    {
        gui_window_switch_next (gui_current_window);
        return WEECHAT_RC_OK;
    }
    
    /* switch to window above */
    if (string_strcasecmp (argv[1], "up") == 0)
    {
        gui_window_switch_up (gui_current_window);
        return WEECHAT_RC_OK;
    }
    
    /* switch to window below */
    if (string_strcasecmp (argv[1], "down") == 0)
    {
        gui_window_switch_down (gui_current_window);
        return WEECHAT_RC_OK;
    }
    
    /* switch to window on the left */
    if (string_strcasecmp (argv[1], "left") == 0)
    {
        gui_window_switch_left (gui_current_window);
        return WEECHAT_RC_OK;
    }
    
    /* switch to window on the right */
    if (string_strcasecmp (argv[1], "right") == 0)
    {
        gui_window_switch_right (gui_current_window);
        return WEECHAT_RC_OK;
    }
    
    /* scroll in window */
    if (string_strcasecmp (argv[1], "scroll") == 0)
    {
        if (argc > 2)
            gui_window_scroll (gui_current_window, argv[2]);
        return WEECHAT_RC_OK;
    }
    
    gui_chat_printf (NULL,
                     _("%sError: unknown option for \"%s\" "
                       "command"),
                     gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                     "window");
    return WEECHAT_RC_ERROR;
}

/*
 * command_init: hook WeeChat commands
 */

void
command_init ()
{
    hook_command (NULL, "bar",
                  N_("manage bars"),
                  N_("[add barname type[,cond1,cond2,...] position size "
                     "separator item1,item2,...] | [default] | "
                     "[del barname|-all] | [set barname option value] | "
                     "[hide|show barname] | [scroll barname buffer "
                     "scroll_value] | [list] | [listfull] | [listitems]"),
                  N_("          add: add a new bar\n"
                     "      barname: name of bar (must be unique)\n"
                     "         type:   root: outside windows),\n"
                     "               window: inside windows, with optional "
                     "conditions (see below)\n"
                     "    cond1,...: condition(s) for displaying bar (only for "
                     "type \"window\"):\n"
                     "                 active: on active window\n"
                     "               inactive: on inactive windows\n"
                     "               nicklist: on windows with nicklist\n"
                     "               without condition, bar is always displayed\n"
                     "     position: bottom, top, left or right\n"
                     "      filling: horizontal, vertical, columns_horizontal "
                     "or columns_vertical\n"
                     "         size: size of bar (in chars)\n"
                     "    separator: 1 for using separator (line), 0 or nothing "
                     "means no separator\n"
                     "    item1,...: items for this bar (items can be separated "
                     "by comma (space between items) or \"+\" (glued items))\n"
                     "      default: create default bars\n"
                     "          del: delete a bar (or all bars with -all)\n"
                     "          set: set a value for a bar property\n"
                     "       option: option to change (for options list, look "
                     "at /set weechat.bar.<barname>.*)\n"
                     "        value: new value for option\n"
                     "         hide: hide a bar\n"
                     "         show: show an hidden bar\n"
                     "       toggle: hide/show a bar\n"
                     "       scroll: scroll bar up/down\n"
                     "       buffer: name of buffer to scroll ('*' "
                     "means current buffer, you should use '*' for root bars)\n"
                     " scroll_value: value for scroll: 'x' or 'y', followed by "
                     "'+', '-', 'b' (beginning) or 'e' (end), value (for +/-), "
                     "and optional %% (to scroll by %% of width/height, "
                     "otherwise value is number of chars)\n"
                     "         list: list all bars\n"
                     "     listfull: list all bars (verbose)\n"
                     "    listitems: list all bar items\n\n"
                     "Examples:\n"
                     "  create a bar with time, buffer number + name, and completion:\n"
                     "    /bar add mybar root bottom 1 0 [time],buffer_number+:+buffer_name,completion\n"
                     "  hide a bar:\n"
                     "    /bar hide mybar\n"
                     "  scroll nicklist 10 lines down on current buffer:\n"
                     "    /bar scroll nicklist * y+10\n"
                     "  scroll nicklist one page up on #weechat buffer:\n"
                     "    /bar scroll nicklist #weechat y-100%\n"
                     "  scroll to end of nicklist on current buffer:\n"
                     "    /bar scroll nicklist * ye"),
                  "add|default|del|set|hide|show|toggle|scroll|list|listfull|"
                  "listitems %r name|hidden|priority|conditions|position|"
                  "filling_top_bottom|filling_left_right|size|size_max|"
                  "color_fg|color_delim|color_bg|separator|items",
                  &command_bar, NULL);
    hook_command (NULL, "buffer",
                  N_("manage buffers"),
                  N_("[action [args] | number | [[server] [channel]]]"),
                  N_("  action: action to do:\n"
                     "   clear: clear buffer content (-all for all buffers, "
                     "number for a buffer, or nothing for current buffer)\n"
                     "    move: move buffer in the list (may be relative, for "
                     "example -1)\n"
                     "   close: close buffer\n"
                     "    list: list buffers (no parameter implies this list)\n"
                     "  notify: display notify levels for all opened buffers\n"
                     "localvar: display local variables for current buffer\n"
                     "  scroll: scroll in history (may be relative, and may "
                     "end by a letter: s=sec, m=min, h=hour, d=day, M=month, "
                     "y=year); if there is only letter, then scroll to "
                     "beginning of this item\n\n"
                     "  number: jump to buffer by number\n"
                     "server,\n"
                     " channel: jump to buffer by server and/or channel name\n\n"
                     "Examples:\n"
                     "clear current buffer: /buffer clear\n"
                     "   clear all buffers: /buffer clear -all\n"
                     "         move buffer: /buffer move 5\n"
                     "        close buffer: /buffer close this is part msg\n"
                     "     scroll 1 day up: /buffer scroll 1d  ==  /buffer "
                     "scroll -1d  ==  /buffer scroll -24h\n"
                     " scroll to beginning\n"
                     "         of this day: /buffer scroll d\n"
                     "  scroll 15 min down: /buffer scroll +15m\n"
                     "   scroll 20 msgs up: /buffer scroll -20\n"
                     "    jump to #weechat: /buffer #weechat"),
                  "clear|move|close|list|notify|localvar|scroll|set|%b %b",
                  &command_buffer, NULL);
    hook_command (NULL, "command",
                  N_("launch explicit WeeChat or plugin command"),
                  N_("plugin command"),
                  N_(" plugin: plugin name ('weechat' for WeeChat internal "
                     "command)\n"
                     "command: command to execute (a '/' is automatically "
                     "added if not found at beginning of command)"),
                  "%p|weechat %P",
                  &command_command, NULL);
    hook_command (NULL, "debug",
                  N_("control debug for core/plugins"),
                  N_("[list | plugin level | dump | buffer | windows]"),
                  N_(" plugin: name of plugin (\"core\" for WeeChat core)\n"
                     "  level: debug level for plugin (0 = disable debug)\n"
                     "   dump: save memory dump in WeeChat log file (same "
                     "dump is written when WeeChat crashes)\n"
                     " buffer: dump buffer content with hexadecimal values "
                     "in log file\n"
                     "windows: display windows tree\n"
                     "   text: send \"debug\" signal with \"text\" as "
                     "argument"),
                  "%p|core|list|dump|buffer|windows",
                  &command_debug, NULL);
    hook_command (NULL, "filter",
                  N_("filter messages in buffers, to hide/show them according "
                     "to tags or regex"),
                  N_("[list] | [enable|disable|toggle [name]] | "
                     "[add name plugin.buffer tags regex] | "
                     "[del name|-all]"),
                  N_("         list: list all filters\n"
                     "       enable: enable filters (filters are enabled by "
                      "default)\n"
                     "      disable: disable filters\n"
                     "       toggle: toggle filters\n"
                     "         name: filter name\n"
                     "          add: add a filter\n"
                     "          del: delete a filter\n"
                     "         -all: delete all filters\n"
                     "plugin.buffer: plugin and buffer where filter is active "
                     "(\"*\" for all buffers)\n"
                     "         tags: comma separated list of tags, for "
                     "example: \"irc_join,irc_part,irc_quit\"\n"
                     "        regex: regular expression to search in "
                     "line (use \\t to separate prefix from message)\n\n"
                     "Examples:\n"
                     "  use IRC smart filter for join/part/quit messages:\n"
                     "    /filter add irc_smart * irc_smart_filter *\n"
                     "  filter all IRC join/part/quit messages:\n"
                     "    /filter add joinquit * irc_join,irc_part,irc_quit *\n"
                     "  filter nick \"toto\" on IRC channel #weechat:\n"
                     "    /filter add toto irc.freenode.#weechat * toto\\t\n"
                     "  filter lines containing word \"spam\":\n"
                     "    /filter add filterspam * * spam\n"
                     "  filter lines containing \"weechat sucks\" on IRC "
                     "channel #weechat:\n"
                     "    /filter add sucks irc.freenode.#weechat * weechat sucks"),
                  "list|enable|disable|toggle|add|rename|del %F",
                  &command_filter, NULL);
    hook_command (NULL, "help",
                  N_("display help about commands and options"),
                  N_("[command | option]"),
                  N_("command: a command name\n"
                     " option: an option name (use /set to see list)"),
                  "%h|%o",
                  &command_help, NULL);
    hook_command (NULL, "history",
                  N_("show buffer command history"),
                  N_("[clear | value]"),
                  N_("clear: clear history\n"
                     "value: number of history entries to show"),
                  "clear",
                  &command_history, NULL);
    hook_command (NULL, "input",
                  N_("functions for command line"),
                  "return | complete_next | complete_previous | search_next | "
                  "delete_previous_char | delete_next_char | "
                  "delete_previous_word | delete_next_word | "
                  "delete_beginning_of_line | delete_end_of_line | "
                  "delete_line | clipboard_paste | transpose_chars | "
                  "move_beginning_of_line | move_end_of_line | "
                  "move_previous_char | move_next_char | move_previous_word | "
                  "move_next_word | history_previous | history_next | "
                  "history_global_previous | history_global_next | "
                  "jump_smart | jump_last_buffer | jump_previous_buffer | "
                  "hotlist_clear | grab_key | scroll_unread | set_unread | "
                  "set_unread_current_buffer | insert [args]",
                  _("This command is used by key bindings or plugins."),
                  "return|complete_next|complete_previous|search_next|"
                  "delete_previous_char|delete_next_char|"
                  "delete_previous_word|delete_next_word|"
                  "delete_beginning_of_line|delete_end_of_line|"
                  "delete_line|clipboard_paste|transpose_chars|"
                  "move_beginning_of_line|move_end_of_line|"
                  "move_previous_char|move_next_char|move_previous_word|"
                  "move_next_word|history_previous|history_next|"
                  "history_global_previous|history_global_next|"
                  "jump_smart|jump_last_buffer|jump_previous_buffer|"
                  "hotlist_clear|grab_key|scroll_unread|set_unread|"
                  "set_unread_current_buffer|insert",
                  &command_input, NULL);
    hook_command (NULL, "key",
                  N_("bind/unbind keys"),
                  N_("[key [command [args]]] | [unbind key] | [reset -yes]"),
                  N_("      key: display or bind this key to a command\n"
                     "   unbind: unbind a key\n"
                     "    reset: restore bindings to the default values and "
                     "delete ALL personal bindings (use carefully!)"),
                  "unbind|reset",
                  &command_key, NULL);
    hook_command (NULL, "layout",
                  N_("save/apply/reset layout for buffers and windows"),
                  N_("[[save | apply | reset] [buffers | windows]]"),
                  N_("   save: save current layout\n"
                     "  apply: apply saved layout\n"
                     "  reset: remove saved layout\n"
                     "buffers: save/apply only buffers (order of buffers)\n"
                     "windows: save/apply only windows (buffer displayed by "
                     "each window)\n\n"
                     "Without argument, this command displays saved layout."),
                  "save|apply|reset buffers|windows",
                  &command_layout, NULL);
    hook_command (NULL, "plugin",
                  N_("list/load/unload plugins"),
                  N_("[list [name]] | [listfull [name]] | [load filename] | "
                     "[autoload] | [reload [name]] | [unload [name]]"),
                  N_("    list: list loaded plugins\n"
                     "listfull: list loaded plugins (verbose)\n"
                     "    load: load a plugin\n"
                     "autoload: autoload plugins in system or user directory\n"
                     "  reload: reload one plugin (if no name given, unload "
                     "all plugins, then autoload plugins)\n"
                     "  unload: unload one or all plugins\n\n"
                     "Without argument, this command lists loaded plugins."),
                  "list|listfull|load|autoload|reload|unload %f|%p",
                  &command_plugin, NULL);
    hook_command (NULL, "proxy",
                  N_("manage proxies"),
                  N_("[add proxyname type address port [username "
                     "[password]]] | [del proxyname|-all] | [set "
                     "proxyname option value] | [list]"),
                  N_("          add: add a new proxy\n"
                     "    proxyname: name of proxy (must be unique)\n"
                     "         type: http, socks4 or socks5\n"
                     "      address: IP or hostname\n"
                     "         port: port\n"
                     "     username: username (optional)\n"
                     "     password: password (optional)\n"
                     "          del: delete a proxy (or all proxies with -all)\n"
                     "          set: set a value for a proxy property\n"
                     "       option: option to change (for options list, look "
                     "at /set weechat.proxy.<proxyname>.*)\n"
                     "        value: new value for option\n"
                     "         list: list all proxies\n\n"
                     "Examples:\n"
                     "  create a http proxy, running on local host, port 8888:\n"
                     "    /proxy add local http 127.0.0.1 8888\n"
                     "  create a http proxy using IPv6 protocol:\n"
                     "    /proxy add local http 127.0.0.1 8888\n"
                     "    /proxy set local ipv6 on\n"
                     "  create a socks5 proxy with username/password:\n"
                     "    /proxy add myproxy socks5 sample.host.org 3128 myuser mypass\n"
                     "  delete a proxy:\n"
                     "    /proxy del myproxy"),
                  "add|del|set|list %y name|type|ipv6|address|port|username|password",
                  &command_proxy, NULL);
    hook_command (NULL, "quit",
                  N_("quit WeeChat"),
                  "", "",
                  "",
                  &command_quit, NULL);
    hook_command (NULL, "reload",
                  N_("reload configuration files from disk"),
                  N_("[file [file...]]"),
                  N_("file: configuration file to reload\n\n"
                     "Without argument, all files (WeeChat and plugins) are "
                     "reloaded."),
                  "%c|%*",
                  &command_reload, NULL);
    hook_command (NULL, "save",
                  N_("save configuration files to disk"),
                  N_("[file [file...]]"),
                  N_("file: configuration file to save\n\n"
                     "Without argument, all files (WeeChat and plugins) are "
                     "saved."),
                  "%c|%*",
                  &command_save, NULL);
    hook_command (NULL, "set",
                  N_("set config options"),
                  N_("[option [value]]"),
                  N_("option: name of an option\n"
                     " value: new value for option\n\n"
                     "New value can be, according to variable type:\n"
                     "  boolean: on, off ou toggle\n"
                     "  integer: number, ++number ou --number"
                     "  string : any string (\"\" for empty string)\n"
                     "  color  : color name, ++number ou --number\n\n"
                     "For all types, you can use null to remove "
                     "option value (undefined value). This works only "
                     "for some special plugin variables."),
                  "%o %v",
                  &command_set, NULL);
    hook_command (NULL, "unset",
                  N_("unset/reset config options"),
                  N_("[option]"),
                  N_("option: name of an option (may begin or end with \"*\" "
                     "to mass-reset options, use carefully!)\n\n"
                     "According to option, it's reset (for standard options) "
                     "or removed (for optional settings, like server values)."),
                  "%o",
                  &command_unset, NULL);
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
                     "merge [all] | page_up | page_down | scroll | scroll_up | "
                     "scroll_down | scroll_top | scroll_bottom | "
                     "scroll_previous_highlight | scroll_next_highlight ]"),
                  N_("  list: list opened windows (no parameter implies this "
                     "list)\n"
                     "           -1: jump to previous window\n"
                     "           +1: jump to next window\n"
                     "           b#: jump to next window displaying buffer number #\n"
                     "           up: switch to window above current one\n"
                     "         down: switch to window below current one\n"
                     "         left: switch to window on the left\n"
                     "        right: switch to window on the right\n"
                     "       splith: split current window horizontally\n"
                     "       splitv: split current window vertically\n"
                     "       resize: resize window size, new size is <pct> "
                     "percentage of parent window\n"
                     "        merge: merge window with another (all = keep only one "
                     "window)\n\n"
                     "      page_up: scroll one page up\n"
                     "    page_down: scroll one page down\n"
                     "       scroll: scroll number of lines (+/-N) or with time: "
                     "s=seconds, m=minutes, h=hours, d=days, M=months, y=years\n"
                     "    scroll_up: scroll a few lines up\n"
                     "  scroll_down: scroll a few lines down\n"
                     "   scroll_top: scroll to top of buffer\n"
                     "scroll_bottom: scroll to bottom of buffer\n"
                     "scroll_previous_highlight: scroll to previous highlight\n"
                     "scroll_next_highlight: scroll to next highlight\n"
                     "      refresh: refresh screen\n\n"
                     "For splith and splitv, pct is a percentage which "
                     "represents size of new window, computed with current "
                     "window as size reference. For example 25 means create a "
                     "new window with size = current_size / 4\n\n"
                     "Examples:\n"
                     "  jump to window displaying buffer #1: /window b1"
                     "  scroll 2 lines up: /window scroll -2\n"
                     "  scroll 2 days up: /window scroll -2d\n"
                     "  scroll to beginning of current day: /window scroll -d"),
                     "list|-1|+1|up|down|left|right|splith|splitv|resize|merge|"
                  "page_up|page_down|scroll_up|scroll|scroll_down|scroll_top|"
                  "scroll_bottom|scroll_previous_highlight|"
                  "scroll_next_highlight|refresh all",
                  &command_window, NULL);
}

/*
 * command_startup: execute command at startup
 */

void
command_startup (int plugins_loaded)
{
    char *command, **commands, **ptr_cmd;
    struct t_gui_buffer *weechat_buffer;
    
    if (plugins_loaded)
        command = CONFIG_STRING(config_startup_command_after_plugins);
    else
        command = CONFIG_STRING(config_startup_command_before_plugins);
    
    if (command && command[0])
    {
        commands = string_split_command (command, ';');
        if (commands)
        {
            weechat_buffer = gui_buffer_search_main ();
            for (ptr_cmd = commands; *ptr_cmd; ptr_cmd++)
            {
                input_data (weechat_buffer, *ptr_cmd);
            }
            string_free_splitted_command (commands);
        }
    }
}
