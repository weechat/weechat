/*
 * demo.c - demo plugin for WeeChat
 *
 * Copyright (C) 2003-2013 Sebastien Helleu <flashcode@flashtux.org>
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

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#include "../weechat-plugin.h"


#define DEMO_PLUGIN_NAME "demo"

WEECHAT_PLUGIN_NAME(DEMO_PLUGIN_NAME);
WEECHAT_PLUGIN_DESCRIPTION(N_("Demo plugin"));
WEECHAT_PLUGIN_AUTHOR("Sebastien Helleu <flashcode@flashtux.org>");
WEECHAT_PLUGIN_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_LICENSE(WEECHAT_LICENSE);

struct t_weechat_plugin *weechat_demo_plugin = NULL;
#define weechat_plugin weechat_demo_plugin


/*
 * Callback for command "/demo_printf".
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
                        "demo_printf: \"%s\"", argv_eol[1]);
    else
    {
        weechat_printf (buffer,
                        _("demo message without prefix"));
        weechat_printf (buffer,
                        _("%sdemo message with error prefix"),
                        weechat_prefix ("error"));
        weechat_printf (buffer,
                        "%s: %s chat_buffer %s chat_server "
                        "%s chat_host",
                        _("colors"),
                        weechat_color ("chat_buffer"),
                        weechat_color ("chat_server"),
                        weechat_color ("chat_host"));
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for input data on buffer.
 */

int
demo_buffer_input_data_cb (void *data, struct t_gui_buffer *buffer,
                           const char *input_data)
{
    /* make C compiler happy */
    (void) data;

    weechat_printf (buffer,
                    "buffer_input_data_cb: buffer = %lx (%s), "
                    "input_data = \"%s\"",
                    (long unsigned int)buffer,
                    weechat_buffer_get_string (buffer, "name"),
                    input_data);

    return WEECHAT_RC_OK;
}

/*
 * Callback called when buffer is closed.
 */

int
demo_buffer_close_cb (void *data, struct t_gui_buffer *buffer)
{
    /* make C compiler happy */
    (void) data;

    if (weechat_demo_plugin->debug)
    {
        weechat_printf (NULL,
                        "buffer_close_cb: buffer = %lx (%s)",
                        (long unsigned int)buffer,
                        weechat_buffer_get_string (buffer, "name"));
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "/demo_buffer".
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

    if (argc > 1)
    {
        new_buffer = weechat_buffer_new (argv[1],
                                         &demo_buffer_input_data_cb, NULL,
                                         &demo_buffer_close_cb, NULL);
        if (new_buffer)
        {
            weechat_buffer_set (new_buffer, "display", "1");
            weechat_hook_signal_send ("logger_backlog",
                                      WEECHAT_HOOK_SIGNAL_POINTER, new_buffer);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Displays an infolist.
 */

void
demo_infolist_print (struct t_infolist *infolist, const char *item_name)
{
    const char *fields;
    char **argv;
    void *pointer;
    int i, j, argc, size;
    time_t time;

    if (!infolist)
        return;

    i = 1;
    while (weechat_infolist_next (infolist))
    {
        weechat_printf (NULL, "--- %s #%d ---", item_name, i);
        fields = weechat_infolist_fields (infolist);
        if (fields)
        {
            argv = weechat_string_split (fields, ",", 0, 0, &argc);
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
                            weechat_printf (NULL, "  %s: %lx",
                                            argv[j] + 2,
                                            (long unsigned int)weechat_infolist_pointer (infolist,
                                                                                         argv[j] + 2));
                            break;
                        case 'b':
                            pointer = weechat_infolist_buffer (infolist,
                                                               argv[j] + 2,
                                                               &size);
                            weechat_printf (NULL, "  %s: %lx (size: %d)",
                                            argv[j] + 2,
                                            (long unsigned int)pointer,
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
                weechat_string_free_split (argv);
        }
        i++;
    }
}

/*
 * Callback for command "/demo_info".
 */

int
demo_info_command_cb (void *data, struct t_gui_buffer *buffer, int argc,
                      char **argv, char **argv_eol)
{
    struct t_infolist *infolist;

    /* make C compiler happy */
    (void) data;
    (void) buffer;

    if (argc > 1)
        weechat_printf (NULL, "info \"%s\" = \"%s\"",
                        argv[1],
                        weechat_info_get (argv[1],
                                          (argc > 2) ? argv_eol[2] : NULL));
    else
    {
        infolist = weechat_infolist_get ("hook", NULL, "info");
        if (infolist)
        {
            weechat_printf (NULL, "");
            weechat_printf (NULL, _("Available infos:"));
            while (weechat_infolist_next (infolist))
            {
                weechat_printf (NULL,
                                "  %s: %s",
                                weechat_infolist_string (infolist, "info_name"),
                                weechat_gettext (weechat_infolist_string (infolist,
                                                                          "description")));
            }
            weechat_infolist_free (infolist);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "/demo_infolist".
 */

int
demo_infolist_command_cb (void *data, struct t_gui_buffer *buffer, int argc,
                          char **argv, char **argv_eol)
{
    struct t_infolist *infolist;

    /* make C compiler happy */
    (void) data;
    (void) buffer;

    if (argc > 1)
    {
        infolist = weechat_infolist_get (argv[1], NULL,
                                         (argc > 2) ? argv_eol[2] : NULL);
        if (infolist)
        {
            demo_infolist_print (infolist, argv[1]);
            weechat_infolist_free (infolist);
        }
    }
    else
    {
        infolist = weechat_infolist_get ("hook", NULL, "infolist");
        if (infolist)
        {
            weechat_printf (NULL, "");
            weechat_printf (NULL, _("Available infolists:"));
            while (weechat_infolist_next (infolist))
            {
                weechat_printf (NULL,
                                "  %s: %s",
                                weechat_infolist_string (infolist, "infolist_name"),
                                weechat_gettext (weechat_infolist_string (infolist, "description")));
            }
            weechat_infolist_free (infolist);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for a signal hooked.
 */

int
demo_signal_cb (void *data, const char *signal, const char *type_data,
                void *signal_data)
{
    /* make C compiler happy */
    (void) data;

    if (weechat_demo_plugin->debug)
    {
        if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_STRING) == 0)
        {
            weechat_printf (NULL,
                            _("demo_signal: signal: %s, type_data: %s, "
                              "signal_data: \"%s\""),
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
                              "signal_data: 0x%lx"),
                            signal, type_data, (long unsigned int)signal_data);
        }
        else
        {
            weechat_printf (NULL,
                            _("demo_signal: signal: %s, type_data: %s, "
                              "signal_data: 0x%lx (unknown type)"),
                            signal, type_data, (long unsigned int)signal_data);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Initializes demo plugin.
 */

int
weechat_plugin_init (struct t_weechat_plugin *plugin, int argc, char *argv[])
{
    /* make C compiler happy */
    (void) argc;
    (void) argv;

    weechat_plugin = plugin;

    weechat_hook_command ("demo_printf",
                          N_("print some messages on current buffer"),
                          N_("<text>"),
                          N_("text: write this text"),
                          "",
                          &demo_printf_command_cb, NULL);

    weechat_hook_command ("demo_buffer",
                          N_("open a new buffer"),
                          N_("<name>"),
                          "",
                          "",
                          &demo_buffer_command_cb, NULL);

    weechat_hook_command ("demo_info",
                          N_("get and display an info"),
                          N_("<info> [<arguments>]"),
                          N_("     info: info to display\n"
                             "arguments: optional arguments for info\n\n"
                             "Without argument, this command displays list "
                             "of available infos"),
                          "%(infos)",
                          &demo_info_command_cb, NULL);

    weechat_hook_command ("demo_infolist",
                          N_("get and display an infolist"),
                          N_("<infolist> [<arguments>]"),
                          N_(" infolist: infolist to display\n"
                             "arguments: optional arguments for infolist\n\n"
                             "Without argument, this command displays list "
                             "of available infolists"),
                          "%(infolists)",
                          &demo_infolist_command_cb, NULL);

    weechat_hook_signal ("*", &demo_signal_cb, NULL);

    return WEECHAT_RC_OK;
}

/*
 * Ends demo plugin.
 */

int
weechat_plugin_end (struct t_weechat_plugin *plugin)
{
    /* make C compiler happy */
    (void) plugin;

    return WEECHAT_RC_OK;
}
