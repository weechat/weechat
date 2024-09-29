/*
 * core-debug.c - debug functions
 *
 * Copyright (C) 2003-2024 Sébastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#if defined(HAVE_MALLINFO) || defined(HAVE_MALLINFO2)
#include <malloc.h>
#endif
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <gcrypt.h>
#include <curl/curl.h>
#include <zlib.h>
#ifdef HAVE_ZSTD
#include <zstd.h>
#endif
#ifdef HAVE_CJSON
#include <cjson/cJSON.h>
#endif

#include <gnutls/gnutls.h>

#include "weechat.h"
#include "core-backtrace.h"
#include "core-config-file.h"
#include "core-hashtable.h"
#include "core-hdata.h"
#include "core-hook.h"
#include "core-infolist.h"
#include "core-list.h"
#include "core-log.h"
#include "core-proxy.h"
#include "core-string.h"
#include "core-utf8.h"
#include "core-util.h"
#include "core-version.h"
#include "../gui/gui-bar.h"
#include "../gui/gui-bar-item.h"
#include "../gui/gui-buffer.h"
#include "../gui/gui-chat.h"
#include "../gui/gui-color.h"
#include "../gui/gui-completion.h"
#include "../gui/gui-filter.h"
#include "../gui/gui-hotlist.h"
#include "../gui/gui-key.h"
#include "../gui/gui-layout.h"
#include "../gui/gui-main.h"
#include "../gui/gui-window.h"
#include "../plugins/plugin.h"

#define DEBUG_DISPLAY_BUILD_OPTION_STR(OPTION)                  \
    string_fprintf (stdout, "  %s: \"%s\"\n", #OPTION, OPTION);
#define DEBUG_DISPLAY_BUILD_OPTION_BOOL(OPTION)                 \
    string_fprintf (stdout,                                     \
                    "  %s: %s\n",                               \
                    #OPTION,                                    \
                    (OPTION) ? "ON" : "OFF");

int debug_dump_active = 0;

long long debug_long_callbacks = 0;    /* callbacks taking more than        */
                                       /* N microseconds will be traced     */


/*
 * Displays build information on stdout.
 */

void
debug_build_info ()
{
    /* display version and compilation date/time */
    string_fprintf (
        stdout,
        /* TRANSLATORS: "%s %s" after "compiled on" is date and time */
        _("WeeChat %s, compiled on %s %s\n"),
        version_get_version_with_git (),
        version_get_compilation_date (),
        version_get_compilation_time ());

    /* display build options */
    string_fprintf (stdout, _("Build options:\n"));
    DEBUG_DISPLAY_BUILD_OPTION_STR(CMAKE_BUILD_TYPE);
    DEBUG_DISPLAY_BUILD_OPTION_STR(CMAKE_INSTALL_PREFIX);
    DEBUG_DISPLAY_BUILD_OPTION_STR(WEECHAT_HOME);
    DEBUG_DISPLAY_BUILD_OPTION_BOOL(ENABLE_ALIAS);
    DEBUG_DISPLAY_BUILD_OPTION_BOOL(ENABLE_BUFLIST);
    DEBUG_DISPLAY_BUILD_OPTION_BOOL(ENABLE_CHARSET);
    DEBUG_DISPLAY_BUILD_OPTION_BOOL(ENABLE_CJSON);
    DEBUG_DISPLAY_BUILD_OPTION_BOOL(ENABLE_CODE_COVERAGE);
    DEBUG_DISPLAY_BUILD_OPTION_BOOL(ENABLE_DOC);
    DEBUG_DISPLAY_BUILD_OPTION_BOOL(ENABLE_DOC_INCOMPLETE);
    DEBUG_DISPLAY_BUILD_OPTION_BOOL(ENABLE_ENCHANT);
    DEBUG_DISPLAY_BUILD_OPTION_BOOL(ENABLE_EXEC);
    DEBUG_DISPLAY_BUILD_OPTION_BOOL(ENABLE_FIFO);
    DEBUG_DISPLAY_BUILD_OPTION_BOOL(ENABLE_FSET);
    DEBUG_DISPLAY_BUILD_OPTION_BOOL(ENABLE_GUILE);
    DEBUG_DISPLAY_BUILD_OPTION_BOOL(ENABLE_HEADLESS);
    DEBUG_DISPLAY_BUILD_OPTION_BOOL(ENABLE_IRC);
    DEBUG_DISPLAY_BUILD_OPTION_BOOL(ENABLE_JAVASCRIPT);
    DEBUG_DISPLAY_BUILD_OPTION_BOOL(ENABLE_LARGEFILE);
    DEBUG_DISPLAY_BUILD_OPTION_BOOL(ENABLE_LOGGER);
    DEBUG_DISPLAY_BUILD_OPTION_BOOL(ENABLE_LUA);
    DEBUG_DISPLAY_BUILD_OPTION_BOOL(ENABLE_MAN);
    DEBUG_DISPLAY_BUILD_OPTION_BOOL(ENABLE_NCURSES);
    DEBUG_DISPLAY_BUILD_OPTION_BOOL(ENABLE_NLS);
    DEBUG_DISPLAY_BUILD_OPTION_BOOL(ENABLE_PERL);
    DEBUG_DISPLAY_BUILD_OPTION_BOOL(ENABLE_PHP);
    DEBUG_DISPLAY_BUILD_OPTION_BOOL(ENABLE_PYTHON);
    DEBUG_DISPLAY_BUILD_OPTION_BOOL(ENABLE_RELAY);
    DEBUG_DISPLAY_BUILD_OPTION_BOOL(ENABLE_RUBY);
    DEBUG_DISPLAY_BUILD_OPTION_BOOL(ENABLE_SCRIPT);
    DEBUG_DISPLAY_BUILD_OPTION_BOOL(ENABLE_SCRIPTS);
    DEBUG_DISPLAY_BUILD_OPTION_BOOL(ENABLE_SPELL);
    DEBUG_DISPLAY_BUILD_OPTION_BOOL(ENABLE_TCL);
    DEBUG_DISPLAY_BUILD_OPTION_BOOL(ENABLE_TESTS);
    DEBUG_DISPLAY_BUILD_OPTION_BOOL(ENABLE_TRIGGER);
    DEBUG_DISPLAY_BUILD_OPTION_BOOL(ENABLE_TYPING);
    DEBUG_DISPLAY_BUILD_OPTION_BOOL(ENABLE_XFER);
    DEBUG_DISPLAY_BUILD_OPTION_BOOL(ENABLE_ZSTD);
}

/*
 * Writes dump of data to WeeChat log file.
 */

void
debug_dump (int crash)
{
    /* prevent reentrancy */
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
        log_printf ("****** WeeChat CRASH DUMP ******");
    else
        log_printf ("****** WeeChat dump request ******");

    gui_window_print_log ();
    gui_buffer_print_log ();
    gui_completion_print_log ();
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
    log_printf ("****** End of WeeChat dump ******");
    log_printf ("");
}

/*
 * Callback for signal "debug_dump".
 *
 * This function is called when WeeChat is crashing or when command
 * "/debug dump" is issued.
 */

int
debug_dump_cb (const void *pointer, void *data,
               const char *signal, const char *type_data,
               void *signal_data)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) signal;
    (void) type_data;

    if (!signal_data || (strcmp ((char *)signal_data, PLUGIN_CORE) == 0))
        debug_dump (0);

    return WEECHAT_RC_OK;
}

/*
 * Callback for system signal SIGSEGV handler.
 *
 * Writes dump of data and backtrace to WeeChat log file, then exit.
 */

void
debug_sigsegv_cb ()
{
    debug_dump (1);
    unhook_all ();
    gui_main_end (0);

    string_fprintf (stderr,
                    "\n*** Very bad! WeeChat is crashing (SIGSEGV received)"
                    "\n");
    if (!log_crash_rename ())
    {
        string_fprintf (stderr,
                        "*** Full crash dump was saved to %s/weechat.log file."
                        "\n",
                        weechat_state_dir);
    }
    string_fprintf (
        stderr,
        "***\n"
        "*** Please help WeeChat developers to fix this bug:\n"
        "***\n"
        "***   1. If you have a core file, please run: gdb /path/to/weechat core\n"
        "***      then issue command: \"bt full\" and send result to developers.\n"
        "***      See the user's guide for more info about enabling the core files\n"
        "***      and reporting crashes:\n"
        "***      https://weechat.org/doc/weechat/stable/user/#report_crashes\n"
        "***\n"
        "***   2. Otherwise send the backtrace (below), only if it is a complete trace.\n"
        "***      Keep the crash log file, just in case developers ask you some info\n"
        "***      (be careful, private info like passwords may be in this file).\n\n");

    weechat_backtrace ();

    /* shutdown with error code */
    weechat_shutdown (EXIT_FAILURE, 1);
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
                      "%%-%dsleaf: "
                      "%%p, "
                      "parent:%%p, "
                      "child1=%%p, "
                      "child2=%%p, "
                      "win=%%p "
                      "(%%d,%%d %%dx%%d %%d%%%%x%%d%%%%)",
                      indent * 2);
            gui_chat_printf (NULL,
                             format,
                             " ",
                             tree,
                             tree->parent_node,
                             tree->child1,
                             tree->child2,
                             tree->window,
                             tree->window->win_x,
                             tree->window->win_y,
                             tree->window->win_width,
                             tree->window->win_height,
                             tree->window->win_width_pct,
                             tree->window->win_height_pct);
        }
        else
        {
            /* node */
            snprintf (format,
                      sizeof (format),
                      "%%-%dsnode: "
                      "%%p, "
                      "parent:%%p, "
                      "pct:%%d, "
                      "horizontal:%%d, "
                      "child1=%%p, "
                      "child2=%%p",
                      indent * 2);
            gui_chat_printf (NULL,
                             format,
                             " ",
                             tree,
                             tree->parent_node,
                             tree->split_pct,
                             tree->split_horizontal,
                             tree->child1,
                             tree->child2);
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
 * Displays information about dynamic memory allocation.
 */

void
debug_memory ()
{
#ifdef HAVE_MALLINFO2
    struct mallinfo2 info;

    info = mallinfo2 ();

    gui_chat_printf (NULL, "");
    gui_chat_printf (NULL, _("Memory usage (see \"man mallinfo\" for help):"));
    gui_chat_printf (NULL, "  arena   :%10zu", info.arena);
    gui_chat_printf (NULL, "  ordblks :%10zu", info.ordblks);
    gui_chat_printf (NULL, "  smblks  :%10zu", info.smblks);
    gui_chat_printf (NULL, "  hblks   :%10zu", info.hblks);
    gui_chat_printf (NULL, "  hblkhd  :%10zu", info.hblkhd);
    gui_chat_printf (NULL, "  usmblks :%10zu", info.usmblks);
    gui_chat_printf (NULL, "  fsmblks :%10zu", info.fsmblks);
    gui_chat_printf (NULL, "  uordblks:%10zu", info.uordblks);
    gui_chat_printf (NULL, "  fordblks:%10zu", info.fordblks);
    gui_chat_printf (NULL, "  keepcost:%10zu", info.keepcost);
#else
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
#endif /* HAVE_MALLINFO */
#endif /* HAVE_MALLINFO2 */
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
                     "    list: %s -> %p",
                     (char *)key,
                     *((void **)value));
}

/*
 * Callback called for each hdata in memory.
 */

void
debug_hdata_map_cb (void *data,
                    struct t_hashtable *hashtable,
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
                     "  hdata %p: \"%s\", %d vars, %d lists:",
                     ptr_hdata,
                     (const char *)key,
                     ptr_hdata->hash_var->items_count,
                     ptr_hdata->hash_list->items_count);

    /* display lists */
    hashtable_map (ptr_hdata->hash_list, &debug_hdata_hash_list_map_cb, NULL);

    /* display vars */
    list = weelist_new ();
    hashtable_map (ptr_hdata->hash_var, &debug_hdata_hash_var_map_cb, list);
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

    count = weechat_hdata->items_count;

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
    int i;

    gui_chat_printf (NULL, "");
    gui_chat_printf (NULL, "hooks in memory:");

    for (i = 0; i < HOOK_NUM_TYPES; i++)
    {
        gui_chat_printf (NULL, "%17s:%5d",
                         hook_type_string[i], hooks_count[i]);
    }
    gui_chat_printf (NULL, "%17s------", "---------");
    gui_chat_printf (NULL, "%17s:%5d", "total", hooks_count_total);
}

/*
 * Displays info about hooks for a specific plugin.
 */

void
debug_hooks_plugin (const char *plugin_name)
{
    struct t_weechat_plugin *ptr_plugin;
    struct t_hook *ptr_hook;
    char *desc, **result, **result_type, str_type[128];
    int i, count_total, count_type;

    if (!plugin_name)
        return;

    if (strcmp (plugin_name, PLUGIN_CORE) == 0)
    {
        ptr_plugin = NULL;
    }
    else
    {
        ptr_plugin = plugin_search (plugin_name);
        if (!ptr_plugin)
        {
            gui_chat_printf (NULL, "%sPlugin \"%s\" not found",
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             plugin_name);
            return;
        }
    }

    result = string_dyn_alloc (1024);
    if (!result)
        return;

    result_type = string_dyn_alloc (1024);
    if (!result_type)
    {
        string_dyn_free (result, 1);
        return;
    }

    count_total = 0;

    for (i = 0; i < HOOK_NUM_TYPES; i++)
    {
        count_type = 0;
        string_dyn_copy (result_type, NULL);

        for (ptr_hook = weechat_hooks[i]; ptr_hook;
             ptr_hook = ptr_hook->next_hook)
        {
            if (ptr_hook->deleted || (ptr_hook->plugin != ptr_plugin))
                continue;

            desc = hook_get_description (ptr_hook);
            if (desc)
            {
                string_dyn_concat (result_type, "    ", -1);
                string_dyn_concat (result_type, desc, -1);
                string_dyn_concat (result_type, "\n", -1);
                free (desc);
            }
            count_type++;
        }

        snprintf (str_type, sizeof (str_type),
                  "  %s (%d)%s\n",
                  hook_type_string[i],
                  count_type,
                  (count_type > 0) ? ":" : "");
        string_dyn_concat (result, str_type, -1);

        if (count_type > 0)
            string_dyn_concat (result, *result_type, -1);

        count_total += count_type;
    }

    if (count_total > 0)
    {
        gui_chat_printf (NULL, "");
        gui_chat_printf (NULL,
                         "hooks in plugin \"%s\" (%d)%s",
                         plugin_name,
                         count_total,
                         (count_total > 0) ? ":" : "");
        gui_chat_printf (NULL, *result);
    }
    else
    {
        gui_chat_printf (NULL, "No hooks in plugin \"%s\"", plugin_name);
    }
    string_dyn_free (result, 1);
    string_dyn_free (result_type, 1);
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
                     "this is OK!" :
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
                            case INFOLIST_NUM_TYPES:
                                break;
                        }
                    }
                }
            }
            gui_chat_printf (NULL,
                             "%4d: infolist %p: %d items, %d vars - "
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
 * Callback for signal "debug_libs": displays infos about external libraries
 * used (called when command "/debug libs" is issued).
 *
 * Note: this function displays libraries for WeeChat core only: plugins can
 * catch this signal to display their external libraries.
 */

int
debug_libs_cb (const void *pointer, void *data,
               const char *signal, const char *type_data,
               void *signal_data)
{
    const char *version_gcrypt = GCRYPT_VERSION;
    const char *version_gnutls = GNUTLS_VERSION;
    const char *version_libcurl = LIBCURL_VERSION;
    const char *version_zlib = ZLIB_VERSION;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) signal;
    (void) type_data;
    (void) signal_data;

    gui_chat_printf (NULL, "  core:");

    /* display ncurses version */
    gui_main_debug_libs ();

    /* display gcrypt version */
    gui_chat_printf (NULL, "    gcrypt: %s%s%s%s",
                     version_gcrypt,
                     (weechat_no_gcrypt) ? " (" : "",
                     (weechat_no_gcrypt) ? _("not initialized") : "",
                     (weechat_no_gcrypt) ? ")" : "");

    /* display gnutls version */
    gui_chat_printf (NULL, "    gnutls: %s%s%s%s",
                     version_gnutls,
                     (weechat_no_gnutls) ? " (" : "",
                     (weechat_no_gnutls) ? _("not initialized") : "",
                     (weechat_no_gnutls) ? ")" : "");

    /* display curl version */
    gui_chat_printf (NULL, "    curl: %s", version_libcurl);

    /* display zlib version */
    gui_chat_printf (NULL, "    zlib: %s", version_zlib);

    /* display zstd version */
#ifdef HAVE_ZSTD
    gui_chat_printf (NULL, "    zstd: %d.%d.%d",
                    ZSTD_VERSION_MAJOR,
                    ZSTD_VERSION_MINOR,
                    ZSTD_VERSION_RELEASE);
#else
    gui_chat_printf (NULL, "    zstd: %s", _("not available"));
#endif /* HAVE_ZSTD */

    /* display cJSON version */
#ifdef HAVE_CJSON
    gui_chat_printf (NULL, "    cJSON: %d.%d.%d",
                     CJSON_VERSION_MAJOR,
                     CJSON_VERSION_MINOR,
                     CJSON_VERSION_PATCH);
#else
    gui_chat_printf (NULL, "    cJSON: %s", _("not available"));
#endif /* HAVE_CJSON */

    return WEECHAT_RC_OK;
}

/*
 * Displays WeeChat directories.
 */

void
debug_directories ()
{
    char *extra_libdir, str_temp[1024];

    extra_libdir = getenv (WEECHAT_EXTRA_LIBDIR);

    if (weechat_home_temp)
    {
        snprintf (str_temp, sizeof (str_temp),
                  " (%s)", _("TEMPORARY, deleted on exit"));
    }
    else
    {
        str_temp[0] = '\0';
    }

    gui_chat_printf (NULL, "");
    gui_chat_printf (NULL, _("Directories:"));
    gui_chat_printf (NULL, "  home:");
    gui_chat_printf (NULL, "    config: %s%s", weechat_config_dir, str_temp);
    gui_chat_printf (NULL, "    data: %s%s", weechat_data_dir, str_temp);
    gui_chat_printf (NULL, "    state: %s%s", weechat_state_dir, str_temp);
    gui_chat_printf (NULL, "    cache: %s%s", weechat_cache_dir, str_temp);
    gui_chat_printf (NULL, "    runtime: %s%s", weechat_runtime_dir, str_temp);
    gui_chat_printf (NULL, "  lib: %s", WEECHAT_LIBDIR);
    gui_chat_printf (NULL, "  lib (extra): %s",
                     (extra_libdir && extra_libdir[0]) ? extra_libdir : "-");
    gui_chat_printf (NULL, "  share: %s", WEECHAT_SHAREDIR);
    gui_chat_printf (NULL, "  locale: %s", LOCALEDIR);
}

/*
 * Display time elapsed between two times.
 *
 * If display is 1, the message is displayed in core buffer, otherwise it's
 * written in log file.
 */

void
debug_display_time_elapsed (struct timeval *time1, struct timeval *time2,
                            const char *message, int display)
{
    struct timeval debug_timeval_end;
    char *str_diff;
    long long diff;

    gettimeofday (&debug_timeval_end, NULL);
    diff = util_timeval_diff (time1, time2);
    str_diff = util_get_microseconds_string (diff);

    if (display)
    {
        gui_chat_printf (NULL,
                         "debug: time[%s] -> %s",
                         (message) ? message : "?",
                         (str_diff) ? str_diff : "?");
    }
    else
    {
        log_printf ("debug: time[%s] -> %s",
                    (message) ? message : "?",
                    (str_diff) ? str_diff : "?");
    }

    free (str_diff);
}

/*
 * Display unicode information for a codepoint.
 */

void
debug_unicode_char (unsigned int codepoint)
{
    char utf8_char[5], hexa[64], *ptr_hexa;
    int i, size, width;

    utf8_int_string (codepoint, utf8_char);
    size = strlen (utf8_char);
    width = wcwidth ((wchar_t)codepoint);

    hexa[0] = '\0';
    ptr_hexa = hexa;
    for (i = 0; i < size; i++)
    {
        ptr_hexa[0] = '0';
        ptr_hexa[1] = 'x';
        ptr_hexa += 2;
        string_base16_encode (utf8_char + i, 1, ptr_hexa);
        ptr_hexa += 2;
        if (i < size - 1)
        {
            ptr_hexa[0] = ' ';
            ptr_hexa++;
        }
    }
    ptr_hexa[0] = '\0';

    gui_chat_printf (NULL,
                     "\t  \"%s\" (U+%04X, %u, %s): %d %s/%s %d, %d %s/%s "
                     "%d, %d, %d, %d",
                     utf8_char,
                     codepoint,
                     codepoint,
                     hexa,
                     strlen (utf8_char),
                     GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                     GUI_COLOR(GUI_COLOR_CHAT),
                     utf8_strlen (utf8_char),
                     gui_chat_strlen (utf8_char),
                     GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                     GUI_COLOR(GUI_COLOR_CHAT),
                     width,
                     utf8_char_size_screen (utf8_char),
                     utf8_strlen_screen (utf8_char),
                     gui_chat_strlen_screen (utf8_char));
}

/*
 * Display unicode information for a string.
 */

void
debug_unicode_string (const char *string)
{
    int num_char, width;
    wchar_t *wstring;

    num_char = mbstowcs (NULL, string, 0) + 1;
    wstring = malloc ((num_char + 1) * sizeof (wstring[0]));
    if (!wstring)
        return;

    if (mbstowcs (wstring, string, num_char) == (size_t)(-1))
    {
        free (wstring);
        return;
    }

    width = wcswidth (wstring, num_char);

    gui_chat_printf (NULL,
                     "\t  \"%s\": %d %s/%s %d, %d %s/%s %d, %d, %d",
                     string,
                     strlen (string),
                     GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                     GUI_COLOR(GUI_COLOR_CHAT),
                     utf8_strlen (string),
                     gui_chat_strlen (string),
                     GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                     GUI_COLOR(GUI_COLOR_CHAT),
                     width,
                     utf8_strlen_screen (string),
                     gui_chat_strlen_screen (string));

    free (wstring);
}

/*
 * Display information about all unicode chars of a string.
 */

void
debug_unicode (const char *string)
{
    const char *ptr_string;
    if (!string || !string[0])
        return;

    /* info about string */
    gui_chat_printf (NULL, "");
    gui_chat_printf (NULL,
                     _("Unicode: \"string\": "
                       "strlen %s/%s "
                       "utf8_strlen, gui_chat_strlen %s/%s "
                       "wcswidth, utf8_strlen_screen, "
                       "gui_chat_strlen_screen:"),
                     GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                     GUI_COLOR(GUI_COLOR_CHAT),
                     GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                     GUI_COLOR(GUI_COLOR_CHAT));
    debug_unicode_string (string);

    /* info about chars in string */
    gui_chat_printf (NULL, "");
    gui_chat_printf (NULL,
                     _("Unicode: \"char\" "
                       "(hex codepoint, codepoint, UTF-8 sequence): "
                       "strlen %s/%s "
                       "utf8_strlen, gui_chat_strlen %s/%s "
                       "wcwidth, utf8_char_size_screen, utf8_strlen_screen, "
                       "gui_chat_strlen_screen:"),
                     GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                     GUI_COLOR(GUI_COLOR_CHAT),
                     GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                     GUI_COLOR(GUI_COLOR_CHAT));
    ptr_string = string;
    while (ptr_string && ptr_string[0])
    {
        debug_unicode_char (utf8_char_int (ptr_string));
        ptr_string = utf8_next_char (ptr_string);
    }
}

/*
 * Initializes debug.
 */

void
debug_init ()
{
    /*
     * hook signals with high priority, to be sure they will be used before
     * plugins (they should anyway because this function is called before load
     * of plugins)
     */
    hook_signal (NULL, "2000|debug_dump", &debug_dump_cb, NULL, NULL);
    hook_signal (NULL, "2000|debug_libs", &debug_libs_cb, NULL, NULL);
}

/*
 * Ends debug.
 */

void
debug_end ()
{
}
