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
#include "demo.h"


static struct t_weechat_plugin *weechat_plugin = NULL;


/*
 * demo_print_list: display a list
 */

static void
demo_print_list (void *list, char *item_name)
{
    char *fields, **argv;
    int i, j, argc;
    time_t date;
    
    i = 1;
    while (weechat_list_next (list))
    {
        weechat_printf (NULL, "--- %s #%d ---", item_name, i);
        fields = weechat_list_fields (list);
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
                                            weechat_list_int (list,
                                                              argv[j] + 2));
                            break;
                        case 's':
                            weechat_printf (NULL, "  %s: %s",
                                            argv[j] + 2,
                                            weechat_list_string (list,
                                                                 argv[j] + 2));
                            break;
                        case 'p':
                            weechat_printf (NULL, "  %s: %X",
                                            argv[j] + 2,
                                            weechat_list_pointer (list,
                                                                  argv[j] + 2));
                            break;
                        case 't':
                            date = weechat_list_time (list, argv[j] + 2);
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
 * demo_list_command: demo command for list
 */

static int
demo_list_command (void *data, int argc, char **argv, char **argv_eol)
{
    struct t_plugin_list *list;
    
    /* make C compiler happy */
    (void) data;
    (void) argv_eol;
    
    if (argc > 1)
    {
        if (weechat_strcasecmp (argv[1], "buffer") == 0)
        {
            list = weechat_list_get ("buffer", NULL);
            if (list)
            {
                demo_print_list (list, "buffer");
                weechat_list_free (list);
            }
            return PLUGIN_RC_SUCCESS;
        }
        if (weechat_strcasecmp (argv[1], "buffer_lines") == 0)
        {
            list = weechat_list_get ("buffer_lines", NULL);
            if (list)
            {
                demo_print_list (list, "buffer_line");
                weechat_list_free (list);
            }
            return PLUGIN_RC_SUCCESS;
        }
    }
    
    weechat_printf (NULL,
                    "Demo: missing argument for /demo_list command "
                    "(try /help demo_list)");
    
    return PLUGIN_RC_SUCCESS;
}

/*
 * demo_printf_command: demo command for printf
 */

static int
demo_printf_command (void *data, int argc, char **argv, char **argv_eol)
{
    (void) data;
    (void) argc;
    (void) argv;
    (void) argv_eol;
    
    weechat_printf (NULL, "demo message without prefix");
    weechat_printf (NULL, "%sdemo message with info prefix",
                    weechat_prefix ("info"));
    weechat_printf (NULL, "%sdemo message with error prefix",
                    weechat_prefix ("error"));
    weechat_printf (NULL,
                    "colors: %s buffer %s server %s nick1 %s nick2 %s nick3",
                    weechat_color ("col_chat_buffer"),
                    weechat_color ("col_chat_server"),
                    weechat_color ("col_chat_nick_color1"),
                    weechat_color ("col_chat_nick_color2"),
                    weechat_color ("col_chat_nick_color3"));

    return PLUGIN_RC_SUCCESS;
}

/*
 * demo_info_command: demo command for info_get
 */

static int
demo_info_command (void *data, int argc, char **argv, char **argv_eol)
{
    /* make C compiler happy */
    (void) data;
    (void) argv_eol;
    
    if (argc > 1)
        weechat_printf (NULL, "%sinfo \"%s\" = \"%s\"",
                        weechat_prefix ("info"),
                        argv[1],
                        weechat_info_get (argv[1]));
    else
        weechat_printf (NULL,
                        "Demo: missing argument for /demo_info command "
                        "(try /help demo_info)");

    return PLUGIN_RC_SUCCESS;
}

/*
 * weechat_plugin_init: init demo plugin
 */

int
weechat_plugin_init (struct t_weechat_plugin *plugin)
{
    weechat_plugin = plugin;

    weechat_hook_command ("demo_list", "demo command: get and display list",
                          "list",
                          "list: list to display (values: buffer, "
                          "buffer_lines)",
                          "buffer|buffer_lines",
                          demo_list_command, NULL);

    weechat_hook_command ("demo_printf", "demo command: print some messages",
                          "", "", "",
                          demo_printf_command, NULL);

    weechat_hook_command ("demo_info", "demo command: get and display info",
                          "info",
                          "info: info to display (values: version, "
                          "weechat_dir, weechat_libdir, weechat_sharedir, "
                          "charset_terminal, charset_internal, inactivity, "
                          "input, input_mask, input_pos)",
                          "version|weechat_dir|weechat_libdir|"
                          "weechat_sharedir|charset_terminal|charset_internal|"
                          "inactivity|input|input_mask|input_pos",
                          demo_info_command, NULL);
    
    return PLUGIN_RC_SUCCESS;
}

/*
 * weechat_plugin_end: end demo plugin
 */

int
weechat_plugin_end ()
{
    return PLUGIN_RC_SUCCESS;
}
