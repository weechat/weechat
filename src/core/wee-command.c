/*
 * Copyright (C) 2003-2012 Sebastien Helleu <flashcode@flashtux.org>
 * Copyright (C) 2005-2006 Emmanuel Bouthenot <kolter@openics.org>
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
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * wee-command.c: WeeChat core commands
 */

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
#include "wee-hashtable.h"
#include "wee-hdata.h"
#include "wee-hook.h"
#include "wee-input.h"
#include "wee-list.h"
#include "wee-log.h"
#include "wee-proxy.h"
#include "wee-string.h"
#include "wee-upgrade.h"
#include "wee-utf8.h"
#include "wee-util.h"
#include "../gui/gui-bar.h"
#include "../gui/gui-bar-item.h"
#include "../gui/gui-buffer.h"
#include "../gui/gui-chat.h"
#include "../gui/gui-color.h"
#include "../gui/gui-cursor.h"
#include "../gui/gui-filter.h"
#include "../gui/gui-history.h"
#include "../gui/gui-hotlist.h"
#include "../gui/gui-input.h"
#include "../gui/gui-key.h"
#include "../gui/gui-layout.h"
#include "../gui/gui-main.h"
#include "../gui/gui-mouse.h"
#include "../gui/gui-window.h"
#include "../plugins/plugin.h"
#include "../plugins/plugin-config.h"


/*
 * command_away: toggle away status
 */

COMMAND_EMPTY(away)

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

COMMAND_CALLBACK(bar)
{
    int i, type, position, number;
    long value;
    char *error, *str_type, *pos_condition;
    struct t_gui_bar *ptr_bar;
    struct t_gui_bar_item *ptr_item;
    struct t_gui_window *ptr_window;

    /* make C compiler happy */
    (void) data;
    (void) buffer;

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
        COMMAND_MIN_ARGS(8, "bar add");
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
            return WEECHAT_RC_OK;
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
            return WEECHAT_RC_OK;
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
            return WEECHAT_RC_OK;
        }
        error = NULL;
        value = strtol (argv[5], &error, 10);
        (void) value;
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
            return WEECHAT_RC_OK;
        }
        free (str_type);

        return WEECHAT_RC_OK;
    }

    /* create default bars */
    if (string_strcasecmp (argv[1], "default") == 0)
    {
        if (argc > 2)
        {
            for (i = 2; i < argc; i++)
            {
                if (string_strcasecmp (argv[i], "input") == 0)
                    gui_bar_create_default_input ();
                else if (string_strcasecmp (argv[i], "title") == 0)
                    gui_bar_create_default_title ();
                else if (string_strcasecmp (argv[i], "status") == 0)
                    gui_bar_create_default_status ();
                else if (string_strcasecmp (argv[i], "nicklist") == 0)
                    gui_bar_create_default_nicklist ();
            }
        }
        else
            gui_bar_create_default ();
        return WEECHAT_RC_OK;
    }

    /* delete a bar */
    if (string_strcasecmp (argv[1], "del") == 0)
    {
        COMMAND_MIN_ARGS(3, "bar del");
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
                return WEECHAT_RC_OK;
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
        COMMAND_MIN_ARGS(5, "bar set");
        ptr_bar = gui_bar_search (argv[2]);
        if (!ptr_bar)
        {
            gui_chat_printf (NULL,
                             _("%sError: unknown bar \"%s\""),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             argv[2]);
            return WEECHAT_RC_OK;
        }
        if (!gui_bar_set (ptr_bar, argv[3], argv_eol[4]))
        {
            gui_chat_printf (NULL,
                             _("%sError: unable to set option \"%s\" for "
                               "bar \"%s\""),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             argv[3], argv[2]);
            return WEECHAT_RC_OK;
        }

        return WEECHAT_RC_OK;
    }

    /* hide a bar */
    if (string_strcasecmp (argv[1], "hide") == 0)
    {
        COMMAND_MIN_ARGS(3, "bar hide");
        ptr_bar = gui_bar_search (argv[2]);
        if (!ptr_bar)
        {
            gui_chat_printf (NULL,
                             _("%sError: unknown bar \"%s\""),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             argv[2]);
            return WEECHAT_RC_OK;
        }
        if (!CONFIG_BOOLEAN(ptr_bar->options[GUI_BAR_OPTION_HIDDEN]))
            gui_bar_set (ptr_bar, "hidden", "1");

        return WEECHAT_RC_OK;
    }

    /* show a bar */
    if (string_strcasecmp (argv[1], "show") == 0)
    {
        COMMAND_MIN_ARGS(3, "bar show");
        ptr_bar = gui_bar_search (argv[2]);
        if (!ptr_bar)
        {
            gui_chat_printf (NULL,
                             _("%sError: unknown bar \"%s\""),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             argv[2]);
            return WEECHAT_RC_OK;
        }
        if (CONFIG_BOOLEAN(ptr_bar->options[GUI_BAR_OPTION_HIDDEN]))
            gui_bar_set (ptr_bar, "hidden", "0");

        return WEECHAT_RC_OK;
    }

    /* toggle a bar visible/hidden */
    if (string_strcasecmp (argv[1], "toggle") == 0)
    {
        COMMAND_MIN_ARGS(3, "bar toggle");
        ptr_bar = gui_bar_search (argv[2]);
        if (!ptr_bar)
        {
            gui_chat_printf (NULL,
                             _("%sError: unknown bar \"%s\""),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             argv[2]);
            return WEECHAT_RC_OK;
        }
        gui_bar_set (ptr_bar, "hidden",
                     CONFIG_BOOLEAN(ptr_bar->options[GUI_BAR_OPTION_HIDDEN]) ? "0" : "1");

        return WEECHAT_RC_OK;
    }

    /* scroll in a bar */
    if (string_strcasecmp (argv[1], "scroll") == 0)
    {
        COMMAND_MIN_ARGS(5, "bar scroll");
        ptr_bar = gui_bar_search (argv[2]);
        if (ptr_bar)
        {
            if (strcmp (argv[3], "*") == 0)
                ptr_window = gui_current_window;
            else
            {
                ptr_window = NULL;
                error = NULL;
                number = (int)strtol (argv[3], &error, 10);
                if (error && !error[0])
                    ptr_window = gui_window_search_by_number (number);
            }
            if (!ptr_window)
            {
                gui_chat_printf (NULL,
                                 _("%sError: window not found for \"%s\" command"),
                                 gui_chat_prefix[GUI_CHAT_PREFIX_ERROR], "bar");
                return WEECHAT_RC_OK;
            }
            if (!gui_bar_scroll (ptr_bar, ptr_window, argv_eol[4]))
            {
                gui_chat_printf (NULL,
                                 _("%sError: unable to scroll bar \"%s\""),
                                 gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                 argv[2]);
                return WEECHAT_RC_OK;
            }
        }
        return WEECHAT_RC_OK;
    }

    gui_chat_printf (NULL,
                     _("%sError: unknown option for \"%s\" "
                       "command"),
                     gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                     "bar");
    return WEECHAT_RC_OK;
}

/*
 * command_buffer_display_localvar: display local variable for a buffer
 */

void
command_buffer_display_localvar (void *data,
                                 struct t_hashtable *hashtable,
                                 const void *key, const void *value)
{
    /* make C compiler happy */
    (void) data;
    (void) hashtable;

    if (key)
    {
        if (value)
        {
            gui_chat_printf (NULL,
                             "  %s: \"%s\"",
                             (const char *)key,
                             (const char *)value);
        }
        else
        {
            gui_chat_printf (NULL,
                             "  %s: (null)",
                             (const char *)key);
        }
    }
}

/*
 * command_buffer: manage buffers
 */

COMMAND_CALLBACK(buffer)
{
    struct t_gui_buffer *ptr_buffer, *ptr_buffer2, *weechat_buffer;
    long number, number1, number2;
    char *error, *value, *pos, *str_number1, *pos_number2;
    int i, target_buffer, error_main_buffer, num_buffers;

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
                             _("  %s[%s%d%s]%s (%s) %s%s%s (notify: %s)"),
                             GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                             GUI_COLOR(GUI_COLOR_CHAT),
                             ptr_buffer->number,
                             GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                             GUI_COLOR(GUI_COLOR_CHAT),
                             gui_buffer_get_plugin_name (ptr_buffer),
                             GUI_COLOR(GUI_COLOR_CHAT_BUFFER),
                             ptr_buffer->name,
                             GUI_COLOR(GUI_COLOR_CHAT),
                             gui_buffer_notify_string[ptr_buffer->notify]);
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
            else if (string_strcasecmp (argv[2], "-merged") == 0)
            {
                for (ptr_buffer = gui_buffers; ptr_buffer;
                     ptr_buffer = ptr_buffer->next_buffer)
                {
                    if ((ptr_buffer->number == buffer->number)
                        && (ptr_buffer->type == GUI_BUFFER_TYPE_FORMATTED))
                    {
                        gui_buffer_clear (ptr_buffer);
                    }
                }
            }
            else
            {
                for (i = 2; i < argc; i++)
                {
                    error = NULL;
                    number = strtol (argv[i], &error, 10);
                    if (error && !error[0])
                    {
                        for (ptr_buffer = gui_buffers; ptr_buffer;
                             ptr_buffer = ptr_buffer->next_buffer)
                        {
                            if ((ptr_buffer->number == number)
                                && (ptr_buffer->type == GUI_BUFFER_TYPE_FORMATTED))
                            {
                                gui_buffer_clear (ptr_buffer);
                            }
                        }
                    }
                    else
                    {
                        ptr_buffer = gui_buffer_search_by_full_name (argv[i]);
                        if (ptr_buffer)
                            gui_buffer_clear (ptr_buffer);
                    }
                }
            }
        }
        else
        {
            if (buffer->type == GUI_BUFFER_TYPE_FORMATTED)
                gui_buffer_clear (buffer);
        }

        return WEECHAT_RC_OK;
    }

    /* move buffer to another number in the list */
    if (string_strcasecmp (argv[1], "move") == 0)
    {
        COMMAND_MIN_ARGS(3, "buffer move");
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
            return WEECHAT_RC_OK;
        }

        return WEECHAT_RC_OK;
    }

    /* swap buffers */
    if (string_strcasecmp (argv[1], "swap") == 0)
    {
        COMMAND_MIN_ARGS(3, "buffer swap");

        ptr_buffer = NULL;
        ptr_buffer2 = NULL;

        /* first buffer for swap */
        number = strtol (argv[2], &error, 10);
        if (error && !error[0])
            ptr_buffer = gui_buffer_search_by_number (number);
        else
        {
            ptr_buffer = gui_buffer_search_by_full_name (argv[2]);
            if (!ptr_buffer)
                ptr_buffer = gui_buffer_search_by_partial_name (NULL, argv[2]);
        }

        /* second buffer for swap */
        if (argc > 3)
        {
            number = strtol (argv[3], &error, 10);
            if (error && !error[0])
                ptr_buffer2 = gui_buffer_search_by_number (number);
            else
            {
                ptr_buffer2 = gui_buffer_search_by_full_name (argv[3]);
                if (!ptr_buffer2)
                    ptr_buffer2 = gui_buffer_search_by_partial_name (NULL, argv[3]);
            }
        }
        else
            ptr_buffer2 = buffer;

        if (!ptr_buffer || !ptr_buffer2)
        {
            /* invalid buffer name/number */
            gui_chat_printf (NULL,
                             _("%sError: buffer not found"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR]);
            return WEECHAT_RC_OK;
        }

        gui_buffer_swap (ptr_buffer, ptr_buffer2);

        return WEECHAT_RC_OK;
    }

    /* merge buffer with another number in the list */
    if (string_strcasecmp (argv[1], "merge") == 0)
    {
        COMMAND_MIN_ARGS(3, "buffer merge");
        error = NULL;
        number = strtol (argv[2], &error, 10);
        if (error && !error[0])
        {
            ptr_buffer = gui_buffer_search_by_number ((int) number);
            if (ptr_buffer)
                gui_buffer_merge (buffer, ptr_buffer);
        }
        else
        {
            /* invalid number */
            gui_chat_printf (NULL,
                             _("%sError: incorrect buffer number"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR]);
            return WEECHAT_RC_OK;
        }

        return WEECHAT_RC_OK;
    }

    /* unmerge buffer */
    if (string_strcasecmp (argv[1], "unmerge") == 0)
    {
        number = -1;
        if (argc >= 3)
        {
            if (string_strcasecmp (argv[2], "-all") == 0)
            {
                gui_buffer_unmerge_all ();
                return WEECHAT_RC_OK;
            }
            else
            {
                error = NULL;
                number = strtol (argv[2], &error, 10);
                if (!error || error[0])
                {
                    /* invalid number */
                    gui_chat_printf (NULL,
                                     _("%sError: incorrect buffer number"),
                                     gui_chat_prefix[GUI_CHAT_PREFIX_ERROR]);
                    return WEECHAT_RC_OK;
                }
            }
        }
        gui_buffer_unmerge (buffer, (int) number);

        return WEECHAT_RC_OK;
    }

    /* close buffer */
    if (string_strcasecmp (argv[1], "close") == 0)
    {
        weechat_buffer = gui_buffer_search_main();
        if (argc < 3)
        {
            if (buffer == weechat_buffer)
            {
                gui_chat_printf (NULL,
                                 _("%sError: WeeChat main buffer can't be "
                                   "closed"),
                                 gui_chat_prefix[GUI_CHAT_PREFIX_ERROR]);
            }
            else
            {
                gui_buffer_close (buffer);
            }
        }
        else
        {
            if (isdigit ((unsigned char)argv_eol[2][0]))
            {
                number1 = -1;
                number2 = -1;
                pos = strchr (argv_eol[2], '-');
                if (pos)
                {
                    str_number1 = string_strndup (argv_eol[2],
                                                  pos - argv_eol[2]);
                    pos_number2 = pos + 1;
                }
                else
                {
                    str_number1 = strdup (argv_eol[2]);
                    pos_number2 = NULL;
                }
                if (str_number1)
                {
                    error = NULL;
                    number1 = strtol (str_number1, &error, 10);
                    if (error && !error[0])
                    {
                        if (pos_number2)
                        {
                            error = NULL;
                            number2 = strtol (pos_number2, &error, 10);
                            if (!error || error[0])
                                return WEECHAT_RC_ERROR;
                        }
                        else
                            number2 = number1;
                    }
                    else
                    {
                        number1 = -1;
                        number2 = -1;
                    }
                    free (str_number1);
                }
                if ((number1 >= 1) && (number2 >= 1) && (number2 >= number1))
                {
                    error_main_buffer = 0;
                    num_buffers = 0;
                    for (i = number2; i >= number1; i--)
                    {
                        for (ptr_buffer = last_gui_buffer; ptr_buffer;
                             ptr_buffer = ptr_buffer->prev_buffer)
                        {
                            if (ptr_buffer->number == i)
                            {
                                num_buffers++;
                                if (ptr_buffer == weechat_buffer)
                                {
                                    error_main_buffer = 1;
                                }
                                else
                                {
                                    gui_buffer_close (ptr_buffer);
                                }
                            }
                        }
                    }
                    /*
                     * display error for main buffer if it was the only
                     * buffer to close with matching number
                     */
                    if (error_main_buffer && (num_buffers <= 1))
                    {
                        gui_chat_printf (NULL,
                                         _("%sError: WeeChat main "
                                           "buffer can't be closed"),
                                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR]);
                    }
                }
            }
            else
            {
                ptr_buffer = gui_buffer_search_by_full_name (argv_eol[2]);
                if (ptr_buffer)
                    gui_buffer_close (ptr_buffer);
            }
        }

        return WEECHAT_RC_OK;
    }

    /* set notify level */
    if (string_strcasecmp (argv[1], "notify") == 0)
    {
        COMMAND_MIN_ARGS(3, "buffer notify");
        config_weechat_notify_set (buffer, argv_eol[2]);
        return WEECHAT_RC_OK;
    }

    /* display local variables on buffer */
    if (string_strcasecmp (argv[1], "localvar") == 0)
    {
        if (buffer->local_variables
            && (buffer->local_variables->items_count > 0))
        {
            gui_chat_printf (NULL, "");
            gui_chat_printf (NULL, _("Local variables for buffer \"%s\":"),
                             buffer->name);
            hashtable_map (buffer->local_variables,
                           &command_buffer_display_localvar, NULL);
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
        COMMAND_MIN_ARGS(4, "buffer set");
        value = string_remove_quotes (argv_eol[3], "'\"");
        gui_buffer_set (buffer, argv[2], (value) ? value : argv_eol[3]);
        if (value)
            free (value);

        return WEECHAT_RC_OK;
    }

    /* get a buffer property */
    if (string_strcasecmp (argv[1], "get") == 0)
    {
        COMMAND_MIN_ARGS(3, "buffer get");
        if (gui_buffer_property_in_list (gui_buffer_properties_get_integer,
                                         argv[2]))
        {
            gui_chat_printf (NULL, "%s%s%s: (int) %s = %d",
                             GUI_COLOR(GUI_COLOR_CHAT_BUFFER),
                             buffer->full_name,
                             GUI_COLOR(GUI_COLOR_CHAT),
                             argv[2],
                             gui_buffer_get_integer (buffer, argv[2]));
        }
        if (gui_buffer_property_in_list (gui_buffer_properties_get_string,
                                         argv[2]))
        {
            gui_chat_printf (NULL, "%s%s%s: (str) %s = %s",
                             GUI_COLOR(GUI_COLOR_CHAT_BUFFER),
                             buffer->full_name,
                             GUI_COLOR(GUI_COLOR_CHAT),
                             argv[2],
                             gui_buffer_get_string (buffer, argv[2]));
        }
        if (gui_buffer_property_in_list (gui_buffer_properties_get_pointer,
                                         argv[2]))
        {
            gui_chat_printf (NULL, "%s%s%s: (ptr) %s = 0x%lx",
                             GUI_COLOR(GUI_COLOR_CHAT_BUFFER),
                             buffer->full_name,
                             GUI_COLOR(GUI_COLOR_CHAT),
                             argv[2],
                             gui_buffer_get_pointer (buffer, argv[2]));
        }
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
        else
        {
            /* invalid number */
            gui_chat_printf (NULL,
                             _("%sError: incorrect buffer number"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR]);
            return WEECHAT_RC_OK;
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
        else
        {
            /* invalid number */
            gui_chat_printf (NULL,
                             _("%sError: incorrect buffer number"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR]);
            return WEECHAT_RC_OK;
        }

        return WEECHAT_RC_OK;
    }

    /* smart jump (jump to previous buffer for current number) */
    if (argv[1][0] == '*')
    {
        error = NULL;
        number = strtol (argv[1] + 1, &error, 10);
        if (error && !error[0])
        {
            /* buffer is currently displayed ? then jump to previous buffer */
            if ((number == buffer->number)
                && (CONFIG_BOOLEAN(config_look_jump_current_to_previous_buffer))
                && gui_buffers_visited)
            {
                gui_input_jump_previously_visited_buffer (buffer);
            }
            else
            {
                if (number != buffer->number)
                {
                    gui_buffer_switch_by_number (gui_current_window,
                                                 (int) number);
                }
            }
        }
        else
        {
            /* invalid number */
            gui_chat_printf (NULL,
                             _("%sError: incorrect buffer number"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR]);
            return WEECHAT_RC_OK;
        }

        return WEECHAT_RC_OK;
    }

    /* jump to buffer by number or name */
    error = NULL;
    number = strtol (argv[1], &error, 10);
    if (error && !error[0])
    {
        gui_buffer_switch_by_number (gui_current_window,
                                     (int) number);
        return WEECHAT_RC_OK;
    }
    else
    {
        ptr_buffer = gui_buffer_search_by_full_name (argv_eol[1]);
        if (!ptr_buffer)
            ptr_buffer = gui_buffer_search_by_partial_name (NULL, argv_eol[1]);
        if (ptr_buffer)
        {
            gui_window_switch_to_buffer (gui_current_window, ptr_buffer, 1);
            return WEECHAT_RC_OK;
        }
    }

    gui_chat_printf (NULL,
                     _("%sError: unknown option for \"%s\" "
                       "command"),
                     gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                     "buffer");

    return WEECHAT_RC_OK;
}

/*
 * command_color: define custom colors and display palette of colors
 */

COMMAND_CALLBACK(color)
{
    char *str_alias, *str_rgb, *pos, *error;
    char str_color[1024], str_command[1024];
    long number;
    int i;
    struct t_gui_color_palette *color_palette;

    /* make C compiler happy */
    (void) data;
    (void) argv_eol;

    if (argc == 1)
    {
        gui_color_buffer_open ();
        return WEECHAT_RC_OK;
    }

    /* add a color alias */
    if (string_strcasecmp (argv[1], "alias") == 0)
    {
        COMMAND_MIN_ARGS(4, "color alias");

        /* check color number */
        error = NULL;
        number = strtol (argv[2], &error, 10);
        if (error && !error[0])
        {
            if ((number < 0) || (number > gui_color_get_term_colors ()))
                number = -1;
        }
        else
        {
            number = -1;
        }
        if (number < 0)
        {
            gui_chat_printf (NULL,
                             _("%sInvalid color number \"%s\" (must be "
                               "between %d and %d)"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             argv[2], 0, gui_color_get_term_colors ());
            return WEECHAT_RC_OK;
        }

        /* check other arguments */
        str_alias = NULL;
        str_rgb = NULL;
        for (i = 3; i < argc; i++)
        {
            pos = strchr (argv[i], '/');
            if (pos)
                str_rgb = argv[i];
            else
                str_alias = argv[i];
        }
        str_color[0] = '\0';
        if (str_alias)
        {
            strcat (str_color, ";");
            strcat (str_color, str_alias);
        }
        if (str_rgb)
        {
            strcat (str_color, ";");
            strcat (str_color, str_rgb);
        }

        /* add color alias */
        snprintf (str_command, sizeof (str_command),
                  "/set weechat.palette.%d \"%s\"",
                  (int)number,
                  (str_color[0]) ? str_color + 1 : "");
        input_exec_command (buffer, 1, NULL, str_command);
        return WEECHAT_RC_OK;
    }

    /* delete a color alias */
    if (string_strcasecmp (argv[1], "unalias") == 0)
    {
        COMMAND_MIN_ARGS(3, "color unalias");

        /* check color number */
        error = NULL;
        number = strtol (argv[2], &error, 10);
        if (error && !error[0])
        {
            if ((number < 0) || (number > gui_color_get_term_colors ()))
                number = -1;
        }
        else
        {
            number = -1;
        }
        if (number < 0)
        {
            gui_chat_printf (NULL,
                             _("%sInvalid color number \"%s\" (must be "
                               "between %d and %d)"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             argv[2], 0, gui_color_get_term_colors ());
            return WEECHAT_RC_OK;
        }

        /* search color */
        color_palette = gui_color_palette_get ((int)number);
        if (!color_palette)
        {
            gui_chat_printf (NULL,
                             _("%sColor \"%s\" is not defined in palette"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             argv[2]);
            return WEECHAT_RC_OK;
        }

        /* delete color alias */
        snprintf (str_command, sizeof (str_command),
                  "/unset weechat.palette.%d",
                  (int)number);
        input_exec_command (buffer, 1, NULL, str_command);
        return WEECHAT_RC_OK;
    }

    /* reset color pairs */
    if (string_strcasecmp (argv[1], "reset") == 0)
    {
        gui_color_reset_pairs ();
        return WEECHAT_RC_OK;
    }

    /* switch WeeChat/terminal colors */
    if (string_strcasecmp (argv[1], "switch") == 0)
    {
        gui_color_switch_colors ();
        return WEECHAT_RC_OK;
    }

    return WEECHAT_RC_OK;
}

/*
 * command_command: launch explicit WeeChat or plugin command
 */

COMMAND_CALLBACK(command)
{
    int length;
    char *command;
    struct t_weechat_plugin *ptr_plugin;

    /* make C compiler happy */
    (void) data;

    if (argc > 2)
    {
        ptr_plugin = NULL;
        if (string_strcasecmp (argv[1], PLUGIN_CORE) != 0)
        {
            ptr_plugin = plugin_search (argv[1]);
            if (!ptr_plugin)
            {
                gui_chat_printf (NULL, _("%sPlugin \"%s\" not found"),
                                 gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                 argv[1]);
                return WEECHAT_RC_OK;
            }
        }
        if (string_is_command_char (argv_eol[2]))
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
 * command_cursor: free movement of cursor on screen
 */

COMMAND_CALLBACK(cursor)
{
    char *pos, *str_x, *error;
    int x, y;

    /* make C compiler happy */
    (void) data;
    (void) buffer;
    (void) argv_eol;

    if (argc == 1)
    {
        gui_cursor_mode_toggle ();
        return WEECHAT_RC_OK;
    }

    if (string_strcasecmp (argv[1], "go") == 0)
    {
        if (argc > 2)
        {
            pos = strchr (argv[2], ',');
            if (pos)
            {
                str_x = string_strndup (argv[2], pos - argv[2]);
                pos++;
                if (str_x)
                {
                    error = NULL;
                    x = (int) strtol (str_x, &error, 10);
                    if (error && !error[0])
                    {
                        error = NULL;
                        y = (int) strtol (pos, &error, 10);
                        if (error && !error[0])
                        {
                            gui_cursor_move_xy (x, y);
                        }
                    }
                    free (str_x);
                }
            }
            else
                gui_cursor_move_area (argv[2]);
        }
        return WEECHAT_RC_OK;
    }

    if (string_strcasecmp (argv[1], "move") == 0)
    {
        if (argc > 2)
        {
            if (string_strcasecmp (argv[2], "up") == 0)
                gui_cursor_move_add_xy (0, -1);
            else if (string_strcasecmp (argv[2], "down") == 0)
                gui_cursor_move_add_xy (0, 1);
            else if (string_strcasecmp (argv[2], "left") == 0)
                gui_cursor_move_add_xy (-1, 0);
            else if (string_strcasecmp (argv[2], "right") == 0)
                gui_cursor_move_add_xy (1, 0);
            else if (string_strcasecmp (argv[2], "area_up") == 0)
                gui_cursor_move_area_add_xy (0, -1);
            else if (string_strcasecmp (argv[2], "area_down") == 0)
                gui_cursor_move_area_add_xy (0, 1);
            else if (string_strcasecmp (argv[2], "area_left") == 0)
                gui_cursor_move_area_add_xy (-1, 0);
            else if (string_strcasecmp (argv[2], "area_right") == 0)
                gui_cursor_move_area_add_xy (1, 0);
        }
        return WEECHAT_RC_OK;
    }

    if (string_strcasecmp (argv[1], "stop") == 0)
    {
        gui_cursor_mode_toggle ();
        return WEECHAT_RC_OK;
    }

    return WEECHAT_RC_OK;
}

/*
 * command_debug: control debug for core/plugins
 */

COMMAND_CALLBACK(debug)
{
    struct t_config_option *ptr_option;
    struct t_weechat_plugin *ptr_plugin;
    int debug;

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
        if (argc > 2)
            log_printf ("Dump request for plugin: \"%s\"", argv_eol[2]);
        else
            log_printf ("Dump request for WeeChat core and plugins");
        weechat_log_use_time = 0;
        hook_signal_send ("debug_dump", WEECHAT_HOOK_SIGNAL_STRING,
                          (argc > 2) ? argv_eol[2] : NULL);
        weechat_log_use_time = 1;
    }
    else if (string_strcasecmp (argv[1], "buffer") == 0)
    {
        gui_buffer_dump_hexa (buffer);
        gui_chat_printf (NULL,
                         _("Raw content of buffers has been written in log "
                           "file"));
    }
    else if (string_strcasecmp (argv[1], "color") == 0)
    {
        gui_color_dump (buffer);
    }
    else if (string_strcasecmp (argv[1], "cursor") == 0)
    {
        if (gui_cursor_debug)
            gui_cursor_debug_set (0);
        else
        {
            debug = ((argc > 2)
                     && (string_strcasecmp (argv[2], "verbose") == 0)) ? 2 : 1;
            gui_cursor_debug_set (debug);
        }
    }
    else if (string_strcasecmp (argv[1], "hdata") == 0)
    {
        if ((argc > 2) && (string_strcasecmp (argv[2], "free") == 0))
            hdata_free_all ();
        else
            debug_hdata ();
    }
    else if (string_strcasecmp (argv[1], "hooks") == 0)
    {
        debug_hooks ();
    }
    else if (string_strcasecmp (argv[1], "infolists") == 0)
    {
        debug_infolists ();
    }
    else if (string_strcasecmp (argv[1], "memory") == 0)
    {
        debug_memory ();
    }
    else if (string_strcasecmp (argv[1], "mouse") == 0)
    {
        if (gui_mouse_debug)
            gui_mouse_debug_set (0);
        else
        {
            debug = ((argc > 2)
                     && (string_strcasecmp (argv[2], "verbose") == 0)) ? 2 : 1;
            gui_mouse_debug_set (debug);
        }
    }
    else if (string_strcasecmp (argv[1], "tags") == 0)
    {
        gui_chat_display_tags ^= 1;
        gui_window_ask_refresh (2);
    }
    else if (string_strcasecmp (argv[1], "term") == 0)
    {
        gui_window_term_display_infos ();
    }
    else if (string_strcasecmp (argv[1], "windows") == 0)
    {
        debug_windows_tree ();
    }
    else if (string_strcasecmp (argv[1], "set") == 0)
    {
        COMMAND_MIN_ARGS(4, "debug set");
        if (strcmp (argv[3], "0") == 0)
        {
            /* disable debug for a plugin */
            ptr_option = config_weechat_debug_get (argv[2]);
            if (ptr_option)
            {
                config_file_option_free (ptr_option);
                config_weechat_debug_set_all ();
                gui_chat_printf (NULL, _("Debug disabled for \"%s\""),
                                 argv[2]);
            }
        }
        else
        {
            /* set debug level for a plugin */
            if (config_weechat_debug_set (argv[2], argv[3]) != WEECHAT_CONFIG_OPTION_SET_ERROR)
            {
                ptr_option = config_weechat_debug_get (argv[2]);
                if (ptr_option)
                {
                    gui_chat_printf (NULL, "%s: \"%s\" => %d",
                                     "debug", argv[2],
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
                               _("  %s[%s%s%s]%s buffer: %s%s%s "
                                 "/ tags: %s / regex: %s %s"),
                               GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                               GUI_COLOR(GUI_COLOR_CHAT),
                               filter->name,
                               GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                               GUI_COLOR(GUI_COLOR_CHAT),
                               GUI_COLOR(GUI_COLOR_CHAT_BUFFER),
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

COMMAND_CALLBACK(filter)
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
                    ptr_filter->enabled = 1;
                    gui_filter_all_buffers ();
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
                return WEECHAT_RC_OK;
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
            /* disable a filter */
            ptr_filter = gui_filter_search_by_name (argv[2]);
            if (ptr_filter)
            {
                if (ptr_filter->enabled)
                {
                    ptr_filter->enabled = 0;
                    gui_filter_all_buffers ();
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
                return WEECHAT_RC_OK;
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
                ptr_filter->enabled ^= 1;
                gui_filter_all_buffers ();
            }
            else
            {
                gui_chat_printf_date_tags (NULL, 0, GUI_FILTER_TAG_NO_FILTER,
                                           _("%sError: filter \"%s\" not found"),
                                           gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                           argv[2]);
                return WEECHAT_RC_OK;
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
        COMMAND_MIN_ARGS(6, "filter add");
        if (gui_filter_search_by_name (argv[2]))
        {
            gui_chat_printf_date_tags (NULL, 0, GUI_FILTER_TAG_NO_FILTER,
                                       _("%sError: filter \"%s\" already exists"),
                                       gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                       argv[2]);
            return WEECHAT_RC_OK;
        }
        if ((strcmp (argv[4], "*") == 0) && (strcmp (argv_eol[5], "*") == 0))
        {
            gui_chat_printf_date_tags (NULL, 0, GUI_FILTER_TAG_NO_FILTER,
                                       _("%sError: you must specify at least "
                                         "tag(s) or regex for filter"),
                                       gui_chat_prefix[GUI_CHAT_PREFIX_ERROR]);
            return WEECHAT_RC_OK;
        }

        ptr_filter = gui_filter_new (1, argv[2], argv[3], argv[4], argv_eol[5]);

        if (ptr_filter)
        {
            gui_filter_all_buffers ();
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
        COMMAND_MIN_ARGS(4, "filter rename");
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
                return WEECHAT_RC_OK;
            }
        }
        else
        {
            gui_chat_printf_date_tags (NULL, 0, GUI_FILTER_TAG_NO_FILTER,
                                       _("%sError: filter \"%s\" not found"),
                                       gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                       argv[2]);
            return WEECHAT_RC_OK;
        }

        return WEECHAT_RC_OK;
    }

    /* delete filter */
    if (string_strcasecmp (argv[1], "del") == 0)
    {
        COMMAND_MIN_ARGS(3, "filter del");
        if (string_strcasecmp (argv[2], "-all") == 0)
        {
            if (gui_filters)
            {
                gui_filter_free_all ();
                gui_filter_all_buffers ();
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
                gui_filter_all_buffers ();
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
                return WEECHAT_RC_OK;
            }
        }
        return WEECHAT_RC_OK;
    }

    gui_chat_printf_date_tags (NULL, 0, GUI_FILTER_TAG_NO_FILTER,
                               _("%sError: unknown option for \"%s\" "
                                 "command"),
                               gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                               "filter");
    return WEECHAT_RC_OK;

}

/*
 * command_help_list_plugin_commands: display help for commands of a plugin
 *                                    (or core commands if plugin is NULL)
 */

void
command_help_list_plugin_commands (struct t_weechat_plugin *plugin,
                                   int verbose)
{
    struct t_hook *ptr_hook;
    struct t_weelist *list;
    struct t_weelist_item *item;
    int command_found, length, max_length, list_size;
    int cols, lines, col, line, index;
    char str_format[64], str_command[256], str_line[2048];

    if (verbose)
    {
        command_found = 0;
        for (ptr_hook = weechat_hooks[HOOK_TYPE_COMMAND]; ptr_hook;
             ptr_hook = ptr_hook->next_hook)
        {
            if (!ptr_hook->deleted && (ptr_hook->plugin == plugin)
                && HOOK_COMMAND(ptr_hook, command)
                && HOOK_COMMAND(ptr_hook, command)[0])
            {
                if (!command_found)
                {
                    gui_chat_printf (NULL, "");
                    gui_chat_printf (NULL,
                                     "%s[%s%s%s]",
                                     GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                     GUI_COLOR(GUI_COLOR_CHAT_BUFFER),
                                     plugin_get_name (plugin),
                                     GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS));
                    command_found = 1;
                }
                gui_chat_printf (NULL, "  %s%s%s%s%s",
                                 GUI_COLOR(GUI_COLOR_CHAT_BUFFER),
                                 HOOK_COMMAND(ptr_hook, command),
                                 GUI_COLOR(GUI_COLOR_CHAT),
                                 (HOOK_COMMAND(ptr_hook, description)
                                  && HOOK_COMMAND(ptr_hook, description)[0]) ?
                                 " - " : "",
                                 (HOOK_COMMAND(ptr_hook, description)
                                  && HOOK_COMMAND(ptr_hook, description)[0]) ?
                                 _(HOOK_COMMAND(ptr_hook, description)) : "");
            }
        }
    }
    else
    {
        max_length = -1;
        list = weelist_new ();

        /*
         * build list of commands for plugin and save max length of command
         * names
         */
        for (ptr_hook = weechat_hooks[HOOK_TYPE_COMMAND]; ptr_hook;
             ptr_hook = ptr_hook->next_hook)
        {
            if (!ptr_hook->deleted && (ptr_hook->plugin == plugin)
                && HOOK_COMMAND(ptr_hook, command)
                && HOOK_COMMAND(ptr_hook, command)[0])
            {
                length = utf8_strlen_screen (HOOK_COMMAND(ptr_hook, command));
                if (length > max_length)
                    max_length = length;
                weelist_add (list, HOOK_COMMAND(ptr_hook, command),
                             WEECHAT_LIST_POS_SORT, NULL);
            }
        }

        /* use list to display commands, sorted by columns */
        list_size = weelist_size (list);
        if ((max_length > 0) && (list_size > 0))
        {
            gui_chat_printf (NULL, "");
            gui_chat_printf (NULL,
                             "%s[%s%s%s]",
                             GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                             GUI_COLOR(GUI_COLOR_CHAT_BUFFER),
                             plugin_get_name (plugin),
                             GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS));

            snprintf (str_format, sizeof (str_format),
                      " %%-%ds", max_length);

            /* auto compute number of colums, max size is 90% of chat width */
            cols = ((gui_current_window->win_chat_width * 90) / 100) / (max_length + 1);
            if (cols == 0)
                cols = 1;
            lines = ((list_size - 1) / cols) + 1;
            for (line = 0; line < lines; line++)
            {
                str_line[0] = '\0';
                for (col = 0; col < cols; col++)
                {
                    index = (col * lines) + line;
                    if (index < list_size)
                    {
                        item = weelist_get (list, index);
                        if (item)
                        {
                            if (strlen (str_line) + strlen (weelist_string (item)) + 1 < (int)sizeof (str_line))
                            {
                                snprintf (str_command, sizeof (str_command),
                                          str_format, weelist_string (item));
                                strcat (str_line, str_command);
                            }
                        }
                    }
                }
                gui_chat_printf (NULL, " %s", str_line);
            }
        }

        weelist_free (list);
    }
}

/*
 * command_help_list_commands: display help for commands
 */

void
command_help_list_commands (int verbose)
{
    struct t_weechat_plugin *ptr_plugin;

    /* WeeChat commands */
    command_help_list_plugin_commands (NULL, verbose);

    /* plugins commands */
    for (ptr_plugin = weechat_plugins; ptr_plugin;
         ptr_plugin = ptr_plugin->next_plugin)
    {
        command_help_list_plugin_commands (ptr_plugin, verbose);
    }
}

/*
 * command_help: display help about commands
 */

COMMAND_CALLBACK(help)
{
    struct t_hook *ptr_hook;
    struct t_weechat_plugin *ptr_plugin;
    struct t_config_option *ptr_option;
    int i, length, command_found, first_line_displayed, verbose;
    char *string, *ptr_string, *pos_double_pipe, *pos_end;
    char empty_string[1] = { '\0' }, str_format[64];

    /* make C compiler happy */
    (void) data;
    (void) buffer;
    (void) argv_eol;

    /* display help for all commands */
    if ((argc == 1)
        || ((argc > 1) && (string_strncasecmp (argv[1], "-list", 5) == 0)))
    {
        verbose = ((argc > 1) && (string_strcasecmp (argv[1], "-listfull") == 0));
        if (argc > 2)
        {
            for (i = 2; i < argc; i++)
            {
                if (string_strcasecmp (argv[i], PLUGIN_CORE) == 0)
                    command_help_list_plugin_commands (NULL, verbose);
                else
                {
                    ptr_plugin = plugin_search (argv[i]);
                    if (ptr_plugin)
                        command_help_list_plugin_commands (ptr_plugin, verbose);
                }
            }
        }
        else
            command_help_list_commands (verbose);
        return WEECHAT_RC_OK;
    }

    /* look for command */
    command_found = 0;
    for (ptr_hook = weechat_hooks[HOOK_TYPE_COMMAND]; ptr_hook;
         ptr_hook = ptr_hook->next_hook)
    {
        if (!ptr_hook->deleted
            && HOOK_COMMAND(ptr_hook, command)
            && HOOK_COMMAND(ptr_hook, command)[0]
            && (string_strcasecmp (HOOK_COMMAND(ptr_hook, command),
                                   argv[1]) == 0))
        {
            command_found = 1;
            gui_chat_printf (NULL, "");
            length = utf8_strlen_screen (plugin_get_name (ptr_hook->plugin)) +
                ((ptr_hook->subplugin && ptr_hook->subplugin[0]) ? utf8_strlen_screen (ptr_hook->subplugin) + 1 : 0) +
                utf8_strlen_screen (HOOK_COMMAND(ptr_hook, command)) + 7;
            snprintf (str_format, sizeof (str_format),
                      "%%-%ds%%s", length);
            first_line_displayed = 0;
            ptr_string = (HOOK_COMMAND(ptr_hook, args) && HOOK_COMMAND(ptr_hook, args)[0]) ?
                _(HOOK_COMMAND(ptr_hook, args)) : empty_string;
            while (ptr_string)
            {
                string = NULL;
                pos_double_pipe = strstr (ptr_string, "||");
                if (pos_double_pipe)
                {
                    pos_end = pos_double_pipe - 1;
                    while ((pos_end > ptr_string) && (pos_end[0] == ' '))
                    {
                        pos_end--;
                    }
                    string = string_strndup (ptr_string,
                                             pos_end - ptr_string + 1);
                }
                if (first_line_displayed)
                {
                    gui_chat_printf (NULL, str_format,
                                     " ",
                                     (string) ? string : ptr_string);
                }
                else
                {
                    gui_chat_printf (NULL,
                                     "%s[%s%s%s%s%s%s%s]  %s/%s  %s%s",
                                     GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                     GUI_COLOR(GUI_COLOR_CHAT),
                                     plugin_get_name (ptr_hook->plugin),
                                     (ptr_hook->subplugin && ptr_hook->subplugin[0]) ?
                                     GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS) : "",
                                     (ptr_hook->subplugin && ptr_hook->subplugin[0]) ?
                                     "/" : "",
                                     (ptr_hook->subplugin && ptr_hook->subplugin[0]) ?
                                     GUI_COLOR(GUI_COLOR_CHAT) : "",
                                     (ptr_hook->subplugin && ptr_hook->subplugin[0]) ?
                                     ptr_hook->subplugin : "",
                                     GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                     GUI_COLOR(GUI_COLOR_CHAT_BUFFER),
                                     HOOK_COMMAND(ptr_hook, command),
                                     GUI_COLOR(GUI_COLOR_CHAT),
                                     (string) ? string : ptr_string);
                    first_line_displayed = 1;
                }
                if (string)
                    free (string);

                if (pos_double_pipe)
                {
                    ptr_string = pos_double_pipe + 2;
                    while (ptr_string[0] == ' ')
                    {
                        ptr_string++;
                    }
                }
                else
                    ptr_string = NULL;
            }
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
        }
    }
    if (command_found)
        return WEECHAT_RC_OK;

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
                         (ptr_option->description && ptr_option->description[0]) ?
                         _(ptr_option->description) : "");
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
                                     GUI_COLOR(GUI_COLOR_CHAT_VALUE),
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
                                             GUI_COLOR(GUI_COLOR_CHAT_VALUE),
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
                                         GUI_COLOR(GUI_COLOR_CHAT_VALUE),
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
                                     GUI_COLOR(GUI_COLOR_CHAT_VALUE),
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
                                 _("values"),
                                 _("a WeeChat color name (default, black, "
                                   "(dark)gray, white, (light)red, (light)green, "
                                   "brown, yellow, (light)blue, (light)magenta, "
                                   "(light)cyan), a terminal color number or "
                                   "an alias; attributes are allowed before "
                                   "color (for text color only, not "
                                   "background): \"*\" for bold, \"!\" for "
                                   "reverse, \"_\" for underline"));
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
                                     GUI_COLOR(GUI_COLOR_CHAT_VALUE),
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
        if (ptr_option->null_value_allowed)
        {
            gui_chat_printf (NULL, "  %s",
                             /* TRANSLATORS: please do not translate "(null)" */
                             _("undefined value allowed (null)"));
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

COMMAND_CALLBACK(history)
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
                gui_chat_printf_date_tags (buffer, 0, "no_log,cmd_history", "");
                gui_chat_printf_date_tags (buffer, 0, "no_log,cmd_history",
                                           _("Buffer command history:"));
            }
            gui_chat_printf_date_tags (buffer, 0, "no_log,cmd_history",
                                       "%s", ptr_history->text);
            displayed = 1;
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * command_input: input actions (used by key bindings)
 */

COMMAND_CALLBACK(input)
{
    /* make C compiler happy */
    (void) data;

    if (argc > 1)
    {
        if (string_strcasecmp (argv[1], "clipboard_paste") == 0)
            gui_input_clipboard_paste (buffer);
        else if (string_strcasecmp (argv[1], "return") == 0)
            gui_input_return (buffer);
        else if (string_strcasecmp (argv[1], "complete_next") == 0)
            gui_input_complete_next (buffer);
        else if (string_strcasecmp (argv[1], "complete_previous") == 0)
            gui_input_complete_previous (buffer);
        else if (string_strcasecmp (argv[1], "search_text") == 0)
            gui_input_search_text (buffer);
        else if (string_strcasecmp (argv[1], "search_previous") == 0)
            gui_input_search_previous (buffer);
        else if (string_strcasecmp (argv[1], "search_next") == 0)
            gui_input_search_next (buffer);
        else if (string_strcasecmp (argv[1], "search_switch_case") == 0)
            gui_input_search_switch_case (buffer);
        else if (string_strcasecmp (argv[1], "search_stop") == 0)
            gui_input_search_stop (buffer);
        else if (string_strcasecmp (argv[1], "delete_previous_char") == 0)
            gui_input_delete_previous_char (buffer);
        else if (string_strcasecmp (argv[1], "delete_next_char") == 0)
            gui_input_delete_next_char (buffer);
        else if (string_strcasecmp (argv[1], "delete_previous_word") == 0)
            gui_input_delete_previous_word (buffer);
        else if (string_strcasecmp (argv[1], "delete_next_word") == 0)
            gui_input_delete_next_word (buffer);
        else if (string_strcasecmp (argv[1], "delete_beginning_of_line") == 0)
            gui_input_delete_beginning_of_line (buffer);
        else if (string_strcasecmp (argv[1], "delete_end_of_line") == 0)
            gui_input_delete_end_of_line (buffer);
        else if (string_strcasecmp (argv[1], "delete_line") == 0)
            gui_input_delete_line (buffer);
        else if (string_strcasecmp (argv[1], "transpose_chars") == 0)
            gui_input_transpose_chars (buffer);
        else if (string_strcasecmp (argv[1], "move_beginning_of_line") == 0)
            gui_input_move_beginning_of_line (buffer);
        else if (string_strcasecmp (argv[1], "move_end_of_line") == 0)
            gui_input_move_end_of_line (buffer);
        else if (string_strcasecmp (argv[1], "move_previous_char") == 0)
            gui_input_move_previous_char (buffer);
        else if (string_strcasecmp (argv[1], "move_next_char") == 0)
            gui_input_move_next_char (buffer);
        else if (string_strcasecmp (argv[1], "move_previous_word") == 0)
            gui_input_move_previous_word (buffer);
        else if (string_strcasecmp (argv[1], "move_next_word") == 0)
            gui_input_move_next_word (buffer);
        else if (string_strcasecmp (argv[1], "history_previous") == 0)
            gui_input_history_local_previous (buffer);
        else if (string_strcasecmp (argv[1], "history_next") == 0)
            gui_input_history_local_next (buffer);
        else if (string_strcasecmp (argv[1], "history_global_previous") == 0)
            gui_input_history_global_previous (buffer);
        else if (string_strcasecmp (argv[1], "history_global_next") == 0)
            gui_input_history_global_next (buffer);
        else if (string_strcasecmp (argv[1], "jump_smart") == 0)
            gui_input_jump_smart (buffer);
        else if (string_strcasecmp (argv[1], "jump_last_buffer") == 0)
            gui_input_jump_last_buffer (buffer);
        else if (string_strcasecmp (argv[1], "jump_last_buffer_displayed") == 0)
            gui_input_jump_last_buffer_displayed (buffer);
        else if (string_strcasecmp (argv[1], "jump_previously_visited_buffer") == 0)
            gui_input_jump_previously_visited_buffer (buffer);
        else if (string_strcasecmp (argv[1], "jump_next_visited_buffer") == 0)
            gui_input_jump_next_visited_buffer (buffer);
        else if (string_strcasecmp (argv[1], "hotlist_clear") == 0)
            gui_input_hotlist_clear (buffer);
        else if (string_strcasecmp (argv[1], "grab_key") == 0)
            gui_input_grab_key (buffer, 0, (argc > 2) ? argv[2] : NULL);
        else if (string_strcasecmp (argv[1], "grab_key_command") == 0)
            gui_input_grab_key (buffer, 1, (argc > 2) ? argv[2] : NULL);
        else if (string_strcasecmp (argv[1], "grab_mouse") == 0)
            gui_input_grab_mouse (buffer, 0);
        else if (string_strcasecmp (argv[1], "grab_mouse_area") == 0)
            gui_input_grab_mouse (buffer, 1);
        else if (string_strcasecmp (argv[1], "set_unread") == 0)
            gui_input_set_unread ();
        else if (string_strcasecmp (argv[1], "set_unread_current_buffer") == 0)
            gui_input_set_unread_current (buffer);
        else if (string_strcasecmp (argv[1], "switch_active_buffer") == 0)
            gui_input_switch_active_buffer (buffer);
        else if (string_strcasecmp (argv[1], "switch_active_buffer_previous") == 0)
            gui_input_switch_active_buffer_previous (buffer);
        else if (string_strcasecmp (argv[1], "insert") == 0)
        {
            if (argc > 2)
                gui_input_insert (buffer, argv_eol[2]);
        }
        else if (string_strcasecmp (argv[1], "undo") == 0)
            gui_input_undo (buffer);
        else if (string_strcasecmp (argv[1], "redo") == 0)
            gui_input_redo (buffer);
        else if (string_strcasecmp (argv[1], "paste_start") == 0)
        {
            /* do nothing here */
        }
        else if (string_strcasecmp (argv[1], "paste_stop") == 0)
        {
            /* do nothing here */
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * command_key_display: display a key binding
 */

void
command_key_display (struct t_gui_key *key, struct t_gui_key *default_key)
{
    char *expanded_name;

    expanded_name = gui_key_get_expanded_name (key->key);

    if (default_key)
    {
        gui_chat_printf (NULL, "  %s%s => %s%s  %s(%s%s %s%s)",
                         (expanded_name) ? expanded_name : key->key,
                         GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                         GUI_COLOR(GUI_COLOR_CHAT),
                         key->command,
                         GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                         GUI_COLOR(GUI_COLOR_CHAT),
                         _("default command:"),
                         default_key->command,
                         GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS));
    }
    else
    {
        gui_chat_printf (NULL, "  %s%s => %s%s",
                         (expanded_name) ? expanded_name : key->key,
                         GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                         GUI_COLOR(GUI_COLOR_CHAT),
                         key->command);
    }

    if (expanded_name)
        free (expanded_name);
}

/*
 * command_key_display_list: display a list of keys
 */

void
command_key_display_list (const char *message_no_key,
                          const char *message_keys,
                          int context,
                          struct t_gui_key *keys,
                          int keys_count)
{
    struct t_gui_key *ptr_key;

    if (keys_count == 0)
        gui_chat_printf (NULL, message_no_key,
                         gui_key_context_string[context]);
    else
    {
        gui_chat_printf (NULL, "");
        gui_chat_printf (NULL, message_keys,
                         keys_count, gui_key_context_string[context]);
        for (ptr_key = keys; ptr_key; ptr_key = ptr_key->next_key)
        {
            command_key_display (ptr_key, NULL);
        }
    }
}

/*
 * command_key_display_listdiff: list differences between default and current
 *                               keys (keys added, redefined or removed)
 */

void
command_key_display_listdiff (int context)
{
    struct t_gui_key *ptr_key, *ptr_default_key;
    int count_added, count_deleted;

    /* list keys added or redefined */
    count_added = 0;
    for (ptr_key = gui_keys[context]; ptr_key; ptr_key = ptr_key->next_key)
    {
        ptr_default_key = gui_key_search (gui_default_keys[context],
                                          ptr_key->key);
        if (!ptr_default_key
            || (strcmp (ptr_default_key->command, ptr_key->command) != 0))
        {
            count_added++;
        }
    }
    if (count_added > 0)
    {
        gui_chat_printf (NULL, "");
        gui_chat_printf (NULL,
                         /* TRANSLATORS: first "%d" is number of keys */
                         _("%d key bindings added or redefined for "
                           "context \"%s\":"),
                         count_added,
                         _(gui_key_context_string[context]));
        for (ptr_key = gui_keys[context]; ptr_key; ptr_key = ptr_key->next_key)
        {
            ptr_default_key = gui_key_search (gui_default_keys[context],
                                              ptr_key->key);
            if (!ptr_default_key
                || (strcmp (ptr_default_key->command, ptr_key->command) != 0))
            {
                command_key_display (ptr_key, ptr_default_key);
            }
        }
    }

    /* list keys deleted */
    count_deleted = 0;
    for (ptr_default_key = gui_default_keys[context]; ptr_default_key;
         ptr_default_key = ptr_default_key->next_key)
    {
        ptr_key = gui_key_search (gui_keys[context], ptr_default_key->key);
        if (!ptr_key)
            count_deleted++;
        }
    if (count_deleted > 0)
    {
        gui_chat_printf (NULL, "");
        gui_chat_printf (NULL,
                         /* TRANSLATORS: first "%d" is number of keys */
                         _("%d key bindings deleted for context \"%s\":"),
                         count_deleted,
                         _(gui_key_context_string[context]));
        for (ptr_default_key = gui_default_keys[context]; ptr_default_key;
             ptr_default_key = ptr_default_key->next_key)
        {
            ptr_key = gui_key_search (gui_keys[context], ptr_default_key->key);
            if (!ptr_key)
            {
                command_key_display (ptr_default_key, NULL);
            }
        }
    }

    /* display a message if all key bindings are default bindings */
    if ((count_added == 0) && (count_deleted == 0))
    {
        gui_chat_printf (NULL, "");
        gui_chat_printf (NULL,
                         _("No key binding added, redefined or removed "
                           "for context \"%s\""),
                         _(gui_key_context_string[context]));
    }
}

/*
 * command_key_reset: reset a key for a given context
 */

int
command_key_reset (int context, const char *key)
{
    char *internal_code;
    struct t_gui_key *ptr_key, *ptr_default_key, *ptr_new_key;
    int rc;

    internal_code = gui_key_get_internal_code (key);
    if (!internal_code)
        return WEECHAT_RC_ERROR;

    ptr_key = gui_key_search (gui_keys[context],
                              internal_code);
    ptr_default_key = gui_key_search (gui_default_keys[context],
                                      internal_code);
    free (internal_code);

    if (ptr_key || ptr_default_key)
    {
        if (ptr_key && ptr_default_key)
        {
            if (strcmp (ptr_key->command, ptr_default_key->command) != 0)
            {
                gui_key_verbose = 1;
                ptr_new_key = gui_key_bind (NULL, context, key,
                                            ptr_default_key->command);
                gui_key_verbose = 0;
                if (!ptr_new_key)
                {
                    gui_chat_printf (NULL,
                                     _("%sError: unable to bind key \"%s\""),
                                     gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                     key);
                    return WEECHAT_RC_OK;
                }
            }
            else
            {
                gui_chat_printf (NULL,
                                 _("Key \"%s\" has already default "
                                   "value"),
                                 key);
            }
        }
        else if (ptr_key)
        {
            /* no default key, so just unbind key */
            gui_key_verbose = 1;
            rc = gui_key_unbind (NULL, context, key);
            gui_key_verbose = 0;
            if (!rc)
            {
                gui_chat_printf (NULL,
                                 _("%sError: unable to unbind key \"%s\""),
                                 gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                 key);
                return WEECHAT_RC_OK;
            }
        }
        else
        {
            /* no key, but default key exists */
            gui_key_verbose = 1;
            ptr_new_key = gui_key_bind (NULL, context, key,
                                        ptr_default_key->command);
            gui_key_verbose = 0;
            if (!ptr_new_key)
            {
                gui_chat_printf (NULL,
                                 _("%sError: unable to bind key \"%s\""),
                                 gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                 key);
                return WEECHAT_RC_OK;
            }
        }
    }
    else
    {
        gui_chat_printf (NULL, _("%sKey \"%s\" not found"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         key);
    }
    return WEECHAT_RC_OK;
}

/*
 * command_key: bind/unbind keys
 */

COMMAND_CALLBACK(key)
{
    char *internal_code;
    struct t_gui_key *ptr_new_key;
    int old_keys_count, keys_added, i, context, rc;

    /* make C compiler happy */
    (void) data;
    (void) buffer;

    /* display all key bindings (current keys) */
    if ((argc == 1) || (string_strcasecmp (argv[1], "list") == 0))
    {
        for (i = 0; i < GUI_KEY_NUM_CONTEXTS; i++)
        {
            if ((argc < 3)
                || (string_strcasecmp (argv[2], gui_key_context_string[i]) == 0))
            {
                command_key_display_list (_("No key binding defined for "
                                            "context \"%s\""),
                                          /* TRANSLATORS: first "%d" is number of keys */
                                          _("%d key bindings for context "
                                            "\"%s\":"),
                                          i, gui_keys[i], gui_keys_count[i]);
            }
        }
        return WEECHAT_RC_OK;
    }

    /* display redefined or key bindings added */
    if (string_strcasecmp (argv[1], "listdiff") == 0)
    {
        for (i = 0; i < GUI_KEY_NUM_CONTEXTS; i++)
        {
            if ((argc < 3)
                || (string_strcasecmp (argv[2], gui_key_context_string[i]) == 0))
            {
                command_key_display_listdiff (i);
            }
        }
        return WEECHAT_RC_OK;
    }

    /* display default key bindings */
    if (string_strcasecmp (argv[1], "listdefault") == 0)
    {
        for (i = 0; i < GUI_KEY_NUM_CONTEXTS; i++)
        {
            if ((argc < 3)
                || (string_strcasecmp (argv[2], gui_key_context_string[i]) == 0))
            {
                command_key_display_list (_("No default key binding for "
                                            "context \"%s\""),
                                          /* TRANSLATORS: first "%d" is number of keys */
                                          _("%d default key bindings for "
                                            "context \"%s\":"),
                                          i,
                                          gui_default_keys[i],
                                          gui_default_keys_count[i]);
            }
        }
        return WEECHAT_RC_OK;
    }

    /* bind a key (or display binding) */
    if (string_strcasecmp (argv[1], "bind") == 0)
    {
        COMMAND_MIN_ARGS(3, "key bind");

        /* display a key binding */
        if (argc == 3)
        {
            ptr_new_key = NULL;
            internal_code = gui_key_get_internal_code (argv[2]);
            if (internal_code)
                ptr_new_key = gui_key_search (gui_keys[GUI_KEY_CONTEXT_DEFAULT],
                                              internal_code);
            if (ptr_new_key)
            {
                gui_chat_printf (NULL, "");
                gui_chat_printf (NULL, _("Key:"));
                command_key_display (ptr_new_key, NULL);
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
        gui_key_verbose = 1;
        ptr_new_key = gui_key_bind (NULL, GUI_KEY_CONTEXT_DEFAULT,
                                    argv[2], argv_eol[3]);
        gui_key_verbose = 0;
        if (!ptr_new_key)
        {
            gui_chat_printf (NULL,
                             _("%sError: unable to bind key \"%s\""),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             argv[2]);
            return WEECHAT_RC_OK;
        }
        return WEECHAT_RC_OK;
    }

    /* bind a key for given context (or display binding) */
    if (string_strcasecmp (argv[1], "bindctxt") == 0)
    {
        COMMAND_MIN_ARGS(4, "key bindctxt");

        /* search context */
        context = gui_key_search_context (argv[2]);
        if (context < 0)
        {
            gui_chat_printf (NULL,
                             _("%sError: context \"%s\" not found"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             argv[2]);
            return WEECHAT_RC_OK;
        }

        /* display a key binding */
        if (argc == 4)
        {
            ptr_new_key = NULL;
            internal_code = gui_key_get_internal_code (argv[2]);
            if (internal_code)
                ptr_new_key = gui_key_search (gui_keys[context],
                                              internal_code);
            if (ptr_new_key)
            {
                gui_chat_printf (NULL, "");
                gui_chat_printf (NULL, _("Key:"));
                command_key_display (ptr_new_key, NULL);
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
        gui_key_verbose = 1;
        ptr_new_key = gui_key_bind (NULL, context,
                                    argv[3], argv_eol[4]);
        gui_key_verbose = 0;
        if (!ptr_new_key)
        {
            gui_chat_printf (NULL,
                             _("%sError: unable to bind key \"%s\""),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             argv[3]);
            return WEECHAT_RC_OK;
        }
        return WEECHAT_RC_OK;
    }

    /* unbind a key */
    if (string_strcasecmp (argv[1], "unbind") == 0)
    {
        COMMAND_MIN_ARGS(3, "key unbind");

        gui_key_verbose = 1;
        rc = gui_key_unbind (NULL, GUI_KEY_CONTEXT_DEFAULT, argv[2]);
        gui_key_verbose = 0;
        if (!rc)
        {
            gui_chat_printf (NULL,
                             _("%sError: unable to unbind key \"%s\""),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             argv[2]);
            return WEECHAT_RC_OK;
        }
        return WEECHAT_RC_OK;
    }

    /* unbind a key for a given context */
    if (string_strcasecmp (argv[1], "unbindctxt") == 0)
    {
        COMMAND_MIN_ARGS(4, "key unbindctxt");

        /* search context */
        context = gui_key_search_context (argv[2]);
        if (context < 0)
        {
            gui_chat_printf (NULL,
                             _("%sError: context \"%s\" not found"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             argv[2]);
            return WEECHAT_RC_OK;
        }

        gui_key_verbose = 1;
        rc = gui_key_unbind (NULL, context, argv[3]);
        gui_key_verbose = 0;
        if (!rc)
        {
            gui_chat_printf (NULL,
                             _("%sError: unable to unbind key \"%s\""),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             argv[3]);
            return WEECHAT_RC_OK;
        }
        return WEECHAT_RC_OK;
    }

    /* reset a key to default binding */
    if (string_strcasecmp (argv[1], "reset") == 0)
    {
        COMMAND_MIN_ARGS(3, "key reset");

        return command_key_reset (GUI_KEY_CONTEXT_DEFAULT, argv[2]);
    }

    /* reset a key to default binding for a given context */
    if (string_strcasecmp (argv[1], "resetctxt") == 0)
    {
        COMMAND_MIN_ARGS(4, "key reset");

        /* search context */
        context = gui_key_search_context (argv[2]);
        if (context < 0)
        {
            gui_chat_printf (NULL,
                             _("%sError: context \"%s\" not found"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             argv[2]);
            return WEECHAT_RC_OK;
        }

        return command_key_reset (context, argv[3]);
    }

    /* reset ALL keys (only with "-yes", for security reason) */
    if (string_strcasecmp (argv[1], "resetall") == 0)
    {
        if ((argc >= 3) && (string_strcasecmp (argv[2], "-yes") == 0))
        {
            for (i = 0; i < GUI_KEY_NUM_CONTEXTS; i++)
            {
                if ((argc < 4)
                    || (string_strcasecmp (argv[3], gui_key_context_string[i]) == 0))
                {
                    gui_key_free_all (&gui_keys[i], &last_gui_key[i],
                                      &gui_keys_count[i]);
                    gui_key_default_bindings (i);
                    gui_chat_printf (NULL,
                                     _("Default key bindings restored for "
                                       "context \"%s\""),
                                     gui_key_context_string[i]);
                }
            }
        }
        else
        {
            gui_chat_printf (NULL,
                             _("%sError: \"-yes\" argument is required for "
                               "keys reset (security reason)"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR]);
            return WEECHAT_RC_OK;
        }
        return WEECHAT_RC_OK;
    }

    /* add missing keys */
    if (string_strcasecmp (argv[1], "missing") == 0)
    {
        for (i = 0; i < GUI_KEY_NUM_CONTEXTS; i++)
        {
            if ((argc < 3)
                || (string_strcasecmp (argv[2], gui_key_context_string[i]) == 0))
            {
                old_keys_count = gui_keys_count[i];
                gui_key_verbose = 1;
                gui_key_default_bindings (i);
                gui_key_verbose = 0;
                keys_added = (gui_keys_count[i] > old_keys_count) ?
                    gui_keys_count[i] - old_keys_count : 0;
                gui_chat_printf (NULL,
                                 NG_("%d new key added", "%d new keys added "
                                     "(context: \"%s\")", keys_added),
                                 keys_added,
                                 gui_key_context_string[i]);
            }
        }
        return WEECHAT_RC_OK;
    }

    gui_chat_printf (NULL,
                     _("%sError: unknown option for \"%s\" "
                       "command"),
                     gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                     "key");

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

COMMAND_CALLBACK(layout)
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
            gui_layout_buffer_save (&gui_layout_buffers, &last_gui_layout_buffer);
            gui_chat_printf (NULL,
                             _("Layout saved for buffers (order of buffers)"));
        }
        if (flag_windows)
        {
            gui_layout_window_save (&gui_layout_windows);
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
            gui_layout_buffer_apply (gui_layout_buffers);
        if (flag_windows)
            gui_layout_window_apply (gui_layout_windows, -1);

        return WEECHAT_RC_OK;
    }

    /* reset layout */
    if (string_strcasecmp (argv[1], "reset") == 0)
    {
        if (flag_buffers)
        {
            gui_layout_buffer_reset (&gui_layout_buffers, &last_gui_layout_buffer);
            gui_chat_printf (NULL,
                             _("Layout reset for buffers"));
        }
        if (flag_windows)
        {
            gui_layout_window_reset (&gui_layout_windows);
            gui_chat_printf (NULL,
                             _("Layout reset for windows"));
        }

        return WEECHAT_RC_OK;
    }

    return WEECHAT_RC_OK;
}

/*
 * command_mouse_timer_cb: callback for mouse timer
 */

int
command_mouse_timer_cb (void *data, int remaining_calls)
{
    /* make C compiler happy */
    (void) data;
    (void) remaining_calls;

    if (gui_mouse_enabled)
    {
        gui_mouse_disable ();
        config_file_option_set (config_look_mouse, "0", 1);
    }
    else
    {
        gui_mouse_enable ();
        config_file_option_set (config_look_mouse, "1", 1);
    }

    return WEECHAT_RC_OK;
}

/*
 * command_mouse_timer: timer for toggling mouse
 */

void
command_mouse_timer (const char *delay)
{
    long seconds;
    char *error;

    error = NULL;
    seconds = strtol (delay, &error, 10);
    if (error && !error[0] && (seconds > 0))
    {
        hook_timer (NULL, seconds * 1000, 0, 1, &command_mouse_timer_cb, NULL);
    }
}

/*
 * command_mouse: mouse control
 */

COMMAND_CALLBACK(mouse)
{
    /* make C compiler happy */
    (void) data;
    (void) buffer;
    (void) argv_eol;

    if (argc == 1)
    {
        gui_mouse_display_state ();
        return WEECHAT_RC_OK;
    }

    /* enable mouse */
    if (string_strcasecmp (argv[1], "enable") == 0)
    {
        gui_mouse_enable ();
        config_file_option_set (config_look_mouse, "1", 1);
        gui_chat_printf (NULL, _("Mouse enabled"));
        if (argc > 2)
            command_mouse_timer (argv[2]);
        return WEECHAT_RC_OK;
    }

    /* disable mouse */
    if (string_strcasecmp (argv[1], "disable") == 0)
    {
        gui_mouse_disable ();
        config_file_option_set (config_look_mouse, "0", 1);
        gui_chat_printf (NULL, _("Mouse disabled"));
        if (argc > 2)
            command_mouse_timer (argv[2]);
        return WEECHAT_RC_OK;
    }

    /* toggle mouse */
    if (string_strcasecmp (argv[1], "toggle") == 0)
    {
        if (gui_mouse_enabled)
        {
            gui_mouse_disable ();
            config_file_option_set (config_look_mouse, "0", 1);
            gui_chat_printf (NULL, _("Mouse disabled"));
        }
        else
        {
            gui_mouse_enable ();
            config_file_option_set (config_look_mouse, "1", 1);
            gui_chat_printf (NULL, _("Mouse enabled"));
        }
        if (argc > 2)
            command_mouse_timer (argv[2]);
        return WEECHAT_RC_OK;
    }

    gui_chat_printf (NULL,
                     _("%sError: unknown option for \"%s\" "
                       "command"),
                     gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                     "mouse");

    return WEECHAT_RC_OK;
}

/*
 * command_mute: execute a command mute
 */

COMMAND_CALLBACK(mute)
{
    int length, mute_mode;
    char *command, *ptr_command;
    struct t_gui_buffer *mute_buffer, *ptr_buffer;

    /* make C compiler happy */
    (void) data;

    if (argc >= 2)
    {
        mute_mode = GUI_CHAT_MUTE_BUFFER;
        mute_buffer = gui_buffer_search_main ();
        ptr_command = argv_eol[1];

        if (string_strcasecmp (argv[1], "-current") == 0)
        {
            mute_buffer = buffer;
            ptr_command = argv_eol[2];
        }
        else if (string_strcasecmp (argv[1], "-buffer") == 0)
        {
            if (argc < 3)
                return WEECHAT_RC_ERROR;
            ptr_buffer = gui_buffer_search_by_full_name (argv[2]);
            if (ptr_buffer)
                mute_buffer = ptr_buffer;
            ptr_command = argv_eol[3];
        }
        else if (string_strcasecmp (argv[1], "-all") == 0)
        {
            mute_mode = GUI_CHAT_MUTE_ALL_BUFFERS;
            mute_buffer = NULL;
            ptr_command = argv_eol[2];
        }

        if (ptr_command && ptr_command[0])
        {
            gui_chat_mute = mute_mode;
            gui_chat_mute_buffer = mute_buffer;

            if (string_is_command_char (ptr_command))
            {
                input_exec_command (buffer, 1, NULL, ptr_command);
            }
            else
            {
                length = strlen (ptr_command) + 2;
                command = malloc (length);
                if (command)
                {
                    snprintf (command, length, "/%s", ptr_command);
                    input_exec_command (buffer, 1, NULL, command);
                    free (command);
                }
            }

            gui_chat_mute = GUI_CHAT_MUTE_DISABLED;
            gui_chat_mute_buffer = NULL;
        }
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

            if (full)
            {
                gui_chat_printf (NULL, "");

                /* plugin info */
                gui_chat_printf (NULL,
                                 "  %s%s %s[%sv%s%s]%s: %s (%s)",
                                 GUI_COLOR(GUI_COLOR_CHAT_BUFFER),
                                 ptr_plugin->name,
                                 GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                 GUI_COLOR(GUI_COLOR_CHAT),
                                 ptr_plugin->version,
                                 GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                 GUI_COLOR(GUI_COLOR_CHAT),
                                 (ptr_plugin->description && ptr_plugin->description[0]) ? _(ptr_plugin->description) : "",
                                 ptr_plugin->filename);

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

                /* command_run hooked */
                hook_found = 0;
                for (ptr_hook = weechat_hooks[HOOK_TYPE_COMMAND_RUN]; ptr_hook;
                     ptr_hook = ptr_hook->next_hook)
                {
                    if (!ptr_hook->deleted && (ptr_hook->plugin == ptr_plugin))
                    {
                        if (!hook_found)
                            gui_chat_printf (NULL, _("    command_run hooked:"));
                        hook_found = 1;
                        gui_chat_printf (NULL, "      %s",
                                         HOOK_COMMAND_RUN(ptr_hook, command));
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

                /* process hooked */
                hook_found = 0;
                for (ptr_hook = weechat_hooks[HOOK_TYPE_PROCESS]; ptr_hook;
                     ptr_hook = ptr_hook->next_hook)
                {
                    if (!ptr_hook->deleted && (ptr_hook->plugin == ptr_plugin))
                    {
                        if (!hook_found)
                            gui_chat_printf (NULL,
                                             _("    process hooked:"));
                        hook_found = 1;
                        gui_chat_printf (NULL,
                                         _("      command: '%s', child "
                                           "pid: %d"),
                                         (HOOK_PROCESS(ptr_hook, command)),
                                         HOOK_PROCESS(ptr_hook, child_pid));
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
                                           "port: %d, child pid: %d"),
                                         HOOK_CONNECT(ptr_hook, sock),
                                         HOOK_CONNECT(ptr_hook, address),
                                         HOOK_CONNECT(ptr_hook, port),
                                         HOOK_CONNECT(ptr_hook, child_pid));
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
            else
            {
                /* plugin info */
                gui_chat_printf (NULL,
                                 "  %s%s%s: %s",
                                 GUI_COLOR(GUI_COLOR_CHAT_BUFFER),
                                 ptr_plugin->name,
                                 GUI_COLOR(GUI_COLOR_CHAT),
                                 (ptr_plugin->description && ptr_plugin->description[0]) ? _(ptr_plugin->description) : "");
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

COMMAND_CALLBACK(plugin)
{
    int plugin_argc;
    char **plugin_argv, *full_name;

    /* make C compiler happy */
    (void) data;
    (void) buffer;

    if (argc == 1)
    {
        /* list all plugins */
        command_plugin_list (NULL, 0);
        return WEECHAT_RC_OK;
    }

    if (string_strcasecmp (argv[1], "list") == 0)
    {
        command_plugin_list ((argc > 2) ? argv[2] : NULL, 0);
        return WEECHAT_RC_OK;
    }

    if (string_strcasecmp (argv[1], "listfull") == 0)
    {
        command_plugin_list ((argc > 2) ? argv[2] : NULL, 1);
        return WEECHAT_RC_OK;
    }

    if (string_strcasecmp (argv[1], "autoload") == 0)
    {
        if (argc > 2)
        {
            plugin_argv = string_split (argv_eol[2], " ", 0, 0,
                                        &plugin_argc);
            plugin_auto_load (plugin_argc, plugin_argv);
        }
        else
            plugin_auto_load (0, NULL);
        return WEECHAT_RC_OK;
    }

    if (string_strcasecmp (argv[1], "load") == 0)
    {
        if (argc > 2)
        {
            plugin_argv = NULL;
            plugin_argc = 0;
            if (argc > 3)
            {
                plugin_argv = string_split (argv_eol[3], " ", 0, 0,
                                            &plugin_argc);
            }
            full_name = util_search_full_lib_name (argv[2], "plugins");
            plugin_load (full_name, plugin_argc, plugin_argv);
            if (full_name)
                free (full_name);
            if (plugin_argv)
                string_free_split (plugin_argv);
        }
        else
        {
            gui_chat_printf (NULL,
                             _("%sError: wrong argument count for \"%s\" "
                               "command"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             "plugin");
        }
        return WEECHAT_RC_OK;
    }

    if (string_strcasecmp (argv[1], "reload") == 0)
    {
        if (argc > 2)
        {
            if (argc > 3)
            {
                plugin_argv = string_split (argv_eol[3], " ", 0, 0,
                                            &plugin_argc);
                plugin_reload_name (argv[2], plugin_argc, plugin_argv);
                if (plugin_argv)
                    string_free_split (plugin_argv);
            }
            else
                plugin_reload_name (argv[2], 0, NULL);
        }
        else
        {
            plugin_unload_all ();
            plugin_auto_load (0, NULL);
        }
        return WEECHAT_RC_OK;
    }

    if (string_strcasecmp (argv[1], "unload") == 0)
    {
        if (argc > 2)
            plugin_unload_name (argv[2]);
        else
            plugin_unload_all ();
        return WEECHAT_RC_OK;
    }

    gui_chat_printf (NULL,
                     _("%sError: unknown option for \"%s\" "
                       "command"),
                     gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                     "plugin");

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

COMMAND_CALLBACK(proxy)
{
    int type;
    long value;
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
        COMMAND_MIN_ARGS(6, "proxy add");
        type = proxy_search_type (argv[3]);
        if (type < 0)
        {
            gui_chat_printf (NULL,
                             _("%sError: wrong type \"%s\" for proxy "
                               "\"%s\""),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             argv[3], argv[2]);
            return WEECHAT_RC_OK;
        }
        error = NULL;
        value = strtol (argv[5], &error, 10);
        (void) value;
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
            return WEECHAT_RC_OK;
        }

        return WEECHAT_RC_OK;
    }

    /* delete a proxy */
    if (string_strcasecmp (argv[1], "del") == 0)
    {
        COMMAND_MIN_ARGS(3, "proxy del");
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
                return WEECHAT_RC_OK;
            }
            proxy_free (ptr_proxy);
            gui_chat_printf (NULL, _("Proxy deleted"));
        }

        return WEECHAT_RC_OK;
    }

    /* set a proxy property */
    if (string_strcasecmp (argv[1], "set") == 0)
    {
        COMMAND_MIN_ARGS(5, "proxy set");
        ptr_proxy = proxy_search (argv[2]);
        if (!ptr_proxy)
        {
            gui_chat_printf (NULL,
                             _("%sError: unknown proxy \"%s\""),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             argv[2]);
            return WEECHAT_RC_OK;
        }
        if (!proxy_set (ptr_proxy, argv[3], argv_eol[4]))
        {
            gui_chat_printf (NULL,
                             _("%sError: unable to set option \"%s\" for "
                               "proxy \"%s\""),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             argv[3], argv[2]);
            return WEECHAT_RC_OK;
        }

        return WEECHAT_RC_OK;
    }

    gui_chat_printf (NULL,
                     _("%sError: unknown option for \"%s\" "
                       "command"),
                     gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                     "proxy");
    return WEECHAT_RC_OK;
}

/*
 * command_quit: quit WeeChat
 */

COMMAND_CALLBACK(quit)
{
    int confirm_ok;
    char *pos_args;

    /* make C compiler happy */
    (void) data;
    (void) buffer;

    confirm_ok = 0;
    pos_args = NULL;
    if (argc > 1)
    {
        if (string_strcasecmp (argv[1], "-yes") == 0)
        {
            confirm_ok = 1;
            if (argc > 2)
                pos_args = argv_eol[2];
        }
        else
            pos_args = argv_eol[1];
    }

    /* if confirmation is required, check that "-yes" is given */
    if (CONFIG_BOOLEAN(config_look_confirm_quit) && !confirm_ok)
    {
        gui_chat_printf (NULL,
                         _("%sYou must confirm quit command with extra "
                           "argument \"-yes\" (see /help quit)"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR]);
        return WEECHAT_RC_OK;
    }

    /*
     * send quit signal: some plugins like irc use this signal to disconnect
     * from servers
     */
    hook_signal_send ("quit", WEECHAT_HOOK_SIGNAL_STRING, pos_args);

    /* force end of WeeChat main loop */
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

    if (rc == WEECHAT_RC_OK)
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

COMMAND_CALLBACK(reload)
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
 * command_repeat_timer_cb: callback for repeat timer
 */

int
command_repeat_timer_cb (void *data, int remaining_calls)
{
    char **repeat_args;
    int i;
    struct t_gui_buffer *ptr_buffer;

    repeat_args = (char **)data;

    if (!repeat_args)
        return WEECHAT_RC_ERROR;

    if (repeat_args[0] && repeat_args[1])
    {
        /* search buffer, fallback to core buffer if not found */
        ptr_buffer = gui_buffer_search_by_full_name (repeat_args[0]);
        if (!ptr_buffer)
            ptr_buffer = gui_buffer_search_main ();

        /* execute command */
        if (ptr_buffer)
            input_exec_command (ptr_buffer, 1, NULL, repeat_args[1]);
    }

    if (remaining_calls == 0)
    {
        for (i = 0; i < 2; i++)
        {
            if (repeat_args[i])
                free (repeat_args[i]);
        }
        free (repeat_args);
    }

    return WEECHAT_RC_OK;
}

/*
 * command_repeat: execute a command several times
 */

COMMAND_CALLBACK(repeat)
{
    int arg_count, count, interval, length, i;
    char *error, *command, **repeat_args;

    /* make C compiler happy */
    (void) data;

    if (argc < 3)
        return WEECHAT_RC_OK;

    arg_count = 1;
    interval = 0;

    if ((argc >= 5) && (string_strcasecmp (argv[1], "-interval") == 0))
    {
        error = NULL;
        interval = (int)strtol (argv[2], &error, 10);
        if (!error || error[0] || (interval < 1))
            interval = 0;
        arg_count = 3;
    }

    error = NULL;
    count = (int)strtol (argv[arg_count], &error, 10);
    if (!error || error[0] || (count < 1))
    {
        /* invalid count */
        gui_chat_printf (NULL,
                         _("%sError: incorrect number"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR]);
        return WEECHAT_RC_OK;
    }

    if (string_is_command_char (argv_eol[arg_count + 1]))
        command = strdup (argv_eol[arg_count + 1]);
    else
    {
        length = strlen (argv_eol[arg_count + 1]) + 2;
        command = malloc (length);
        if (command)
            snprintf (command, length, "/%s", argv_eol[arg_count + 1]);
    }

    if (command)
    {
        input_exec_command (buffer, 1, NULL, command);
        if (count > 1)
        {
            if (interval == 0)
            {
                for (i = 0; i < count - 1; i++)
                {
                    input_exec_command (buffer, 1, NULL, command);
                }
                free (command);
            }
            else
            {
                repeat_args = malloc (2 * sizeof (*repeat_args));
                if (!repeat_args)
                {
                    gui_chat_printf (NULL,
                                     _("%sNot enough memory"),
                                     gui_chat_prefix[GUI_CHAT_PREFIX_ERROR]);
                    return WEECHAT_RC_OK;
                }
                repeat_args[0] = strdup (buffer->full_name);
                repeat_args[1] = command;
                hook_timer (NULL, interval, 0, count - 1,
                            &command_repeat_timer_cb, repeat_args);
            }
        }
        else
        {
            free (command);
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

COMMAND_CALLBACK(save)
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
                gui_chat_printf_date_tags (NULL, 0, GUI_CHAT_TAG_NO_HIGHLIGHT,
                                           "%s%s.%s.%s%s = %s%s",
                                           (message) ? message : "  ",
                                           option->config_file->name,
                                           option->section->name,
                                           option->name,
                                           GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                           GUI_COLOR(GUI_COLOR_CHAT_VALUE),
                                           (CONFIG_BOOLEAN(option) == CONFIG_BOOLEAN_TRUE) ?
                                           "on" : "off");
                break;
            case CONFIG_OPTION_TYPE_INTEGER:
                if (option->string_values)
                {
                    gui_chat_printf_date_tags (NULL, 0, GUI_CHAT_TAG_NO_HIGHLIGHT,
                                               "%s%s.%s.%s%s = %s%s",
                                               (message) ? message : "  ",
                                               option->config_file->name,
                                               option->section->name,
                                               option->name,
                                               GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                               GUI_COLOR(GUI_COLOR_CHAT_VALUE),
                                               option->string_values[CONFIG_INTEGER(option)]);
                }
                else
                {
                    gui_chat_printf_date_tags (NULL, 0, GUI_CHAT_TAG_NO_HIGHLIGHT,
                                               "%s%s.%s.%s%s = %s%d",
                                               (message) ? message : "  ",
                                               option->config_file->name,
                                               option->section->name,
                                               option->name,
                                               GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                               GUI_COLOR(GUI_COLOR_CHAT_VALUE),
                                               CONFIG_INTEGER(option));
                }
                break;
            case CONFIG_OPTION_TYPE_STRING:
                gui_chat_printf_date_tags (NULL, 0, GUI_CHAT_TAG_NO_HIGHLIGHT,
                                           "%s%s.%s.%s%s = \"%s%s%s\"",
                                           (message) ? message : "  ",
                                           option->config_file->name,
                                           option->section->name,
                                           option->name,
                                           GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                           GUI_COLOR(GUI_COLOR_CHAT_VALUE),
                                           CONFIG_STRING(option),
                                           GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS));
                break;
            case CONFIG_OPTION_TYPE_COLOR:
                color_name = gui_color_get_name (CONFIG_COLOR(option));
                gui_chat_printf_date_tags (NULL, 0, GUI_CHAT_TAG_NO_HIGHLIGHT,
                                           "%s%s.%s.%s%s = %s%s",
                                           (message) ? message : "  ",
                                           option->config_file->name,
                                           option->section->name,
                                           option->name,
                                           GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                           GUI_COLOR(GUI_COLOR_CHAT_VALUE),
                                           (color_name) ? color_name : _("(unknown)"));
                break;
            case CONFIG_NUM_OPTION_TYPES:
                /* make C compiler happy */
                break;
        }
    }
    else
    {
        gui_chat_printf_date_tags (NULL, 0, GUI_CHAT_TAG_NO_HIGHLIGHT,
                                   "%s%s.%s.%s",
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

COMMAND_CALLBACK(set)
{
    char *value;
    int number_found, rc;
    struct t_config_option *ptr_option, *ptr_option_before;

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
            {
                gui_chat_printf (NULL,
                                 _("%sOption \"%s\" not found (tip: you can use "
                                   "\"*\" at beginning and/or end of option to "
                                   "see a sublist)"),
                                 gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                 argv[1]);
            }
            else
            {
                gui_chat_printf (NULL,
                                 _("No configuration option found"));
            }
        }
        else
        {
            gui_chat_printf (NULL, "");
            if (argc == 2)
            {
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
            }
            else
            {
                gui_chat_printf (NULL,
                                 NG_("%s%d%s configuration option found",
                                     "%s%d%s configuration options found",
                                     number_found),
                                 GUI_COLOR(GUI_COLOR_CHAT_BUFFER),
                                 number_found,
                                 GUI_COLOR(GUI_COLOR_CHAT));
            }
        }
        return WEECHAT_RC_OK;
    }

    /* set option value */
    config_file_search_with_string (argv[1], NULL, NULL, &ptr_option_before,
                                    NULL);
    value = (string_strcasecmp (argv_eol[2], WEECHAT_CONFIG_OPTION_NULL) == 0) ?
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
            return WEECHAT_RC_OK;
        case WEECHAT_CONFIG_OPTION_SET_OPTION_NOT_FOUND:
            gui_chat_printf (NULL,
                             _("%sError: configuration option \"%s\" not "
                               "found"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             argv[1]);
            return WEECHAT_RC_OK;
        default:
            config_file_search_with_string (argv[1], NULL, NULL,
                                            &ptr_option, NULL);
            if (ptr_option)
            {
                command_set_display_option (ptr_option,
                                            (ptr_option_before) ?
                                            _("Option changed: ") :
                                            _("Option created: "));
            }
            else
                gui_chat_printf (NULL, _("Option changed"));
            break;
    }

    return WEECHAT_RC_OK;
}

/*
 * command_unset: unset/reset config options
 */

COMMAND_CALLBACK(unset)
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
        if (strcmp (argv_eol[1], "*") == 0)
        {
            gui_chat_printf (NULL,
                             _("%sReset of all options is not allowed"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR]);
            return WEECHAT_RC_OK;
        }
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

COMMAND_CALLBACK(upgrade)
{
    char *ptr_binary;
    char *exec_args[7] = { NULL, "-a", "--dir", NULL, "--upgrade", NULL };
    struct stat stat_buf;
    int rc;

    /* make C compiler happy */
    (void) data;
    (void) buffer;
    (void) argv;

    /*
     * it is forbidden to upgrade while there are some background process
     * (hook type "process" or "connect")
     */
    if (weechat_hooks[HOOK_TYPE_PROCESS] || weechat_hooks[HOOK_TYPE_CONNECT])
    {
        gui_chat_printf (NULL,
                         _("%sCan't upgrade: there is one or more background "
                           "process (hook type 'process' or 'connect')"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR]);
        return WEECHAT_RC_OK;
    }

    if (argc > 1)
    {
        ptr_binary = string_expand_home (argv_eol[1]);
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
                return WEECHAT_RC_OK;
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
                return WEECHAT_RC_OK;
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
        return WEECHAT_RC_OK;
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
        return WEECHAT_RC_OK;
    }

    exec_args[0] = ptr_binary;
    exec_args[3] = strdup (weechat_home);

    weechat_quit = 1;
    weechat_upgrading = 1;

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

COMMAND_CALLBACK(uptime)
{
    time_t running_time;
    int day, hour, min, sec;
    char string[512];

    /* make C compiler happy */
    (void) data;
    (void) argv_eol;

    running_time = time (NULL) - weechat_first_start_time;
    day = running_time / (60 * 60 * 24);
    hour = (running_time % (60 * 60 * 24)) / (60 * 60);
    min = ((running_time % (60 * 60 * 24)) % (60 * 60)) / 60;
    sec = ((running_time % (60 * 60 * 24)) % (60 * 60)) % 60;

    if ((argc >= 2) && (string_strcasecmp (argv[1], "-o") == 0))
    {
        snprintf (string, sizeof (string),
                  "WeeChat uptime: %d %s %02d:%02d:%02d, started on %s",
                  day,
                  (day > 1) ? "days" : "day",
                  hour,
                  min,
                  sec,
                  ctime (&weechat_first_start_time));
        string[strlen (string) - 1] = '\0';
        input_data (buffer, string);
    }
    else if ((argc >= 2) && (string_strcasecmp (argv[1], "-ol") == 0))
    {
        snprintf (string, sizeof (string),
                  /* TRANSLATORS: "%s" after "started on" is a date */
                  _("WeeChat uptime: %d %s %02d:%02d:%02d, started on %s"),
                  day,
                  NG_("day", "days", day),
                  hour,
                  min,
                  sec,
                  util_get_time_string (&weechat_first_start_time));
        input_data (buffer, string);
    }
    else
    {
        gui_chat_printf (NULL,
                         /* TRANSLATORS: "%s%s" after "started on" is a date */
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
                         util_get_time_string (&weechat_first_start_time));
    }

    return WEECHAT_RC_OK;
}

/*
 * command_version_display: display WeeChat version
 */

void
command_version_display (struct t_gui_buffer *buffer,
                         int send_to_buffer_as_input,
                         int translated_string)
{
    char string[512];

    if (send_to_buffer_as_input)
    {
        if (translated_string)
        {
            snprintf (string, sizeof (string),
                      "WeeChat %s [%s %s %s]",
                      PACKAGE_VERSION,
                      _("compiled on"),
                      __DATE__,
                      __TIME__);
            input_data (buffer, string);
            if (weechat_upgrade_count > 0)
            {
                snprintf (string, sizeof (string),
                          _("Upgraded %d %s, first start: %s"),
                          weechat_upgrade_count,
                          /* TRANSLATORS: text is: "upgraded xx times" */
                          NG_("time", "times", weechat_upgrade_count),
                          util_get_time_string (&weechat_first_start_time));
                input_data (buffer, string);
            }
        }
        else
        {
            snprintf (string, sizeof (string),
                      "WeeChat %s [%s %s %s]",
                      PACKAGE_VERSION,
                      "compiled on",
                      __DATE__,
                      __TIME__);
            input_data (buffer, string);
            if (weechat_upgrade_count > 0)
            {
                snprintf (string, sizeof (string),
                          "Upgraded %d %s, first start: %s",
                          weechat_upgrade_count,
                          (weechat_upgrade_count > 1) ? "times" : "time",
                          ctime (&weechat_first_start_time));
                string[strlen (string) - 1] = '\0';
                input_data (buffer, string);
            }
        }
    }
    else
    {
        gui_chat_printf (NULL, "%sWeeChat %s %s[%s%s %s %s%s]",
                         GUI_COLOR(GUI_COLOR_CHAT_BUFFER),
                         PACKAGE_VERSION,
                         GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                         GUI_COLOR(GUI_COLOR_CHAT_VALUE),
                         _("compiled on"),
                         __DATE__,
                         __TIME__,
                         GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS));
        if (weechat_upgrade_count > 0)
        {
            gui_chat_printf (NULL,
                             _("Upgraded %d %s, first start: %s"),
                             weechat_upgrade_count,
                             /* TRANSLATORS: text is: "upgraded xx times" */
                             NG_("time", "times", weechat_upgrade_count),
                             util_get_time_string (&weechat_first_start_time));
        }
    }
}

/*
 * command_version: display WeeChat version
 */

COMMAND_CALLBACK(version)
{
    int send_to_buffer_as_input, translated_string;

    /* make C compiler happy */
    (void) data;
    (void) argv_eol;

    send_to_buffer_as_input = 0;
    translated_string = 0;

    if (argc >= 2)
    {
        if (string_strcasecmp (argv[1], "-o") == 0)
            send_to_buffer_as_input = 1;
        else if (string_strcasecmp (argv[1], "-ol") == 0)
        {
            send_to_buffer_as_input = 1;
            translated_string = 1;
        }
    }

    command_version_display (buffer, send_to_buffer_as_input,
                             translated_string);

    return WEECHAT_RC_OK;
}

/*
 * command_wait_timer_cb: callback for timer set by command_wait
 */

int
command_wait_timer_cb (void *data, int remaining_calls)
{
    char **timer_args;
    int i;
    struct t_gui_buffer *ptr_buffer;

    /* make C compiler happy */
    (void) remaining_calls;

    timer_args = (char **)data;

    if (!timer_args)
        return WEECHAT_RC_ERROR;

    if (timer_args[0] && timer_args[1])
    {
        /* search buffer, fallback to core buffer if not found */
        ptr_buffer = gui_buffer_search_by_full_name (timer_args[0]);
        if (!ptr_buffer)
            ptr_buffer = gui_buffer_search_main ();

        /* execute command */
        if (ptr_buffer)
            input_data (ptr_buffer, timer_args[1]);
    }

    for (i = 0; i < 2; i++)
    {
        if (timer_args[i])
            free (timer_args[i]);
    }
    free (timer_args);

    return WEECHAT_RC_OK;
}

/*
 * command_wait: schedule a command execution in future
 */

COMMAND_CALLBACK(wait)
{
    char *pos, *str_number, *error;
    long number, factor, delay;
    char **timer_args;

    /* make C compiler happy */
    (void) data;

    if (argc > 2)
    {
        pos = argv[1];
        while (pos[0] && isdigit ((unsigned char)pos[0]))
        {
            pos++;
        }

        /* default is seconds (1000 milliseconds) */
        factor = 1000;

        if ((pos != argv[1]) && pos[0])
        {
            str_number = string_strndup (argv[1], pos - argv[1]);
            if (strcmp (pos, "ms") == 0)
                factor = 1;
            else if (strcmp (pos, "s") == 0)
                factor = 1000;
            else if (strcmp (pos, "m") == 0)
                factor = 1000 * 60;
            else if (strcmp (pos, "h") == 0)
                factor = 1000 * 60 * 60;
            else
                return WEECHAT_RC_ERROR;
        }
        else
            str_number = strdup (argv[1]);

        if (str_number)
        {
            error = NULL;
            number = strtol (str_number, &error, 10);
            if (error && !error[0])
            {
                free (str_number);
                delay = number * factor;

                /* build arguments for timer callback */
                timer_args = malloc (2 * sizeof (*timer_args));
                if (!timer_args)
                {
                    gui_chat_printf (NULL,
                                     _("%sNot enough memory"),
                                     gui_chat_prefix[GUI_CHAT_PREFIX_ERROR]);
                    return WEECHAT_RC_OK;
                }
                timer_args[0] = strdup (buffer->full_name);
                timer_args[1] = strdup (argv_eol[2]);

                /* schedule command, execute it after "delay" milliseconds */
                hook_timer (NULL, delay, 0, 1,
                            &command_wait_timer_cb, timer_args);

                return WEECHAT_RC_OK;
            }
            free (str_number);
            return WEECHAT_RC_ERROR;
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * command_window: manage windows
 */

COMMAND_CALLBACK(window)
{
    struct t_gui_window *ptr_win;
    char *error;
    long number;
    int win_args;

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

        for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
        {
            gui_chat_printf (NULL, "%s[%s%d%s] (%s%d:%d%s;%s%dx%d%s) ",
                             GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                             GUI_COLOR(GUI_COLOR_CHAT),
                             ptr_win->number,
                             GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                             GUI_COLOR(GUI_COLOR_CHAT),
                             ptr_win->win_x,
                             ptr_win->win_y,
                             GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                             GUI_COLOR(GUI_COLOR_CHAT),
                             ptr_win->win_width,
                             ptr_win->win_height,
                             GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS));
        }

        return WEECHAT_RC_OK;
    }

    /* refresh screen */
    if (string_strcasecmp (argv[1], "refresh") == 0)
    {
        gui_window_ask_refresh (2);
        return WEECHAT_RC_OK;
    }

    /* balance windows */
    if (string_strcasecmp (argv[1], "balance") == 0)
    {
        if (gui_window_balance (gui_windows_tree))
            gui_window_ask_refresh (1);
        return WEECHAT_RC_OK;
    }

    /*
     * search window, for actions related to a given window
     * (default is current window if no number is given)
     */
    ptr_win = gui_current_window;
    win_args = 2;
    if ((argc > 3) && (string_strcasecmp (argv[2], "-window") == 0))
    {
        error = NULL;
        number = strtol (argv[3], &error, 10);
        if (error && !error[0])
            ptr_win = gui_window_search_by_number (number);
        else
            ptr_win = NULL;
        win_args = 4;
    }
    if (!ptr_win)
    {
        /* invalid number */
        gui_chat_printf (NULL,
                         _("%sError: incorrect window number"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR]);
        return WEECHAT_RC_OK;
    }

    /* page up */
    if (string_strcasecmp (argv[1], "page_up") == 0)
    {
        gui_window_page_up (ptr_win);
        return WEECHAT_RC_OK;
    }

    /* page down */
    if (string_strcasecmp (argv[1], "page_down") == 0)
    {
        gui_window_page_down (ptr_win);
        return WEECHAT_RC_OK;
    }

    /* vertical scroll */
    if (string_strcasecmp (argv[1], "scroll") == 0)
    {
        if (argc > win_args)
            gui_window_scroll (ptr_win, argv[win_args]);
        return WEECHAT_RC_OK;
    }

    /* horizontal scroll in window (for buffers with free content) */
    if (string_strcasecmp (argv[1], "scroll_horiz") == 0)
    {
        if ((argc > win_args)
            && (ptr_win->buffer->type == GUI_BUFFER_TYPE_FREE))
        {
            gui_window_scroll_horiz (ptr_win, argv[win_args]);
        }
        return WEECHAT_RC_OK;
    }

    /* scroll up */
    if (string_strcasecmp (argv[1], "scroll_up") == 0)
    {
        gui_window_scroll_up (ptr_win);
        return WEECHAT_RC_OK;
    }

    /* scroll down */
    if (string_strcasecmp (argv[1], "scroll_down") == 0)
    {
        gui_window_scroll_down (ptr_win);
        return WEECHAT_RC_OK;
    }

    /* scroll to top of window */
    if (string_strcasecmp (argv[1], "scroll_top") == 0)
    {
        gui_window_scroll_top (ptr_win);
        return WEECHAT_RC_OK;
    }

    /* scroll to bottom of window */
    if (string_strcasecmp (argv[1], "scroll_bottom") == 0)
    {
        gui_window_scroll_bottom (ptr_win);
        return WEECHAT_RC_OK;
    }

    /* scroll to previous highlight */
    if (string_strcasecmp (argv[1], "scroll_previous_highlight") == 0)
    {
        gui_window_scroll_previous_highlight (ptr_win);
        return WEECHAT_RC_OK;
    }

    /* scroll to next highlight */
    if (string_strcasecmp (argv[1], "scroll_next_highlight") == 0)
    {
        gui_window_scroll_next_highlight (ptr_win);
        return WEECHAT_RC_OK;
    }

    /* scroll to unread marker */
    if (string_strcasecmp (argv[1], "scroll_unread") == 0)
    {
        gui_window_scroll_unread (ptr_win);
        return WEECHAT_RC_OK;
    }

    /* split window horizontally */
    if (string_strcasecmp (argv[1], "splith") == 0)
    {
        if (argc > win_args)
        {
            error = NULL;
            number = strtol (argv[win_args], &error, 10);
            if (error && !error[0]
                && (number > 0) && (number < 100))
            {
                gui_window_split_horizontal (ptr_win, number);
            }
        }
        else
            gui_window_split_horizontal (ptr_win, 50);

        return WEECHAT_RC_OK;
    }

    /* split window vertically */
    if (string_strcasecmp (argv[1], "splitv") == 0)
    {
        if (argc > win_args)
        {
            error = NULL;
            number = strtol (argv[win_args], &error, 10);
            if (error && !error[0]
                && (number > 0) && (number < 100))
            {
                gui_window_split_vertical (ptr_win, number);
            }
        }
        else
            gui_window_split_vertical (ptr_win, 50);

        return WEECHAT_RC_OK;
    }

    /* resize window */
    if (string_strcasecmp (argv[1], "resize") == 0)
    {
        if (argc > win_args)
        {
            if ((argv[win_args][0] == '+') || (argv[win_args][0] == '-'))
            {
                error = NULL;
                number = strtol (argv[win_args] + 1, &error, 10);
                if (error && !error[0])
                {
                    if (argv[win_args][0] == '-')
                        number *= -1;
                    gui_window_resize_delta (ptr_win, number);
                }
            }
            else
            {
                error = NULL;
                number = strtol (argv[win_args], &error, 10);
                if (error && !error[0]
                    && (number > 0) && (number < 100))
                {
                    gui_window_resize (ptr_win, number);
                }
            }
        }
        return WEECHAT_RC_OK;
    }

    /* merge windows */
    if (string_strcasecmp (argv[1], "merge") == 0)
    {
        if (argc > win_args)
        {
            if (string_strcasecmp (argv[win_args], "all") == 0)
                gui_window_merge_all (ptr_win);
            else
            {
                gui_chat_printf (NULL,
                                 _("%sError: unknown option for \"%s\" "
                                   "command"),
                                 gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                 "window merge");
                return WEECHAT_RC_OK;
            }
        }
        else
        {
            if (!gui_window_merge (ptr_win))
            {
                gui_chat_printf (NULL,
                                 _("%sError: can not merge windows, "
                                   "there's no other window with same "
                                   "size near current one"),
                                 gui_chat_prefix[GUI_CHAT_PREFIX_ERROR]);
                return WEECHAT_RC_OK;
            }
        }
        return WEECHAT_RC_OK;
    }

    /* switch to previous window */
    if (string_strcasecmp (argv[1], "-1") == 0)
    {
        gui_window_switch_previous (ptr_win);
        return WEECHAT_RC_OK;
    }

    /* switch to next window */
    if (string_strcasecmp (argv[1], "+1") == 0)
    {
        gui_window_switch_next (ptr_win);
        return WEECHAT_RC_OK;
    }

    /* switch to window above */
    if (string_strcasecmp (argv[1], "up") == 0)
    {
        gui_window_switch_up (ptr_win);
        return WEECHAT_RC_OK;
    }

    /* switch to window below */
    if (string_strcasecmp (argv[1], "down") == 0)
    {
        gui_window_switch_down (ptr_win);
        return WEECHAT_RC_OK;
    }

    /* switch to window on the left */
    if (string_strcasecmp (argv[1], "left") == 0)
    {
        gui_window_switch_left (ptr_win);
        return WEECHAT_RC_OK;
    }

    /* switch to window on the right */
    if (string_strcasecmp (argv[1], "right") == 0)
    {
        gui_window_switch_right (ptr_win);
        return WEECHAT_RC_OK;
    }

    /* swap windows */
    if (string_strcasecmp (argv[1], "swap") == 0)
    {
        if (argc > win_args)
        {
            if (string_strcasecmp (argv[win_args], "up") == 0)
                gui_window_swap (ptr_win, 1);
            else if (string_strcasecmp (argv[win_args], "down") == 0)
                gui_window_swap (ptr_win, 3);
            else if (string_strcasecmp (argv[win_args], "left") == 0)
                gui_window_swap (ptr_win, 4);
            else if (string_strcasecmp (argv[win_args], "right") == 0)
                gui_window_swap (ptr_win, 2);
            else
            {
                gui_chat_printf (NULL,
                                 _("%sError: unknown option for \"%s\" "
                                   "command"),
                                 gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                 "window swap");
                return WEECHAT_RC_OK;
            }
        }
        else
        {
            gui_window_swap (ptr_win, 0);
        }
        return WEECHAT_RC_OK;
    }

    /* zoom window */
    if (string_strcasecmp (argv[1], "zoom") == 0)
    {
        gui_window_zoom (ptr_win);
        return WEECHAT_RC_OK;
    }

    /* jump to window by buffer number */
    if (string_strncasecmp (argv[1], "b", 1) == 0)
    {
        error = NULL;
        number = strtol (argv[1] + 1, &error, 10);
        if (error && !error[0])
        {
            gui_window_switch_by_buffer (ptr_win, number);
            return WEECHAT_RC_OK;
        }
    }

    /* jump to window by number */
    error = NULL;
    number = strtol (argv[1], &error, 10);
    if (error && !error[0])
    {
        gui_window_switch_by_number (number);
        return WEECHAT_RC_OK;
    }

    gui_chat_printf (NULL,
                     _("%sError: unknown option for \"%s\" "
                       "command"),
                     gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                     "window");
    return WEECHAT_RC_OK;
}

/*
 * command_init: hook WeeChat commands
 */

void
command_init ()
{
    hook_command (NULL, "away",
                  N_("toggle away status"),
                  N_("[-all] [<message>]"),
                  N_("   -all: toggle away status on all connected "
                     "servers\n"
                     "message: message for away (if no message is "
                     "given, away status is removed)"),
                  "-all", &command_away, NULL);
    hook_command (NULL, "bar",
                  N_("manage bars"),
                  N_("list|listfull|listitems"
                     " || add <name> <type>[,<cond1>[,<cond2>...]] <position> "
                     "<size> <separator> <item1>[,<item2>...]"
                     " || default [input|title|status|nicklist]"
                     " || del <name>|-all"
                     " || set <name> <option> <value>"
                     " || hide|show|toggle <name>"
                     " || scroll <name> <window> <scroll_value>"),
                  N_("         list: list all bars\n"
                     "     listfull: list all bars (verbose)\n"
                     "    listitems: list all bar items\n"
                     "          add: add a new bar\n"
                     "         name: name of bar (must be unique)\n"
                     "         type:   root: outside windows,\n"
                     "               window: inside windows, with optional "
                     "conditions (see below)\n"
                     "    cond1,...: condition(s) for displaying bar (only for "
                     "type \"window\"):\n"
                     "                 active: on active window\n"
                     "               inactive: on inactive windows\n"
                     "               nicklist: on windows with nicklist\n"
                     "               without condition, bar is always displayed\n"
                     "     position: bottom, top, left or right\n"
                     "         size: size of bar (in chars)\n"
                     "    separator: 1 for using separator (line), 0 or nothing "
                     "means no separator\n"
                     "    item1,...: items for this bar (items can be separated "
                     "by comma (space between items) or \"+\" (glued items))\n"
                     "      default: create a default bar (all default bars "
                     "if no bar name is given)\n"
                     "          del: delete a bar (or all bars with -all)\n"
                     "          set: set a value for a bar property\n"
                     "       option: option to change (for options list, look "
                     "at /set weechat.bar.<barname>.*)\n"
                     "        value: new value for option\n"
                     "         hide: hide a bar\n"
                     "         show: show an hidden bar\n"
                     "       toggle: hide/show a bar\n"
                     "       scroll: scroll bar\n"
                     "       window: window number (use '*' for current window "
                     "or for root bars)\n"
                     " scroll_value: value for scroll: 'x' or 'y' (optional), "
                     "followed by '+', '-', 'b' (beginning) or 'e' (end), "
                     "value (for +/-), and optional % (to scroll by % of "
                     "width/height, otherwise value is number of chars)\n\n"
                     "Examples:\n"
                     "  create a bar with time, buffer number + name, and completion:\n"
                     "    /bar add mybar root bottom 1 0 [time],buffer_number+:+buffer_name,completion\n"
                     "  hide a bar:\n"
                     "    /bar hide mybar\n"
                     "  scroll nicklist 10 lines down on current buffer:\n"
                     "    /bar scroll nicklist * y+10\n"
                     "  scroll to end of nicklist on current buffer:\n"
                     "    /bar scroll nicklist * ye"),
                  "list"
                  " || listfull"
                  " || listitems"
                  " || add %(bars_names) root|window bottom|top|left|right"
                  " || default input|title|status|nicklist|%*"
                  " || del %(bars_names)|-all"
                  " || set %(bars_names) %(bars_options)"
                  " || hide %(bars_names)"
                  " || show %(bars_names)"
                  " || toggle %(bars_names)"
                  " || scroll %(bars_names) %(windows_numbers)|*",
                  &command_bar, NULL);
    hook_command (NULL, "buffer",
                  N_("manage buffers"),
                  N_("list"
                     " || clear [<number>|<name>|-merged|-all]"
                     " || move|merge <number>"
                     " || swap <number1>|<name1> [<number2>|<name2>]"
                     " || unmerge [<number>|-all]"
                     " || close [<n1>[-<n2>]|<name>]"
                     " || notify <level>"
                     " || localvar"
                     " || set <property> <value>"
                     " || get <property>"
                     " || <number>|<name>"),
                  N_("    list: list buffers (without argument, this list is "
                     "displayed)\n"
                     "   clear: clear buffer content (number for a buffer, "
                     "-merged for merged buffers, -all for all buffers, or "
                     "nothing for current buffer)\n"
                     "    move: move buffer in the list (may be relative, for "
                     "example -1)\n"
                     "    swap: swap two buffers (swap with current buffer if "
                     "only one number/name given)\n"
                     "   merge: merge current buffer to another buffer (chat "
                     "area will be mix of both buffers)\n"
                     "          (by default ctrl-x switches between merged "
                     "buffers)\n"
                     " unmerge: unmerge buffer from other buffers which have "
                     "same number\n"
                     "   close: close buffer (number/range or name is optional)\n"
                     "  notify: set notify level for current buffer: this "
                     "level determines whether buffer will be added to "
                     "hotlist or not:\n"
                     "               none: never\n"
                     "          highlight: for highlights only\n"
                     "            message: for messages from users + highlights\n"
                     "                all: all messages\n"
                     "              reset: reset to default value (all)\n"
                     "localvar: display local variables for current buffer\n"
                     "     set: set a property for current buffer\n"
                     "     get: display a property of current buffer\n"
                     "  number: jump to buffer by number, possible prefix:\n"
                     "          '+': relative jump, add number to current\n"
                     "          '-': relative jump, sub number to current\n"
                     "          '*': jump to number, using option "
                     "\"weechat.look.jump_current_to_previous_buffer\"\n"
                     "    name: jump to buffer by (partial) name\n\n"
                     "Examples:\n"
                     "  clear current buffer:\n"
                     "    /buffer clear\n"
                     "  move buffer to number 5:\n"
                     "    /buffer move 5\n"
                     "  swap buffer 1 with 3:\n"
                     "    /buffer swap 1 3\n"
                     "  swap buffer #weechat with current buffer:\n"
                     "    /buffer swap #weechat\n"
                     "  merge with core buffer:\n"
                     "    /buffer merge 1\n"
                     "  unmerge buffer:\n"
                     "    /buffer unmerge\n"
                     "  close current buffer:\n"
                     "    /buffer close\n"
                     "  close buffers 5 to 7:\n"
                     "    /buffer close 5-7\n"
                     "  jump to #weechat:\n"
                     "    /buffer #weechat\n"
                     "  jump to next buffer:\n"
                     "    /buffer +1"),
                  "clear -merged|-all|%(buffers_numbers)|%(buffers_plugins_names)"
                  " || move %(buffers_numbers)"
                  " || swap %(buffers_numbers)"
                  " || merge %(buffers_numbers)"
                  " || unmerge %(buffers_numbers)|-all"
                  " || close %(buffers_plugins_names)"
                  " || list"
                  " || notify reset|none|highlight|message|all"
                  " || localvar"
                  " || set %(buffer_properties_set)"
                  " || get %(buffer_properties_get)"
                  " || %(buffers_plugins_names)|%(buffers_names)|"
                  "%(irc_channels)|%(irc_privates)|%(buffers_numbers)",
                  &command_buffer, NULL);
    hook_command (NULL, "color",
                  N_("define color aliases and display palette of colors"),
                  N_("alias <color> <name>"
                     " || unalias <color>"
                     " || reset"),
                  N_("  alias: add an alias for a color\n"
                     "unalias: delete an alias\n"
                     "  color: color number (greater than or equal to 0, max "
                     "depends on terminal, commonly 63 or 255)\n"
                     "   name: alias name for color (for example: \"orange\")\n"
                     "  reset: reset all color pairs (required when no more "
                     "color pairs are available if automatic reset is disabled, "
                     "see option weechat.look.color_pairs_auto_reset)\n\n"
                     "Without argument, this command displays colors in a new "
                     "buffer.\n\n"
                     "Examples:\n"
                     "  add alias \"orange\" for color 214:\n"
                     "    /color alias 214 orange\n"
                     "  delete color 214:\n"
                     "    /color unalias 214"),
                  "alias %(palette_colors)"
                  " || unalias %(palette_colors)"
                  " || reset",
                  &command_color, NULL);
    hook_command (NULL, "command",
                  N_("launch explicit WeeChat or plugin command"),
                  N_("<plugin> <command>"),
                  N_(" plugin: plugin name ('weechat' for WeeChat internal "
                     "command)\n"
                     "command: command to execute (a '/' is automatically "
                     "added if not found at beginning of command)"),
                  "%(plugins_names)|" PLUGIN_CORE " %(plugins_commands)",
                  &command_command, NULL);
    hook_command (NULL, "cursor",
                  N_("free movement of cursor on screen to execute actions on "
                     "specific areas of screen"),
                  N_("go chat|<bar>|<x>,<y>"
                     " || move up|down|left|right|area_up|area_down|area_left|"
                     "area_right"
                     " || stop"),
                  N_("  go: move cursor to chat area, a bar (using bar name) "
                     "or coordinates \"x,y\"\n"
                     "move: move cursor with direction\n"
                     "stop: stop cursor mode\n\n"
                     "Without argument, this command toggles cursor mode.\n\n"
                     "When mouse is enabled (see /help mouse), by default a "
                     "middle click will start cursor mode at this point.\n\n"
                     "Examples:\n"
                     "  go to nicklist:\n"
                     "    /cursor go nicklist\n"
                     "  go to coordinates x=10, y=5:\n"
                     "    /cursor go 10,5"),
                  "go %(cursor_areas)"
                  " || move up|down|left|right|area_up|area_down|area_left|"
                  "area_right"
                  " || stop",
                  &command_cursor, NULL);
    hook_command (NULL, "debug",
                  N_("control debug for core/plugins"),
                  N_("list"
                     " || set <plugin> <level>"
                     " || dump [<plugin>]"
                     " || buffer|color|infolists|memory|tags|term|windows"
                     " || mouse|cursor [verbose]"
                     " || hdata [free]"),
                  N_("     list: list plugins with debug levels\n"
                     "      set: set debug level for plugin\n"
                     "   plugin: name of plugin (\"core\" for WeeChat core)\n"
                     "    level: debug level for plugin (0 = disable debug)\n"
                     "     dump: save memory dump in WeeChat log file (same "
                     "dump is written when WeeChat crashes)\n"
                     "   buffer: dump buffer content with hexadecimal values "
                     "in log file\n"
                     "    color: display infos about current color pairs\n"
                     "   cursor: toggle debug for cursor mode\n"
                     "    hdata: display infos about hdata (with free: remove "
                     "all hdata in memory)\n"
                     "    hooks: display infos about hooks\n"
                     "infolists: display infos about infolists\n"
                     "   memory: display infos about memory usage\n"
                     "    mouse: toggle debug for mouse\n"
                     "     tags: display tags for lines\n"
                     "     term: display infos about terminal\n"
                     "  windows: display windows tree"),
                  "list"
                  " || set %(plugins_names)|core"
                  " || dump %(plugins_names)|core"
                  " || buffer"
                  " || color"
                  " || cursor verbose"
                  " || hdata free"
                  " || hooks"
                  " || infolists"
                  " || memory"
                  " || mouse verbose"
                  " || tags"
                  " || term"
                  " || windows",
                  &command_debug, NULL);
    hook_command (NULL, "filter",
                  N_("filter messages in buffers, to hide/show them according "
                     "to tags or regex"),
                  N_("list"
                     " || enable|disable|toggle [<name>]"
                     " || add <name> <buffer>[,<buffer>...] <tags> <regex>"
                     " || del <name>|-all"),
                  N_("   list: list all filters\n"
                     " enable: enable filters (filters are enabled by "
                      "default)\n"
                     "disable: disable filters\n"
                     " toggle: toggle filters\n"
                     "   name: filter name\n"
                     "    add: add a filter\n"
                     "    del: delete a filter\n"
                     "   -all: delete all filters\n"
                     " buffer: comma separated list of buffers where filter "
                     "is active:\n"
                     "         - this is full name including plugin (example: "
                     "\"irc.freenode.#weechat\")\n"
                     "         - \"*\" means all buffers\n"
                     "         - a name starting with '!' is excluded\n"
                     "         - name can start or end with '*' to "
                     "match many buffers\n"
                     "   tags: comma separated list of tags, for "
                     "example: \"irc_join,irc_part,irc_quit\"\n"
                     "  regex: regular expression to search in line\n"
                     "         - use '\\t' to separate prefix from message, special "
                     "chars like '|' must be escaped: '\\|'\n"
                     "         - if regex starts with '!', then matching "
                     "result is reversed (use '\\!' to start with '!')\n"
                     "         - two regular expressions are created: one for "
                     "prefix and one for message\n"
                     "         - regex are case insensitive, they can start by "
                     "\"(?-i)\" to become case sensitive\n\n"
                     "The default key alt+'=' toggles filtering on/off.\n\n"
                     "Tags most commonly used:\n"
                     "  no_filter, no_highlight, no_log, log0..log9 (log level),\n"
                     "  notify_none, notify_message, notify_private, "
                     "notify_highlight,\n"
                     "  nick_xxx (xxx is nick in message), "
                     "prefix_nick_ccc (ccc is color of nick),\n"
                     "  irc_xxx (xxx is command name or number, see /server raw),\n"
                     "  irc_numeric, irc_error, irc_action, irc_ctcp, "
                     "irc_ctcp_reply, irc_smart_filter, away_info.\n"
                     "To see tags for lines in buffers: /debug tags\n\n"
                     "Examples:\n"
                     "  use IRC smart filter on all buffers:\n"
                     "    /filter add irc_smart * irc_smart_filter *\n"
                     "  use IRC smart filter on all buffers except those with "
                     "\"#weechat\" in name:\n"
                     "    /filter add irc_smart *,!*#weechat* irc_smart_filter *\n"
                     "  filter all IRC join/part/quit messages:\n"
                     "    /filter add joinquit * irc_join,irc_part,irc_quit *\n"
                     "  filter nicks displayed when joining channels or with /names:\n"
                     "    /filter add nicks * irc_366 *\n"
                     "  filter nick \"toto\" on IRC channel #weechat:\n"
                     "    /filter add toto irc.freenode.#weechat nick_toto *\n"
                     "  filter lines containing \"weechat sucks\" on IRC "
                     "channel #weechat:\n"
                     "    /filter add sucks irc.freenode.#weechat * weechat sucks"),
                  "list"
                  " || enable %(filters_names)"
                  " || disable %(filters_names)"
                  " || toggle %(filters_names)"
                  " || add %(filters_names) %(buffers_plugins_names)|*"
                  " || del %(filters_names)|-all",
                  &command_filter, NULL);
    hook_command (NULL, "help",
                  N_("display help about commands and options"),
                  N_("-list|-listfull [<plugin> [<plugin>...]]"
                     " || <command>"
                     " || <option>"),
                  N_("    -list: list commands, by plugin (without argument, "
                     "this list is displayed)\n"
                     "-listfull: list commands with description, by plugin\n"
                     "   plugin: list commands for this plugin\n"
                     "  command: a command name\n"
                     "   option: an option name (use /set to see list)"),
                  "-list|-listfull|%(commands)|%(config_options) "
                  "%(plugins_names)|" PLUGIN_CORE "|%*",
                  &command_help, NULL);
    hook_command (NULL, "history",
                  N_("show buffer command history"),
                  N_("clear"
                     " || <value>"),
                  N_("clear: clear history\n"
                     "value: number of history entries to show"),
                  "clear",
                  &command_history, NULL);
    hook_command (NULL, "input",
                  N_("functions for command line"),
                  N_("<action> [<arguments>]"),
                  N_("list of actions:\n"
                     "  return: simulate key \"enter\"\n"
                     "  complete_next: complete word with next completion\n"
                     "  complete_previous: complete word with previous "
                     "completion\n"
                     "  search_text: search text in buffer\n"
                     "  search_switch_case: switch exact case for search\n"
                     "  search_previous: search previous line\n"
                     "  search_next: search next line\n"
                     "  search_stop: stop search\n"
                     "  delete_previous_char: delete previous char\n"
                     "  delete_next_char: delete next char\n"
                     "  delete_previous_word: delete previous word\n"
                     "  delete_next_word: delete next word\n"
                     "  delete_beginning_of_line: delete from beginning of "
                     "line until cursor\n"
                     "  delete_end_of_line: delete from cursor until end of "
                     "line\n"
                     "  delete_line: delete entire line\n"
                     "  clipboard_paste: paste from clipboard\n"
                     "  transpose_chars: transpose two chars\n"
                     "  undo: undo last command line action\n"
                     "  redo: redo last command line action\n"
                     "  move_beginning_of_line: move cursor to beginning of "
                     "line\n"
                     "  move_end_of_line: move cursor to end of line\n"
                     "  move_previous_char: move cursor to previous char\n"
                     "  move_next_char: move cursor to next char\n"
                     "  move_previous_word: move cursor to previous word\n"
                     "  move_next_word: move cursor to next word\n"
                     "  history_previous: recall previous command in current "
                     "buffer history\n"
                     "  history_next: recall next command in current buffer "
                     "history\n"
                     "  history_global_previous: recall previous command in "
                     "global history\n"
                     "  history_global_next: recall next command in global "
                     "history\n"
                     "  jump_smart: jump to next buffer with activity\n"
                     "  jump_last_buffer: jump to last buffer\n"
                     "  jump_last_buffer_displayed: jump to last buffer "
                     "displayed (before last jump to a buffer)\n"
                     "  jump_previously_visited_buffer: jump to previously "
                     "visited buffer\n"
                     "  jump_next_visited_buffer: jump to next visited buffer\n"
                     "  hotlist_clear: clear hotlist\n"
                     "  grab_key: grab a key (optional argument: delay for end "
                     "of grab, default is 500 milliseconds)\n"
                     "  grab_key_command: grab a key with its associated "
                     "command (optional argument: delay for end of grab, "
                     "default is 500 milliseconds)\n"
                     "  grab_mouse: grab mouse event code\n"
                     "  grab_mouse_area: grab mouse event code with area\n"
                     "  set_unread: set unread marker for all buffers\n"
                     "  set_unread_current_buffer: set unread marker for "
                     "current buffer\n"
                     "  switch_active_buffer: switch to next merged buffer\n"
                     "  switch_active_buffer_previous: switch to previous "
                     "merged buffer\n"
                     "  insert: insert text in command line\n"
                     "  paste_start: start paste (bracketed paste mode)\n"
                     "  paste_stop: stop paste (bracketed paste mode)\n\n"
                     "This command is used by key bindings or plugins."),
                  "return|complete_next|complete_previous|search_text|"
                  "search_switch_case|search_previous|search_next|search_stop|"
                  "delete_previous_char|delete_next_char|"
                  "delete_previous_word|delete_next_word|"
                  "delete_beginning_of_line|delete_end_of_line|"
                  "delete_line|clipboard_paste|transpose_chars|undo|redo|"
                  "move_beginning_of_line|move_end_of_line|"
                  "move_previous_char|move_next_char|move_previous_word|"
                  "move_next_word|history_previous|history_next|"
                  "history_global_previous|history_global_next|"
                  "jump_smart|jump_last_buffer|jump_previously_visited_buffer|"
                  "jump_next_visited_buffer|hotlist_clear|grab_key|"
                  "grab_key_command|grab_mouse|grab_mouse_area|set_unread|"
                  "set_unread_current_buffer|switch_active_buffer|"
                  "switch_active_buffer_previous|insert|paste_start|paste_stop",
                  &command_input, NULL);
    hook_command (NULL, "key",
                  N_("bind/unbind keys"),
                  N_("list|listdefault|listdiff [<context>]"
                     " || bind <key> [<command> [<args>]]"
                     " || bindctxt <context> <key> [<command> [<args>]]"
                     " || unbind <key>"
                     " || unbindctxt <context> <key>"
                     " || reset <key>"
                     " || resetctxt <context> <key>"
                     " || resetall -yes [<context>]"
                     " || missing [<context>]"),
                  N_("       list: list all current keys (without argument, "
                     "this list is displayed)\n"
                     "listdefault: list default keys\n"
                     "   listdiff: list differences between current and "
                      "default keys (keys added, redefined or deleted)\n"
                     "    context: name of context (\"default\" or "
                     "\"search\")\n"
                     "       bind: bind a command to a key or display command "
                     "bound to key (for context \"default\")\n"
                     "   bindctxt: bind a command to a key or display command "
                     "bound to key, for given context\n"
                     "    command: command (many commands can be separated by "
                     "semicolons)\n"
                     "     unbind: remove a key binding (for context "
                     "\"default\")\n"
                     " unbindctxt: remove a key binding for given context\n"
                     "      reset: reset a key to default binding (for "
                     "context \"default\")\n"
                     "  resetctxt: reset a key to default binding, for given "
                     "context\n"
                     "   resetall: restore bindings to the default values and "
                     "delete ALL personal bindings (use carefully!)\n"
                     "    missing: add missing keys (using default bindings), "
                     "useful after installing new WeeChat version\n\n"
                     "When binding a command to a key, it is recommended to "
                     "use key alt+k (or Esc then k), and then press the key "
                     "to bind: this will insert key code in command line.\n\n"
                     "For context \"mouse\" (possible in context \"cursor\" "
                     "too), key has format: \"@area:key\" or "
                     "\"@area1>area2:key\" where area can be:\n"
                     "          *: any area on screen\n"
                     "       chat: chat area (any buffer)\n"
                     "  chat(xxx): char area for buffer with name \"xxx\" (full "
                     "name including plugin)\n"
                     "     bar(*): any bar\n"
                     "   bar(xxx): bar \"xxx\"\n"
                     "    item(*): any bar item\n"
                     "  item(xxx): bar item \"xxx\"\n"
                     "The key can start or end with '*' to match many mouse "
                     "events.\n"
                     "A special value for command with format \"hsignal:name\" "
                     "can be used for context mouse, this will send the hsignal "
                     "\"name\" with the focus hashtable as argument.\n"
                     "Another special value \"-\" can be used to disable key "
                     "(it will be ignored when looking for keys).\n\n"
                     "Examples:\n"
                     "  key alt-x to toggle nicklist bar:\n"
                     "    /key bind meta-x /bar toggle nicklist\n"
                     "  key alt-r to jump to #weechat IRC channel:\n"
                     "    /key bind meta-r /buffer #weechat\n"
                     "  restore default binding for key alt-r:\n"
                     "    /key reset meta-r\n"
                     "  key \"tab\" to stop search in buffer:\n"
                     "    /key bindctxt search ctrl-I /input search_stop\n"
                     "  middle button of mouse on a nick to retrieve info on "
                     "nick:\n"
                     "    /key bindctxt mouse @item(buffer_nicklist):button3 "
                     "/msg nickserv info ${nick}"),
                  "list %(keys_contexts)"
                  " || listdefault %(keys_contexts)"
                  " || listdiff %(keys_contexts)"
                  " || bind %(keys_codes) %(commands)"
                  " || bindctxt %(keys_contexts) %(keys_codes) %(commands)"
                  " || unbind %(keys_codes)"
                  " || unbindctxt %(keys_contexts) %(keys_codes)"
                  " || reset %(keys_codes_for_reset)"
                  " || resetctxt %(keys_contexts) %(keys_codes_for_reset)"
                  " || resetall %- %(keys_contexts)"
                  " || missing %(keys_contexts)",
                  &command_key, NULL);
    hook_command (NULL, "layout",
                  N_("save/apply/reset layout for buffers and windows"),
                  N_("save [buffers|windows]"
                     " || apply [buffers|windows]"
                     " || reset [buffers|windows]"),
                  N_("   save: save current layout\n"
                     "  apply: apply saved layout\n"
                     "  reset: remove saved layout\n"
                     "buffers: save/apply only buffers (order of buffers)\n"
                     "windows: save/apply only windows (buffer displayed by "
                     "each window)\n\n"
                     "Without argument, this command displays saved layout."),
                  "save|apply|reset buffers|windows",
                  &command_layout, NULL);
    hook_command (NULL, "mouse",
                  N_("mouse control"),
                  N_("enable|disable|toggle [<delay>]"),
                  N_(" enable: enable mouse\n"
                     "disable: disable mouse\n"
                     " toggle: toggle mouse\n"
                     "  delay: delay (in seconds) after which initial mouse "
                     "state is restored (useful to temporarily disable mouse)\n\n"
                     "The mouse state is saved in option \"weechat.look.mouse\".\n\n"
                     "Examples:\n"
                     "  enable mouse:\n"
                     "    /mouse enable\n"
                     "  toggle mouse for 5 seconds:\n"
                     "    /mouse toggle 5"),
                  "enable"
                  " || disable"
                  " || toggle",
                  &command_mouse, NULL);
    hook_command (NULL, "mute",
                  N_("execute a command silently"),
                  N_("[-current | -buffer <name> | -all] command"),
                  N_("-current: no output on current buffer\n"
                     " -buffer: no output on specified buffer\n"
                     "    name: full buffer name (examples: "
                     "\"irc.server.freenode\", \"irc.freenode.#weechat\")\n"
                     "    -all: no output on ALL buffers\n"
                     " command: command to execute silently (a '/' is "
                     "automatically added if not found at beginning of "
                     "command)\n\n"
                     "If no target is specified (-current, -buffer or -all), "
                     "then default is to mute WeeChat core buffer only.\n\n"
                     "Examples:\n"
                     "  config save:\n"
                     "    /mute save\n"
                     "  message to current IRC channel:\n"
                     "    /mute -current msg * hi!\n"
                     "  message to #weechat channel:\n"
                     "    /mute -buffer irc.freenode.#weechat msg #weechat hi!"),
                  "-current|-buffer|-all|%(commands) %(commands)|%*",
                  &command_mute, NULL);
    hook_command (NULL, "plugin",
                  N_("list/load/unload plugins"),
                  N_("list|listfull [<name>]"
                     " || load <filename> [<arguments>]"
                     " || autoload [<arguments>]"
                     " || reload [<name> [<arguments>]]"
                     " || unload [<name>]"),
                  N_("     list: list loaded plugins\n"
                     " listfull: list loaded plugins (verbose)\n"
                     "     load: load a plugin\n"
                     " autoload: autoload plugins in system or user directory\n"
                     "   reload: reload a plugin (if no name given, unload "
                     "all plugins, then autoload plugins)\n"
                     "   unload: unload a plugin (if no name given, unload "
                     "all plugins)\n"
                     " filename: plugin (file) to load\n"
                     "     name: a plugin name\n"
                     "arguments: arguments given to plugin on load\n\n"
                     "Without argument, this command lists loaded plugins."),
                  "list %(plugins_names)"
                  " || listfull %(plugins_names)"
                  " || load %(filename)"
                  " || autoload"
                  " || reload %(plugins_names)"
                  " || unload %(plugins_names)",
                  &command_plugin, NULL);
    hook_command (NULL, "proxy",
                  N_("manage proxies"),
                  N_("list"
                     " || add <name> <type> <address> <port> [<username> "
                     "[<password>]]"
                     " || del <name>|-all"
                     " || set <name> <option> <value>"),
                  N_("    list: list all proxies\n"
                     "     add: add a new proxy\n"
                     "    name: name of proxy (must be unique)\n"
                     "    type: http, socks4 or socks5\n"
                     " address: IP or hostname\n"
                     "    port: port\n"
                     "username: username (optional)\n"
                     "password: password (optional)\n"
                     "     del: delete a proxy (or all proxies with -all)\n"
                     "     set: set a value for a proxy property\n"
                     "  option: option to change (for options list, look "
                     "at /set weechat.proxy.<proxyname>.*)\n"
                     "   value: new value for option\n\n"
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
                  "list"
                  " || add %(proxies_names) http|socks4|socks5"
                  " || del %(proxies_names)"
                  " || set %(proxies_names) %(proxies_options)",
                  &command_proxy, NULL);
    hook_command (NULL, "quit",
                  N_("quit WeeChat"),
                  N_("[-yes] [<arguments>]"),
                  N_("     -yes: required if option weechat.look.confirm_quit "
                     "is enabled\n"
                     "arguments: text sent with signal \"quit\"\n"
                     "           (for example irc plugin uses this text to "
                     "send quit message to server)"),
                  "",
                  &command_quit, NULL);
    hook_command (NULL, "reload",
                  N_("reload configuration files from disk"),
                  N_("[<file> [<file>...]]"),
                  N_("file: configuration file to reload (without extension "
                     "\".conf\")\n\n"
                     "Without argument, all files (WeeChat and plugins) are "
                     "reloaded."),
                  "%(config_files)|%*",
                  &command_reload, NULL);
    hook_command (NULL, "repeat",
                  N_("execute a command several times"),
                  N_("[-interval <delay>] <count> <command>"),
                  N_("  delay: delay between execution of commands (in "
                     "milliseconds)\n"
                     "  count: number of times to execute command\n"
                     "command: command to execute (a '/' is automatically "
                     "added if not found at beginning of command)\n\n"
                     "All commands are executed on buffer where this command "
                     "was issued.\n\n"
                     "Example:\n"
                     "  scroll 2 pages up:\n"
                     "    /repeat 2 /window page_up"),
                  "%- %(commands)",
                  &command_repeat, NULL);
    hook_command (NULL, "save",
                  N_("save configuration files to disk"),
                  N_("[<file> [<file>...]]"),
                  N_("file: configuration file to save (without extension "
                     "\".conf\")\n\n"
                     "Without argument, all files (WeeChat and plugins) are "
                     "saved."),
                  "%(config_files)|%*",
                  &command_save, NULL);
    hook_command (NULL, "set",
                  N_("set config options"),
                  N_("[<option> [<value>]]"),
                  N_("option: name of an option (can start or end with '*' "
                     "to list many options)\n"
                     " value: new value for option\n\n"
                     "New value can be, according to variable type:\n"
                     "  boolean: on, off or toggle\n"
                     "  integer: number, ++number or --number\n"
                     "  string : any string (\"\" for empty string)\n"
                     "  color  : color name, ++number or --number\n\n"
                     "For all types, you can use null to remove "
                     "option value (undefined value). This works only "
                     "for some special plugin variables.\n\n"
                     "Examples:\n"
                     "  display options about highlight:\n"
                     "    /set *highlight*\n"
                     "  add a word to highlight:\n"
                     "    /set weechat.look.highlight \"word\""),
                  "%(config_options) %(config_option_values)",
                  &command_set, NULL);
    hook_command (NULL, "unset",
                  N_("unset/reset config options"),
                  N_("<option>"),
                  N_("option: name of an option (may begin or end with \"*\" "
                     "to mass-reset options, use carefully!)\n\n"
                     "According to option, it's reset (for standard options) "
                     "or removed (for optional settings, like server values).\n\n"
                     "Examples:\n"
                     "  reset one option:\n"
                     "    /unset weechat.look.item_time_format\n"
                     "  reset all color options:\n"
                     "    /unset weechat.color.*"),
                  "%(config_options)",
                  &command_unset, NULL);
    hook_command (NULL, "upgrade",
                  N_("upgrade WeeChat without disconnecting from servers"),
                  N_("[<path_to_binary>]"),
                  N_("path_to_binary: path to WeeChat binary (default is "
                     "current binary)\n\n"
                     "This command upgrades and reloads a running WeeChat "
                     "session. The new WeeChat binary must have been compiled "
                     "or installed with a package manager before running this "
                     "command.\n\n"
                     "Note: SSL connections are lost during upgrade, because "
                     "reload of SSL sessions is currently not possible with "
                     "GnuTLS. There is automatic reconnection after "
                     "upgrade.\n\n"
                     "Upgrade process has 4 steps:\n"
                     "  1. save session into files for core and plugins "
                     "(buffers, history, ..)\n"
                     "  2. unload all plugins (configuration files (*.conf) "
                     "are written on disk)\n"
                     "  3. save WeeChat configuration (weechat.conf)\n"
                     "  4. execute new WeeChat binary and reload session."),
                  "%(filename)",
                  &command_upgrade, NULL);
    hook_command (NULL, "uptime",
                  N_("show WeeChat uptime"),
                  "[-o | -ol]",
                  N_(" -o: send uptime to current buffer as input (english "
                     "string)\n"
                     "-ol: send uptime to current buffer as input (translated "
                     "string)"),
                  "-o|-ol",
                  &command_uptime, NULL);
    hook_command (NULL, "version",
                  N_("show WeeChat version and compilation date"),
                  "[-o | -ol]",
                  N_(" -o: send version to current buffer as input (english "
                     "string)\n"
                     "-ol: send version to current buffer as input (translated "
                     "string)"),
                  "-o|-ol",
                  &command_version, NULL);
    hook_command (NULL, "wait",
                  N_("schedule a command execution in future"),
                  N_("<number>[<unit>] <command>"),
                  N_(" number: amount of time to wait (integer number)\n"
                     "   unit: optional, values are:\n"
                     "           ms: milliseconds\n"
                     "            s: seconds (default)\n"
                     "            m: minutes\n"
                     "            h: hours\n"
                     "command: command to execute (or text to send to buffer "
                     "if command does not start with '/')\n\n"
                     "Note: command is executed on buffer where /wait was "
                     "executed (if buffer is not found (for example if it has "
                     "been closed before execution of command), then command "
                     "is executed on WeeChat core buffer).\n\n"
                     "Examples:\n"
                     "  join channel in 10 seconds:\n"
                     "    /wait 10 /join #test\n"
                     "  set away in 15 minutes:\n"
                     "    /wait 15m /away -all I'm away\n"
                     "  say 'hello' in 2 minutes:\n"
                     "    /wait 2m hello"),
                  "%- %(commands)",
                  &command_wait, NULL);
    hook_command (NULL, "window",
                  N_("manage windows"),
                  N_("list"
                     " || -1|+1|b#|up|down|left|right [-window <number>]"
                     " || <number>"
                     " || splith|splitv [-window <number>] [<pct>]"
                     " || resize [-window <number>] [+/-]<pct>"
                     " || balance"
                     " || merge [-window <number>] [all]"
                     " || page_up|page_down [-window <number>]"
                     " || refresh"
                     " || scroll [-window <number>] [+/-]<value>[s|m|h|d|M|y]"
                     " || scroll_horiz [-window <number>] [+/-]<value>[%]"
                     " || scroll_up|scroll_down|scroll_top|"
                     "scroll_bottom|scroll_previous_highlight|"
                     "scroll_next_highlight|scroll_unread [-window <number>]"
                     " || swap [-window <number>] [up|down|left|right]"
                     " || zoom[-window <number>]"),
                  N_("         list: list opened windows (without argument, "
                     "this list is displayed)\n"
                     "           -1: jump to previous window\n"
                     "           +1: jump to next window\n"
                     "           b#: jump to next window displaying buffer number #\n"
                     "           up: switch to window above current one\n"
                     "         down: switch to window below current one\n"
                     "         left: switch to window on the left\n"
                     "        right: switch to window on the right\n"
                     "       number: window number (see /window list)\n"
                     "       splith: split current window horizontally\n"
                     "       splitv: split current window vertically\n"
                     "       resize: resize window size, new size is <pct> "
                     "percentage of parent window\n"
                     "      balance: balance the sizes of all windows\n"
                     "        merge: merge window with another (all = keep only one "
                     "window)\n"
                     "      page_up: scroll one page up\n"
                     "    page_down: scroll one page down\n"
                     "      refresh: refresh screen\n"
                     "       scroll: scroll a number of lines (+/-N) or with time: "
                     "s=seconds, m=minutes, h=hours, d=days, M=months, y=years\n"
                     " scroll_horiz: scroll horizontally a number of columns "
                     "(+/-N) or percentage of window size (this scrolling is "
                     "possible only on buffers with free content)\n"
                     "    scroll_up: scroll a few lines up\n"
                     "  scroll_down: scroll a few lines down\n"
                     "   scroll_top: scroll to top of buffer\n"
                     "scroll_bottom: scroll to bottom of buffer\n"
                     "scroll_previous_highlight: scroll to previous highlight\n"
                     "scroll_next_highlight: scroll to next highlight\n"
                     "scroll_unread: scroll to unread marker\n"
                     "         swap: swap buffers of two windows (with optional "
                     "direction for target window)\n"
                     "         zoom: zoom on window\n\n"
                     "For splith and splitv, pct is a percentage which "
                     "represents size of new window, computed with current "
                     "window as size reference. For example 25 means create a "
                     "new window with size = current_size / 4\n\n"
                     "Examples:\n"
                     "  jump to window displaying buffer #1:\n"
                     "    /window b1\n"
                     "  scroll 2 lines up:\n"
                     "    /window scroll -2\n"
                     "  scroll 2 days up:\n"
                     "    /window scroll -2d\n"
                     "  scroll to beginning of current day:\n"
                     "    /window scroll -d\n"
                     "  zoom on window #2:\n"
                     "    /window zoom -window 2"),
                  "list"
                  " || -1 -window %(windows_numbers)"
                  " || +1 -window %(windows_numbers)"
                  " || up -window %(windows_numbers)"
                  " || down -window %(windows_numbers)"
                  " || left -window %(windows_numbers)"
                  " || right -window %(windows_numbers)"
                  " || splith -window %(windows_numbers)"
                  " || splitv -window %(windows_numbers)"
                  " || resize -window %(windows_numbers)"
                  " || balance"
                  " || page_up -window %(windows_numbers)"
                  " || page_down -window %(windows_numbers)"
                  " || refresh"
                  " || scroll -window %(windows_numbers)"
                  " || scroll_horiz -window %(windows_numbers)"
                  " || scroll_up -window %(windows_numbers)"
                  " || scroll_down -window %(windows_numbers)"
                  " || scroll_top -window %(windows_numbers)"
                  " || scroll_bottom -window %(windows_numbers)"
                  " || scroll_previous_highlight -window %(windows_numbers)"
                  " || scroll_next_highlight -window %(windows_numbers)"
                  " || scroll_unread  -window %(windows_numbers)"
                  " || swap up|down|left|right|-window %(windows_numbers)"
                  " || zoom -window %(windows_numbers)"
                  " || merge all|-window %(windows_numbers)",
                  &command_window, NULL);
}

/*
 * command_exec_list: execute command list
 */

void
command_exec_list (const char *command_list)
{
    char **commands, **ptr_cmd;
    struct t_gui_buffer *weechat_buffer;

    if (command_list && command_list[0])
    {
        commands = string_split_command (command_list, ';');
        if (commands)
        {
            weechat_buffer = gui_buffer_search_main ();
            for (ptr_cmd = commands; *ptr_cmd; ptr_cmd++)
            {
                input_data (weechat_buffer, *ptr_cmd);
            }
            string_free_split_command (commands);
        }
    }
}

/*
 * command_startup: execute command at startup
 */

void
command_startup (int plugins_loaded)
{
    if (plugins_loaded)
    {
        command_exec_list(CONFIG_STRING(config_startup_command_after_plugins));
        command_exec_list(weechat_startup_commands);
    }
    else
        command_exec_list(CONFIG_STRING(config_startup_command_before_plugins));
}
