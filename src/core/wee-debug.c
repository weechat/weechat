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

/* wee-debug.c: some debug functions for WeeChat */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>

#include "weechat.h"
#include "wee-backtrace.h"
#include "wee-config-file.h"
#include "wee-log.h"
#include "wee-hook.h"
#include "wee-string.h"
#include "../gui/gui-bar.h"
#include "../gui/gui-bar-item.h"
#include "../gui/gui-buffer.h"
#include "../gui/gui-chat.h"
#include "../gui/gui-filter.h"
#include "../gui/gui-hotlist.h"
#include "../gui/gui-keyboard.h"
#include "../gui/gui-main.h"
#include "../gui/gui-window.h"
#include "../plugins/plugin.h"


int debug_dump_active = 0;


/*
 * debug_dump: write dump to WeeChat log file
 */

void
debug_dump (int crash)
{
    /* prevent reentrance */
    if (debug_dump_active)
        exit (EXIT_FAILURE);
    
    if (crash)
    {
        debug_dump_active = 1;
        log_printf ("Very bad, WeeChat is crashing (SIGSEGV received)...");
    }
    
    log_printf ("");
    if (crash)
    {
        log_printf ("******             WeeChat CRASH DUMP              ******");
        log_printf ("****** Please send this file to WeeChat developers ******");
        log_printf ("******    and explain when this crash happened     ******");
    }
    else
    {
        log_printf ("******            WeeChat dump request             ******");
    }

    gui_window_print_log ();
    gui_buffer_print_log ();
    gui_keyboard_print_log (NULL);
    gui_filter_print_log ();
    gui_bar_print_log ();
    gui_bar_item_print_log ();
    gui_hotlist_print_log ();
    
    hook_print_log ();
    
    config_file_print_log ();
    
    plugin_print_log ();
    
    log_printf ("");
    log_printf ("******             End of WeeChat dump             ******");
    log_printf ("");
}

/*
 * debug_dump_cb: callback for "debug_dump" signal hooked
 */

int
debug_dump_cb (void *data, char *signal, char *type_data, void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;
    (void) signal_data;
    
    debug_dump (0);
    
    return WEECHAT_RC_OK;
}

/*
 * debug_sigsegv: SIGSEGV handler: save crash log to
 *                <weechat_home>/weechat.log and exit
 */

void
debug_sigsegv ()
{
    debug_dump (1);
    unhook_all ();
    gui_main_end ();
    
    string_iconv_fprintf (stderr, "\n");
    string_iconv_fprintf (stderr, "*** Very bad! WeeChat is crashing (SIGSEGV received)\n");
    if (!log_crash_rename ())
        string_iconv_fprintf (stderr,
                              "*** Full crash dump was saved to %s/weechat.log file.\n",
                              weechat_home);
    string_iconv_fprintf (stderr, "***\n");
    string_iconv_fprintf (stderr, "*** Please help WeeChat developers to fix this bug:\n");
    string_iconv_fprintf (stderr, "***   1. If you have a core file, please run:  gdb weechat-curses core\n");
    string_iconv_fprintf (stderr, "***      then issue \"bt\" command and send result to developers\n");
    string_iconv_fprintf (stderr, "***      To enable core files with bash shell: ulimit -c 10000\n");
    string_iconv_fprintf (stderr, "***   2. Otherwise send backtrace (below) and weechat.log\n");
    string_iconv_fprintf (stderr, "***      (be careful, private info may be in this file since\n");
    string_iconv_fprintf (stderr, "***      part of chats are displayed, so remove lines if needed)\n\n");
    
    weechat_backtrace ();
    
    /* shutdown with error code */
    weechat_shutdown (EXIT_FAILURE, 1);
}

/*
 * debug_buffer_cb: callback for "debug_buffer" signal hooked
 */

int
debug_buffer_cb (void *data, char *signal, char *type_data, void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;
    
    gui_buffer_dump_hexa ((struct t_gui_buffer *)signal_data);
    
    return WEECHAT_RC_OK;
}

/*
 * debug_windows_tree_display: display tree of windows
 */

void
debug_windows_tree_display (struct t_gui_window_tree *tree, int indent)
{
    char format[128];
    
    if (tree)
    {
        if (tree->window)
        {
            /* leaf */
            snprintf (format,
                      sizeof (format),
                      "%%-%dsleaf: 0x%%X (parent:0x%%X), win=0x%%X, "
                      "child1=0x%%X, child2=0x%%X, %%d,%%d %%dx%%d, "
                      "%%d%%%%x%%d%%%%",
                      indent * 2);
            gui_chat_printf (NULL,
                             format,
                             " ", tree, tree->parent_node, tree->window,
                             tree->child1, tree->child2,
                             tree->window->win_x, tree->window->win_y,
                             tree->window->win_width, tree->window->win_height,
                             tree->window->win_width_pct,
                             tree->window->win_height_pct);
        }
        else
        {
            /* node */
            snprintf (format,
                      sizeof (format),
                      "%%-%dsnode: 0x%%X (parent:0x%%X), win=0x%%X, "
                      "child1=0x%%X, child2=0x%%X)",
                      indent * 2);
            gui_chat_printf (NULL,
                             format,
                             " ", tree, tree->parent_node, tree->window,
                             tree->child1, tree->child2);
        }
        
        if (tree->child1)
            debug_windows_tree_display (tree->child1, indent + 1);
        if (tree->child2)
            debug_windows_tree_display (tree->child2, indent + 1);
    }
}

/*
 * debug_windows_tree_display: display tree of windows
 */

void
debug_windows_tree ()
{
    gui_chat_printf (NULL, "");
    gui_chat_printf (NULL, "DEBUG: windows tree:");
    debug_windows_tree_display (gui_windows_tree, 1);
}

/*
 * debug_windows_cb: callback for "debug_windows" signal hooked
 */

int
debug_windows_cb (void *data, char *signal, char *type_data, void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;
    (void) signal_data;
    
    debug_windows_tree ();
    
    return WEECHAT_RC_OK;
}

/*
 * debug_init: hook signals for debug
 */

void
debug_init ()
{
    hook_signal (NULL, "debug_dump", debug_dump_cb, NULL);
    hook_signal (NULL, "debug_buffer", debug_buffer_cb, NULL);
    hook_signal (NULL, "debug_windows", debug_windows_cb, NULL);
}
