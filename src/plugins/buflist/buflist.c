/*
 * buflist.c - Bar with list of buffers
 *
 * Copyright (C) 2003-2017 Sébastien Helleu <flashcode@flashtux.org>
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
#include <stdio.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "buflist.h"
#include "buflist-bar-item.h"
#include "buflist-command.h"
#include "buflist-config.h"
#include "buflist-mouse.h"


WEECHAT_PLUGIN_NAME(BUFLIST_PLUGIN_NAME);
WEECHAT_PLUGIN_DESCRIPTION(N_("Buffers list"));
WEECHAT_PLUGIN_AUTHOR("Sébastien Helleu <flashcode@flashtux.org>");
WEECHAT_PLUGIN_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_LICENSE(WEECHAT_LICENSE);
WEECHAT_PLUGIN_PRIORITY(8000);

struct t_weechat_plugin *weechat_buflist_plugin = NULL;

struct t_hdata *buflist_hdata_buffer = NULL;
struct t_hdata *buflist_hdata_hotlist = NULL;


/*
 * Get IRC server and channel pointers for a buffer.
 *
 * According to buffer:
 * - non IRC buffer: both are NULL
 * - IRC server/private: server is set, channel is NULL
 * - IRC channel: server and channel are set
 */

void
buflist_buffer_get_irc_pointers(struct t_gui_buffer *buffer,
                                struct t_irc_server **server,
                                struct t_irc_channel **channel)
{
    const char *ptr_server_name, *ptr_channel_name;
    char str_condition[512];
    struct t_hdata *hdata_irc_server, *hdata_irc_channel;

    *server = NULL;
    *channel = NULL;

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
    snprintf (str_condition, sizeof (str_condition),
              "${irc_server.name} == %s",
              ptr_server_name);
    *server = weechat_hdata_get_list (hdata_irc_server,
                                      "irc_servers");
    *server = weechat_hdata_search (hdata_irc_server,
                                    *server,
                                    str_condition,
                                    1);
    if (!*server)
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
    snprintf (str_condition, sizeof (str_condition),
              "${irc_channel.name} == %s",
              ptr_channel_name);
    *channel = weechat_hdata_pointer (hdata_irc_server,
                                      *server,
                                      "channels");
    *channel = weechat_hdata_search (hdata_irc_channel,
                                     *channel,
                                     str_condition,
                                     1);
}

/*
 * Compares a hdata variable of two objects.
 *
 * Returns:
 *   -1: variable1 < variable2
 *    0: variable1 == variable2
 *    1: variable1 > variable2
 */

int
buflist_compare_hdata_var (struct t_hdata *hdata,
                           void *pointer1, void *pointer2,
                           const char *variable)
{
    int type, rc, int_value1, int_value2;
    long long_value1, long_value2;
    char char_value1, char_value2;
    const char *pos, *str_value1, *str_value2;
    void *ptr_value1, *ptr_value2;
    time_t time_value1, time_value2;

    rc = 0;

    pos = strchr (variable, '|');
    type = weechat_hdata_get_var_type (hdata, (pos) ? pos + 1 : variable);
    switch (type)
    {
        case WEECHAT_HDATA_CHAR:
            char_value1 = weechat_hdata_char (hdata, pointer1, variable);
            char_value2 = weechat_hdata_char (hdata, pointer2, variable);
            rc = (char_value1 < char_value2) ?
                -1 : ((char_value1 > char_value2) ? 1 : 0);
            break;
        case WEECHAT_HDATA_INTEGER:
            int_value1 = weechat_hdata_integer (hdata, pointer1, variable);
            int_value2 = weechat_hdata_integer (hdata, pointer2, variable);
            rc = (int_value1 < int_value2) ?
                -1 : ((int_value1 > int_value2) ? 1 : 0);
            break;
        case WEECHAT_HDATA_LONG:
            long_value1 = weechat_hdata_long (hdata, pointer1, variable);
            long_value2 = weechat_hdata_long (hdata, pointer2, variable);
            rc = (long_value1 < long_value2) ?
                -1 : ((long_value1 > long_value2) ? 1 : 0);
            break;
        case WEECHAT_HDATA_STRING:
        case WEECHAT_HDATA_SHARED_STRING:
            str_value1 = weechat_hdata_string (hdata, pointer1, variable);
            str_value2 = weechat_hdata_string (hdata, pointer2, variable);
            if (!str_value1 && !str_value2)
                rc = 0;
            else if (str_value1 && !str_value2)
                rc = 1;
            else if (!str_value1 && str_value2)
                rc = -1;
            else
            {
                rc = strcmp (str_value1, str_value2);
                if (rc < 0)
                    rc = -1;
                else if (rc > 0)
                    rc = 1;
            }
            break;
        case WEECHAT_HDATA_POINTER:
            ptr_value1 = weechat_hdata_pointer (hdata, pointer1, variable);
            ptr_value2 = weechat_hdata_pointer (hdata, pointer2, variable);
            rc = (ptr_value1 < ptr_value2) ?
                -1 : ((ptr_value1 > ptr_value2) ? 1 : 0);
            break;
        case WEECHAT_HDATA_TIME:
            time_value1 = weechat_hdata_time (hdata, pointer1, variable);
            time_value2 = weechat_hdata_time (hdata, pointer2, variable);
            rc = (time_value1 < time_value2) ?
                -1 : ((time_value1 > time_value2) ? 1 : 0);
            break;
    }

    return rc;
}

/*
 * Compares two inactive merged buffers.
 *
 * Buffers are sorted so that the active buffer and buffers immediately after
 * this one are first in list, followed by the buffers before the active one.
 * This sort respects the order of next active buffers that can be selected
 * with ctrl-X.
 *
 * For example with such list of merged buffers:
 *
 *     weechat
 *     freenode
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
 *     freenode
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
    int i, reverse, rc;
    const char *ptr_field;
    struct t_gui_hotlist *ptr_hotlist1, *ptr_hotlist2;
    struct t_irc_server *ptr_server1, *ptr_server2;
    struct t_irc_channel *ptr_channel1, *ptr_channel2;
    struct t_hdata *hdata_irc_server, *hdata_irc_channel;

    /* make C compiler happy */
    (void) data;
    (void) arraylist;

    hdata_irc_server = weechat_hdata_get ("irc_server");
    hdata_irc_channel = weechat_hdata_get ("irc_channel");

    for (i = 0; i < buflist_config_sort_fields_count; i++)
    {
        rc = 0;
        reverse = 1;
        if (buflist_config_sort_fields[i][0] == '-')
        {
            ptr_field = buflist_config_sort_fields[i] + 1;
            reverse = -1;
        }
        else
        {
            ptr_field = buflist_config_sort_fields[i];
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
                rc = buflist_compare_hdata_var (buflist_hdata_hotlist,
                                                pointer1, pointer2,
                                                ptr_field + 8);
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
                rc = buflist_compare_hdata_var (hdata_irc_server,
                                                ptr_server1, ptr_server2,
                                                ptr_field + 11);
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
                rc = buflist_compare_hdata_var (hdata_irc_channel,
                                                ptr_channel1, ptr_channel2,
                                                ptr_field + 12);
            }
        }
        else
        {
            rc = buflist_compare_hdata_var (buflist_hdata_buffer,
                                            pointer1, pointer2,
                                            ptr_field);

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

    return 1;
}

/*
 * Builds a list of pointers to buffers, sorted according to option
 * "buflist.look.sort".
 *
 * Returns an arraylist that must be freed by weechat_arraylist_free after use.
 */

struct t_arraylist *
buflist_sort_buffers ()
{
    struct t_arraylist *buffers;
    struct t_gui_buffer *ptr_buffer;

    buffers = weechat_arraylist_new (128, 1, 1,
                                     &buflist_compare_buffers, NULL,
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
 * Initializes buflist plugin.
 */

int
weechat_plugin_init (struct t_weechat_plugin *plugin, int argc, char *argv[])
{
    /* make C compiler happy */
    (void) argc;
    (void) argv;

    weechat_plugin = plugin;

    buflist_hdata_buffer = weechat_hdata_get ("buffer");
    buflist_hdata_hotlist = weechat_hdata_get ("hotlist");

    if (!buflist_config_init ())
        return WEECHAT_RC_ERROR;

    buflist_config_read ();

    if (!buflist_bar_item_init ())
        return WEECHAT_RC_ERROR;

    buflist_command_init ();

    weechat_bar_new (BUFLIST_BAR_NAME, "off", "0", "root", "", "left",
                     "columns_vertical", "vertical", "0", "0",
                     "default", "default", "default", "on",
                     BUFLIST_BAR_ITEM_NAME);

    buflist_bar_item_update ();

    buflist_mouse_init ();

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

    return WEECHAT_RC_OK;
}
