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

/* demo.c: demo plugin for WeeChat */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#include "../weechat-plugin.h"


char plugin_name[] = "demo";
char plugin_version[]     = "0.1";
char plugin_description[] = "Demo plugin for WeeChat";

struct t_weechat_plugin *weechat_demo_plugin = NULL;
#define weechat_plugin weechat_demo_plugin


/*
 * demo_printf_command_cb: demo command for printf
 */

int
demo_printf_command_cb (void *data, struct t_gui_buffer *buffer, int argc,
                        char **argv, char **argv_eol)
{
    /* make C compiler happy */
    (void) data;
    (void) argv;
    
    if (argc > 1)
        weechat_printf (buffer,
                        "demo_printf: '%s'", argv_eol[1]);
    else
    {
        weechat_printf (buffer,
                        _("demo message without prefix"));
        weechat_printf (buffer,
                        _("%sdemo message with info prefix"),
                        weechat_prefix ("info"));
        weechat_printf (buffer,
                        _("%sdemo message with error prefix"),
                        weechat_prefix ("error"));
        weechat_printf (buffer,
                        _("colors: %s buffer %s nick1 %s nick2 %s nick3 "
                          "%s nick4"),
                        weechat_color ("color_chat_buffer"),
                        weechat_color ("color_chat_nick_color1"),
                        weechat_color ("color_chat_nick_color2"),
                        weechat_color ("color_chat_nick_color3"),
                        weechat_color ("color_chat_nick_color4"));
    }
    
    return WEECHAT_RC_OK;
}

/*
 * demo_infobar_command_cb: demo command for infobar
 */

int
demo_infobar_command_cb (void *data, struct t_gui_buffer *buffer, int argc,
                         char **argv, char **argv_eol)
{
    /* make C compiler happy */
    (void) data;
    (void) buffer;
    (void) argv;
    
    weechat_infobar_printf (10, NULL,
                            (argc > 1) ?
                            argv_eol[1] : _("test message in infobar"));
    
    return WEECHAT_RC_OK;
}

/*
 * demo_buffer_input_data_cb: callback for input data on buffer
 */

void
demo_buffer_input_data_cb (struct t_gui_buffer *buffer, char *data)
{
    weechat_printf (buffer, "buffer input_data_cb: data = '%s'", data);
}

/*
 * demo_buffer_command_cb: demo command for creatig new buffer
 */

int
demo_buffer_command_cb (void *data, struct t_gui_buffer *buffer, int argc,
                        char **argv, char **argv_eol)
{
    struct t_gui_buffer *new_buffer;

    /* make C compiler happy */
    (void) data;
    (void) buffer;
    (void) argv_eol;
    
    if (argc > 2)
    {
        new_buffer = weechat_buffer_new (argv[1], argv[2],
                                         demo_buffer_input_data_cb);
        if (new_buffer)
            weechat_buffer_set (new_buffer, "display", "1");
        weechat_hook_signal_send ("logger_backlog", new_buffer);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * demo_infolist_print: display an infolist
 */

void
demo_infolist_print (struct t_plugin_infolist *infolist, char *item_name)
{
    char *fields, **argv;
    int i, j, argc;
    time_t date;
    
    i = 1;
    while (weechat_infolist_next (infolist))
    {
        weechat_printf (NULL, "--- %s #%d ---", item_name, i);
        fields = weechat_infolist_fields (infolist);
        if (fields)
        {
            argv = weechat_string_explode (fields, ",", 0, 0, &argc);
            if (argv && (argc > 0))
            {
                for (j = 0; j < argc; j++)
                {
                    switch (argv[j][0])
                    {
                        case 'i':
                            weechat_printf (NULL, "  %s: %d",
                                            argv[j] + 2,
                                            weechat_infolist_integer (infolist,
                                                                      argv[j] + 2));
                            break;
                        case 's':
                            weechat_printf (NULL, "  %s: %s",
                                            argv[j] + 2,
                                            weechat_infolist_string (infolist,
                                                                     argv[j] + 2));
                            break;
                        case 'p':
                            weechat_printf (NULL, "  %s: %X",
                                            argv[j] + 2,
                                            weechat_infolist_pointer (infolist,
                                                                      argv[j] + 2));
                            break;
                        case 't':
                            date = weechat_infolist_time (infolist, argv[j] + 2);
                            weechat_printf (NULL, "  %s: (%ld) %s",
                                            argv[j] + 2,
                                            date, ctime (&date));
                            break;
                    }
                }

            }
            if (argv)
                weechat_string_free_exploded (argv);
        }
        i++;
    }
}

/*
 * demo_infolist_command_cb: demo command for list
 */

int
demo_infolist_command_cb (void *data, struct t_gui_buffer *buffer, int argc,
                          char **argv, char **argv_eol)
{
    struct t_plugin_infolist *infolist;
    
    /* make C compiler happy */
    (void) data;
    (void) buffer;
    (void) argv_eol;
    
    if (argc > 1)
    {
        if (weechat_strcasecmp (argv[1], "buffer") == 0)
        {
            infolist = weechat_infolist_get ("buffer", NULL);
            if (infolist)
            {
                demo_infolist_print (infolist, "buffer");
                weechat_infolist_free (infolist);
            }
            return WEECHAT_RC_OK;
        }
        if (weechat_strcasecmp (argv[1], "buffer_lines") == 0)
        {
            infolist = weechat_infolist_get ("buffer_lines", NULL);
            if (infolist)
            {
                demo_infolist_print (infolist, "buffer_line");
                weechat_infolist_free (infolist);
            }
            return WEECHAT_RC_OK;
        }
    }
    
    weechat_printf (NULL,
                    _("%s%s: missing argument for \"%s\" command "
                      "(try /help %s)"),
                    weechat_prefix ("error"), "Demo",
                    "demo_infolist", "demo_infolist");
    
    return WEECHAT_RC_OK;
}

/*
 * demo_info_command_cb: demo command for info_get
 */

int
demo_info_command_cb (void *data, struct t_gui_buffer *buffer, int argc,
                      char **argv, char **argv_eol)
{
    /* make C compiler happy */
    (void) data;
    (void) buffer;
    (void) argv_eol;
    
    if (argc > 1)
        weechat_printf (NULL, "%sinfo \"%s\" = \"%s\"",
                        weechat_prefix ("info"),
                        argv[1],
                        weechat_info_get (argv[1]));
    else
        weechat_printf (NULL,
                        _("%s%s: missing argument for \"%s\" command "
                          "(try /help %s)"),
                        weechat_prefix ("error"), "Demo",
                        "demo_info", "demo_info");
    
    return WEECHAT_RC_OK;
}

/*
 * demo_signal_cb: callback for signal hook
 */

int
demo_signal_cb (void *data, char *signal, void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    
    weechat_printf (NULL,
                    _("demo_signal: signal: %s, signal_data: %X"),
                    signal, signal_data);
    
    return WEECHAT_RC_OK;
}

/*
 * weechat_plugin_init: initialize demo plugin
 */

int
weechat_plugin_init (struct t_weechat_plugin *plugin)
{
    weechat_plugin = plugin;
    
    weechat_hook_command ("demo_printf",
                          _("print some messages on current ubffer"),
                          _("[text]"),
                          _("text: write this text"),
                          "",
                          &demo_printf_command_cb, NULL);

    weechat_hook_command ("demo_infobar",
                          _("print a message in infobar for 10 seconds"),
                          _("[text]"),
                          _("text: write this text"),
                          "",
                          &demo_infobar_command_cb, NULL);

    weechat_hook_command ("demo_buffer",
                          _("open a new buffer"),
                          _("category name"),
                          "",
                          "",
                          &demo_buffer_command_cb, NULL);
    
    weechat_hook_command ("demo_infolist",
                          _("get and display an infolist"),
                          _("infolist"),
                          _("infolist: infolist to display (values: buffer, "
                            "buffer_lines)"),
                          "buffer|buffer_lines",
                          &demo_infolist_command_cb, NULL);
    
    weechat_hook_command ("demo_info",
                          _("get and display an info"),
                          _("info"),
                          _("info: info to display (values: version, "
                            "weechat_dir, weechat_libdir, weechat_sharedir, "
                            "charset_terminal, charset_internal, inactivity, "
                            "input, input_mask, input_pos)"),
                          "version|weechat_dir|weechat_libdir|"
                          "weechat_sharedir|charset_terminal|charset_internal|"
                          "inactivity|input|input_mask|input_pos",
                          &demo_info_command_cb, NULL);

    weechat_hook_signal ("*", &demo_signal_cb, NULL);
    
    return WEECHAT_RC_OK;
}

/*
 * weechat_plugin_end: end demo plugin
 */

int
weechat_plugin_end ()
{
    return WEECHAT_RC_OK;
}
