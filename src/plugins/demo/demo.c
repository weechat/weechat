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

/* demo.c: demo plugin for WeeChat */


#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#include "../weechat-plugin.h"


WEECHAT_PLUGIN_NAME("demo");
WEECHAT_PLUGIN_DESCRIPTION("Demo plugin for WeeChat");
WEECHAT_PLUGIN_AUTHOR("FlashCode <flashcode@flashtux.org>");
WEECHAT_PLUGIN_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_WEECHAT_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_LICENSE("GPL3");

struct t_weechat_plugin *weechat_demo_plugin = NULL;
#define weechat_plugin weechat_demo_plugin

int demo_debug = 0;


/*
 * demo_debug_signal_debug_cb: callback for "debug" signal
 */

int
demo_debug_signal_debug_cb (void *data, const char *signal,
                            const char *type_data, void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;
    
    if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_STRING) == 0)
    {
        if (weechat_strcasecmp ((char *)signal_data, "demo") == 0)
        {
            demo_debug ^= 1;
            if (demo_debug)
                weechat_printf (NULL, _("%s: debug enabled"), "demo");
            else
                weechat_printf (NULL, _("%s: debug disabled"), "demo");
        }
    }
    
    return WEECHAT_RC_OK;
}

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
                        _("%sdemo message with error prefix"),
                        weechat_prefix ("error"));
        weechat_printf (buffer,
                        _("colors: %s buffer %s nick1 %s nick2 %s nick3 "
                          "%s nick4"),
                        weechat_color ("chat_buffer"),
                        weechat_color ("chat_nick_color1"),
                        weechat_color ("chat_nick_color2"),
                        weechat_color ("chat_nick_color3"),
                        weechat_color ("chat_nick_color4"));
    }
    
    return WEECHAT_RC_OK;
}

/*
 * demo_buffer_input_data_cb: callback for input data on buffer
 */

int
demo_buffer_input_data_cb (void *data, struct t_gui_buffer *buffer,
                           const char *input_data)
{
    /* make C compiler happy */
    (void) data;
    
    weechat_printf (buffer,
                    "buffer_input_data_cb: buffer = %x (%s / %s), "
                    "input_data = '%s'",
                    buffer,
                    weechat_buffer_get_string (buffer, "category"),
                    weechat_buffer_get_string (buffer, "name"),
                    input_data);
    
    return WEECHAT_RC_OK;
}

/*
 * demo_buffer_close_cb: callback for buffer closed
 */

int
demo_buffer_close_cb (void *data, struct t_gui_buffer *buffer)
{
    /* make C compiler happy */
    (void) data;

    if (demo_debug)
    {
        weechat_printf (NULL,
                        "buffer_close_cb: buffer = %x (%s / %s)",
                        buffer,
                        weechat_buffer_get_string (buffer, "category"),
                        weechat_buffer_get_string (buffer, "name"));
    }
    
    return WEECHAT_RC_OK;
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
                                         &demo_buffer_input_data_cb, NULL,
                                         &demo_buffer_close_cb, NULL);
        if (new_buffer)
            weechat_buffer_set (new_buffer, "display", "1");
        weechat_hook_signal_send ("logger_backlog",
                                  WEECHAT_HOOK_SIGNAL_POINTER, new_buffer);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * demo_buffer_set_command_cb: demo command for setting buffer property
 */

int
demo_buffer_set_command_cb (void *data, struct t_gui_buffer *buffer, int argc,
                            char **argv, char **argv_eol)
{
    /* make C compiler happy */
    (void) data;
    
    if (argc > 2)
    {
        weechat_buffer_set (buffer, argv[1], argv_eol[2]);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * demo_infolist_print: display an infolist
 */

void
demo_infolist_print (struct t_infolist *infolist, const char *item_name)
{
    char *fields, **argv;
    void *pointer;
    int i, j, argc, size;
    time_t time;
    
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
                        case 'b':
                            pointer = weechat_infolist_buffer (infolist,
                                                               argv[j] + 2,
                                                               &size);
                            weechat_printf (NULL, "  %s: %X (size: %d)",
                                            argv[j] + 2,
                                            pointer,
                                            size);
                            break;
                        case 't':
                            time = weechat_infolist_time (infolist, argv[j] + 2);
                            weechat_printf (NULL, "  %s: (%ld) %s",
                                            argv[j] + 2,
                                            time, ctime (&time));
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
    struct t_infolist *infolist;
    
    /* make C compiler happy */
    (void) data;
    (void) buffer;
    (void) argv_eol;
    
    if (argc > 1)
    {
        if (weechat_strcasecmp (argv[1], "buffer") == 0)
        {
            infolist = weechat_infolist_get ("buffer", NULL, NULL);
            if (infolist)
            {
                demo_infolist_print (infolist, "buffer");
                weechat_infolist_free (infolist);
            }
            return WEECHAT_RC_OK;
        }
        if (weechat_strcasecmp (argv[1], "buffer_lines") == 0)
        {
            infolist = weechat_infolist_get ("buffer_lines", NULL, NULL);
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
                    weechat_prefix ("error"), "demo",
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
        weechat_printf (NULL, "info \"%s\" = \"%s\"",
                        argv[1],
                        weechat_info_get (argv[1]));
    else
        weechat_printf (NULL,
                        _("%s%s: missing argument for \"%s\" command "
                          "(try /help %s)"),
                        weechat_prefix ("error"), "demo",
                        "demo_info", "demo_info");
    
    return WEECHAT_RC_OK;
}

/*
 * demo_signal_cb: callback for signal hook
 */

int
demo_signal_cb (void *data, const char *signal, const char *type_data,
                void *signal_data)
{
    /* make C compiler happy */
    (void) data;

    if (demo_debug)
    {
        if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_STRING) == 0)
        {
            weechat_printf (NULL,
                            _("demo_signal: signal: %s, type_data: %s, "
                              "signal_data: '%s'"),
                            signal, type_data, (char *)signal_data);
        }
        else if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_INT) == 0)
        {
            weechat_printf (NULL,
                            _("demo_signal: signal: %s, type_data: %s, "
                              "signal_data: %d"),
                            signal, type_data, *((int *)signal_data));
        }
        else if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_POINTER) == 0)
        {
            weechat_printf (NULL,
                            _("demo_signal: signal: %s, type_data: %s, "
                              "signal_data: 0x%x"),
                            signal, type_data, signal_data);
        }
        else
        {
            weechat_printf (NULL,
                            _("demo_signal: signal: %s, type_data: %s, "
                              "signal_data: 0x%x (unknown type)"),
                            signal, type_data, signal_data);
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * weechat_plugin_init: initialize demo plugin
 */

int
weechat_plugin_init (struct t_weechat_plugin *plugin, int argc, char *argv[])
{
    /* make C compiler happy */
    (void) argc;
    (void) argv;
    
    weechat_plugin = plugin;
    
    demo_debug = weechat_config_boolean (weechat_config_get ("weechat.plugin.debug"));
    
    weechat_hook_command ("demo_printf",
                          _("print some messages on current ubffer"),
                          _("[text]"),
                          _("text: write this text"),
                          "",
                          &demo_printf_command_cb, NULL);
    
    weechat_hook_command ("demo_buffer",
                          _("open a new buffer"),
                          _("category name"),
                          "",
                          "",
                          &demo_buffer_command_cb, NULL);

    weechat_hook_command ("demo_buffer_set",
                          _("set a buffer property"),
                          _("property value"),
                          "",
                          "",
                          &demo_buffer_set_command_cb, NULL);
    
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
    
    weechat_hook_signal ("debug", &demo_debug_signal_debug_cb, NULL);
    weechat_hook_signal ("*", &demo_signal_cb, NULL);
    
    return WEECHAT_RC_OK;
}

/*
 * weechat_plugin_end: end demo plugin
 */

int
weechat_plugin_end (struct t_weechat_plugin *plugin)
{
    /* make C compiler happy */
    (void) plugin;
    
    return WEECHAT_RC_OK;
}
