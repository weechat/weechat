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
                            weechat_printf (NULL, "  %s: %ld",
                                            argv[j] + 2,
                                            weechat_list_time (list,
                                                               argv[j] + 2));
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
 * demo_buffer_infos: display buffer infos
 */

static void
demo_buffer_infos ()
{
    struct t_plugin_list *list;
    
    list = weechat_list_get ("buffer", NULL);
    if (list)
    {
        demo_print_list (list, "buffer");
        weechat_list_free (list);
    }
}

/*
 * demo_command: demo command 
 */

static int
demo_command (void *data, int argc, char **argv, char **argv_eol)
{
    /* make C compiler happy */
    (void) data;
    (void) argv_eol;
    
    if (argc > 1)
    {
        if (weechat_strcasecmp (argv[1], "buffer") == 0)
        {
            demo_buffer_infos ();
            return PLUGIN_RC_SUCCESS;
        }
    }
    
    weechat_printf (NULL,
                    "Demo: missing argument for /demo command "
                    "(try /help demo)");
    
    return PLUGIN_RC_SUCCESS;
}

/*
 * weechat_plugin_init: init demo plugin
 */

int
weechat_plugin_init (struct t_weechat_plugin *plugin)
{
    weechat_plugin = plugin;

    weechat_hook_command ("demo", "demo command", "[action]",
                          "action: one of following actions:\n"
                          "  buffer  display infos about buffers",
                          "buffer", demo_command, NULL);
    
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
