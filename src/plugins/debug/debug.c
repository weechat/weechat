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

/* debug.c: Debug plugin for WeeChat */


#include <stdlib.h>
#include <string.h>

#include "../weechat-plugin.h"


WEECHAT_PLUGIN_NAME("debug");
WEECHAT_PLUGIN_DESCRIPTION("Debug plugin for WeeChat");
WEECHAT_PLUGIN_AUTHOR("FlashCode <flashcode@flashtux.org>");
WEECHAT_PLUGIN_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_WEECHAT_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_LICENSE("GPL");

struct t_weechat_plugin *weechat_debug_plugin = NULL;
#define weechat_plugin weechat_debug_plugin


/*
 * debug_command_cb: callback for /debug command
 */

int
debug_command_cb (void *data, struct t_gui_buffer *buffer, int argc,
                  char **argv, char **argv_eol)
{
    /* make C compiler happy */
    (void) data;
    (void) buffer;
    
    if (argc >= 2)
    {
        if (weechat_strcasecmp (argv[1], "dump") == 0)
        {
            weechat_hook_signal_send ("debug_dump",
                                      WEECHAT_HOOK_SIGNAL_STRING, NULL);
            //debug_dump (0);
        }
        else if (weechat_strcasecmp (argv[1], "buffer") == 0)
        {
            weechat_hook_signal_send ("debug_buffer",
                                      WEECHAT_HOOK_SIGNAL_POINTER, buffer);
            /*gui_buffer_dump_hexa (buffer);
            gui_chat_printf (NULL,
                             "DEBUG: buffer content written in WeeChat "
                             "log file");
            */
        }
        else if (weechat_strcasecmp (argv[1], "windows") == 0)
        {
            weechat_hook_signal_send ("debug_windows",
                                      WEECHAT_HOOK_SIGNAL_STRING, NULL);
        }
        else
        {
            weechat_hook_signal_send ("debug",
                                      WEECHAT_HOOK_SIGNAL_STRING, argv_eol[1]);
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * weechat_plugin_init: initialize debug plugin
 */

int
weechat_plugin_init (struct t_weechat_plugin *plugin)
{
    weechat_plugin = plugin;

    weechat_hook_command ("debug",
                          N_("print debug messages"),
                          N_("dump | buffer | windows | text"),
                          N_("   dump: save memory dump in WeeChat log file (same "
                             "dump is written when WeeChat crashes)\n"
                             " buffer: dump buffer content with hexadecimal values "
                             "in log file\n"
                             "windows: display windows tree\n"
                             "   text: send \"debug\" signal with \"text\" as "
                             "argument"),
                          "dump|buffer|windows",
                          &debug_command_cb, NULL);
    
    return WEECHAT_RC_OK;
}

/*
 * weechat_plugin_end: end debug plugin
 */

int
weechat_plugin_end (struct t_weechat_plugin *plugin)
{
    /* make C compiler happy */
    (void) plugin;
    
    return WEECHAT_RC_OK;
}
