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
#include <string.h>

#include "../weechat-plugin.h"
#include "buflist.h"
#include "buflist-bar-item.h"
#include "buflist-config.h"


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

    /* make C compiler happy */
    (void) data;
    (void) arraylist;

    for (i = 0; i < buflist_config_sort_fields_count; i++)
    {
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
        rc = 0;
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
        else
        {
            rc = buflist_compare_hdata_var (buflist_hdata_buffer,
                                            pointer1, pointer2,
                                            ptr_field);
        }
        rc *= reverse;
        if (rc != 0)
            return rc;
    }

    return rc;
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
 * Callback for a signal on a buffer.
 */

int
buflist_signal_buffer_cb (const void *pointer, void *data,
                          const char *signal, const char *type_data,
                          void *signal_data)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) signal;
    (void) type_data;
    (void) signal_data;

    weechat_bar_item_update (BUFLIST_BAR_ITEM_NAME);

    return WEECHAT_RC_OK;
}

/*
 * Initializes buflist plugin.
 */

int
weechat_plugin_init (struct t_weechat_plugin *plugin, int argc, char *argv[])
{
    char *signals_buffers[] =
    { "buffer_opened", "buffer_closed", "buffer_merged", "buffer_unmerged",
      "buffer_moved", "buffer_renamed", "buffer_switch", "buffer_hidden",
      "buffer_unhidden", "buffer_localvar_added", "buffer_localvar_changed",
      "window_switch", "hotlist_changed", NULL
    };
    int i;

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

    /* hook some signals */
    for (i = 0; signals_buffers[i]; i++)
    {
        weechat_hook_signal (signals_buffers[i],
                             &buflist_signal_buffer_cb, NULL, NULL);
    }

    weechat_bar_new (BUFLIST_BAR_NAME, "off", "0", "root", "", "left",
                     "columns_vertical", "vertical", "0", "0",
                     "default", "default", "default", "on",
                     BUFLIST_BAR_ITEM_NAME);

    weechat_bar_item_update (BUFLIST_BAR_ITEM_NAME);

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

    buflist_bar_item_end ();

    buflist_config_write ();
    buflist_config_free ();

    return WEECHAT_RC_OK;
}
