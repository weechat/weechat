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
#include <time.h>

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
#include "../gui/gui-filter.h"
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
    int type, position;
    long number;
    char *error, *str_type, *pos_condition, str_size[16];
    struct t_gui_bar *ptr_bar;
    struct t_gui_bar_item *ptr_item;
    
    /* make C compiler happy */
    (void) data;
    (void) buffer;
    
    /* list of bars */
    if ((argc == 1)
        || ((argc == 2) && (string_strcasecmp (argv[1], "list") == 0)))
    {
        if (gui_bars)
        {
            gui_chat_printf (NULL, "");
            gui_chat_printf (NULL, _("List of bars:"));
            for (ptr_bar = gui_bars; ptr_bar;
                 ptr_bar = ptr_bar->next_bar)
            {
                snprintf (str_size, sizeof (str_size),
                          "%d", CONFIG_INTEGER(ptr_bar->size));
                gui_chat_printf (NULL,
                                 _("  %s%s%s: %s (cond: %s), %s, filling: %s, "
                                   "%s: %s"),
                                 GUI_COLOR(GUI_COLOR_CHAT_BUFFER),
                                 ptr_bar->name,
                                 GUI_COLOR(GUI_COLOR_CHAT),
                                 gui_bar_type_string[CONFIG_INTEGER(ptr_bar->type)],
                                 (CONFIG_STRING(ptr_bar->conditions)
                                  && CONFIG_STRING(ptr_bar->conditions)[0]) ?
                                 CONFIG_STRING(ptr_bar->conditions) : "-",
                                 gui_bar_position_string[CONFIG_INTEGER(ptr_bar->position)],
                                 gui_bar_filling_string[CONFIG_INTEGER(ptr_bar->filling)],
                                 ((CONFIG_INTEGER(ptr_bar->position) == GUI_BAR_POSITION_BOTTOM)
                                  || (CONFIG_INTEGER(ptr_bar->position) == GUI_BAR_POSITION_TOP)) ?
                                 _("height") : _("width"),
                                 (CONFIG_INTEGER(ptr_bar->size) == 0) ? _("auto") : str_size);
                gui_chat_printf (NULL,
                                 _("    priority: %d, fg: %s, bg: %s, items: %s%s (plugin: "
                                   "%s)"),
                                 CONFIG_INTEGER(ptr_bar->priority),
                                 gui_color_get_name (CONFIG_COLOR(ptr_bar->color_fg)),
                                 gui_color_get_name (CONFIG_COLOR(ptr_bar->color_bg)),
                                 (CONFIG_STRING(ptr_bar->items) && CONFIG_STRING(ptr_bar->items)[0]) ?
                                 CONFIG_STRING(ptr_bar->items) : "-",
                                 (CONFIG_INTEGER(ptr_bar->separator)) ?
                                 _(", with separator") : "",
                                 (ptr_bar->plugin) ? ptr_bar->plugin->name : "-");
                                 
            }
        }
        else
            gui_chat_printf (NULL, _("No bar defined"));
        
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
            if (gui_bar_new (NULL, argv[2], "0", str_type, pos_condition,
                             argv[4],
                             ((position == GUI_BAR_POSITION_LEFT)
                              || (position == GUI_BAR_POSITION_RIGHT)) ?
                             "vertical" : "horizontal",
                             argv[5], "0", "default", "default", argv[6],
                             argv_eol[7]))
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
                             "  %s[%s%d%s]%s (%s) %s / %s",
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
                        if (ptr_buffer)
                            gui_buffer_clear (ptr_buffer);
                    }
                }
            }
        }
        else
            gui_buffer_clear (buffer);
        
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
        gui_buffer_close (buffer, 1);
        gui_status_refresh_needed = 1;
        gui_buffer_ask_input_refresh (gui_current_window->buffer, 1);
        
        return WEECHAT_RC_OK;
    }

    /* display/change buffer notify */
    if (string_strcasecmp (argv[1], "notify") == 0)
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
            if (error && !error[0])
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
                                 _("New notify level for %s%s%s: "
                                   "%d %s"),
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

    /* jump to buffer by number or server/channel name */
    error = NULL;
    number = strtol (argv[1], &error, 10);
    if (error && !error[0])
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
 * command_filter: manage message filters
 */

int
command_filter (void *data, struct t_gui_buffer *buffer,
                int argc, char **argv, char **argv_eol)
{
    struct t_gui_filter *ptr_filter;
    int i;
    long number;
    char *error;
    
    /* make C compiler happy */
    (void) data;
    (void) buffer;
    
    if ((argc == 1)
        || ((argc == 2) && (string_strcasecmp (argv[1], "list") == 0)))
    {
        /* display all key bindings */
        gui_chat_printf (NULL, "");
        gui_chat_printf (NULL, "%s",
                         (gui_filters_enabled) ?
                         _("Filters are enabled") : _("Filters are disabled"));
        
        if (gui_filters)
        {
            gui_chat_printf (NULL, _("Message filters:"));
            i = 0;
            for (ptr_filter = gui_filters; ptr_filter;
                 ptr_filter = ptr_filter->next_filter)
            {
                i++;
                gui_chat_printf (NULL,
                                 _("  %s[%s%d%s]%s buffer: %s%s%s / tags: %s / "
                                   "regex: %s"),
                                 GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                 GUI_COLOR(GUI_COLOR_CHAT),
                                 i,
                                 GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                 GUI_COLOR(GUI_COLOR_CHAT),
                                 GUI_COLOR(GUI_COLOR_CHAT_BUFFER),
                                 ptr_filter->buffer,
                                 GUI_COLOR(GUI_COLOR_CHAT),
                                 ptr_filter->tags,
                                 ptr_filter->regex);
            }
        }
        else
            gui_chat_printf (NULL, _("No message filter defined"));
                             
        return WEECHAT_RC_OK;
    }
    
    /* enable filters */
    if (string_strcasecmp (argv[1], "enable") == 0)
    {
        if (!gui_filters_enabled)
        {
            gui_filter_enable ();
            gui_chat_printf (NULL, _("Filters enabled"));
        }
        return WEECHAT_RC_OK;
    }

    /* disable filters */
    if (string_strcasecmp (argv[1], "disable") == 0)
    {
        if (gui_filters_enabled)
        {
            gui_filter_disable ();
            gui_chat_printf (NULL, _("Filters disabled"));
        }
        return WEECHAT_RC_OK;
    }

    /* toggle filters on/off */
    if (string_strcasecmp (argv[1], "toggle") == 0)
    {
        if (gui_filters_enabled)
            gui_filter_disable ();
        else
            gui_filter_enable ();
        return WEECHAT_RC_OK;
    }
    
    /* add filter */
    if (string_strcasecmp (argv[1], "add") == 0)
    {
        if (argc < 5)
        {
            gui_chat_printf (NULL,
                             _("%sError: missing arguments for \"%s\" "
                               "command"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             "filter add");
            return WEECHAT_RC_ERROR;
        }
        if (gui_filter_search (argv[2], argv[3], argv_eol[4]))
        {
            gui_chat_printf (NULL,
                             _("%sError: filter already exists"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR]);
            return WEECHAT_RC_ERROR;
        }
        if ((strcmp (argv[3], "*") == 0) && (strcmp (argv_eol[4], "*") == 0))
        {
            gui_chat_printf (NULL,
                             _("%sError: you must specify at least tag(s) or "
                               "regex for filter"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR]);
            return WEECHAT_RC_ERROR;
        }
        
        gui_filter_new (argv[2], argv[3], argv_eol[4]);
        gui_chat_printf (NULL, _("Filter added"));
        
        return WEECHAT_RC_OK;
    }
    
    /* delete filter */
    if (string_strcasecmp (argv[1], "del") == 0)
    {
        if (argc < 3)
        {
            gui_chat_printf (NULL,
                             _("%sError: missing arguments for \"%s\" "
                               "command"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             "filter del");
            return WEECHAT_RC_ERROR;
        }
        error = NULL;
        number = strtol (argv[2], &error, 10);
        if (error && !error[0])
        {
            ptr_filter = gui_filter_search_by_number (number);
            if (ptr_filter)
            {
                gui_filter_free (ptr_filter);
                gui_chat_printf (NULL, _("Filter deleted"));
            }
            else
            {
                gui_chat_printf (NULL,
                                 _("%sError: filter not found"),
                                 gui_chat_prefix[GUI_CHAT_PREFIX_ERROR]);
                return WEECHAT_RC_ERROR;
            }
        }
        else
        {
            gui_chat_printf (NULL,
                             _("%sError: wrong filter number"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR]);
            return WEECHAT_RC_ERROR;
        }
        
        return WEECHAT_RC_OK;
    }
    
    gui_chat_printf (NULL,
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
        gui_chat_printf (NULL,
                         _("Option \"%s%s%s\": %s"),
                         GUI_COLOR(GUI_COLOR_CHAT_CHANNEL),
                         argv[1],
                         GUI_COLOR(GUI_COLOR_CHAT),
                         _(ptr_option->description));
        switch (ptr_option->type)
        {
            case CONFIG_OPTION_TYPE_BOOLEAN:
                gui_chat_printf (NULL, _("  type: boolean ('on' or 'off')"));
                gui_chat_printf (NULL, _("  value: %s%s%s (default: %s)"),
                                 GUI_COLOR(GUI_COLOR_CHAT_HOST),
                                 (CONFIG_BOOLEAN(ptr_option) == CONFIG_BOOLEAN_TRUE) ?
                                 "on" : "off",
                                 GUI_COLOR(GUI_COLOR_CHAT),
                                 (CONFIG_BOOLEAN_DEFAULT(ptr_option) == CONFIG_BOOLEAN_TRUE) ?
                                 "on" : "off");
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
                        gui_chat_printf (NULL,
                                         _("  type: string (%s)"),
                                         string);
                        gui_chat_printf (NULL,
                                         _("  value: '%s%s%s' (default: '%s')"),
                                         GUI_COLOR(GUI_COLOR_CHAT_HOST),
                                         ptr_option->string_values[CONFIG_INTEGER(ptr_option)],
                                         GUI_COLOR(GUI_COLOR_CHAT),
                                         ptr_option->string_values[CONFIG_INTEGER_DEFAULT(ptr_option)]);
                        free (string);
                    }
                }
                else
                {
                    gui_chat_printf (NULL, _("  type: integer (between %d and %d)"),
                                     ptr_option->min,
                                     ptr_option->max);
                    gui_chat_printf (NULL, _("  value: %s%d%s (default: %d)"),
                                     GUI_COLOR(GUI_COLOR_CHAT_HOST),
                                     CONFIG_INTEGER(ptr_option),
                                     GUI_COLOR(GUI_COLOR_CHAT),
                                     CONFIG_INTEGER_DEFAULT(ptr_option));
                }
                break;
            case CONFIG_OPTION_TYPE_STRING:
                switch (ptr_option->max)
                {
                    case 0:
                        gui_chat_printf (NULL, _("  type: string (any string)"));
                        break;
                    case 1:
                        gui_chat_printf (NULL, _("  type: char (any char)"));
                        break;
                    default:
                        gui_chat_printf (NULL, _("  type: string (limit: %d chars)"),
                                         ptr_option->max);
                        break;
                }
                gui_chat_printf (NULL,
                                 _("  value: '%s%s%s' (default: '%s')"),
                                 GUI_COLOR(GUI_COLOR_CHAT_HOST),
                                 CONFIG_STRING(ptr_option),
                                 GUI_COLOR(GUI_COLOR_CHAT),
                                 CONFIG_STRING_DEFAULT(ptr_option));
                break;
            case CONFIG_OPTION_TYPE_COLOR:
                gui_chat_printf (NULL,
                                 _("  type: color (values depend on GUI used)"));
                gui_chat_printf (NULL,
                                 _("  value: %s%s%s (default: %s)"),
                                 GUI_COLOR(GUI_COLOR_CHAT_HOST),
                                 gui_color_get_name (CONFIG_COLOR(ptr_option)),
                                 GUI_COLOR(GUI_COLOR_CHAT),
                                 gui_color_get_name (CONFIG_COLOR_DEFAULT(ptr_option)));
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
        else if (string_strcasecmp (argv[1], "infobar_clear") == 0)
            gui_input_infobar_clear ();
        else if (string_strcasecmp (argv[1], "grab_key") == 0)
            gui_input_grab_key ();
        else if (string_strcasecmp (argv[1], "scroll_unread") == 0)
            gui_input_scroll_unread ();
        else if (string_strcasecmp (argv[1], "set_unread") == 0)
            gui_input_set_unread ();
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
    quit_weechat = 1;
    
    return WEECHAT_RC_OK;
}

/*
 * command_reload_file: reload a configuration file
 */

void
command_reload_file (struct t_config_file *config_file)
{
    if ((int) (config_file->callback_reload)
        (config_file->callback_reload_data, config_file) == 0)
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
            if (ptr_config_file->callback_reload)
            {
                command_reload_file (ptr_config_file);
            }
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
                            char *message)
{
    char *color_name;
    
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
                             (option->value) ? CONFIG_STRING(option) : "",
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

/*
 * command_set_display_option_list: display list of options
 *                                  return: number of options displayed
 */

int
command_set_display_option_list (char *message, char *search)
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
    if ((argc >= 4) && (string_strcasecmp (argv[2], "=") == 0))
    {
        value = string_remove_quotes (argv_eol[3], "'\"");
        rc = config_file_option_set_with_string (argv[1],
                                                 (value) ? value : argv_eol[3]);
        if (value)
            free (value);
        switch (rc)
        {
            case 0:
                gui_chat_printf (NULL,
                                 _("%sError: failed to set option \"%s\""),
                                 gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                 argv[1]);
                return WEECHAT_RC_ERROR;
            case -1:
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
                                case -1: /* error */
                                    gui_chat_printf (NULL,
                                                     _("%sFailed to unset "
                                                       "option \"%s\""),
                                                     gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                                     option_full_name);
                                    break;
                                case 0: /* unset not needed on this option */
                                    break;
                                case 1: /* option reset */
                                    command_set_display_option (ptr_option,
                                                                _("Option reset: "));
                                    number_reset++;
                                    break;
                                case 2: /* option removed */
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
    filename = malloc (filename_length);
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
    gui_main_end (1);
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
    
    /* scroll topic left for current window */
    if (string_strcasecmp (argv[1], "scroll_topic_left") == 0)
    {
        gui_window_scroll_topic_left (gui_current_window);
        return WEECHAT_RC_OK;
    }
    
    /* scroll topic right for current window */
    if (string_strcasecmp (argv[1], "scroll_topic_right") == 0)
    {
        gui_window_scroll_topic_right (gui_current_window);
        return WEECHAT_RC_OK;
    }
    
    /* page up for nicklist in current window */
    if (string_strcasecmp (argv[1], "nicklist_page_up") == 0)
    {
        gui_window_nicklist_page_up (gui_current_window);
        return WEECHAT_RC_OK;
    }
    
    /* page down for nicklist in current window */
    if (string_strcasecmp (argv[1], "nicklist_page_down") == 0)
    {
        gui_window_nicklist_page_down (gui_current_window);
        return WEECHAT_RC_OK;
    }
    
    /* beginning of nicklist for current window */
    if (string_strcasecmp (argv[1], "nicklist_beginning") == 0)
    {
        gui_window_nicklist_beginning (gui_current_window);
        return WEECHAT_RC_OK;
    }
    
    /* end of nicklist for current window */
    if (string_strcasecmp (argv[1], "nicklist_end") == 0)
    {
        gui_window_nicklist_end (gui_current_window);
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
                gui_window_split_horiz (gui_current_window, number);
        }
        else
            gui_window_split_horiz (gui_current_window, 50);

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
                gui_window_split_vertic (gui_current_window, number);
        }
        else
            gui_window_split_vertic (gui_current_window, 50);

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
    
    return WEECHAT_RC_OK;
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
                     "separator item1,item2,...] | [del barname] | "
                     "[set barname name|priority|condition|position|filling|"
                     "size|separator|items value] | [list] | [listitems]"),
                  N_("      add: add a new bar\n"
                     "  barname: name of bar (must be unique)\n"
                     "     type:   root: outside windows),\n"
                     "           window: inside windows, with optional "
                     "conditions (see below)\n"
                     "cond1,...: condition(s) for displaying bar (only for "
                     "type \"window\"):\n"
                     "             active: on active window\n"
                     "           inactive: on inactive windows\n"
                     "           nicklist: on windows with nicklist\n"
                     "           without condition, bar is always displayed\n"
                     " position: bottom, top, left or right\n"
                     "  filling: horizontal or vertical\n"
                     "     size: size of bar (in chars)\n"
                     "separator: 1 for using separator (line), 0 or nothing "
                     "means no separator\n"
                     "item1,...: items for this bar\n"
                     "      del: delete a bar\n"
                     "      set: set a value for a bar property\n"
                     "     list: list all bars\n"
                     "listitems: list all bar items"),
                  "add|del|set|list|listitems %r name|priority|conditions|"
                  "position|filling|size|separator|items",
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
    hook_command (NULL, "command",
                  N_("launch explicit WeeChat or plugin command"),
                  N_("plugin command"),
                  N_(" plugin: plugin name ('weechat' for WeeChat internal "
                     "command)\n"
                     "command: command to execute (a '/' is automatically "
                     "added if not found at beginning of command)"),
                  "%p|weechat %P",
                  &command_command, NULL);
    hook_command (NULL, "filter",
                  N_("filter messages in buffers, to hide/show them according "
                     "to tags or regex"),
                  N_("[list] | [enable|disable|toggle] | "
                     "[add buffer tags regex] | "
                     "[del number]"),
                  N_("   list: list all filters\n"
                     " enable: enable filters (filters are enabled by "
                      "default)\n"
                     "disable: disable filters\n"
                     " toggle: toggle filters\n"
                     "    add: add a filter\n"
                     "    del: delete a filter\n"
                     " number: number of filter to delete (look at list to "
                     "find it)\n"
                     " buffer: buffer where filter is active: it may be "
                     "a name (category.name) or \"*\" for all buffers\n"
                     "   tags: comma separated list of tags, for "
                     "example: \"irc_join,irc_part,irc_quit\"\n"
                     "  regex: regular expression to search in "
                     "line (use \\t to separate prefix from message)"),
                  "list|enable|disable|toggle|add|del",
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
                  "hotlist_clear | infobar_clear | grab_key | scroll_unread | "
                  "set_unread | insert [args]",
                  _("This command is used by key bindings or plugins, it must "
                    "NOT be called by user"),
                  "",
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
                  "",
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
                  N_("option: name of an option\n"
                     " value: value for option"),
                  "%o = %v",
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
