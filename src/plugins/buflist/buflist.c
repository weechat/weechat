/*
 * buflist.c - Bar with list of buffers
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>

#include "../weechat-plugin.h"
#include "buflist.h"
#include "buflist-bar-item.h"
#include "buflist-command.h"
#include "buflist-completion.h"
#include "buflist-config.h"
#include "buflist-info.h"
#include "buflist-mouse.h"


WEECHAT_PLUGIN_NAME(BUFLIST_PLUGIN_NAME);
WEECHAT_PLUGIN_DESCRIPTION(N_("Buffers list"));
WEECHAT_PLUGIN_AUTHOR("Sébastien Helleu <flashcode@flashtux.org>");
WEECHAT_PLUGIN_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_LICENSE(WEECHAT_LICENSE);
WEECHAT_PLUGIN_PRIORITY(BUFLIST_PLUGIN_PRIORITY);

struct t_weechat_plugin *weechat_buflist_plugin = NULL;

struct t_hdata *buflist_hdata_window = NULL;
struct t_hdata *buflist_hdata_buffer = NULL;
struct t_hdata *buflist_hdata_hotlist = NULL;
struct t_hdata *buflist_hdata_bar = NULL;
struct t_hdata *buflist_hdata_bar_item = NULL;
struct t_hdata *buflist_hdata_bar_window = NULL;


/*
 * Adds the buflist bar.
 */

void
buflist_add_bar ()
{
    weechat_bar_new (
        BUFLIST_BAR_NAME,
        (weechat_config_boolean (buflist_config_look_enabled)) ? "off" : "on",
        "0", "root", "", "left",
        "columns_vertical", "vertical", "0", "0",
        "default", "default", "default", "default", "on",
        BUFLIST_BAR_ITEM_NAME);
}

/*
 * Gets IRC server and channel pointers for a buffer.
 *
 * According to buffer:
 * - non IRC buffer: both are NULL
 * - IRC server/private: server is set, channel is NULL
 * - IRC channel: server and channel are set
 */

void
buflist_buffer_get_irc_pointers (struct t_gui_buffer *buffer,
                                 void **irc_server, void **irc_channel)
{
    const char *ptr_server_name, *ptr_channel_name, *ptr_name;
    struct t_hdata *hdata_irc_server, *hdata_irc_channel;

    *irc_server = NULL;
    *irc_channel = NULL;

    /* check if the buffer belongs to IRC plugin */
    if (strcmp (weechat_buffer_get_string (buffer, "plugin"), "irc") != 0)
        return;

    /* get server name from buffer local variable */
    ptr_server_name = weechat_buffer_get_string (buffer, "localvar_server");
    if (!ptr_server_name || !ptr_server_name[0])
        return;

    /* get hdata "irc_server" (can be NULL if irc plugin is not loaded) */
    hdata_irc_server = weechat_hdata_get ("irc_server");
    if (!hdata_irc_server)
        return;

    /* search the server by name in list of servers */
    *irc_server = weechat_hdata_get_list (hdata_irc_server, "irc_servers");
    while (*irc_server)
    {
        ptr_name = weechat_hdata_string (hdata_irc_server, *irc_server, "name");
        if (strcmp (ptr_name, ptr_server_name) == 0)
            break;
        *irc_server = weechat_hdata_move (hdata_irc_server, *irc_server, 1);
    }
    if (!*irc_server)
        return;

    /* get channel name from buffer local variable */
    ptr_channel_name = weechat_buffer_get_string (buffer,
                                                  "localvar_channel");
    if (!ptr_channel_name || !ptr_channel_name[0])
        return;

    /* get hdata "irc_channel" (can be NULL if irc plugin is not loaded) */
    hdata_irc_channel = weechat_hdata_get ("irc_channel");
    if (!hdata_irc_channel)
        return;

    /* search the channel by name in list of channels on the server */
    *irc_channel = weechat_hdata_pointer (hdata_irc_server,
                                          *irc_server, "channels");
    while (*irc_channel)
    {
        ptr_name = weechat_hdata_string (hdata_irc_channel,
                                         *irc_channel, "name");
        if (strcmp (ptr_name, ptr_channel_name) == 0)
            break;
        *irc_channel = weechat_hdata_move (hdata_irc_channel, *irc_channel, 1);
    }
}

/*
 * Compares two inactive merged buffers.
 *
 * Buffers are sorted so that the active buffer and buffers immediately after
 * this one are first in list, followed by the buffers before the active one.
 * This sort respects the order of next active buffers that can be selected
 * with ctrl-x.
 *
 * For example with such list of merged buffers:
 *
 *     weechat
 *     libera
 *     oftc      (active)
 *     test
 *     another
 *
 * Buffers will be sorted like that:
 *
 *     oftc      (active)
 *     test
 *     another
 *     weechat
 *     libera
 *
 * Returns:
 *   -1: buffer1 must be sorted before buffer2
 *    0: no sort (buffer2 will be after buffer1 by default)
 *    1: buffer2 must be sorted after buffer1
 */

int
buflist_compare_inactive_merged_buffers (struct t_gui_buffer *buffer1,
                                         struct t_gui_buffer *buffer2)
{
    int number1, number, priority, priority1, priority2, active;
    struct t_gui_buffer *ptr_buffer;

    number1 = weechat_hdata_integer (buflist_hdata_buffer,
                                     buffer1, "number");

    priority = 20000;
    priority1 = 0;
    priority2 = 0;

    ptr_buffer = weechat_hdata_get_list (buflist_hdata_buffer,
                                         "gui_buffers");
    while (ptr_buffer)
    {
        number = weechat_hdata_integer (buflist_hdata_buffer,
                                        ptr_buffer, "number");
        if (number > number1)
            break;
        if (number == number1)
        {
            active = weechat_hdata_integer (buflist_hdata_buffer,
                                            ptr_buffer,
                                            "active");
            if (active > 0)
                priority += 20000;
            if (ptr_buffer == buffer1)
                priority1 = priority;
            if (ptr_buffer == buffer2)
                priority2 = priority;
            priority--;
        }
        ptr_buffer = weechat_hdata_move (buflist_hdata_buffer,
                                         ptr_buffer, 1);
    }

    return (priority1 > priority2) ?
        1 : ((priority1 < priority2) ? -1 : 0);
}

/*
 * Compares two buffers in order to add them in the sorted arraylist.
 *
 * The comparison is made using the list of fields defined in the option
 * "buflist.look.sort".
 *
 * Returns:
 *   -1: buffer1 < buffer2
 *    0: buffer1 == buffer2
 *    1: buffer1 > buffer2
 */

int
buflist_compare_buffers (void *data, struct t_arraylist *arraylist,
                         void *pointer1, void *pointer2)
{
    int i, item_number, reverse, case_sensitive, rc;
    const char *ptr_field;
    struct t_gui_hotlist *ptr_hotlist1, *ptr_hotlist2;
    void *ptr_server1, *ptr_server2, *ptr_channel1, *ptr_channel2;
    struct t_hdata *hdata_irc_server, *hdata_irc_channel;
    struct t_gui_bar_item *ptr_item;

    /* make C compiler happy */
    (void) arraylist;

    ptr_item = (struct t_gui_bar_item *)data;
    item_number = buflist_bar_item_get_index_with_pointer (ptr_item);
    if (item_number < 0)
        item_number= 0;

    hdata_irc_server = weechat_hdata_get ("irc_server");
    hdata_irc_channel = weechat_hdata_get ("irc_channel");

    for (i = 0; i < buflist_config_sort_fields_count[item_number]; i++)
    {
        rc = 0;
        reverse = 1;
        case_sensitive = 1;
        ptr_field = buflist_config_sort_fields[item_number][i];
        while ((ptr_field[0] == '-') || (ptr_field[0] == '~'))
        {
            if (ptr_field[0] == '-')
                reverse *= -1;
            else if (ptr_field[0] == '~')
                case_sensitive ^= 1;
            ptr_field++;
        }
        if (strncmp (ptr_field, "hotlist.", 8) == 0)
        {
            ptr_hotlist1 = weechat_hdata_pointer (buflist_hdata_buffer,
                                                  pointer1, "hotlist");
            ptr_hotlist2 = weechat_hdata_pointer (buflist_hdata_buffer,
                                                  pointer2, "hotlist");
            if (!ptr_hotlist1 && !ptr_hotlist2)
                rc = 0;
            else if (ptr_hotlist1 && !ptr_hotlist2)
                rc = 1;
            else if (!ptr_hotlist1 && ptr_hotlist2)
                rc = -1;
            else
            {
                rc = weechat_hdata_compare (buflist_hdata_hotlist,
                                            ptr_hotlist1, ptr_hotlist2,
                                            ptr_field + 8,
                                            case_sensitive);
            }
        }
        else if (strncmp (ptr_field, "irc_server.", 11) == 0)
        {
            if (hdata_irc_server)
            {
                buflist_buffer_get_irc_pointers (pointer1,
                                                 &ptr_server1, &ptr_channel1);
                buflist_buffer_get_irc_pointers (pointer2,
                                                 &ptr_server2, &ptr_channel2);
                rc = weechat_hdata_compare (hdata_irc_server,
                                            ptr_server1, ptr_server2,
                                            ptr_field + 11,
                                            case_sensitive);
            }
        }
        else if (strncmp (ptr_field, "irc_channel.", 12) == 0)
        {
            if (hdata_irc_channel)
            {
                buflist_buffer_get_irc_pointers (pointer1,
                                                 &ptr_server1, &ptr_channel1);
                buflist_buffer_get_irc_pointers (pointer2,
                                                 &ptr_server2, &ptr_channel2);
                rc = weechat_hdata_compare (hdata_irc_channel,
                                            ptr_channel1, ptr_channel2,
                                            ptr_field + 12,
                                            case_sensitive);
            }
        }
        else
        {
            rc = weechat_hdata_compare (buflist_hdata_buffer,
                                        pointer1, pointer2,
                                        ptr_field,
                                        case_sensitive);

            /*
             * In case we are sorting on "active" flag and that both
             * buffers have same value (it should be 0),
             * we sort buffers so that the buffers immediately after the
             * active one is first in list, followed by the next ones, followed
             * by the buffers before the active one.
             */
            if ((rc == 0)
                && (strcmp (ptr_field, "active") == 0)
                && (weechat_hdata_integer (buflist_hdata_buffer,
                                           pointer1, "number") ==
                    weechat_hdata_integer (buflist_hdata_buffer,
                                           pointer2, "number")))
            {
                rc = buflist_compare_inactive_merged_buffers (pointer1,
                                                              pointer2);
            }
        }
        rc *= reverse;
        if (rc != 0)
            return rc;
    }

    return 0;
}

/*
 * Builds a list of pointers to buffers, sorted according to option
 * "buflist.look.sort".
 *
 * Returns an arraylist that must be freed by weechat_arraylist_free after use.
 */

struct t_arraylist *
buflist_sort_buffers (struct t_gui_bar_item *item)
{
    struct t_arraylist *buffers;
    struct t_gui_buffer *ptr_buffer;

    buffers = weechat_arraylist_new (128, 1, 1,
                                     &buflist_compare_buffers, item,
                                     NULL, NULL);

    ptr_buffer = weechat_hdata_get_list (buflist_hdata_buffer, "gui_buffers");
    while (ptr_buffer)
    {
        weechat_arraylist_add (buffers, ptr_buffer);
        ptr_buffer = weechat_hdata_move (buflist_hdata_buffer, ptr_buffer, 1);
    }

    return buffers;
}

/*
 * Callback called when a Perl script is loaded: if the script is buffers.pl,
 * then we display a warning.
 */

int
buflist_script_loaded_cb (const void *pointer, void *data, const char *signal,
                          const char *type_data, void *signal_data)
{
    char *base_name, *base_name2;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) signal;
    (void) type_data;

    /* display a warning only if buflist is enabled */
    if (!weechat_config_boolean (buflist_config_look_enabled))
        return WEECHAT_RC_OK;

    if (!signal_data)
        return WEECHAT_RC_OK;

    base_name = basename (signal_data);
    if (!base_name)
        return WEECHAT_RC_OK;
    base_name2 = strdup (base_name);
    if (!base_name2)
        return WEECHAT_RC_OK;

    if (strcmp (base_name2, "buffers.pl") == 0)
    {
        weechat_printf (NULL,
                        _("%sbuflist: warning: the script buffers.pl is "
                          "loaded and provides a bar with list of buffers "
                          "similar to the buflist plugin; you may want to "
                          "uninstall the script buffers.pl "
                          "(/script remove buffers.pl) or disable/unload the "
                          "buflist plugin; see WeeChat release notes for more "
                          "information"),
                        weechat_prefix ("error"));
    }

    free (base_name2);

    return WEECHAT_RC_OK;
}

/*
 * Initializes buflist plugin.
 */

int
weechat_plugin_init (struct t_weechat_plugin *plugin, int argc, char *argv[])
{
    struct t_hashtable *keys;
    int i;
    char str_key[256];
    char *default_keys[][2] = {
        { "meta-B",  "/buflist toggle"             },
        { "f1",      "/bar scroll buflist * -100%" },
        { "f2",      "/bar scroll buflist * +100%" },
        { "ctrl-f1", "/bar scroll buflist * -100%" },
        { "ctrl-f2", "/bar scroll buflist * +100%" },
        { "meta-f1", "/bar scroll buflist * b"     },
        { "meta-f2", "/bar scroll buflist * e"     },
        { NULL, NULL },
    };

    /* make C compiler happy */
    (void) argc;
    (void) argv;

    weechat_plugin = plugin;

    buflist_hdata_window = weechat_hdata_get ("window");
    buflist_hdata_buffer = weechat_hdata_get ("buffer");
    buflist_hdata_hotlist = weechat_hdata_get ("hotlist");
    buflist_hdata_bar = weechat_hdata_get ("bar");
    buflist_hdata_bar_item = weechat_hdata_get ("bar_item");
    buflist_hdata_bar_window = weechat_hdata_get ("bar_window");

    if (!buflist_config_init ())
        return WEECHAT_RC_ERROR;

    buflist_config_read ();

    if (!buflist_bar_item_init ())
        return WEECHAT_RC_ERROR;

    buflist_config_change_sort (NULL, NULL, NULL);

    buflist_command_init ();
    buflist_completion_init ();

    buflist_add_bar ();

    buflist_bar_item_update (-1, 0);

    buflist_mouse_init ();

    /* default keys and mouse actions */
    keys = weechat_hashtable_new (32,
                                  WEECHAT_HASHTABLE_STRING,
                                  WEECHAT_HASHTABLE_STRING,
                                  NULL, NULL);
    if (keys)
    {
        /* default keys */
        for (i = 0; default_keys[i][0]; i++)
        {
            weechat_hashtable_set (keys,
                                   default_keys[i][0], default_keys[i][1]);
        }
        weechat_hashtable_set (keys, "__quiet", "1");
        weechat_key_bind ("default", keys);

        /* default mouse actions */
        weechat_hashtable_remove_all (keys);
        for (i = 0; i < BUFLIST_BAR_NUM_ITEMS; i++)
        {
            snprintf (str_key, sizeof (str_key),
                      "@item(%s):button1*",
                      buflist_bar_item_get_name (i));
            weechat_hashtable_set (keys,
                                   str_key, "hsignal:" BUFLIST_MOUSE_HSIGNAL);
            snprintf (str_key, sizeof (str_key),
                      "@item(%s):button2*",
                      buflist_bar_item_get_name (i));
            weechat_hashtable_set (keys,
                                   str_key, "hsignal:" BUFLIST_MOUSE_HSIGNAL);
        }
        weechat_hashtable_set (keys,
                               "@bar(" BUFLIST_BAR_NAME "):ctrl-wheelup",
                               "hsignal:" BUFLIST_MOUSE_HSIGNAL);
        weechat_hashtable_set (keys,
                               "@bar(" BUFLIST_BAR_NAME "):ctrl-wheeldown",
                               "hsignal:" BUFLIST_MOUSE_HSIGNAL);
        weechat_hashtable_set (keys, "__quiet", "1");
        weechat_key_bind ("mouse", keys);
    }
    weechat_hashtable_free (keys);

    weechat_hook_signal ("perl_script_loaded",
                         &buflist_script_loaded_cb, NULL, NULL);

    buflist_info_init ();

    return WEECHAT_RC_OK;
}

/*
 * Ends buflist plugin.
 */

int
weechat_plugin_end (struct t_weechat_plugin *plugin)
{
    /* make C compiler happy */
    (void) plugin;

    buflist_mouse_end ();

    buflist_bar_item_end ();

    buflist_config_write ();
    buflist_config_free ();

    buflist_hdata_window = NULL;
    buflist_hdata_buffer = NULL;
    buflist_hdata_hotlist = NULL;
    buflist_hdata_bar = NULL;
    buflist_hdata_bar_item = NULL;
    buflist_hdata_bar_window = NULL;

    return WEECHAT_RC_OK;
}
