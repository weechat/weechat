/*
 * wee-debug.c - debug functions
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#ifdef HAVE_MALLINFO
#include <malloc.h>
#endif
#include <string.h>
#include <time.h>

#include "weechat.h"
#include "wee-backtrace.h"
#include "wee-config-file.h"
#include "wee-hashtable.h"
#include "wee-hdata.h"
#include "wee-hook.h"
#include "wee-infolist.h"
#include "wee-list.h"
#include "wee-log.h"
#include "wee-proxy.h"
#include "wee-string.h"
#include "../gui/gui-bar.h"
#include "../gui/gui-bar-item.h"
#include "../gui/gui-buffer.h"
#include "../gui/gui-chat.h"
#include "../gui/gui-filter.h"
#include "../gui/gui-hotlist.h"
#include "../gui/gui-key.h"
#include "../gui/gui-layout.h"
#include "../gui/gui-main.h"
#include "../gui/gui-window.h"
#include "../plugins/plugin.h"


int debug_dump_active = 0;


/*
 * Writes dump of data to WeeChat log file.
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
        weechat_log_use_time = 0;
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
    gui_layout_print_log ();
    gui_key_print_log (NULL);
    gui_filter_print_log ();
    gui_bar_print_log ();
    gui_bar_item_print_log ();
    gui_hotlist_print_log ();

    hdata_print_log ();

    infolist_print_log ();

    hook_print_log ();

    config_file_print_log ();

    proxy_print_log ();

    plugin_print_log ();

    log_printf ("");
    log_printf ("******             End of WeeChat dump             ******");
    log_printf ("");
}

/*
 * Callback for signal "debug_dump".
 *
 * This function is called when WeeChat is crashing or when command
 * "/debug dump" is issued.
 */

int
debug_dump_cb (void *data, const char *signal, const char *type_data,
               void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;

    if (!signal_data || (string_strcasecmp ((char *)signal_data, "core") == 0))
        debug_dump (0);

    return WEECHAT_RC_OK;
}

/*
 * Callback for system signal SIGSEGV handler.
 *
 * Writes dump of data and backtrace to WeeChat log file, then exit.
 */

void
debug_sigsegv ()
{
    debug_dump (1);
    unhook_all ();
    gui_main_end (0);

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
 * Callback for signal "debug_buffer".
 *
 * This function is called when command "/debug buffer" is issued.
 */

int
debug_buffer_cb (void *data, const char *signal, const char *type_data,
                 void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;

    gui_buffer_dump_hexa ((struct t_gui_buffer *)signal_data);

    return WEECHAT_RC_OK;
}

/*
 * Displays tree of windows (this function must not be called directly).
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
                      "%%-%dsleaf: 0x%%lx, parent:0x%%lx, child1=0x%%lx, "
                      "child2=0x%%lx, win=0x%%lx (%%d,%%d %%dx%%d"
                      " %%d%%%%x%%d%%%%)",
                      indent * 2);
            gui_chat_printf (NULL,
                             format,
                             " ", tree, tree->parent_node,
                             tree->child1, tree->child2,
                             tree->window,
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
                      "%%-%dsnode: 0x%%lx, parent:0x%%lx, pct:%%d, "
                      "horizontal:%%d, child1=0x%%lx, child2=0x%%lx",
                      indent * 2);
            gui_chat_printf (NULL,
                             format,
                             " ", tree, tree->parent_node, tree->split_pct,
                             tree->split_horizontal,
                             tree->child1, tree->child2);
        }

        if (tree->child1)
            debug_windows_tree_display (tree->child1, indent + 1);
        if (tree->child2)
            debug_windows_tree_display (tree->child2, indent + 1);
    }
}

/*
 * Displays tree of windows.
 */

void
debug_windows_tree ()
{
    gui_chat_printf (NULL, "");
    gui_chat_printf (NULL, _("Windows tree:"));
    debug_windows_tree_display (gui_windows_tree, 1);
}

/*
 * Callback for signal "debug_windows".
 *
 * This function is called when command "/debug windows" is issued.
 */

int
debug_windows_cb (void *data, const char *signal, const char *type_data,
                  void *signal_data)
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
 * Displays information about dynamic memory allocation.
 */

void
debug_memory ()
{
#ifdef HAVE_MALLINFO
    struct mallinfo info;

    info = mallinfo ();

    gui_chat_printf (NULL, "");
    gui_chat_printf (NULL, _("Memory usage (see \"man mallinfo\" for help):"));
    gui_chat_printf (NULL, "  arena   :%10d", info.arena);
    gui_chat_printf (NULL, "  ordblks :%10d", info.ordblks);
    gui_chat_printf (NULL, "  smblks  :%10d", info.smblks);
    gui_chat_printf (NULL, "  hblks   :%10d", info.hblks);
    gui_chat_printf (NULL, "  hblkhd  :%10d", info.hblkhd);
    gui_chat_printf (NULL, "  usmblks :%10d", info.usmblks);
    gui_chat_printf (NULL, "  fsmblks :%10d", info.fsmblks);
    gui_chat_printf (NULL, "  uordblks:%10d", info.uordblks);
    gui_chat_printf (NULL, "  fordblks:%10d", info.fordblks);
    gui_chat_printf (NULL, "  keepcost:%10d", info.keepcost);
#else
    gui_chat_printf (NULL,
                     _("Memory usage not available (function \"mallinfo\" not "
                       "found)"));
#endif
}

/*
 * Callback called for each variable in hdata.
 */

void
debug_hdata_hash_var_map_cb (void *data,
                             struct t_hashtable *hashtable,
                             const void *key, const void *value)
{
    struct t_weelist *list;
    struct t_hdata_var *var;
    char str_offset[16];

    /* make C compiler happy */
    (void) hashtable;

    list = (struct t_weelist *)data;
    var = (struct t_hdata_var *)value;

    snprintf (str_offset, sizeof (str_offset), "%12d", var->offset);
    weelist_add (list, str_offset, WEECHAT_LIST_POS_SORT, (void *)key);
}

/*
 * Callback called for each list in hdata.
 */

void
debug_hdata_hash_list_map_cb (void *data,
                              struct t_hashtable *hashtable,
                              const void *key, const void *value)
{
    /* make C compiler happy */
    (void) data;
    (void) hashtable;

    gui_chat_printf (NULL,
                     "    list: %s -> 0x%lx",
                     (char *)key,
                     *((void **)value));
}

/*
 * Callback called for each hdata in memory.
 */

void
debug_hdata_map_cb (void *data, struct t_hashtable *hashtable,
                    const void *key, const void *value)
{
    struct t_hdata *ptr_hdata;
    struct t_hdata_var *ptr_var;
    struct t_weelist *list;
    struct t_weelist_item *ptr_item;

    /* make C compiler happy */
    (void) data;
    (void) hashtable;

    ptr_hdata = (struct t_hdata *)value;

    gui_chat_printf (NULL,
                     "  hdata 0x%lx: \"%s\", %d vars, %d lists:",
                     ptr_hdata, (const char *)key,
                     hashtable_get_integer (ptr_hdata->hash_var,
                                            "items_count"),
                     hashtable_get_integer (ptr_hdata->hash_list,
                                            "items_count"));

    /* display lists */
    hashtable_map (ptr_hdata->hash_list,
                   &debug_hdata_hash_list_map_cb, NULL);

    /* display vars */
    list = weelist_new ();
    hashtable_map (ptr_hdata->hash_var,
                   &debug_hdata_hash_var_map_cb, list);
    for (ptr_item = list->items; ptr_item;
         ptr_item = ptr_item->next_item)
    {
        ptr_var = hashtable_get (ptr_hdata->hash_var, ptr_item->user_data);
        if (ptr_var)
        {
            gui_chat_printf (NULL,
                             "    %04d -> %s (%s%s%s%s%s%s)",
                             ptr_var->offset,
                             (char *)ptr_item->user_data,
                             hdata_type_string[(int)ptr_var->type],
                             (ptr_var->update_allowed) ? ", R/W" : "",
                             (ptr_var->array_size) ? ", array size: " : "",
                             (ptr_var->array_size) ? ptr_var->array_size : "",
                             (ptr_var->hdata_name) ? ", hdata: " : "",
                             (ptr_var->hdata_name) ? ptr_var->hdata_name : "");
        }
    }
    weelist_free (list);
}

/*
 * Displays a list of hdata in memory.
 */

void
debug_hdata ()
{
    int count;

    count = hashtable_get_integer (weechat_hdata, "items_count");

    gui_chat_printf (NULL, "");
    gui_chat_printf (NULL, "%d hdata in memory", count);

    if (count > 0)
        hashtable_map (weechat_hdata, &debug_hdata_map_cb, NULL);
}

/*
 * Displays info about hooks.
 */

void
debug_hooks ()
{
    int i, num_hooks, num_hooks_total;
    struct t_hook *ptr_hook;

    gui_chat_printf (NULL, "");
    gui_chat_printf (NULL, "hooks in memory:");

    num_hooks_total = 0;
    for (i = 0; i < HOOK_NUM_TYPES; i++)
    {
        num_hooks = 0;
        for (ptr_hook = weechat_hooks[i]; ptr_hook;
             ptr_hook = ptr_hook->next_hook)
        {
            num_hooks++;
        }
        gui_chat_printf (NULL, "%17s:%5d", hook_type_string[i], num_hooks);
        num_hooks_total += num_hooks;
    }
    gui_chat_printf (NULL, "%17s------", "---------");
    gui_chat_printf (NULL, "%17s:%5d", "total", num_hooks_total);
}

/*
 * Displays a list of infolists in memory.
 */

void
debug_infolists ()
{
    struct t_infolist *ptr_infolist;
    struct t_infolist_item *ptr_item;
    struct t_infolist_var *ptr_var;
    int i, count, count_items, count_vars, size_structs, size_data;
    int total_items, total_vars, total_size;

    count = 0;
    for (ptr_infolist = weechat_infolists; ptr_infolist;
         ptr_infolist = ptr_infolist->next_infolist)
    {
        count++;
    }

    gui_chat_printf (NULL, "");
    gui_chat_printf (NULL, "%d infolists in memory (%s)", count,
                     (count == 0) ?
                     "this is ok!" :
                     "WARNING: this is probably a memory leak in WeeChat or "
                     "plugins/scripts!");

    if (count > 0)
    {
        i = 0;
        total_items = 0;
        total_vars = 0;
        total_size = 0;
        for (ptr_infolist = weechat_infolists; ptr_infolist;
             ptr_infolist = ptr_infolist->next_infolist)
        {
            count_items = 0;
            count_vars = 0;
            size_structs = sizeof (*ptr_infolist);
            size_data = 0;
            for (ptr_item = ptr_infolist->items; ptr_item;
                 ptr_item = ptr_item->next_item)
            {
                count_items++;
                total_items++;
                size_structs += sizeof (*ptr_item);
                for (ptr_var = ptr_item->vars; ptr_var;
                     ptr_var = ptr_var->next_var)
                {
                    count_vars++;
                    total_vars++;
                    size_structs += sizeof (*ptr_var);
                    if (ptr_var->value)
                    {
                        switch (ptr_var->type)
                        {
                            case INFOLIST_INTEGER:
                                size_data += sizeof (int);
                                break;
                            case INFOLIST_STRING:
                                size_data += strlen ((char *)(ptr_var->value));
                                break;
                            case INFOLIST_POINTER:
                                size_data += sizeof (void *);
                                break;
                            case INFOLIST_BUFFER:
                                size_data += ptr_var->size;
                                break;
                            case INFOLIST_TIME:
                                size_data += sizeof (time_t);
                                break;
                        }
                    }
                }
            }
            gui_chat_printf (NULL,
                             "%4d: infolist 0x%lx: %d items, %d vars - "
                             "structs: %d, data: %d (total: %d bytes)",
                             i + 1, ptr_infolist, count_items, count_vars,
                             size_structs, size_data, size_structs + size_data);
            total_size += size_structs + size_data;
            i++;
        }
        gui_chat_printf (NULL,
                         "Total: %d items, %d vars - %d bytes",
                         total_items, total_vars, total_size);
    }
}

/*
 * Hooks signals for debug.
 */

void
debug_init ()
{
    hook_signal (NULL, "debug_dump", &debug_dump_cb, NULL);
    hook_signal (NULL, "debug_buffer", &debug_buffer_cb, NULL);
    hook_signal (NULL, "debug_windows", &debug_windows_cb, NULL);
}
